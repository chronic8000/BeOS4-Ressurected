#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>

#include "sonorus.h"

/* debug stuff */
#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf (void)
#endif

/* globals */
extern sonorus_dev cards[MAX_STUDIO_BD];

extern int card_count;
extern int dsp_caps;
extern int outputs_per_bd;
extern int inputs_per_bd;
extern int packed_mode;
extern int is_playing;

/* explicit declarations */
uint32	 get_host_stat( int );
uint32	 get_host_ctrl( int );
uint32	 read_host_word( int );
void	 send_host_cmd(	int, uint32 );
void	 write_host_word( int, uint32 );
void	 set_host_ctrl( int, uint32 );
void	 set_host_ctrl_bits( int, uint32, uint32);
void	 read_host_buf( int, void *, uint32, studXferType_t, void * );
void	 write_host_buf( int, void *, uint32, studXferType_t, void * );
void	 send_host_cmd_nmi( int, uint32);



typedef enum { BCAST_PARM , CHNL_PARM } parmType_t;
typedef struct
{
	uint32		dspLoc;
	memSpace_t	dspMem;
	parmType_t	parmType;
}
parmInfo_t;

/* BCAST_PARM = g/set_device_param */
/* CHNL_PARM = g/set_channel_param */

static parmInfo_t parmInfo[] =
{
	{	0x003	,	xMem	,	BCAST_PARM	}		// STUD_PARM_SPEED
  , {	0x005	,	xMem	,	BCAST_PARM	}		// STUD_PARM_POSL
  , {	0x004	,	xMem	,	BCAST_PARM	}		// STUD_PARM_POSH
  , {	0x007	,	xMem	,	BCAST_PARM	}		// STUD_PARM_INITOFST
  , {	0x050	,	yMem	,	CHNL_PARM	}		// STUD_PARM_METER
  , {	0x030	,	xMem	,	CHNL_PARM	}		// STUD_PARM_INPMON
  , {	0x040	,	xMem	,	CHNL_PARM	}		// STUD_PARM_OUTENB
  , {	0x002	,	xMem	,	BCAST_PARM	}		// STUD_PARM_RECENB
  , {	0x000	,	xMem	,	BCAST_PARM	}		// STUD_PARM_SRATE
  , {	0x001	,	xMem	,	BCAST_PARM	}		// STUD_PARM_SPARECYCS
  ,	{	0x014	,	xMem	,	BCAST_PARM	}		// STUD_PARM_PORTMODE
  ,	{	0x016	,	xMem	,	BCAST_PARM	}		// STUD_PARM_PACKMODE
  ,	{	0x017	,	xMem	,	BCAST_PARM	}		// STUD_PARM_SYNCSELECT
  ,	{	0xFFFF8A,	xMem	,	BCAST_PARM	}		// STUD_PARM_DSPSRADJ
  ,	{	0x01A	,	xMem	,	BCAST_PARM	}		// STUD_PARM_BOARDREV
  , {	0x01D	,	xMem	,	BCAST_PARM	}		// STUD_PARM_TIMECODEL
  , {	0x01C	,	xMem	,	BCAST_PARM	}		// STUD_PARM_TIMECODEH
  , {	0x020	,	xMem	,	BCAST_PARM	}		// STUD_PARM_TCPOSL
  , {	0x021	,	xMem	,	BCAST_PARM	}		// STUD_PARM_TCPOSH
};

#define N_PARM_INFO	(sizeof(parmInfo)/sizeof(parmInfo_t))

#define STUD_RESET_TIMEOUT 1000 // was 24000
#define STAT_POLL_TIMEOUT 512 // 256 these should be TIME based!!

//#define ADJUST_MODULO( a , n )	if ((a) < 0) (a) += (n); else if ((a) >= (n)) (a) -= (n)
#define ADJUST_MODULO_US( a , n) if ((a) >= (n)) (a) -= (n)

uint32
read_loc(
	int ix,
	uint32 loc,
	memSpace_t memtype)
{
	uint32 data;
	cpu_status cp;
	int timeout;
	
	/* make sure host interface completely empty! */
#if 0
	stat = get_host_stat(ix);
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		/* I don't know why we usually fail first time through */
		kprintf("sonorus: host is not ready in read_loc()\n");
		spin(1000);
	}
#else
	timeout = 0;
	while ((get_host_stat(ix) & M_HSTR_TRDY) == 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			kprintf("studio: read_loc(%d, 0x%lx) timeout\n", ix, loc);
			break;
		}
		else {
			spin(1);
		}
	}
