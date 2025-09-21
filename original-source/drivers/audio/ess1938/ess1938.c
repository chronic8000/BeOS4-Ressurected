
#include "ess1938.h"
/* has PCI.h */

#include <string.h>
#include <stdio.h>
#include <Drivers.h> 
#include <byteorder.h>

#ifdef __POWERPC__
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);

int32	api_version = B_CUR_DRIVER_API_VERSION;

extern device_hooks joy_hooks;
extern device_hooks midi_hooks;
extern device_hooks pcm_hooks;

/* Globals */

int32 num_cards;	
int num_names;		
ess1938_dev cards[NUM_CARDS];
char * names[NUM_CARDS*7+1];

static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;

static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module * mpu401;

static char gameport_name[] = "generic/gameport/v1";
generic_gameport_module * gameport;


/*
 *  PCI_CFG_RD/WR functions
 */

uint32
PCI_CFG_RD ( 	uchar	bus,
				uchar	device,	
				uchar	function,
				uchar	offset,
				uchar	size )
{									
	return (*pci->read_pci_config) (bus, device, function, offset, size);
}

void
PCI_CFG_WR ( 	uchar	bus,
				uchar	device,	
				uchar	function,
				uchar	offset,
				uchar	size,
				uint32	value )
{					
	(*pci->write_pci_config) (bus, device, function, offset, size, value);
}

/* 
 *	PCI_IO_RD/WR functions
 */

uint8
PCI_IO_RD (int offset)
{
	return (*pci->read_io_8) (offset);
}

uint16
PCI_IO_RD_16 (int offset)
{
	return (*pci->read_io_16) (offset);
}

void
PCI_IO_WR (int offset, uint8 val)
{
	(*pci->write_io_8) (offset, val);
}

/* detect presence of our hardware		*/
/* modify config space for native mode	*/
/* (as opposed to compatibility mode)	*/
/* because there aren't many dos apps	*/
/* that	will run on BeOS....			*/
status_t 
init_hardware(void)
{
	int ix=0;
	uint32 ui32;
	pci_info info;
	status_t err = ENODEV;

	ddprintf("ess1938: init_hardware()\n");
	ddprintf("/*************************************************************/\n");
	ddprintf("/*                                                           */\n");
	ddprintf("/*                                                           */\n");
	ddprintf("/*               ESS1938 NEW HARDWARE LOAD                   */\n");
	ddprintf("/*                                                           */\n");
	ddprintf("/*                                                           */\n");
	ddprintf("/*************************************************************/\n");
	ddprintf("ess1938: %s %s\n",__TIME__,__DATE__);

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == ESS1938_VENDOR_ID &&
			info.device_id == ESS1938_DEVICE_ID) {
			err = B_OK;

			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x60,						/* DDMA Control Register */
						2,							
						(info.u.h0.base_registers[2] & 0xfff0) | 0x0001 );	/* vcbase */

			ui32 =  PCI_CFG_RD(	info.bus,
								info.device,
								info.function,
								0x50,
								2 );

			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x50,						
						2,							
						ui32 & 0x98FF );

			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x04,						
						2,							
						5 );

			/* The undocumented sequence */
			/* set, clear, set.......... */ 
			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x40,						
						2,							
						0x801f );

			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x40,						
						2,							
						0x001f );

			PCI_CFG_WR(	info.bus,
						info.device,
						info.function,
						0x40,						
						2,							
						0x801f );

			/* quick sanity check */
			if 	( info.u.h0.interrupt_line == 0xff ) {
				dprintf("ess1938: Interrupt line not initialized, PCI config error (fatal)\n");
				err = B_ERROR;
			}
		}
		ix++;
	}
	put_module(pci_name);
	return err;
}

void
set_direct(
	int regno,
	uchar value,
	uchar mask)
{
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD( regno );
		value = (value & mask)|(old & ~mask);
	}
	PCI_IO_WR( regno, value );
}

