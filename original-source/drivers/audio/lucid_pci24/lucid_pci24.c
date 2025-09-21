#include "audio_driver.h"
#include "lucid_pci24.h"
#include "dspcode.h"

#include <string.h>
#include <stdio.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#if !defined(_MODULE_H)
	#include <module.h>
#endif /* _MODULE_H */

EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);

int output_int_count(int a, char ** b);

//module
static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;

#define DO_PCM 1

#if DO_PCM
extern device_hooks pcm_hooks;
#endif /* DO_PCM */

int32 int_count = 0;

int32 num_cards=0;
pci24_dev cards[NUM_CARDS];
int num_names=0;
char * names[NUM_CARDS+1];
vuchar *io_base;

/* Send Host Command to DSP */
uint32
send_host_cmd(uint32 command, uint32 *dsp)
{
    // Pick a timeout value
    // TODO: Tune value to machine.
    uint32 	timeoutValue = 1000;
    uint32 	IOValue;
    
    do{
        IOValue = dsp[CVR];
        timeoutValue--;
    } while( (IOValue & 0x1) && timeoutValue );
    
    if(timeoutValue < 1) kprintf("pci24: Timeout send_host_command\n");
    
    // Write the command even if we did timeout
    dsp[CVR] = command;

    return timeoutValue;
}

/* Spin on the status bit until it's set.
 (Meaning the device is ready for more goo.) */
uint32
wait_on_status(uint32 *dsp)
{
    // Pick a timeout value
    // TODO: Tune value to machine.
    uint32 timeoutValue = 1000;
    uint32 IOValue;
    
    do
    {
        // Read the registers
        IOValue = dsp[STR];
        timeoutValue--;
        // Is the bit set?
    } while( ((IOValue & kHTRQ) != kHTRQ ) && timeoutValue );
    // then we're done.

    if(timeoutValue < 1) kprintf("pci24: Timeout wait_on_status\n");
    return IOValue;
}   

/* Disable Interrupts */
static void
disable_card_interrupts(uint32 *dsp)
{
	ddprintf("pci24: Turn off snd/rcv interrupts\n" );

    send_host_cmd( kHstCmdXmitDisable, dsp );
    wait_on_status( dsp );
    send_host_cmd( kHstCmdRecvDisable, dsp );
    wait_on_status( dsp );
}

static void
make_device_names(pci24_dev *card)
{
	char * name = card->name;
	sprintf(name, "audio/lucid_pci24_%d", card-cards+1);

#if DO_PCM
	//module
	if ((*pci->ram_address)(NULL) == NULL) {
		sprintf(card->pcm.name, "%s", name);
		names[num_names++] = card->pcm.name;
	}
#endif /* DO_PCM */
	names[num_names] = NULL;
}


