/* includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>

#include "multi_audio.h"
#include "sonorus.h"


/* debug */
#if DEBUG
#define ddprintf dprintf
#define CHECK_CODE 1
#define TESTING
#else
#define ddprintf (void)
#define CHECK_CODE 0
#undef  TESTING
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


extern device_hooks sonorus_hooks;


/* globals */
int card_count 		= 0;
int port_caps		= 10; 	// 8 ch ADAT i/o A 2 ch SPDIF i/o B
int dsp_caps		= kSupportsMixing;
int outputs_per_bd	= 10;	// for 1111
int inputs_per_bd	= 10;	// for 1111
int monitors_per_bd = 2;	// l,r for 1111
int packed_mode 	= 0;
int is_playing 		= 0;

sonorus_dev cards[MAX_STUDIO_BD];

static char * names[MAX_STUDIO_BD+1];
static char * init_names[MAX_STUDIO_BD+1] = {
	"audio/multi/sonorus/1",
	"audio/multi/sonorus/2",
	"audio/multi/sonorus/3",
	"audio/multi/sonorus/4",
	NULL
};

const char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;

extern uint32 dsp_MonMix_ImageSize;
extern uint32 dsp_MonMix_Image[];
extern uint32 fpga_1111d_ImageSize;
extern uint8 fpga_1111d_Image[];
extern uint32 fpga_1111b_ImageSize;
extern uint8 fpga_1111b_Image[];

/* explicit declarations for the compiler */
/* sonorus.c */
static int32 sonorus_interrupt(void * data);
/* sonorus_bd.c */
void send_host_cmd( int, uint32);
void set_host_ctrl_bits( int, uint32, uint32);
uint32 get_host_stat( int);
/* sonorus_dev.c */
status_t init_dsp( int,	uint32 *, size_t );
status_t init_altera( int, uint8 *,	size_t);
uint32 read_loc( int, uint32, memSpace_t);
void set_device_param( studParmID_t, int32);
void set_sync_params_MonMix_1111( uint32, studSyncSrc_t);
void fill_x_fast( uint16, uint32, uint32, uint32, uint32);
void set_channel_param( studParmID_t, int32, int32);
void set_dac_params_MonMix_1111( uint32, uint8, bool);
/* sonorus_ioctl.c */
status_t sonorus_multi_get_description( void *, void *, size_t);
status_t sonorus_multi_force_stop( void *);
status_t sonorus_multi_buffer_exchange( void *, void *, size_t);
status_t sonorus_multi_get_event_info( void *, void *, size_t);
status_t sonorus_multi_set_event_info( void *, void *, size_t);
status_t sonorus_multi_get_event( void *, void *, size_t);
status_t sonorus_multi_get_enabled_channels( void *, void *, size_t);
status_t sonorus_multi_set_enabled_channels( void *, void *, size_t);
status_t sonorus_multi_get_global_format( void *, void *, size_t);
status_t sonorus_multi_set_global_format( void *, void *, size_t);
status_t sonorus_multi_get_buffers( void *, void *, size_t);
status_t sonorus_multi_set_buffers( void *, void *, size_t);
status_t sonorus_multi_list_modes( void *, void *, size_t);
status_t sonorus_multi_get_mode( void *, void *, size_t);
status_t sonorus_multi_set_mode( void *, void *, size_t);
status_t sonorus_multi_list_extensions( void *, void *, size_t);
status_t sonorus_multi_get_extension( void *, void *, size_t);
status_t sonorus_multi_set_extension( void *, void *, size_t);
status_t sonorus_multi_set_start_time( void *, void *, size_t);
/* sonorus_mix.c */
status_t sonorus_multi_get_mix( void *, void *, size_t);
status_t sonorus_multi_set_mix( void *, void *, size_t);
status_t sonorus_multi_list_mix_channels( void *, void *, size_t);
status_t sonorus_multi_list_mix_controls( void *, void *, size_t);
status_t sonorus_multi_list_mix_connections( void *, void *, size_t);