uchar
get_direct(
	int regno)
{
	return (uchar) PCI_IO_RD( regno );
}

void
set_controller(
	ess1938_dev * card,
	uchar value)
{
	/* NO MASK! */

	/* turn ints off */
	cpu_status cp;
	bigtime_t start_time = system_time();
	cp = disable_interrupts();
	
	/* a spinlock is not needed as we must*/
	/* hold the control semaphore before  */
	/* entering this routine.  Ints must be disabled */
	/* so that we are not preempted */

	/* poll for ready */
	
	while ( PCI_IO_RD(card->sbbase + CONTROL_STATUS) & 0x80) {
		if( system_time() - start_time > 50 ) {
			/* it's bad to disable int's for more than 50 uSec */
			KPRINTF("ess1938: set controller timeout !\n");
			goto bail;
		}
	}
	/* we now have 13 uS to send the data  */
	PCI_IO_WR(card->sbbase + CONTROL_WRITE, value);
		
	/* turn ints on */
bail:
	restore_interrupts(cp);
}

void
set_controller_fast(
	ess1938_dev * card,
	uchar reg,
	uchar value)
{
	/* NO MASK! */

	/* turn ints off */
	cpu_status cp;
	bigtime_t start_time = system_time();
	cp = disable_interrupts();
	
	/* a spinlock is not needed as we must*/
	/* hold the control semaphore before  */
	/* entering this routine.  Ints must be disabled */
	/* so that we are not preempted */

	/* poll for ready */
	
	while ( PCI_IO_RD(card->sbbase + CONTROL_STATUS) & 0x80) {
		if( system_time() - start_time > 50 ) {
			/* it's bad to disable int's for more than 50 uSec */
			KPRINTF("ess1938: set controller fast timeout !\n");
			goto bail;
		}
	}
	/* we now have 13 uS to send the data  */
	PCI_IO_WR(card->sbbase + CONTROL_WRITE, reg);
	/* no need for 	EIEIO() -- it's part of pci->read_* */
	PCI_IO_WR(card->sbbase + CONTROL_WRITE, value);
	
	/* turn ints on */
bail:
	restore_interrupts(cp);
}



uchar
get_controller(
	ess1938_dev * card )
{
	int cnt = 0;
	uchar ret = 0;

	while ( !(PCI_IO_RD(card->sbbase + CONTROL_STATUS) & 0x40)) {
		spin(10);
		/* 500 times should be plenty */
		if( cnt++ > 500) {
			ddprintf("ess1938: get controller timeout !\n");
			goto bail;
		}
	}
	ret = PCI_IO_RD(card->sbbase + CONTROL_READ);
bail:
	return ret;
}

void
set_indirect(
	int addr_command,
	int addr_data,
	int regno,
	uchar value,
	uchar mask)
{

	PCI_IO_WR(addr_command, regno);
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD(addr_data);
		value = (value&mask)|(old&~mask);
	}
	PCI_IO_WR(addr_data, value);
}

uchar
get_indirect(
	int addr_command,
	int addr_data,
	int regno)
{

	PCI_IO_WR(addr_command, regno);
	EIEIO();
	return (uchar) PCI_IO_RD(addr_data);
}

void
disable_card_interrupts(
	ess1938_dev * card)
{
	/* AUDIO_1 | AUDIO_2 | MPU 401 Master Interrupt Mask */
	set_direct(	card->iobase + IMR_ISR,
				0x00,
				0xff);
	/* clear AUDIO_1 */
	get_direct(card->sbbase + RD_BUF_STATUS);
	/* AUDIO_2 interrupt */
	/* this will also clear the interrupt */
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_2,
					0x00,
					0xc0);
	/* MPU 401 */				
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					MASTER_VOL_CTRL,
					0x00,
					0x40);
}

