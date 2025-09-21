/*includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>

#include <game_audio.h>
#include "xpress.h"

#include "ac97.h"

#define EXPORT __declspec(dllexport)
EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);

extern device_hooks xpress_hooks;

int32 api_version = B_CUR_DRIVER_API_VERSION;


#define MAGIC_PORT (0x920)
#define USING_INSYDE (0x00)
#define ONLY_ONE_OPENER (0x00)	


/* globals ************/
int card_count 		= 0;
xpress_dev cards[MAX_XPRESS_BD];


volatile struct _PRD_data prd_table_pb[PRD_DEPTH + 1];
volatile struct _PRD_data prd_table_cap[PRD_DEPTH + 1];

static char * init_names[MAX_XPRESS_BD+1] = {
	"audio/game/xpress/1",
	NULL
};
static char * names[MAX_XPRESS_BD+1];

pci_module_info	*pci = NULL;
const char pci_name[] = B_PCI_MODULE_NAME;

/* explicit declarations */
static int32 xpress_interrupt( void *);
static void  handle_interrupt( xpress_dev *, struct _stream_data *, sem_id *, int16 *);

/* xpress_hw.c */
inline uint32 PCI_CFG_RD( uchar, uchar, uchar, uchar, uchar);
inline void   PCI_CFG_WR( uchar, uchar, uchar, uchar, uchar, uint32);
inline uint8  PCI_IO_RD( int);
inline void	  PCI_IO_WR ( int, uint8);
inline uint16 PCI_IO_RD16( int);
inline void   PCI_IO_WR16( int, uint16);

/* xpress_ioctl.c */
status_t xpress_game_open_stream( void *, void *, size_t);
status_t xpress_game_queue_stream_buffer( void *, void *, size_t);
status_t xpress_game_run_streams( void *, void *, size_t);
status_t xpress_game_close_stream( void *, void *, size_t);
status_t xpress_game_open_stream_buffer( void *, void *, size_t);
status_t xpress_game_close_stream_buffer( void *, void *, size_t);
status_t xpress_game_get_info( void *, void *, size_t );
status_t xpress_game_get_adc_infos( void *, void *, size_t );
status_t xpress_game_get_dac_infos(	void *,	void *,	size_t );
status_t xpress_game_set_codec_formats( void *, void *, size_t );
status_t xpress_game_get_mixer_infos( void *, void *, size_t );
status_t xpress_game_get_mixer_controls( void *, void *, size_t );
status_t xpress_game_get_mixer_level_info( void *, void *, size_t );
status_t xpress_game_get_mixer_mux_info( void *, void *, size_t );
status_t xpress_game_get_mixer_enable_info( void *, void *, size_t );
status_t xpress_game_get_mixer_control_values( void *, void *, size_t );
status_t xpress_game_set_mixer_control_values( void *, void *, size_t );
status_t xpress_game_get_stream_timing(void *, void *, size_t );
status_t xpress_game_get_device_state( void *, void *, size_t );
status_t xpress_game_set_device_state( void *, void *, size_t );
status_t xpress_game_get_interface_info( void *, void *, size_t );
status_t xpress_game_get_interfaces( void *, void *, size_t );
//status_t xpress_game_get_stream_description(void *, void *, size_t );
status_t xpress_game_get_stream_controls( void *, void *, size_t );
status_t xpress_game_set_stream_controls( void *, void *, size_t );


/* xpress_ac97glue.c */
status_t xpress_init_codec(xpress_dev *, uchar, bool );
void	 xpress_uninit_codec(xpress_dev *, uchar );
status_t xpress_codec_write(void* host, uchar offset, uint16 value, uint16 mask );
void	 lock_ac97(xpress_dev *, uchar);
void	 unlock_ac97(xpress_dev *, uchar);


/* xpress_stream.c */
inline void start_stream_dma(xpress_dev *, struct _stream_data *);
inline void remove_queue_item(xpress_dev *,struct _stream_data *, sem_id *, int16 *);
status_t lock_stream(open_xpress * );
void     unlock_stream(open_xpress * );
inline void load_prd( struct _stream_data * , queue_item *);
status_t free_buffer( void *, void *);

/* xpress_mix.c */
void write_initial_cached_controls( xpress_dev *);
void read_initial_cached_controls( xpress_dev *);

/* Load firmaware as needed. */
/* called serialized */
static status_t
load_card(
	int ix)
{
	status_t err = B_OK;

	if (ix < 0 || ix >= card_count) {
		kprintf("xpress: load_card() called for bad index %d\n", ix);
		err = B_BAD_VALUE;
	}

	/* kill VSA and enable Native AUDIO */
	/* send 0E5h to I/O 920h            */
	/* the magic bullet!				*/
	/* ( for DT Research )              */
	{
		PCI_IO_WR( MAGIC_PORT, 0xE5 );
	}
	
	/* or if we are using Insyde ....   */
	/* VSA2                             */
	{
		/* allow status to 40:f0 */
		PCI_IO_WR16( 0xAC1C, 0xFC53 ); /* unlock */
		PCI_IO_WR16( 0xAC1C, 0x0100 ); /* index  audio class = 1, audio version = 0 */
		PCI_IO_WR16( 0xAC1E, 0x0000 ); /* value  set ver to 0 (dt compatible ) */

		/* enable native audio */
		PCI_IO_WR16( 0xAC1C, 0xFC53 ); /* unlock */
		PCI_IO_WR16( 0xAC1C, 0x0108 ); /* index  audio class = 1, audio int = 8 */
		PCI_IO_WR16( 0xAC1E, 0x0005 ); /* value  use int 5 */

	}	 
	ddprintf(("xpress: sh0ot the magic bullet.\n"));					


	return err;
}

