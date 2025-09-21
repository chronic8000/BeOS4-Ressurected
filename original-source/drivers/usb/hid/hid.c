/* hid.c
 *
 * A USB HID Device Class driver.  Currently, we only implement
 * the HID boot protocol for keyboard and mouse.
 *
 */

#include <SupportDefs.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <USB.h>
#include <string.h>
#include <malloc.h>

#include "hid.h"
#include "keyboard.h"
#include "mouse.h"

usb_module_info		*usb = NULL;
keyboard_device		*keyboard_list[MAX_USB_DEVICES];
mouse_device		*mouse_list[MAX_USB_DEVICES];
sem_id				keyboard_list_sem;
sem_id				mouse_list_sem;

static char			*publish_names[MAX_USB_DEVICES];

static int dump_devices(int argc, char **argv);

static status_t
add_device(const usb_device *device, const usb_configuration_info *ci,
		   const usb_interface_info *ii, int32 iindex)
{
	size_t len;
	
	if (usb->set_configuration(device, ci) != B_OK) {
		zprintf("error setting configuration\n");
	}

		
	if (ii->descr->interface_protocol == HID_KEYBOARD_PROTOCOL) {
		zprintf("adding keyboard\n");

		/* set protocol to boot protocol - XXX #defines for some of these zeroes */
		if (usb->send_request(device, USB_REQTYPE_CLASS|USB_REQTYPE_INTERFACE_OUT,
								HID_SET_PROTOCOL, 0, iindex, 0, NULL, 0, &len) != B_OK) {
			zprintf("error setting protocol to boot protocol\n");
		}

		if(keyboard_added((usb_device*)device, (usb_interface_info*)ii, iindex) == B_OK){
			return B_OK;
		}
	}
	
	if (ii->descr->interface_protocol == HID_MOUSE_PROTOCOL) {
		zprintf("adding mouse\n");
		if(mouse_added((usb_device*)device, (usb_interface_info*)ii, iindex) == B_OK){
			return B_OK;
		}
	}
	return B_ERROR;
}

static void
remove_device(usb_device *device)
{
	keyboard_removed(device);
	mouse_removed(device);
}

static status_t 
device_added(const usb_device *device, void **cookie)
{
	status_t res = B_ERROR;
	const usb_device_descriptor *dd;
	const usb_configuration_info *ci;
	const usb_interface_info *ii;
	int i, j;

	/* get the configuration */
	dd = usb->get_device_descriptor(device);
	
	for (i = 0; i < dd->num_configurations; i++) {
		ci = usb->get_nth_configuration(device, i);
		for (j = 0; j < ci->interface_count; j++) {
			ii = ci->interface[j].active;
			if (ii->descr->interface_class == HID_CLASS && 
				ii->descr->interface_subclass == HID_BOOT_SUBCLASS) {
				if(add_device(device, ci, ii, j) == B_OK){
					res = B_OK;
				}
			}
		}
	}
	if(res == B_OK){
		*cookie = device;
	}
	return res;
}

static status_t device_removed(void *cookie)
{
	remove_device((usb_device*)cookie);
	return B_OK;
}

static usb_notify_hooks notify_hooks = 
{
	&device_added,
	&device_removed
};

usb_support_descriptor supported_devices =
	{ HID_CLASS, 0, 0, 0, 0 };

status_t
init_driver(void)
{
	add_debugger_command("hiddump", dump_devices, "dump all USB keyboards and mice");

	memset(keyboard_list, 0, sizeof(keyboard_list));
	memset(mouse_list, 0, sizeof(mouse_list));
	memset(publish_names, 0, sizeof(publish_names));
	keyboard_list_sem = create_sem(1, "keyboard list sem");
	if (keyboard_list_sem < B_OK)
		goto bail;
	
	mouse_list_sem = create_sem(1, "mouse list sem");
	if (mouse_list_sem < B_OK)
		goto bail2;
			
	/* register as an interested party */
	if (get_module(B_USB_MODULE_NAME, (module_info **)&usb) < B_OK)
		goto bail3;

	if (usb->register_driver(DRIVER_NAME, &supported_devices, 1, NULL) < B_OK)
		goto bail4;

	if (usb->install_notify(DRIVER_NAME, &notify_hooks) < B_OK)
		goto bail4;
			
	return B_OK;

bail4:
	put_module(B_USB_MODULE_NAME);
bail3:
	delete_sem(mouse_list_sem);
bail2:
	delete_sem(keyboard_list_sem);
bail:
	remove_debugger_command("hiddump", dump_devices);
	return B_ERROR;
}

