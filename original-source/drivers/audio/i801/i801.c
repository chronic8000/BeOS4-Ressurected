/************************************************************************/
/*                                                                      */
/*                              i801.c                              	*/
/*                                                                      */
/* copyright 1999 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>
#include <isapnp.h>
#include <config_manager.h>

#include <stdio.h>
#include <stdlib.h>

#include "audio_drv.h"
#include "misc.h"
#include "debug.h"
#include <ac97_module.h>
#include "i801.h"

#define AC97_BAD_VALUE		(0xffff)

//#define BUFFER_SIZE 2048
#define DEFAULT_BUF_SIZE B_PAGE_SIZE*4
#define SIZE_TO_LENGTH(x) ((x)>>1)

#define ENABLE_PLAYBACK 1
#define ENABLE_CAPTURE 1


extern pci_module_info *pci;

typedef struct _timers_data_t {
	vuint64 IntCount;
	bigtime_t SystemClock;
	int32 Lock;
} timers_data_t;

typedef struct SGTableEnt {
	void *		base;
	uint16		count;
	uint16		cmd;
} SGTableEnt;


typedef struct _i801 {

	timers_data_t timers_data;	
	ac97_t codec;

	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	void * PcmPlaybackBuf;

	area_id PcmCaptureAreaId;
	size_t PcmCaptureBufSize; 
	void * PcmCaptureBuf;

	area_id sgTablesAreaID;
	SGTableEnt * sgPlaybackTable;
	SGTableEnt * sgCaptureTable;

	uint16 NAMBAR;
	uint16 NABMBAR;
	
	sem_id ac97sem;
	int32 ac97ben;

	int32 SRHz;
	int32 playbackLastValid;
	int32 captureLastValid;
	long midi_interrupt;
	struct timer midi_timer;
	sound_card_t * self;
} i801_hw; 

bool g_enableMPU = true;


static status_t 	i801_Init(sound_card_t * card);
static status_t 	i801_Uninit(sound_card_t * card);
static void 		i801_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	i801_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		i801_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		i801_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		i801_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		i801_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		i801_StartPcm(sound_card_t *card);
static void 		i801_StopPcm(sound_card_t *card);
static status_t		i801_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		i801_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		i801_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		i801_InitJoystick(sound_card_t * card);
static void 		i801_enable_gameport(sound_card_t * card);
static void 		i801_disable_gameport(sound_card_t * card);

static status_t 	i801_InitMPU401(sound_card_t * card);
static void 		i801_enable_mpu401(sound_card_t *card);
static void 		i801_disable_mpu401(sound_card_t *card);
static void 		i801_enable_mpu401_interrupts(sound_card_t *card);
static void 		i801_disable_mpu401_interrupts(sound_card_t *card);






static sound_card_functions_t i801_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&i801_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&i801_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&i801_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&i801_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&i801_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&i801_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&i801_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&i801_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&i801_SetPlaybackDmaBuf,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&i801_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&i801_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&i801_SetCaptureDmaBuf,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&i801_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&i801_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&i801_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&i801_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&i801_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&i801_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&i801_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&i801_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&i801_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
} ;



card_descrtiptor_t i801_audio_descr = {
	"i801",
	PCI, 
	&i801_functions,
	0x8086, // Intel Corporation 
	0x2415 // i801AA AC97,
};
card_descrtiptor_t i801e_audio_descr = {
	"i801e",
	PCI, 
	&i801_functions,
	0x8086, // Intel Corporation 
	0x2425 // i801AB AC97,
};

 
static status_t 	i801_InitCodec(sound_card_t *card);
static void 		i801_UninitCodec(sound_card_t *card);
static uint16 		i801_CodecRead(void* host, uchar offset);
static status_t 	i801_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);


/* --- 
	Interrupt service routines
	---- */

static int32 i801_mpu_isr(void * data)
{
	int32 ret = B_UNHANDLED_INTERRUPT;
	sound_card_t * card = (sound_card_t *)data;

//	kprintf("m");
	if ((*card->midi_interrupt)(card)) {
		ret = B_INVOKE_SCHEDULER;
//		kprintf("n");
	}
	return ret;
}

int32
i801_mpu401_timer_hook(
	struct timer * t)
{
	return i801_mpu_isr(*(void**)(t+1));
}