void
enable_card_interrupts(
	ess1938_dev * card)
{

	/* AUDIO_1 | AUDIO_2 interrupt | MPU 401 Master Mask*/
	set_direct(	card->iobase + IMR_ISR,
				0x10 |  0x20  | 0x80,
				0xff);

	/* AUDIO_2 */
	set_indirect(	card->sbbase + MIXER_COMMAND,
					card->sbbase + MIXER_DATA,
					AUDIO_2_CTRL_2,
					0x40,
					0x40);
					
	/* MPU 401 */
	// set by enable_int op			
}

static void
set_default_registers(
	ess1938_dev * card)
{
	/* called with spinlock held */
	/* should init regs here     */
}


static void
make_device_names(
	ess1938_dev * card)
{
	char * name = card->name;
	/* used to be zero based, now it's one */
	sprintf(name, "ess1938/%d", card-cards+1);
#if __INTEL__
	sprintf(card->midi.name, "midi/%s", name);
	names[num_names++] = card->midi.name;
	sprintf(card->joy.name, "joystick/%s", name);
	names[num_names++] = card->joy.name;
#endif	
	sprintf(card->pcm.oldname, "audio/old/%s", name);
	names[num_names++] = card->pcm.oldname;
	names[num_names] = NULL;
}

static status_t
setup_ess1938(
	ess1938_dev * card)
{
	status_t err = B_OK;
	cpu_status cp;

	ddprintf("ess1938: setup_ess1938(%x)\n", card);
	/* init */
	card->open_close_sem	= B_ERROR;
	card->mixer_sem			= B_ERROR;
	card->pcmwrite_sem		= B_ERROR;
	card->pcmread_sem		= B_ERROR;
	card->controller_sem	= B_ERROR;
	card->write_sem			= B_ERROR;
	card->read_sem			= B_ERROR;
	card->old_play_sem		= B_ERROR;
	card->old_cap_sem		= B_ERROR;
	card->lastint_sem		= B_ERROR;
	card->dma_area 			= B_ERROR;
	card->init_cnt			= 0;
	card->write_waiting		= 0;
	card->read_waiting		= 0;
	card->closing			= 0;
	card->low_byte_l		= 0;
	card->high_byte_l		= 0;
	card->low_byte_r		= 0;
	/* These if/else are backwards (Intel optimization says most likely follows if) */	
	if ((card->open_close_sem = create_sem(1, "ess1938 open_close sem")) < B_OK) {
		err = card->open_close_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->open_close_sem, B_SYSTEM_TEAM);
	}
	if ((card->mixer_sem = create_sem(1, "ess1938 mixer sem")) < B_OK) {
		err = card->mixer_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->mixer_sem, B_SYSTEM_TEAM);
	}
	if ((card->pcmwrite_sem = create_sem(1, "ess1938 pcmwrite sem")) < B_OK) {
		err = card->pcmwrite_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->pcmwrite_sem, B_SYSTEM_TEAM);
	}
	if ((card->pcmread_sem = create_sem(1, "ess1938 pcmread sem")) < B_OK) {
		err = card->pcmread_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->pcmread_sem, B_SYSTEM_TEAM);
	}
	if ((card->controller_sem = create_sem(1, "ess1938 controller sem")) < B_OK) {
		err = card->controller_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->controller_sem, B_SYSTEM_TEAM);
	}
	if ((card->write_sem = create_sem(0, "ess1938 write sem")) < B_OK) {
		err = card->write_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->write_sem, B_SYSTEM_TEAM);
	}
	if ((card->read_sem = create_sem(0, "ess1938 read sem")) < B_OK) {
		err = card->read_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->read_sem, B_SYSTEM_TEAM);
	}
	if ((card->lastint_sem = create_sem(0, "ess1938 lastint sem")) < B_OK) {
		err = card->lastint_sem;
		ddprintf("ess1938: create_sem failed\n");
		goto bail;
	}
	else {
		set_sem_owner(card->lastint_sem, B_SYSTEM_TEAM);
	}