/* called serialized; maps card and initializes struct */
static status_t
setup_card(
	int ix)
{
	char name[32];
	uint32 config = 0;
	int16 i;
	status_t err = B_OK;

	ddprintf(("xpress: setup_card(%d)\n", ix));

	/* zeroed in find_cards */
	cards[ix].ix = ix;
	sprintf(name, "xpress %d open_sem", ix);
	cards[ix].open_sem = create_sem(1, name);
	set_sem_owner(cards[ix].open_sem, B_SYSTEM_TEAM);
	sprintf(name, "xpress %d control_sem", ix);
	cards[ix].control_sem = create_sem(1, name);
	set_sem_owner(cards[ix].control_sem, B_SYSTEM_TEAM);
	sprintf(name, "xpress %d irq_sem", ix);
	cards[ix].irq_sem = create_sem(1, name);
	set_sem_owner(cards[ix].irq_sem, B_SYSTEM_TEAM);
	sprintf(name, "xpress %d stream_sem", ix);
	cards[ix].stream_sem = create_sem(1, name);
	set_sem_owner(cards[ix].stream_sem, B_SYSTEM_TEAM);
	sprintf(name, "xpress %d ac97_sem", ix);
	cards[ix].ac97sem[0] = create_sem(1, name);
	set_sem_owner(cards[ix].ac97sem[0], B_SYSTEM_TEAM);

	/* enable memory access */
	config = PCI_CFG_RD(
		cards[ix].pci_bus,
		cards[ix].pci_device,
		cards[ix].pci_function,
		0x04,
		4);
	config = config | 0x2;
	PCI_CFG_WR(
		cards[ix].pci_bus,
		cards[ix].pci_device,
		cards[ix].pci_function,
		0x04,
		4,
		config);

	/* get base address register from pci config space */	
	cards[ix].f3bar	= PCI_CFG_RD(cards[ix].pci_bus,
										cards[ix].pci_device,
										cards[ix].pci_function,
										0x10,
										4 );
	cards[ix].f3bar &= 0xfffffffe;									

	ddprintf(("xpress: bus %ld  device %ld function %ld int %ld\n", cards[ix].pci_bus, cards[ix].pci_device, cards[ix].pci_function, cards[ix].pci_interrupt));
	ddprintf(("xpress: f3bar %lx\n", cards[ix].f3bar));
	
	sprintf(name, "xpress %d memory", ix);
	cards[ix].f3_area = map_physical_memory(
		name, 
		(void*)(cards[ix].f3bar), 
		4096,
		B_ANY_KERNEL_ADDRESS, 
		B_READ_AREA | B_WRITE_AREA,
		(void **)(&cards[ix].f3) );

	ddprintf(("xpress: memory %lx mapped at %p\n",
		cards[ix].f3bar,
		cards[ix].f3));		

	err = cards[ix].f3_area < B_OK ? cards[ix].f3_area : B_OK;

	if (err == B_OK) {
		sprintf(name, "xpress %d 4f0", ix);
		cards[ix].fourf0_area = map_physical_memory(
			name, 
			(void*)(0x0), /* 40:f0 */ 
			4096,
			B_ANY_KERNEL_ADDRESS, 
			B_READ_AREA | B_WRITE_AREA ,
			(void **)(&cards[ix].fourf0) );

		cards[ix].fourf0  += 0x4f0;
		/* zero the memory */
		 atomic_and((uint32 *)(cards[ix].fourf0), 0x00000000);
		
		ddprintf(("xpress: memory %x mapped at %p\n",
			0x4f0,
			cards[ix].fourf0));		

		err = cards[ix].fourf0_area < B_OK ? cards[ix].fourf0_area : B_OK;

	}
	
	/* setup resource list		*/
	/* how many codecs?			*/
	/* properties of each		*/
	/* for now, we'll hardcode	*/
	/* all the values			*/
	/* not all values are used..*/
	{
		game_adc_info * padc = &(cards[ix].adc_info[0]);
		game_dac_info * pdac = &(cards[ix].dac_info[0]);
		game_mixer_info *pmix= &(cards[ix].mixer_info[0]);
		
		memset(padc, 0, sizeof(game_adc_info) * X_NUM_ADCS);
		padc->adc_id = GAME_MAKE_ADC_ID(0);
		padc->max_stream_count = X_NUM_ADC_STREAMS;
		padc->cur_stream_count = 0;
		padc->stream_flags = 0;
		padc->channel_counts = 0x2; /* stereo */
		padc->linked_dac_id = GAME_MAKE_DAC_ID(0);
		padc->linked_mixer_id = GAME_MAKE_MIXER_ID(0);
		/* DANGER DANGER DANGER DANGER DANGER DANGER DANGER */
		padc->min_chunk_frame_count = X_MIN_BUFF_ALLOC >> 2;
		padc->chunk_frame_count_increment = 1;
		padc->max_chunk_frame_count = (1024*64) >> 2;
		padc->cur_chunk_frame_count = padc->min_chunk_frame_count;
		strcpy(padc->name,"AC97 Capture");
		padc->frame_rates = B_SR_CVSR;
		padc->cvsr_min =  8000.0;
		padc->cvsr_max = 48000.0;
		padc->formats  = B_FMT_16BIT;
		padc->designations = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		padc->cur_frame_rate = B_SR_CVSR;
		padc->cur_cvsr = 0.0; /* fill it in AFTER the codec is initialized */
		padc->cur_format = B_FMT_16BIT;
		padc->cur_channel_count = 0x2;
		
		memset(pdac, 0, sizeof(game_dac_info) * X_NUM_DACS);
		pdac->dac_id = GAME_MAKE_DAC_ID(0);
		pdac->max_stream_count = X_NUM_DAC_STREAMS;
		pdac->cur_stream_count = 0;
		pdac->stream_flags = 0;
		pdac->channel_counts = 0x2; /* stereo */
		pdac->linked_adc_id = GAME_MAKE_ADC_ID(0);
		pdac->linked_mixer_id = GAME_MAKE_MIXER_ID(1);
		/* DANGER DANGER DANGER DANGER DANGER DANGER DANGER Will Robinson */
		pdac->min_chunk_frame_count = X_MIN_BUFF_ALLOC >> 2;
		pdac->chunk_frame_count_increment = 1;
		pdac->max_chunk_frame_count = (1024*64) >> 2;
		pdac->cur_chunk_frame_count = pdac->min_chunk_frame_count;
		strcpy(pdac->name,"AC97 Playback");
		pdac->frame_rates = B_SR_CVSR;
		pdac->cvsr_min =  8000.0;
		pdac->cvsr_max = 48000.0;
		pdac->formats  = B_FMT_16BIT;
		pdac->designations = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		pdac->cur_frame_rate = B_SR_CVSR;
		pdac->cur_cvsr = 0.0; /* fill it in AFTER the codec is initialized */
		pdac->cur_format = B_FMT_16BIT;
		pdac->cur_channel_count = 0x2;

		/* adc mixer */
		memset(pmix, 0, sizeof(game_mixer_info) * X_NUM_MIXERS);
		pmix->mixer_id = GAME_MAKE_MIXER_ID(0);
		pmix->linked_codec_id = padc->adc_id; 
		strcpy(pmix->name,"Input Mixer");
		pmix->control_count = X_NUM_ADC_CONTROLS;
		pmix->_reserved_ = 0x534F; /* so */

		/* dac mixer */
		pmix++;
		pmix->mixer_id = GAME_MAKE_MIXER_ID(1);
		pmix->linked_codec_id = pdac->dac_id; 
		strcpy(pmix->name,"Output Mixer");
		pmix->control_count = X_NUM_DAC_CONTROLS;
		pmix->_reserved_ = 0x534F; /* so */


	}
	for (i = 0; i < X_MAX_STREAMS; i++) {
		cards[ix].stream_data[i].stream_buffer_queue = NULL;
		cards[ix].stream_data[i].free_list  	= NULL;
		cards[ix].stream_data[i].prd_add 		= 0;
		cards[ix].stream_data[i].prd_remove 	= 0;
		cards[ix].stream_data[i].frame_count	= 0;
		cards[ix].stream_data[i].timestamp		= 0LL;
		cards[ix].stream_data[i].my_flags 		= X_DMA_STOPPED;
		cards[ix].stream_data[i].state 			= GAME_STREAM_STOPPED;
		cards[ix].stream_data[i].gos_stream_sem = B_ERROR;
		cards[ix].stream_data[i].gos_adc_dac_id = GAME_NO_ID;
		cards[ix].stream_data[i].bind_count 	= 0;
		/* no need to set owner to system_team... */
		sprintf(name, "xpress %d done_sem%d", ix,i);
		cards[ix].stream_data[i].all_done_sem 	= create_sem(0, name);
		cards[ix].stream_data[i].cookie 		= NULL;
	
	} 
	for (i = 0; i < X_MAX_BUFFERS; i++) {
		cards[ix].buffer_data[i].stream		= GAME_NO_ID;
		cards[ix].buffer_data[i].ref_count	= 0;
		cards[ix].buffer_data[i].area		= B_ERROR;
		cards[ix].buffer_data[i].offset		= 0;
		cards[ix].buffer_data[i].byte_size	= 0;
		cards[ix].buffer_data[i].virt_addr	= NULL;
		cards[ix].buffer_data[i].num_entries= 0;
		cards[ix].buffer_data[i].idx		= i;
		cards[ix].buffer_data[i].phys_entry	= NULL;
	} 
	cards[ix].current_buffer_count = 0;
	for (i = 0; i < PRD_DEPTH; i++) { 
		prd_table_pb[i].buff_phys_addr = NULL;
		prd_table_pb[i].size = 0;
		prd_table_pb[i].flags= X_EOT;
		prd_table_cap[i].buff_phys_addr = NULL;
		prd_table_cap[i].size = 0;
		prd_table_cap[i].flags= X_EOT;
	}
	{
		physical_entry pe;
		get_memory_map( (void *)&prd_table_pb[0], 4096, &pe, 1); 
	
		prd_table_pb[PRD_DEPTH].buff_phys_addr = pe.address ;
		prd_table_pb[PRD_DEPTH].size = 0;
		prd_table_pb[PRD_DEPTH].flags= X_JMP;
	
		get_memory_map( (void *)&prd_table_cap[0], 4096, &pe, 1); 
	
		prd_table_cap[PRD_DEPTH].buff_phys_addr = pe.address ;
		prd_table_cap[PRD_DEPTH].size = 0;
		prd_table_cap[PRD_DEPTH].flags= X_JMP;
		ddprintf(("xpress: pb prd %p cap %p \n",prd_table_pb[PRD_DEPTH].buff_phys_addr,
												prd_table_cap[PRD_DEPTH].buff_phys_addr));
	}
	
	return err;
}

