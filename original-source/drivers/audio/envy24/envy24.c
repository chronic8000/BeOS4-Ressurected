/* Copyright 2001, Be Incorporated. All Rights Reserved */
/*includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>

#include "multi_audio.h"
#include "envy24.h"
#include "eeprom.h"
#include "pci_helpers.h"

#if THIRD_PARTY 
#include "thirdparty.h"
#endif

#if defined (__POWERPC__)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);

extern device_hooks envy24_hooks;


/* globals ************/
int card_count 		= 0;
envy24_dev cards[MAX_ENVY24_BD];


static char * init_names[MAX_ENVY24_BD+1] = {
	"audio/multi/envy24/1",
	"audio/multi/envy24/2",
	"audio/multi/envy24/3",
	"audio/multi/envy24/4",
	NULL
};
static char * names[MAX_ENVY24_BD+1];

pci_module_info	*pci = NULL;
const char pci_name[] = B_PCI_MODULE_NAME;

ac97_module_info_v3 * ac97 = NULL;
const char ac97_name[] = B_AC97_MODULE_NAME_V3;

/* explicit declarations */
static int32 envy24_interrupt(void * data);
/* envy24_bd.c */
uint8 read_eeprom(uint32 , uint8 );
status_t read_eeprom_contents(uint32 , char * , uint8);
void InitGpIo(envy24_dev * );

/* envy24_ioctl.c */
status_t envy24_multi_get_description( void *, void *, size_t);
status_t envy24_multi_force_stop( void *);
status_t envy24_multi_buffer_exchange( void *, void *, size_t);
status_t envy24_multi_get_event_info( void *, void *, size_t);
status_t envy24_multi_set_event_info( void *, void *, size_t);
status_t envy24_multi_get_event( void *, void *, size_t);
status_t envy24_multi_get_enabled_channels( void *, void *, size_t);
status_t envy24_multi_set_enabled_channels( void *, void *, size_t);
status_t envy24_multi_get_global_format( void *, void *, size_t);
status_t envy24_multi_set_global_format( void *, void *, size_t);
status_t envy24_multi_get_buffers( void *, void *, size_t);
status_t envy24_multi_set_buffers( void *, void *, size_t);
status_t envy24_multi_list_modes( void *, void *, size_t);
status_t envy24_multi_get_mode( void *, void *, size_t);
status_t envy24_multi_set_mode( void *, void *, size_t);
status_t envy24_multi_list_extensions( void *, void *, size_t);
status_t envy24_multi_get_extension( void *, void *, size_t);
status_t envy24_multi_set_extension( void *, void *, size_t);
status_t envy24_multi_set_start_time( void *, void *, size_t);

/* envy24_ac97glue.c */
status_t envy24_init_ac97_module(envy24_dev *, uchar);
status_t envy24_uninit_ac97_module(envy24_dev *, uchar);
/* envy24_mix.c */
status_t envy24_multi_get_mix( void *, void *, size_t);
status_t envy24_multi_set_mix( void *, void *, size_t);
status_t envy24_multi_list_mix_channels( void *, void *, size_t);
status_t envy24_multi_list_mix_controls( void *, void *, size_t);
status_t envy24_multi_list_mix_connections( void *, void *, size_t);
void initialize_cached_controls( envy24_dev * );

/* Load firmaware as needed. */
/* called serialized */
static status_t
load_card(
	int ix)
{
	status_t err = B_OK;

	if (ix < 0 || ix >= card_count) {
		kprintf("envy24: load_card() called for bad index %d\n", ix);
		err = B_BAD_VALUE;
	}
	return err;
}