#if __INTEL__
	if ((*mpu401->create_device)(card->info.u.h0.base_registers[3], &card->midi.driver,
		0, midi_interrupt_op, &card->midi) < B_OK) {
		ddprintf("ess1938: create midi device failed\n");
		goto bail;
	}
	if ((*gameport->create_device)(card->info.u.h0.base_registers[4],
		&card->joy.driver) < B_OK) {
		ddprintf("ess1938: create joystick device failed\n");
		goto bail;
	}
#endif
	card->midi.card = card;
	card->pcm.card = card;
	make_device_names(card);

	card->iobase = card->info.u.h0.base_registers[0];
	card->sbbase = card->info.u.h0.base_registers[1];
	card->vcbase = card->info.u.h0.base_registers[2];	
	ddprintf("ess1938: %s iobase at %x\n", card->name, card->iobase);
	ddprintf("ess1938: %s sbbase at %x\n", card->name, card->sbbase);
	ddprintf("ess1938: %s vcbase at %x\n", card->name, card->vcbase);

	card->dma_area = create_area("ess1938", (void **)&card->dma_buf,
									B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, 
									B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if(card->dma_area < B_OK){
		err = card->dma_area;
		ddprintf("ess1938: error creating area\n");
		goto bail;
	}
	else {
		/* Get a physical address from a virtual address */
		if (get_memory_map(card->dma_buf, B_PAGE_SIZE, &card->dma_phys, 1) < 0) {
			delete_area(card->dma_area);
			err = card->dma_area = B_ERROR; /* don't delete it twice */
			ddprintf("ess1938: error mapping memory\n");
			goto bail;
		}
		else {
		/* for BeBoxen */
		/* convert physical address (as seen by CPU)   */
		/* to physical address (as seen by PCI bus)    */
		/* however, we don't support BeBoxen with this */
		/* card, so we fail it below...                */
		card->dma_phys.address = (*pci->ram_address)(card->dma_phys.address);
		}
	}
	card->dma_buf_ptr = card->dma_buf;

	card->dma_cap_area = create_area(	"ess1938", (void **)&card->dma_cap_buf,
										B_ANY_KERNEL_ADDRESS,
									 	B_PAGE_SIZE , 
										B_LOMEM,
										B_READ_AREA | B_WRITE_AREA);
	if(card->dma_cap_area < B_OK){
		err = card->dma_cap_area;
		ddprintf("ess1938: error creating area\n");
		goto bail;
	}
	else {
		/* Get a physical address from a virtual address */
		if (get_memory_map(card->dma_cap_buf, B_PAGE_SIZE , &card->dma_cap_phys, 1) < 0) {
			delete_area(card->dma_cap_area);
			err = card->dma_cap_area = B_ERROR; /* don't delete it twice */
			ddprintf("ess1938: error mapping memory\n");
			goto bail;
		}
		else {
			/* for BeBoxen */
			/* convert physical address (as seen by CPU)    */
			/* to physical address (as seen by PCI bus)     */
			/* however, dma capture address is only 24 bits */
			/* so fail everything if we are on a BeBox      */
			if ((*pci->ram_address)(card->dma_cap_phys.address) != card->dma_cap_phys.address) {
				err = B_ERROR;
				ddprintf("ess1938: BeBox (or bus equivalent) not supported\n");
				goto bail;
			}
		}
	}
	
	card->dma_cap_buf_ptr = card->dma_cap_buf;

	/* reset will be done in pcm_open.... */
	
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);
	
	disable_card_interrupts(card);
	set_default_registers(card);

	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

bail:
	return err; 
}