static uint32 b0count=0,b1count=0,count=0;

static int32 i801_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;
	i801_hw * it = (i801_hw *)card->hw;
	uint32 mask, pinfo, cinfo;

	acquire_spinlock(&card->bus.pci.lock);
	mask = (*pci->read_io_32)(it->NABMBAR+I801_GLOBAL_STATUS);
	/* clear interrupt bits */
	if (mask & I801_GS_POINT) {
		pinfo = (*pci->read_io_32)(it->NABMBAR+I801_PLAYBACK_INFO);
		(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_SR, I801_SR_BCIS);
		(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_LVIX, I801_INFO_CIX(pinfo)-1);
	}
	if (mask & I801_GS_PIINT) {
		cinfo = (*pci->read_io_32)(it->NABMBAR+I801_CAPTURE_INFO);
		(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_SR, I801_SR_BCIS);
		(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_LVIX, I801_INFO_CIX(cinfo)-1);
	}
	release_spinlock(&card->bus.pci.lock);

	if (mask & I801_GS_POINT) {
		int half;
		bigtime_t time = system_time();
		ret = B_INVOKE_SCHEDULER;
		acquire_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);	
		((i801_hw*)card->hw)->timers_data.SystemClock = time;
		((i801_hw*)card->hw)->timers_data.IntCount++;
		release_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);
		half = 1 - (I801_INFO_CIX(pinfo) & 1);
		(*card->pcm_playback_interrupt)(card, 1 - ((I801_INFO_CIX(pinfo) & 1)));
		//kprintf("%c", '1'+half);
	}
	if (mask & I801_GS_PIINT) {
		ret = B_INVOKE_SCHEDULER;
		(*card->pcm_capture_interrupt)(card, 1 - ((I801_INFO_CIX(cinfo) & 1)));
	}

//	if (!(mask & (I801_GS_POINT|I801_GS_PIINT))) {
//		kprintf("x");
//	}

	return ret;
#if 0
	uint32 istat;
	static int half = 0;
	static int chalf= 0;
	uint32 phys_p;
	static char buf[2];

	acquire_spinlock(&card->bus.pci.lock);
	phys_p = READ_REG(((i801_hw *)card->hw)->BA0, BA0_HSAR) & 0xfff;
	istat = READ_REG(((i801_hw*)card->hw)->BA0, BA0_HISR);
	buf[0] = '0'+(istat&3)+((istat&0x100000)>>18);
//	kprintf(buf);
	if (!(istat & (0x1|0x2|0x100000))) {	/*	not one of ours	*/
		release_spinlock(&card->bus.pci.lock);
		return ret;
	}
	//	ask another interrupt
	ret = B_HANDLED_INTERRUPT;
	WRITE_REG(((i801_hw*)card->hw)->BA0, BA0_HICR, 0x3, -1);
	release_spinlock(&card->bus.pci.lock);

	//fixme:	save time information for the clocks
	if (istat & 0x1) {
		//fixme:	figure out which half of the playback buffer

#if 1
	{// Store internal time for synchronization 
		bigtime_t time = system_time();
			
		acquire_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);	
		((i801_hw*)card->hw)->timers_data.SystemClock = time;
		((i801_hw*)card->hw)->timers_data.IntCount++;
		release_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);
	}			
#endif

		half = !half;
		buf[0] = 'A'+half;
//		kprintf(buf);
		//fixme:	call the card->pcm_playback_interrupt function
		if ((*card->pcm_playback_interrupt)(card, half))
			ret = B_INVOKE_SCHEDULER;
	}
	if (istat & 0x2) {
		//fixme:	figure out which half of the capture buffer
//		kprintf("C");
		chalf = !chalf;
		if ((*card->pcm_capture_interrupt)(card, chalf))
			ret = B_INVOKE_SCHEDULER;
	}
	if (istat & 0x100000) {
		//fixme:	MIDI interrupt
	}
	return ret;

#endif
}