/* called serialized; maps card and initializes struct */
static status_t
setup_card(
	int ix)
{
	char name[32];
	status_t err = B_OK;

	ddprintf(("envy24: setup_card(%d)\n", ix));

	/* zeroed in find_cards */
	cards[ix].ix = ix;
	sprintf(name, "envy24 %d open_sem", ix);
	cards[ix].open_sem = create_sem(1, name);
	set_sem_owner(cards[ix].open_sem, B_SYSTEM_TEAM);
	sprintf(name, "envy24 %d control_sem", ix);
	cards[ix].control_sem = create_sem(1, name);
	set_sem_owner(cards[ix].control_sem, B_SYSTEM_TEAM);
	sprintf(name, "envy24 %d irq_sem", ix);
	cards[ix].irq_sem = create_sem(1, name);
	set_sem_owner(cards[ix].irq_sem, B_SYSTEM_TEAM);
	cards[ix].mce.info_size	  = sizeof(multi_channel_enable);
	cards[ix].mce.lock_source = DEFAULT_MULTI_LOCK_SOURCE;
	cards[ix].mce.lock_data	  = 0;
	cards[ix].mce.timecode_source = DEFAULT_MULTI_TIMECODE_SOURCE;
	cards[ix].mce.enable_bits = (uchar *)3;
	cards[ix].channel_buff_size = DEFAULT_MULTI_BUFF_SIZE;  // in SAMPLES!
	cards[ix].channel_area_pb	= B_ERROR;
	cards[ix].channel_area_cap	= B_ERROR;
	cards[ix].is_playing		= 0;
	cards[ix].ctlr_iobase		= 0;
	cards[ix].mt_iobase			= 0;
	cards[ix].event_sem			= B_ERROR;
	cards[ix].event_count		= 0;
	cards[ix].current_mask		= 0;
	cards[ix].queue_size		= MAX_EVENTS;
	cards[ix].preroll			= 0;
	cards[ix].start_time		= (bigtime_t)0;
	cards[ix].sample_rate		= DEFAULT_SAMPLE_RATE;
	
	return err;
}

/* called serialized */
static void
teardown_card(
	int ix)
{
	ddprintf(("envy24: teardown_card(%d)\n", ix));
	/* disable ints */
	/*	reset DSP */
	/*	clean up resources */
	if (cards[ix].open_sem >= B_OK) 
		delete_sem(cards[ix].open_sem);
	if (cards[ix].control_sem >= B_OK) 
		delete_sem(cards[ix].control_sem);
	if (cards[ix].irq_sem >= B_OK) 
		delete_sem(cards[ix].irq_sem);
	if (cards[ix].channel_area_pb >= B_OK) 
		delete_area(cards[ix].channel_area_pb);
	if (cards[ix].channel_area_cap >= B_OK) 
		delete_area(cards[ix].channel_area_cap);
		
	memset(&cards[ix], 0, sizeof(envy24_dev));
	cards[ix].open_sem 		= B_ERROR;
	cards[ix].control_sem	= B_ERROR;
	cards[ix].irq_sem		= B_ERROR;
	cards[ix].channel_area_pb  = B_ERROR;
	cards[ix].channel_area_cap = B_ERROR;

}

static int
allow_null_card(
	pci_info * info)
{
	ddprintf(("envy24: null card: size 0x%lx\n", info->u.h0.base_register_sizes[0]));
	return (info->vendor_id == ICENSEMBLE_VENDOR_ID) &&
		(info->device_id == ENVY24_DEVICE_ID) &&
		(info->u.h0.base_register_sizes[3] == 0x40) &&
		(info->u.h0.base_register_sizes[2] == 0x10) &&
		(info->u.h0.base_register_sizes[1] == 0x10) &&
		(info->u.h0.base_register_sizes[0] == 0x20);
}

