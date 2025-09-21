/************************************************************************/
/*                                                                      */
/*                          usb_speaker.c                              	*/
/*                                                                      */
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "speaker.h"

#define ddprintf //dprintf
#define DEBUG_ON //set_dprintf_enabled(1)
#define DEBUG_DRIVER 0

spkr_dev* pdev[DEV_MAX];
usb_module_info *usb;

static const char* spkr_name[DEV_MAX + 1];
static char* usb_name = B_USB_MODULE_NAME;
static benaphore dev_table_lock;

static device_hooks spkr_hooks = {
  old_open,
  old_close,
  old_free,
  old_control,
  old_read,
  old_write,
  NULL, NULL, NULL, NULL
};


status_t
init_ben(benaphore* b, char* name)
{
  b->atom = 0;
  b->sem = create_sem(0, name);
  return b->sem < 0 ? b->sem : B_OK;
}

status_t
acquire_ben(benaphore* b)
{
  int32 prev = atomic_add(&b->atom, 1);
  if (prev > 0)
	return acquire_sem(b->sem);
  return B_OK;
}

void
release_ben(benaphore* b)
{
  int32 prev = atomic_add(&b->atom, -1);
  if (prev > 1)
	release_sem(b->sem);
}

void
delete_ben(benaphore* b)
{
  delete_sem(b->sem);
}

status_t
lock_dev_table()
{
  return acquire_ben(&dev_table_lock);
}

void
unlock_dev_table()
{
  release_ben(&dev_table_lock);
}

static bool
valid_device(spkr_dev* p)
{
  int i;
  for (i = 0; i < DEV_MAX; i++)
	if (p == pdev[i])
	  break;
  return (p && i < DEV_MAX);
}

static const usb_interface_info*
find_interface(const usb_configuration_info *conf)
{
  usb_format_type_descr* ft;
  int ifc;
  int alt;
  int i;

  for (ifc = 0; ifc < conf->interface_count; ifc++)
	for (alt = 0; alt < conf->interface[ifc].alt_count; alt++) {
	  const usb_interface_info* info = &conf->interface[ifc].alt[alt];

	  /* does it have an AudioStreaming interface? */
	  if (info->descr->interface_class == AC_AUDIO
		  && info->descr->interface_subclass == AC_AUDIOSTREAMING
		  && info->endpoint_count == 1) {

		/* ignore input endpoints */
		if ((info->endpoint[0].descr->endpoint_address & 0x80) == 0) {

		  /* does it support 2 channel, 16 bit audio? */
		  for (i = 0; i < info->generic_count; i++) {
			ft = (usb_format_type_descr*) info->generic[i];
			if (ft->type == AC_CS_INTERFACE
				&& ft->subtype == AC_FORMAT_TYPE
				&& ft->length >= sizeof(usb_format_type_descr)
				&& ft->num_channels == 2
				&& ft->subframe_size == 2
				&& ft->sample_freq_type == 0) {
			  ddprintf(ID "found a 2x16 isoc output (ifc=%d, alt=%d, mp=%d)\n",
					   ifc, alt, info->endpoint[0].descr->max_packet_size);
			  return info;
			}
		  }
		}
	  }
	}

  dprintf(ID "suitable interface not found.\n");
  return NULL;
}

static int32
find_volume_control(const usb_configuration_info *conf)
{
  usb_feature_unit_descr* fu;
  int ifc;
  int alt;
  int i;

  for (ifc = 0; ifc < conf->interface_count; ifc++)
	for (alt = 0; alt < conf->interface[ifc].alt_count; alt++) {
	  const usb_interface_info* info = &conf->interface[ifc].alt[alt];

	  /* does it have an AudioControl interface? */
	  if (info->descr->interface_class == AC_AUDIO)
		if (info->descr->interface_subclass == AC_AUDIOCONTROL)
		  /* does it have a volume control? */
		  for (i = 0; i < info->generic_count; i++) {
			fu = (usb_feature_unit_descr*) info->generic[i];
			if (fu->type == AC_CS_INTERFACE)
			  if (fu->subtype == AC_FEATURE_UNIT) {
				int ch1 = fu->controls[1];
				int ch2 = fu->controls[2];
				if (fu->control_size != 2)
				  if (fu->control_size == 4) {
					ch1 = ((int32*) fu->controls)[1];
					ch1 = ((int32*) fu->controls)[2];
				  }
				  else
					continue;
				if (ch1 & (1 << (AC_VOLUME_CONTROL - 1)))
				  if (ch2 & (1 << (AC_VOLUME_CONTROL - 1)))
					return ifc + ((uint32) fu->unit_id << 8);
			  }
		  }
	}

  return -1;
}