static void i801_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
#if 1
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((i801_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((i801_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((i801_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = ((i801_hw*)card->hw)->PcmPlaybackBufSize 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((i801_hw*)card->hw)->SRHz) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
#endif
}

static void i801_enable_mpu401(sound_card_t *card)
{
}

static void i801_disable_mpu401(sound_card_t *card)
{
}

static void i801_enable_mpu401_interrupts(sound_card_t *card)
{
	i801_hw * it = (i801_hw *)card->hw;
	dprintf("enable i801 MPU int (IRQ %d)\n", it->midi_interrupt);
	install_io_interrupt_handler(it->midi_interrupt, i801_mpu_isr, card, 0);
	memset(&it->midi_timer, 0, sizeof(it->midi_timer));
	it->self = card;
	add_timer(&it->midi_timer, i801_mpu401_timer_hook, 1000L, B_PERIODIC_TIMER);
}

static void i801_disable_mpu401_interrupts(sound_card_t *card)
{
	i801_hw * it = (i801_hw *)card->hw;
	cancel_timer(&it->midi_timer);
	remove_io_interrupt_handler(it->midi_interrupt, i801_mpu_isr, card);
	dprintf("disabled i801 MPU int (IRQ %d)\n", it->midi_interrupt);
}


static void i801_enable_gameport(sound_card_t * card)
{
}

static void i801_disable_gameport(sound_card_t * card)
{
}

static status_t i801_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static void
unpack_eisa_id(EISA_PRODUCT_ID pid, unsigned char* str)
{
        str[0] = ((pid.b[0] >> 2) & 0x1F) + 'A' - 1;
        str[1] = ((pid.b[1] & 0xE0)>>5) | ((pid.b[0] & 0x3) << 3) + 'A' - 1;
        str[2] = (pid.b[1] & 0x1F) + 'A' - 1;
        str[3] = '\0';
}

static void
dump_eisa_id(char* mes, uint32 id)
{
        unsigned char vendor[4];
		EISA_PRODUCT_ID pid;

		pid.id = id;
        unpack_eisa_id(pid, vendor);
        dprintf("%s vendor %s, product# %x%x, revision %x\n",
				mes, vendor, pid.b[2], pid.b[3]>>4, pid.b[3] & 0xf);
}

static int
log_2(
	uint32 m)
{
	int r = 0;
	if (!m) return 0;
	while (!(m & 1)) {
		m >>= 1;
		r++;
	}
	return r;
}

static int
get_i801_midi_irq(
	struct isa_device_info * info,
	uint64 cookie,
	config_manager_for_driver_module_info * config,
	int * mpu_base)
{
	int irq = 0;
	int ix = 0;
	resource_descriptor desc;
	struct device_configuration * setup;
	int size = (*config->get_size_of_current_configuration_for)(cookie);
	EISA_PRODUCT_ID that;
	MAKE_EISA_PRODUCT_ID(&that, 'P', 'N', 'P', 0xb00, 6);
	if (!EQUAL_EISA_PRODUCT_ID(that, (*(EISA_PRODUCT_ID *)&info->vendor_id))) {
		return 0;
	}
	if (size <= 0) return 0;
	*mpu_base = 0;
	setup = (struct device_configuration *)malloc(size);
	if (!setup) return 0;
	if ((*config->get_current_configuration_for)(cookie, setup, size) != B_OK) {
		free(setup);
		return 0;
	}
	if ((*config->get_nth_resource_descriptor_of_type)(setup, 0, B_IRQ_RESOURCE,
			&desc, sizeof(desc)) < 0) {
		free(setup);
		return 0;
	}
	irq = log_2(desc.d.m.mask);
	while ((*config->get_nth_resource_descriptor_of_type)(setup, ix++, B_IO_PORT_RESOURCE,
			&desc, sizeof(desc)) >= B_OK) {
		if (desc.d.r.minbase >= 0x300 && desc.d.r.minbase <= 0x330) {
			*mpu_base = desc.d.r.minbase;
			break;
		}
	}
	free(setup);
	if (*mpu_base == 0) irq = 0;
	return irq;
}

static status_t i801_InitMPU401(sound_card_t * card)
{
	pci_info bridge;
	int32 offset = 0;
	int cfg = 0;
	bool found = false;
	config_manager_for_driver_module_info *config;
	uint64 cookie = 0;
	struct isa_device_info dev;
	int irq;

	if (!g_enableMPU) {
		dprintf("i801: MPU disabled (enable with enable_mpu in settings)\n");
		return ENODEV;
	}
	while (pci->get_nth_pci_info(offset++, &bridge) == B_OK) {
		if ((bridge.vendor_id == 0x8086) && ((bridge.device_id == 0x2410) || (bridge.device_id == 0x2420)) &&
				(bridge.device == 31) && (bridge.function == 0)) {
			found = true;
			break;
		}
	}
	if (!found) {
		dprintf("i801: no MIDI device found\n");
		return ENODEV;
	}
	if (get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&config) < 0) {
		dprintf("i801: get_module(config) failed.\n");
		card->mpu401_base = 0;
		return ENOSYS;
	}
	irq = 0;
	while ((*config->get_next_device_info)(B_ISA_BUS, &cookie, &dev.info, sizeof(dev)) >= B_OK) {
		if ((irq = get_i801_midi_irq(&dev, cookie, config, &card->mpu401_base)) > 0)
			break;
	}
	if (irq <= 0) {
		dprintf("i801: could not find MIDI device\n");
		card->mpu401_base = 0;
		return ENODEV;
	}
	((i801_hw *)card->hw)->midi_interrupt = irq;
	dprintf("i801: MPU IRQ = %d\n", irq);
	cfg = pci->read_pci_config(bridge.bus, bridge.device, bridge.function, 0xe2, 1);
	if (card->mpu401_base == 0x330) {
		cfg &= ~8;
	}
	else {
		cfg |= 8;
	}
	pci->write_pci_config(bridge.bus, bridge.device, bridge.function, 0xe2, 1, cfg);
	card->func->mpu401_io_hooks = NULL;
	dprintf("i801: MIDI base 0x%x (cfg 0x%x)\n", card->mpu401_base, cfg);
	cfg = pci->read_pci_config(bridge.bus, bridge.device, bridge.function, 0xe6, 2);
	if (!(cfg & (1<<5))) {
		cfg |= (1<<5);
		dprintf("i801: enable MIDI decode\n");
		pci->write_pci_config(bridge.bus, bridge.device, bridge.function, 0xe6, 2, cfg);
	}
	else {
		dprintf("i801: MIDI decode already enabled\n");
	}
	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);

	return B_OK;
}


static void lock_ac97(sound_card_t * card)
{
	if (atomic_add(&((i801_hw*)card->hw)->ac97ben, 1) > 0)
		acquire_sem(((i801_hw*)card->hw)->ac97sem);
}
static void unlock_ac97(sound_card_t * card)
{
	if (atomic_add(&((i801_hw*)card->hw)->ac97ben, -1) > 1)
		release_sem_etc(((i801_hw*)card->hw)->ac97sem, 1, B_DO_NOT_RESCHEDULE);
}

static void duh(short *buf, int frames)
{
	while (frames-- > 0) {
		*buf++ = ((frames & 0x3f)-0x20)*1000;
		*buf++ = ((frames & 0x3f)-0x20)*1000;
	}
}

static status_t i801_setup_sg_tables(i801_hw * it)
{	//	set up SG tables
	physical_entry ent_p;
	physical_entry ent_c;
	int ix;
	status_t err;

	if ((err = get_memory_map(it->PcmPlaybackBuf, it->PcmPlaybackBufSize, &ent_p, 1)) < 0) {
		derror("get_memory_map(%p) failed (PCMP): 0x%x\n", it->PcmPlaybackBuf, err);
		return err;
	}
	if ((err = get_memory_map(it->PcmCaptureBuf, it->PcmCaptureBufSize, &ent_c, 1)) < 0) {
		derror("get_memory_map(%p) failed (PCMC): 0x%x\n", it->PcmCaptureBuf, err);
		return err;
	}
	for (ix=0; ix<I801_SG_TABLE_ENTS; ix++) {

		it->sgPlaybackTable[ix].base = ((char *)ent_p.address)+it->PcmPlaybackBufSize/2*(ix&1);
		it->sgPlaybackTable[ix].cmd = I801_CMD_INT | I801_CMD_ZEROP;
		it->sgPlaybackTable[ix].count = SIZE_TO_LENGTH(it->PcmPlaybackBufSize/2);

		it->sgCaptureTable[ix].base = ((char *)ent_c.address)+it->PcmCaptureBufSize/2*(ix&1);
		it->sgCaptureTable[ix].cmd = I801_CMD_INT | I801_CMD_ZEROP;
		it->sgCaptureTable[ix].count = SIZE_TO_LENGTH(it->PcmCaptureBufSize/2);
	}
	if ((err = get_memory_map(it->sgPlaybackTable,
			sizeof(SGTableEnt)*I801_SG_TABLE_ENTS, &ent_p, 1)) < 0) {
		derror("get_memory_map(%p) failed (SGP): 0x%x\n", it->sgPlaybackTable, err);
		return err;
	}
	if ((err = get_memory_map(it->sgCaptureTable,
			sizeof(SGTableEnt)*I801_SG_TABLE_ENTS, &ent_c, 1)) < 0) {
		derror("get_memory_map(%p) failed (SGC): 0x%x\n", it->sgCaptureTable, err);
		return err;
	}
	/* set up DMA base registers */
	(*pci->write_io_32)(it->NABMBAR+I801_PLAYBACK_BASE, (uint32)ent_p.address);
	(*pci->write_io_32)(it->NABMBAR+I801_CAPTURE_BASE, (uint32)ent_c.address);

	DB_PRINTF(("i801: phys play sg = %p, capt sg = %p\n", ent_p.address, ent_c.address));
#if 0
	DB_PRINTF(("playback:\n"));
	for (ix=0; ix<I801_SG_TABLE_ENTS; ix++) {
		DB_PRINTF(("%p %04x %04x\n", it->sgPlaybackTable[ix].base, 
			it->sgPlaybackTable[ix].cmd, it->sgPlaybackTable[ix].count));
	}
	DB_PRINTF(("capture:\n"));
	for (ix=0; ix<I801_SG_TABLE_ENTS; ix++) {
		DB_PRINTF(("%p %04x %04x\n", it->sgCaptureTable[ix].base, 
			it->sgCaptureTable[ix].cmd, it->sgCaptureTable[ix].count));
	}
#endif
	return B_OK;
}

static status_t i801_Init(sound_card_t * card)
{
	i801_hw * it;
	uint32 value;
	status_t err = B_OK;
	bigtime_t timeout;
	bool ready;

	DB_PRINTF(("i801_Init(): here! \n"));

dprintf("i801: subvendor id 0x%x, subsystem id 0x%x\n",
		card->bus.pci.info.u.h0.subsystem_vendor_id, 
		card->bus.pci.info.u.h0.subsystem_id);

	/*	Enable PCI config space	*/
	value = (*pci->read_pci_config)(
			card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function,
			0x04, 0x02);
	(*pci->write_pci_config)(
			card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function,
			0x04, 0x02, value | 0x0005);

	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(i801_hw));
	if(!card->hw) {
		derror("i801_Init(): malloc error!\n");
        i801_Uninit(card);
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(i801_hw));	// zero it out

	it = (i801_hw*)card->hw;
	/* Some initialization */
	it->PcmCaptureAreaId = -1; 
	it->PcmPlaybackAreaId = -1; 
	it->sgTablesAreaID = -1;
	it->NAMBAR = card->bus.pci.info.u.h0.base_registers[0];		//	0x100 size
	it->NABMBAR = card->bus.pci.info.u.h0.base_registers[1];	//	0x40 size

	it->ac97sem = create_sem(1, "i801 ac97sem");
	if (it->ac97sem < 0) {
		derror("cannot create ac97sem\n");
		return it->ac97sem;
	}
	dprintf("i801: NAMBAR=0x%x, NABMBAR=0x%x\n", it->NAMBAR, it->NABMBAR);

	it->PcmPlaybackAreaId = create_area("i801_PlaybackDmaBuf",
												&it->PcmPlaybackBuf, 
												B_ANY_KERNEL_ADDRESS, 
												DEFAULT_BUF_SIZE,
												B_CONTIGUOUS,
												B_READ_AREA | B_WRITE_AREA);
	if (it->PcmPlaybackAreaId < 0) {
		derror("i801_Init: Memory allocation for i801__PlaybackDmaBuf failed!\n");
		err = it->PcmPlaybackAreaId;
        i801_Uninit(card);
		return err;
	}
	memset(it->PcmPlaybackBuf, 0x00, DEFAULT_BUF_SIZE);
//	duh((short *)it->PcmPlaybackBuf, 1024);
	it->PcmPlaybackBufSize = DEFAULT_BUF_SIZE;

	it->PcmCaptureAreaId = create_area("i801_CaptureDmaBuf",
												&it->PcmCaptureBuf, 
												B_ANY_KERNEL_ADDRESS, 
												DEFAULT_BUF_SIZE,
												B_CONTIGUOUS,
												B_READ_AREA | B_WRITE_AREA);
	if (it->PcmCaptureAreaId < 0) {
		derror("i801_Init: Memory allocation for i801__CaptureDmaBuf failed!\n");
		err = it->PcmCaptureAreaId;
        i801_Uninit(card);
		return err;
	}
	memset(it->PcmCaptureBuf, 0x00, DEFAULT_BUF_SIZE);
	it->PcmCaptureBufSize = DEFAULT_BUF_SIZE;

	it->sgTablesAreaID = create_area("i801_sgTablesArea",
												(void **)&it->sgPlaybackTable, 
												B_ANY_KERNEL_ADDRESS, 
												B_PAGE_SIZE,
												B_CONTIGUOUS,
												B_READ_AREA | B_WRITE_AREA);
	if (it->sgTablesAreaID < 0) {
		derror("i801_Init: Memory allocation for i801__sgTablesAreaID failed!\n");
		err = it->sgTablesAreaID;
        i801_Uninit(card);
		return err;
	}
	memset(it->sgPlaybackTable, 0x00, B_PAGE_SIZE);
	it->sgCaptureTable = it->sgPlaybackTable+I801_SG_TABLE_ENTS;

	/* take AC97 out of reset condition */
	(*pci->write_io_32)(it->NABMBAR+I801_GLOBAL_CONTROL, I801_GC_ACCR);

	timeout = system_time()+600000LL;
	do {
		snooze(400);
		ready = (0 != ((*pci->read_io_32)(it->NABMBAR+I801_GLOBAL_STATUS) & I801_GS_PCR));
	} while (!ready && (system_time() < timeout));
	if (!ready) {
		derror("primary AC97 codec did not become ready!\n");
		return EIO;
	}
	ddprintf("i801: AC97 primary codec became ready in %Ld us\n",
			system_time()-(timeout-600000LL));
	ddprintf("i801: GS is 0x%x\n", (*pci->read_io_32)(it->NABMBAR+I801_GLOBAL_STATUS));

	err = i801_setup_sg_tables(it);
	if (err < 0) {
		i801_Uninit(card);
		return err;
	}

	/* Install i801 isr */
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, i801_isr, card, 0);

	/* Init codec */
	i801_InitCodec(card);

	DB_PRINTF(("i801_Init(): B_OK!\n"));
	return B_OK;
}