#endif	
	cp = disable_interrupts();
	acquire_spinlock(&cards[ix].spinlock);
	{
		/* do host commands to read location */
		write_host_word(ix, loc);
		send_host_cmd(ix, HC_SETR7);
		send_host_cmd(ix, HC_READX + memtype);
		data = read_host_word(ix);
	}
	release_spinlock(&cards[ix].spinlock);
	restore_interrupts(cp);

	return data;
}

void
write_loc(
	int ix,
	uint32 address,
	memSpace_t memtype,
	uint32 data)
{
	uint32 stat;
	int timeout;
	cpu_status cp;

	/* make sure the host interface is completely empty! */
	stat = get_host_stat(ix);
	if (!(stat & M_HSTR_TRDY)) {
		/* this normally an assert! */
		kprintf("sonorus: write_loc() not ready\n");
		spin(1000);
	}
	
	cp = disable_interrupts();
	acquire_spinlock(&cards[ix].spinlock);
	{
		/* do host commands to write location */
		write_host_word(ix, address);
		send_host_cmd(ix, HC_SETR7);
		write_host_word(ix, data);
		send_host_cmd(ix, HC_WRITEX + memtype);
	}
	release_spinlock(&cards[ix].spinlock);
	restore_interrupts(cp);

	/* wait for TX port to drain */
	timeout = 0;
	while ((get_host_stat(ix) & M_HSTR_TRDY) == 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			kprintf("sonorus: write_loc(%d, 0x%lx) timeout\n", ix, address);
			break;
		}
		else {
			spin(1);
		}
	}
}

void
read_buf(
		int ix,
		uint32 addr ,
		memSpace_t memSpace ,
		uint32* data ,
		uint32 len )
{
	uint32		stat;
	cpu_status cp;
	long timeout = 0; 	
	/* make sure host interface completely empty! */
	stat = get_host_stat(ix);
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		kprintf("sonorus: host is not ready in read_buf()\n");
		spin(1000);
	}

	cp = disable_interrupts();
	acquire_spinlock(&cards[ix].spinlock);
	{
		write_host_word( ix, addr );
		send_host_cmd( ix, HC_SETR7 );
		while (len-- > 0)
		{
			send_host_cmd( ix, HC_READX + memSpace );
			*data++ = read_host_word(ix);
		}
	}
	release_spinlock(&cards[ix].spinlock);
	restore_interrupts(cp);
	
	// wait for TX port to drain
	while ((get_host_stat(ix) & M_HSTR_TRDY) == 0)
		if (timeout++ > STAT_POLL_TIMEOUT) {
			ddprintf("sonorus: read buf end timeout\n");
			break;
		}	
}

void
write_buf(
		int ix,
		uint32 addr ,
		memSpace_t memSpace ,
		uint32* data ,
		uint32 len )
{
	uint32		stat;
	cpu_status cp;
	long timeout = 0; 	
	/* make sure host interface completely empty! */
	stat = get_host_stat(ix);
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		kprintf("sonorus: host is not ready in write_buf()\n");
		spin(1000);
	}

	cp = disable_interrupts();
	acquire_spinlock(&cards[ix].spinlock);
	{
		write_host_word( ix, addr );
		send_host_cmd( ix, HC_SETR7 );
		while (len-- > 0)
		{
			write_host_word( ix, *data++ );
			send_host_cmd( ix, HC_WRITEX + memSpace );
		}
	}
	release_spinlock(&cards[ix].spinlock);
	restore_interrupts(cp);
	
	// wait for TX port to drain
	while ((get_host_stat(ix) & M_HSTR_TRDY) == 0)
		if (timeout++ > STAT_POLL_TIMEOUT) {
			ddprintf("sonorus: write buf end timeout\n");
			break;
		}	
}

