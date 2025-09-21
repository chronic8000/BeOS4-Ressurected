#include "sisCRTC.h"
#include "sis620defs.h"


data_ioctl_sis_CRT_settings sis_CRT_settings ;


///////////////////////////
// Computes CRT settings //
///////////////////////////

struct data_ioctl_sis_CRT_settings *prepare_CRT(devinfo *di, display_mode *dm, uint32 screen_start_address, uint32 pixels_per_row) {

	sis_card_info *ci = di->di_sisCardInfo ;

	uchar DoubleScan,t;
	ulong original_dot_clock;

	ulong h_front_p, h_sync, h_back_p;
	ulong h_vis, v_vis, v_front_p, v_sync, v_back_p;
	ulong v_blank;
	uchar v_blank_e, hvidmid;
	ulong hTotal, hDispEnableEnd, hBlankStart, calcHorizBlankEnd, hSyncStart, hRetraceEnd;
	ulong vTotal, vSyncStart, vDisplayEnable, vRetraceEnd, offset, vBlankStart, linecompare;
	int is_interlaced;
	int vclkhtotal, vclkhvis, vclkhsync, vclkhbackp;
	int vclkhblank, vclkhfrontp;
	int temp;

	// ------------------------

	h_front_p	= (ulong)dm->timing.h_sync_start - dm->timing.h_display;
	h_sync		= (ulong)dm->timing.h_sync_end - dm->timing.h_sync_start;
	h_back_p	= (ulong)dm->timing.h_total - dm->timing.h_sync_end;
	h_vis		= (ulong)dm->timing.h_display;

	v_front_p	= (ulong)dm->timing.v_sync_start - dm->timing.v_display;
	v_sync		= (ulong)dm->timing.v_sync_end - dm->timing.v_sync_start;
	v_back_p	= (ulong)dm->timing.v_total - dm->timing.v_sync_end;
	v_vis		= (ulong)dm->timing.v_display;

	is_interlaced	= (ulong)dm->timing.flags & B_TIMING_INTERLACED;
	vddprintf(("Interlaced?:  %s\n", (is_interlaced ? "YES" : "NO")));

	/*---------------- calculation of reg htotal --------------------*/
	temp = h_vis + h_front_p + h_sync + h_back_p;
	vclkhtotal = temp >> 3;
	hTotal = vclkhtotal - 5;
	vddprintf(("hTotal: %d 0x%02x\n", hTotal, hTotal));

	/*---- calculation of the horizontal display enable end reg ----*/
	vclkhvis = h_vis >> 3;
	hDispEnableEnd = (uchar)vclkhvis-1;

	/*----- calculation of the horizontal blanking start register ----*/
	hBlankStart = vclkhvis - 1;
	vddprintf(("hBlankStart: %d 0x%02x\n", hBlankStart, hBlankStart));

	/*----- calculation of horizontal blanking end register -------*/
	vclkhfrontp	= h_front_p >> 3;
	vclkhsync	= h_sync >> 3;
	vclkhbackp	= h_back_p >> 3;
	vclkhblank	= vclkhfrontp + vclkhbackp + vclkhsync;
	calcHorizBlankEnd = (vclkhvis + vclkhblank) - 1;
	vddprintf(("calcHorizBlankEnd: %d 0x%02x\n", calcHorizBlankEnd, calcHorizBlankEnd));

	/*----- calculation of the horizontal retrace start register -----*/
	hSyncStart = (vclkhvis + vclkhfrontp) - 1;
	vddprintf(("hSyncStart: %d 0x%02x\n", hSyncStart, hSyncStart));

	/*----- calculation of the horizontal retrace end register -------*/
	hRetraceEnd =  ((vclkhvis + vclkhfrontp + vclkhsync) - 1);
	vddprintf(("retrace end: %d 0x%02x\n", hRetraceEnd, hRetraceEnd));

	/*----- calculation of the vertical total register -------------*/
	vTotal = (v_vis + v_front_p + v_sync + v_back_p) - 2;