static int
find_cards()
{
	int ix;
	pci_info info;
	int cc = 0;

	memset(cards, 0, sizeof(cards));
	for (ix=0; !(*pci->get_nth_pci_info)(ix, &info); ix++) {
		if (info.vendor_id != ICENSEMBLE_VENDOR_ID || info.device_id != ENVY24_DEVICE_ID) {
			continue;
		}
		if ((info.header_type & PCI_header_type_mask) != PCI_header_type_generic) {
			continue;
		}
		if (info.u.h0.subsystem_vendor_id != ICE_SSVID ||
			info.u.h0.subsystem_id != ENVY24_SSDID) {
#if 1
			if (allow_null_card(&info)) {
			} else
#endif			
			if (info.u.h0.subsystem_vendor_id != 0 ||
				info.u.h0.subsystem_id != 0 ||
				!allow_null_card(&info)) {
				continue;
			}

		}
		if (cc >= MAX_ENVY24_BD) {
			dprintf("envy24: there are more than %d envy24 cards installed!\n", MAX_ENVY24_BD);
			break;
		}
		ddprintf(("envy24: found card %d\n", cc));
		cards[cc].pci_bus		= info.bus;
		cards[cc].pci_device	= info.device;
		cards[cc].pci_function	= info.function;
		cards[cc].pci_interrupt	= info.u.h0.interrupt_line;
		names[cc] = init_names[cc];
		cc++;
		
		ddprintf(("envy24: the int in find cards is %d\n",info.u.h0.interrupt_line));
		/* NOTE: until the driver supports multiple cards, */
		/* only publish 1 device.                          */
		break;
	}
	names[cc] = NULL;
	return cc;
}


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	status_t err = ENODEV;
	int ix;


	ddprintf(("envy24: init_hardware() 0x%p\n",init_hardware));
	ddprintf(("/*************************************************************/\n"));
	ddprintf(("/*                ENVY24 NEW HARDWARE LOAD                   */\n"));
	ddprintf(("/*************************************************************/\n"));
	dprintf("envy24: init_hardware() %s %s\n",__TIME__,__DATE__);

	if (get_module(pci_name, (module_info **) &pci)) {
		dprintf("envy24: module '%s' not found\n", pci_name);
		pci = NULL;
		return ENOSYS;
	}
	if (get_module(ac97_name, (module_info **) &ac97)) {
		dprintf("envy24: module '%s' not found\n", ac97_name);
		put_module(pci_name);
		pci  = NULL;
		ac97 = NULL;
		return ENOSYS;
	}
	else {
		ddprintf(("envy24: found the module '%s' \n", ac97_name));
		put_module(ac97_name);
	}

#if DEBUG
	set_dprintf_enabled(1);
#endif

	/* figure out what cards are on the bus and set the card_count accordingly */
	card_count = find_cards();
	if (card_count < 1) {
		ddprintf(("envy24: no cards\n"));
		return ENODEV;
	}

	/* reset the cards to a known state */
	for (ix = 0; ix<card_count; ix++) {
		err = setup_card(ix);
		if (err == B_OK) {
			err = load_card(ix);	
			teardown_card(ix);
		}
	}
	card_count = 0;

	put_module(pci_name);
	pci = NULL;

	return err;
}