static void
download_dsp_code(pci24_dev *card)
{
//	ddprintf("pci24: download_dsp_code()\n");

    uint32		tmp;
    uint32     	sizeOfCode = sizeof(dspCode)/4;
    uint32     	dspControlReg = 0x00000900; // This sets up the DSP for ***24-bit transfers Left-Justified***
    uint32     	dspStatusReg = 0;

    // Reset the dsp->
    // This is needed in case of a warm boot. If the DSP code
    // is already downloaded and running, this will put it in
    // the proper state. If the DSP code hasn't been downloaded
    // yet, this is a harmless command.
	// DON'T USE -- CRASHER!
    //send_host_cmd(kHstCmdReset, card->dsp);
    
    // wait until the DSP says it's ready (or times out).
    wait_on_status(card->dsp);
    
    // Set up 24-bit transfers (LS 24-bits of a 23-bit word = program word)
    card->dsp[CTR] = dspControlReg;
    
    wait_on_status(card->dsp);

    // Tell the DSP how many words are coming its way
    card->dsp[RXS] = sizeOfCode;
    
    wait_on_status(card->dsp);
    
    // Tell the DSP the bootload start address
    card->dsp[RXS] = 0x0;
    
    wait_on_status(card->dsp);
    
    // download the code
    for(tmp=0; tmp < sizeOfCode; tmp++)
    {
        card->dsp[RXS] = dspCode[tmp];
        wait_on_status(card->dsp);
    }
    ddprintf("pci24: DSP code download done\n");
    
    // The entire host program has been sent, now read the HostSTR
    dspControlReg = card->dsp[CTR];
    dspControlReg = dspControlReg | kHF0;
    card->dsp[CTR] = dspControlReg;

    dspStatusReg = card->dsp[STR];
    ddprintf("pci24: status reg = %x\n", dspStatusReg );
    wait_on_status(card->dsp);
    
    disable_card_interrupts(card->dsp);
    
    switch(FIFO_DEPTH)// set DSP buffer size
    {
		case 0x1000:
			ddprintf("pci24: Setting 0x1000 fifo size\n" );
			send_host_cmd( kHstCmdBuffer1000, card->dsp );
			break;
		case 0x800:
			ddprintf("pci24: Setting 0x800 fifo size\n" );
			send_host_cmd( kHstCmdBuffer800, card->dsp );
			break;
		case 0x400:
			ddprintf("pci24: Setting 0x400 fifo size\n" );
			send_host_cmd( kHstCmdBuffer400, card->dsp );
			break;
		case 0x200:
			ddprintf("pci24: Setting 0x200 fifo size\n" );
			send_host_cmd( kHstCmdBuffer200, card->dsp );
			break;
		case 0x100:
		default:
			ddprintf("pci24: Setting 0x100 fifo size\n" );
			send_host_cmd( kHstCmdBuffer100, card->dsp );
			break;
	}
    wait_on_status(card->dsp);

    // set DSP's transmit sample rate to 44k 
    ddprintf("pci24: Setting 44k output rate\n" );
    send_host_cmd( kHstCmdXmt44, card->dsp );
    wait_on_status(card->dsp);

    // set DSP's receiver sample rate to 44k
    ddprintf("pci24: Setting 44k input rate\n" );
    send_host_cmd( kHstCmdRcv44, card->dsp );
    wait_on_status(card->dsp);
/*    
	// turn on playthru (monitoring) for testing
    ddprintf("pci24: Setting monitoring ON\n" );
   	send_host_cmd( kHstCmdPlaythruEnable, card->dsp );
    wait_on_status(card->dsp);
*/

}

static status_t
setup_pci24(pci24_dev *card)
{
	status_t 	err = B_OK;
	area_id 	reg_area;
	uint32		*reg_base;
	uint32  	base;
	uint32		size;

	ddprintf("pci24: setup_lucid_pci24(%x)\n", card);

	acquire_spinlock(&card->hardware);

	make_device_names(card);

	base = card->info.u.h0.base_registers[0]; /* get address of the reg */ 
    size = card->info.u.h0.base_register_sizes[0];   /* get reg size */ 
      
    /* Round the base address of the register down to 
    the nearest page boundary. */ 
    base = base & ~(B_PAGE_SIZE - 1);
      
    /* Adjust the size of our register space based on 
    the rounding we just did. */ 
    size += card->info.u.h0.base_registers[0] - base;
      
    /* And round up to the nearest page size boundary, 
    so that we occupy only complete pages, and not 
    any partial pages. */ 
    size = (size + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1); 
      
    /* Now we ask the kernel to create a memory area 
    which locks this I/O space in memory. */ 
   
	ddprintf("pci24: about to map memory\n");
    reg_area = map_physical_memory ("lucid_pci24_device_regs", 
          							(void *)base, 
          							size, 
          							B_ANY_KERNEL_ADDRESS, 
          							B_READ_AREA + B_WRITE_AREA, 
          							&reg_base); 
    
   	/* Negative results from map_physical_memory() are 
    errors.  If we got an error, return that. */ 
    if ((int32)reg_area < 0){
		ddprintf("pci24: map memory didn't work\n");
		release_spinlock(&card->hardware);
	   	return reg_area;
	}    
	
	ddprintf("pci24: map memory worked!\n");
	card->area = reg_area;
	
	/* correct for offset within page */
	reg_base += (card->info.u.h0.base_registers[0] & (B_PAGE_SIZE -1))/sizeof(uint32);
	ddprintf("pci24: %s base at %x\n", card->name, reg_base);

	card->dsp = reg_base;	
	
	/* Enable card for memory space accesses */	
	(*pci->write_pci_config)(card->info.bus,card->info.device,card->info.function,0x04,4,0x02800002);
	
	download_dsp_code(card);
	
	ddprintf("pci24: creating buffer area\n");
	card->buf_area = create_area("lucid_pci24", (void *)&card->buf,
									B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, 
									B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if(card->buf_area < B_OK){
		err = card->buf_area;
		ddprintf("pci24: error creating area\n");
	}
	ddprintf("pci24: %s buffer base at %x\n", card->name, card->buf);
	
	release_spinlock(&card->hardware);

	return err;
}


static int
debug_pci24(int argc, char * argv[])
{
	int ix = 0;
	if (argc == 2) {
		ix = parse_expression(argv[1])-1;
	}
	if (argc > 2 || ix < 0 || ix >= num_cards) {
		ddprintf("pci24: man, you gotta watch your syntax!\n");
		return -1;
	}
	//Call dprintf() with various useful information here (specific to device)
	return 0;
}


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	int ix=0;
	pci_info info;
	status_t err = ENODEV;

	ddprintf("pci24: init_hardware()\n");

//module
	if (get_module(pci_name, &(module_info *)pci))
		return ENOSYS;

//module
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {	
	//while (get_nth_pci_info(ix, &info) == B_OK) {
		if (info.device_id == PCI24_DEVICE_ID &&
			info.vendor_id == LUCID_VENDOR_ID){
			err = B_OK;
			ddprintf("pci24: info.vendor_id = %x\n", info.vendor_id);
			ddprintf("pci24: info.device_id = %x\n", info.device_id);
		}
		ix++;
	}		

#if defined(__POWERPC__)
	{
		char		area_name [32];
		area_info	area;
		area_id		id;

		sprintf (area_name, "pci_bus%d_isa_io", info.bus);
		id = find_area (area_name);
		if (id < 0)
			err = id;
		else if ((err = get_area_info (id, &area)) == B_OK)
			io_base = area.address;
	}
#endif
		
//module
	put_module(pci_name);
	
	return err;
}