/* Load the DSP and FPGA according to the configuration specified. */
/* called serialized */
static status_t
load_card(
	int ix)
{
	status_t err = B_OK;

	if (ix < 0 || ix >= card_count) {
		kprintf("sonorus: load_card() called for bad index %d\n", ix);
	}
	if (err == B_OK) {
		err = init_dsp(ix, dsp_MonMix_Image, dsp_MonMix_ImageSize);
	}
	if (err == B_OK) {
		cards[ix].revision = 'A' + read_loc(ix, 0x01a, xMem);
		ddprintf("sonorus: card version %c\n", cards[ix].revision);
		if (cards[ix].revision < 'C') {
			err = init_altera(ix, fpga_1111b_Image, fpga_1111b_ImageSize);
		}
		else {
			err = init_altera(ix, fpga_1111d_Image, fpga_1111d_ImageSize);
		}
	}
	else {
		dprintf("sonorus: error in init_dsp\n");
	}
	return err;
}

/* called serialized; maps card and initializes struct */
static status_t
setup_card(
	int ix)
{
	char name[32];
	uint32 config = 0;
	int chan;
	void* base_reg;

	ddprintf("sonorus: setup_card(%d)\n", ix);

	//	enable memory access in config space

	config = (*pci->read_pci_config)(
		cards[ix].card_info.bus,
		cards[ix].card_info.device,
		cards[ix].card_info.function,
		0x04,
		2);
	config = config | 0x2;
	(*pci->write_pci_config)(
		cards[ix].card_info.bus,
		cards[ix].card_info.device,
		cards[ix].card_info.function,
		0x04,
		2,
		config);
	
	cards[ix].ix = ix;
	sprintf(name, "sonorus %d open_sem", ix);
	cards[ix].open_sem = create_sem(1, name);
	set_sem_owner(cards[ix].open_sem, B_SYSTEM_TEAM);
	sprintf(name, "sonorus %d irq_sem", ix);
	cards[ix].irq_sem = create_sem(1, name); /* was 2 */
	set_sem_owner(cards[ix].irq_sem, B_SYSTEM_TEAM);
	sprintf(name, "sonorus %d control_sem", ix);
	cards[ix].control_sem = create_sem(1, name);
	set_sem_owner(cards[ix].control_sem, B_SYSTEM_TEAM);
	sprintf(name, "sonorus %d queue_sem", ix);
	cards[ix].queue_sem = create_sem(1, name);
	set_sem_owner(cards[ix].queue_sem, B_SYSTEM_TEAM);
	cards[ix].current_mask    = 0;
	cards[ix].queue_size      = MAX_EVENTS;
	cards[ix].event_count     = 0;
	cards[ix].event_sem		  = B_ERROR;
	cards[ix].sample_rate 	  = DEFAULT_SAMPLE_RATE;
	cards[ix].lock_source     = DEFAULT_LOCK_SOURCE;
	cards[ix].channels_enabled= 0;
	cards[ix].timecode		  = 0;
	cards[ix].tc_position	  = 0;
	cards[ix].timestamp		  = 0;
	cards[ix].position		  = 0;
	cards[ix].open_mode		  = 0;
	cards[ix].mce.enable_bits = NULL;
	cards[ix].mce.lock_source = DEFAULT_MULTI_LOCK_SOURCE;
	cards[ix].mce.lock_data   = DEFAULT_MULTI_LOCK_DATA;
	cards[ix].mce.timecode_source= DEFAULT_MULTI_TIMECODE_SOURCE;
	cards[ix].channel_area       = B_ERROR;
	cards[ix].channels[0].buf[0] = NULL;
	cards[ix].channel_buff_size	 = 128; 
	for (chan = 0; chan < inputs_per_bd + outputs_per_bd; chan++) {
		cards[ix].channels[chan].index = 0;
		cards[ix].channels[chan].monitor_ramp = -cards[ix].channel_buff_size;
		cards[ix].channels[chan].monitor_ts   = 0;
		cards[ix].channels[chan].output_ramp  = -cards[ix].channel_buff_size;
		cards[ix].channels[chan].output_ts    = 0;
		cards[ix].chix[0].begGain[ chan ] = STUD_UNITY_GAIN ;
		cards[ix].chiy[0].endGain[ chan ] = STUD_UNITY_GAIN ;
		cards[ix].chix[0].begPanL[ chan ] = (chan&1) ? 0 : STUD_UNITY_GAIN;
		cards[ix].chiy[0].endPanL[ chan ] = (chan&1) ? 0 : STUD_UNITY_GAIN;
		cards[ix].chix[0].begPanR[ chan ] = (chan&1) ? STUD_UNITY_GAIN : 0;
		cards[ix].chiy[0].endPanR[ chan ] = (chan&1) ? STUD_UNITY_GAIN : 0;
		cards[ix].chix[1].begGain[ chan ] = STUD_UNITY_GAIN ;
		cards[ix].chiy[1].endGain[ chan ] = STUD_UNITY_GAIN ;
		cards[ix].chix[1].begPanL[ chan ] = (chan&1) ? 0 : STUD_UNITY_GAIN;
		cards[ix].chiy[1].endPanL[ chan ] = (chan&1) ? 0 : STUD_UNITY_GAIN;
		cards[ix].chix[1].begPanR[ chan ] = (chan&1) ? STUD_UNITY_GAIN : 0;
		cards[ix].chiy[1].endPanR[ chan ] = (chan&1) ? STUD_UNITY_GAIN : 0;
	}
	cards[ix].preroll = 0;
	cards[ix].start_time = 0;
	memset(&(cards[ix].timer_stuff),0, sizeof(timer_plus));
	cards[ix].record_enable= true;  
	sprintf(name, "sonorus %d memory", ix);
	base_reg = (void*)cards[ix].card_info.u.h0.base_registers[0];
	cards[ix].dsp_area = map_physical_memory(
		name, 
		base_reg, 
		cards[ix].card_info.u.h0.base_register_sizes[0],
		B_ANY_KERNEL_ADDRESS, 
		B_READ_AREA | B_WRITE_AREA,
		(void **)&cards[ix].dsp);
	cards[ix].hdata_area = B_ERROR;
#if 0
	/* write combining (bursting) doesn't work with the */
	/* Studi/o for some reason (timing?). However, I'll */
	/* leave this example here for future reference.... */
	sprintf(name, "sonorus %d WC memory", ix);
	cards[ix].hdata_area = map_physical_memory(
		name, 
		((uint8*)base_reg) + 32*1024, 
		32*1024,
		B_ANY_KERNEL_ADDRESS  /* | B_MTR_WC */ , 
		B_READ_AREA | B_WRITE_AREA,
		(void **)&cards[ix].hdata);

	cards[ix].hdata = ((uint32*)cards[ix].hdata) + 7;

	if(cards[ix].hdata_area < B_OK)
#endif
	{
		ddprintf("sonorus: using first memory mapping \n");
		cards[ix].hdata = (uint32*) &cards[ix].dsp->hdat ;
	}
	ddprintf("sonorus: memory %x mapped at %x\n",
		cards[ix].card_info.u.h0.base_registers[0]&~0x3,
		cards[ix].dsp);		
	return cards[ix].dsp_area < B_OK ? cards[ix].dsp_area : B_OK;
}