void
read_x_fast(
		uint16 bdNo ,
		uint32 addr ,
		uint32 stride ,
		void*  data ,
		uint32 len ,
		studBufType_t bufType ,
		void* args )
{
	uint32		stat, timeout;
	cpu_status	cp;
#ifdef NO_SHIFT	
	uint32		savedHCTR;
#endif

	/* no packed mode support, period. */
	packed_mode = false;		
	
	/* make sure host interface completely empty and DMA not in progress. */
	stat = get_host_stat(bdNo);
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		dprintf("sonorus: host is not ready in read_x_fast()\n");
		spin(1000);
	}
	stat = get_host_stat( bdNo );
	if (( stat & M_HSTR_HRRQ )) {
		/* this normally an assert! */
		dprintf("sonorus: buffer not empty in read_x_fast()\n");
		spin(1000);
	}
	stat = get_host_stat( bdNo );
	if (( stat & M_HSTR_HF5 )) {
		/* this normally an assert! */
		dprintf("sonorus: dma not done in read_x_fast()\n");
		spin(1000);
	}

	cp = disable_interrupts();
	acquire_spinlock(&cards[bdNo].spinlock);

		// set up r7 to base address, then enable DSP tx interrupt
		write_host_word( bdNo, addr );
		send_host_cmd( bdNo, HC_SETR7 );
		write_host_word( bdNo, stride );
		send_host_cmd( bdNo, HC_SETN7 );
		send_host_cmd( bdNo, HC_FXR_ON );

		// wait for some data to show up.
		timeout = 0;
		while ((get_host_stat( bdNo ) & M_HSTR_TRDY) == 0) {
			if (timeout++ > STAT_POLL_TIMEOUT){
				kprintf( "sonorus: DMA TRDY start timeout\n" );
				break;
			}
		}	
		timeout = 0;
		while ((get_host_stat( bdNo ) & M_HSTR_HRRQ) == 0) {
			if (timeout++ > STAT_POLL_TIMEOUT) {
				kprintf( "sonorus: DMA read start timeout\n" );
				break;
			}	
		}

	release_spinlock(&cards[bdNo].spinlock);
	restore_interrupts(cp);

	/* start the fast transfer based on buffer type. */
	switch (bufType)
	{
		case STUD_BUF_INT32:
#ifdef NO_SHIFT
			/* save value of HCTR */
			savedHCTR = get_host_ctrl(bdNo);

			/* let's not be the slowest.... */
			set_host_ctrl_bits( bdNo,
								M_HCTR_HTF_MASK | M_HCTR_HRF_MASK ,
								M_HCTR_HTF3     | M_HCTR_HRF3 ); /* left aligned */									
			read_host_buf( bdNo, data , len , kStudXfer32 , NULL );
			set_host_ctrl( bdNo, savedHCTR );
#else
			read_host_buf( bdNo, data , len , kStudXfer32 , NULL );
#endif
			break;
			
		case STUD_BUF_INT16:
		case STUD_BUF_INT16_GAIN:
		case STUD_BUF_FLOAT_GAIN:		
		default:
			dprintf( "sonorus: Unsupported xfer type in read_x_fast\n" );
			break;
	}

	// stop fast interrupt reads!!!
	cp = disable_interrupts();
	acquire_spinlock(&cards[bdNo].spinlock);
		send_host_cmd( bdNo, HC_FXR_OFF );
		get_host_stat( bdNo );
	release_spinlock(&cards[bdNo].spinlock);
	restore_interrupts(cp);

	// flush any extra data
	timeout = 0;
	while ((get_host_stat( bdNo ) & M_HSTR_HRRQ) != 0) {
		stat = read_host_word( bdNo );

		cp = disable_interrupts();
		acquire_spinlock(&cards[bdNo].spinlock);
			send_host_cmd( bdNo, HC_FXR_OFF );
		release_spinlock(&cards[bdNo].spinlock);
		restore_interrupts(cp);

		if (timeout++ > 16) {
			dprintf( "sonorus: DMA read drain timeout\n" );
			break;
		}	
	}
	timeout = 0;
	while ((get_host_stat( bdNo ) & M_HSTR_HF5) != 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			dprintf( "sonorus: DMA HF5 end timeout\n" );
			break;
		}
	}
}



