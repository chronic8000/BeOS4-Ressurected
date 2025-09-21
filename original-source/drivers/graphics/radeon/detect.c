#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"

//DWORD BPPOverride = 0; // Override 8bpp mode for Chapter 6 3D samples.
extern pci_module_info *pci_bus;

/*
void Radeon_ShutDown (void)
{
    DWORD temp;

    Radeon_ResetEngine ();

    if ((Radeon_AdapterInfo.FLAGS & (Radeon_USE_BIOS)) == 0)
    {
        temp = PLL_regr (PPLL_CNTL);
        temp &= ~(PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN
	      | PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN);
        PLL_regw (PPLL_CNTL, temp);
    }

    set_old_mode ();
    return;
} // Radeon_ShutDown
*/


/****************************************************************************
 * phys_to_virt                                                             *
 *  Function: takes a physical segment address and size and gets the        *
 *            equivalent virtual or logical address which can directly      *
 *            access this memory.  INT 31h function 800h                    *
 *    Inputs: physical - physical address of memory                         *
 *            size - size of address                                        *
 *   Outputs: virtual address that is direct memory accessible              *
 ****************************************************************************/

#if 0
DWORD phys_to_virt (DWORD physical, DWORD size)
{
    union REGS r;
    struct SREGS sr;
    DWORD retval=0;

    memset (&r, 0, sizeof (r));
    memset (&sr, 0, sizeof (sr));
    r.w.ax = 0x0800;
    r.w.bx = physical >> 16;
    r.w.cx = physical & 0xFFFF;
    r.w.si = size >> 16;
    r.w.di = size & 0xFFFF;
    int386x (0x31, &r, &r, &sr);
    if ((r.w.cflag & INTR_CF) == 0)
    {
        retval = (long) (((long) r.w.bx << 16) | r.w.cx);
    } // if

    return (retval);

} // phys_to_virt
#endif

/****************************************************************************
 * Radeon_PrintInfoStruct                                                     *
 *  Function: Print AdapterInfo Structure                                   *
 *    Inputs: none                                                          *
 *   Outputs: none                                                          *
 ****************************************************************************/

#if 0
void Radeon_PrintInfoStruct (void)
{
    printf ("\nAdapterInfo:\n");
    printf ("PCI Vendor ID: %4.4X\n", Radeon_AdapterInfo.PCI_Vendor_ID);
    printf ("PCI Device ID: %4.4X\n", Radeon_AdapterInfo.PCI_Device_ID);
    printf ("PCI Revision ID: %2.2X\n", Radeon_AdapterInfo.PCI_Revision_ID);
    printf ("PCI Bus Number: %2.2X\n", Radeon_AdapterInfo.bus_num);
    printf ("PCI Device Number: %2.2X\n", Radeon_AdapterInfo.dev_num);
    printf ("Memory Aperture Base: %8.8X\n", Radeon_AdapterInfo.MEM_BASE);
    printf ("Virtual Memory Aperture Base: %8.8X\n", Radeon_AdapterInfo.virtual_MEM_BASE);
    printf ("I/O Register Aperture Base: %8.8X\n", Radeon_AdapterInfo.IO_BASE);
    printf ("Memory Mapped Register Base: %8.8X\n", Radeon_AdapterInfo.REG_BASE);
    printf ("Virtual Memory Mapped Register Base: %8.8X\n", Radeon_AdapterInfo.virtual_REG_BASE);
    printf ("BIOS Segment: %8.8X\n", Radeon_AdapterInfo.BIOS_SEG);
    printf ("FLAGS: %8.8X\n", Radeon_AdapterInfo.FLAGS);
    printf ("pitch: %4.4X (%d)\n", Radeon_AdapterInfo.pitch,Radeon_AdapterInfo.pitch);
    printf ("bpp=%d, bytepp=%d\n", Radeon_AdapterInfo.bpp,Radeon_AdapterInfo.bytepp);
    printf ("Installed Memory (in Mb): %2.2d\n", Radeon_AdapterInfo.memsize/1048576);
    printf ("\nX resolution: %d, Y resolution: %d, bits per pixel: %d\n",
            Radeon_AdapterInfo.xres, Radeon_AdapterInfo.yres,
            Radeon_AdapterInfo.bpp);

    return;
}  // Radeon_PrintInfoStruct
#endif