/* called serialized */
static void
teardown_card(
	int ix)
{
	ddprintf("sonorus: teardown_card(%d)\n", ix);
	/* disable ints */
	send_host_cmd( ix, HC_INTDIS );
	/*	reset DSP */
	//This is an effort to decrease pci bus lockups....
	//send_host_cmd_nmi(ix, HC_RESET);
	/*	clean up resources */
	delete_area(cards[ix].dsp_area);
	delete_sem(cards[ix].open_sem);
	delete_sem(cards[ix].irq_sem);
	delete_sem(cards[ix].control_sem);
	delete_sem(cards[ix].queue_sem);
	if (cards[ix].channel_area >= B_OK ) {
		delete_area(cards[ix].channel_area);
		cards[ix].channel_area = B_ERROR;
	}
	if (cards[ix].hdata_area >= B_OK) {
		delete_area(cards[ix].hdata_area);
		cards[ix].hdata_area = B_ERROR;
	}
	cards[ix].channels[0].buf[0] = NULL;

}

static int
allow_null_card(
	pci_info * info)
{
	ddprintf("sonorus: null card: size 0x%x\n", info->u.h0.base_register_sizes[0]);
	return (info->vendor_id == MOTOROLA_VENDOR_ID) &&
		(info->device_id == DSP56300_DEVICE_ID) &&
		(info->u.h0.base_register_sizes[1] == 0) &&
		(info->u.h0.base_register_sizes[0] == 0x10000);
}