/* called serialized */
static void
teardown_card(
	int ix)
{
	int16 i;
	ddprintf(("xpress: teardown_card(%d)\n", ix));
	/* disable ints */
	/*	clean up resources */
	if (cards[ix].open_sem >= B_OK) 
		delete_sem(cards[ix].open_sem);
	if (cards[ix].control_sem >= B_OK) 
		delete_sem(cards[ix].control_sem);
	if (cards[ix].irq_sem >= B_OK) 
		delete_sem(cards[ix].irq_sem);
	if (cards[ix].f3_area >= B_OK) 
		delete_area(cards[ix].f3_area);
	if (cards[ix].fourf0_area >= B_OK) 
		delete_area(cards[ix].fourf0_area);
	if (cards[ix].stream_sem >= B_OK) 
		delete_sem(cards[ix].stream_sem);
	if (cards[ix].ac97sem[0] >= B_OK) 
		delete_sem(cards[ix].ac97sem[0]);
	for (i = 0; i < X_MAX_STREAMS; i++) {
		if (cards[ix].stream_data[i].all_done_sem >= B_OK)
			delete_sem(cards[ix].stream_data[i].all_done_sem);
	} 
		
	memset(&cards[ix], 0, sizeof(xpress_dev));
	cards[ix].open_sem 			= B_ERROR;
	cards[ix].control_sem		= B_ERROR;
	cards[ix].irq_sem			= B_ERROR;
	cards[ix].f3_area			= B_ERROR;
	cards[ix].fourf0_area		= B_ERROR;
	cards[ix].stream_sem		= B_ERROR;
	cards[ix].ac97sem[0]		= B_ERROR;
	for (i = 0; i < X_MAX_STREAMS; i++) {
		cards[ix].stream_data[i].all_done_sem = B_ERROR;
	} 

}