status_t
init_driver(void)
{
	int ix;
	status_t err;
	dprintf("envy24: init_driver() %s %s\n",__TIME__,__DATE__);

	if (get_module(pci_name, (module_info **) &pci)) {
		pci = NULL;
		return ENOSYS;
	}
	
	if (get_module(ac97_name, (module_info **) &ac97)) {
		dprintf("envy24: module '%s' not found\n", ac97_name);
		put_module(pci_name);
		pci  = NULL;
		ac97 = NULL;
		return ENOSYS;
	}
	
	/* figure out what cards are there and fill in pci info  */
	card_count = find_cards();
	
	if (card_count > 0) {
		for (ix=0; ix<card_count; ix++) {
			ddprintf(("envy24: ix %d  card_count %d \n", ix, card_count));		

			err = setup_card(ix);
			if (err == B_OK) {
				err = load_card(ix);	
			}
			if (err < B_OK) {
				dprintf("envy24: card %d is not cooperating, no more cards will be added\n", ix);
				teardown_card(ix);
				card_count = ix;
			}
			
			/* pci config space stuff */	
			cards[ix].ctlr_iobase  = PCI_CFG_RD(cards[ix].pci_bus,
												cards[ix].pci_device,
												cards[ix].pci_function,
												0x10,
												4 );
			cards[ix].ctlr_iobase &= 0xfffffffe;									
			cards[ix].mt_iobase	   = PCI_CFG_RD(cards[ix].pci_bus,
												cards[ix].pci_device,
												cards[ix].pci_function,
												0x1c,
												4 );
			cards[ix].mt_iobase &= 0xfffffffe;									

			ddprintf(("envy24: bus %ld  device %ld function %ld int %ld\n", cards[ix].pci_bus, cards[ix].pci_device, cards[ix].pci_function, cards[ix].pci_interrupt));
			ddprintf(("envy24: ctlbase %lx  mtbase %lx  \n", cards[ix].ctlr_iobase, cards[ix].mt_iobase));

			PCI_CFG_WR( cards[ix].pci_bus,
						cards[ix].pci_device,
						cards[ix].pci_function,
						0x04,
						2,
						0x5 ); /* bm/io enable */

		    if ( (PCI_IO_RD(cards[ix].ctlr_iobase + ICE_I2C_PORT_CONTROL_REG) & 0x80) == 0) {
				dprintf("envy24: No eeprom on this card\n");
			}
			else {						
				/* read the eeprom */
				ddprintf(("envy24: subvendor id in eeprom is %x%x and %x%x\n",
					read_eeprom(cards[ix].ctlr_iobase, 0),			
					read_eeprom(cards[ix].ctlr_iobase, 1),			
					read_eeprom(cards[ix].ctlr_iobase, 2),			
					read_eeprom(cards[ix].ctlr_iobase, 3)			));
			}
		}	
		for (ix=0; ix<card_count; ix++) {
			/* init device */
			/* reset */
			PCI_IO_WR( cards[ix].ctlr_iobase, 0x80);
			PCI_IO_WR( cards[ix].ctlr_iobase, 0x00);
			/* find and init codecs here */
			{
#if THIRD_PARTY
				status_t err;
#endif
				uint8 * z;
				uint16 x;
				uint32 y;
				
				read_eeprom_contents( cards[ix].ctlr_iobase, (char *) &(cards[ix].teepee), (uint8)sizeof(EEPROM));
				
				for (x = 0, z = (uint8 *) &(cards[ix].teepee); x < sizeof(EEPROM); x++) {
					ddprintf(("envy24: eeprom %d 0x%2x\n",x, *z++ ));
				}
				
				PCI_CFG_WR( cards[ix].pci_bus,
							cards[ix].pci_device,
							cards[ix].pci_function,
							0x60,
							1,
							cards[ix].teepee.bCodecConfig ); 
				
				y  = PCI_CFG_RD(cards[ix].pci_bus,
								cards[ix].pci_device,
								cards[ix].pci_function,
								0x61,
								1 );
					
				dprintf("envy24: MT converter is type 0x%x 0 = ac97\n",(unsigned int)y);
				y &= 0x80;				
				cards[ix].teepee.bACLinkConfig &= 0x7f;
				cards[ix].teepee.bACLinkConfig |= y;
				
				PCI_CFG_WR( cards[ix].pci_bus,
							cards[ix].pci_device,
							cards[ix].pci_function,
							0x61,
							1,
							cards[ix].teepee.bACLinkConfig ); 

				PCI_CFG_WR( cards[ix].pci_bus,
							cards[ix].pci_device,
							cards[ix].pci_function,
							0x62,
							1,
							cards[ix].teepee.bI2SID ); 

				PCI_CFG_WR( cards[ix].pci_bus,
							cards[ix].pci_device,
							cards[ix].pci_function,
							0x63,
							1,
							cards[ix].teepee.bSpdifConfig ); 
				/* must be called in a thread safe manner.... */
				InitGpIo(&cards[ix]);
				
				/* init the consumer AC97 (if it existe) for digital mix out */
				if ( !(cards[ix].teepee.bCodecConfig & 0x10)) {
					ddprintf(("envy24: Consumer AC97 codec is PRESENT (and accounted for)\n"));
					if (envy24_init_ac97_module( &cards[ix], CONSUMER_AC97 ) != B_OK ) {
						/* pretend the codec does not exist */
						envy24_uninit_ac97_module( &cards[ix], CONSUMER_AC97 );
						cards[ix].teepee.bCodecConfig |= 0x10;
						PCI_CFG_WR( cards[ix].pci_bus,
									cards[ix].pci_device,
									cards[ix].pci_function,
									0x60,
									1,
									cards[ix].teepee.bCodecConfig ); 
					}
				}
				else {
					ddprintf(("envy24: Consumer AC97 codec is AWOL!\n"));
				}
#if THIRD_PARTY 
				if (cards[ix].teepee.dwSubVendorID == THIRD_PARTY_ID) {
					ddprintf(("envy24: got a third party board\n"));
					
					// init the AKM4524 codecs
					err = ak4524_init(&cards[ix]);
				}
#endif
			}
			/* set up int masks */
			PCI_IO_WR(cards[ix].ctlr_iobase + INT_MASK_REG, 0xff );
			
			install_io_interrupt_handler(cards[ix].pci_interrupt, envy24_interrupt, (void *)ix, 0);
			ddprintf(("envy24: int handler %p\n",envy24_interrupt));
			/* set up int masks */
			PCI_IO_WR(cards[ix].ctlr_iobase + INT_MASK_REG, ~MTPR_INT );
			PCI_IO_WR(cards[ix].mt_iobase , 0x43 );

			/* vol ctl update rate */
			PCI_IO_WR(cards[ix].mt_iobase + 0x3b, 0x00 );
			/* the rest of the ctls */
			initialize_cached_controls( &cards[ix] );
		}	
	}
	return card_count > 0 ? B_OK : ENODEV;
}