static status_t i801_Uninit(sound_card_t * card)
{
	i801_hw * it = (i801_hw *)card->hw;

	DB_PRINTF(("i801_Uninit(): here! \n"));

	/* we should make sure we're not running here ... */
	(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_CR, 0);
	(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_CR, 0);

	/* Remove i801 isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, i801_isr, card);
	remove_io_interrupt_handler(((i801_hw *)card->hw)->midi_interrupt, i801_mpu_isr, card);

	/* Uninit codec */
	i801_UninitCodec(card);

	/* turn off the AClink (hold the codec in cold reset) */
	(*pci->write_io_32)(it->NABMBAR+I801_GLOBAL_CONTROL, I801_GC_ACOFF);

	if (it->ac97sem > 0) delete_sem(it->ac97sem);

	if (it->PcmCaptureAreaId > 0) delete_area(it->PcmCaptureAreaId);
	if (it->PcmPlaybackAreaId > 0) delete_area(it->PcmPlaybackAreaId);
	if (it->sgTablesAreaID > 0) delete_area(it->sgTablesAreaID);

	/* Clean up */
	free(card->hw);
	card->hw = NULL;

	return B_OK;
}


static void i801_StartPcm(sound_card_t *card)
{
	cpu_status cp;
	i801_hw * it = (i801_hw *)card->hw;
	DB_PRINTF(("i801_StartPcm\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

#if ENABLE_PLAYBACK
	/*start playback*/
	(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_CIX, 0);
	(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_LVIX, -1);
	(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_CR, I801_CR_IOCE|I801_CR_RPBM);
#endif
#if ENABLE_CAPTURE
	/*start capture*/
	(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_CIX, 0);
	(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_LVIX, -1);
	(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_CR, I801_CR_IOCE|I801_CR_RPBM);
#endif

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void i801_StopPcm(sound_card_t *card)
{
	cpu_status cp;
	i801_hw * it = (i801_hw *)card->hw;
	DB_PRINTF(("i801_PcmStopPcm\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	/*terminate playback*/
	(*pci->write_io_8)(it->NABMBAR+I801_PLAYBACK_CR, I801_CR_IOCE);
	/*terminate capture*/
	(*pci->write_io_8)(it->NABMBAR+I801_CAPTURE_CR, I801_CR_IOCE);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void i801_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	if (((i801_hw *)card->hw)->SRHz != sample_rate) {
		if (sample_rate == 48000) {	//	hard-code
			//	turn off VRA
			i801_CodecWrite(card, 0x2a, 0x0, 0x1);
		}
		else {		//	try for variable rate
			//	turn on VRA
			i801_CodecWrite(card, 0x2a, 0x1, 0x1);
		}
		if (i801_CodecRead(card, 0x2a) & 1) {
			//	use same sample rate for playback/capture
			i801_CodecWrite(card, 0x2c, sample_rate, -1);
			i801_CodecWrite(card, 0x32, sample_rate, -1);
			((i801_hw*)card->hw)->SRHz = i801_CodecRead(card, 0x2c);
		}
		else
		  ((i801_hw*)card->hw)->SRHz = 48000;
	}
	DB_PRINTF(("i801 SR: %d (requested %d)\n", ((i801_hw*)card->hw)->SRHz, sample_rate));
}

static status_t i801_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** log_addr)
{
	i801_hw * it = (i801_hw*)card->hw;
	status_t err;
	DB_PRINTF(("In i801_SetPlaybackDmaBuf\n"));

	if (size > DEFAULT_BUF_SIZE) {
		dprintf("i801: Playback buffer cannot be bigger than %ld! request was %ld\n",DEFAULT_BUF_SIZE,size);
		return B_ERROR;
	}
	it->PcmPlaybackBufSize = size;
	err = i801_setup_sg_tables(it);
	*log_addr = it->PcmPlaybackBuf;
	
	dprintf("i801: DMA half-page size %d frames\n", size/8);
	return err;
}

static void i801_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	/* this is a no-op because we only do stereo 16 bit */
}

static void i801_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	if (sample_rate != ((i801_hw *)card->hw)->SRHz) {
		i801_SetPlaybackSR(card, sample_rate);
	}
}

static status_t i801_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** log_addr)
{
	i801_hw * it = (i801_hw*)card->hw;
	status_t err;
	DB_PRINTF(("In i801_SetCaptureDmaBuf\n"));

	if (size > DEFAULT_BUF_SIZE) {
		dprintf("i801: Capture buffer cannot be bigger than %ld! request was %ld\n",DEFAULT_BUF_SIZE,size);
		return B_ERROR;
	}
	it->PcmCaptureBufSize = size;
	err = i801_setup_sg_tables(it);
	*log_addr = it->PcmCaptureBuf;
	
	DB_PRINTF(("i801_SetCaptureDmaBuf OK\n"));
	return err;
}

static void i801_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	/* this is a no-op because we only do stereo 16 bits */
}