	/*------ calculation of the vertical retrace start register -----*/
	vSyncStart = (v_vis + v_front_p) - 1;

	/*------ calculation of the vertical retrace end register ------*/
	vRetraceEnd = (v_vis + v_front_p + v_sync - 1) & 0x0f;

	/*----- calculation of the vertical display enable register ---*/
	vDisplayEnable = v_vis - 1;

	/*----- calculation of the offset register ------------------*/
	vddprintf(("dm->virtual_width: %d\n", dm->virtual_width));
	vddprintf(("pixels_per_row: %d\n", pixels_per_row));
	/* offset = (uint32)dm->virtual_width / (64 / bits_per_pixel); */
	if (dm->space==B_RGB32_LITTLE)		offset = pixels_per_row / 2 ;
	else if (dm->space==B_RGB16_LITTLE)	offset = pixels_per_row / 4 ;
	else if (dm->space==B_CMAP8)		offset = pixels_per_row / 8 ;
	vddprintf(("sis: CRT offset = %d\n", offset));

	/*-----------calculation of the vertical blanking start register ----------*/
	/*FFB OK */
	vBlankStart = v_vis - 1;

	/*-----------calculation of the vertical blanking end register ------------*/
	/*FFB OK */
	v_blank = v_back_p + v_sync + v_front_p;
	v_blank_e= (uchar)(v_vis + v_blank) - 1;

	/*---- line compare register ---------*/

	linecompare = 0x3ff; // 0x0ff to 0x3ff
	
	// = 0xff; /* linecomp (the 0x0ff from 0x3ff) */
	//linecompare = (uchar)(vBlankStart & 0x3ff);
	
	if (vTotal < 300) {
		DoubleScan=0x80;
		vTotal *= 2;
		}
	else DoubleScan=0x00;

	vddprintf(("sis: Calculated refresh rate %d\n",(int) (1000*dm->timing.pixel_clock)/(hTotal*vTotal*8) )); 
	
	/* Encode CRT parameters */
	
	vddprintf(("sis:Htotal: %d, Hdisp: %d\n",(int)hTotal,(int)hDispEnableEnd));
	vddprintf(("sis:Hstart-blank: %d, Hend-blank %d\n",(int)hBlankStart,(int)calcHorizBlankEnd));
	vddprintf(("sis:Hstart-sync: %d, Hend-sync %d\n",(int)hSyncStart,(int)hRetraceEnd));
	
	if (ci->ci_DeviceId == SIS630_DEVICEID) {
		vddprintf(("sis630: - warning - hSyncStart += 5\n"));
		hSyncStart += 5; // experimental result...
		}

	vddprintf(("sis: vTotal: %d, vSyncStart: %d, vDisplayEnable: %d\n",(int)vTotal, (int)vSyncStart, (int)vDisplayEnable));
	vddprintf(("sis: vRetraceEnd: %d, offset: %d, vBlankStart: %d\n",(int)vRetraceEnd, (int)offset, (int)vBlankStart));
	vddprintf(("sis: linecompare: %d\n",linecompare));
	
	calcHorizBlankEnd=(calcHorizBlankEnd & 0x00ff);
	hRetraceEnd=(hRetraceEnd & 0x003f);
	
	vRetraceEnd=(vRetraceEnd&15);
	v_blank_e=(v_blank_e&255);
	
	///////////////////////////
	// Assign CRT parameters //
	///////////////////////////
	