volume_union
vc_get(spkr_dev* p, uint8 request)
{
  if (p->vc.index >= 0)
	usb->send_request(p->usbdev,
					  USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
					  request, 0xff + (AC_VOLUME_CONTROL << 8),
					  p->vc.index,
					  sizeof(p->vc.volume), &p->vc.volume,
					  NULL);
  return p->vc.volume;
}

status_t
vc_set(spkr_dev* p, float left, float right)
{
  status_t err = B_OK;
  p->vc.volume.i16[0] = left*p->vc.max.i16[0] + (1-left)*p->vc.min.i16[0];
  p->vc.volume.i16[1] = right*p->vc.max.i16[1] + (1-right)*p->vc.min.i16[1];
  if (p->vc.index >= 0)
	err = usb->queue_request(p->usbdev,
							 USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
							 AC_SET_CUR, 0xff + (AC_VOLUME_CONTROL << 8),
							 p->vc.index,
							 sizeof(p->vc.volume), &p->vc.volume,
							 NULL, NULL);
  return err;
}

status_t
vc_mute(spkr_dev* p, bool mute)
{
  status_t err = B_OK;
  p->vc.mute = mute ? -1 : 0;
  if (p->vc.index >= 0)
	err = usb->queue_request(p->usbdev,
							 USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
							 AC_SET_CUR, 0xff + (AC_MUTE_CONTROL << 8),
							 p->vc.index,
							 sizeof(p->vc.mute), &p->vc.mute,
							 NULL, NULL);
  return err;
}

static status_t
init_device(spkr_dev* p, usb_device dev,
			const usb_configuration_info *conf)
{
  const usb_interface_info* info = find_interface(conf);
  status_t err;

  if (!info)
	return B_ERROR;

  err = usb->set_alt_interface(dev, info);
  if(err != B_OK) {
	dprintf(ID "set_alt_interface() returns %x\n", err);
	return err;
  }

  err = usb->set_configuration(dev, conf);
  if(err != B_OK) {
	dprintf(ID "set_configuration() returns %x\n", err);
	return err;
  }

  p->audio_out.pipe = info->endpoint[0].handle;
#if 0 
  err = usb->set_pipe_policy(p->audio_out.pipe, NB, TS, SAMPLE_SIZE);
  if(err != B_OK) {
	dprintf("set_pipe_policy(out) returns %x\n", err);
	return err;
  }
#endif

  ddprintf(ID "added %ld / ld (/dev/audio/old/usb_speaker/%d)\n",
		   dev, p->audio_out.pipe, p->n);

  p->usbdev = dev;
  p->conf = conf;
  p->connected = true;
  p->vc.index = find_volume_control(conf);
  p->vc.min = vc_get(p, AC_GET_MIN);
  p->vc.max = vc_get(p, AC_GET_MAX);
  if (p->vc.index >= 0)
	ddprintf(ID "volume control index = %x\n", p->vc.index);

  return B_OK;
}

static spkr_dev*
add_device(usb_device dev)
{
  const usb_configuration_info* conf = usb->get_nth_configuration(dev, 0);
  spkr_dev* p;
  int i;

  ddprintf(ID "add_device(%p, %p)\n", dev, conf);

  if (!conf) {
	dprintf(ID "device has no config 0\n");
	return NULL;
  }

  // look for disconnected device first
  for (i = 0; i < DEV_MAX; i++)
	if (pdev[i] && !pdev[i]->connected)
	  break;

  if (i >= DEV_MAX)
	for (i = 0; i < DEV_MAX; i++)
	  if (!pdev[i])
		break;

  if (i >= DEV_MAX) {
	dprintf(ID "device table full\n");
	return NULL;
  }

  p = pdev[i];
  if (!p) {
	p = (spkr_dev*) calloc(1, sizeof(*p));
	if (p)
	  p->n = i;
	else
	  return NULL;
  }

  if (init_device(p, dev, conf) != B_OK) {
	if (!pdev[i])
	  free(p);
	return NULL;
  }

  if (pdev[i])		// restarting?
	start_audio(p);
  else
	pdev[i] = p;

  return p;
}

static void
remove_device(spkr_dev* p)
{
  ddprintf(ID "removed 0x%08x (/dev/audio/old/usb_speaker/%d)\n", p, p->n);
  pdev[p->n] = NULL;
  free(p);
  ddprintf("Done remove_device(%p)\n", p);
}

static status_t
device_added(usb_device dev, void **cookie)
{
  spkr_dev* p;

  ddprintf(ID "device_added(%p, %p)\n", dev, cookie);
  lock_dev_table();
  p = add_device(dev);
  unlock_dev_table();
  if (p) {
	*cookie = p;
	return B_OK;
  }

  ddprintf(ID "device_added(): failed.\n");
  return B_ERROR;
}