/****************************************************************************
 * Radeon_Detect                                                                *
 *  Function: Radeon chip detection.                                        *
 *    Inputs: none                                                          *
 *   Outputs: 1 - chip detected                                             *
 *            0 - chip not detected                                         *
 ****************************************************************************/

uint8 Card_Detect()
{
	pci_info pcii;
	int32 idx = 0;
	int8 found = 0;

	dprintf (("Radeon: Card_Detect \n" ));

	if ( get_module (B_PCI_MODULE_NAME, (module_info **) & pci_bus) != B_OK )
	{
		dprintf (("Radeon: Card_Detect get_module falied \n" ));
		return 0;
	}

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0; (*pci_bus->get_nth_pci_info) (idx, &pcii) == B_NO_ERROR; idx++)
	{
		if( pcii.vendor_id == ATI_PCI_VENDOR_ID )
		{
			switch( pcii.device_id )
			{
			case DEVICE_ID_CODE_QD:		//Radeon QD
			case DEVICE_ID_CODE_QE:		//Radeon QE
			case DEVICE_ID_CODE_QF:		//Radeon QF
			case DEVICE_ID_CODE_QG:		//Radeon QG

			case DEVICE_ID_CODE_M6:		//Radeon M6
			case DEVICE_ID_CODE_RV100:      //Radeon RV100
				found = 1;
				break;
			
			default:       // no Radeon based product detected, 
				break;
			}
		}
	}

	put_module (B_PCI_MODULE_NAME);
	pci_bus = NULL;
	return found;
}