void
uninit_driver(void)
{
	int ix;
	uchar index;

	ddprintf(("envy24: uninit_driver()\n"));

	for (ix=0; ix<card_count; ix++) {
		remove_io_interrupt_handler(cards[ix].pci_interrupt, envy24_interrupt, (void *)ix);
		if (ac97)
		{
			for (index = 0; index < MAX_AC97_CODECS; index++) {
				envy24_uninit_ac97_module(&cards[ix], index);
			}
		}	
		teardown_card(ix);
	}
	memset(&cards, 0, sizeof(cards));
	if (pci) {
		put_module(pci_name);
		pci = NULL;
	}
	if (ac97) {
		put_module(ac97_name);
		ac97 = NULL;
	}	
}


const char **
publish_devices(void)
{
	int ix;
	ddprintf(("envy24: publish_devices()\n"));

	for (ix=0; names[ix]; ix++) {
		ddprintf(("%s\n", names[ix]));
	}
	return (const char **)names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	ddprintf(("envy24: find_device()\n"));

	for (ix=0; names[ix]; ix++) {
		if (!strcmp(name, names[ix])) {
			return &envy24_hooks;
		}
	}
	return NULL;
}

static status_t envy24_open(const char *name, uint32 flags, void **cookie);
static status_t envy24_close(void *cookie);
static status_t envy24_free(void *cookie);
static status_t envy24_control(void *cookie, uint32 op, void *data, size_t len);
static status_t envy24_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t envy24_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks envy24_hooks = {
    &envy24_open,
    &envy24_close,
    &envy24_free,
    &envy24_control,
    &envy24_read,
    &envy24_write,
	NULL,				/* select and scatter/gather */
	NULL,
	NULL,
	NULL
};



static status_t
envy24_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	const char * val;
	int p;
	open_envy24 * card;
	status_t err;
	uint32 mode = (flags & 3) + 1; // rd = 1 wr = 2 rdwr = 3 multi_ctrl = 4

	ddprintf(("envy24: envy24_open(%s, %ld, %p\n", name, flags, cookie));

	if (!name || !cookie) {
		return EINVAL;
	}
	*cookie = NULL;
	val = strrchr(name, '/');
	if (val) {
		val++;
	}
	else {
		val = name;
	}
	p = *val - '1';
	if (p < 0 || p >= MAX_ENVY24_BD) {
		return EINVAL;
	}

	err = acquire_sem_etc(cards[p].open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}
	if (cards[p].open_mode && (mode & 3) ) {
		err = EPERM;
		ddprintf(("envy24: Too many opens\n"));
		goto cleanup_1;
	}
	card = (open_envy24 *)malloc(sizeof(open_envy24));
	if (!card) {
		err = ENOMEM;
		goto cleanup_1;
	}
	
	/* ignore multiple "control" opens */
	/* but only allow 1 opener of type rd, wr, or rdwr */
	cards[p].open_mode |= (mode & 3);

	card->device = &cards[p];
	card->mode = mode;
	*cookie = card;

	err = B_OK;
cleanup_1:
	release_sem(cards[p].open_sem);
	return err;	
	
}


static status_t
envy24_close(
	void * cookie)
{
	open_envy24 * card = (open_envy24 *)cookie;
	status_t err;

	ddprintf(("envy24: envy24_close()\n"));

	err = acquire_sem_etc(card->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("envy24: acquire_sem_etc failed in envy24_close() \n");
		return err;
	}
	/* only rd, wr, and rdwr can start/stop data */
	if (card->mode & 3) {
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("envy24: acquire_sem_etc failed in force_stop [%lx]\n", err);
		}
		else {
			err = envy24_multi_force_stop(cookie);
			release_sem(card->device->control_sem);
		}

		card->device->open_mode &= ~card->mode;
	}	
	release_sem(card->device->open_sem);
	
	return err;
}