static status_t
device_removed(void *cookie)
{
  spkr_dev* p = (spkr_dev*) cookie;

  ddprintf(ID "device_removed(%p)\n", cookie);

  lock_dev_table();
  p->connected = false;
  stop_audio(p);
  //usb->set_configuration(p->usbdev, NULL);
  if(p->open_count == 0)
	remove_device(p);
  else
	ddprintf(ID "device /dev/audio/old/usb_speaker/%d still open"
			 "-- marked for removal\n", p->n);
  unlock_dev_table();

  ddprintf("Done device_removed(%p)\n", cookie);
  return B_OK;
}

static usb_notify_hooks notify_hooks =
{
  &device_added,
  &device_removed
};

_EXPORT
status_t
init_hardware(void)
{
  DEBUG_ON;
  ddprintf(ID "### init_hardware()\n");
  return B_OK;
}                   

usb_support_descriptor supported_devices[] =
{
  {AC_AUDIO, 0, 0, 0, 0}
};

_EXPORT
status_t
init_driver(void)
{
  int i;
  DEBUG_ON;
  ddprintf(ID "### init_driver(): %s %s\n", __DATE__, __TIME__);

#if DEBUG_DRIVER  
  if(load_driver_symbols(CHIP_NAME) == B_OK)
	ddprintf(ID "loaded symbols\n");
  else
	ddprintf(ID "no symbols for you!\n");
#endif

  if (get_module(usb_name,(module_info**) &usb) != B_OK) {
	dprintf(ID "cannot get module \"%s\"\n",usb_name);
	return B_ERROR;
  }

  if (init_ben(&dev_table_lock, CHIP_NAME ":devtab_lock") < 0) {
	put_module(usb_name);
	return dev_table_lock.sem;
  }
  for (i = 0; i < DEV_MAX; i++)
	pdev[i] = NULL;
  spkr_name[0] = NULL;

  usb->register_driver(CHIP_NAME, supported_devices, 1, NULL);
  usb->install_notify(CHIP_NAME, &notify_hooks);
  ddprintf(ID "init_driver() done\n");
  return B_OK;

err:
  put_module(usb_name);
  return B_ERROR;
}

_EXPORT
void
uninit_driver(void)
{
  int i;

  ddprintf(ID "### uninit_driver()\n");

  usb->uninstall_notify(CHIP_NAME);
  delete_ben(&dev_table_lock);
  put_module(usb_name);
  for (i = 0; spkr_name[i]; i++)
	free((char*) spkr_name[i]);
  ddprintf(ID "uninit_driver() done\n");
}

_EXPORT
const char**
publish_devices()
{
  int i, j, k;
  ddprintf(ID "publish_devices()\n");

  for (i = 0; spkr_name[i]; i++)
	free((char*) spkr_name[i]);
	
  lock_dev_table();
  for (i = 0, j = 0; i < DEV_MAX; i++)
	if (pdev[i])
	  if (pdev[i]->connected || pdev[i]->open_count)
		if (spkr_name[j] = (char *) malloc(32)) {
		  sprintf((char*)spkr_name[j], "audio/old/" CHIP_NAME "/%d", i + 1);
		  ddprintf(ID "publishing %s\n", spkr_name[j]);
		  j++;
		}
  spkr_name[j] = NULL;
  unlock_dev_table();

  return spkr_name;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
  ddprintf(ID "find_device(%s)\n", name);
  return &spkr_hooks;
}

status_t
spkr_open(const char *dname, int i)
{	
  ddprintf(ID "opening \"%s\"\n",dname);

  if (i >= 0 && i < DEV_MAX && pdev[i])
	if(!pdev[i]->connected)
	  dprintf(ID "device is going offline. cannot open\n");
	else {
	  ++pdev[i]->open_count;
	  return B_OK;
	}

  return ENODEV;
}

status_t
spkr_close(void *cookie)
{
  spkr_dev* p = (spkr_dev*) cookie;

  ddprintf("spkr_close(%p): %d\n", p, p->open_count);
  if (!valid_device(p))
	return B_ERROR;
  --p->open_count;
  ddprintf("spkr_close(%p): done.\n", p);
  return B_OK;
}

status_t
spkr_free(void *cookie)
{
  spkr_dev* p = (spkr_dev*) cookie;
  ddprintf("spkr_free(%p): %d\n", p, p->open_count);
  if (!valid_device(p))
	return B_ERROR;
  if (p->open_count == 0 && !p->connected)
	remove_device(p);
  return B_OK;
}