static uint16 i801_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	uint16 ret = AC97_BAD_VALUE;
	i801_hw * it = (i801_hw *)card->hw;
	bigtime_t timeout = system_time()+500000LL;

	while ((*pci->read_io_8)(it->NABMBAR+I801_CODEC_ACCESS) & I801_CA_IN_PROGRESS) {
		if (system_time() > timeout) {
			dprintf("i801: CodecRead times out waiting for hw semaphore\n");
			return AC97_BAD_VALUE;
		}
		spin(100);
	}
	ret = (*pci->read_io_16)(it->NAMBAR+offset);
//	(*pci->write_io_32)(it->NABMBAR+I801_CODEC_ACCESS, 0);
	return ret;
}


static uint16 i801_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret;
	
	lock_ac97(card);
	ret = i801_CodecReadUnsecure(card, offset);
	unlock_ac97(card);	
	return ret;
}

static status_t i801_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	sound_card_t* card = (sound_card_t*) host;
	i801_hw * it = (i801_hw *)card->hw;
	status_t ret = B_OK;
	bigtime_t timeout;

	if (mask == 0)
		return B_OK;
	if (mask != 0xffff) {
		uint16 old  = i801_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}

	lock_ac97(card);
	timeout = system_time()+500000LL;
	while ((*pci->read_io_8)(it->NABMBAR+I801_CODEC_ACCESS) & I801_CA_IN_PROGRESS) {
		if (system_time() > timeout) {
			dprintf("i801: CodecWrite times out waiting for hw semaphore\n");
			ret = B_TIMED_OUT;
			break;
		}
		spin(100);
	}
	if (ret == B_OK) {
		(*pci->write_io_16)(it->NAMBAR+offset, value);
	}