	sis_CRT_settings.CRT_data[0x00]=hTotal&255;
	sis_CRT_settings.CRT_data[0x01]=hDispEnableEnd&255;
	sis_CRT_settings.CRT_data[0x02]=hBlankStart&255;
	sis_CRT_settings.CRT_data[0x03]=(calcHorizBlankEnd&31)|128;  /* 128 : ? reserved */
	sis_CRT_settings.CRT_data[0x04]=hSyncStart&255;
	sis_CRT_settings.CRT_data[0x05]=(hRetraceEnd&31)|((calcHorizBlankEnd&32)<<2);
	sis_CRT_settings.CRT_data[0x06]=vTotal&255;
	sis_CRT_settings.CRT_data[0x07]=((vTotal&256)>>8) |
									((vDisplayEnable&256)>>7) |
									((vSyncStart&256)>>6) |
									((vBlankStart&256)>>5) |
									((linecompare&256)>>4) |
									((vTotal&512)>>4) |
									((vDisplayEnable&512)>>3) |
									((vSyncStart&512)>>2);
	sis_CRT_settings.CRT_data[0x08]=0x00; 
	sis_CRT_settings.CRT_data[0x09]=DoubleScan | ((linecompare&512)>>3) | ((vBlankStart&512)>>4);
	sis_CRT_settings.CRT_data[0x0A]=0x20; // text-cursor off
	sis_CRT_settings.CRT_data[0x0B]=0x00;
	sis_CRT_settings.CRT_data[0x0C]=(screen_start_address & 0xff00) >> 8; /* Screen start address bit */
	sis_CRT_settings.CRT_data[0x0D]=(screen_start_address & 0x00ff);
	//sis_CRT_settings.CRT_data[0x0E]=; 	//cursor position
	//sis_CRT_settings.CRT_data[0x0F]=;
	sis_CRT_settings.CRT_data[0x10]=vSyncStart&255;
	sis_CRT_settings.CRT_data[0x11]=0x80|(vRetraceEnd&15); /* Lock CRT0-7,enable and clear vertical interrupt */
	sis_CRT_settings.CRT_data[0x12]=vDisplayEnable&255;
	sis_CRT_settings.CRT_data[0x13]=(offset & 0x00ff);
	sis_CRT_settings.CRT_data[0x14]=0x40; // DoubleWord mode enable (0x40) and count-by-4 (0x20)
	sis_CRT_settings.CRT_data[0x15]=vBlankStart&255;
	sis_CRT_settings.CRT_data[0x16]=v_blank_e&255;
	sis_CRT_settings.CRT_data[0x17]=0xe3; /* 0xe3 : byte refresh // a3 ?*/
	sis_CRT_settings.CRT_data[0x18]=linecompare&255;

//	if (ci->ci_DeviceId != SIS630_DEVICEID) {
		sis_CRT_settings.extended_CRT_overflow=
							 ((offset>>4)&0xf0)
							|((vSyncStart&1024)>>7)
							|((vBlankStart&1024)>>8)
							|((vDisplayEnable&1024)>>9)
							|((vTotal&1024)>>10);
		vddprintf(("sis: sisCRT_overflow 0x%02x\n",sis_CRT_settings.extended_CRT_overflow));
						
		sis_CRT_settings.extended_horizontal_overflow=
							 ((calcHorizBlankEnd&64)>>2)
							|((hSyncStart&256)>>5)
							|((hBlankStart&256)>>6)
							|((hDispEnableEnd&256)>>7)
							|((hTotal&256)>>8);
		vddprintf(("sis: sisCRT_hor_overflow 0x%02x\n",sis_CRT_settings.extended_horizontal_overflow));
//		}
//	else {
		sis_CRT_settings.sis630_ext_vertical_overflow=
							 ((vRetraceEnd		& (1<<4 )) << 1)
							|((v_blank_e		& (1<<8 )) >> 4)
							|((vSyncStart		& (1<<10)) >> 7)
							|((vBlankStart		& (1<<10)) >> 8)
							|((vDisplayEnable	& (1<<10)) >> 9)
							|((vTotal			& (1<<10)) >>10) ;

		sis_CRT_settings.sis630_ext_horizontal_overflow1=
							 ((hSyncStart		& (3<<8 )) >> 2)
							|((hBlankStart		& (3<<8 )) >> 4)
							|((hDispEnableEnd	& (3<<8 )) >> 6)
							|((hTotal			& (3<<8 )) >> 8) ;

