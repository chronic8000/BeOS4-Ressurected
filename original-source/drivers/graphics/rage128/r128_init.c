//////////////////////////////////////////////////////////////////////////////
// Initialization Code : r128_init.c
//
//    This file contains various initialization functions required by the
// generic kernel driver.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#include <KernelExport.h>
#include <stdio.h>
#include <string.h>


#include <common_includes.h>
// HACK - Make a header file that bundles these, just as with the accelerant.
#include <kernel_driver_private.h>
#include <driver_hardware_specific.h>
#include <hardware_specific_driver_macros.h>

//////////////////////////////////////////////////////////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
// Defines to be moved to Hardware_specific_macros..
////////////////////////////////////////////////////////////
// ATI specific ROM signiture data 
#define ATI_PRODUCT_SIGNATURE  "761295520"
#define ATI_MACH64_STRING "MACH64"
#define ATI_RAGE128_STRING "R128"
// PCI register addresses for ATI Card.
#define R128_PCI_FB 0
#define R128_PCI_IO 1
#define R128_PCI_CR 2
#define R128_PCI_ROM 8


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Aperture Pointer Extraction Function
//    This function extracts frame buffer and register aperture pointers from
// PCI configuration space. This usually follows standard conventions, but not
// always, so it has to be considered hardware-specific.
//    The pointers are physical addresses. The generic skeleton will map them
// into virtual address space.
//    A NULL register pointer will be handled gracefully. This might occur if
// a particular card doesn't use memory mapped registers.

void GetCardPointers(pci_info *pcii, void **fb_pointer, uint32 *fb_size,
  void **reg_pointer, uint32 *reg_size)
{
  // We have to return a pointer to _something_ for the frame buffer.

  // MESSY HACK - For now, point to East Hyperspace and hope that we never try
  // to write to whatever the heck is out there. This _will_ cause problems
  // when RAM counting is brought into the generic driver. The Right Way to
  // solve this problem is to dynamically allocate a contiguous region of
  // locked memory and return a physical pointer to it. We'll have to modify
  // the RAM counting routines slightly to avoid walking off the end of the
  // mapped area (i.e. store the size of the region mapped in a SHARED_INFO
  // field and check it).


  // PCI card memory pointers sourced from ati_r4
  // references ATI Rage128 programming guide : sections 3.2.4
  // default: frame buffer in [0], control regs in [1]
  //

   long lTmp ;
  
	// read and filter frame buffer address from PCI info 
	lTmp = pcii->u.h0.base_registers[R128_PCI_FB] & 0xFC000000;
	*fb_pointer = (void *) lTmp; 	
	// get frame buffer size
	*fb_size = pcii->u.h0.base_register_sizes[R128_PCI_FB] ;
	ddprintf(("R128:FB : %08lx:%lx \n",lTmp, *fb_size));

	// read and filter register aperature address
	lTmp = pcii->u.h0.base_registers[R128_PCI_CR] & 0xFFFFC000;
	*reg_pointer = (void *)lTmp;	
	// get register aperature size
	*reg_size = pcii->u.h0.base_register_sizes[R128_PCI_CR] ;	
	ddprintf(("R128:REG: %08lx:%lx\n",lTmp, *reg_size));

	// debug of other addresses
	ddprintf(("R128:IO : %08lx:%lx\n",
				pcii->u.h0.base_registers[R128_PCI_IO],
				pcii->u.h0.base_register_sizes[R128_PCI_IO])) ;
}