void
write_x_fast(
			uint16 bdNo,
			uint32 addr,
			uint32 stride,
			void*  data,
			uint32 len,
			studBufType_t bufType,
			void* args )
{
	uint32		stat, timeout;
	cpu_status cp;
#ifdef NO_SHIFT
	uint32		savedHCTR;
#endif	
	
	stat = get_host_stat( bdNo );
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		dprintf("sonorus: host is not ready in write_x_fast()\n");
		spin(1000);
	}
	stat = get_host_stat( bdNo );
	if (( stat & M_HSTR_HF5 )) {
		/* this normally an assert! and was !---- */
		//kprintf("sonorus: dma not done in write_x_fast()\n");
		spin(1000);
		stat = get_host_stat( bdNo );
		if (( stat & M_HSTR_HF5 )) {
			dprintf("sonorus: write_x_fast dma [fatal] 0x%lx\n",stat);
			return;
		}
	}

	/* if packed mode, ensure even number of samples to xfer */
	/* we do not currently support packed data */
	if (packed_mode) {
		dprintf("sonorus: packed mode not supported (fatal)\n");
		return;
	}

	cp = disable_interrupts();
	acquire_spinlock(&cards[bdNo].spinlock);
	{
		write_host_word( bdNo, addr );
		write_host_word( bdNo, stride );
		write_host_word( bdNo, (len - 1) << 12 );
		send_host_cmd( bdNo, HC_DMA_WR_X );
		timeout = 0;
		while ((get_host_stat( bdNo ) & M_HSTR_HF5) == 0) {
			if (timeout++ > STAT_POLL_TIMEOUT) {
				kprintf( "sonorus: DMA start timeout" );
				break;
			}
		}
	}	
	release_spinlock(&cards[bdNo].spinlock);
	restore_interrupts(cp);
	switch (bufType)
	{
		case STUD_BUF_INT32:
#ifdef NO_SHIFT
			/* let's not be the slowest.... */
			// save value of HCTR 
			savedHCTR = get_host_ctrl(bdNo);
			set_host_ctrl_bits( bdNo,
								M_HCTR_HTF_MASK | M_HCTR_HRF_MASK ,
								M_HCTR_HTF3 | M_HCTR_HRF3 ); /* left aligned */									
			write_host_buf( bdNo, data , len , kStudXfer32 , NULL );
			set_host_ctrl( bdNo, savedHCTR );
#else
			write_host_buf( bdNo, data , len , kStudXfer32 , NULL );
#endif			
			break;
	
		case STUD_BUF_INT16:
		case STUD_BUF_INT16_GAIN:
		case STUD_BUF_FLOAT_GAIN:
		default:
			dprintf( "sonorus: Unsupported xfer type in write_x_fast()\n" );
		break;
	}
	
	timeout = 0;
	while (((stat = get_host_stat( bdNo )) & M_HSTR_TRDY) == 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			dprintf( "sonorus: write drain timeout in write_x_fast\n" );
			break;
		}
	}		
	timeout = 0;
	while ((stat = get_host_stat( bdNo ) & M_HSTR_HF5) != 0) {
		if (timeout++ > STAT_POLL_TIMEOUT ) {
			dprintf( "sonorus: DMA end timeout in write_x_fast \n" );
			break;
		}
	}		
}

void
fill_x_fast(
		uint16 bdNo,
		uint32 addr,
		uint32 stride,
		uint32 data,
		uint32 len )
{
	uint32		i, stat, timeout;
	cpu_status cp;
	
	/* make sure host interface completely empty! */
	stat = get_host_stat( bdNo );
	if (!(stat & M_HSTR_TRDY )) {
		/* this normally an assert! */
		kprintf("sonorus: host is not ready in fill_x_fast()\n");
		spin(1000);
	}

	cp = disable_interrupts();
	acquire_spinlock(&cards[bdNo].spinlock);
	{
		write_host_word( bdNo, addr );
		write_host_word( bdNo, stride );
		write_host_word( bdNo, (len-1) << 12 );
		send_host_cmd( bdNo, HC_DMA_WR_X );
	}
	release_spinlock(&cards[bdNo].spinlock);
	restore_interrupts(cp);
	
	timeout = 0;
	while ((get_host_stat( bdNo ) & M_HSTR_HF5) == 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			ddprintf( "sonorus: DMA start timeout\n" );
			break;
		}
	}		

	// I'm thinking that only send_host_cmd needs protection....
	for (i = 0; i < len; i++) {
		write_host_word( bdNo, data );
	}
			
	timeout = 0;
	while ((get_host_stat( bdNo ) & M_HSTR_HF5) != 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			ddprintf( "sonorus: DMA end timeout\n" );
			break;
		}
	}		
	timeout = 0;
	while ((get_host_stat( bdNo ) & M_HSTR_TRDY) == 0) {
		if (timeout++ > STAT_POLL_TIMEOUT) {
			ddprintf( "sonorus: write drain timeout\n" );
			break;
		}
	}		
}

