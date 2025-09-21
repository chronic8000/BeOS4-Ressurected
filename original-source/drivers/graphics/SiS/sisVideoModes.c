#include "driver.h"
#include "driver_ioctl.h"

#include "sisCRTC.h"
#include "sisVideoModes.h"
#include "sisClock.h"

extern genpool_module	*ggpm;

#define SIS_3D_IDLE_AND_3DQUEUE_EMPTY (inl(0x89fc)&2)
#define SIS_2D_IDLE_3D_IDLE_AND_QUEUE_EMPTY ((inl(0x8240)&0xe0000000)==0xe0000000)

uint32 colorspacebits (uint32 cs /* a color_space */) {
	switch (cs) {
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		return (32);
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
		return (24);
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		return (16);
	case B_RGB15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_BIG:
	case B_RGBA15_LITTLE:
		return (15);
	case B_CMAP8:
		return (8);
	}
	return (0);
}

status_t allocDesktopVideoMem(devinfo *di) {
	status_t retval ;
	sis_card_info *ci = di->di_sisCardInfo ;
	int size = ci->ci_BytesPerRow * ci->ci_CurDispMode.virtual_height ;

	if ( size == ci->ci_FBMemSpec.ms_MR.mr_Size ) {
		vddprintf(("sis: Desktop memory size is similar -> using the same memory allocation\n"));
		return(B_OK);
		}
		
	if (ci->ci_FBMemSpec.ms_MR.mr_Size !=0 ) {
		vddprintf(("sis: freeing older Desktop from video mem\n"));
//		BFreeByMemSpec (&ci->ci_FBMemSpec) ;
ggpm->gpm_FreeByMemSpec( &ci->ci_FBMemSpec);
		}

	memset(&ci->ci_FBMemSpec, 0, sizeof(ci->ci_FBMemSpec));
	ci->ci_FBMemSpec.ms_PoolID			= ci->ci_PoolID;
	ci->ci_FBMemSpec.ms_AddrCareBits	= 7 ;
	ci->ci_FBMemSpec.ms_AddrStateBits	= 0 ;
	ci->ci_FBMemSpec.ms_MR.mr_Size		= size ;
	ci->ci_FBMemSpec.ms_AllocFlags		= 0 ;

//	if ((retval = BAllocByMemSpec (&ci->ci_FBMemSpec)) < 0) {
if ((retval = ggpm->gpm_AllocByMemSpec (&ci->ci_FBMemSpec)) < 0) {
		ddprintf(("sis : error couldn't allocate %d kb of memory for the framebuffer\n", (int)(ci->ci_FBMemSpec.ms_MR.mr_Size>>10) ));
		ddprintf(("sis : BAllocByMemSpec returned code %d\n",(int)retval));
		vddprintf(("sis : PoolID was 0x%08x\n",(uint32)ci->ci_FBMemSpec.ms_PoolID ));
		ci->ci_FBBase = 0;
		ci->ci_FBBase_offset = 0;
		ci->ci_FBBase_DMA = 0 ;
		return (B_ERROR);
		}

	ci->ci_FBBase_offset = (uint32)ci->ci_FBMemSpec.ms_MR.mr_Offset ;
	ci->ci_FBBase		= (void*) ( (uint8*)ci->ci_BaseAddr0 		+ ci->ci_FBBase_offset );
	ci->ci_FBBase_DMA	= (void*) ( (uint8*)ci->ci_BaseAddr0_DMA	+ ci->ci_FBBase_offset );
	vddprintf(("sis: DesktopScreen FrameBuffer allocated successfully at 0x%08x\n",(uint32)ci->ci_FBBase_offset));
	return(B_OK);
	}


static void sis_waitengineidle (devinfo *di) {
	int t;
	sis_card_info *ci = di->di_sisCardInfo ;
	vvddprintf(("sis_accelerant: B_WAITENGINEIDLE\n"));

	t=inb(0x82ab) ;
	while ( (t&0x40) || // GE busy or Harware command queue not empty
  		  (!(t&0x80))  ) // Hardware queue empty
  		{
  		snooze(100); // spin ?
  		t=inb(0x82ab);
  		}
	vvddprintf(("sis_accelerant: Engine is now idle\n"));
	}


static void sis620_waitengineidle (devinfo *di) {

	sis_card_info *ci = di->di_sisCardInfo ;

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	lockBena4(&ci->ci_EngineRegsLock);
#endif

	while ( !SIS_3D_IDLE_AND_3DQUEUE_EMPTY ) snooze(300);
	
	while( ! SIS_2D_IDLE_3D_IDLE_AND_QUEUE_EMPTY ) snooze(500);

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	unlockBena4(&ci->ci_EngineRegsLock);
#endif

	}