//	(*pci->write_io_32)(it->NABMBAR+I801_CODEC_ACCESS, 0);
	unlock_ac97(card);

	return ret;
}


static status_t i801_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;
	i801_hw * it = (i801_hw*)card->hw;
	bigtime_t timeout;
	bool ready = false;

	DB_PRINTF(("In i801_InitCodec\n"));

	/* reset codec access semaphore */
	(*pci->write_io_32)(it->NABMBAR+I801_CODEC_ACCESS, 0);
#if 0
	/* The databook talks about this being ignored if the bit clock */
	/* is running, and that's what seems to be happening. */

	(*pci->write_io_32)(it->NABMBAR+I801_GLOBAL_CONTROL, I801_GC_ACCR |
		I801_GC_ACWR);
	timeout = system_time()+2000000L;
	do {
		snooze(400);
		ready = (0 != ((*pci->read_io_32)(it->NABMBAR+I801_GLOBAL_CONTROL) & I801_GC_ACWR));
	} while (!ready && (system_time() < timeout));
	if (!ready) {
		derror("warm AC97 reset did not end!\n");
		return EIO;
	}
	DB_PRINTF(("i801: AC97 primary codec warm reset took %Ld us\n",
			system_time()-(timeout-600000LL)));
#endif

	err = ac97init(&it->codec, 
					(void*)card,
					i801_CodecWrite,
					i801_CodecRead);

	if (err!=B_OK) {
		DB_PRINTF(("i801_InitCodec(): ac97init err = %ld!\n",err));
	}
	return err;				
}

