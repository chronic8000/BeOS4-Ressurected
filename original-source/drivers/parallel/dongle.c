#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>

#include <OS.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>

#include <parallel_driver.h>
#include "parallel.h"
#include "dongle.h"


static int lookup_device_name(const char *name) 
{
	if (strncmp(name, kDongleBaseName, strlen(kDongleBaseName)))
		return -1;
	return name[strlen(name) - 1] - '1';
}

status_t dev_dgl_open(const char *name, uint32 flags, void **cookie)
{
	int port_num;
	par_port_info *d;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_dgl_open(%s, %lu, %08lx)\n", name, flags, cookie));

	/* Check the access mode: RDWR only */
	if ((flags & O_ACCMODE) != O_RDWR)
		return EACCES;

	/* Check device name, select right par_port_info */
	port_num = lookup_device_name(name);
	if (port_num < 0)
		return EINVAL;
	d = pinfo + port_num;

	return do_dev_open(d, flags, cookie);
}

status_t dev_dgl_close(void *cookie)
{
	par_port_info *d = ((user_info *)cookie)->d;
	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_dgl_close\n"));
	if (d == NULL)
		return EINVAL;
	return B_OK;
}

status_t dev_dgl_free(void *cookie)
{
	return dev_free(cookie);
}

status_t dev_dgl_control(void *cookie, uint32 op, void *data, size_t len)
{
	par_port_info *d = ((user_info *)cookie)->d;

	DD(bug("parallel: ------------------------\n"));
	DD(bug("parallel: dev_dgl_control(%08lx, %08lx, %08lx, %ld)\n", cookie, op, data, len));

	switch (op)
	{
		case DGL_READ_STATUS_REGISTER:
			/* Return PSTAT register (no need to acquire hw_lock) */
			*(uint8 *)data = read_io_8(d->io_base + PSTAT) & 0xF8;
			return B_OK;

		case DGL_READ_CONTROL_REGISTER:
			/* Return PCON register (no need to acquire hw_lock) */
			*(uint8 *)data = read_io_8(d->io_base + PCON) & 0x3F;
			return B_OK;

		case DGL_WRITE_CONTROL_REGISTER:
			/* Write PCON register (no need to acquire hw_lock) */
			write_io_8(d->io_base + PCON, (*(uint8 *)data));
			return B_OK;

		case DGL_LOCK:
			/* Lock the parallel port */
			return acquire_sem_etc(d->hw_lock, 1, B_CAN_INTERRUPT, 0);

		case DGL_UNLOCK:
			/* Unock the parallel port */
			return release_sem(d->hw_lock);
	}
	return EINVAL;
}

status_t dev_dgl_read(void *cookie, off_t position, void *data, size_t *len)
{
	par_port_info *d = ((user_info *)cookie)->d;
	if (*len == 0)
		return B_OK;
	*(uint8 *)data = read_io_8(d->io_base + PDATA);
	*len = 1;
	return B_OK;
}

status_t dev_dgl_write(void *cookie, off_t position, const void *data, size_t *len)
{
	par_port_info *d = ((user_info *)cookie)->d;
	if (*len == 0)
		return B_OK;
	write_io_8(d->io_base + PDATA, (*(uint8 *)data));
	*len = 1;
	return B_OK;
}