void
uninit_driver(void)
{
	int j;

	usb->uninstall_notify(DRIVER_NAME);
	put_module(B_USB_MODULE_NAME);

	for (j = 0; j < MAX_USB_DEVICES; j++) {
		if (keyboard_list[j])
			free_keyboard(keyboard_list[j]);
		if (mouse_list[j])
			free_mouse(mouse_list[j]);
		if (publish_names[j])
			free(publish_names[j]);
	}
	
	delete_sem(mouse_list_sem);
	delete_sem(keyboard_list_sem);

	remove_debugger_command("hiddump", dump_devices);
}


const char **
publish_devices(void)
{
	int i = 0, j;
	char *str;
	
	/* to avoid a race condition, we have to copy the device names
	 * before returning them.  since we can't free them after we've
	 * we've returned, we have to clean up now.
	 */
	for (j = 0; j < MAX_USB_DEVICES; j++) {
		if (publish_names[j]) {
			free(publish_names[j]);
			publish_names[j] = NULL;
		}
	}
	
	acquire_sem(keyboard_list_sem);
	for (j = 0; j < MAX_USB_DEVICES; j++) {
		if (keyboard_list[j] && keyboard_list[j]->present) {
			str = strdup(keyboard_list[j]->name);
			if (str) {
				publish_names[i++] = str;
				zprintf("publishing %s\n", keyboard_list[j]->name);
			}
		}
	}
	release_sem_etc(keyboard_list_sem, 1, B_DO_NOT_RESCHEDULE);

	acquire_sem(mouse_list_sem);
	for (j = 0; j < MAX_USB_DEVICES; j++) {
		if (mouse_list[j] && mouse_list[j]->present) {
			str = strdup(mouse_list[j]->name);
			if (str) {
				publish_names[i++] = str;
				zprintf("publishing %s\n", mouse_list[j]->name);
			}
		}
	}
	release_sem_etc(mouse_list_sem, 1, B_DO_NOT_RESCHEDULE);

	publish_names[i] = NULL;

zprintf("published %d devices\n", i);

	return (const char **)publish_names;
}

device_hooks *
find_device(const char *name)
{
	if (!strncmp(name, KEYBOARD_PREFIX, strlen(KEYBOARD_PREFIX)))
		return &keyboard_device_hooks;

	if (!strncmp(name, MOUSE_PREFIX, strlen(MOUSE_PREFIX)))
		return &mouse_device_hooks;

	return NULL;
}

// --------------------------------------------------------------------

#define boolstr(b)	((b) ? "TRUE" : "FALSE")

static int
dump_devices(int argc, char **argv)
{
	int i;
	
	if (keyboard_list[0] == NULL) {
		kprintf("No keyboards.\n");
	} else {
		kprintf("Keyboards:\n");
		for (i = 0; keyboard_list[i] != NULL; i++) {
			keyboard_device *kb = keyboard_list[i];
			kprintf("#%02d: name=%s, index=%ld\n", i, kb->name, kb->index);
			kprintf("     present=%s, lock=%ld\n", boolstr(kb->present), kb->lock);
			kprintf("     device=%p, int_endpoint=%p\n",
						kb->device, kb->int_endpoint);
			kprintf("     interface_index=%ld, packet_size=%ld\n",
						kb->interface_index, kb->packet_size);
			kprintf("     buffer=%p, sem=%ld\n", kb->buffer, kb->sem);
			kprintf("     repeat_delay=%Ld, repeat_interval=%Ld\n",
						kb->repeat_delay, kb->repeat_interval);
			kprintf("     control_alt_del_timeout=%Ld, reset_pending=%s\n",
						kb->control_alt_del_timeout, boolstr(kb->reset_pending));
			kprintf("     led_state=0x%lx\n", kb->led_state);
			if (keyboard_list[i+1] != NULL)
				kprintf("------------------------------------------------\n");
		}
	}

	if (mouse_list[0] == NULL) {
		kprintf("No mice.\n");
	} else {
		kprintf("Mice:\n");
		for (i = 0; mouse_list[i] != NULL; i++) {
			mouse_device *ms = mouse_list[i];
			kprintf("#%02d: name=%s, index=%ld\n", i, ms->name, ms->index);
			kprintf("     present=%s, lock=%ld\n", boolstr(ms->present), ms->lock);
			kprintf("     device=%p, int_endpoint=%p\n",
						ms->device, ms->int_endpoint);
			kprintf("     packet_size=%ld\n", ms->packet_size);
			kprintf("     buffer=%p, sem=%ld\n", ms->buffer, ms->sem);
			kprintf("     click_interval=%Ld, last_click=%Ld\n",
						ms->click_interval, ms->last_click);
			kprintf("     type=0x%x, click_count=%ld\n", ms->type, ms->click_count);
			if (mouse_list[i+1] != NULL)
				kprintf("------------------------------------------------\n");
		}
	}

	return B_OK;
}