////////////////////////////////////////
//                                    //
// -------- SET_DISPLAY_MODE -------- //
//                                    //
////////////////////////////////////////

status_t _set_display_mode (devinfo *di, display_mode *m) { // mode = mode to set
	ulong old_dot_clock, same_depth, same_crt_config ;
	ulong pix_clk_range ;
	struct data_ioctl_sis_CRT_settings *sis_CRT_settings ;
	uint32 t;
	status_t retval ;
	sis_card_info *ci = di->di_sisCardInfo ;
	
	vddprintf(("sis_driver: -> _set_display_mode()\n"));

	// --- Get locks on all graphics hardware and wait until graphics engine is idle
	
	lockBena4 (&ci->ci_CRTCLock);
	lockBena4 (&ci->ci_CLUTLock);
	lockBena4 (&ci->ci_EngineLock);
	lockBena4 (&ci->ci_SequencerLock);

	vvddprintf(("sis: waiting until engine is idle...\n"));

	switch(ci->ci_DeviceId) {
#if OBJ_SIS620
		case SIS620_DEVICEID:
			sis620_waitengineidle(di);
			break;
#endif
#if ( OBJ_SIS5598 | OBJ_SIS6326 )
		case SIS5598_DEVICEID:
		case SIS6326_DEVICEID:
			sis_waitengineidle(di);
			break;
#endif
		}
		
	vvddprintf(("sis: ...engine is now idle\n"));
	
	// --- change Depth
	// fixme : save old depth
	t = colorspacebits(m->space);
	if (ci->ci_Depth!=t) {
		ci->ci_Depth=t;
		same_depth = 0;
		}
	else same_depth=1;
	
	// --- change CRT configuration
	// fixme : save old crt configuration
	same_crt_config = 1 ;
	ci->ci_BytesPerPixel = (ci->ci_Depth+7)>>3 ; 
	t=m->virtual_width*ci->ci_BytesPerPixel ;
	if (ci->ci_BytesPerRow!=t) {
		ci->ci_BytesPerRow=t;
		same_crt_config = 0;
		}
	if ((ci->ci_CurDispMode.virtual_width!=m->virtual_width) ||
	    (ci->ci_CurDispMode.virtual_height!=m->virtual_height)) same_crt_config = 0 ;
	
	old_dot_clock = ci->ci_CurDispMode.timing.pixel_clock;

	// --- writes Current-Display-Mode into CardInfo structure
	memcpy(&ci->ci_CurDispMode,m,sizeof(ci->ci_CurDispMode));

	sis_CRT_settings = prepare_CRT(di, m,0,m->virtual_width); 

	// Frame Buffer Memory Allocation

	if (allocDesktopVideoMem(di) != B_OK ) {
		ddprintf(("sis: *** error *** allocDesktopVideoMem() failed, cannot set display mode\n"));
		return(B_ERROR);
		}

	if ((!same_depth)||(!same_crt_config)) {
		int i;
// ***		ioctl( driver_fd, SIS_IOCTL_SCREEN_OFF, 0, 0);
			Screen_Off();

		// clear screen
		for(i=0;i<((ci->ci_BytesPerRow * ci->ci_CurDispMode.virtual_height)>>2);i++)
			((uint32*)ci->ci_FBBase)[i]= 0x00000000;
		}
	vvddprintf(("sis: screen cleared\n"));
		
	if (((100*(ci->ci_CurDispMode.timing.pixel_clock - old_dot_clock))/old_dot_clock) > 2 ) {
		uint32 clock_value = ci->ci_CurDispMode.timing.pixel_clock ;
		// setClock(ci->ci_CurDispMode.timing.pixel_clock);
// ***		ioctl (driver_fd, SIS_IOCTL_SETCLOCK, &clock_value, sizeof (clock_value) ) ;
setClock(ci->ci_CurDispMode.timing.pixel_clock, ci->ci_DeviceId);
		}
	else vddprintf(("sis: set_display_mode/setClock not executed : clock remains almost the same"));

	if (!same_depth) {
		uint32 depth = ci->ci_Depth ;
// ***		ioctl( driver_fd, SIS_IOCTL_SET_COLOR_MODE, &depth, sizeof (depth) );
			if (ci->ci_DeviceId != SIS630_DEVICEID)
				set_color_mode(depth, di);
			else
				sis630_SetColorMode(depth, di);

		}
	else vddprintf(("sis : _set_display_mode : depth is still %d\n",ci->ci_Depth));
	
	if (!same_crt_config) {
// ***		ioctl( driver_fd, SIS_IOCTL_SET_CRT, (void*)sis_CRT_settings, sizeof (*sis_CRT_settings) );
			Set_CRT(sis_CRT_settings, di);

		}
	else vddprintf(("sis : _set_display_mode : CRT config (resolution didn't change\n"));

// ***	ioctl( driver_fd, SIS_IOCTL_INIT_CRT_THRESHOLD, & ci->ci_CurDispMode.timing.pixel_clock, sizeof (ci->ci_CurDispMode.timing.pixel_clock) );
	Init_CRT_Threshold(ci->ci_CurDispMode.timing.pixel_clock, di);

// ***	ioctl( driver_fd, SIS_IOCTL_SETUP_DAC, 0, 0);
	setup_DAC();


// ***	ioctl( driver_fd, SIS_IOCTL_RESTART_DISPLAY, 0, 0);
	Restart_Display();


	
	unlockBena4 (&ci->ci_SequencerLock);
	unlockBena4 (&ci->ci_EngineLock);
	unlockBena4 (&ci->ci_CLUTLock);
	unlockBena4 (&ci->ci_CRTCLock);
   
	return(B_OK);
	}