static int
debug_ess1938(
	int argc,
	char * argv[])
{
	int ix = 1;
	if (argc == 2) {
		ix = parse_expression(argv[1]);
	}
	if (argc > 2 || ix < 1 || ix > num_cards) {
		kprintf("ess1938: dude, you gotta watch your syntax!\n");
		return B_ERROR;
	}
	
	ix -= 1; /* convert from 1's based to 0's */
	kprintf("%s: io base register at 0x%x\n", cards[ix].name, cards[ix].iobase);
	kprintf("%s: sb base register at 0x%x\n", cards[ix].name, cards[ix].sbbase);
	kprintf("%s: vc base register at 0x%x\n", cards[ix].name, cards[ix].vcbase);
	kprintf("%s: pb phys address at 0x%x\n", cards[ix].name, cards[ix].dma_phys.address);
	kprintf("%s: pb virt address at 0x%x\n", cards[ix].name, cards[ix].dma_buf);
	kprintf("%s: cap phys address at 0x%x\n", cards[ix].name, cards[ix].dma_cap_phys.address);
	kprintf("%s: cap virt address at 0x%x\n", cards[ix].name, cards[ix].dma_cap_buf);
	kprintf("%s: card structure is at 0x%x\n",cards[ix].name, cards[ix]);
	kprintf("%s: Channel 1 DMA Counter is 0x%4x\n",cards[ix].name, B_LENDIAN_TO_HOST_INT16(PCI_IO_RD_16(cards[ix].vcbase + 4))); 
	return B_OK;
}


static void
teardown_ess1938(
	ess1938_dev * card)
{
	cpu_status cp;

	if(card->dma_area >= B_OK) {
		delete_area (card->dma_area);
	}
	if(card->dma_cap_area >= B_OK) {
		delete_area (card->dma_cap_area);
	}
	if(card->open_close_sem >= B_OK) {
		delete_sem (card->open_close_sem);
	}
	if(card->pcmwrite_sem >= B_OK) {
		delete_sem (card->pcmwrite_sem);
	}
	if(card->pcmread_sem >= B_OK) {
		delete_sem (card->pcmread_sem);
	}
	if(card->mixer_sem >= B_OK) {
		delete_sem (card->mixer_sem);
	}
	if(card->write_sem >= B_OK) {
		delete_sem (card->write_sem);
	}
	if(card->read_sem >= B_OK) {
		delete_sem (card->read_sem);
	}
	if(card->controller_sem >= B_OK) {
		delete_sem (card->controller_sem);
	}
	if(card->lastint_sem >= B_OK) {
		delete_sem (card->lastint_sem);
	}	
	cp = disable_interrupts();
	acquire_spinlock(&card->spinlock);

	/* mute output, turn off playback, etc... */
	/* at some time TBD...                    */
	disable_card_interrupts(card);

	release_spinlock(&card->spinlock);
	restore_interrupts(cp);

	/* remove created devices */
#if __INTEL__
	if (card->joy.driver) {
		(*gameport->delete_device)(card->joy.driver);
	}
	if (card->midi.driver) {
		(*mpu401->delete_device)(card->midi.driver);
	}
#endif	
	memset(card, 0, sizeof(*card));

	card->dma_area = B_ERROR;
	card->dma_cap_area = B_ERROR;		
	card->open_close_sem = B_ERROR;
	card->pcmwrite_sem = B_ERROR;
	card->pcmread_sem = B_ERROR;
	card->mixer_sem = B_ERROR;
	card->write_sem = B_ERROR;
	card->read_sem = B_ERROR;
	card->controller_sem = B_ERROR;
	card->lastint_sem = B_ERROR;
	/* we didn't create them, we don't delete them */
	card->old_play_sem = B_ERROR;
	card->old_cap_sem  = B_ERROR;

}