static int
find_cards()
{
	int ix;
	pci_info info;
	int cc = 0;

	memset(cards, 0, sizeof(cards));
	for (ix=0; !(*pci->get_nth_pci_info)(ix, &info); ix++) {
		if (info.vendor_id != MOTOROLA_VENDOR_ID || info.device_id != DSP56300_DEVICE_ID) {
			continue;
		}
		if ((info.header_type & PCI_header_type_mask) != PCI_header_type_generic) {
			continue;
		}
		if (info.u.h0.subsystem_vendor_id != SONORUS_VENDOR_ID ||
			info.u.h0.subsystem_id != STUDIO_DEVICE_ID) {
			if (info.u.h0.subsystem_vendor_id != 0 ||
				info.u.h0.subsystem_id != 0 ||
				!allow_null_card(&info)) {
				continue;
			}
		}
		if (cc >= MAX_STUDIO_BD) {
			dprintf("sonorus: there are more than %d Sonorus cards installed!\n", MAX_STUDIO_BD);
			break;
		}
		ddprintf("sonorus: found card %d\n", cc);
		cards[cc].card_info = info;
		names[cc] = init_names[cc];
		cc++;
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

	ddprintf("sonorus: init_hardware() 0x%x\n",init_hardware);
	ddprintf("/*************************************************************/\n");
	ddprintf("/*                                                           */\n");
	ddprintf("/*               Sonorus NEW HARDWARE LOAD                   */\n");
	ddprintf("/*                    PRIVATE BUILD                          */\n");
	ddprintf("/*************************************************************/\n");
	dprintf("sonorus: init_hardware() %s %s\n",__TIME__,__DATE__);

	if (get_module(pci_name, (module_info **) &pci)) {
		dprintf("sonorus: module '%s' not found\n", pci_name);
		return ENOSYS;
	}

#if DEBUG
	set_dprintf_enabled(1);
#endif

	/* figure out what cards are on the bus and set the card_count accordingly */
	memset(cards, 0, sizeof(cards));
	card_count = find_cards();
	if (card_count < 1) {
		ddprintf("sonorus: no cards\n");
		return ENODEV;
	}

	/* reset the cards to a known state */
	for (ix = 0; ix<card_count; ix++) {
		err = setup_card(ix);
		if (err == B_OK) {
			err = load_card(ix);	/* this is the meat */
			teardown_card(ix);
		}
	}
	card_count = 0;

	put_module(pci_name);

	return err;
}


status_t
init_driver(void)
{
	int ix, channel;
	status_t err;
	ddprintf("sonorus: init_driver()\n");

	if (get_module(pci_name, (module_info **) &pci))
		return ENOSYS;

	/* figure out what cards are there and fill out the card info structs */
	card_count = find_cards();
	
	if (card_count > 0) {
		for (ix=0; ix<card_count; ix++) {
			ddprintf("sonorus: ix %d  card_count %d \n", ix, card_count);		
			err = setup_card(ix);
			if (err == B_OK) {
//This is an effort to decrease pci bus lockups....
//				err = load_card(ix);	/* this is the meat */
			}
			if (err < B_OK) {
				dprintf("sonorus: card %d is not cooperating, no more cards will be added\n", ix);
				teardown_card(ix);
				card_count = ix;
			}
		}	
		for (ix=0; ix<card_count; ix++) {
			install_io_interrupt_handler(cards[ix].card_info.u.h0.interrupt_line, sonorus_interrupt, (void *)ix, 0);
			send_host_cmd( ix, HC_INTENB );
			ddprintf("sonorus: int handler 0x%x\n",sonorus_interrupt);
				
			/* this will fix the spdif problem... */				
			set_device_param( STUD_PARM_PORTMODE , port_caps );
				
			/* prepare device: set sync source to "internal, 44.1 KHz" */
			set_sync_params_MonMix_1111(cards[ix].sample_rate ,
										cards[ix].lock_source );
	
			/* wait for internal and external things to sync up. */
			snooze(10 * 1000);
			/* prepare device: set play speed to unity. */
			set_device_param( STUD_PARM_SPEED , STUD_SPEED_UNITY );

			/* prepare device: set initial sample counter to zero */
			set_device_param( STUD_PARM_POSL , 0 );
			set_device_param( STUD_PARM_POSH , 0 );
			
			/* prepare device: set initial DSP buffer index to start of channel buffers */
			set_device_param( STUD_PARM_INITOFST , 0 );

			/* don't use packed mode */	
			set_device_param( STUD_PARM_PACKMODE , false );
			
			/* status and control is right aligned. use left aligned only for data. */
			set_host_ctrl_bits( ix,
								M_HCTR_HTF_MASK | M_HCTR_HRF_MASK ,
								M_HCTR_HTF1     | M_HCTR_HRF1 ); /* was 1 for right aligned */									

			for (channel = 0; channel < outputs_per_bd; channel++) {
				/* try to stop the noise! */
				fill_x_fast(ix,
							CHNL_BUF_ORG + channel,
							CHNL_BUF_STRIDE,
							0,
							CHNL_BUF_SIZE);

				/* enable input monitor */
				set_channel_param( 	STUD_PARM_INPMON,
									channel + ix * outputs_per_bd,
									0x8000000L / cards[ix].channels[channel].monitor_ramp );
				cards[ix].channels[channel].monitor_ts = system_time();
				/* enable output enable */
				set_channel_param( 	STUD_PARM_OUTENB,
									channel + ix * outputs_per_bd,
									0x8000000L / cards[ix].channels[channel].output_ramp );
				cards[ix].channels[channel].output_ts = system_time();
			}

			/* enable record enable */
			set_device_param( STUD_PARM_RECENB, cards[ix].record_enable);					
			/* enable the DAC */
			/* according to the source code */
			/* "lower so that Rch noise is less evident (DAC part bug)" */
			set_dac_params_MonMix_1111( ix, 12, true);
			set_dac_params_MonMix_1111( ix, 12, false);
								
		}
	}
	return card_count > 0 ? B_OK : ENODEV;
}


void
uninit_driver(void)
{
	int ix;
	int channel;
	
	ddprintf("sonorus: uninit_driver()\n");

	/* shut down input channels */
	set_device_param( STUD_PARM_RECENB , false );

	for (ix=0; ix<card_count; ix++) {
		/* shut down output channels */
		for (channel = 0; channel < outputs_per_bd; channel++) {
			set_channel_param(	STUD_PARM_OUTENB,
								channel + ix * outputs_per_bd ,
								0x8000000L / abs(cards[ix].channels[channel].output_ramp) );
		}	 	
	
		teardown_card(ix);
		remove_io_interrupt_handler(cards[ix].card_info.u.h0.interrupt_line, sonorus_interrupt, (void *)ix);
	}
	memset(&cards, 0, sizeof(cards));
	put_module(pci_name);
}


const char **
publish_devices(void)
{
	int ix;
	ddprintf("sonorus: publish_devices()\n");

	for (ix=0; names[ix]; ix++) {
		ddprintf("%s\n", names[ix]);
	}
	return (const char **)names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	ddprintf("sonorus: find_device()\n");

	for (ix=0; names[ix]; ix++) {
		if (!strcmp(name, names[ix])) {
			return &sonorus_hooks;
		}
	}
	return NULL;
}

static status_t sonorus_open(const char *name, uint32 flags, void **cookie);
static status_t sonorus_close(void *cookie);
static status_t sonorus_free(void *cookie);
static status_t sonorus_control(void *cookie, uint32 op, void *data, size_t len);
static status_t sonorus_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t sonorus_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks sonorus_hooks = {
    &sonorus_open,
    &sonorus_close,
    &sonorus_free,
    &sonorus_control,
    &sonorus_read,
    &sonorus_write,
	NULL,				/* select and scatter/gather */
	NULL,
	NULL,
	NULL
};



static status_t
sonorus_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	const char * val;
	int p;
	open_sonorus * card;
	status_t err;
	uint32 mode = (flags & 3) + 1; // rd = 1 wr = 2 rdwr = 3 multi_ctrl = 4

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
	if (p < 0 || p >= MAX_STUDIO_BD) {
		return EINVAL;
	}
	err = acquire_sem_etc(cards[p].open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}
	if (cards[p].open_mode && (mode & 3) ) {
		err = EPERM;
		ddprintf("sonorus: Too many opens\n");
		goto cleanup_1;
	}
	card = (open_sonorus *)malloc(sizeof(open_sonorus));
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
sonorus_close(
	void * cookie)
{
	open_sonorus * card = (open_sonorus *)cookie;
	status_t err;

	ddprintf("sonorus: sonorus_close()\n");

	err = acquire_sem_etc(card->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("sonorus: acquire_sem_etc failed in sonorus_close() \n");
		return err;
	}
	/* only rd, wr, and rdwr can start/stop data */
	if (card->mode & 3) {
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("sonorus: acquire_sem_etc failed in force_stop [%lx]\n", err);
		}
		else {
			err = sonorus_multi_force_stop(cookie);
			release_sem(card->device->control_sem);
		}

		card->device->open_mode &= ~card->mode;
	}	
	release_sem(card->device->open_sem);
	
	return B_OK;
}