		sis_CRT_settings.sis630_ext_horizontal_overflow2=
							 ((hRetraceEnd		& (1<<5 )) >> 3)
							|((calcHorizBlankEnd& (3<<6 )) >> 6) ;
							
		sis_CRT_settings.sis630_ext_pitch =
							 ((offset			& 0x0f00 ) >> 8) ;
		sis_CRT_settings.sis630_ext_starting_address =
							 ((screen_start_address >> 16) & 0xff);
		sis_CRT_settings.sis630_display_line_width = 
							640/4 ;							
//		}

	vddprintf(("sis:prepare CRT finished\n"));
	return(&sis_CRT_settings);
	}


/////////////////////////////////////////////////
// Set CRT mode as defined in sis_CRT_settings //
/////////////////////////////////////////////////

void Set_CRT(data_ioctl_sis_CRT_settings *sis_CRT_settings, devinfo *di) {
	uchar i,t;
	uint32 addr;
	sis_card_info *ci = di->di_sisCardInfo ;
	
	//  isa_outb(CRTC_INDEX,0x80);
	//  isa_outb(CRTC_DATA,0x86); // turn on video...

	isa_outb(CRTC_INDEX,0x11);
	isa_outb(CRTC_DATA,0x00); // disable CRT0-7 protection
	isa_outb(CRTC_INDEX,0x11);
	if ( isa_inb(CRTC_DATA) & 0x80 )
		ddprintf(("sis: - error - Disable CRT0-7 protection failed (CRT11=0x%02x)\n",isa_inb(CRTC_DATA)));
	else vddprintf(("sis: CRT0-7 protection disabled\n"));
	//vddprintf(("sis:ISA-Enable CRT(0x11): 0x%02x\n",isa_inb(CRTC_DATA)));

	vddprintf(("sis: Setting CRTC Register from 0x00 to 0x18 :\n"));
	for(i=0x00;i<=0x18;i++) {
		isa_outb(CRTC_INDEX,(uchar)i);
		isa_outb(CRTC_DATA,(uchar)sis_CRT_settings->CRT_data[(int)i]);
		vddprintf(("sis: CRTC Index (0x%02x) ",isa_inb(CRTC_INDEX)));
		vddprintf(("is now 0x%02x (wanted 0x%02x)\n",isa_inb(CRTC_DATA),sis_CRT_settings->CRT_data[i]));
		}

	isa_outb(SEQ_INDEX,0x05);
	isa_outb(SEQ_DATA,0x86);


	if (di->di_sisCardInfo->ci_DeviceId != SIS630_DEVICEID) {
		isa_outb(SEQ_INDEX,0x0A);
		isa_outb(SEQ_DATA,sis_CRT_settings->extended_CRT_overflow);
		vvddprintf(("sis:Extended CRT overflow register 0x%02x\n",sis_CRT_settings->extended_CRT_overflow));
	
		isa_outb(SEQ_INDEX,0x12);
		t=isa_inb(SEQ_DATA)&0xe0;
		t&=~0xe0; // Horizontal retrace skew : no delay
		t|=sis_CRT_settings->extended_horizontal_overflow;
		isa_outb(SEQ_DATA,t);
		vvddprintf(("sis:Extended horizontal overflow register 0x%02x\n",t));

		// move display area(0,0)
		addr = di->di_sisCardInfo->ci_FBBase_offset >>2;
		isa_outb(SEQ_INDEX,0x30);
		isa_outb(SEQ_DATA,addr&0xff);
		isa_outb(SEQ_INDEX,0x31);
		isa_outb(SEQ_DATA,(addr>>8)&0xff);
		isa_outb(SEQ_INDEX,0x32);
		t=isa_inb(SEQ_DATA);
		t&=~0x0f;
		isa_outb(SEQ_DATA,t|((addr>>16)&0x0f));
		}
	else {	// -------- SiS 630 --------
	
		// Screen Starting Address
		uint32 addr = di->di_sisCardInfo->ci_FBBase_offset >> 2 ; // unit of DW	
		isa_outb(SEQ_INDEX, 0x09);
		isa_outb(SEQ_DATA, 0x2f); // MMIO8540 Starting Address + ThresholdHigh 0xf
		outl(0x8540, addr );
		
		// threshold
		isa_outb(SEQ_INDEX, 0x08 );
		isa_outb(SEQ_DATA, 0x6f );
		
		
		isa_outb(SEQ_INDEX, 0x0a );
		isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_ext_vertical_overflow ) ;
		isa_outb(SEQ_INDEX, 0x0b );
		isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_ext_horizontal_overflow1 ) ;
		isa_outb(SEQ_INDEX, 0x0c );
		isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_ext_horizontal_overflow2 ) ;
		//isa_outb(SEQ_INDEX, 0x0d );
		//isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_ext_starting_address ) ;
		isa_outb(SEQ_INDEX, 0x0e );
		isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_ext_pitch ) ;

		isa_outb(SEQ_INDEX, 0x10 );
		isa_outb(SEQ_DATA,	sis_CRT_settings->sis630_display_line_width ) ;
			

		}
		
	ddprintf(("sis: CRT configuration done\n"));
	}



