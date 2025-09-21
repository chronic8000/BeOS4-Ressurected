#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <stdio.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>

#include "driver.h"

static int32 Bios_scan (DeviceInfo * di);
static uint8 Radeon_FindRom (DeviceInfo *di);
static void Radeon_GetPLLInfo( DeviceInfo *di );


////////////////////////////////////////////////////////////////
// map_bios()
// maps the ATI graphic card bios into linear memeory
// and sets pointer in the user defined address space.
// see: hardware_specific_common_macros for defintions.
//
static int32 map_bios (DeviceInfo * di)
{
	int32 retval = B_OK;
	char buffer[B_OS_NAME_LENGTH];

	dprintf(("Radeon: map_bios \n"));

// default ROM addresses accress
#define ATI_ROM_ADDRESS 0x000c0000
#define ATI_ROM_SIZE    0x00020000

	sprintf (buffer, "%04X_%04X_%02X%02X%02X rom", di->PCI.vendor_id,
				di->PCI.device_id, di->PCI.bus, di->PCI.device, di->PCI.function);


dprintf(("Radeon: hard bios addr = 0x%x \n", ATI_ROM_ADDRESS));
dprintf(("Radeon: soft bios addr = 0x%x \n", di->PCI.u.h0.rom_base));

	/* try the motherboard BIOS location */
	di->AreaBios = map_physical_memory (buffer, (void *) ATI_ROM_ADDRESS /*di->PCI.u.h0.rom_base*/,
										ATI_ROM_SIZE, B_ANY_KERNEL_ADDRESS, B_READ_AREA,
										(void **) &(di->bios));

	/* return the error if there was some problem */
	if ( di->AreaBios < 0)
	{
		retval = B_ERROR;
	}

	/* check for it here */
	if (!Radeon_FindRom (di))
	{
		/* oops, not the right ROM on the motherboard either */
		delete_area (di->AreaBios);
		di->AreaBios = -1;
		return B_ERROR;
	}
	return retval;
}

//////////////////////////////////////////////////////////////////////
// unmap_bios()
// unmaps the ATI Rom BIOS from linear memory space, and releases
// its resoureces.
//
static int32 unmap_bios (DeviceInfo * di)
{
	dprintf(("Radeon: unmap_bios \n"));
	delete_area (di->AreaBios);
	di->AreaBios = -1;
	di->bios = 0;
	di->biosInfo = 0;
	return B_OK;
}

uint8 Radeon_TestBios( DeviceInfo *di )
{
	dprintf(("Radeon: Radeon_TestBios \n"));
	if( map_bios( di ) >= B_OK )
	{
		// Extract usefull info here.
		Radeon_GetPLLInfo( di );
		
		dprintf(("Radeon: Radeon_TestBios OK\n"));
		unmap_bios( di );
		return 1;
	}
	dprintf(("Radeon: Radeon_TestBios Failed !\n"));
	return 0;
}

/****************************************************************************
 * Radeon_FindRom                                                             *
 *  Function: Find ATI ROM segment.                                         *
 *    Inputs: none                                                          *
 *   Outputs: 1 - chip detected                                             *
 *            0 - chip not detected                                         *
 ****************************************************************************/