status_t
init_driver(void)
{
	int ix=0;
	pci_info info;
	uint32 ui32;

	num_cards = 0;	/* call me paranoid */
	num_names = 0;	/* call me paranoid */ 
	ddprintf("ess1938: init_driver()\n");

	if (get_module(pci_name, (module_info **) &pci)) {
		dprintf("ess1938: No PCI module (fatal)\n");
		return ENOSYS;
	}
#if __INTEL__
	if (get_module(mpu401_name, (module_info **) &mpu401)) {
		dprintf("ess1938: No MPU401 module (fatal)\n");
		put_module(pci_name);
		return ENOSYS;
	}
	if (get_module(gameport_name, (module_info **) &gameport)) {
		dprintf("ess1938: No gameport module (fatal)\n");
		put_module(mpu401_name);
		put_module(pci_name);
		return ENOSYS;
	}
#endif
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == ESS1938_VENDOR_ID &&
			info.device_id == ESS1938_DEVICE_ID) {
			if (num_cards == NUM_CARDS) {
				dprintf("Too many ESS1938 cards installed!\n");
				break;
			}
			memset(&cards[num_cards], 0, sizeof(ess1938_dev));
			cards[num_cards].info = info;
			if (setup_ess1938(&cards[num_cards])) {
				dprintf("Setup of ESS1938 %d failed\n", num_cards + 1);
				teardown_ess1938(&cards[num_cards]);
			}
			else {
				num_cards++;
			}
		}
		ix++;
	}
	if (!num_cards) {
		put_module(pci_name);
#if __INTEL__		
		put_module(mpu401_name);
		put_module(gameport_name);
#endif
		ddprintf("ess1938: no suitable cards found\n");
		return ENODEV;
	}

#if DEBUG
	add_debugger_command("ess1938", debug_ess1938, "ess1938 [card# (1 - n)]");
#endif

	return B_OK;
}

void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	ddprintf("ess1938: uninit_driver()\n");
	remove_debugger_command("ess1938", debug_ess1938);

	for (ix=0; ix<cnt; ix++) {
		teardown_ess1938(&cards[ix]);
	}
	
	put_module(pci_name);
#if __INTEL__
	put_module(mpu401_name);
	put_module(gameport_name);
#endif
}


const char **
publish_devices(void)
{
	int ix = 0;

	for (ix=0; names[ix]; ix++) {
		ddprintf("ess1938: publish %s\n", names[ix]);
	}
	return (const char **)names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	ddprintf("ess1938: find_device(%s)\n", name);

	for (ix=0; ix<num_cards; ix++) {
#if __INTEL__
		/* MPU 401 */
			if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
		/* Joystick */
		if (!strcmp(cards[ix].joy.name, name)) {
			return &joy_hooks;
		}
#endif
		/* PCM */
		if (!strcmp(cards[ix].pcm.oldname, name)) {
			return &pcm_hooks;
		}
	}
	ddprintf("ess1938: find_device(%s) failed\n", name);
	return NULL;
}