////////////////////////////
// standard VGA DAC setup //
////////////////////////////

void setup_DAC(void) {
	vddprintf(("sis: setup DAC()\n"));
	isa_outb(DAC_ADR_MASK,0xff);

	isa_inb(DAC_ADR_MASK);
	isa_inb(DAC_ADR_MASK);
	isa_inb(DAC_ADR_MASK);
	isa_inb(DAC_ADR_MASK);
	isa_outb(DAC_ADR_MASK,0x00);
	isa_inb(DAC_WR_INDEX);
	isa_outb(DAC_ADR_MASK,0xff);
	vvddprintf(("sis: setup_dac finished\n"));
	}


/////////////////////////////
// reads actual color mode //
/////////////////////////////

static void sis_read_color_mode() {
	uchar t;
	isa_outb(SEQ_INDEX,0x06);
	t=isa_inb(SEQ_DATA);
	if (t&0x10) vddprintf(("sis: Color Mode is True Color\n"));
	if (t&0x08) vddprintf(("sis: Color Mode is 64K Color\n"));
	if (t&0x04) vddprintf(("sis: Color Mode is 32K Color\n"));
	isa_outb(GCR_INDEX,0x05);
	t=isa_inb(GCR_DATA);
	if (t&0x40) vddprintf(("sis: Color Mode is 256 color mode (enabled)\n"));
	}


/***************************************************************
 * set_color_mode (int depth)                                  /
 *                                                             /
 * configure SEQ and GCR register to set requested color mode  /
 ***************************************************************
 */
 
void set_color_mode(uint32 depth, devinfo *di) {
	uchar t;
	vddprintf(("sis: Set_Color_Mode(%d)\n",(int)depth));
#if DEBUG > 0
	sis_read_color_mode();
#endif
	isa_outb(SEQ_INDEX,0x07);
	isa_outb(SEQ_DATA,0xa2);	// Merge Video line buffer into CRT FIFO 
								// Internel RAMDAC HighPower Mode
								// High speed DAC Operation
	// Same setting as w* :
	isa_outb(SEQ_DATA,0x20);	// Don't Merge Video line buffer into CRT FIFO 


	isa_outb(SEQ_INDEX,0x27);
	t=isa_inb(SEQ_DATA);
	t&=~0x30 ; // w* : Logical Screen Width 1024
	// t|=0x30 ; // xfree : Logical Screen Width invalid
	isa_outb(SEQ_DATA,t);	// Logical Screen Width : invalid 

	isa_outb(GCR_INDEX,0x05);
	t=isa_inb(GCR_DATA);
	t&=~0x40;
	isa_outb(GCR_DATA,t);

	if (di->di_PCI.device_id == SIS620_DEVICEID) { // 32bpp mode on 620 only
		isa_outb(SEQ_INDEX,0x09);
		t=isa_inb(SEQ_DATA);
		t&=~0x80; // clear 32 bpp mode
		isa_outb(SEQ_DATA,t);
		}
		
	isa_outb(SEQ_INDEX,0x06); // Extended Graphics Mode control register
	t=isa_inb(SEQ_DATA);
	t&=~0x3c; // no color mode, interlace disable
	t|=0x02; // enhanced graphics mode (over 4 bits / pixel :) )


	switch(depth) {
		case 8:
			isa_outb(SEQ_DATA,t);
			isa_outb(GCR_INDEX,0x05);
			t=isa_inb(GCR_DATA);
			t|=0x40;
			isa_outb(GCR_DATA,t);		
			break ;
		case 16:
			t|=0x08;
			isa_outb(SEQ_DATA,t);		
			break ;
		case 24: // doesn't seem to work - not published as supported
			t|=0x10;
			isa_outb(SEQ_DATA,t);
			break;
		case 32: // 620 only
			t|=0x10; // true color ( ? )
			isa_outb(SEQ_DATA,t);
			isa_outb(SEQ_INDEX,0x09);
			t=isa_inb(SEQ_DATA);
			t|=0x80;
			isa_outb(SEQ_DATA,t);

			isa_outb(SEQ_INDEX,0x07);
			isa_outb(SEQ_DATA,0x22);	// Don't Merge Video line buffer into CRT FIFO 

			isa_outb(SEQ_INDEX,0x35);
			t=isa_inb(SEQ_DATA);
			t&=~0x20; // enable SGRAM burst timings
			isa_outb(SEQ_DATA,t);
			break;
		}
		
#if DEBUG > 0 
	sis_read_color_mode();
#endif
	} 

