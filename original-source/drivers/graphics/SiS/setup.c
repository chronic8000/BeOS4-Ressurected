#include "setup.h"
#include <surface/genpool.h>
#include <drivers/genpool_module.h>

extern genpool_module	*ggpm;
extern BPoolInfo		*gfbpi;


/* standard settings vor VGA attribute registers */
static ushort attribute_table[]= {
  0x00,0x00,
  0x01,0x01,
  0x02,0x02,
  0x03,0x03,
  0x04,0x04,
  0x05,0x05,
  0x06,0x14,
  0x07,0x07,
  0x08,0x38,
  0x09,0x39,
  0x0a,0x3a,
  0x0b,0x3b,
  0x0c,0x3c,
  0x0d,0x3d,
  0x0e,0x3e,
  0x0f,0x3f,
  0x10,0x01, /* | 0X40 : Double Clock select */
  0x11,0x00,
  0x12,0x0f,
  0x13,0x00,
  0x14,0x00,
  0xff,0xff /* end of table */
  };

/* Standard settings for VGA GCR registers */
static ushort gcr_table[]= {
  GCR_INDEX, 0x00, GCR_DATA, 0x00,
  GCR_INDEX, 0x01, GCR_DATA, 0x00, /* 0x0f to enable set/reset ? */
  GCR_INDEX, 0x02, GCR_DATA, 0x00,
  GCR_INDEX, 0x03, GCR_DATA, 0x00,
  GCR_INDEX, 0x04, GCR_DATA, 0x00, /* plane 0 */
  GCR_INDEX, 0x05, GCR_DATA, 0x40, /* 0x40/0x00: enable/disable 256 colors mode */
  GCR_INDEX, 0x06, GCR_DATA, 0x05, /* 0x05 : graphics mode - 0xA0000 */
  GCR_INDEX, 0x07, GCR_DATA, 0x0f, 
  GCR_INDEX, 0x08, GCR_DATA, 0xff,
  0x00, 0x00
  };
    
/* Standard settings for VGA Sequencer registers */
static ushort sequencer_table[] = {
  SEQ_INDEX, 0x00, SEQ_DATA, 0x00,
  SEQ_INDEX, 0x00, SEQ_DATA, 0x03,
  SEQ_INDEX, 0x00, SEQ_DATA, 0x00,
  SEQ_INDEX, 0x00, SEQ_DATA, 0x03,
  SEQ_INDEX, 0x01, SEQ_DATA, 0x21,  // 0x20: screen off
  SEQ_INDEX, 0x02, SEQ_DATA, 0x0f,
  SEQ_INDEX, 0x03, SEQ_DATA, 0x00,
  SEQ_INDEX, 0x04, SEQ_DATA, 0x0e,
  SEQ_INDEX, 0x05, SEQ_DATA, 0x86,  // unlock extended registers
  0x00, 0x00
  };

void set_attr_table(ushort *ptr) {
  ushort p1,p2;
  uchar v,t;
  vddprintf(("sis: set_attr_table begins\n"));
  t=isa_inb(INPUT_STATUS_1);
  v=isa_inb(ATTR_REG);
  while (1) {
    p1=*ptr++;
    p2=*ptr++;
    if ((p1==0xff)&&(p2==0xff)) {
      t=isa_inb(INPUT_STATUS_1);
      isa_outb(ATTR_REG,v|0x20);
      vddprintf(("sis: set_attr_table() ends\n"));
      return;
      }
    t=isa_inb(INPUT_STATUS_1);
    isa_outb(ATTR_REG,p1);
    isa_outb(ATTR_REG,p2);
    }
  }
  
void set_table(ushort *ptr) {
  ushort p1,p2;
  while (1) {
    ushort old_p1,old_p2;
    p1=*ptr++;
    p2=*ptr++;
    if ((p1|p2)==0) return;
    isa_outb((uint32)p1,p2); // set index
    vddprintf(("sis: reg 0x%04x (index 0x%02x) was ",p1,p2));
    old_p1=p1;
    old_p2=p2;
    p1=*ptr++;
    p2=*ptr++;
    vddprintf(("sis: 0x%02x, now 0x%02x ",isa_inb((uint32)p1),p2));
    isa_outb((uint32)p1,p2); // set data
    isa_outb((uint32)old_p1,old_p2);
    vddprintf(("sis: (isa_inb=0x%02x)\n",isa_inb((uint32)p1)));    
    }
  }


///////////////
// MAP_DEVICE
///////////////