int
output_int_count(int a, char ** b)
{
	kprintf("int_count = %d\n", int_count);
	return 0;
}

/* initialize your driver (not necessarily the hardware) */
status_t
init_driver(void)
{
	int ix=0;
	pci_info info;
	num_cards = 0;

	ddprintf("pci24: init_driver()\n");

	add_debugger_command("pci24", debug_pci24, "pci24 [card# (1-n)]");
	add_debugger_command("pci24_int_count", output_int_count, "pci24 [card# (1-n)]");

	load_driver_symbols("lucid_pci24");
//module
	if (get_module(pci_name, &(module_info *)pci))
		return ENOSYS;

	//module
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
	//while (get_nth_pci_info(ix, &info) == B_OK) {
		if (info.device_id == PCI24_DEVICE_ID &&
			info.vendor_id == LUCID_VENDOR_ID) {
			if (num_cards == NUM_CARDS) {
				ddprintf("Too many pci24 cards installed!\n");
				break;
			}
			memset(&cards[num_cards], 0, sizeof(pci24_dev));
			cards[num_cards].info = info;
			if (setup_pci24(&cards[num_cards])) {
				ddprintf("pci24: Setup of pci24 %d failed\n", num_cards+1);
			}
			else {
				num_cards++;
			}
		}
		ix++;
	}
	if (!num_cards) {
		//module
		put_module(pci_name);
		ddprintf("pci24: no suitable cards found\n");
		return ENODEV;
	}

	return B_OK;
}


static void
teardown_pci24(pci24_dev *card)
{
	ddprintf("pci24: teardown()\n");
	
	delete_area (card->area);
	delete_area (card->buf_area);
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	ddprintf("pci24: uninit_driver()\n");
	ddprintf("pci24: int count %d\n", int_count);
	remove_debugger_command("pci24", debug_pci24);
	remove_debugger_command("pci24_int_count", output_int_count);

		
	for (ix=0; ix<cnt; ix++) {
		teardown_pci24(&cards[ix]);
	}
	memset(&cards, 0, sizeof(cards));
//module
	put_module(pci_name);
}


const char **
publish_devices(void)
{
	ddprintf("pci24: publish_devices()\n");

	return names;
}


device_hooks *
find_device(const char *name)
{
	int ix;

	ddprintf("pci24: find_device(%s)\n", name);

	for (ix=0; ix<num_cards; ix++) {
#if DO_PCM
		if (!strcmp(cards[ix].pcm.name, name)) {
			return &pcm_hooks;
		}
#endif /* DO_PCM */
	}
	ddprintf("pci24: find_device(%s) failed\n", name);
	return NULL;
}


static void
empty_fifo(void * data)
{
	int32 i=0;
	int32 sample=0;
	pci24_dev *card = (pci24_dev *)data;
	int16 *output = card->pcm.rd_int_ptr;
	int16 *base = card->pcm.rd_base;
	
	while(i<FIFO_DEPTH)
	{
		wait_on_status(card->dsp);
		//read the sample
		sample = card->dsp[RXS];
		/*shift sample into 16bits*/
		sample >>= 8;
		//sample = (int32)output;
		*output = (int16)sample;
		output++;
		if(output-base >= RD_BUF_SIZE/2)
		{
			output = base;
			//kprintf("pci24: cycling int_ptr\n");
		}
		i++;
	}
	card->pcm.rd_int_ptr = output;
}