status_t
init_dsp(
	int ix,
	uint32 * image,
	size_t count)
{
	uint32 i;
	uint32 loc;
	uint32 timeout;
	uint32 retry;
	bool wasRunning = false;
	cpu_status cp;
	status_t err = B_OK;
//	uint32 stat;
//	stat = get_host_ctrl(ix);

	ddprintf("sonorus: init_dsp()\n");

	/* wait for DSP->host FIFO to be completely empty! */
	if (get_host_stat(ix) & M_HSTR_HRRQ) {
		ddprintf("DSP was running\n");
		wasRunning = true;
		timeout = 0;
		while (get_host_stat(ix) & M_HSTR_HRRQ) {
			spin(100);
			if (timeout++ > STUD_RESET_TIMEOUT) {
				kprintf("sonorus: timeout draining host FIFO (init_dsp)\n");
				return B_TIMED_OUT;
			}
			(void)read_host_word(ix);
		}
	}
	
	/* wait for host->DSP FIFO to be completely empty! */
	timeout = 0;
	while ((get_host_stat(ix) & M_HSTR_TRDY) == 0) {
		wasRunning = true;
		spin(100);
		if (timeout++ > STUD_RESET_TIMEOUT) {
			dprintf("sonorus: timeout draining DSP FIFO (trying)\n");
			return B_TIMED_OUT;
		}
	}
	
	/* if appeared to be running, try issuing a reset host command. */
	if (wasRunning) {
		cp = disable_interrupts();
		acquire_spinlock(&cards[ix].spinlock);
			send_host_cmd_nmi(ix, HC_RESET);
		release_spinlock(&cards[ix].spinlock);
		restore_interrupts(cp);
		
		snooze(1000);
		timeout = 0;
		while ((get_host_stat(ix) & M_HSTR_TRDY) == 0) {
			spin(100);
			if (timeout++ > STUD_RESET_TIMEOUT) {
				dprintf("sonorus: error emptying host port at boot\n");
				return EIO;
			}
		}
	}
	
	/* set the host port control for 24 bit xfers, right alligned.			*/
	/* (this also assures that HF0 is cleared, otherwise booting aborts!).	*/
	set_host_ctrl(ix,  (M_HCTR_HRF1|M_HCTR_HTF1));
	
	/* write bootstrap header (nWords, org), make sure the first word isn't stuck */
	retry = 0;
	cp = disable_interrupts();
	acquire_spinlock(&cards[ix].spinlock);
	while (true) {
		write_host_word(ix, count);
		spin(5);
		if ((get_host_stat(ix) & M_HSTR_TRDY) != 0) {
			break;
		}
		send_host_cmd_nmi(ix, HC_RESET);
		release_spinlock(&cards[ix].spinlock);
		restore_interrupts(cp);
		if (retry++ > 5) {
			dprintf("sonorus: unable to reset DSP\n");
			return EIO;
		}
		snooze(1000);
		cp = disable_interrupts();
		acquire_spinlock(&cards[ix].spinlock);
	}
	/*	come out of loop with interrupts disabled */

	/* write the rest of the image. */
	write_host_word(ix, 0);			/*	org at zero */

	for (i = 0; i < count; i++) {
		timeout = 0;
		while ((get_host_stat(ix) & M_HSTR_TRDY) == 0) {
			spin(10);
			if (timeout++ > STUD_RESET_TIMEOUT) {
				kprintf( "sonorus: timeout loading DSP code\n" );
				err = B_TIMED_OUT;
				break;
			}
		}
		write_host_word(ix,  image[i]);
	}
	release_spinlock(&cards[ix].spinlock);
	restore_interrupts(cp);

	if (err < B_OK) {
		return err;
	}
	snooze(100);

	// now check proper load by reading back using host commands
	loc = read_loc(ix, 255, pMem);
	ddprintf("sonorus: code version 0x%06lx\n", loc);
	if (loc != image[255]) {
		dprintf("sonorus: code version mismatch [0x%06lx != 0x%06lx]\n", loc, image[255]);
		return EIO;
	}

#if CHECK_CODE
	for (i = 0; i < count; i++) {
		if ((loc = read_loc(ix, i, pMem)) != image[i]) {
			dprintf("sonorus: code corrupt at 0x%06x [%06x != %06x]\n",
				i, loc, image[i]);
		}
	}
#endif

	return B_OK;
}