static status_t
envy24_free(
	void * cookie)
{
	open_envy24 * card_open = (open_envy24 *)cookie;
	status_t err;
	int ix;

	ddprintf(("envy24: envy24_free()\n"));

	err = acquire_sem_etc(card_open->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("envy24: acquire_sem_etc failed in envy24_free() [%lx]\n", err);
		return err;
	}
	ix = card_open->device->ix;
	free(card_open);
	release_sem(cards[ix].open_sem);
	
	return B_OK;
}

static status_t
envy24_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	status_t err = B_OK;
	open_envy24 * card = (open_envy24 *)cookie;

	/*ddprintf(("envy24: envy24_control(%p, %ld, %p, %ld\n",
		cookie, iop, data, len));*/

	if (iop == B_MULTI_BUFFER_EXCHANGE && (card->mode & 3)) {
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("envy24: acquire_sem_etc failed in multi_buffer_exchange [%lx]\n", err);
		}
		else {
			err = envy24_multi_buffer_exchange(cookie, data, len);
			release_sem(card->device->control_sem);
		}	
		return err; 
	}

	switch (iop) {

	case B_MULTI_GET_EVENT_INFO: 
		ddprintf(("envy24: get_event_info \n"));
		err = envy24_multi_get_event_info(cookie, data, len);
		break;
	case B_MULTI_SET_EVENT_INFO:
		ddprintf(("envy24: set_event_info \n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in set_event [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_event_info(cookie, data, len);
				release_sem(card->device->control_sem);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;
	case B_MULTI_GET_EVENT: 
		ddprintf(("envy24: get_event \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in get_event [%lx]\n", err);
			}
			else {
				err = envy24_multi_get_event(cookie, data, len);
				release_sem(card->device->control_sem);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case B_MULTI_GET_DESCRIPTION : 
		ddprintf(("envy24: get_description \n"));
		err = envy24_multi_get_description(cookie, data, len);
		break;

	case B_MULTI_GET_ENABLED_CHANNELS :
		ddprintf(("envy24: get_enabled_channels \n"));
		err = envy24_multi_get_enabled_channels(cookie, data, len);
		break;

	case B_MULTI_SET_ENABLED_CHANNELS : 
		ddprintf(("envy24: set_enabled_channels \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_enabled_channels(cookie, data, len);
				release_sem(card->device->control_sem);
			}	
		}
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;

	case B_MULTI_GET_GLOBAL_FORMAT :
		ddprintf(("envy24: get_global_format \n"));
		err = envy24_multi_get_global_format(cookie, data, len);
		break;
	case B_MULTI_SET_GLOBAL_FORMAT :
		ddprintf(("envy24: set_global_format \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_global_format(cookie, data, len);
				release_sem(card->device->control_sem);
			}
		}	
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;
	case B_MULTI_GET_BUFFERS :
		ddprintf(("envy24: get_buffers \n"));
		if (card->mode & 3) {
			err = envy24_multi_get_buffers(cookie, data, len);
		}
		else {
			err = B_PERMISSION_DENIED;
		}	
		break;
	case B_MULTI_SET_BUFFERS :
		ddprintf(("envy24: set_buffers \n"));
		if (card->mode & 3) {
			err = envy24_multi_set_buffers(cookie, data, len);
		}
		else {
			err = B_PERMISSION_DENIED;
		}	
		break;

	case B_MULTI_BUFFER_FORCE_STOP : 
		ddprintf(("envy24: force stop \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in force_stop [%lx]\n", err);
			}
			else {
				err = envy24_multi_force_stop(cookie);
				release_sem(card->device->control_sem);
			}
		}	
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;

	case B_MULTI_GET_MIX :
		ddprintf(("envy24: get_mix \n"));
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("envy24: acquire_sem_etc failed in get_mix [%lx]\n", err);
		}
		else {
			err = envy24_multi_get_mix(cookie, data, len);
			release_sem(card->device->control_sem);
		}	
		break;
	case B_MULTI_SET_MIX :
		ddprintf(("envy24: set_mix \n"));
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
		}
		else {
			err = envy24_multi_set_mix(cookie, data, len);
			release_sem(card->device->control_sem);
		}
		break;
	case B_MULTI_LIST_MIX_CHANNELS:
		ddprintf(("envy24: list_mix_channels \n"));
		err = envy24_multi_list_mix_channels(cookie, data, len);
		break;
	case B_MULTI_LIST_MIX_CONTROLS:
		ddprintf(("envy24: list_mix_controls \n"));
		err = envy24_multi_list_mix_controls(cookie, data, len);
		break;
	case B_MULTI_LIST_MIX_CONNECTIONS:
		ddprintf(("envy24: list_mix_connections \n"));
		err = envy24_multi_list_mix_connections(cookie, data, len);
		break;
		
	case B_MULTI_LIST_MODES:
		ddprintf(("envy24: list_modes \n"));
		err = envy24_multi_list_modes(cookie, data, len);
		break;
	case B_MULTI_GET_MODE:
		ddprintf(("envy24: get_mode \n"));
		err = envy24_multi_get_mode(cookie, data, len);
		break;
	case B_MULTI_SET_MODE :
		ddprintf(("envy24: set_mode \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_mode(cookie, data, len);
				release_sem(card->device->control_sem);
			}
		}
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;

	case B_MULTI_LIST_EXTENSIONS:
		ddprintf(("envy24: list_extensions \n"));
		err = envy24_multi_list_extensions(cookie, data, len);
		break;
	case B_MULTI_GET_EXTENSION:
		ddprintf(("envy24: get_extension \n"));
		err = envy24_multi_get_extension(cookie, data, len);
		break;
	case B_MULTI_SET_EXTENSION :
		ddprintf(("envy24: set_extension \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_extension(cookie, data, len);
				release_sem(card->device->control_sem);
			}
		}
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;

	case B_MULTI_SET_START_TIME:
		ddprintf(("envy24: set_start_time \n"));
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("envy24: acquire_sem_etc failed in envy24_control() [%lx]\n", err);
			}
			else {
				err = envy24_multi_set_start_time(cookie, data, len);
				release_sem(card->device->control_sem);
			}
		}