uint8 Radeon_Probe()
{
	uint32 pci_index = 0;
	DeviceInfo *di = &radeon_di.Device[0];

	dprintf(("Radeon: Radeon_Probe called \n" )); 
    // clear out the Radeon_AdapterInfo structure

    memset ( &radeon_di, 0, sizeof(radeon_di) );
	if ( get_module( B_PCI_MODULE_NAME, (module_info **) &pci_bus) != B_OK )
		return 0;
		
	dprintf(("3dfx: probe_devices - ENTER\n"));

	while( (radeon_di.DeviceCount < MAX_CARDS) &&
			((*pci_bus->get_nth_pci_info) (pci_index, &(di->PCI)) == B_NO_ERROR))
	{
		if( di->PCI.vendor_id == ATI_PCI_VENDOR_ID )
		{
			switch( di->PCI.device_id )
			{
			case DEVICE_ID_CODE_QD:		//Radeon QD
			case DEVICE_ID_CODE_QE:		//Radeon QE
			case DEVICE_ID_CODE_QF:		//Radeon QF
			case DEVICE_ID_CODE_QG:		//Radeon QG

			case DEVICE_ID_CODE_M6:		//Radeon M6
			case DEVICE_ID_CODE_RV100:      //Radeon RV100
				{
					// We found one !!!
					dprintf(("Radeon: Radeon_Probe found an ATI card \n" )); 
					
					if( !Radeon_TestBios(di) )
					{
						// OK maybe not
						dprintf(("Radeon: Radeon_Probe bios test failed ! \n" )); 
						break;
					}
					
					sprintf (di->Name,
							"graphics%s/%04X_%04X_%02X%02X%02X", GRAPHICS_DEVICE_PATH,
							di->PCI.vendor_id, di->PCI.device_id,
							di->PCI.bus, di->PCI.device, di->PCI.function);

					di->info.PCI_Vendor_ID = di->PCI.vendor_id;
					di->info.PCI_Device_ID = di->PCI.device_id;
					di->info.PCI_Revision_ID = di->PCI.revision;

					if( (di->PCI.device_id == DEVICE_ID_CODE_M6) ||
						(di->PCI.device_id == DEVICE_ID_CODE_RV100) )
					{
						di->isMobility = 1;
					}
					else
					{
						di->isMobility = 0;
					}
					dprintf(("Radeon: Radeon_Probe isMobility %x \n", di->isMobility )); 
					dprintf(("Radeon: X1 Radeon_Probe isMobility %x \n", radeon_di.Device[0].isMobility )); 
					
					radeon_di.DevNames[radeon_di.DeviceCount] = di->Name;
					
					di->SlotNum = pci_index;

					di->info.bus_num = di->PCI.bus;
					di->info.dev_num = di->PCI.device;
					di->info.MEM_BASE = (*pci_bus->read_pci_config)(di->PCI.bus, di->PCI.device, 0, 0x10, 4 ) & 0xfffffff0;
					di->info.IO_BASE = (*pci_bus->read_pci_config)(di->PCI.bus, di->PCI.device, 0, 0x14, 4 ) & 0xffffff00;
					di->info.REG_BASE = (*pci_bus->read_pci_config)(di->PCI.bus, di->PCI.device, 0, 0x18, 4 ) & 0xfff00000;

					dprintf(("Radeon: X2 Radeon_Probe isMobility %x \n", radeon_di.Device[0].isMobility ));
					
					di++;
					radeon_di.DeviceCount++;
				}
				break;
			
			default:       // no Radeon based product detected, 
				break;
			}
		}
		pci_index++;
	}


dprintf(("Radeon: X3 Radeon_Probe isMobility %x \n", radeon_di.Device[0].isMobility ));
/*
    // get the virtual address for the register base.
    Radeon_AdapterInfo.virtual_REG_BASE = phys_to_virt(Radeon_AdapterInfo.REG_BASE, (10*1024));

    // Before we can determine the virtual address of the frame buffer,
    // we need to know how much memory is installed.  Read CONFIG_MEMSIZE
    temp = regr (CONFIG_MEMSIZE);
    // the memory size is in bits [28:0], so we'll mask off [31:29]
    temp = temp & 0x1FFFFFFF;
    Radeon_AdapterInfo.memsize = temp;

    Radeon_AdapterInfo.virtual_MEM_BASE = phys_to_virt(Radeon_AdapterInfo.MEM_BASE, temp);
*/



	/*  Terminate list of device names with a null pointer  */
	
	radeon_di.DevNames[radeon_di.DeviceCount] = 0;

	dprintf( ("Radeon: Radeon_Probe %x Cards found. \n", radeon_di.DeviceCount ) ); 
dprintf(("Radeon: X4 Radeon_Probe isMobility %x %p\n", radeon_di.Device[0].isMobility, &radeon_di.Device[0].isMobility ));
dprintf(("Radeon: X4 Radeon_Probe di %p\n", &radeon_di.Device[0] ));
    return radeon_di.DeviceCount;

} 


/****************************************************************************
 * Radeon_FindRom                                                             *
 *  Function: Find ATI ROM segment.                                         *
 *    Inputs: none                                                          *
 *   Outputs: 1 - chip detected                                             *
 *            0 - chip not detected                                         *
 ****************************************************************************/

#if 0
BYTE Radeon_FindRom (void)
{
    DWORD segstart;                     // ROM start address
    char *rom_base;                     // base of ROM segment
    char *rom;                          // offset pointer
    int stage;                          // stage 4 - done all 3 detections
    int i;                              // loop counter
    char ati_rom_sig[] = "761295520";   // ATI signature
    char Radeon_sig[] = "RG6";          // Radeon signature
    BYTE flag = 0;                      // assume Radeon not found

    // Traverse ROM, looking at every 4 KB segment for ROM ID (stage 1).
    for (segstart = 0x000C0000; segstart < 0x000F0000; segstart += 0x00001000)
    {
        stage = 1;
        rom_base = (char *) segstart;
        if ((*rom_base == 0x55) && (*(rom_base + 1) == 0xAA))
        {
            stage = 2;                  // ROM found
        } // if
        if (stage != 2) continue;       // ROM ID not found.
        rom = rom_base;

        // Search for ATI signature (stage 2).
        for (i = 0; (i < 128 - strlen (ati_rom_sig)) && (stage != 3); i++)
        {
            if (ati_rom_sig[0] == *rom)
            {
                if (strncmp (ati_rom_sig, rom, strlen (ati_rom_sig)) == 0)
                {
                    stage = 3;          // Found ATI signature.
                } // if
            } // if
            rom++;
        } // for
        if (stage != 3) continue;       // ATI signature not found.
        rom = rom_base;

        // Search for Radeon signature (stage 3).
        for (i = 0; (i < 512) && (stage != 4); i++)
        {
            if (Radeon_sig[0] == *rom)
            {
                if (strncmp (Radeon_sig, rom, strlen (Radeon_sig)) == 0)
                {
                    stage = 4;          // Found Radeon signature.
                } // if
            } // if
            rom++;
        } // for
        if (stage != 4) continue;       // Radeon signature not found.

        // Successful! (stage 4)
        //fprintf (op, "\nRadeon BIOS located at segment %4.4X\n", rom_base);
        // Fill in AdapterInfo struct accordingly
        Radeon_AdapterInfo.BIOS_SEG = (DWORD) rom_base;
        flag = 1;                       // Return value set to successful.
        break;
    } // for

    return (flag);

} // Radeon_FindRom