static uint8 Radeon_FindRom ( DeviceInfo *di )
{
	uint32 segstart;                     // ROM start address
	uint8 *rom_base;                     // base of ROM segment
	uint8 *rom;                          // offset pointer
	int32 stage;                          // stage 4 - done all 3 detections
	int32 i;                              // loop counter
	uint8 ati_rom_sig[] = "761295520";   // ATI signature
	uint8 Radeon_sig[] = "RG6";          // Radeon signature
    uint8 M6_sig[] = "M6";          // Radeon signature
    uint8 RV100_sig[] = "RV100";          // Radeon signature
	uint8 flag = 0;                      // assume Radeon not found

	dprintf(("Radeon: Radeon_FindRom \n"));
	
	// Traverse ROM, looking at every 4 KB segment for ROM ID (stage 1).
	for (segstart = 0x000C0000; segstart < 0x000F0000; segstart += 0x00001000)
	{
		dprintf(("Radeon: Radeon_FindRom segstart = 0x%x\n", segstart ));
		
		stage = 1;
		rom_base = di->bios + (segstart - 0xC0000);
		if ((*rom_base == 0x55) && (*(rom_base + 1) == 0xAA))
		{
			stage = 2;                  // ROM found
		}
		if (stage != 2)
			continue;       // ROM ID not found.
		rom = rom_base;

		dprintf(("Radeon: Radeon_FindRom Found ID \n" ));
		
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
		if (stage != 3)
			continue;       // ATI signature not found.
		rom = rom_base;

		dprintf(("Radeon: Radeon_FindRom Found ATI signature \n" ));
		
		// Search for Radeon signature (stage 3).
		for (i = 0; (i < 512) && (stage != 4); i++)
		{
			if ((Radeon_sig[0] == *rom) ||
				(M6_sig[0] == * rom) ||
				(RV100_sig[0] == * rom))
			{
				if ((strncmp (Radeon_sig, rom, strlen (Radeon_sig)) == 0)
					|| (strncmp (M6_sig, rom, strlen (M6_sig)) == 0)
					|| (strncmp (RV100_sig, rom, strlen (RV100_sig)) == 0))
				{
					stage = 4;          // Found Radeon signature.
				} // if
			} // if
			rom++;
		} // for
		if (stage != 4)
			continue;       // Radeon signature not found.

		dprintf(("Radeon: Radeon_FindRom Found Radeon signature \n" ));
		
		// Successful! (stage 4)
		//fprintf (op, "\nRadeon BIOS located at segment %4.4X\n", rom_base);
		// Fill in AdapterInfo struct accordingly
		
		di->biosInfo = rom_base;
		flag = 1;                       // Return value set to successful.
		break;
	} // for
	
	return (flag);
	
} // Radeon_FindRom

static void Radeon_GetPLLInfo( DeviceInfo *di )
{
	uint32 bios_header;
	uint32 *header_ptr;
	uint16 bios_header_offset, pll_info_offset;
	
	// The pointer to the PLL info block is defined as being located at byte offset
	// 30-31h in the BIOS header.  A pointer to the BIOS header itself is
	// located at 48h from the BIOS segment address.
	
	// First, let's get the pointer to the BIOS header.
	bios_header = (uint32)(di->biosInfo + 0x48);
	
	// convert the value into a pointer
	header_ptr = (uint32 *)bios_header;
	// get the WORD value that is pointed to by the pointer
	bios_header_offset = (uint16)*header_ptr;
	
	// Let's create a pointer to BIOS header location
	bios_header = (uint32)(di->biosInfo + bios_header_offset);
	
	// The pointer to the PLL info block is at offset 30h within the BIOS header
	bios_header += 0x30;
	
	// Get the actual pointer value (offset)
	header_ptr = (uint32 *)bios_header;
	pll_info_offset = (uint16)*header_ptr;
	
	header_ptr = (uint32 *) (di->biosInfo + (uint32)pll_info_offset);

dprintf(("Radeon: sizeof( PLL_BLOCK ) %l \n", sizeof( PLL_BLOCK ) ));

	// copy data into PLL_BLOCK
	memcpy ( &di->pll, header_ptr, 50);

#if 1
	dprintf(("Radeon: clock_chip_type 0x%x \n", di->pll.clock_chip_type ));
	dprintf(("Radeon: struct_size 0x%x \n", di->pll.struct_size ));
	dprintf(("Radeon: acclerator_entry 0x%x \n", di->pll.acclerator_entry ));
	dprintf(("Radeon: VGA_entry 0x%x \n", di->pll.VGA_entry ));
	dprintf(("Radeon: VGA_table_offset 0x%x \n", di->pll.VGA_table_offset ));
	dprintf(("Radeon: POST_table_offset 0x%x \n", di->pll.POST_table_offset ));
	dprintf(("Radeon: XCLK 0x%x \n", di->pll.XCLK ));
	dprintf(("Radeon: MCLK 0x%x \n", di->pll.MCLK ));
	dprintf(("Radeon: num_PLL_blocks 0x%x \n", di->pll.num_PLL_blocks ));
	dprintf(("Radeon: size_PLL_blocks 0x%x \n", di->pll.size_PLL_blocks ));
	dprintf(("Radeon: PCLK_ref_freq 0x%x \n", di->pll.PCLK_ref_freq ));
	dprintf(("Radeon: PCLK_ref_divider 0x%x \n", di->pll.PCLK_ref_divider ));
	dprintf(("Radeon: PCLK_min_freq 0x%x \n", di->pll.PCLK_min_freq ));
	dprintf(("Radeon: PCLK_max_freq 0x%x \n", di->pll.PCLK_max_freq ));
	dprintf(("Radeon: MCLK_ref_freq 0x%x \n", di->pll.MCLK_ref_freq ));
	dprintf(("Radeon: MCLK_ref_divider 0x%x \n", di->pll.MCLK_ref_divider ));
	dprintf(("Radeon: MCLK_min_freq 0x%x \n", di->pll.MCLK_min_freq ));
	dprintf(("Radeon: MCLK_max_freq 0x%x \n", di->pll.MCLK_max_freq ));
	dprintf(("Radeon: XCLK_ref_freq 0x%x \n", di->pll.XCLK_ref_freq ));
	dprintf(("Radeon: XCLK_ref_divider 0x%x \n", di->pll.XCLK_ref_divider ));
	dprintf(("Radeon: XCLK_min_freq 0x%x \n", di->pll.XCLK_min_freq ));
	dprintf(("Radeon: XCLK_max_freq 0x%x \n", di->pll.XCLK_max_freq ));
#endif
}


