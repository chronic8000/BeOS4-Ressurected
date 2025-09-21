#include "driver.h"
#include "driver_ioctl.h"
#include "sisCursor.h"

static uint16 _hotX,_hotY;

status_t _set_cursor_shape(struct data_ioctl_set_cursor_shape *sis_data, devinfo *di) {
	uchar t;
	uint32 i,j,k,cursor_address;
	sis_card_info *ci = di->di_sisCardInfo ;

	cursor_address = 0 ; // could have put the cursor shape at the end of video memory for example
	// like this: cursor_address = (ci->ci_MemSize + 256)*1024 -1;
	// but I don't have the doc for chipset 630 to configure this address
	// I you want to change it, to spare some mem (960*256 bytes) just set the MEMORY_MARGIN to 0 and rebuild ALL
	 
	cursor_address=(cursor_address>>18); // align 256k
	
	// vddprintf(("cursor_address=0x%08x\n",cursor_address<<18));
	isa_outb(SEQ_INDEX,0x38); // bits [21:18]
	t=isa_inb(SEQ_DATA);
	t&=~0xf0;
	t|=((cursor_address&0x0f)<<4);
	isa_outb(SEQ_DATA,t);
	if (di->di_PCI.device_id!=SIS5598_DEVICEID) { // = if 6326 or 620
		isa_outb(SEQ_INDEX,0x3e); // bit [22]
		t=isa_inb(SEQ_DATA);
		t&=~0x04;
		t|=((cursor_address&0x10)>>2);
		isa_outb(SEQ_DATA,t);
		}
		
	cursor_address=cursor_address<<18;
	
	cursor_address+=(uint32)ci->ci_BaseAddr0; // mapped memory address

	// in SiS5598, cursor bitmap located at the last 128*128 block
	// its size is 64x64 so it uses 16*64 bytes of memory
	switch(di->di_PCI.device_id) {
		case SIS5598_DEVICEID:
			//cursor_address+=0x40000-128*128;
			//break;
		case SIS6326_DEVICEID:
		case SIS620_DEVICEID:
			cursor_address+=960*256; // [960*256: 964*256] because its size is 16x64 = 4*256
			break;
		default:
			ddprintf(("sis: cursor configuration for this chip not found\n"));
		}

	// 2 bits per pixel so its size is 64*64
   
	// the transformation for each byte is like this:
	// INPUT : a7 a6 a5 a4 a3 a2 a1 a0 : andMask
	//         x7 x6 x5 x4 x3 x2 x1 x0 : xorMask
	// OUTPUT : a7 x7 a7 x7 a6 x6 a6 x6
	//          a5 x5 a5 x5 a4 x4 a4 x4
	// ... and the same starting at a3 and a1

	// Set invisible by default 
	for(i=0;i<16*64;i++) *((uchar*)cursor_address++)=0xaa;
	cursor_address-=16*64;
  
	// Draw the required cursor
	for(j=0;j<32;j+=2) { // j/2 is the vertical position
		for(i=0;i<2;i++) { // 16 pixels -> 2 bytes
		uint16 xorData,andData,bmap1;
		xorData=(sis_data->xorMask[j+i]<<1);
		andData=sis_data->andMask[j+i];

		for(k=0;k<8;k+=4) { // each loop reads and writes 4 pixels
			switch((xorData&0x100)|(andData&0x80)) {
				case 0:
					bmap1=0x01;            
					break;
				case 0x80:
					bmap1=0x02;            
					break;
				case 0x100:
					bmap1=0x00;            
					break;
				case 0x180:
					bmap1=0x03;        
				default:
					bmap1=0; // only used to disable compiler warning "bmap1 may be uninitialised"    
				}
			xorData<<=1;
			andData<<=1;
			bmap1<<=2;
			switch((xorData&0x100)|(andData&0x80)) {
				case 0:
					bmap1|=0x01;            
					break;
				case 0x80:
					bmap1|=0x02;            
					break;
				case 0x100:
					bmap1|=0x00;            
					break;
				case 0x180:
					bmap1|=0x03;            
				}  // switch
			xorData<<=1;
			andData<<=1;
			bmap1<<=2;
			switch((xorData&0x100)|(andData&0x80)) {
				case 0:
					bmap1|=0x01;            
					break;
				case 0x80:
					bmap1|=0x02;            
					break;
				case 0x100:
					bmap1|=0x00;            
					break;
				case 0x180:
					bmap1|=0x03;            
				}  // switch
			xorData<<=1;
			andData<<=1;
			bmap1<<=2;
			switch((xorData&0x100)|(andData&0x80)) {
				case 0:
					bmap1|=0x01;            
					break;
				case 0x80:
					bmap1|=0x02;            
					break;
				case 0x100:
					bmap1|=0x00;            
					break;
				case 0x180:
					bmap1|=0x03;            
				}  // switch
			xorData<<=1;
			andData<<=1;
			*((uchar*)cursor_address++)= bmap1;
			}  // for k
		}  // for i
	cursor_address+=12;
	} // for j
  
	isa_outb(SEQ_INDEX,0x14);  // 14-15-16 : Color 0 RGB
	isa_outb(SEQ_DATA,0x00);
	isa_outb(SEQ_INDEX,0x15);
	isa_outb(SEQ_DATA,0x00);
	isa_outb(SEQ_INDEX,0x16);
	isa_outb(SEQ_DATA,0x00);

	isa_outb(SEQ_INDEX,0x17);  // 17-18-19 : Color 1 RGB
	isa_outb(SEQ_DATA,0xff);
	isa_outb(SEQ_INDEX,0x18);
	isa_outb(SEQ_DATA,0xff);
	isa_outb(SEQ_INDEX,0x19);
	isa_outb(SEQ_DATA,0xff);

	isa_outb(SEQ_INDEX,0x1c);
	isa_outb(SEQ_DATA,0x00); // preset x
	isa_outb(SEQ_INDEX,0x1f);
	isa_outb(SEQ_DATA,0x00); // preset y
	isa_outb(SEQ_DATA,0x00);
	_hotX=sis_data->hotX;
	_hotY=sis_data->hotY;
  
	return(B_NO_ERROR);
	}
  
void _move_cursor(int16 screenX,int16 screenY, devinfo *di) {
	int x=screenX-_hotX;
	int y=screenY-_hotY;
  	sis_card_info *ci = di->di_sisCardInfo ;

	if (x<0) x=0;
	if (y<0) y=0;
  
	lockBena4 (&ci->ci_SequencerLock);
	isa_outb(SEQ_INDEX,0x1a);        // x coordinate
	isa_outb(SEQ_DATA,(x&0xff));
	isa_outb(SEQ_INDEX,0x1b);
	isa_outb(SEQ_DATA,(x>>8)&0xff);
	isa_outb(SEQ_INDEX,0x1d);        // y coordinate
	isa_outb(SEQ_DATA,(y&0xff));
	isa_outb(SEQ_INDEX,0x1e);
	isa_outb(SEQ_DATA,(y>>8)&0xff);
	unlockBena4 (&ci->ci_SequencerLock);
	}
  
void _show_cursor(uint32 on) {
  uchar t;
  isa_outb(SEQ_INDEX,0x06);
  t=isa_inb(SEQ_DATA);
  if (on) t=t|0x40; /* Hardware Cursor Enable */
  else t=t&(0xbf);    /* disable */
  isa_outb(SEQ_DATA,t);
  }
  