////////////////////////////////////////////////////////////////
// map_bios()
// maps the ATI graphic card bios into linear memeory
// and sets pointer in the user defined address space.
// see: hardware_specific_common_macros for defintions.
//
int32 map_bios(DEVICE_INFO *di)
{	int32 retval = B_OK;
	char buffer[B_OS_NAME_LENGTH];
  
	SHARED_INFO *si = di->si;
	ddprintf(("[R128: map_bios()] \n"));


// default ROM addresses accress
#define ATI_ROM_ADDRESS 0x000c0000
#define ATI_ROM_SIZE    0x00020000

    sprintf(buffer,
            "%04X_%04X_%02X%02X%02X rom",
            di->pcii.vendor_id,
            di->pcii.device_id,
            di->pcii.bus,
            di->pcii.device,
            di->pcii.function);

	/* try the motherboard BIOS location */
	si->hw_specific.rom_area = map_physical_memory(
			buffer,
			(void *) ATI_ROM_ADDRESS /* di->pcii.u.h0.rom_base */,
			ATI_ROM_SIZE,
			B_ANY_KERNEL_ADDRESS,
			B_READ_AREA,
			(void **)&(si->hw_specific.rom_ptr));
						
	/* return the error if there was some problem */
	if (si->hw_specific.rom_area < 0) {
			retval = B_ERROR;
	}

	/* check for it here */
	if (!Bios_scan(si)) {
			/* oops, not the right ROM on the motherboard either */
			delete_area(si->hw_specific.rom_area);
			si->hw_specific.rom_area = -1;
			return B_ERROR;
	}
	return retval;
}

//////////////////////////////////////////////////////////////////////
// unmap_bios()
// unmaps the ATI Rom BIOS from linear memory space, and releases
// its resoureces.
//
int32 unmap_bios(DEVICE_INFO *di)
{	int32 retval = B_OK; 
	SHARED_INFO *si = di->si;

	ddprintf(("[R128: unmap_bios()] \n"));
	
	delete_area(si->hw_specific.rom_area);
	si->hw_specific.rom_area = -1;

	return retval;	
}

////////////////////////////////////////////////////////////////////
// Bios_scan
// This function is called following successful mapping of the 
// BIOS rom on the ATI card.   Its purpose is to validate the
// ROM signitures, and to extract and set the SHARED_INFO
// clock frequeny.   This logic will be moved to a kernel
// level IOCTL in the future.
//
int32 Bios_scan( SHARED_INFO *si)
{	int32 retval = B_OK;
	int iJ;

	unsigned char	*CrystalPtr;	
	unsigned long	LTemp ;


	unsigned char *ROMPtr = (unsigned char*)si->hw_specific.rom_ptr;

	ddprintf(("[GR128 Bios_scan] ROMPtr = 0x%08lx\n", ROMPtr));
	
	if ((ROMPtr[0] != 0x55) || (ROMPtr[1] != 0xAA))
	{
		/* Diagnostics */
		ddprintf((" - Didn't find 0xAA55. (IsOurROM)\n"));

		return B_ERROR;
	}

	iJ = 0;
	while ((iJ < 256) && (strncmp((char *) (&(ROMPtr[iJ])), ATI_PRODUCT_SIGNATURE, 9) != 0))
	{
		iJ++;
	}
	if (iJ >= 256)
	{
		/* Diagnostics */
		ddprintf((" - Didn't find '761295520'. (IsOurROM)\n"));

		return B_ERROR;
	}

	iJ = 0;
	while ((iJ < 1024) && (strncmp((char *) (&(ROMPtr[iJ])), ATI_RAGE128_STRING, 4) != 0))
	{
		iJ++;
	}
	
	if (iJ >= 1024)
	{	ddprintf((" - Didn't find '%s'(IsOurROM)\n", ATI_RAGE128_STRING));
		return B_ERROR;		
	}

	/* Diagnostics */
	ddprintf((" - ROM looks ok. (IsOurROM)\n"));

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
	ddprintf(("[R128:Bios_scan() ] 1: %08x \n",LTemp ));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x30));
	ddprintf(("[R128:Bios_scan() ] 2: %08x \n",LTemp ));
	LTemp = *((uint16 *) (CrystalPtr + LTemp + 0x0e));	
	ddprintf(("[R128:Bios_scan() ] 3: %08x \n",LTemp ));


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
	ddprintf(("[R128 Bios_scan] Crystal frequency autodetected as %ld Hz.\n", (int32)(si->hw_specific.CrystalFreq) ));


	return retval;
}

//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