#ifndef TESTING
		else {
			err = B_PERMISSION_DENIED;
		}	
#endif
		break;

	default:
		dprintf("envy24: unknown ioctl() %ld\n", iop);
		err = B_DEV_INVALID_IOCTL;;
		break;
	}

	return err;
}


static status_t
envy24_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	(void)pos;	// unused

	if (!cookie || !data || !nread) {
		return EINVAL;
	}
	*nread = 0;
	return B_ERROR;
}


static status_t
envy24_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	(void)pos;	// unused

	if (!cookie || !data || !nwritten) {
		return EINVAL;
	}
	*nwritten = 0;
	return B_ERROR;
}

static int32
envy24_interrupt(
	void * data)
{
	int32 ix = (int32) data;
	int32 handled = B_UNHANDLED_INTERRUPT;
	uint32 tmp;


//kprintf("#");

	acquire_spinlock(&(cards[ix].spinlock));

	if (PCI_IO_RD(cards[ix].ctlr_iobase + INT_STATUS_REG) & MTPR_INT) {
		cards[ix].timestamp = system_time();
		tmp  = (PCI_IO_RD_32(cards[ix].mt_iobase + MTPR_DMA_REC_ADDR_REG) - (uint32)(cards[ix].channel_physical_cap.address)) /48;

		/* turn off the interrupt */
		PCI_IO_WR(cards[ix].mt_iobase, 0x43);

		/* adjust position returned to match buffer boundary */
		/* as the multinode expects the timestamp at the interrupt. */
		/* we should check to see if we missed interrupts....(rev. 2 )*/
		cards[ix].timestamp = cards[ix].timestamp - (((int64)(tmp % cards[ix].channel_buff_size)*1000000LL)/(int64)cards[ix].sample_rate);
		cards[ix].rec_position +=  cards[ix].channel_buff_size;
		cards[ix].pb_position = cards[ix].rec_position;
		
		release_spinlock(&(cards[ix].spinlock));
		
		release_sem_etc(cards[ix].irq_sem, 1, B_DO_NOT_RESCHEDULE);
		handled = B_INVOKE_SCHEDULER;
	}

	if (handled != B_INVOKE_SCHEDULER) {
		release_spinlock(&(cards[ix].spinlock));
	}		


	return handled;
}