static int32
ess1938_interrupt(
	void * data)
{
	bigtime_t	time   = system_time();
	ess1938_dev * card = (ess1938_dev *)data;
	uchar u;
	uint16 v;
	int32 n;
	int32 handled = B_UNHANDLED_INTERRUPT;
	sem_id semx[3];	
	uint32 cnt = 0;

	acquire_spinlock(&card->spinlock);
	u = get_direct( card->iobase + IMR_ISR );
		
	if ( u & 0x10 ) {
		/* clears int 1 */
		get_direct(card->sbbase + RD_BUF_STATUS);
//KPRINTF("`");
		/* This is a count-down register, 0 == end */

		v = B_LENDIAN_TO_HOST_INT16(PCI_IO_RD_16(card->vcbase + 4));

		if (v >= card->pcm.config.rec_buf_size) {
			card->dma_cap_buf_ptr = card->dma_cap_buf + card->pcm.config.rec_buf_size; 
		}
		else {
			card->dma_cap_buf_ptr =  card->dma_cap_buf ;
		}

		/* compensate for interrupt latency */
		/* bytes / 4 = samples * 1000000 uS (system time) / 44.1 K samples/sec */
		/* n = (total buf size - # read) & buf size - 1) */
		n = (2*card->pcm.config.rec_buf_size-v-1)&(card->pcm.config.rec_buf_size-1);
		card->read_time = time - (n * 250000LL / (int64) card->pcm.config.sample_rate);
		card->read_total += card->pcm.config.rec_buf_size;

		n = atomic_add(&card->read_waiting,-1);
		if (n <= 0 ) {
			/* oops, no read waiting */
			atomic_add(&card->read_waiting, 1);
			//stop_dma_1(card);
			handled = B_HANDLED_INTERRUPT;
		}
		else {
			/* make cyril happy.... */
			semx[cnt++] = card->read_sem;
			handled = B_INVOKE_SCHEDULER;
		}
	}

	if ( u & 0x20 ) {
		/* clear the interrupt */
		set_indirect(	card->sbbase + MIXER_COMMAND,
						card->sbbase + MIXER_DATA,
						AUDIO_2_CTRL_2,
						0x00,
						0x80);

		v = B_LENDIAN_TO_HOST_INT16(PCI_IO_RD_16(card->iobase + 4));
		if ( v > card->pcm.config.play_buf_size) {
			card->dma_buf_ptr = card->dma_buf + card->pcm.config.play_buf_size;
		}
		else {
			card->dma_buf_ptr =  card->dma_buf;
		}

		/* compensate for interrupt latency */
		/* bytes / 4 = samples * 1000000 uS (system time) / 44.1 K samples/sec */
		/* n = (total buf size - # read) & buf size - 1) */
		n = ((2*card->pcm.config.play_buf_size)-v)&(card->pcm.config.play_buf_size-1);
		card->write_time = time - (n * 250000LL / (int64) card->pcm.config.sample_rate);
		card->write_total += card->pcm.config.play_buf_size ;

		n = atomic_add(&card->write_waiting,-1);
		if (n <= 0 ) {
			/* oops, no write waiting */
			atomic_add(&card->write_waiting, 1);
			memset(card->dma_buf_ptr, 0, card->pcm.config.play_buf_size);
			if (handled == B_UNHANDLED_INTERRUPT) {
				handled = B_HANDLED_INTERRUPT;
			}
		}
		else {
			/* make cyril happy.... */
			semx[cnt++] = card->write_sem;
			handled = B_INVOKE_SCHEDULER;
		}
		/* we must check to make sure that we stop on the right buffer. Otherwise, */
		/* we may stop before the buffer plays out--which may cause the pan bug    */	
		if (!(get_indirect(	card->sbbase + MIXER_COMMAND,
							card->sbbase + MIXER_DATA,
							AUDIO_2_CTRL_1) & 0x10)) {
			/* make cyril happy.... */
			semx[cnt++] = card->lastint_sem;
		}			
	}
	if (u & 0x80) {
		if (midi_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			if (handled == B_UNHANDLED_INTERRUPT) {
				handled = B_HANDLED_INTERRUPT;
			}
		}
	}

	release_spinlock(&card->spinlock);
	/* make cyril happy.... */
	for (n=0; n<cnt; n++) {
		release_sem_etc(semx[n], 1, B_DO_NOT_RESCHEDULE);
	}
	return handled;
}


void
increment_interrupt_handler(
	ess1938_dev * card)
{
	KPRINTF("ess1938: increment_interrupt_handler()\n");
	if (atomic_add(&card->inth_count, 1) == 0) {
		KPRINTF("ess1938: intline %d int %d\n", card->info.u.h0.interrupt_line, ess1938_interrupt);
		install_io_interrupt_handler(card->info.u.h0.interrupt_line,ess1938_interrupt, card, 0);
	}
}


void
decrement_interrupt_handler(
	ess1938_dev * card)
{
	KPRINTF("ess1938: decrement_interrupt_handler()\n");
	if (atomic_add(&card->inth_count, -1) == 1) {
		KPRINTF("ess1938: remove_io_interrupt_handler()\n");
		remove_io_interrupt_handler(card->info.u.h0.interrupt_line, ess1938_interrupt, card);
	}
}