static void i801_UninitCodec(sound_card_t *card)
{
	DB_PRINTF(("i801_UninitCodec()\n"));
	ac97uninit(&((i801_hw*)card->hw)->codec);
}

static status_t i801_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t err;
	DB_PRINTF(("SoundSetup A: ss->sample_rate = %d\n",ss->sample_rate));
	err = AC97_SoundSetup(&((i801_hw*)card->hw)->codec, ss);
	DB_PRINTF(("SoundSetup B: ss->sample_rate = %d\n",ss->sample_rate));
	if (err == B_OK) {
		i801_SetPlaybackSR(card, sr_from_constant(ss->sample_rate));
		switch (ss->playback_format) {
#if 0
		case linear_8bit_unsigned_mono:
			i801_SetPlaybackFormat(card, 8, 1);
			break;
		case linear_8bit_unsigned_stereo:
			i801_SetPlaybackFormat(card, 8, 2);
			break;
		case linear_16bit_big_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
			i801_SetPlaybackFormat(card, 16, 1);
			break;
#endif
		case linear_16bit_big_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
			i801_SetPlaybackFormat(card, 16, 2);
			break;
		default:
			derror("unknown sample format %d\n", ss->playback_format);
			i801_SetPlaybackFormat(card, 16, 2);
			err = B_BAD_VALUE;
		}
	}
	return err;
}

static status_t  i801_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ret = AC97_GetSoundSetup(&((i801_hw*)card->hw)->codec,ss);
	ss->sample_rate = constant_from_sr(((i801_hw*)card->hw)->SRHz);;
	ss->playback_format = linear_16bit_big_endian_stereo;
	ss->capture_format = ss->playback_format;
	DB_PRINTF(("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate));
	return ret;
}









