#include <ByteOrder.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>

#include <driver_settings.h>
#include <driver_settings_p.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if USE_DMALLOC
	#include <dmalloc.h>
#endif

#include <1394.h>

#include "list.h"

#define TOUCH(x) ((void)(x))

#define PREFIX "\033[34m1394 driver:\033[0m "
#define DPRINTF(a, b) \
		do { \
			if (debug_level >= a) \
				dprintf(PREFIX), dprintf b; \
		} while (0)

int debug_level = 0;

#define ASSERT(c) \
		(((debug_level > 0) && !(c)) ? _assert_(__FILE__,__LINE__,#c) : 0)
int _assert_(char *a, int b, char *c)
{
	kprintf("tripped assertion in %s/%d (%s)\n", a, b, c);
	kernel_debugger("tripped assertion");
	return 0;
}

static bool
safe_mode_enabled(void)
{
	void *handle;
	bool result;

	handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	result = get_driver_boolean_parameter(handle, B_SAFEMODE_SAFE_MODE,
			false, true);
	unload_driver_settings(handle);

	return result;
}


static int num_busses;
static void **busses;

static struct _1394_module_info *_1394;

/*
   NTSC:

   30000 / 1001 frames / second
   1 / 8000 seconds / packet without delays
   250 packets / frame
   250 / 8000 seconds / frame without delays
   1001 / 30000 seconds / frame with delays

   transmission_time += (1 / 8000 = 1875 / 15000000)
   ideal_transmission_time += (1001 / (30000 * 250) = 2002 / 15000000)
   2002 - 1875 = 127
   4096 * 1875 = 7680000 == 15000000 / 2

   PAL:

   25 frames / second
   1 / 8000 seconds / packet without delays
   300 packets / frame
   300 / 8000 seconds / frame without delays
   1 / 25 seconds / frame with delays

   transmission_time += (1 / 8000 = 15 / 120000)
   ideal_transmission_time += (1 / (25 * 300) = 16 / 120000)
   16 - 15 = 1
*/

/* 450 us = 3 cycles = 0x3000 */
static status_t
dv_write(void *bus, struct ieee1394_dvwrite *dv)
{
	status_t err;
	int i, index;
	struct ieee1394_iio_buffer *iio;
	struct iovec *vecs;
	uint32 packets, fraction_increment, fraction_threshold, y;

	iio = malloc(sizeof(*iio) * (300 + 25));
	if (!iio)
		return ENOMEM;
	vecs = malloc(sizeof(*vecs) * 2 * (300 + 25));
	if (!vecs) {
		err = ENOMEM;
		goto err1;
	}

	if (dv->pal) {
		packets = 300;
		fraction_increment = 1;
		fraction_threshold = 15;
		y = 0x80800000;
	} else {
		packets = 250;
		fraction_increment = 127;
		fraction_threshold = 1875;
		y = 0x80000000;
	}

	i = 0;
	for (index=0;index<packets;index++) {
		if (dv->state.fraction >= fraction_threshold) {
			dv->state.fraction -= fraction_threshold;

			/* XXX: should patch in self node id */
			dv->headers[2*i+0] = B_HOST_TO_BENDIAN_INT32(0x00000000 | (dv->state.seq & 0xff));
			dv->headers[2*i+1] = B_HOST_TO_BENDIAN_INT32(y | 0xffff);
			
			iio[i].hdr = NULL;
			vecs[2*i+0].iov_len = 8;
			vecs[2*i+0].iov_base = dv->headers + 2 * i;
			iio[i].veclen = 1;
			iio[i].vec = vecs + 2 * i;
			iio[i].tagmask = B_1394_ISOCHRONOUS_TAG_01;
			iio[i].sync = 0;
			iio[i].sem = -1;
			iio[i].start_event.type = B_1394_ISOC_EVENT_NOW;

			i++;
		}

		dv->state.fraction += fraction_increment;

		/* XXX: should patch in self node id */
		dv->headers[2*i+0] = B_HOST_TO_BENDIAN_INT32(0x00780000 | (dv->state.seq++ & 0xff));
		dv->headers[2*i+1] = B_HOST_TO_BENDIAN_INT32(y | 0xffff);

		iio[i].hdr = NULL;
		vecs[2*i+0].iov_len = 8;
		vecs[2*i+0].iov_base = dv->headers + 2 * i;
		vecs[2*i+1].iov_len = 480;
		vecs[2*i+1].iov_base = dv->buffer + index * 480;
		iio[i].veclen = 2;
		iio[i].vec = vecs + 2 * i;
		iio[i].tagmask = B_1394_ISOCHRONOUS_TAG_01;
		iio[i].sync = 0;
		iio[i].sem = (index == packets - 1) ? dv->sem : -1;
		iio[i].start_event.type = B_1394_ISOC_EVENT_NOW;

		i++;
	}

	dv->headers[2*0+1] = B_HOST_TO_BENDIAN_INT32( \
			y | (dv->state.cycle_time & 0xffff));

	err = _1394->queue_isochronous(
			bus, dv->port, B_1394_ISOCHRONOUS_SINGLE_SHOT | 
			(dv->state.cycle_time ? 0 : B_1394_ISOCHRONOUS_FILL_CYCLE_TIME),
			&dv->state.cycle_time, 0x3f, 0, i, iio);

	dv->state.cycle_time |= 1;
	dv->state.cycle_time += i * 0x1000;
	if ((dv->state.cycle_time & 0x01fff000) >= 8000 * 0x1000)
		dv->state.cycle_time -= 8000 * 0x1000;

//err2:
	free(vecs);
err1:
	free(iio);

	return err;
}

/* Driver hooks */

static char **_1394_names = NULL;

struct open_cookie {
	void *bus;

	struct list *locked_memory;
	struct list *bus_reset_notifications;
	struct list *isochronous_ports;
	struct list *acquired_guids;
};

static status_t
_1394_open(const char *name, ulong flags, void **_cookie)
{
	int i;
	struct open_cookie *cookie;
	void *bus;

	TOUCH(flags);

	bus = NULL;

	for (i=0;i<num_busses;i++) {
		if (!strcmp(name, _1394_names[i])) {
			bus = busses[i];
			break;
		}
	}

	if (!bus)
		return ENOENT;

	cookie = calloc(sizeof(*cookie), 1);
	if (!cookie)
		return ENOMEM;

	cookie->bus = bus;
	*_cookie = (void *)cookie;

	return B_OK;
}

static status_t
_1394_close(void *cookie)
{
	TOUCH(cookie);
	return B_OK;
}

static status_t
_1394_free(void *_cookie)
{
	struct open_cookie *cookie = (struct open_cookie *)_cookie;
	struct list *l;

	DPRINTF(2, ("Releasing isochronous ports\n"));
	for (l=cookie->isochronous_ports;l;l=l->next) {
		DPRINTF(1, ("Releasing port %lx\n", *(uint32 *)(l->data)));
		_1394->release_isochronous_port(cookie->bus, *(uint32 *)(l->data));
	}
	list_free(&cookie->isochronous_ports);

	DPRINTF(2, ("Unregistering bus reset notifications\n"));
	for (l=cookie->bus_reset_notifications;l;l=l->next) {
		sem_id sem = *(sem_id *)l->data;
		DPRINTF(1, ("Unregistering bus reset notification %lx\n", sem));
		_1394->unregister_bus_reset_notification(cookie->bus, sem);
	}
	list_free(&cookie->bus_reset_notifications);

	DPRINTF(2, ("Deleting locked areas\n"));
	for (l=cookie->locked_memory;l;l=l->next) {
		struct ieee1394_area *area = (struct ieee1394_area *)l->data;
		DPRINTF(1, ("Deleting locked area %lx\n", area->aid));
		unlock_memory(area->addr, area->size, B_DMA_IO | B_READ_DEVICE);
		delete_area(area->aid);
	}
	list_free(&cookie->locked_memory);

	DPRINTF(2, ("Releasing acquired GUIDs\n"));
	for (l=cookie->acquired_guids;l;l=l->next) {
		DPRINTF(1, ("Releasing GUID %Lx\n", *(uint64 *)(l->data)));
		_1394->release_guid(cookie->bus, *(uint64 *)(l->data));
	}
	list_free(&cookie->acquired_guids);

	free(cookie);

	return B_OK;
}

static status_t
_1394_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	TOUCH(cookie); TOUCH(pos); TOUCH(buf); TOUCH(count);
	return 0;
}


static status_t
_1394_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	TOUCH(cookie); TOUCH(pos); TOUCH(buf); TOUCH(count);
	return 0;
}