////////////////////////////////////////////////////////////////////
// Bios_scan
// This function is called following successful mapping of the 
// BIOS rom on the ATI card.   Its purpose is to validate the
// ROM signitures, and to extract and set the SHARED_INFO
// clock frequeny.   This logic will be moved to a kernel
// level IOCTL in the future.
//
#if 0
static int32 Bios_scan (DeviceInfo * di)
{
	int32 retval = B_OK;
	int32 iJ;
	uint8 *CrystalPtr;
	uint32 LTemp;
	uint8 *ROMPtr = di->bios;
    char ati_rom_sig[] = "761295520";   // ATI signature
    char Radeon_sig[] = "RG6";          // Radeon signature

	dprintf(("Radeon: Bios_scan ROMPtr = 0x%08lx\n", ROMPtr));

	/* If we reach here, then the ROM looks ok. */

	// while we are here, we need to get the Rage128 clock frequency
	// from 3.3.2.2 in the Rage128 programming guide the
	// following is a description of how to find the frequency
	// get the BIOS header, at offset 48h. within this table
	// at offset 30-31h there is a pointer to the clock frequency.
	// this contains a pointer to the crlock frequency in 10khz.
	// it is either 2950, 2863, or 1432

	CrystalPtr = (unsigned char *) ROMPtr;
	LTemp = *((uint16 *) (CrystalPtr + 0x48));
	dprintf(("[R128:Bios_scan() ] 1: %08x \n", LTemp));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x30));
	dprintf(("[R128:Bios_scan() ] 2: %08x \n", LTemp));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x0e));
	dprintf(("[R128:Bios_scan() ] 3: %08x \n", LTemp));

	// According to ATI, there are only three values that will show up here.
	// If we recognize these, store them at full precision. Otherwise, take
	// the 3-4 digits we're given.

	switch (LTemp)
	{
	case 1432:
		si->hw_specific.CrystalFreq = 14.318;
		break;
	case 2860:
		si->hw_specific.CrystalFreq = 28.636;
		break;
	case 2950:
		si->hw_specific.CrystalFreq = 29.498;
		break;
	default:
		si->hw_specific.CrystalFreq = LTemp * 0.01;
	}

// Diagnostics.
	dprintf (
				 ("[R128 Bios_scan] Crystal frequency autodetected as %ld Hz.\n",
				  (int32) (si->hw_specific.CrystalFreq)));

	return retval;
}

#endif

