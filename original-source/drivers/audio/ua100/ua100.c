/************************************************************************/
/*                                                                      */
/*                              ua100.c                              	*/
/*                                                                      */
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "ua100.h"

#define ddprintf //dprintf
#define DEBUG_ON //set_dprintf_enabled(1)
#define DEBUG_DRIVER 0

#define	VENDOR_ID 0x0582
#define DEVICE_ID 0	

ua100dev* pdev[DEV_MAX];
usb_module_info *usb;

static const char* ua100_name[(CHAN_COUNT+1)*DEV_MAX+1];
static char* usb_name = B_USB_MODULE_NAME;
static sem_id dev_table_lock = -1;

static device_hooks ua100_hooks = {
  old_open,
  old_close,
  old_free,
  old_control,
  old_read,
  old_write,
  NULL, NULL, NULL, NULL
};

static device_hooks midi_hooks = {
  midi_open,
  midi_close,
  midi_free,
  midi_control,
  midi_read,
  midi_write,
  NULL, NULL, NULL, NULL
};


status_t
lock_dev_table()
{
  return acquire_sem(dev_table_lock);
}

void
unlock_dev_table()
{
  release_sem(dev_table_lock);
}

void
init_ben(benaphore* b, char* name)
{
  b->atom = 0;
  b->sem = create_sem(0, name);
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

static bool
valid_device(ua100dev* p)
{
  int i;
  for (i = 0; i < DEV_MAX; i++)
	if (p == pdev[i])
	  break;
  return (p && i < DEV_MAX);
}

static status_t
init_device(ua100dev* p,  usb_device dev,
			const usb_configuration_info *conf)
{
  status_t err;

  if (p == NULL)
	return B_NO_MEMORY;

  err = usb->set_alt_interface(dev, &conf->interface[0].alt[1]);
  if(err != B_OK) {
	dprintf("set_alt_interface(0) returns %x\n", err);
	return err;
  }

  err = usb->set_alt_interface(dev, &conf->interface[1].alt[1]);
  if(err != B_OK) {
	dprintf("set_alt_interface(1) returns %x\n", err);
	return err;
  }

#if MIDI_INTERRUPT
  err = usb->set_alt_interface(dev, &conf->interface[2].alt[1]);
  if(err != B_OK) {
	dprintf("set_alt_interface(2) returns %x\n", err);
	return err;
  }
#endif

  err = usb->set_configuration(dev, conf);
  if(err != B_OK) {
	dprintf("set_configuration() returns %x\n", err);
	return err;
  }

  p->audio_in.pipe = conf->interface[1].active->endpoint[0].handle;
  p->audio_out.pipe = conf->interface[0].active->endpoint[0].handle;
  p->midi_in = conf->interface[2].active->endpoint[1].handle;
  p->midi_out = conf->interface[2].active->endpoint[0].handle;

#if 0
  err = usb->set_pipe_policy(p->audio_in.pipe, NB, TS, 4);
  if(err != B_OK) {
	dprintf("set_pipe_policy(in) returns %x\n", err);
	return err;
  }
#endif

  err = usb->set_pipe_policy(p->audio_out.pipe, NB, TS, 8);
  if(err != B_OK) {
	dprintf("set_pipe_policy(out) returns %x\n", err);
	return err;
  }

  ddprintf(ID "added %p / %p /%p (/dev/audio/old/ua100/%d)\n",
		   dev, p->audio_in.pipe, p->audio_out.pipe, p->n);

  p->usbdev = dev;
  p->conf = conf;
  p->connected = true;
  return B_OK;
}

static ua100dev*
add_device(usb_device dev)
{
  const usb_configuration_info* conf = usb->get_nth_configuration(dev, 0);
  ua100dev* p;
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
	p = (ua100dev*) calloc(1, sizeof(*p));
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

  if (pdev[i])
	start_audio(p);
  else {
	pdev[i] = p;
	if (ENABLE_MIDI)
	  init_midi(p);
  }

  return p;
}

static void
remove_device(ua100dev* p)
{
  ddprintf(ID "removed 0x%08x (/dev/audio/old/ua100/%d)\n", p, p->n);
  if (ENABLE_MIDI)
	uninit_midi(p);
  pdev[p->n] = NULL;
  free(p);
  ddprintf("Done remove_device(%p)\n", p);
}

static status_t
device_added(usb_device dev, void **cookie)
{
  const usb_device_descriptor* desc = usb->get_device_descriptor(dev);
  ua100dev* p;

  ddprintf(ID "device_added(%p, %p)\n", dev, cookie);

  if (desc->vendor_id == VENDOR_ID)
	if (desc->product_id == DEVICE_ID) {
	  lock_dev_table();
	  p = add_device(dev);
	  unlock_dev_table();
	  if (p) {
		*cookie = p;
		return B_OK;
	  }
	}
  return B_ERROR;
}

static status_t
device_removed(void *cookie)
{
  ua100dev* p = (ua100dev*) cookie;

  ddprintf(ID "device_removed(%p)\n", cookie);

  lock_dev_table();
  p->connected = false;
  stop_audio(p);
  //usb->set_configuration(p->usbdev, NULL);
  if(p->open_count == 0)
	remove_device(p);
  else
	ddprintf(ID "device /dev/audio/old/ua100/%d still open"
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
  ddprintf(ID "init_hardware()\n");
  return B_OK;
}                   

usb_support_descriptor supported_devices[] =
{
  {0, 0, 0, VENDOR_ID, DEVICE_ID}
};

_EXPORT
status_t
init_driver(void)
{
  int i;
  DEBUG_ON;
  ddprintf(ID "init_driver(): %s %s\n", __DATE__, __TIME__);



  if (get_module(usb_name,(module_info**) &usb) != B_OK) {
	dprintf(ID "cannot get module \"%s\"\n",usb_name);
	return B_ERROR;
  }

  dev_table_lock = create_sem(1, CHIP_NAME ":devtab_lock");
  if (dev_table_lock < 0) {
	put_module(usb_name);
	return dev_table_lock;
  }
  for (i = 0; i < DEV_MAX; i++)
	pdev[i] = NULL;
  ua100_name[0] = NULL;

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

  ddprintf(ID "uninit_driver()\n");

  usb->uninstall_notify(CHIP_NAME);
  delete_sem(dev_table_lock);
  put_module(usb_name);
  for (i = 0; ua100_name[i]; i++)
	free((char*) ua100_name[i]);

  ddprintf(ID "uninit_driver() done\n");
}

_EXPORT
const char**
publish_devices()
{
  int i, j, k;
  ddprintf(ID "publish_devices()\n");

  for (i = 0; ua100_name[i]; i++)
	free((char*) ua100_name[i]);
	
  lock_dev_table();
  for (i = 0, j = 0; i < DEV_MAX; i++)
	if (pdev[i])
	  if (pdev[i]->connected || pdev[i]->open_count) {
		if (ua100_name[j] = (char *) malloc(24)) {
		  sprintf((char*)ua100_name[j], "audio/old/" CHIP_NAME "/%d", i + 1);
		  ddprintf(ID "publishing %s\n", ua100_name[j]);
		  j++;
		}
		if (ENABLE_MIDI)
		  for (k = 1; k <= CHAN_COUNT; k++)
			if (ua100_name[j] = (char *) malloc(24))
			  sprintf((char*)ua100_name[j++],
					  "midi/" CHIP_NAME "/%d/%d", i + 1, k);
	  }
  ua100_name[j] = NULL;
  unlock_dev_table();
  return ua100_name;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
  ddprintf(ID "find_device(%s)\n", name);
  return strstr(name, "midi") ? &midi_hooks : &ua100_hooks;
}

status_t
ua100_open(const char *dname, int i)
{	
  ddprintf(ID "opening \"%s\"\n",dname);

  if (i >= 0 && i < DEV_MAX && pdev[i])
	if(!pdev[i]->connected)
	  dprintf(ID "device is going offline. cannot open\n");
	else {
	  if (++pdev[i]->open_count == 1)
		if (ENABLE_MIDI)
		  start_midi(pdev[i]);
	  return B_OK;
	}

  return ENODEV;
}

status_t
ua100_close(void *cookie)
{
  ua100dev* p = (ua100dev*) cookie;

  ddprintf("ua100_close(%p): %d\n", p, p->open_count);
  if (!valid_device(p))
	return B_ERROR;
  if (--p->open_count == 0 && ENABLE_MIDI)
	stop_midi(p);
  ddprintf("ua100_close(%p): done.\n", p);
  return B_OK;
}

status_t
ua100_free(void *cookie)
{
  ua100dev* p = (ua100dev*) cookie;
  ddprintf("ua100_free(%p): %d\n", p, p->open_count);
  if (!valid_device(p))
	return B_ERROR;
  if (p->open_count == 0 && !p->connected)
	remove_device(p);
  return B_OK;
}