static status_t
_1394_ioctl(void *_cookie, ulong cmd, void *buf, size_t len)
{
	struct open_cookie *cookie = (struct open_cookie *)_cookie;
	void *bus = cookie->bus;
	struct ieee1394_io *io = (struct ieee1394_io *)buf;
	struct ieee1394_iio *iio = (struct ieee1394_iio *)buf;
	struct ieee1394_area *area = (struct ieee1394_area *)buf;
	struct ieee1394_istop *stop = (struct ieee1394_istop *)buf;
	struct ieee1394_fcp *fcp = (struct ieee1394_fcp *)buf;
	uint32 *arg = (uint32 *)buf;
	struct list *l;
	status_t err;

	TOUCH(len);

	if (!buf && (cmd != B_1394_RESET_BUS)) return EINVAL;

	switch (cmd) {
		case B_1394_GET_BUS_INFO :
			return _1394->get_bus_info(bus,
						(struct ieee1394_bus_info *)buf);

		case B_1394_GET_NODE_INFO :
		{
			struct ieee1394_node_info *ninfo = (struct ieee1394_node_info *)buf;

			return _1394->get_node_info(bus, ninfo->guid, ninfo);

#if 0
			i = (ninfo->config_rom[0] >> 16) & 0xff;
			if (	(ninfo->config_rom[0] & 0xffff) !=
					CRC16(ninfo->config_rom + 1, i)) {
				DPRINTF(-1, ("CRC error in configuration rom\n"));
				return EINVAL;
			}
#endif
		}

		case B_1394_GET_CYCLE_TIME :
			return _1394->get_cycle_time(bus, (uint32 *)buf);
	
		case B_1394_RESET_BUS :
			return _1394->reset_bus(bus, TRUE);

		case B_1394_REGISTER_BUS_RESET_NOTIFICATION :
		{
			sem_id sem = *(sem_id *)buf;
			err = _1394->register_bus_reset_notification(bus, sem);
			if (!err) {
				DPRINTF(2, ("adding notification %lx\n", sem));
				l = list_alloc(sizeof(sem), free);
				if (!l) {
					_1394->unregister_bus_reset_notification(bus, sem);
					return ENOMEM;
				}
				memcpy(l->data, &sem, sizeof(sem));
				list_append(&cookie->bus_reset_notifications, l);
				DPRINTF(2, ("added notification %lx\n", sem));
			}
			return err;
		}

		case B_1394_UNREGISTER_BUS_RESET_NOTIFICATION :
		{
			sem_id sem = *(sem_id *)buf;
			err = _1394->unregister_bus_reset_notification(bus, sem);
			if (!err) {
				DPRINTF(2, ("removing reset %lx\n", sem));
				l = list_find(cookie->bus_reset_notifications, memcmp, &sem, sizeof(sem));
				ASSERT(l);
				if (l) {
					list_remove_item(l, &cookie->bus_reset_notifications);
					list_free(&l);
					DPRINTF(2, ("removed reset %lx\n", sem));
				}
			}
			return err;
		}

		case B_1394_QUADLET_READ :
			return _1394->quadlet_read(bus, B_1394_SPEED_MAX_MBITS,
					io->guid, io->offset, &io->x.quadlet, io->timeout);
		
		case B_1394_BLOCK_READ :
			return _1394->block_read(bus, B_1394_SPEED_MAX_MBITS,
					io->guid, io->offset, io->x.block.veclen, io->x.block.vec,
					io->timeout);
		
		case B_1394_QUADLET_WRITE :
			return _1394->quadlet_write(bus, B_1394_SPEED_MAX_MBITS,
					io->guid, io->offset, io->x.quadlet, io->timeout);
		
		case B_1394_BLOCK_WRITE :
			return _1394->block_write(bus, B_1394_SPEED_MAX_MBITS,
					io->guid, io->offset, io->x.block.veclen, io->x.block.vec,
					io->timeout);

		case B_1394_LOCK32 :
			return _1394->lock_32(bus, B_1394_SPEED_MAX_MBITS,
					io->guid, io->offset, io->x.lock.extended_tcode,
					io->x.lock.arg, io->x.lock.data, io->timeout);

		case B_1394_SEND_PHY_PACKET :
			return _1394->send_phy_packet(bus, io->x.phy_packet, io->timeout);

		case B_1394_INIT_FCP_CONTEXT :
			return _1394->init_fcp_context(bus, fcp->guid);

		case B_1394_UNINIT_FCP_CONTEXT :
			return _1394->uninit_fcp_context(bus, fcp->guid);

		case B_1394_SEND_FCP_COMMAND :
			return _1394->send_fcp_command(bus, fcp->guid,
					fcp->cmd, fcp->cmd_len, fcp->rsp, fcp->rsp_len,
					fcp->timeout);

		case B_1394_WAIT_FOR_FCP_RESPONSE :
			return _1394->wait_for_fcp_response(bus, fcp->guid,
					fcp->rsp, fcp->rsp_len, fcp->timeout);

		case B_1394_QUEUE_ISOCHRONOUS :
			return _1394->queue_isochronous(bus, iio->port,
					iio->flags, NULL, iio->channel, iio->speed,
					iio->num_buffers, iio->buffers);

		case B_1394_STOP_ISOCHRONOUS :
			return _1394->stop_isochronous(bus, stop);

		case B_1394_ACQUIRE_ISOCHRONOUS_PORT :
			err = _1394->acquire_isochronous_port(bus, *arg);
			if (err < 0)
				return err;
			*arg = err;

			DPRINTF(2, ("adding iport\n"));
			l = list_alloc(sizeof(uint32), free);
			if (!l) {
				_1394->release_isochronous_port(bus, *arg);
				return ENOMEM;
			}
			memcpy(l->data, arg, sizeof(uint32));
			list_append(&cookie->isochronous_ports, l);
			DPRINTF(2, ("added iport %lx\n", *arg));

			return B_OK;

		case B_1394_RELEASE_ISOCHRONOUS_PORT :
			err = _1394->release_isochronous_port(bus, *arg);

			if (!err) {
				DPRINTF(2, ("releasing iport %lx\n", *arg));
				l = list_find(cookie->isochronous_ports, memcmp, arg, sizeof(uint32));
				ASSERT(l);
				if (l) {
					list_remove_item(l, &cookie->isochronous_ports);
					list_free(&l);
					DPRINTF(2, ("released iport %lx\n", *arg));
				}
			}

			return B_OK;

		case B_1394_RESERVE_BANDWIDTH :
			return _1394->reserve_bandwidth(bus, *arg, 1000000LL);

		case B_1394_RESERVE_CHANNEL :
			return _1394->reserve_isochronous_channel(bus, *arg, 1000000LL);

		case B_1394_MALLOC_LOCKED :
			area->aid = create_area("1394 buffer", &area->addr,
					B_ANY_KERNEL_ADDRESS, area->size, area->flags,
					B_READ_AREA | B_WRITE_AREA);
			if (area->aid < 0) return area->aid;

			err = lock_memory(area->addr, area->size, B_DMA_IO | B_READ_DEVICE);
			if (err) {
				delete_area(area->aid);
			} else {
				DPRINTF(2, ("adding locked\n"));
				l = list_alloc(sizeof(*area), free);
				if (!l) {
					unlock_memory(area->addr, area->size, B_DMA_IO | B_READ_DEVICE);
					delete_area(area->aid);
					return ENOMEM;
				}
				memcpy(l->data, area, sizeof(*area));
				list_append(&cookie->locked_memory, l);
				DPRINTF(2, ("added locked %lx\n", area->aid));
			}
			return err;

		case B_1394_FREE_LOCKED :
			unlock_memory(area->addr, area->size, B_DMA_IO | B_READ_DEVICE);
			delete_area(area->aid);

			DPRINTF(2, ("removing locked %lx\n", area->aid));
			l = list_find(cookie->locked_memory, memcmp, area, sizeof(*area));
			if (l) {
				list_remove_item(l, &cookie->locked_memory);
				list_free(&l);
				DPRINTF(2, ("removed locked %lx\n", area->aid));
			}

			return B_OK;

		case B_1394_DV_WRITE :
			return dv_write(cookie->bus, (struct ieee1394_dvwrite *)buf);

		case B_1394_ACQUIRE_GUID :
		{
			uint64 guid = *(uint64 *)buf;
			DPRINTF(2, ("acquiring GUID %Lx\n", guid));
			err = _1394->acquire_guid(cookie->bus, guid);
			if (err < B_OK)
				return err;
			DPRINTF(2, ("adding acquired GUID\n"));
			l = list_alloc(sizeof(guid), free);
			if (!l) {
				_1394->release_guid(cookie->bus, guid);
				return ENOMEM;
			}
			memcpy(l->data, &guid, sizeof(guid));
			list_append(&cookie->acquired_guids, l);
			DPRINTF(2, ("added acquired %Lx\n", guid));
			return B_OK;
		}

		case B_1394_RELEASE_GUID :
		{
			uint64 guid = *(uint64 *)buf;
			DPRINTF(2, ("releasing GUID %Lx\n", guid));
			l = list_find(cookie->acquired_guids, memcmp, &guid, sizeof(guid));
			if (l) {
				list_remove_item(l, &cookie->acquired_guids);
				list_free(&l);
				_1394->release_guid(cookie->bus, guid);
				DPRINTF(2, ("removed released guid %Lx\n", guid));
			}
			return B_OK;
		}
	}

	return ENOSYS;
}