static void
fill_fifo(void *data)
{
	int i=0;
	int32 sample=0;
	pci24_dev *card = (pci24_dev *)data;
	int16 *input = card->pcm.wr_int_ptr;
	int16 *base = card->pcm.wr_base;
	
	while(i<FIFO_DEPTH)
	{
		/*do format conversions as necessary*/
		sample = (int32)*input;
		sample <<= 8;
			
		/*copy the data from the buffer into the fifo*/
		// Wait on the device...                
        wait_on_status(card->dsp);
        // write the sample             
        card->dsp[TXR] = sample;

		input++;
		if(input-base >= RD_BUF_SIZE/2)
		{
			input = base;
			//kprintf("pci24: cycling int_ptr\n");
		}
		i++;
	}
	card->pcm.wr_int_ptr = input;
}

static void
fill_silence(void *data)
{
	int i=0;
	int32 sample=0;
	pci24_dev *card = (pci24_dev *)data;
	
	while(i<FIFO_DEPTH)
	{
		/*do format conversions as necessary*/
		sample = 0;
			
		/*copy the data from the buffer into the fifo*/
		// Wait on the device...                
        wait_on_status(card->dsp);
        // write the sample             
        card->dsp[TXR] = sample;

		i++;
	}
}
	

/* This is the main interrupt service routine; it gets called for */
/* all subsystems of the pci24 card, and dispatches to whatever */
/* specific routine is needed. */

static int32
pci24_interrupt(void * data)
{
	pci24_dev *card = (pci24_dev *)data;
	uint32 DspStatus = 0;
	int32  count = 0;
	status_t err = B_OK;
	int32 handled = B_UNHANDLED_INTERRUPT;

	int_count++;
	
	acquire_spinlock(&card->hardware);
	DspStatus = card->dsp[STR];
	release_spinlock(&card->hardware);
	
	//kprintf("int\n");
		
#if DO_PCM
	//Did the interrupt come from the dsp?
    if( DspStatus & kHINT )
    {
		handled = B_HANDLED_INTERRUPT;
        // Is this a record interrupt?
        if( DspStatus & kRecReq )
        {
			card->pcm.next_rd_time = system_time();
            //kprintf("rec\n");
			acquire_spinlock(&card->hardware);
    		send_host_cmd( kHstIntAckRec, card->dsp );
			empty_fifo(data);
			release_spinlock(&card->hardware);
			if(atomic_or(&card->pcm.read_waiting, 1) == 0)
			{
				err = release_sem_etc(card->pcm.rd_int_sem, 1, B_DO_NOT_RESCHEDULE);
				if(err < B_OK)
					kprintf("pci24: read what the f*ck?\n");
			}
        }
        // Is this a playback interrupt?
        if( DspStatus & kPlayReq )
        {
            //kprintf("play\n");
			acquire_spinlock(&card->hardware);
    		send_host_cmd( kHstIntAckPlay, card->dsp );
			release_spinlock(&card->hardware);
			if(atomic_or(&card->pcm.write_waiting, 1) == 0)
			{
				fill_fifo(data);
				err = release_sem_etc(card->pcm.wr_int_sem, 1, B_DO_NOT_RESCHEDULE);
				if(err < B_OK)
					kprintf("pci24: write what the f*ck?\n");
			}
			else
			{
				fill_silence(data);
			}
		}
	}
#endif /* DO_PCM */

	/* It is VERY IMPORTANT to return TRUE if and only if you found an actual */
	/* serviceable condition. This is because of interrupt sharing. */
	return handled; 
}


void
increment_interrupt_handler(pci24_dev *card)
{
	if (atomic_add(&card->inth_count, 1) == 0) {
		ddprintf("pci24: install interrupt handler\n");
		ddprintf("pci24: interrupt_line = %x\n", card->info.u.h0.interrupt_line);
		if(install_io_interrupt_handler(card->info.u.h0.interrupt_line,
										pci24_interrupt, (void *)card, 0) != B_OK)
			ddprintf("pci24: install interrupt handler failed!\n");
	}
}


void
decrement_interrupt_handler(pci24_dev *card)
{
	if (atomic_add(&card->inth_count, -1) == 1) {
		ddprintf("pci24: remove interrupt handler\n");
		remove_io_interrupt_handler(card->info.u.h0.interrupt_line,
			pci24_interrupt, card);
	}
}