status_t
init_altera(
	int ix,
	uint8 * image,
	size_t count)
{
	uint32 data;
	uint32 timeout;
	bool isRevB;
	cpu_status cp;
	status_t err = B_OK;

	ddprintf("sonorus: init_altera()\n");

	/* the DSP has checked at init whether an EEPROM is present */
	/*	- so read that flag										*/
	data = read_loc(ix, EEPROM_PRESENT_FLAG, xMem);
	isRevB = (data != 0);

	ddprintf("sonorus: isRevB: %s\n", isRevB ? "true" : "false");
	
	/*	disable interrupts, just to prevent re-scheduling */
	cp = disable_interrupts();
	/* pulse reprogram bit for 15 usec */
	write_loc(ix, M_AAR2, xMem, ALTERA_REPROGRAM_ON);
	spin(15);
	write_loc(ix, M_AAR2, xMem, ALTERA_REPROGRAM_OFF);
	spin(10);

	if (isRevB) {
		/* if is a revB board, then map EEPROM to AAR1 space, and put	*/
		/*	Altera in AAR3 space - then start transfer by pulsing		*/	
		/*	AAR3 CS manually											*/
		uint32 stat;
	
		/* make sure host interface completely empty!					*/
		stat = get_host_stat(ix);
		if (!(stat & M_HSTR_TRDY)) {
			kprintf("sonorus: init_altera() host status not ready!\n");
			err = EIO;
		}
		while (!err && (count-- > 0)) {
			data = *image++;
			acquire_spinlock(&cards[ix].spinlock);
			{
				write_host_word(ix, data);
				send_host_cmd(ix, HC_WRITE_FPGA_B);
			}	
			release_spinlock(&cards[ix].spinlock);
			spin(1);
			
			if (count == 0)
				break;
				
			timeout = 0;
			while ((read_loc(ix, UNMAPPED_Y_LOCATION, yMem) & 0x80) == 0) {
				if (timeout++ > 256) {
					kprintf("sonorus: timeout loading altera byte\n");
					err = EIO;
					break;
				}
			}
			spin(1);
		}
		
		/* now, restore AAR3 to map the FPGA memory space */
		write_loc(ix, M_AAR3, xMem, 0xe00c21);		/* Altera space at y:E00000 */
	}
	else {
		/* otherwise, do the more straightforward rev A image load */
		while (count-- > 0) {
			data = *image++;
			write_loc(ix, FPGA_BASE, yMem, data);
			spin(1);
			
			if (count == 0)
				break;
				
			timeout = 0;
			while ((read_loc(ix, FPGA_BASE, yMem) & 0x80) == 0) {
				if (timeout++ > 256) {
					kprintf("sonorus: timeout loading altera byte\n");
					err = EIO;
					break;
				}
			}		
			spin(1);
		}
	}
	restore_interrupts(cp);

	return err;
}

void
set_serial_enbl(
	bool turnItOn )
{
	cpu_status cp;
	int32	bdNo, loc;
	
//	Assert( _nStudBd > 0 );
//	Assert( _studBd[ 0 ] != NULL );
//	Assert( !_isPlaying );
	
	if (turnItOn) {
		/* turn on serial I/O on each board. */
		spin(1000);		/* wait 1 msec */
		for (bdNo = 0; bdNo < card_count; bdNo++) {
			if ((get_host_stat( bdNo ) & M_HSTR_HF4) == 0) {
				cp = disable_interrupts();
				acquire_spinlock(&cards[bdNo].spinlock);
					send_host_cmd(  bdNo, HC_STARTIO );
					get_host_stat( bdNo );
				release_spinlock(&cards[bdNo].spinlock);
				restore_interrupts(cp);
				spin(1000);		/* wait 1 msec */
				if ((get_host_stat( bdNo ) & M_HSTR_HF4) == 0) {
					kprintf( "sonorus: Serial start timeout\n" );
				}	
			}
		}
	}
	else {
		/* turn off serial I/O on each board. */
		for (bdNo = 0; bdNo < card_count; bdNo++) {
			if ((get_host_stat( bdNo ) & M_HSTR_HF4) != 0) {
				cp = disable_interrupts();
				acquire_spinlock(&cards[bdNo].spinlock);
					send_host_cmd( bdNo ,HC_STOPIO );
					get_host_stat( bdNo );
				release_spinlock(&cards[bdNo].spinlock);
				restore_interrupts(cp);
				spin(1000);		/* wait 1 msec */
				if ((get_host_stat( bdNo ) & M_HSTR_HF4) != 0) {
					loc = read_loc( bdNo , 0xFFFFF4 , xMem );
					kprintf( "sonorus: Serial stop timeout\n" );
				}
			}
		}
	}
}

int32
get_device_param(
	studParmID_t parm )
{
	return read_loc( 0 , parmInfo[ parm ].dspLoc , parmInfo[ parm ].dspMem );
}

int32
get_channel_param(
	studParmID_t parm,
	int32 index )
{
	uint32		bdNo, dspLoc;
	memSpace_t	dspMem;
	
	bdNo = index / outputs_per_bd;
	dspLoc = parmInfo[ parm ].dspLoc + (index % outputs_per_bd);
	dspMem = parmInfo[ parm ].dspMem;
	
	return read_loc( bdNo , dspLoc , dspMem );
}

void
set_device_param(
	studParmID_t parm,
	int32 value )
{
	int	bdNo;
	uint32	dspLoc;
	memSpace_t	dspMem;
	
	dspLoc = parmInfo[ parm ].dspLoc;
	dspMem = parmInfo[ parm ].dspMem;
	
	for (bdNo = 0; bdNo < card_count; bdNo++)
		write_loc( bdNo , dspLoc , dspMem , value );
	
	if (parm == STUD_PARM_PACKMODE)
		packed_mode = value;
}