void sis630_SetColorMode(uint32 depth, devinfo *di) {
	vddprintf(("sis630: SetColorMode(%d)\n",depth));
	isa_outb(SEQ_INDEX, 0x06 );
	switch(depth) {
		case 8:
			isa_outb(SEQ_DATA, (0<<2) | 0x02 );
			break;
		case 16:
			isa_outb(SEQ_DATA, (2<<2) | 0x02 );
			break;
		case 32:
			isa_outb(SEQ_DATA, (4<<2) | 0x02 );
			break;
		}
	}

/////////////////////
// MoveDisplayArea //
/////////////////////

status_t Move_Display_Area (uint16 x, uint16 y, devinfo *di) {
	register display_mode	*dm;
	uint32			addr;
	int			bytespp;
	uchar		t;
	
	sis_card_info *ci = di->di_sisCardInfo ;
	dm = &ci->ci_CurDispMode;

	if (x + dm->timing.h_display > dm->virtual_width  ||
	    y + dm->timing.v_display > dm->virtual_height) {
	    ddprintf(("sis : Move_Display_Area refused\n"));
		return (B_ERROR);
		}

	lockBena4 (&ci->ci_CRTCLock);

	bytespp = (ci->ci_Depth + 7) >> 3;
	addr = ci->ci_FBBase_offset ;
	addr += (y * dm->virtual_width + x) * bytespp;
	addr = (addr>>2) ; // 32 bits addressing mode ..

if (di->di_sisCardInfo->ci_DeviceId != SIS630_DEVICEID) {
	isa_outb(SEQ_INDEX,0x30);
	isa_outb(SEQ_DATA,addr&0xff);
	isa_outb(SEQ_INDEX,0x31);
	isa_outb(SEQ_DATA,(addr>>8)&0xff);
	isa_outb(SEQ_INDEX,0x32);
	t=isa_inb(SEQ_DATA);
	t&=~0x0f;
	isa_outb(SEQ_DATA,t|((addr>>16)&0x0f));
	}
	
	unlockBena4 (&ci->ci_CRTCLock);

	return (B_OK);
	}


////////////////////////
// Set Indexed Colors //
////////////////////////

void Set_Indexed_Colors (struct data_ioctl_set_indexed_colors *p) {
	vvddprintf(("sis: _set_indexed_colors()\n"));
	while(p->count--) {
    	isa_outb(DAC_ADR_MASK,0xff);
    	isa_outb(DAC_WR_INDEX,p->first++);
    	isa_outb(DAC_DATA,p->color_data[0]>>2);
    	isa_outb(DAC_DATA,p->color_data[1]>>2);
    	isa_outb(DAC_DATA,p->color_data[2]>>2);
		p->color_data+=3;
		}	  
	}


////////////////
// Screen Off //
////////////////

void Screen_Off(void) {
	isa_outb(SEQ_INDEX,0x01);
	isa_outb(SEQ_DATA,0x21);  // screen off
	}

	
/////////////////////
// Restart Display //
/////////////////////

void Restart_Display(void) {
    vddprintf(("sis: restarting sequencer\n"));
	isa_outb(SEQ_INDEX,0x00); /* Restart sequencer */
	isa_outb(SEQ_DATA,0x00);
	isa_outb(SEQ_INDEX,0x00); /* Restart sequencer */
	isa_outb(SEQ_DATA,0x03);
	isa_outb(SEQ_INDEX,0x00); /* Restart sequencer */
	isa_outb(SEQ_DATA,0x00);
	isa_outb(SEQ_INDEX,0x00); /* Restart sequencer */
	isa_outb(SEQ_DATA,0x03);

	snooze(10000);
	isa_outb(SEQ_INDEX,0x01);
	isa_outb(SEQ_DATA,0x01);  /* SCREEN ON */      
	}