/****************************************************************************
 * Radeon_RegTest                                                             *
 *  Function: Uses BIOS_0_SCRATCH to test read/write functionality          *
 *    Inputs: NONE                                                          *
 *   Outputs: 0 - unsuccessful                                              *
 *            1 - successful                                                *
 ****************************************************************************/

BYTE Radeon_RegTest (void)
{
    DWORD save_value;
    BYTE flag = 0;

    // Save old value.
    save_value = regr (BIOS_0_SCRATCH);

    // Test odd bits for readability.
    regw (BIOS_0_SCRATCH, 0x55555555);
    if (regr (BIOS_0_SCRATCH) == 0x55555555)
    {
        // Test even bits for readability.
        regw (BIOS_0_SCRATCH, 0xAAAAAAAA);
        if (regr (BIOS_0_SCRATCH) == 0xAAAAAAAA)
        {
            flag = 1;
        } // if
    } // if

    // Restore old value.
    regw (BIOS_0_SCRATCH, save_value);

    return (flag);

} // Radeon_RegTest


/****************************************************************************
 * Radeon_GetBPPValue                                                         *
 *  Function: Get destination bpp format value based on bpp.                *
 *    Inputs: bpp - bit per pixel count                                     *
 *   Outputs: destination bpp format value                                  *
 ****************************************************************************/

DWORD Radeon_GetBPPValue (WORD bpp)
{
    switch (bpp)
    {
        case 8:     return (DST_8BPP);
                    break;
        case 15:    return (DST_15BPP);
                    break;
        case 16:    return (DST_16BPP);
                    break;
        case 32:    return (DST_32BPP);
                    break;
        default:    return (0);
                    break;
    }

    return (0);
}   // Radeon_GetBPPValue


/****************************************************************************
 * Radeon_GetBPPFromValue                                                     *
 *  Function: Get bpp count based on destination bpp format.                *
 *    Inputs: value - destination bpp format                                *
 *   Outputs: bit per pixel count                                           *
 ****************************************************************************/

WORD Radeon_GetBPPFromValue (DWORD value)
{

    switch (value)
    {
        case DST_8BPP:  return 8;
                        break;
        case DST_15BPP: return 15;
                        break;
        case DST_16BPP:
        case DST_16BPP_YVYU422:
        case DST_16BPP_VYUY422: return 16;
                                break;
        case DST_32BPP: return 32;
                        break;
        default:        return 0;
                        break;
    }

}   // Radeon_GetBPPFromValue ()


/****************************************************************************
 * Radeon_Delay                                                               *
 *  Function: Delay for given number of ticks.                              *
 *    Inputs: ticks - count of ticks to delay                               *
 *   Outputs: none                                                          *
 ****************************************************************************/

void Radeon_Delay (WORD ticks)
{
    WORD i;
    DWORD start, end;

    for (i=0;i<ticks;i++)
    {
        start = *((WORD *)(DOS_TICK_ADDRESS));
        end = start;
        while (end == start)
        {
            end = *((WORD *)(DOS_TICK_ADDRESS));
        }
    }
    return;
} // Radeon_Delay ()

#endif