void
set_channel_param(
	studParmID_t parm,
	int32 index,
	int32 value )
{
	uint32		bdNo, dspLoc;
	memSpace_t	dspMem;
	
	bdNo = index / outputs_per_bd;
	dspLoc = parmInfo[ parm ].dspLoc + (index % outputs_per_bd);
	dspMem = parmInfo[ parm ].dspMem;
	
	write_loc( bdNo , dspLoc , dspMem , value );
}

void
set_play_enbl(
	bool enabled )
{
	int	i;
	
	if (enabled) {
		is_playing = true;
	}
	for (i = 0; i < card_count; i++) {
		set_host_ctrl_bits( i, M_HCTR_HF0, enabled ? M_HCTR_HF0 : 0 );
	}
	if (!enabled) {
		is_playing = false;
	}	
}

void
write_play_buf(
			uint32 chnl,
			uint32 ofst,
			void* buf,
			uint32 nSamps,
			studBufType_t bufType,
			void* args )
{
	uint32	nToWrite, nToEnd;
	char*	cBufP = (char*) buf;
	uint32	sampSize = sampleSizeFromBufType( bufType );
	
	uint16	bdNo = chnl / outputs_per_bd;
	uint16	bdCh = chnl % outputs_per_bd;

	while (nSamps > 0)
	{
		ADJUST_MODULO_US( ofst , CHNL_BUF_SIZE );
		nToEnd = CHNL_BUF_SIZE - ofst;
		nToWrite = min( nSamps , nToEnd );
		
		write_x_fast( bdNo
				  , CHNL_BUF_ORG + (CHNL_BUF_STRIDE*ofst) + bdCh
				  , CHNL_BUF_STRIDE
				  , cBufP , nToWrite
				  , bufType , args );
		
		nSamps -= nToWrite;
		ofst += nToWrite;
		cBufP += nToWrite * sampSize;
		spin( 10 );  /* (was spin 1) was TEST_DELAY( 30000) */
	}
}
	
void 
read_recd_buf(
			uint32 chnl,
			uint32 ofst,
			void* buf,
			uint32 nSamps,
			studBufType_t bufType,
			void* args )
{
	uint16	bdNo = chnl / inputs_per_bd;
	uint16	bdCh = chnl % inputs_per_bd;
	uint32	nToRead, nToEnd;
	char*	cBufP = (char*) buf;
	uint32	sampSize = sampleSizeFromBufType( bufType );

	while (nSamps > 0)
	{
		ADJUST_MODULO_US( ofst , CHNL_BUF_SIZE );
		nToEnd = CHNL_BUF_SIZE - ofst;
		nToRead = min( nSamps , nToEnd );
		
		read_x_fast( bdNo
				 , CHNL_BUF_ORG + (CHNL_BUF_STRIDE*ofst) + bdCh
				 , CHNL_BUF_STRIDE
				 , cBufP , nToRead
				 , bufType , args ); 
		
		nSamps -= nToRead;
		ofst += nToRead;
		cBufP += nToRead * sampSize;
		spin(1); /* was TEST_DELAY( 30000 ) */
	}
}

void
write_chnl_infos(
			uint32 ciNo ,
			chnlInfoX_t ciX ,
			chnlInfoY_t ciY )
{
	int i;
	uint32 ciOrg, indx;
	
	ciX.reserved = 0x7FFFFF / ciY.nSamps;	// field is really nSampsRecip
	
	for (i = 0; i < card_count; i++)  
	{
		ciOrg = CHNL_INFO_ORG + (ciNo * CHNL_INFO_SIZE);
		indx  = i * outputs_per_bd;
		write_loc( i, ciOrg , xMem , ciX.reserved );
		write_loc( i, ciOrg , yMem , ciY.nSamps );
		if (dsp_caps & kSupportsMixing)
		{
			write_buf( i, ciOrg + 1, xMem, (uint32*) &ciX.begGain[ indx ], outputs_per_bd );
			write_buf( i, ciOrg + 1, yMem, (uint32*) &ciY.endGain[ indx ], outputs_per_bd );
			write_buf( i, ciOrg + 17, xMem, (uint32*) &ciX.begPanL[ indx ], outputs_per_bd );
			write_buf( i, ciOrg + 17, yMem, (uint32*) &ciY.endPanL[ indx ], outputs_per_bd );
			write_buf( i, ciOrg + 33, xMem, (uint32*) &ciX.begPanR[ indx ], outputs_per_bd );
			write_buf( i, ciOrg + 33, yMem, (uint32*) &ciY.endPanR[ indx ], outputs_per_bd );
		}
	}
}