status_t map_device (register struct devinfo *di) {
	sis_card_info	*ci = di->di_sisCardInfo;
	status_t		retval=B_ERROR;
	int				len;
	char			name[B_OS_NAME_LENGTH];
	uchar			t;
	uint32			x,y,linear_address,graphics_frame_buffer;
	uint8			seq_2f ;

	//  Create a goofy basename for the areas
	sprintf (name, "%04X_%04X_%02X%02X%02X",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (name);

	// Unlock SEQ extended registers
	isa_outb(SEQ_INDEX,0x05);
	isa_outb(SEQ_DATA,0x86);
	t=isa_inb(SEQ_DATA);
	if (t!=0xa1) {
		ddprintf(("sis: Could not unlock SEQ extended registers - return code 0x%02x\n",t));
		}
	else vddprintf(("sis: Unlock SEQ extended registers succeed\n"));      

	switch(ci->ci_DeviceId) {
		case SIS6326_DEVICEID:
		case SIS620_DEVICEID:
			isa_outb(SEQ_INDEX,0x0d);
			t=isa_inb(SEQ_DATA);
			t&=~0x10; // disable internal AGP bus
			isa_outb(SEQ_DATA,t);
			break;			
		}
		
	vddprintf(("sis: pci_register[0]: 0x%08x 0x%08x 0x%08x\n",
		(uint)di->di_PCI.u.h0.base_registers[0],
		(uint)di->di_PCI.u.h0.base_registers_pci[0],
		(uint)di->di_PCI.u.h0.base_register_sizes[0] ));

	vddprintf(("sis:ISA ans MemMapped IO ? (02000003) 0x%08x\n",(uint)get_pci(&di->di_PCI,0x04,4)));
	vddprintf(("sis:Read config via GET_PCI:\n"));
	vddprintf(("sis:4Mb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x10,4)));
	vddprintf(("sis:64kb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x14,4)));
	vddprintf(("sis:64kb IO base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x18,4)));     
	
		
	// Support for SIS5598 and SIS6326
	// should work for SIS5597 considered as SIS5598
	// and for SIS6205, 6215, 6225 considered as SIS6326
	
	// Read Memory Configuration
	isa_outb(SEQ_INDEX,0x0c); // Memory Configuration bits
	t=isa_inb(SEQ_DATA);
	if 	((ci->ci_DeviceId==SIS6326_DEVICEID)
		||(ci->ci_DeviceId==SIS620_DEVICEID)
		||(ci->ci_DeviceId==SIS630_DEVICEID)) {
		t= ((t&0x10)>>2) | ((t&0x06)>>1);
		// Shared Frame Buffer or Local Frame Buffer ?
		isa_outb(SEQ_INDEX,0x0d); // ext conf status 0
		if (isa_inb(SEQ_DATA)&1) ci->ci_Shared_Mode=TRUE; else ci->ci_Shared_Mode=FALSE;
		}
	else {
		ci->ci_Shared_Mode = TRUE;
		t = ((t&0x06)>>1) ;			
		}
		
	switch(t) {
		case 0:
			ci->ci_MemSize = 1024;
			break;
		case 1:
		case 5:
			ci->ci_MemSize = 2*1024;
			break;			
		case 2:
		case 6:
			ci->ci_MemSize = 4*1024;
			break;
		case 7:
			ci->ci_MemSize = 8*1024;
			break;
		default:
			ci->ci_MemSize = 0 ;
		}

	// Shared Frame Buffer Size
	if (ci->ci_Shared_Mode) switch(ci->ci_DeviceId) {
		case SIS5598_DEVICEID: {
			int memblocks,memconf ;
			isa_outb(SEQ_INDEX,0x0C);
			memconf=(isa_inb(SEQ_DATA)&6)>>1;
			isa_outb(SEQ_INDEX,0x2f);
			seq_2f = isa_inb(SEQ_DATA) ;
			memblocks=1+(seq_2f&7);
			ci->ci_MemSize=memblocks*256;
			if (memconf>0) ci->ci_MemSize=ci->ci_MemSize<<1;
			vddprintf(("sis: sis5598 (shared frame buffer), MemConf=%d, Blocks=%d\n",memconf,memblocks));
			}
			break;
		case SIS6326_DEVICEID:
			ddprintf(("sis: SIS6326 using Shared Frame Buffer *** has not been tested ***\n"));
		case SIS620_DEVICEID:
		case SIS630_DEVICEID:
			isa_outb(SEQ_INDEX,0x2f);
			seq_2f = isa_inb(SEQ_DATA) ;
			t=seq_2f & 0x03;
			switch(t) {
				case 1:
					ci->ci_MemSize = 2*1024;
					break;
				case 2:
					ci->ci_MemSize = 4*1024;
					break;
				case 3:
					ci->ci_MemSize = 8*1024;
					break;
				default: // don't do anything, maybe Memory size was configured only at 0x0c
				}		
			break;
		default:
			ddprintf(("sis: Shared Frame Buffer not supported on this chip\n"));
		}
	else vddprintf(("sis: using Local Frame Buffer\n"));		

	ddprintf(("sis: Frame Buffer Size = ci->ci_MemSize (kb) =%d\n",(int)ci->ci_MemSize));

	isa_outb(SEQ_INDEX,0x24);
	graphics_frame_buffer=isa_inb(SEQ_DATA);;
	vddprintf(("sis: SEQ24: Graphics Frame Buffer onboard-location 0x%02x\n",isa_inb(SEQ_DATA)));
	
  	if (ci->ci_MemSize!=(di->di_PCI.u.h0.base_register_sizes[0]>>10)) {
    	ddprintf(("sis: Memory configuration on chipset (%d) and on PCI (%d) is different\n",(int)ci->ci_MemSize,(int)di->di_PCI.u.h0.base_register_sizes[0]>>10 ));    
    	//goto err_base0;
    	ddprintf(("sis: *** WARNING *** trying to force to %d kb mode\n",(int)ci->ci_MemSize));        
		di->di_PCI.u.h0.base_register_sizes[0] = ci->ci_MemSize<<10;
    	}
  	  
  	// TRICK 1 : Write Linear Address to SiS registers 20h and 21h
	// WARNING : the PCI address may be different from the CPU address !!!
	linear_address=di->di_PCI.u.h0.base_registers_pci[0]; // PCI
	x=linear_address;
	x>>=27;
	x=x&0x1f;
	isa_outb(SEQ_INDEX,0x21);
	y=isa_inb(SEQ_DATA);
	y&=~0x1f;

	if		(ci->ci_MemSize>4*1024) x|=0x80; // 8 Mb linear space apperture
	else if	(ci->ci_MemSize>2*1024) x|=0x60; // 4 Mb
	else if	(ci->ci_MemSize>  1024) x|=0x40; // 2 Mb
	else if	(ci->ci_MemSize>   512) x|=0x20; // 1 Mb
	else if (ci->ci_MemSize==  512) x|=0x00; // 512 kb
	else return(B_ERROR); // memory badly configured

	isa_outb(SEQ_DATA,x|y);
 	x=linear_address;
	x>>=19;
	x=x&0xff;
	isa_outb(SEQ_INDEX,0x20);
	isa_outb(SEQ_DATA,x);

	isa_outb(SEQ_INDEX,0x06);
	t=isa_inb(SEQ_DATA);
	t|=0x80; // Graphics mode linear addressing enable
	isa_outb(SEQ_DATA,t);

if (ci->ci_DeviceId != SIS630_DEVICEID) {
	isa_outb(CRTC_INDEX,0x80);
	isa_outb(CRTC_DATA,0x86);
	t=isa_inb(CRTC_DATA);
	if (t!=0xa1) {
		ddprintf(("sis : couldn't unlock video registers\n"));
		goto err_base0;
		}
	}
	
	isa_outb(SEQ_INDEX, 0x24);
	t=isa_inb(SEQ_DATA); // Graphics Frame Buffer Location	
	isa_outb(CRTC_INDEX,0xb1);
	isa_outb(CRTC_DATA,t);
  
	// TRICK 2 : Then disable mapped memory
	set_pci(&di->di_PCI,0x04,4,0x02000000);
	isa_outb(SEQ_INDEX,0x0b);
	t=isa_inb(SEQ_DATA);
	t&=~0x60;
	t|=0x60; // memMapped IO Base : 0x20: Axxxx, 0x40: Bxxxx, 0x60: PCI#14h
	isa_outb(SEQ_DATA,t);  

	// ******** and restart mapped-memory ********
	
	snooze(100000);
	if (ci->ci_DeviceId==SIS5598_DEVICEID) set_pci(&di->di_PCI,0x04,4,0x02000003);
	else set_pci(&di->di_PCI,0x04,4,0x02300007);

	isa_outb(SEQ_INDEX,0x01); // turns off the screen (again...)
	isa_outb(SEQ_DATA,0x21);

	// if anything changed here, it's not good...
	vddprintf(("sis:Read config via GET_PCI:\n"));
	vddprintf(("sis:4Mb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x10,4)));
	vddprintf(("sis:64kb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x14,4)));
	vddprintf(("sis:64kb IO base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x18,4)));     

	// it never happened but who knows...
	if ( ((get_pci(&di->di_PCI,0x10,4)&0xffff0000) != di->di_PCI.u.h0.base_registers_pci[0] )
		|(get_pci(&di->di_PCI,0x14,4) != di->di_PCI.u.h0.base_registers_pci[1] )) {
		ddprintf(("sis: pci address changed !!!\n"));     
		goto err_base0;
		}

	// Reconfigure mapped memory (configuration lost during reset)
	isa_outb(SEQ_INDEX,0x0b);
	t=isa_inb(SEQ_DATA);
	t&=~0x60;
	t|=0x60; // 0x60: PCI#14h
	t|=0x80; // BGR in byte order (RGB little indian)
	t|=0x08; // Dual segment register mode enable
	t|=0x04; // IO gating enable while write buffer is not empty
	isa_outb(SEQ_DATA,t);  
	vddprintf(("sis:SR0B: 0x%02x\n",isa_inb(SEQ_DATA)));

	isa_outb(SEQ_INDEX,0x06);
	t=isa_inb(SEQ_DATA);
	t|=0x80; // Graphics mode linear addressing enable
	isa_outb(SEQ_DATA,t);

	// restore DRAM buffer size register
	isa_outb(SEQ_INDEX,0x2f);
	seq_2f &=~0x20 ; // disable fast change mode timing
	isa_outb(SEQ_DATA, seq_2f);

	// Configure some other cool stuffs
  
	isa_outb(SEQ_INDEX,0x0C);
	t=isa_inb(SEQ_DATA);
	t|=0x80; // 64 bits mode / doc:graphics mode 32-bit memory access enable
	t|=0x20; // Read-ahead cache
	isa_outb(SEQ_DATA,t);

	if (ci->ci_DeviceId == SIS5598_DEVICEID) {
		isa_outb(SEQ_INDEX,0x26);
		t=isa_inb(SEQ_DATA);
		t|=0x40; // PCI Burst-write mode enable 
		t|=0x20; // Continuous Memory Data Access enable
		isa_outb(SEQ_DATA,t);
  
		isa_outb(SEQ_INDEX,0x33);
		t=isa_inb(SEQ_DATA);
		t&=~0x30; // enable standard VGA IO and PCI_18h
		isa_outb(SEQ_DATA,t);
		}


	// Map Frame Buffer
	strcpy (name + len, " framebuffer");
	retval = map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[0],
			      di->di_PCI.u.h0.base_register_sizes[0],
			      B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr0 );

	if (retval <= 0) {/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		ddprintf(("sis: map_physical_memory : write-combining not supported\n"));
		retval = map_physical_memory
			  (name,
			   (void *) di->di_PCI.u.h0.base_registers[0],
			   di->di_PCI.u.h0.base_register_sizes[0],
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr0);
		}
	vddprintf(("sis: BaseAddr0  =0x%08x\n",(uint)ci->ci_BaseAddr0));
	vddprintf(("sis: BaseAddr0ID=%d\n",(int)retval));
	if (retval <= 0) {
		ddprintf(("sis: fatal error : map_physical_memory returned code %d\n",(int)retval));
		goto err_base0;
	}
	di->di_BaseAddr0ID = retval;
  
	// Map chip's extended registers.
	
	strcpy (name + len, " regs");
	retval = map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[1],
			      di->di_PCI.u.h0.base_register_sizes[1],
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr1);
	vddprintf(("sis : map registers returns code %d\n",(int)retval));
	if (retval<=0) {
		ddprintf(("sis : could not map registers\n"));
		retval=29999; // this mapping doesn't seem to work, so everyone access directly to the registers...
		}
	if (retval <= 0) goto err_base1; //  Look down
	di->di_BaseAddr1ID = retval;
	
	// No relocated IO Registers
	di->di_BaseAddr2ID	= -1 ;
	ci->ci_BaseAddr2	= NULL ;
	
	vddprintf(("sis: map_device() finished OK\n"));
	return (B_OK);

err_base2:
		vddprintf(("sis : --- setup err_base2\n"));
		delete_area (di->di_BaseAddr1ID);
		di->di_BaseAddr1ID = -1;
		ci->ci_BaseAddr1 = NULL;
err_base1:
		vddprintf(("sis : --- setup err_base1\n"));
		delete_area (di->di_BaseAddr0ID);
		di->di_BaseAddr0ID = -1;
		ci->ci_BaseAddr0 = NULL;
err_base0:
		return (retval);
	}


/////////////////
// UNMAP_DEVICE
/////////////////

// FIXME !!! Unregister Pool

void unmap_device (struct devinfo *di) {
	register sis_card_info *ci = di->di_sisCardInfo;

	vddprintf (("sis:+++ Unmapping device %s\n", di->di_Name));
		
	if (di->di_BaseAddr2ID >= 0)
		delete_area (di->di_BaseAddr2ID), di->di_BaseAddr2ID = -1;
	if (di->di_BaseAddr1ID >= 0)
		delete_area (di->di_BaseAddr1ID), di->di_BaseAddr1ID = -1;
	if (di->di_BaseAddr0ID >= 0)
		delete_area (di->di_BaseAddr0ID), di->di_BaseAddr0ID = -1;
	ci->ci_BaseAddr0	= NULL;
	ci->ci_BaseAddr1	= NULL;
	ci->ci_BaseAddr2	= NULL;
	ci->ci_FBBase		= NULL;
	ci->ci_FBBase_DMA	= NULL;
    }

/////////////////
// sis_INIT
/////////////////

status_t sis_init(devinfo *di) {
	BMemRange		mr;
	
	register sis_card_info *ci;
	status_t		ret = B_OK;
	uchar			t;
	int				i;
	
	ddprintf (("+++ sis_init()\n"));
	ci = di->di_sisCardInfo; 
	ci->vbl_calls	= 0 ;
	
	//ci->ci_MemSize-= (MEMORY_MARGIN>>10); // reserve place for cursor and turbo-queue
	ci->ci_BaseAddr0_DMA=	(void *) di->di_PCI.u.h0.base_registers_pci[0] + MEMORY_MARGIN ;
	ci->ci_RegBase		=	(uchar*) ci->ci_BaseAddr1 ;
	ci->ci_IORegBase	=	(uchar*) ci->ci_BaseAddr2 ;

	/*
	 * Declare framebuffer to genpool module.
	 * (Jason estimates an average of 128 allocations per megabyte.
	 * Hence the weird MemSize>>13 down there.)
	 */
	 
	gfbpi->pi_Pool_AID	= di->di_BaseAddr0ID;
	gfbpi->pi_Pool		= (void *) ci->ci_BaseAddr0;
	gfbpi->pi_Size		= ci->ci_MemSize*1024;
	strcpy (gfbpi->pi_Name, "SiS Framebuffer");
	if ((ret = (ggpm->gpm_CreatePool) (gfbpi, ci->ci_MemSize >> 2, 0)) < 0) {
		ddprintf (("sis: +++ Failed to create genpool, retval = %d (0x%08lx)\n", ret, ret));
		return (ret);
		}
	di->di_PoolID = gfbpi->pi_PoolID ;
	ci->ci_PoolID = di->di_PoolID ;
	vddprintf(("sis: Frame Buffer of %d kb has been registered to the PoolID %d\n",ci->ci_MemSize,ci->ci_PoolID));
	
	/*
	 * Mark Range for command buffer and cursor out of the pool.

	 */
	switch(ci->ci_DeviceId) {
		case SIS5598_DEVICEID:
		case SIS6326_DEVICEID:
		case SIS620_DEVICEID:
			mr.mr_Offset= 0 ; // FIXME ?
			mr.mr_Size	= 256*1024 ; // FIXME ?
			break;
		case SIS630_DEVICEID:
			mr.mr_Offset= 0 ;
			mr.mr_Size	= 16*1024 ;
			break;
		}
		
	mr.mr_Owner	= 0;

	if ((ret = (ggpm->gpm_MarkRange(ci->ci_PoolID, &mr, NULL, 0))) < 0) {
		ddprintf (("sis: +++ Failed to mark range for cmdbuf and cursor, retval = %d (0x%08lx)\n", ret, ret));
		(ggpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
		return (ret);
		}
		
	ci->ci_Cursor_offset = mr.mr_Offset ;
	vddprintf(("sis: video memory reserved for cursor at 0x%08x\n",ci->ci_Cursor_offset));

	// Display modes
	ci->ci_FBMemSpec.ms_MR.mr_Size	= 0 ;	// proves that no DesktopScreen is allocated
	ci->ci_DispModes	= NULL;
	ci->ci_NDispModes	= -1;
	ci->ci_Depth 		= -1 ;
	ci->ci_BytesPerRow	= -1 ;
	ci->ci_CurDispMode.virtual_width	= -1;
	ci->ci_CurDispMode.virtual_height	= -1;
	ci->ci_CurDispMode.timing.pixel_clock = 1 ; // Be carefull don't put -1 !!! 

	// Overlay
	for(i=0; i<MAX_OVERLAYS;i++) ci->ovl_token[i].used=0;

	// Card Identification
	switch(di->di_PCI.device_id) {
		case SIS5598_DEVICEID:
			strcpy (ci->ci_ADI.name, "Integrated VGA Controller");
			strcpy (ci->ci_ADI.chipset, "SiS 5598");
			ci->ci_Clock_Max = SIS5598_CLOCK_MAX ;
			break;
		case SIS6326_DEVICEID:
			strcpy (ci->ci_ADI.name, "AGP VGA Controller");
			strcpy (ci->ci_ADI.chipset, "SiS 6326");
			ci->ci_Clock_Max = SIS6326_CLOCK_MAX ;
			break;
		case SIS620_DEVICEID:
			strcpy (ci->ci_ADI.name, "AGP VGA Controller");
			strcpy (ci->ci_ADI.chipset, "SiS 620");
			ci->ci_Clock_Max = SIS6326_CLOCK_MAX ;
			break;
		case SIS630_DEVICEID:
			strcpy (ci->ci_ADI.name, "AGP VGA Controller");
			strcpy (ci->ci_ADI.chipset, "SiS 630");
			ci->ci_Clock_Max = SIS6326_CLOCK_MAX ;
			break;
		default:		
			ddprintf(("sis: *** couldn't get info for the chip\n"));
		}
	
	ci->ci_ADI.dac_speed		=	135000;
	ci->ci_ADI.memory			=	ci->ci_MemSize;
	ci->ci_ADI.version			=	B_ACCELERANT_VERSION;

	ci->ci_PrimitivesIssued		=	0;
	ci->ci_PrimitivesCompleted	=	0;

	// init standard VGA registers
	set_table(sequencer_table);
	set_table(gcr_table);
	set_attr_table(attribute_table);


if (ci->ci_DeviceId != SIS630_DEVICEID) {
	// Enable Fast-Page Flip
	isa_outb(SEQ_INDEX,0x2f);
	t=isa_inb(SEQ_DATA);
	t|=0x30; // enable fast page flip
	isa_outb(SEQ_DATA,t);

	isa_outb(SEQ_INDEX,0x30); // Set PAGE address to 0
	isa_outb(SEQ_DATA,0);
	isa_outb(SEQ_INDEX,0x31);
	isa_outb(SEQ_DATA,0);
	isa_outb(SEQ_INDEX,0x32);
	isa_outb(SEQ_DATA,0);
	}

	switch (ci->ci_DeviceId) {			// Chipset specific initialisations

										///////////////////////////////
										// -------- SiS 620 -------- //
										///////////////////////////////
										
		case SIS620_DEVICEID : { // -------- SiS 620 Specific -------- 
			isa_outb(SEQ_INDEX,0x27);	// Graphics Engine Register 1
			t=isa_inb(SEQ_DATA);
			t|=0x40;					// Graphics Engine enable
			t|=0x80;					// enable software turbo queue
			isa_outb(SEQ_DATA,t);
		
			isa_outb(SEQ_INDEX,0x2c);
			isa_outb(SEQ_DATA,0x00);	// turbo-queue base address bit[7:0] (unit of 32k/64k )

			isa_outb(SEQ_INDEX,0x37);	// reserved... value dumped from w*
			isa_outb(SEQ_DATA,0x94);
			
			isa_outb(SEQ_INDEX,0x3d);
			t=isa_inb(SEQ_DATA);
			t&=~0x80;					// disable 62k turbo queue ( doesn't work with synctotoken )
										// So queue is 32 kb instead. anyway, performance is the same...

			isa_outb(SEQ_DATA,t);

			isa_outb(SEQ_INDEX,0x39);
			t=isa_inb(SEQ_DATA);
			isa_outb(SEQ_DATA,t|0x04);	// 3D Accel Control Enable

			isa_outb(SEQ_INDEX,0x3d);
			t=isa_inb(SEQ_DATA);
			// t|=0x01;					// enable 3D pre-setup engine
			t&=~0x01;					// w* : disable 3D pre-setup engine
			isa_outb(SEQ_DATA,t); 
	
			isa_outb(SEQ_INDEX,0x3e);
			t=isa_inb(SEQ_DATA);
			t|=0x08 ;					// enable DRAM stick to texture-read request
			t|=0x01 ;					// enable high speed DCLK
			isa_outb(SEQ_DATA,t);
		
			isa_outb(SEQ_INDEX,0x34);
			t=isa_inb(SEQ_DATA);
			t&=~0x01;					// disable Hardware Command Queue threshold low
			//t|=0x01;					// enable
			isa_outb(SEQ_DATA,t);
			
			if (ci->ci_Device_revision == 0xa2 ) { // SiS 530
				isa_outb(SEQ_INDEX,0x35);
				t=isa_inb(SEQ_DATA);
				t&=~0x20; // enable SGRAM burst timings
				isa_outb(SEQ_DATA,t);
				}
								
			}
			break;

										////////////////////////////////
										// -------- SiS 6326 -------- //
										////////////////////////////////
										
		case SIS6326_DEVICEID:
			isa_outb(SEQ_INDEX,0x27);	// Graphics Engine Register 1
			t=isa_inb(SEQ_DATA);
			t|=0x40;					// Graphics Engine enable
			t|=0x80;					// enable software turbo queue
			isa_outb(SEQ_DATA,t);
		
			isa_outb(SEQ_INDEX,0x2c);
			isa_outb(SEQ_DATA,0x00);	// turbo-queue base address bit[7:0] (unit of 32k/64k )

			isa_outb(SEQ_INDEX,0x3d);
			t=isa_inb(SEQ_DATA);
			//t|=0x80;					// enable 62k turbo queue ( 30k when not set )
			t&=~0x80;					// disable 62k turbo queue : buggy
			isa_outb(SEQ_DATA,t);

			isa_outb(SEQ_INDEX,0x39);
			t=isa_inb(SEQ_DATA);
			isa_outb(SEQ_DATA,t|0x04);	// 3D Accel Control Enable

			isa_outb(SEQ_INDEX,0x3d);
			t=isa_inb(SEQ_DATA);
			//t|=0x01;					// enable 3D pre-setup engine
			t&=~0x01;					// w* : disable 3D pre-setup engine
			isa_outb(SEQ_DATA,t); 

			break;
			
										////////////////////////////////
										// -------- SiS 6326 -------- //
										////////////////////////////////										
			
		case SIS5598_DEVICEID:
			isa_outb(SEQ_INDEX,0x27);	// Graphics Engine Register 1
			t=isa_inb(SEQ_DATA);
			t|=0x40;					// Graphics Engine enable
			t&=~0x80;					// disable software turbo queue (buggy)
			isa_outb(SEQ_DATA,t);
			break;			
		}		

	vddprintf(("sis: --- sis_init() terminated\n"));
	return(B_OK);
	}
  
  
//////////////////////////////////////////////////////
// -------- Map Device for SiS 630 Chipset -------- //
//////////////////////////////////////////////////////


status_t sis630_map_device (register struct devinfo *di) {
	sis_card_info	*ci = di->di_sisCardInfo;
	status_t		retval=B_ERROR;
	int				len;
	char			name[B_OS_NAME_LENGTH];
	uchar			t;

	//  Create a goofy basename for the areas
	sprintf (name, "%04X_%04X_%02X%02X%02X",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (name);

	// Unlock SEQ extended registers
	isa_outb(SEQ_INDEX,0x05);
	isa_outb(SEQ_DATA,0x86);
	t=isa_inb(SEQ_DATA);
	if (t!=0xa1) {
		ddprintf(("sis: Could not unlock SEQ extended registers - return code 0x%02x\n",t));
		}
	else vddprintf(("sis: Unlock SEQ extended registers succeed\n"));      
		
	vddprintf(("sis: pci_register[0]: 0x%08x 0x%08x 0x%08x\n",
		(uint)di->di_PCI.u.h0.base_registers[0],
		(uint)di->di_PCI.u.h0.base_registers_pci[0],
		(uint)di->di_PCI.u.h0.base_register_sizes[0] ));

	vddprintf(("sis:ISA ans MemMapped IO ? (02000003) 0x%08x\n",(uint)get_pci(&di->di_PCI,0x04,4)));
	vddprintf(("sis:Read config via GET_PCI:\n"));
	vddprintf(("sis:4Mb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x10,4)));
	vddprintf(("sis:64kb mem-base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x14,4)));
	vddprintf(("sis:64kb IO base 0x%08x\n",(uint)get_pci(&di->di_PCI,0x18,4)));     
	
		
	// Support for SIS5598 and SIS6326
	// should work for SIS5597 considered as SIS5598
	// and for SIS6205, 6215, 6225 considered as SIS6326
	
	// DRAM Sizing Register
	isa_outb(SEQ_INDEX, 0x14);
	t=isa_inb(SEQ_DATA);
	switch(t>>6) {
		case 0:
			vddprintf(("sis630: Bus was in 32 bits mode\n"));
			break;
		case 1:
			vddprintf(("sis630: Bus was in 64 bits mode\n"));
			break;
		case 2:
			vddprintf(("sis630: Bus was in 128 bits mode\n"));
			break;
		case 3:
		default:
			ddprintf(("sis630: warning - Bus mode invalid\n"));
			break;
		} ;
	t &= ~(0x80|0x40) ;
	t |= 0x80  ;
	vddprintf(("sis630: Setting bus width to 128 bits\n"));
	isa_outb(SEQ_DATA, t );
	
	ci->ci_MemSize = ((t&0x3f)+1 ) << 10 ;
	vddprintf(("sis: memory size is %d Mb\n",ci->ci_MemSize >> 10));
	ci->ci_Shared_Mode = TRUE;

  	if (ci->ci_MemSize!=(di->di_PCI.u.h0.base_register_sizes[0]>>10)) {
    	ddprintf(("sis: Memory configuration on chipset (%d) and on PCI (%d) is different\n",(int)ci->ci_MemSize,(int)di->di_PCI.u.h0.base_register_sizes[0]>>10 ));    
    	ddprintf(("sis: *** WARNING *** trying to force to %d kb mode\n",(int)ci->ci_MemSize));        
		di->di_PCI.u.h0.base_register_sizes[0] = ci->ci_MemSize<<10;
    	}

	// PCI Address Decoder Setting Register
	isa_outb(SEQ_INDEX, 0x20);
	isa_outb(SEQ_DATA, 0xa1); // Linear Address Mode + Enable Relocated VGA IO + Mem Mapped IO Enable
	vddprintf(("sis630: Activate Linear Address Mode returns 0x%02x\n",isa_inb(SEQ_DATA))); 
	
	// Map Frame Buffer
	strcpy (name + len, " framebuffer");
	retval = map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[0],
			      di->di_PCI.u.h0.base_register_sizes[0],
			      B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr0 );

	if (retval <= 0) {/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		ddprintf(("sis: map_physical_memory : write-combining not supported\n"));
		retval = map_physical_memory
			  (name,
			   (void *) di->di_PCI.u.h0.base_registers[0],
			   di->di_PCI.u.h0.base_register_sizes[0],
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr0);
		}
	vddprintf(("sis: FrameBuffer mapping : BaseAddr0=0x%08x\n",(uint32)ci->ci_BaseAddr0));
	vddprintf(("sis: BaseAddr0ID=%d\n",(int)retval));
	if (retval <= 0) {
		ddprintf(("sis: fatal error : map_physical_memory returned code %d\n",(int)retval));
		goto err_base0;
	}
	di->di_BaseAddr0ID = retval;
  
	// Map chip's extended registers.
	
	strcpy (name + len, " regs");
	retval = map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[1],
			      di->di_PCI.u.h0.base_register_sizes[1],
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr1);
	vddprintf(("sis630 : map registers returns code %d\n",(int)retval));
	if (retval<=0) {
		ddprintf(("sis630 : could not map registers\n"));
		goto err_base1;
		}
	di->di_BaseAddr1ID = retval;
	vddprintf(("sis630: Registers mapping BaseAddr1=0x%08x\n",(uint32)ci->ci_BaseAddr1));

	// Map chip's relocated IO registers.
	{
	uint32 base, size ;
	base = (uint32) di->di_PCI.u.h0.base_registers[2] ;
	size = (uint32) di->di_PCI.u.h0.base_register_sizes[2] ;
	vddprintf(("sis630: IO Registers Base=0x%08x, Size=0x%08x\n", base, size));
	base = base & ~(B_PAGE_SIZE - 1) ;
	size += (uint32) di->di_PCI.u.h0.base_registers[2] - base ;
	size = (size + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE - 1) ;
	vddprintf(("sis630: Align IO Registers to Base=0x%08x, Size=0x%08x\n", base, size));
	
	strcpy (name + len, " IO regs");
	retval = map_physical_memory (name,
			      (void *) base ,
			      size,
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr2);
	vddprintf(("sis630 : map IO registers returns code %d\n",(int)retval));
	if (retval<=0) {
		ddprintf(("sis630 : could not map IO registers\n"));
		retval=29999; // this mapping doesn't seem to work, but is only absoluty necessary for video
		di->di_BaseAddr2ID	= -1;
		ci->ci_BaseAddr2	= NULL ;
		}
	else {
		di->di_BaseAddr2ID = retval ;
		vddprintf(("sis630: IO Registers mapping base is 0x%08x but need to include alignment\n",(uint32)ci->ci_BaseAddr2));
		ci->ci_BaseAddr2 += (uint32) di->di_PCI.u.h0.base_registers[2] - base ;
		}
		
	vddprintf(("sis630: IO Registers mapping BaseAddr2=0x%08x\n",(uint32)ci->ci_BaseAddr2));
	}
	
	vddprintf(("sis630: map_device() finished OK\n"));
	return (B_OK);

err_base2:
		vddprintf(("sis630 : --- setup err_base2\n"));
		delete_area (di->di_BaseAddr1ID);
		di->di_BaseAddr1ID = -1;
		ci->ci_BaseAddr1 = NULL;
err_base1:
		vddprintf(("sis630 : --- setup err_base1\n"));
		delete_area (di->di_BaseAddr0ID);
		di->di_BaseAddr0ID = -1;
		ci->ci_BaseAddr0 = NULL;
err_base0:
		return (retval);
	}

void sis630_read_ClockRegisters() ;

status_t sis630_init(devinfo *di) {
	status_t ret ;
	uchar t ;
	sis_card_info	*ci = di->di_sisCardInfo;
	
		
	sis630_Read_ClockRegisters('M');
	sis630_Read_ClockRegisters('D');
	sis_init(di);
	

	/* Setup 2D Engine */
	memset(&ci->ci_TurboQueue_ms, 0, sizeof(ci->ci_TurboQueue_ms));
	ci->ci_TurboQueue_ms.ms_PoolID			= ci->ci_PoolID;
	ci->ci_TurboQueue_ms.ms_AddrCareBits	= 0x0000ffff ;
	ci->ci_TurboQueue_ms.ms_AddrStateBits	= 0x00000000 ;
	ci->ci_TurboQueue_ms.ms_MR.mr_Size		= 64*1024 ; // Only 64 Kb seems to work
	ci->ci_TurboQueue_ms.ms_MR.mr_Owner		= 0 ;
	ci->ci_TurboQueue_ms.ms_AllocFlags		= 0 ;
	if ((ret = ggpm->gpm_AllocByMemSpec(&ci->ci_TurboQueue_ms)) < 0) {
		ddprintf (("sis: +++ Failed to allocated range for TurboQueue, retval = %d (0x%08lx)\n", ret, ret));
		(ggpm->gpm_DeletePool) (ci->ci_PoolID);
		ci->ci_PoolID = 0;
		return (ret);
		}
	vddprintf(("sis: TurboQueue range allocated at 0x%08x, size %d kb\n",ci->ci_TurboQueue_ms.ms_MR.mr_Offset, ci->ci_TurboQueue_ms.ms_MR.mr_Size>>10 ));
	memset((uint32)ci->ci_BaseAddr0 + ci->ci_TurboQueue_ms.ms_MR.mr_Offset, 0, ci->ci_TurboQueue_ms.ms_MR.mr_Size );


	isa_outb(SEQ_INDEX, 0x26);
	isa_outb(SEQ_DATA, ci->ci_TurboQueue_ms.ms_MR.mr_Offset >> 16 );
	t = 0x80 ;			// Turbo Queue Enable

	switch(ci->ci_TurboQueue_ms.ms_MR.mr_Size >> 10) {
		case 64:
			t |= 0 << 4 ;
			break;
		case 128:
			t |= 1 << 4 ;
			break;
		case 256:
			t |= 2 << 4 ;
			break;
		case 512:
			t |= 3 << 4 ;
			break;
		default:
			ddprintf(("sis630: - warning - unknown turbo queue size %d kb\n",ci->ci_TurboQueue_ms.ms_MR.mr_Size >> 10));
		}
	t |= 0x40 ; // Pure Soft Queue
	t |= (0x03&(ci->ci_TurboQueue_ms.ms_MR.mr_Offset >>24)) ; // TurboQueue Base Address [25:24]
	isa_outb(SEQ_INDEX, 0x27 );
	isa_outb(SEQ_DATA, t );
			
	ci->ci_sis630_AGPBase = inw(0x8206) ;
	vddprintf(("sis630: AGPBase Address 0x%08x\n",ci->ci_sis630_AGPBase & 0x03ff ));

	/* Modules Enable Register */
	isa_outb(SEQ_INDEX, 0x1e );
	isa_outb(SEQ_DATA,
						 0x40	// Enable 2D
						|0x20	// Enable CRT2
						|0x10	// Enable 3D AGP command fetch
						|0x08	// Enable 3D command parser
						//|0x04	// Enable VGA BIOS flash memory data write
						|0x02	// Enable 3D Accelerator
						|0x01	// Enable Hardware MPEG
						); 

	}