static int
find_cards()
{
	int ix;
	pci_info info;
	int cc = 0;

	memset(cards, 0, sizeof(cards));
	for (ix=0; !(*pci->get_nth_pci_info)(ix, &info); ix++) {
		if (info.vendor_id != XPRESS_VENDOR_ID || info.device_id != XPRESS_DEVICE_ID) {
			continue;
		}
		if ((info.header_type & PCI_header_type_mask) != PCI_header_type_generic) {
			continue;
		}

		if (cc >= MAX_XPRESS_BD) {
			dprintf("xpress: there are more than %d xpress cards installed!\n", MAX_XPRESS_BD);
			break;
		}
		ddprintf(("xpress: found card %d\n", cc));
		cards[cc].pci_bus		= info.bus;
		cards[cc].pci_device	= info.device;
		cards[cc].pci_function	= info.function;
		cards[cc].pci_interrupt	= info.u.h0.interrupt_line;
	
		ddprintf(("xpress: the int in find cards is %d\n",info.u.h0.interrupt_line));
		ddprintf(("xpress: changing interrupt to 5\n"));
		cards[cc].pci_interrupt = 0x5;

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

#if DEBUG
	set_dprintf_enabled(1);
#endif

	ddprintf(("/*************************************************************/\n"));
	ddprintf(("/*                XPRESS NEW HARDWARE LOAD                   */\n"));
	ddprintf(("/*************************************************************/\n"));
	ddprintf(("Magic Port is 0x%x\n",MAGIC_PORT));
	ddprintf(("xpress: init_hardware() %s %s\n",__TIME__,__DATE__));

	if (get_module(pci_name, (module_info **) &pci)) {
		dprintf("xpress: module '%s' not found\n", pci_name);
		pci = NULL;
		return ENOSYS;
	}


	/* figure out what cards are on the bus and set the card_count accordingly */
	card_count = find_cards();
	if (card_count < 1) {
		ddprintf(("xpress: no cards\n"));
		return ENODEV;
	}

	/* reset the cards to a known state */
	for (ix = 0; ix<card_count; ix++) {
		err = setup_card(ix);
		if (err == B_OK) {
			err = load_card(ix);	
			/* find and init codecs here */
			err = xpress_init_codec(&(cards[ix]), 0, true);
			if (err == B_OK) {
				write_initial_cached_controls( &cards[ix] );
			}
		}
		teardown_card(ix);
		if (err != B_OK) {
			break; /* bail on error */
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
	ddprintf(("xpress: init_driver() %s %s\n",__TIME__,__DATE__));

	if (get_module(pci_name, (module_info **) &pci)) {
		pci = NULL;
		return ENOSYS;
	}
	
	/* figure out what cards are there and fill in pci info  */
	card_count = find_cards();
	
	if (card_count > 0) {
		for (ix=0; ix<card_count; ix++) {
			ddprintf(("xpress: ix %d  card_count %d \n", ix, card_count));		

			err = setup_card(ix);
			if (err == B_OK) {
				err = load_card(ix);	
			}
			if (err < B_OK) {
				dprintf("xpress: card %d is not cooperating, no more cards will be added\n", ix);
				teardown_card(ix);
				card_count = ix;
			}
			
		}	
		for (ix=0; ix<card_count; ix++) {
			/* init device */

			/* set up int masks */
			install_io_interrupt_handler(cards[ix].pci_interrupt, xpress_interrupt, (void *)ix, 0);
			ddprintf(("xpress: int handler %p\n",xpress_interrupt));
			/* initialize any cached controls */
			xpress_init_codec(&(cards[ix]), 0, false);
			read_initial_cached_controls( &cards[ix] );
			/* we now have a sample rate... */
			{
				game_adc_info * padc = &(cards[ix].adc_info[0]);
				game_dac_info * pdac = &(cards[ix].dac_info[0]);
				/* worst case: a second opener changes the sample rate  */
				/* while we are reading. Our read gets messed up and we */
				/* return a bad value. It's not an issue, but to be sure*/
				/* we lock.                                             */
				lock_ac97(&cards[ix], cards[ix].xhost[0].index);
					padc->cur_cvsr = (float) cards[ix].ccc[0x32]; /* adc */
					pdac->cur_cvsr = (float) cards[ix].ccc[0x2c]; /* dac */
				unlock_ac97(&cards[ix], cards[ix].xhost[0].index);
				ddprintf(("xpress: sample rates 0x%x 0x%x\n",cards[ix].ccc[0x2c],cards[ix].ccc[0x32]));
			}	
		}	
	}
	return card_count > 0 ? B_OK : ENODEV;
}


void
uninit_driver(void)
{
	int ix;

	ddprintf(("xpress: uninit_driver()\n"));

	for (ix=0; ix<card_count; ix++) {
		remove_io_interrupt_handler(cards[ix].pci_interrupt, xpress_interrupt, (void *)ix);
		xpress_uninit_codec(&(cards[ix]), 0);
		teardown_card(ix);
	}
	//teardown_card memsets cards
	if (pci) {
		put_module(pci_name);
		pci = NULL;
	}
}


const char **
publish_devices(void)
{
	int ix;
	ddprintf(("xpress: publish_devices()\n"));

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

	ddprintf(("xpress: find_device()\n"));

	for (ix=0; names[ix]; ix++) {
		if (!strcmp(name, names[ix])) {
			return &xpress_hooks;
		}
	}
	return NULL;
}

static status_t xpress_open(const char *name, uint32 flags, void **cookie);
static status_t xpress_close(void *cookie);
static status_t xpress_free(void *cookie);
static status_t xpress_control(void *cookie, uint32 op, void *data, size_t len);
static status_t xpress_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t xpress_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks xpress_hooks = {
    &xpress_open,
    &xpress_close,
    &xpress_free,
    &xpress_control,
    &xpress_read,
    &xpress_write,
	NULL,				/* select and scatter/gather */
	NULL,
	NULL,
	NULL
};



static status_t
xpress_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	const char * val;
	int p;
	open_xpress * card;
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
	if (p < 0 || p >= MAX_XPRESS_BD) {
		return EINVAL;
	}

	err = acquire_sem_etc(cards[p].open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		return err;
	}
	
#if ONLY_ONE_OPENER	
	if (cards[p].open_mode && (mode & 3) ) {
		err = EPERM;
		ddprintf(("xpress: Too many opens\n"));
		goto cleanup_1;
	}
#endif

	card = (open_xpress *)malloc(sizeof(open_xpress));
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
xpress_close(
	void * cookie)
{
	open_xpress * card = (open_xpress *)cookie;
	int16 i;
	status_t err;

	ddprintf(("xpress: xpress_close()\n"));

	err = acquire_sem_etc(card->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("xpress: acquire_sem_etc failed in xpress_close() \n");
		return err;
	}
	/* only rd, wr, and rdwr can start/stop data */
	if (card->mode & 3) {
		err = acquire_sem_etc(card->device->control_sem, 1, B_CAN_INTERRUPT, 0);
		if (err < B_OK) {
			dprintf("xpress: acquire_sem_etc failed in force_stop [%lx]\n", err);
		}
		else {
			uint16 found_cookie = 0;
			for (i = 0; i < X_MAX_STREAMS; i++) {
				lock_stream(card);
					if (card->device->stream_data[i].cookie == cookie) {
						found_cookie = 1;				
					}
					else {
						found_cookie = 0;
					}				
				unlock_stream(card);
				if (found_cookie) {	
					game_close_stream gcs;
					gcs.info_size = sizeof(game_close_stream);
					gcs.stream = GAME_MAKE_STREAM_ID(i);
					gcs.flags = GAME_CLOSE_FLUSH_DATA;
					err = xpress_game_close_stream((void *)card, (void *)&gcs, 0);
				}
			}
			release_sem(card->device->control_sem);
		}

		card->device->open_mode &= ~card->mode;
	}	
	release_sem(card->device->open_sem);
	
	return err;
}


static status_t
xpress_free(
	void * cookie)
{
	open_xpress * card_open = (open_xpress *)cookie;
	status_t err;
	int ix,i;

	ddprintf(("xpress: xpress_free()\n"));

	err = acquire_sem_etc(card_open->device->open_sem, 1, B_CAN_INTERRUPT, 0);
	if (err < B_OK) {
		dprintf("xpress: acquire_sem_etc failed in xpress_free() [%lx]\n", err);
		return err;
	}
	
	/* free all buffers */
	for (i=0; i < X_MAX_BUFFERS; i++) {
		if (card_open->device->buffer_data[i].area != B_ERROR) {
			game_close_stream_buffer gcsb;
			gcsb.info_size = sizeof(game_close_stream_buffer);
			gcsb.buffer = GAME_MAKE_BUFFER_ID(i);
			/* force it */
			card_open->device->buffer_data[i].ref_count = 0;
			free_buffer((void *)card_open, (void *) &gcsb);
		}
	}
	
	ix = card_open->device->ix;
	free(card_open);
	release_sem(cards[ix].open_sem);
	
	return B_OK;
}



static status_t
lock_control(xpress_dev * pdev)
{
	return B_OK;
}

static status_t
unlock_control(xpress_dev * pdev)
{
	return B_OK;
}	



static status_t
xpress_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	status_t err = B_OK;
	open_xpress * card = (open_xpress *)cookie;

	switch (iop) {

	/* yo, lock_control is a nop for now.... */
	/* there is no need to lock individual   */
	/* i/o controls.                         */
	
	case GAME_OPEN_STREAM:
		ddprintf(("xpress: game_open_stream\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_open_stream [%lx]\n", err);
			}
			else {
				err = xpress_game_open_stream(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_QUEUE_STREAM_BUFFER:
//		ddprintf(("xpress: game_queue_stream_buffer\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_queue_stream_buffer [%lx]\n", err);
			}
			else {
				err = xpress_game_queue_stream_buffer(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_RUN_STREAMS:
//		ddprintf(("xpress: game_run_streams\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_run_stream [%lx]\n", err);
			}
			else {
				err = xpress_game_run_streams(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;


	case GAME_CLOSE_STREAM:
		ddprintf(("xpress: game_close_stream\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_close_stream [%lx]\n", err);
			}
			else {
				err = xpress_game_close_stream(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_OPEN_STREAM_BUFFER:
//		ddprintf(("xpress: game_open_stream_buffer\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_open_stream_buffer [%lx]\n", err);
			}
			else {
				err = xpress_game_open_stream_buffer(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			ddprintf(("xpress: permission denied\n"));
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_CLOSE_STREAM_BUFFER:
//		ddprintf(("xpress: game_close_stream_buffer\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_close_stream_buffer [%lx]\n", err);
			}
			else {
				err = xpress_game_close_stream_buffer(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;


	case GAME_GET_INFO:
		ddprintf(("xpress: game_get_info\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_info [%lx]\n", err);
		}
		else {
			err = xpress_game_get_info(cookie, data, len);
			unlock_control(card->device);
		}	
		break;


	case GAME_GET_DAC_INFOS:
		ddprintf(("xpress: game_get_dac_infos\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_dac_infos [%lx]\n", err);
		}
		else {
			err = xpress_game_get_dac_infos(cookie, data, len);
			unlock_control(card->device);
		}	
		break;


	case GAME_GET_ADC_INFOS:
		ddprintf(("xpress: game_get_adc_infos\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_adc_infos [%lx]\n", err);
		}
		else {
			err = xpress_game_get_adc_infos(cookie, data, len);
			unlock_control(card->device);
		}	
		break;


	case GAME_SET_CODEC_FORMATS:
		ddprintf(("xpress: game_set_codec_formats\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_set_codec_formats [%lx]\n", err);
			}
			else {
				err = xpress_game_set_codec_formats(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_GET_MIXER_INFOS:
		ddprintf(("xpress: game_get_mixer_infos\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_infos [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_infos(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_MIXER_CONTROLS:
		ddprintf(("xpress: game_get_mixer_controls\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_controls [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_controls(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_MIXER_LEVEL_INFO:
		ddprintf(("xpress: game_get_mixer_level_info\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_level_info [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_level_info(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_MIXER_MUX_INFO:
		ddprintf(("xpress: game_get_mixer_mux_info\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_mux_info [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_mux_info(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_MIXER_ENABLE_INFO:
		ddprintf(("xpress: game_get_mixer_enable_info\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_enable_info [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_enable_info(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_MIXER_CONTROL_VALUES:
//		ddprintf(("xpress: game_get_mixer_control_values\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_mixer_control_values [%lx]\n", err);
		}
		else {
			err = xpress_game_get_mixer_control_values(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_SET_MIXER_CONTROL_VALUES:
//		ddprintf(("xpress: game_set_mixer_control_values\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_set_mixer_control_values [%lx]\n", err);
			}
			else {
				err = xpress_game_set_mixer_control_values(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_GET_STREAM_TIMING:
		ddprintf(("xpress: game_get_stream_timing\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_stream_timing [%lx]\n", err);
		}
		else {
			err = xpress_game_get_stream_timing(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_DEVICE_STATE:
		ddprintf(("xpress: game_get_device_state\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_device_state [%lx]\n", err);
		}
		else {
			err = xpress_game_get_device_state(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_SET_DEVICE_STATE:
		ddprintf(("xpress: game_set_device_state\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_set_device_state [%lx]\n", err);
			}
			else {
				err = xpress_game_set_device_state(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_GET_INTERFACE_INFO:
		ddprintf(("xpress: game_get_infterface_info\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_interface_info [%lx]\n", err);
		}
		else {
			err = xpress_game_get_interface_info(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_GET_INTERFACES:
		ddprintf(("xpress: game_get_interfaces\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_interfaces [%lx]\n", err);
		}
		else {
			err = xpress_game_get_interfaces(cookie, data, len);
			unlock_control(card->device);
		}	
		break;


#if 0
	case GAME_GET_STREAM_DESCRIPTION:
		ddprintf(("xpress: game_get_stream_description\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_stream_description [%lx]\n", err);
		}
		else {
			err = xpress_game_get_stream_description(cookie, data, len);
			unlock_control(card->device);
		}	
		break;
#endif

	case GAME_GET_STREAM_CONTROLS:
		ddprintf(("xpress: game_get_stream_controls\n"));
		err = lock_control(card->device);
		if (err < B_OK) {
			dprintf("xpress: lock_control failed in game_get_stream_controls [%lx]\n", err);
		}
		else {
			err = xpress_game_get_stream_controls(cookie, data, len);
			unlock_control(card->device);
		}	
		break;

	case GAME_SET_STREAM_CONTROLS:
		ddprintf(("xpress: game_set_stream_controls\n"));
#ifndef TESTING		
		if (card->mode & 3)
#endif		
		{
			err = lock_control(card->device);
			if (err < B_OK) {
				dprintf("xpress: lock_control failed in game_set_stream_controls [%lx]\n", err);
			}
			else {
				err = xpress_game_set_stream_controls(cookie, data, len);
				unlock_control(card->device);
			}	
		}
#ifndef TESTING		
		else {
			err = B_PERMISSION_DENIED;
		}
#endif			
		break;

	case GAME_MIXER_UNUSED_OPCODE_1_:
		break;
	case GAME_GET_STREAM_CAPABILITIES:
#warning implement me later
		break;

	default:
		dprintf("xpress: unknown ioctl() %ld\n", iop);
		err = B_DEV_INVALID_IOCTL;;
		break;
	}

	return err;
}


static status_t
xpress_read(
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
xpress_write(
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
xpress_interrupt(
	void * data)
{
	int32 ix = (int32) data;
	int32 handled = B_UNHANDLED_INTERRUPT;
	uint32 t2 = 0;
	struct _stream_data * pbstream = NULL;
	struct _stream_data * capstream = NULL;
	sem_id release_sem[PRD_DEPTH];
	int16 release_me = 0;


	acquire_spinlock(&(cards[ix].spinlock));
//kprintf("{");
	t2 = atomic_and((uint32 *)(cards[ix].fourf0), 0x00000000);		

	if (t2 & 0x0003030c) { /* ours */
		cards[ix].timestamp = system_time();
//kprintf("$");

		if (t2 & 0x00030000) kprintf("Congratulations! You have locked the PCI bus\n");	

		/* bm 0 (playback) caused the interrupt */
		if (t2 & 0x00010104) {
			/* find the stream */
			if (GAME_IS_DAC(cards[ix].stream_data[0].gos_adc_dac_id) &&
				(cards[ix].stream_data[0].gos_adc_dac_id != GAME_NO_ID)) {
				pbstream = &(cards[ix].stream_data[0]);
			}
			else {
				if (GAME_IS_DAC(cards[ix].stream_data[1].gos_adc_dac_id) &&
					(cards[ix].stream_data[1].gos_adc_dac_id != GAME_NO_ID)) {
					pbstream = &(cards[ix].stream_data[1]);
				}
			}
		}	

		/* bm 1 (capture) caused the interrupt */
		if (t2 & 0x00020208) {
			/* find the stream */
			if (GAME_IS_ADC(cards[ix].stream_data[0].gos_adc_dac_id) &&
				(cards[ix].stream_data[0].gos_adc_dac_id != GAME_NO_ID)) {
				capstream = &(cards[ix].stream_data[0]);
			}
			else {
				if (GAME_IS_ADC(cards[ix].stream_data[1].gos_adc_dac_id) &&
					(cards[ix].stream_data[1].gos_adc_dac_id != GAME_NO_ID)) {
					capstream = &(cards[ix].stream_data[1]);
				}
			}
		}	

		if (pbstream) {
			handle_interrupt(&cards[ix], pbstream, &release_sem[0], &release_me);
		}
		if (capstream){
			handle_interrupt(&cards[ix], capstream, &release_sem[0], &release_me);
		}			
		handled = B_INVOKE_SCHEDULER;
		
	} /* ours */
	
	release_spinlock(&(cards[ix].spinlock));

	while (release_me--) {
//kprintf("R");
		if (release_sem[release_me] != B_ERROR) {
			release_sem_etc(release_sem[release_me], 1, B_DO_NOT_RESCHEDULE);
		}
	}	
//kprintf("}");
	return handled;
}

static void
handle_interrupt(
				xpress_dev * pdev,
				struct _stream_data * pbstream,
				sem_id * prelease_sem,
				int16 *  prelease_me)
{
	queue_item * pqi = NULL;
	uint32 current_slot = 0;

	if (pbstream != NULL) {		

		/* update the prds */
		/* this should solve the missed interrupts problem */
		if (GAME_IS_DAC(pbstream->gos_adc_dac_id)) {
			current_slot = ((MEM_RD32(pdev->f3 + 0x24) - 8) - (uint32)(pbstream->prd_table[PRD_DEPTH].buff_phys_addr)) >> 3;
		}
		else {
			IKASSERT((GAME_IS_ADC(pbstream->gos_adc_dac_id)));
			current_slot = ((MEM_RD32(pdev->f3 + 0x2C) - 8) - (uint32)(pbstream->prd_table[PRD_DEPTH].buff_phys_addr)) >> 3;
		}	

		while ((pbstream->prd_remove % PRD_DEPTH) != current_slot)
		{
			/* tally frames played/captured */
			pbstream->frame_count += ((pbstream->prd_table[pbstream->prd_remove % PRD_DEPTH].size) >> 2);

			/* remove the queue item */
			remove_queue_item( pdev, pbstream, prelease_sem, prelease_me);
		
			pbstream->prd_table[pbstream->prd_remove % PRD_DEPTH].flags          = X_EOT ;
			pbstream->prd_table[pbstream->prd_remove % PRD_DEPTH].size           = 0; 
			pbstream->prd_table[pbstream->prd_remove % PRD_DEPTH].buff_phys_addr = 0; 
			pbstream->prd_remove++;
		}
		pbstream->timestamp = pdev->timestamp;

		IKASSERT((pbstream->prd_remove <= pbstream->prd_add));

		/* release the all done semaphore.....          */
		if (pbstream->prd_table[pbstream->prd_remove % PRD_DEPTH].flags & X_EOT) {
			if (pbstream->my_flags & (X_DMA_DONE_SEM | X_CLOSING)) {
				prelease_sem[(*prelease_me)++] = pbstream->all_done_sem;
//kprintf("*");
			}
//else kprintf("@");			
			pbstream->my_flags |= X_DMA_STOPPED;
//kprintf("!");
		}

		/* move buffers from queue to prds iff we are RUNNING*/
		if (pbstream->state & GAME_STREAM_RUNNING) {
			load_prd( pbstream, pbstream->stream_buffer_queue);
		}
		
		/* if the DMA is X_DMA_STOPPED && the queue is not empty, */
		/* restart the DMA                                        */
		pqi = pbstream->stream_buffer_queue;
		if ((pqi != NULL) && (pbstream->my_flags & X_DMA_STOPPED) &&
			(pbstream->state & GAME_STREAM_RUNNING) && (!(pbstream->my_flags & X_CLOSING))) {			
			if (GAME_IS_DAC(pbstream->gos_adc_dac_id)) {
				MEM_WR32(pdev->f3 + 0x24, ((uchar *)(pbstream->prd_table[PRD_DEPTH].buff_phys_addr)) + (pqi->in_prd * sizeof(struct _PRD_data)));
			}
			else {
				MEM_WR32(pdev->f3 + 0x2c, ((uchar *)(pbstream->prd_table[PRD_DEPTH].buff_phys_addr)) + (pqi->in_prd * sizeof(struct _PRD_data)));
			}	
//kprintf("#");
			start_stream_dma(pdev, pbstream);
		}		
			
	} /* if pbstream != NULL */
return;
}