/************************************************************************************/
/*                                                                                  */
/*           A      CC    H  H   TTTTT   U  U   N   N    GG     !!                  */
/*          A A    C  C   H  H     T     U  U   NN  N   G  G    !!                  */
/*         AAAAA   C      HHHH     T     U  U   N N N   G       !!                  */
/*         A   A   C  C   H  H     T     U  U   N  NN   G  GG                       */
/*         A   A    CC    H  H     T      UU    N   N    GG     !!                  */
/*                                                                                  */
/************************************************************************************/
/*                                                                                  */ 
/* The following routines are specific to the fpga image that is loaded!!           */
/*                                                                                  */
/************************************************************************************/


void
set_sync_params_MonMix_1111( uint32 sRate , studSyncSrc_t syncSrc )
{
	int bdNo;
	uint32	timer, parmVal=0;

	/* stop all serial DMA activity while changing sync */
	set_serial_enbl( false );
	
	/* set the proper bits in the FPGA control register */
	switch (syncSrc)
	{
	case STUD_SYNCSRC_INT:			/* internal sync source */
		parmVal = (sRate == 48000) ? SYNC_SRATE_48K : SYNC_SRATE_44K;
		set_device_param( STUD_PARM_SYNCSELECT, 0 );
		break;
	case STUD_SYNCSRC_EXTA:			/* external sync source from input A */
		parmVal = SYNC_SRATE_PLL | SYNC_EXT_OPTA;
		set_device_param( STUD_PARM_SYNCSELECT, 0 );
		break;
	case STUD_SYNCSRC_EXTB:			/* external sync source from input B */
		parmVal = SYNC_SRATE_AES | SYNC_EXT_OPTB;
		set_device_param( STUD_PARM_SYNCSELECT, 1 );
		break;
	case STUD_SYNCSRC_EXTWC:
		parmVal = SYNC_SRATE_PLL | SYNC_EXT_EXT;
		write_loc( 0, 0xFFFF8B, xMem, 0 );		/* disable DSP timer 1 */
		break;
	case STUD_SYNCSRC_DSP:
		parmVal = SYNC_SRATE_PLL | SYNC_EXT_DSP;
		//Assert( sRate > 20000 );
		timer = ((uint32) (((65000000./4.) / ((double) sRate)) + 0.5)) - 1;
		write_loc( 0, 0xFFFF8A, xMem, 0 );		/* reload register = 0 */
		write_loc( 0, 0xFFFF89, xMem, timer ); 
		write_loc( 0, 0xFFFF8B, xMem, 0x221 );	/* enable DSP timer 1 */
		break;
	default:
		dprintf( "sonorus: Illegal sync source\n" );
	}
	for (bdNo = 0; bdNo < card_count; bdNo++)
	{
		write_loc( bdNo, FPGA_BASE+0, yMem, parmVal );
		write_loc( bdNo, FPGA_BASE+2, yMem, 0x04 );		/* consumer mode, no copyright present */
		write_loc( bdNo, FPGA_BASE+3, yMem, 0x03 );		/* category code, generation == original */
		write_loc( bdNo, FPGA_BASE+4, yMem, 0x00 );		/* byte 3 */
		write_loc( bdNo, FPGA_BASE+5, yMem, (sRate == 48000) ? 0x02 : 0x00 );	/* sample rate */
	}
	
	/* restart all serial DMA activity while changing sync */
	set_serial_enbl( true );
}

#define DAC_CTL_DAT		0x1
#define DAC_CTL_CLK		0x2
#define DAC_CTL_LATCH	0x4

void
set_dac_params_MonMix_1111(
	uint32 bdNo,
	uint8 attenDB,
	bool mute )
{
	uint8	mask, chnl, bit;
	uint32	baseVal = 0xD0;
	
	if (attenDB > 63)
		mute = true;
	
	if (mute)
		baseVal |= 0x8;
	
	for (chnl = 0; chnl < 2; chnl++)
	{
		bit = chnl ? DAC_CTL_DAT : 0;
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | bit );
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | bit | DAC_CTL_CLK );
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | 0   );
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | 0   | DAC_CTL_CLK );
		for (mask = 0x20; mask != 0; mask >>= 1)
		{
			bit = (mask & attenDB) ? DAC_CTL_DAT : 0;
			write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | bit );
			write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | bit | DAC_CTL_CLK );
		}
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal | DAC_CTL_LATCH );
		write_loc( bdNo , FPGA_BASE+1 , yMem , baseVal );
	}
}