static status_t
sonorus_free(
	void * cookie)
{
	open_sonorus * card = (open_sonorus *)cookie;
	status_t err;
	int ix;

	ddprintf("sonorus: sonorus_free()\n");

	err = acquire_sem_etc(card->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("sonorus: acquire_sem_etc failed in sonorus_free() [%lx]\n", err);
		return err;
	}
	ix = card->device->ix;
	free(card);
	release_sem(cards[ix].open_sem);
	
	return B_OK;
}

static status_t
sonorus_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	status_t err = B_OK;
	open_sonorus * card = (open_sonorus *)cookie;


	if (iop == B_MULTI_BUFFER_EXCHANGE && (card->mode & 3)) {
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("sonorus: acquire_sem_etc failed in multi_buffer_exchange [%lx]\n", err);
		}
		else {
			err = sonorus_multi_buffer_exchange(cookie, data, len);
			release_sem(card->device->control_sem);
		}	
		return err; 
	}

	switch (iop) {
	case B_MULTI_GET_EVENT_INFO: 
		ddprintf("sonorus: get_event_info \n");
		err = sonorus_multi_get_event_info(cookie, data, len);
		break;
	case B_MULTI_SET_EVENT_INFO:
		ddprintf("sonorus: set_event_info \n");
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in set_event [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_event_info(cookie, data, len);
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
		ddprintf("sonorus: get_event \n");
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in get_event [%lx]\n", err);
			}
			else {
				err = sonorus_multi_get_event(cookie, data, len);
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
		ddprintf("sonorus: get_description \n");
		err = sonorus_multi_get_description(cookie, data, len);
		break;
	case B_MULTI_GET_ENABLED_CHANNELS :
		ddprintf("sonorus: get_enabled_channels \n");
		err = sonorus_multi_get_enabled_channels(cookie, data, len);
		break;
	case B_MULTI_SET_ENABLED_CHANNELS : 
		ddprintf("sonorus: set_enabled_channels \n");
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_enabled_channels(cookie, data, len);
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
		ddprintf("sonorus: get_global_format \n");
		err = sonorus_multi_get_global_format(cookie, data, len);
		break;
	case B_MULTI_SET_GLOBAL_FORMAT :
		ddprintf("sonorus: set_global_format \n");
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_global_format(cookie, data, len);
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
		ddprintf("sonorus: get_buffers \n");
		if (card->mode & 3) {
			err = sonorus_multi_get_buffers(cookie, data, len);
		}
		else {
			err = B_PERMISSION_DENIED;
		}	
		break;
	case B_MULTI_SET_BUFFERS :
		ddprintf("sonorus: set_buffers \n");
		if (card->mode & 3) {
			err = sonorus_multi_set_buffers(cookie, data, len);
		}
		else {
			err = B_PERMISSION_DENIED;
		}	
		break;
	case B_MULTI_BUFFER_FORCE_STOP : 
		ddprintf("sonorus: force stop (sonorus.c)\n");
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in force_stop [%lx]\n", err);
			}
			else {
				err = sonorus_multi_force_stop(cookie);
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
		ddprintf("sonorus: get_mix \n");
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("sonorus: acquire_sem_etc failed in get_mix [%lx]\n", err);
		}
		else {
			err = sonorus_multi_get_mix(cookie, data, len);
			release_sem(card->device->control_sem);
		}	
		break;
	case B_MULTI_SET_MIX :
		ddprintf("sonorus: set_mix \n");	
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
		}
		else {
			err = sonorus_multi_set_mix(cookie, data, len);
			release_sem(card->device->control_sem);
		}
		break;
	case B_MULTI_LIST_MIX_CHANNELS:
		ddprintf("sonorus: list_mix_channels \n");
		err = sonorus_multi_list_mix_channels(cookie, data, len);
		break;
	case B_MULTI_LIST_MIX_CONTROLS:
		ddprintf("sonorus: list_mix_controls \n");
		err = sonorus_multi_list_mix_controls(cookie, data, len);
		break;
	case B_MULTI_LIST_MIX_CONNECTIONS:
		ddprintf("sonorus: list_mix_connections \n");
		err = sonorus_multi_list_mix_connections(cookie, data, len);
		break;
	case B_MULTI_LIST_MODES:
		ddprintf("sonorus: list_modes \n");
		err = sonorus_multi_list_modes(cookie, data, len);
		break;
	case B_MULTI_GET_MODE:
		ddprintf("sonorus: get_mode \n");
		err = sonorus_multi_get_mode(cookie, data, len);
		break;
	case B_MULTI_SET_MODE :
		ddprintf("sonorus: set_mode \n");	
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_mode(cookie, data, len);
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
		ddprintf("sonorus: list_extensions \n");
		err = sonorus_multi_list_extensions(cookie, data, len);
		break;
	case B_MULTI_GET_EXTENSION:
		ddprintf("sonorus: get_extension \n");
		err = sonorus_multi_get_extension(cookie, data, len);
		break;
	case B_MULTI_SET_EXTENSION :
		ddprintf("sonorus: set_extension \n");	
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_extension(cookie, data, len);
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
		ddprintf("sonorus: set_start_time \n");
