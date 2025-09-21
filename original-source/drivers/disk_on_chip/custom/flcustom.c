/*
 * $Log:   V:/Flite/archives/OSAK/custom/FLCUSTOM.C_V  $
 * 
 *    Rev 1.1   Jun 06 1999 20:28:52   marinak
 * return status in flRegisterComponents
 * 
 *    Rev 1.0   May 23 1999 13:12:16   marinak
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/

/*	Customization for BeOS. Copyright (c) Be Inc. 2000	*/

#include "stdcomp.h"


/* environment variables */
#ifdef ENVIRONMENT_VARS

unsigned char flUse8Bit;
unsigned char flUseNFTLCache;
unsigned char flUseisRAM;
/*-----------------------------------------------------------------------*/
/*                 f l s e t E n v V a r                                 */
/*  Sets the value of all env variables                                  */
/*  Parameters : None                                                    */
/*-----------------------------------------------------------------------*/
void flSetEnvVar(void)
{
 flUse8Bit=0;
 flUseNFTLCache=1;
 flUseisRAM=1;
}

#endif



/*----------------------------------------------------------------------*/
/*      	  f l R e g i s t e r C o m p o n e n t s		*/
/*									*/
/* Register socket, MTD and translation layer components for use	*/
/*									*/
/* This function is called by FLite once only, at initialization of the */
/* FLite system.							*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus flRegisterComponents(void)
{
  /* Registering socket interface */
  #define LOW_ADDR	0xC0000L
  #define HIGH_ADDR	0xF0000L

  checkStatus(flRegisterDOCSOC(LOW_ADDR, HIGH_ADDR));	/* Register DiskOnChip socket interface */

  /* Registering MTD */
  checkStatus(flRegisterDOC2000());	/* Register diskonc.c MTD */

  /* Registering translation layer */
  checkStatus(flRegisterNFTL());	/* Register nftllite.c */

  return flOK;
}

void* physicalToPointer(unsigned long physical, size_t size_unused, unsigned drive_unused)
{
	static uint8* phys_mem_start = (uint8*)0xFFFFFFFF;
	
	if(phys_mem_start == (uint8*)0xFFFFFFFF)
	{
		area_id		ram;
		area_info	ram_info;
		
		phys_mem_start = 0;
		
		ram = find_area("physical_ram");
		if(ram == B_NAME_NOT_FOUND )
		{
			kernel_debugger("Can't find physical_ram area\n");
		}	
		
		get_area_info(ram, &ram_info); 
		phys_mem_start = (uint8*)ram_info.address;
	}

	return (phys_mem_start + physical);	
}
