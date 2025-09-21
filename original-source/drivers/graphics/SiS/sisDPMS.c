#include "sisDPMS.h"
#include "sisVideoModes.h"

#include "driver.h"
#include "sis620defs.h"

uint32 Get_DPMS_mode(void) {
	uchar t;
    isa_outb(SEQ_INDEX,0x01); // Screen on or off ?
    t=isa_inb(SEQ_DATA);
    if (t&0x20) return B_DPMS_OFF;
	isa_outb(SEQ_INDEX,0x11); // DPMS register
	t=isa_inb(SEQ_DATA);
	switch((t>>6)&0x03)  {
    	case 0:  return B_DPMS_ON;
    	case 1:  return B_DPMS_STAND_BY; break; // H: off, V:  on
    	case 2:  return B_DPMS_SUSPEND;  break; // H:  on, V: off
    	case 3:  return B_DPMS_OFF;      break; // H: off, V: off
    	}
	ddprintf(("sis: DPMS_Mode : *** thou should never see this text...\n"));        
	return B_DPMS_ON;
	}


///////////////////
// Set DPMS Mode //
///////////////////

status_t Set_DPMS_mode(uint32 dpms_flags) {
	uchar t;
	vddprintf(("sis: SetDPMS_Mode 0x%08x\n",(uint)dpms_flags));
	
	switch(dpms_flags)
		{

		case B_DPMS_ON:			 // H:  on, V:  on
			isa_outb(DAC_ADR_MASK,0xff);
			setup_DAC();
			isa_outb(SEQ_INDEX,0x00); /* Restart sequencer */
			isa_outb(SEQ_DATA,0x03);

			isa_outb(SEQ_INDEX,0x01);
			t=isa_inb(SEQ_DATA);
			t&=~0x20; // screen on
			isa_outb(SEQ_DATA,t);
			isa_outb(SEQ_INDEX,0x11); // clear DPMS
			t=isa_inb(SEQ_DATA);
			t&=~0xc0;
			isa_outb(SEQ_DATA,t);
			break;

		case B_DPMS_STAND_BY: // H: off, V:  on
			isa_outb(SEQ_INDEX,0x01);
			t=isa_inb(SEQ_DATA);
			t|=0x20;					// screen off
			isa_outb(SEQ_DATA,t);
			isa_outb(SEQ_INDEX,0x11);
			t=isa_inb(SEQ_DATA);
			t|=0x40; // standby
			isa_outb(SEQ_DATA,t);
			break;

		case B_DPMS_SUSPEND:	// H:  on, V: off
			isa_outb(SEQ_INDEX,0x01);
			t=isa_inb(SEQ_DATA);
			t|=0x20;					// screen off
			isa_outb(SEQ_DATA,t);
			isa_outb(SEQ_INDEX,0x11);
			t=isa_inb(SEQ_DATA);
			t|=0x80; // standby
			isa_outb(SEQ_DATA,t);
			break;

		case B_DPMS_OFF:			// H: off, V: off
			isa_outb(SEQ_INDEX,0x01);
			t=isa_inb(SEQ_DATA);
			t|=0x20;					// screen off
			isa_outb(SEQ_DATA,t);
			isa_outb(SEQ_INDEX,0x11);
			t=isa_inb(SEQ_DATA);
			t|=0xc0; // standby & suspend
			isa_outb(SEQ_DATA,t);
			break;

		default:
			return B_ERROR;
			break;
		}
	return B_OK;
	}