device_hooks	
_1394_device = {
	_1394_open,
	_1394_close,
	_1394_free,
	_1394_ioctl,
	_1394_read,
	_1394_write
};

status_t
init_driver (void)
{
	status_t err;
	int32 i;

	if (safe_mode_enabled()) {
		dprintf("Safe mode enabled. 1394 driver disabled.\n");
		return B_ERROR;
	}

	if (debug_level > 0)
		load_driver_symbols("1394");

	if ((err = get_module(B_1394_FOR_DRIVER_MODULE_NAME, (module_info **)&_1394)) < B_OK) {
		dprintf("unable to get 1394 module\n");
		return err;
	}

	err = _1394->get_bus_count();
	if (err < 0)
		return err;

	num_busses = err;

	_1394_names = calloc(num_busses + 1, sizeof(char *));
	if (!_1394_names)
		goto err1;

	busses = calloc(num_busses, sizeof(*busses));
	if (!busses)
		goto err2;

	for (i=0;i<num_busses;i++) {
		_1394_names[i] = malloc(sizeof("bus/1394/") + 16);
		if (!_1394_names[i])
			goto err3;
		sprintf(_1394_names[i], "bus/1394/%ld/raw", i);
		err = _1394->get_bus_cookie(i, busses + i);
		if (err < 0)
			goto err3;
	}

	return B_OK;

err3:
	free(busses);

	for (i=0;i<num_busses;i++)
		if (_1394_names[i])
			free(_1394_names[i]);
err2:
	free(_1394_names);
err1:
	num_busses = 0;
	busses = NULL;

	put_module(B_1394_FOR_DRIVER_MODULE_NAME);

	return (err < 0) ? err : ENOMEM;
}

void
uninit_driver(void)
{
	int i;

	free(busses);

	for (i=0;i<num_busses;i++)
		if (_1394_names[i])
			free(_1394_names[i]);
	free(_1394_names);

	num_busses = 0;
	busses = NULL;

	put_module(B_1394_FOR_DRIVER_MODULE_NAME);
}

const char **
publish_devices()
{
	return (const char **)_1394_names;
}

device_hooks *
find_device(const char *name)
{
	TOUCH(name);

	return &_1394_device;
}
