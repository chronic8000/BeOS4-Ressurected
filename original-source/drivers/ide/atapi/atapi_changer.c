#include "atapi_changer.h"
#include "atapi_cmds.h"
#include "atapi.h"
#include <debugoutput.h>

#include <KernelExport.h>
#include <malloc.h>

typedef struct atapi_mechanism_status {
	atapi_mechanism_status_header header;
	atapi_slot_info slot[256+61];
	// some drives ignore MSB of allocation length, so we allocate 61
	// extra slots to make the LSB 0xfc.
} atapi_mechanism_status;

status_t
load_cd(device_handle *h)
{
	status_t err;
	
	if(h->d->maxlun == 0)
		return B_NO_ERROR;
		
	if(h->d->priv->current_lun == h->lun)
		return B_NO_ERROR;

	err = load_unload_cd(h, h->lun, true /*load*/);

	if(err == B_NO_ERROR)
		h->d->priv->current_lun = h->lun;
	else
		h->d->priv->current_lun = -1;

	if(err == B_NO_ERROR) {
		DF(2) dprintf("IDE ATAPI -- load_cd: loaded cd in slot %lu\n", h->lun);
	}
	else {
		DF(2) dprintf("IDE ATAPI -- load_cd: could not load cd in slot %lu\n",
		              h->lun);
	}
	return err;
}

static void
print_atapi_mechanism_status(atapi_mechanism_status *m)
{
	const char *debugprefix = "IDE ATAPI -- print_atapi_mechanism_status:";
	dprintf("%s changer has %d slots\n",
	        debugprefix, m->header.number_of_slots);
	dprintf("%s current slot is %d\n", debugprefix, m->header.current_slot);
	dprintf("%s changer state = %d\n", debugprefix, m->header.changer_state);
	dprintf("%s fault = %d\n", debugprefix, m->header.fault);
	dprintf("%s cd mechanism state = %d\n", debugprefix,
	        m->header.cd_mechanism_state);

//	dprintf("stsize 0x%02x\n", B_BENDIAN_TO_HOST_INT16(
//	        m->header.slot_table_size));
}

status_t
check_current_slot(device_handle *h)
{
	const char *debugprefix = "IDE ATAPI -- check_current_slot:";
	status_t err;
	atapi_mechanism_status *ms = h->d->priv->ms;

	if(!h->d->priv->Support_Disc_Present)
		return B_ERROR;
	
	if(ms == NULL) {
		DE(1) dprintf("%s changer was not configured\n", debugprefix);
		return B_ERROR;
	}

	err = mechanism_status(h, ms, sizeof(atapi_mechanism_status));

	if(err != B_NO_ERROR)
		goto err;

	if(h->priv->li != NULL) { // lun valid
		if(ms->slot[h->lun].disc_present) {
			DI(2) dprintf("%s disc present in slot %ld\n", debugprefix, h->lun);
			err = B_NO_ERROR;
		}
		else {
			DI(2) dprintf("%s no disc in slot %ld\n", debugprefix, h->lun );
			err = B_DEV_NO_MEDIA;
		}
	}
	else {
		err = B_ERROR;
	}
err:
	return err;
}

status_t
reconfigure_changer(device_entry *d)
{
	const char *debugprefix = "IDE ATAPI -- reconfigure_changer:";
	status_t err;
	uint32 lun;
	atapi_mechanism_status *ms = d->priv->ms;
	device_handle h;
	driver_device_handle hpriv;
	
	h.next = NULL;
	h.name = "reconfigure_changer temp handle";
	h.d = d;
	h.lun = -1; // global commands only
	h.status = B_NO_ERROR;
	h.priv = &hpriv;
	h.priv->li = NULL;
	h.priv->command_in_progress = true;
	
	if(ms == NULL) {
		DE(1) dprintf("%s changer was not configured\n", debugprefix);
		return B_ERROR;
	}
	
	err = mechanism_status(&h, ms, sizeof(atapi_mechanism_status));

	if(err != B_NO_ERROR)
		goto err;

	//print_atapi_mechanism_status(ms);

	for(lun=0; lun <= d->maxlun; lun++) {
		DI(2) dprintf("%s slot %ld -- %s%s\n", debugprefix, lun,
		              ms->slot[lun].disc_present ? "disc present" : "no disc",
		              ms->slot[lun].change ? " (changed)" : "" );
	}
	for(lun=0; lun <= d->maxlun; lun++) {
		if(ms->slot[lun].change) {
			err = media_changed_in_lun(d, lun);
			if(err != B_NO_ERROR) {
				DE(1) dprintf("%s fatal error, "
				              "could not update media changed\n", debugprefix);
				goto err;
			}
		}
	}
err:
	return err;
}

status_t
configure_changer(device_entry *d)
{
	const char *debugprefix = "IDE ATAPI -- configure_changer:";
	status_t err;
	atapi_mechanism_status *ms;
	device_handle h;
	driver_device_handle hpriv;
	
	h.next = NULL;
	h.name = "temp handle";
	h.d = d;
	h.lun = -1; // global commands only
	h.status = B_NO_ERROR;
	h.priv = &hpriv;
	h.priv->command_in_progress = false;
	h.priv->li = NULL;
	
//	memset(&atapi_mechanism_status, 0xff, sizeof(atapi_mechanism_status));
	d->priv->ms = ms = malloc(sizeof(atapi_mechanism_status));
	if(ms == NULL)
		return B_NO_MEMORY;
	
	err = mechanism_status(&h, ms, sizeof(atapi_mechanism_status));
	if(err == B_NO_ERROR) {
		int i;
		d->maxlun = ms->header.number_of_slots - 1;
		DI(2) print_atapi_mechanism_status(ms);
		for(i=0; i < ms->header.number_of_slots; i++) {
			if(ms->slot[i].disc_present) {
				DI(2) dprintf("%s disc present in slot %d\n", debugprefix, i);
			}
			else {
				DI(2) dprintf("%s no disc in slot %d\n", debugprefix, i);
			}
			if(ms->slot[i].change) {
				DI(2) dprintf("%s disc in slot %d has changed\n",
				              debugprefix, i);
			}
		}
	}
	else {
		DE(2) dprintf("%s could not get changer information\n", debugprefix);
	}
	return err;
}