#ifndef TESTING
		if (card->mode & 3)
#endif
		{
			err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
			if (err < B_OK) {
				dprintf("sonorus: acquire_sem_etc failed in sonorus_control() [%lx]\n", err);
			}
			else {
				err = sonorus_multi_set_start_time(cookie, data, len);
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
		dprintf("sonorus: unknown ioctl() %ld\n", iop);
		err = B_DEV_INVALID_IOCTL;;
		break;
	}

	return err;
}


static status_t
sonorus_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	if (!cookie || !data || !nread) {
		return EINVAL;
	}
	*nread = 0;
	return B_ERROR;
}


static status_t
sonorus_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	if (!cookie || !data || !nwritten) {
		return EINVAL;
	}
	*nwritten = 0;
	return B_ERROR;
}

static int32
sonorus_interrupt(
	void * data)
{
	int32 ix = (int32) data;
	int32 handled = B_UNHANDLED_INTERRUPT;

	if ( get_host_stat(ix) & M_HSTR_HINT ) 
	{
		acquire_spinlock(&cards[ix].spinlock);
		send_host_cmd(ix, HC_INTACK );
		release_spinlock(&cards[ix].spinlock);

		cards[ix].channels[outputs_per_bd].current_buffer++;
		cards[ix].channels[0].current_buffer++;
		release_sem_etc(cards[ix].irq_sem, 1, B_DO_NOT_RESCHEDULE);
		handled = B_INVOKE_SCHEDULER;
	}
#if DEBUG	
	if ( (get_host_stat(ix)>>8) & M_HSTR_HINT ) {
		dprintf("sonorus: BAD TIMING ON THE INTERRUPT\n");
		debugger("agh");
	}	 
#endif	
	return handled;
}