////////////////////////////////////////////////
// CRT/CPU threshold and CRT/Engine threshold //
////////////////////////////////////////////////

void Init_CRT_Threshold(uint32 dot_clock, devinfo *di) {
	int threshold_low=10,threshold_high=15;
	uchar t;

if (di->di_PCI.device_id == SIS630_DEVICEID ) {
	vddprintf(("sis630: Initializing threshold values\n"));
	
	return ;
	}
	
	// Compute CRT/CPU threshold low and high depending on dot_clock
	dot_clock = dot_clock * di->di_sisCardInfo->ci_Depth / 8 ; // represents memory bandwith
	
	switch(di->di_PCI.device_id) {
		case SIS5598_DEVICEID:
			threshold_low = (int) 0x9 + ((0xf-0x9)*dot_clock)/110000 ;
			if (threshold_low > 0xf) threshold_low = 0xf;
			threshold_high = (int) 0xc + ((0xf-0xc)*dot_clock)/110000 ;
			if (threshold_high > 0xf) threshold_high = 0xf;
			break;
		case SIS6326_DEVICEID:
		case SIS620_DEVICEID:
		case SIS630_DEVICEID:
			threshold_low = (int) 0xf + ((0x1f-0xf)*dot_clock)/100000 ;
			if (threshold_low > 0x1f) threshold_low = 0x1f;
			threshold_high = (int) 0x18 + ((0x1f-0x18)*dot_clock)/100000 ;
			if (threshold_high > 0x1f) threshold_high = 0x1f;
			break;
		default:
			ddprintf(("sis : no threshold settings for this device\n"));
			break;
		}

	vvddprintf(("sis: threshold low = %d\n",threshold_low));
	vvddprintf(("sis: threshold high = %d\n",threshold_high));

	// writes threshold settings
	
	isa_outb(SEQ_INDEX,0x08);
	isa_outb(SEQ_DATA, 0x0f | (threshold_low<<4) ); // CRT/ENG threshold = 0x0f
	isa_outb(SEQ_INDEX,0x09);
	t = 0x80 & isa_inb(SEQ_DATA); // for SIS6326 save 32bpp color mode if enabled 
	isa_outb(SEQ_DATA, t | (0x0f & threshold_high) ); // threshold high=f

	if (di->di_PCI.device_id != SIS5598_DEVICEID) { // 6326, 620
		isa_outb(SEQ_INDEX,0x3f);
		t=0xc3 & isa_inb(SEQ_DATA);
		t|=0x20; // using 64-stage threshold full
		t|=0x08; // CRT/Engine high bit[4]
		t|= threshold_high & 0x40 ; // threshold high bit[4]
		t|= (threshold_low & 0x40) >>2; // threshold low bit[4]
		t&=~0x80 ; // w* : dont set high-speed DAC bit
		isa_outb(SEQ_DATA,t);
		}

	}
