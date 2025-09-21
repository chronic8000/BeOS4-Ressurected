/* $Header:
** Copyright (c) 1995-1999, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
** File name:   sliaa.c
**
** Description: Routines to Initialize SLI AA for Napalm.
**
** $Revision: 16$
** $Date: 4/21/00 3:30:49 PM$
**
** $History: sliaa.c $
** 
** *****************  Version 10  *****************
** User: Cwilcox      Date: 9/09/99    Time: 5:05p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Fixed lfbMemoryConfig accesses to support 32Mb and 64Mb configs.
** 
** *****************  Version 9  *****************
** User: Andrew       Date: 8/24/99    Time: 5:44p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Fixed a problem with SLICTRL and 4-way SLI
** 
** *****************  Version 8  *****************
** User: Andrew       Date: 8/20/99    Time: 11:43a
** Updated in $/devel/h5/Win9x/dx/minivdd
** Minor change to disable aa when SLI and AA are disabled.
** 
** *****************  Version 7  *****************
** User: Andrew       Date: 7/30/99    Time: 2:00p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Fixed a bug with rendermask and SLI copies
** 
** *****************  Version 6  *****************
** User: Andrew       Date: 7/28/99    Time: 3:46p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Fixes to support SLI on WINSIM
** 
** *****************  Version 5  *****************
** User: Andrew       Date: 7/26/99    Time: 5:53p
** Updated in $/devel/h5/Win9x/dx/minivdd
** fixed some bugs and added code to setup the fb for entering SLI mode
** and leaving sli mode
** 
** *****************  Version 4  *****************
** User: Andrew       Date: 7/20/99    Time: 10:57a
** Updated in $/devel/h5/Win9x/dx/minivdd
** code to init SLICTRL
** 
** *****************  Version 3  *****************
** User: Andrew       Date: 7/16/99    Time: 2:06p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Changed regBase and RegBase from single uint32 to array to support
** sparse register mapping
** 
** *****************  Version 2  *****************
** User: Andrew       Date: 7/07/99    Time: 2:43p
** Updated in $/devel/h5/Win9x/dx/minivdd
** Dibengine workaround
** 
** *****************  Version 1  *****************
** User: Andrew       Date: 6/25/99    Time: 10:05a
** Created in $/devel/h5/Win9x/dx/minivdd
** Code to enable/disable SLI/AA
** 
**
*/

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/common/debug.h>

#include <stdio.h>

#include "driver.h"
#include "sliaa.h"
#include "devtable.h"

uint32 our_mem_rdd (uint32);
void our_mem_wrd (uint32, uint32);
void our_memcpy (uint8 * pDest, uint8 * pSrc, uint32 dwSize);

void PCI_Write_Config (uint32 dwBus, uint32 dwDevFunc, uint32 dwOffset, uint32 dwValue)
{
}

#define PCI_CFG_WR( offset, val, chip ) (*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, (chip)-1, (offset), 4, (val) )
#define PCI_CFG_RD( offset, chip ) ((*_V5_dd.pci_bus->read_pci_config) ( di->PCI.bus, di->PCI.device, (chip)-1, (offset), 4 ))

#define PCI_IO_WR( offset, val ) (*_V5_dd.pci_bus->write_io_32) ( (offset), (val) )
#define PCI_IO_RD( offset )  (*_V5_dd.pci_bus->read_io_32) ( (offset) )

uint32 PCI_Read_Config (uint32 bus, uint32 func, uint32 offset)
{
//      return (*pci_bus->read_pci_config)( di->PCI.bus, di->PCI.device, func, offset, 4 );
	return 0;
}

// There is a bug in A0 silcon wrt SLI mode.  If a read abort <a unowned read>
// is done by the master or slave when the CMDFIFO is running then the CMDFIFO will
// get hosed.  A unowned read is (A) Master -- read of unowned Tile; (B) Slave -- read of unowned Tile
// or linear memory.  To get around this we run with SLI_LFB_RD_EN disabled.

#undef MIN
#define MIN(A,B) ((A) <= (B) ? (A) : (B))

#define SST_PCI_FORCE_FB_HIGH (1<<26)

/*----------------------------------------------------------------------
Function name:  H3_DISABLE_SLI_AA

Description:    Disables SLI and AA on Napalm

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
void H3_DISABLE_SLI_AA (struct devinfo *di)
{
	thdfx_card_info *ci = di->ci;

	uint32 i;
	uint32 h3ChipSetupIoBase = di->info_io_base;
//	uint32 h3ChipSetup3DBase = (uint32) ci->BaseAddr0;
	uint32 h3ChipSetupDeviceNum;

	for (i = 1; i <= di->info_Chips; i++)
	{
		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i-1, 4, 2, 3 );		// Enabled IO access
		
//              h3ChipSetupIoBase = pDevTable->RegBase[HWINFO_SST_IOREGS_INDEX];
//              h3ChipSetup3DBase = pDevTable->RegBase[HWINFO_SST_3DREGS_INDEX];

//              if (SLI_AA_MASTER_DEVICE != pDevTable->Type)
		h3ChipSetupDeviceNum = i;

		PCI_CFG_WR (CFG_INIT_ENABLE,
						(PCI_CFG_RD (CFG_INIT_ENABLE, h3ChipSetupDeviceNum) &
						 ~(CFG_SNOOP_MEMBASE0 | CFG_SNOOP_EN | CFG_SNOOP_MEMBASE0_EN
							| CFG_SNOOP_MEMBASE1_EN | CFG_SNOOP_SLAVE | CFG_SNOOP_FBIINIT_WR_EN |
							CFG_SWAP_ALGORITHM | CFG_SWAP_QUICK)), h3ChipSetupDeviceNum);

		PCI_CFG_WR (CFG_SLI_LFB_CTRL,
						(PCI_CFG_RD (CFG_SLI_LFB_CTRL, h3ChipSetupDeviceNum) &
						 ~(CFG_SLI_LFB_CPU_WR_EN | CFG_SLI_LFB_DPTCH_WR_EN | CFG_SLI_RD_EN)),
						h3ChipSetupDeviceNum);

//		PCI_IO_WR (h3ChipSetup3DBase + SLICTRL, 0x0);
//		PCI_IO_WR (h3ChipSetup3DBase + AACTRL, 0x0);

		PCI_CFG_WR (CFG_AA_LFB_CTRL,
						(PCI_CFG_RD (CFG_AA_LFB_CTRL, h3ChipSetupDeviceNum) &
						 ~(CFG_AA_LFB_CPU_WR_EN | CFG_AA_LFB_DPTCH_WR_EN | CFG_AA_LFB_RD_EN)),
						h3ChipSetupDeviceNum);

		PCI_CFG_WR (CFG_SLI_AA_MISC,
						((PCI_CFG_RD (CFG_SLI_AA_MISC, h3ChipSetupDeviceNum) &
						  ~CFG_VGA_VSYNC_OFFSET) | (0x0 << CFG_VGA_VSYNC_OFFSET_PIXELS_SHIFT) | (0x0 <<
																														 CFG_VGA_VSYNC_OFFSET_CHARS_SHIFT)
						 | (0x0 << CFG_VGA_VSYNC_OFFSET_HXTRA_SHIFT)), h3ChipSetupDeviceNum);

		// Tristate slave Hsync and Vsync's       
		if (1 != i)
		{
			PCI_CFG_WR (CFG_VIDEO_CTRL0, CFG_DAC_VSYNC_TRISTATE | CFG_DAC_HSYNC_TRISTATE,
							h3ChipSetupDeviceNum);
		}
		else
		{
			PCI_CFG_WR (CFG_VIDEO_CTRL0, 0x0, h3ChipSetupDeviceNum);
		}

		PCI_CFG_WR (CFG_VIDEO_CTRL1, 0x0, h3ChipSetupDeviceNum);
		PCI_CFG_WR (CFG_VIDEO_CTRL2, 0x0, h3ChipSetupDeviceNum);

		// This results in a different value then boot and should be verified
		// when we get real hardware      
		if (di->info_PCI)
		{
			if (di->info_Chips > 1)
			{
				PCI_IO_WR (h3ChipSetupIoBase + PCIINIT0,
							((PCI_IO_RD (h3ChipSetupIoBase + PCIINIT0) &
							~(SST_PCI_DISABLE_IO | SST_PCI_DISABLE_MEM |
							SST_PCI_RETRY_INTERVAL)) | (0 << SST_PCI_RETRY_INTERVAL_SHIFT) |
							SST_PCI_FORCE_FB_HIGH));
			}
			else
			{
				PCI_IO_WR (h3ChipSetupIoBase + PCIINIT0,
							  ((PCI_IO_RD
								 (h3ChipSetupIoBase +
								  PCIINIT0) & ~(SST_PCI_DISABLE_IO | SST_PCI_DISABLE_MEM |
													 SST_PCI_RETRY_INTERVAL)) | (0 <<
																						  SST_PCI_RETRY_INTERVAL_SHIFT)));
			}
		}

		// Kill the slave since the DAC are tied together
		if (i > 1)
		{
			PCI_IO_WR (h3ChipSetupIoBase + DACMODE, SST_DAC_DPMS_ON_VSYNC | SST_DAC_DPMS_ON_HSYNC);
			PCI_IO_WR (h3ChipSetupIoBase + VIDPROCCFG,
						  (PCI_IO_RD (h3ChipSetupIoBase + VIDPROCCFG) & ~SST_VIDEO_PROCESSOR_EN));
		}

		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i-1, 4, 2, 0 );		// Disable IO access
	}
	(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, 0, 4, 2, 3 );		// Enable IO access

	return;
}

/*----------------------------------------------------------------------
Function name:  H3_SETUP_SLI_AA

Description:    Sets up SLI and AA on Napalm

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
void GetStrideAndAperature (uint32 lpDriverData, uint32 * pStrideAndAperature);

void H3_SETUP_SLI_AA (struct devinfo *di, SLI_AA_INFO * pInfo )
	// nChips: Number of chips in multi-chip configuration (1-4)
	// sliEn: Sli is to be enabled (0,1)
	// aaEn: Anti-aliasing is to be enabled (0,1)
	// aaSampleHigh: 0->Enable 2-sample AA, 1->Enable 4-sample AA
	// sliAaAnalog: 0->Enable digital SLI/AA, 1->Enable analog Sli/AA
	// sli_nLines: Number of lines owned by each chip in SLI (2-128)
	// fbMem: Amount of memory, in megabytes...
{
	thdfx_card_info *ci = di->ci;

	uint32 sli_renderMask, sli_compareMask, sli_scanMask;
	uint32 sli_nLinesLog2 = 1, nChipsLog2 = 1;
	uint32 i;
	uint32 aaClkOutDel;
	uint32 dwStrideAndAperature;
	uint32 MEMBASE0;
	uint32 MEMBASE1;
	uint32 h3ChipSetupIoBase = di->info_io_base;
//	uint32 h3ChipSetup3DBase = (uint32) ci->BaseAddr0;
	uint32 h3ChipSetupDeviceNum;
	uint32 dwFormat;
	uint32 temp1;
	uint32 temp2;
	uint32 temp3;

	uint32 tileBegin = 0;		  //pMemInfo->TileMark;
// uint32 tileEnd = 0; //pMemInfo->TotalMemory;

	dprintf (("   nChips:%d, sliEn:%d, aaEn:%d, aaSampleHigh:%d\n", di->info_Chips, pInfo->sliEn, pInfo->aaEn, pInfo->aaSampleHigh));
	dprintf (("   sliAaAnalog:%d, sli_nLines:%d\n", pInfo->sliAaAnalog, pInfo->sli_nlines));

	//GetStrideAndAperature(((PDEVTABLE)pInfo->pDevTable)->lpDriverData, &dwStrideAndAperature);

	// Some sanity checking...
	if ((di->info_Chips == 0) || (di->info_Chips == 3) || (di->info_Chips > 4)
		 || (di->info_Chips == 1 && (pInfo->sliEn || pInfo->aaSampleHigh))
		 || (di->info_Chips == 2 && (pInfo->sliEn && pInfo->aaEn && pInfo->aaSampleHigh)))
	{
		dprintf (("H3_SETUP_SLI_AA() ERROR: Unsupported input params...\n"));
	}

	switch (pInfo->sli_nlines)
	{
	case 2:
		sli_nLinesLog2 = 1;
		break;
	case 4:
		sli_nLinesLog2 = 2;
		break;
	case 8:
		sli_nLinesLog2 = 3;
		break;
	case 16:
		sli_nLinesLog2 = 4;
		break;
	case 32:
		sli_nLinesLog2 = 5;
		break;
	case 64:
		sli_nLinesLog2 = 6;
		break;
	case 128:
		sli_nLinesLog2 = 7;
		break;
	default:
		dprintf (("H3_SETUP_SLI_AA() ERROR: Unsupported sli_nLines=%d...\n", pInfo->sli_nlines));
		break;
	}
	switch (di->info_Chips)
	{
	case 1:
		nChipsLog2 = 0;
		break;
	case 2:
		nChipsLog2 = 1;
		break;
	case 4:
		nChipsLog2 = 2;
		break;
	case 8:
		nChipsLog2 = 3;
		break;
	default:
		dprintf (("H3_SETUP_SLI_AA() ERROR:  Unsupported nChips=%d...\n", di->info_Chips));
		break;
	}

	// Each chip has now been setup.  Now connect them all together with SLI...

	MEMBASE0 = (uint32) ci->BaseAddr0;	//((PDEVTABLE)pInfo->pDevTable)->PhysMemBase[0];
	MEMBASE1 = (uint32) ci->BaseAddr1;	//((PDEVTABLE)pInfo->pDevTable)->PhysMemBase[1];


	temp1 = (*_V5_dd.pci_bus->read_pci_config) ( di->PCI.bus, di->PCI.device, 0, 16, 4 );	// membase 0
	temp2 = (*_V5_dd.pci_bus->read_pci_config) ( di->PCI.bus, di->PCI.device, 0, 20, 4 );	// membase 1
	temp3 = (*_V5_dd.pci_bus->read_pci_config) ( di->PCI.bus, di->PCI.device, 0, 24, 4 );	// iobase
	for (i = 1; i < di->info_Chips; i++)
	{
		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i, 16, 4, temp1 );
		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i, 20, 4, temp2 );
		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i, 24, 4, temp3 );
	}


	for (i = 1; i <= di->info_Chips; i++)
	{
		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i-1, 4, 2, 3 );		// Enabled IO access

		//h3ChipSetupIoBase = pDevTable->RegBase[HWINFO_SST_IOREGS_INDEX];
		//h3ChipSetup3DBase = pDevTable->RegBase[HWINFO_SST_3DREGS_INDEX];

		//if (SLI_AA_MASTER_DEVICE != pDevTable->Type)
		h3ChipSetupDeviceNum = i;

		// Each chip MUST must use 2ws reads and 1ws writes.  Snooping does not
		// work with 0ws writes or 1ws reads...
		// Also, each chip MUST use at least a retry interval of ~12...
		// (0x7 gets added to the timeout interval by the chip automatically...)
		// VIA work better without retries
		//if (FALSE == ((PDEVTABLE) pInfo->pDevTable)->VIACoreLogic)
		{
			PCI_IO_WR (h3ChipSetupIoBase + PCIINIT0, ((PCI_IO_RD
							 (h3ChipSetupIoBase +
							  PCIINIT0) & ~(SST_PCI_RETRY_INTERVAL |
							 SST_PCI_FORCE_FB_HIGH)) | SST_PCI_READ_WS | SST_PCI_WRITE_WS |
							SST_PCI_DISABLE_IO | SST_PCI_DISABLE_MEM | (5 <<
																					  SST_PCI_RETRY_INTERVAL_SHIFT)));
		}
		/*else
		   {
		   PCI_IO_WR (h3ChipSetupIoBase + PCIINIT0,
		   ((PCI_IO_RD
		   (h3ChipSetupIoBase +
		   PCIINIT0) & ~(SST_PCI_RETRY_INTERVAL |
		   SST_PCI_FORCE_FB_HIGH)) |
		   SST_PCI_READ_WS | SST_PCI_WRITE_WS));
		   } */

		// This will have to be set properly once hw comes back...For now, use
		// a value that works for simulation...
		aaClkOutDel = 0x2;
		PCI_IO_WR (h3ChipSetupIoBase + TMUGBEINIT,
					  ((PCI_IO_RD
						 (h3ChipSetupIoBase +
						  TMUGBEINIT) & ~(SST_AA_CLK_DELAY | SST_AA_CLK_INVERT)) | (aaClkOutDel <<
						SST_AA_CLK_DELAY_SHIFT)
						| SST_AA_CLK_INVERT));

		// Setup buffer swapping to use external sli_syncin/sli_syncout signals
		if (di->info_Chips > 1)
		{
			PCI_CFG_WR (CFG_INIT_ENABLE, (PCI_CFG_RD (CFG_INIT_ENABLE, h3ChipSetupDeviceNum) |
							 ((pInfo->CfgSwapAlgorithm & 0x01) << CFG_SWAPBUFFER_ALGORITHM_SHIFT) |
							 CFG_SWAP_ALGORITHM | ((i == 1) ? CFG_SWAP_MASTER : 0x0)),
							h3ChipSetupDeviceNum);
		}

		// Setup snooping...
		if (i == 1 && di->info_Chips > 1)
		{
			PCI_CFG_WR (CFG_INIT_ENABLE, (PCI_CFG_RD (CFG_INIT_ENABLE, h3ChipSetupDeviceNum) | CFG_SNOOP_EN), h3ChipSetupDeviceNum);
		}
		else if (di->info_Chips > 1)
		{
			// For real hardware, MEMBASE0 and MEMBASE1 need to be replaced
			// with the memBaseAddr0 and memBaseAddr1 addresses of the
			// first chip (i.e. the Master chip), respectively.
			// Also, we may not need to run with CFG_SWAP_QUICK with real
			// hardware...
			PCI_CFG_WR (CFG_INIT_ENABLE, ((PCI_CFG_RD (CFG_INIT_ENABLE, h3ChipSetupDeviceNum) &
							  ~CFG_SNOOP_MEMBASE0) | CFG_SNOOP_EN | CFG_SNOOP_MEMBASE0_EN | CFG_SNOOP_MEMBASE1_EN |
							 CFG_SNOOP_SLAVE | CFG_SNOOP_FBIINIT_WR_EN | (((MEMBASE0 >> 22) & 0x3ff) << CFG_SNOOP_MEMBASE0_SHIFT) |
							 ((di->info_Chips > 2) ? CFG_SWAP_QUICK : 0x0)), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_PCI_DECODE, ((PCI_CFG_RD (CFG_PCI_DECODE, h3ChipSetupDeviceNum) & ~CFG_SNOOP_MEMBASE1) |
							 (((MEMBASE1 >> 22) & 0x3ff) << CFG_SNOOP_MEMBASE1_SHIFT)), h3ChipSetupDeviceNum);
		}

		// Setup cfgSliLfbCtrl
		if (pInfo->sliEn && (!pInfo->aaEn || !pInfo->aaSampleHigh))
		{
			sli_renderMask = (di->info_Chips - 1) << sli_nLinesLog2;
			sli_compareMask = (i - 1) << sli_nLinesLog2;
			sli_scanMask = pInfo->sli_nlines - 1;
#ifdef RD_ABORT_ERROR
			PCI_CFG_WR (CFG_SLI_LFB_CTRL,
							((sli_renderMask << CFG_SLI_LFB_RENDERMASK_SHIFT) |
							 (sli_compareMask << CFG_SLI_LFB_COMPAREMASK_SHIFT) |
							 (sli_scanMask << CFG_SLI_LFB_SCANMASK_SHIFT) | (nChipsLog2 <<
																							 CFG_SLI_LFB_NUMCHIPS_LOG2_SHIFT)
							 | CFG_SLI_LFB_CPU_WR_EN | CFG_SLI_LFB_DPTCH_WR_EN), h3ChipSetupDeviceNum);
#else
			PCI_CFG_WR (CFG_SLI_LFB_CTRL,
							((sli_renderMask << CFG_SLI_LFB_RENDERMASK_SHIFT) |
							 (sli_compareMask << CFG_SLI_LFB_COMPAREMASK_SHIFT) |
							 (sli_scanMask << CFG_SLI_LFB_SCANMASK_SHIFT) |
							 (nChipsLog2 << CFG_SLI_LFB_NUMCHIPS_LOG2_SHIFT) | CFG_SLI_LFB_CPU_WR_EN |
							 CFG_SLI_LFB_DPTCH_WR_EN | CFG_SLI_RD_EN), h3ChipSetupDeviceNum);
#endif

//			PCI_IO_WR (h3ChipSetup3DBase + SLICTRL,
//						  ((sli_renderMask << SLICTL_3D_RENDERMASK_SHIFT) |
//							(sli_compareMask << SLICTL_3D_COMPAREMASK_SHIFT) | (sli_scanMask <<
//																								 SLICTL_3D_SCANMASK_SHIFT) |
//							(nChipsLog2 << SLICTL_3D_NUMCHIPS_LOG2_SHIFT) | SLICTL_3D_EN));

		}
		else if (!pInfo->sliEn && pInfo->aaEn)
		{
			sli_renderMask = 0x0;
			sli_compareMask = 0x0;
			sli_scanMask = 0x0;
			PCI_CFG_WR (CFG_SLI_LFB_CTRL, ((sli_renderMask << CFG_SLI_LFB_RENDERMASK_SHIFT) |
							 (sli_compareMask << CFG_SLI_LFB_COMPAREMASK_SHIFT) | (sli_scanMask << CFG_SLI_LFB_SCANMASK_SHIFT)
							 | (0x0 << CFG_SLI_LFB_NUMCHIPS_LOG2_SHIFT)), h3ChipSetupDeviceNum);

//			PCI_IO_WR (h3ChipSetup3DBase + SLICTRL,
//						  ((sli_renderMask << SLICTL_3D_RENDERMASK_SHIFT) |
//							(sli_compareMask << SLICTL_3D_COMPAREMASK_SHIFT) | (sli_scanMask <<
//																								 SLICTL_3D_SCANMASK_SHIFT) |
//							(0x0 << SLICTL_3D_NUMCHIPS_LOG2_SHIFT)));

		}
		else
		{
			// pInfo->sliEn && pInfo->aaEn && pInfo->aaSampleHigh
			sli_renderMask = ((di->info_Chips >> 1) - 1) << sli_nLinesLog2;
			sli_compareMask = ((i - 1) >> 1) << sli_nLinesLog2;
			sli_scanMask = pInfo->sli_nlines - 1;

#ifdef RD_ABORT_ERROR
			PCI_CFG_WR (CFG_SLI_LFB_CTRL, ((sli_renderMask << CFG_SLI_LFB_RENDERMASK_SHIFT) |
							 (sli_compareMask << CFG_SLI_LFB_COMPAREMASK_SHIFT) |
							 (sli_scanMask << CFG_SLI_LFB_SCANMASK_SHIFT) | ((nChipsLog2 - 1) << CFG_SLI_LFB_NUMCHIPS_LOG2_SHIFT)
							 | CFG_SLI_LFB_CPU_WR_EN | CFG_SLI_LFB_DPTCH_WR_EN), h3ChipSetupDeviceNum);
#else
			PCI_CFG_WR (CFG_SLI_LFB_CTRL, ((sli_renderMask << CFG_SLI_LFB_RENDERMASK_SHIFT) |
							 (sli_compareMask << CFG_SLI_LFB_COMPAREMASK_SHIFT) |
							 (sli_scanMask << CFG_SLI_LFB_SCANMASK_SHIFT) |
							 ((nChipsLog2 - 1) << CFG_SLI_LFB_NUMCHIPS_LOG2_SHIFT) | CFG_SLI_LFB_CPU_WR_EN
							 | CFG_SLI_LFB_DPTCH_WR_EN | CFG_SLI_RD_EN), h3ChipSetupDeviceNum);
#endif

//			PCI_IO_WR (h3ChipSetup3DBase + SLICTRL,
//						  ((sli_renderMask << SLICTL_3D_RENDERMASK_SHIFT) |
//							(sli_compareMask << SLICTL_3D_COMPAREMASK_SHIFT) | (sli_scanMask <<
//																								 SLICTL_3D_SCANMASK_SHIFT) |
//							((nChipsLog2 - 1) << SLICTL_3D_NUMCHIPS_LOG2_SHIFT) | SLICTL_3D_EN));

		}
		// Sanity checking...
		if ((sli_renderMask > 255) || (sli_compareMask > 255) || (sli_scanMask > 255))
		{
			dprintf (("H3_SETUP_SLI_AA() ERROR: sli render/compare/scan masks greater than 255...\n"));
		}

		// Setup cfgSliAaTiledAperture
		if (pInfo->sliEn && !pInfo->aaEn)
		{
			// SLI only...

			// By default, setup beginning of SLI/AA tiled aperture to be evenly
			// split between linear and tiled (based on memory size...)
			// Make 1/2 memory size be the split...
			PCI_IO_WR (h3ChipSetupIoBase + LFBMEMORYCONFIG, ((tileBegin >> 12) & 0x1fff) |	// tile aperture base bits(12:0)
						  (((tileBegin >> 25) & 0x0003) << 23) |	// tile aperture base bits(14:13)
						  dwStrideAndAperature |	// number of sgram tiles in X...
						  (0x0 << 31));	// write to lfbMemoryTileCtrl
#if 0
			PCI_IO_WR (h3ChipSetupIoBase + LFBMEMORYCONFIG, ((pMemInfo->TileCmpMark >> 12) & 0x7fff) |	// tile aperture base bits(14:0)
						  (0x1 << 15) |	// Use lfbMemoryTileCompare[14:0] for
						  // tiled/linear comparison
						  (0x1 << 31));	// write to lfbMemoryTileCompare
#endif

#if 0									  //def RD_ABORT_ERROR
			{
				PCI_CFG_WR (CFG_AA_LFB_CTRL, CFG_AA_LFB_RD_EN, h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_AA_ZBUFF_APERTURE,
								(((tileBegin >> 12) << CFG_AA_DEPTH_BUFFER_BEG_SHIFT) |
								 ((tileEnd >> 12) << CFG_AA_DEPTH_BUFFER_END_SHIFT)), h3ChipSetupDeviceNum);
			}
#endif

		}
		else
		{
			// AA is enabled...
			uint32 aaSecondaryBuffersBegin, aaDepthBufferBegin;
			uint32 aaDepthBufferEnd;

			// By default for simulation, setup the memory map as follows:
			// (for a 64MB memory example)
			//
			// +==========================+ 64 MB
			// +                          +
			// +  Secondary depth buffer  +
			// +                          +
			// +==========================+ 56 MB
			// +                          +
			// +  Secondary color buffers +
			// +                          +
			// +==========================+ 40 MB
			// +                          +
			// +  Primary depth buffer    +
			// +                          +
			// +==========================+ 32 MB
			// +                          +
			// +  Primary color buffers   +
			// +                          +
			// +==========================+ 16 MB  <-- Tile memory starts here...
			// +                          +
			// +  Linear memory space     +
			// +                          +
			// +==========================+ 0

			aaSecondaryBuffersBegin = pInfo->aaSecondaryColorBufBegin;
			aaDepthBufferBegin = pInfo->aaSecondaryDepthBufBegin;
			aaDepthBufferEnd = pInfo->aaSecondaryDepthBufEnd;

			if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
				 && !pInfo->aaSampleHigh)
				// Two chips-- 2-sample AA (1 subsampled stored in each chip)
				// Fool the PCI frontend section...the AA reads/writes will still
				// be duplicated to the secondary AA buffers, but the secondary
				// buffer start will point to the primary buffers.  This will,
				// in effect, make it as though we only have one AA buffer...
				aaSecondaryBuffersBegin = tileBegin;

			PCI_IO_WR (h3ChipSetupIoBase + LFBMEMORYCONFIG, ((tileBegin >> 12) & 0x1fff) |	// tile aperture base bits(12:0)
						  (((tileBegin >> 25) & 0x0003) << 23) |	// tile aperture base bits(14:13)
						  dwStrideAndAperature | (0x0 << 31));	// write to lfbMemoryTileCtrl

#if 0
			PCI_IO_WR (h3ChipSetupIoBase + LFBMEMORYCONFIG, ((tileBegin >> 12) & 0x7fff) |	// tile aperture base bits(14:0)
						  (0x1 << 15) |	// Use lfbMemoryTileCompare[14:0] for
						  // tiled/linear comparison
						  (0x1 << 31));	// write to lfbMemoryTileCompare
#endif

			if (15 == pInfo->Bpp)
				dwFormat = CFG_AA_LFB_RD_FORMAT_15BPP;
			else if (32 == pInfo->Bpp)
				dwFormat = CFG_AA_LFB_RD_FORMAT_32BPP;
			else
				dwFormat = CFG_AA_LFB_RD_FORMAT_16BPP;

			if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
				 && !pInfo->aaSampleHigh)
				dwFormat |= CFG_AA_LFB_RD_DIVIDE_BY_4;

			PCI_CFG_WR (CFG_AA_LFB_CTRL,
							((aaSecondaryBuffersBegin << CFG_AA_BASEADDR_SHIFT) |
							 CFG_AA_LFB_CPU_WR_EN | CFG_AA_LFB_DPTCH_WR_EN | CFG_AA_LFB_RD_EN | dwFormat |
							 (pInfo->aaSampleHigh ? CFG_AA_LFB_RD_DIVIDE_BY_4 : 0x0)),
							h3ChipSetupDeviceNum);

			// By default, setup beginning of depth buffer to be at (3/8) * total
			// memory size...
			PCI_CFG_WR (CFG_AA_ZBUFF_APERTURE,
							(((aaDepthBufferBegin >> 12) << CFG_AA_DEPTH_BUFFER_BEG_SHIFT) |
							 ((aaDepthBufferEnd >> 12) << CFG_AA_DEPTH_BUFFER_END_SHIFT)),
							h3ChipSetupDeviceNum);
		}

		// Setup vga_vsync_offset field...
		if (di->info_Chips > 1 && i > 1 && (pInfo->aaEn || pInfo->sliEn))
		{
			uint32 vsyncOffsetPixels, vsyncOffsetChars, vsyncOffsetHXtra;

			if (pInfo->sliAaAnalog
				 || (di->info_Chips == 4 && pInfo->sliEn && pInfo->aaEn
					  && pInfo->aaSampleHigh && !pInfo->sliAaAnalog && i == 3))
			{
				// Handle four chips, 2-way analog SLI with digital 4-sample AA...

				// The desired value for the hardware is actually
				// vsyncOffsetPixels = 7, vsyncOffsetChars = 3, but the
				// vga_crtc_fast module has a bug in it which causes us to
				// have to bump the vsyncOffsetChars field when using
				// vsyncOffsetPixels = 7
				vsyncOffsetPixels = 7;
				vsyncOffsetChars = 4;
				vsyncOffsetHXtra = 0;
			}
			else
			{
				// Run slave 8 clocks ahead...

				// Desired value is vsyncOffsetPixels = 7, vsyncOffsetChars = 4
				// but workaround bug in vga_crtc_fast as explained above...
				vsyncOffsetPixels = 7;
				vsyncOffsetChars = 5;
				vsyncOffsetHXtra = 0;
			}
			PCI_CFG_WR (CFG_SLI_AA_MISC,
							((PCI_CFG_RD (CFG_SLI_AA_MISC, h3ChipSetupDeviceNum) &
							  ~CFG_VGA_VSYNC_OFFSET) |
							 (vsyncOffsetPixels <<
							  CFG_VGA_VSYNC_OFFSET_PIXELS_SHIFT) | (vsyncOffsetChars <<
																				 CFG_VGA_VSYNC_OFFSET_CHARS_SHIFT) |
							 (vsyncOffsetHXtra << CFG_VGA_VSYNC_OFFSET_HXTRA_SHIFT)),
							h3ChipSetupDeviceNum);
		}

		if (di->info_Chips == 1 && pInfo->aaEn)
		{
#if 0
			// Single chip-- 2-sample AA...
			PCI_CFG_WR (CFG_VIDEO_CTRL0,
							(CFG_ENHANCED_VIDEO_EN |
							 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					  CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
							 | CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_VIDEO_CTRL1,
							((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																						CFG_SLI_RENDERMASK_CRT_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_VIDEO_CTRL2,
							((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
							 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);

#endif
		}
		else if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
					&& pInfo->aaSampleHigh && !pInfo->sliAaAnalog)
		{
#if 0
			// Two chips-- 4-sample digital AA...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | CFG_DIVIDE_VIDEO_BY_4),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																							CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Second chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																							CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

#endif
		}
		else if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
					&& pInfo->aaSampleHigh && pInfo->sliAaAnalog)
		{
#if 0
			// Two chips-- 4-sample analog AA...
			if (i == 1)
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_4), h3ChipSetupDeviceNum);
			else
				// Tristate HSYNC since both chips share a common hsync signal
				// (since we want to support analog SLI also...)
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_DAC_HSYNC_TRISTATE |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_4), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_VIDEO_CTRL1,
							((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																						CFG_SLI_RENDERMASK_CRT_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_VIDEO_CTRL2,
							((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);

#endif
		}
		else if (di->info_Chips == 2 && pInfo->sliEn && !pInfo->aaEn
					&& !pInfo->sliAaAnalog)
		{
			// Two chips-- 2-way digital SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0, (CFG_ENHANCED_VIDEO_EN |
								 (CFG_VIDEO_OTHERMUX_SEL_AAFIFO << CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1, (((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2, (((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Second chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0, (CFG_ENHANCED_VIDEO_EN | CFG_ENHANCED_VIDEO_SLV |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
								 
				PCI_CFG_WR (CFG_VIDEO_CTRL1, ((((di->info_Chips - 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								(((i - 1) << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								(0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0xff << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
								
				PCI_CFG_WR (CFG_VIDEO_CTRL2, ((((di->info_Chips - 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								(((i - 1) << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
		}
		else if ((di->info_Chips == 2 || di->info_Chips == 4) && pInfo->sliEn
					&& !pInfo->aaEn && pInfo->sliAaAnalog)
		{
			// Two of four chips-- 2/4-way analog SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (0x0 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (((di->info_Chips - 1) << sli_nLinesLog2) <<
									 CFG_SLI_RENDERMASK_CRT_SHIFT) | (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Second, third, fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (((i -
																														1) <<
																													  sli_nLinesLog2)
																													 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (((di->info_Chips - 1) << sli_nLinesLog2) <<
									 CFG_SLI_RENDERMASK_CRT_SHIFT) | (((i - 1) << sli_nLinesLog2) <<
																				 CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 2 && pInfo->sliEn && pInfo->aaEn
					&& !pInfo->aaSampleHigh && !pInfo->sliAaAnalog)
		{
			// Two chips-- 2-sample AA with 2-way digital SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																							CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}
			else
			{
				// Second chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (((i -
																														1) <<
																													  sli_nLinesLog2)
																													 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0xff <<
																						  CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | (((i -
																														 1) <<
																														sli_nLinesLog2)
																													  <<
																													  CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
					&& !pInfo->aaSampleHigh && !pInfo->sliAaAnalog)
		{
			// Two chips-- 2-sample digital AA (1 subsampled stored in each chip)
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | CFG_DIVIDE_VIDEO_BY_2),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																							CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);

			}
			else
			{
				// Second chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN | CFG_ENHANCED_VIDEO_SLV |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																							CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 2 && !pInfo->sliEn && pInfo->aaEn
					&& !pInfo->aaSampleHigh && pInfo->sliAaAnalog)
		{
			// Two chips-- 2-sample analog AA (1 subsampled stored in each chip)
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);

			}
			else
			{
				// Second chip...
				// Tristate HSYNC since both chips share a common hsync signal
				// (since we want to support analog SLI also...)
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV | CFG_DAC_HSYNC_TRISTATE |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
			}
			PCI_CFG_WR (CFG_VIDEO_CTRL1,
							((0x0 << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_FETCH_SHIFT) | (0x0 <<
																						CFG_SLI_RENDERMASK_CRT_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)), h3ChipSetupDeviceNum);
			PCI_CFG_WR (CFG_VIDEO_CTRL2,
							((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
							 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);

		}
		else if ((di->info_Chips == 2 || di->info_Chips == 4) && pInfo->sliEn
					&& pInfo->aaEn && !pInfo->aaSampleHigh && pInfo->sliAaAnalog)
		{
			// Two of four chips-- 2-sample AA with 2/4-way analog SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (0x0 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (((di->info_Chips - 1) << sli_nLinesLog2) <<
									 CFG_SLI_RENDERMASK_CRT_SHIFT) | (0x0 << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Second, third, fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (((i -
																														1) <<
																													  sli_nLinesLog2)
																													 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (((di->info_Chips - 1) << sli_nLinesLog2) <<
									 CFG_SLI_RENDERMASK_CRT_SHIFT) | (((i - 1) << sli_nLinesLog2) <<
																				 CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 4 && pInfo->sliEn && !pInfo->aaEn
					&& !pInfo->sliAaAnalog)
		{
			// Four chips-- 4-way digital SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 (CFG_VIDEO_OTHERMUX_SEL_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT) |
								 CFG_SLI_AAFIFO_COMPARE_INV | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (0x0 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0x0 <<
																						  CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | ((0x0 <<
																														sli_nLinesLog2)
																													  <<
																													  CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}
			else
			{
				// Second, third, fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (((i -
																														1) <<
																													  sli_nLinesLog2)
																													 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0xff <<
																						  CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | (((i -
																														 1) <<
																														sli_nLinesLog2)
																													  <<
																													  CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 4 && pInfo->sliEn && pInfo->aaEn
					&& !pInfo->aaSampleHigh && !pInfo->sliAaAnalog)
		{
			// Four chips-- 2-sample AA with 4-way digital SLI...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE << CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT) |
								 CFG_SLI_AAFIFO_COMPARE_INV | CFG_DIVIDE_VIDEO_BY_2), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (0x0 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0x0 <<
																						  CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | ((0x0 <<
																														sli_nLinesLog2)
																													  <<
																													  CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}
			else
			{
				// Second, third, fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																					 CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) | (((i -
																														1) <<
																													  sli_nLinesLog2)
																													 <<
																													 CFG_SLI_COMPAREMASK_FETCH_SHIFT)
								 | (0x0 << CFG_SLI_RENDERMASK_CRT_SHIFT) | (0xff <<
																						  CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((((di->info_Chips -
									 1) << sli_nLinesLog2) << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) | (((i -
																														 1) <<
																														sli_nLinesLog2)
																													  <<
																													  CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)),
								h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 4 && pInfo->sliEn && pInfo->aaEn
					&& pInfo->aaSampleHigh && !pInfo->sliAaAnalog)
		{
			// Four chips-- 2-way analog SLI with digital 4-sample AA...

			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT) | CFG_DIVIDE_VIDEO_BY_4),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 ((0x0 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 ((0x0 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else if (i == 2 || i == 4)
			{
				// Second and fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_DAC_HSYNC_TRISTATE |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_1), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (((i >> 2) << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x0 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 ((0xff << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Third chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY |
								 (CFG_VIDEO_OTHERMUX_SEL_PIPE_PLUS_AAFIFO <<
								  CFG_VIDEO_OTHERMUX_SEL_FALSE_SHIFT) | CFG_DIVIDE_VIDEO_BY_4),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0xff << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

		}
		else if (di->info_Chips == 4 && pInfo->sliEn && pInfo->aaEn
					&& pInfo->aaSampleHigh && pInfo->sliAaAnalog)
		{
			// Four chips-- 2-way analog SLI with analog 4-sample AA...
			if (i == 1)
			{
				// First chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_4), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 ((0x0 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 ((0x0 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else if (i == 2 || i == 4)
			{
				// Second and fourth chips...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_DAC_HSYNC_TRISTATE |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_4), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 (((i >> 2) << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 (((i >> 2) << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}
			else
			{
				// Third chip...
				PCI_CFG_WR (CFG_VIDEO_CTRL0,
								(CFG_ENHANCED_VIDEO_EN |
								 CFG_ENHANCED_VIDEO_SLV |
								 CFG_VIDEO_LOCALMUX_DESKTOP_PLUS_OVERLAY | (CFG_VIDEO_OTHERMUX_SEL_PIPE <<
																						  CFG_VIDEO_OTHERMUX_SEL_TRUE_SHIFT)
								 | CFG_DIVIDE_VIDEO_BY_4), h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL1,
								(((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_FETCH_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_RENDERMASK_CRT_SHIFT) |
								 ((0x1 << sli_nLinesLog2) << CFG_SLI_COMPAREMASK_CRT_SHIFT)),
								h3ChipSetupDeviceNum);
				PCI_CFG_WR (CFG_VIDEO_CTRL2,
								((0x0 << CFG_SLI_RENDERMASK_AAFIFO_SHIFT) |
								 (0x0 << CFG_SLI_COMPAREMASK_AAFIFO_SHIFT)), h3ChipSetupDeviceNum);
			}

		}
		else
		{
			dprintf (
						("H3_SETUP_SLI_AA() ERROR: Unsupported combination of {nChips, sliEn, aaEn, ...}\n"));
		}

		if (di->info_Chips == 4 && pInfo->sliEn && pInfo->aaEn
			 && pInfo->aaSampleHigh && i == 4)
		{
			// Make sure that last chip properly waits for data to be xfered
			// over the PCI bus before driving...
			PCI_CFG_WR (CFG_SLI_AA_MISC,
							((PCI_CFG_RD
							  (CFG_SLI_AA_MISC, h3ChipSetupDeviceNum) | CFG_AA_LFB_RD_SLV_WAIT)),
							h3ChipSetupDeviceNum);
		}

		if (i > 1)
		{
			// For the slave chips, make the video PLL lock to the Master's 
			// sync_clk_out clock output...
			PCI_CFG_WR (CFG_VIDEO_CTRL0,
							(PCI_CFG_RD (CFG_VIDEO_CTRL0, h3ChipSetupDeviceNum) | CFG_VIDPLL_SEL),
							h3ChipSetupDeviceNum);

			// Power down Slave(s) RAMDAC...
			PCI_IO_WR (h3ChipSetupIoBase + MISCINIT1,
						  (PCI_IO_RD (h3ChipSetupIoBase + MISCINIT1) | SST_POWERDOWN_DAC));
		}
		// TO DO:
		// Adjust pci fifo thresholds?

		(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, i-1, 4, 2, 0 );		// Disable IO access
	}									  // for(i=1; i<=di->info_Chips; i++) ...
	(*_V5_dd.pci_bus->write_pci_config) ( di->PCI.bus, di->PCI.device, 0, 4, 2, 3 );		// Enable IO access
}

#if 0

/*----------------------------------------------------------------------
Function name:  EnableSLIAA

Description:    Enables SLI and AA on Napalm

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
extern uint32 dwWin98;
uint32 GetFBAddr (uint32);
uint32 GetPitch (uint32);
uint32 GetBPP (uint32);
void SetVideoMode (VidProcConfig * pVpc, PDEVTABLE pDev);
void InitFifowoDF (uint32 lpDriverData, FxU32 fifoBase, FxU32 fifoSize);
void InitFifoFixup (uint32 lpDriverData);
void InitRegswoDF (uint32 lpMasterDriverData, uint32 lpSlaveDriverData);
void SaveMemConfig (uint32 lpDriverData, PCONF_SAVE pConf_Save, uint32 * regBase, uint32 lfbBase,
						  uint32 SliaaLfbBase);
void RestoreMemConfig (uint32 lpDriverData, PCONF_SAVE pConf_Save);
extern uint32 DriverData;
#define MAX_RETRYS (10)
void EnableSLIAA (PDEVTABLE pMaster, PSLI_AA_REQUEST pRequest)
{
	PDEVTABLE pSlave;
	uint32 dwOffset;
	uint32 dwPitch;
	uint32 dwSize;
	uint32 dwY;
	uint32 dwDiff;
	uint32 nFixup;
#ifdef DO_MEMCPY
#ifndef WIN_CSIM
	uint32 compareMask;
	uint32 i;
	uint32 NumLines;
	uint8 *pSlaveFB;
#endif
#endif
	uint32 renderMask;
	uint32 N;
#ifdef USE_FAKE_PHYS
	uint32 FakePhysAddr;
#endif
	uint8 *pMasterFB;
	PDEVTABLE pSaveSlave;
	uint32 dwMapSize;
	uint32 dwOldMapSize;
	CHIPINFO ChipInfo;
	SstIORegs *pSrcIORegs;
	SstIORegs *pDstIORegs;
	uint32 k;
	uint32 j;
	uint32 dwGamma;
	uint32 foo;

#ifdef RD_ABORT_ERROR
	pMaster->SLIAARequest = *pRequest;
#endif
	dwOffset = GetFBAddr (pMaster->lpDriverData);
	dwPitch = GetPitch (pMaster->lpDriverData);
	dwY = pMaster->Vpc.height;
	dwSize = pMaster->Vpc.width * GetBPP (pMaster->lpDriverData);
	dwDiff = dwPitch - dwSize;

	switch (pRequest->ChipInfo.dwsli_nlines)
	{
	case 2:
		N = 1;
		break;

	case 4:
		N = 2;
		break;

	case 8:
		N = 3;
		break;

	case 16:
		N = 4;
		break;

	case 32:
		N = 5;
		break;

	case 64:
		N = 6;
		break;

	case 128:
		N = 7;
		break;

	default:
		N = 7;
		break;
	}

#ifdef FAKE_IT
	pRequest->ChipInfo.dwsliEn = 1;
#endif
	pSaveSlave = NULL;
	renderMask = (pRequest->ChipInfo.dwChips - 1) << N;
	dwMapSize = dwPitch * dwY;
	dwOldMapSize = 0x0;
	for (pSlave = pMaster->pSlave; pSlave != NULL; pSlave = pSlave->pSlave)
	{
		pMasterFB = (uint8 *) (pMaster->LfbBase + dwOffset);
		nFixup = FALSE;
		if (0x0 == pSlave->lpDriverData)
		{
			pSlave->lpDriverData = (uint32) & DriverData;
			nFixup = TRUE;
		}

#ifdef FAKE_IT
		// Enable our resources
		if (!dwWin98)
		{
			PCI_Write_Config (0x00, 0x68, 0x10, 0x0A000000);
			PCI_Write_Config (0x00, 0x68, 0x14, 0x0E000000);
			PCI_Write_Config (0x00, 0x68, 0x18, 0x2000);
			PCI_Write_Config (0x00, 0x68, 0x4, 0x03);
		}
#endif

		// Save Memory Configuration
#ifdef USE_FAKE_PHYS
		if (0x0 == pSlave->SliaaLfbBase)
			NAPDEV_Get_Mem (&FakePhysAddr, &pSlave->SliaaLfbBase);
#endif
		SaveMemConfig (pSlave->lpDriverData, &pSlave->SaveMem, pSlave->RegBase, pSlave->LfbBase,
							pSlave->SliaaLfbBase);

#ifndef WIN_CSIM
		// Disable Master
		PCI_Write_Config (pMaster->Bus, pMaster->DevFunc, SST_PCI_COMMAND_ID, 0x02);

		// Enable Slave
		PCI_Write_Config (pSlave->Bus, pSlave->DevFunc, SST_PCI_COMMAND_ID, 0x03);

		// Now Master and Slave Have same mode
		SetVideoMode (&pMaster->Vpc, pSlave);
		// Disable Slave
		PCI_Write_Config (pSlave->Bus, pSlave->DevFunc, SST_PCI_COMMAND_ID, 0x02);

		// Enable Master
		PCI_Write_Config (pMaster->Bus, pMaster->DevFunc, SST_PCI_COMMAND_ID, 0x03);
#endif

		// Setup the CMDFIFO and 2D Engine <even though it is not used>
		InitRegswoDF (pMaster->lpDriverData, pSlave->lpDriverData);

		// At this point we should have promoted to Overlay and so
		// We Need to copy a few registers from the master to the slave
		// vidProcCfg      

		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidProcCfg =
			((SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX])->vidProcCfg;
		// vidScreenSize

		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidScreenSize =
			((SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX])->vidScreenSize;

		// vidOverlayDuDxOffsetSrcWidth

		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidOverlayDudxOffsetSrcWidth =
			((SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX])->vidOverlayDudxOffsetSrcWidth;

		// vidDesktopOverlayStride

		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidDesktopOverlayStride =
			((SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX])->vidDesktopOverlayStride;

		//vidOverlayDudx

		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidOverlayDudx =
			((SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX])->vidOverlayDudx;

#ifdef DO_MEMCPY
#ifndef WIN_CSIM
		if (pRequest->ChipInfo.dwsliEn)
		{
			// This should make the switch up seem seamless
			if (1 == pSlave->UnitNum)
				pSaveSlave = pSlave;

			if (0x0 == dwOldMapSize)
				dwOldMapSize = pSlave->nPages << 12;
			else
				dwOldMapSize = dwMapSize + (dwOffset & 4095);

#ifndef WIN_CSIM
			pSlaveFB = SwapPhysFB (pSlave, dwOffset, dwOldMapSize, dwMapSize);
#else
			pSlaveFB = (uint8 *) (pSlave->LfbBase + dwOffset);
#endif

			compareMask = pSlave->UnitNum << N;

			for (i = 0; i < dwY; i += NumLines)
			{
				NumLines = MIN (pRequest->ChipInfo.dwsli_nlines, dwY - i);
				if ((i & renderMask) == compareMask)
				{
					for (j = 0; j < NumLines; j++)
					{
						our_memcpy (pSlaveFB, pMasterFB, dwSize);
						pSlaveFB += dwPitch;
						pMasterFB += dwPitch;
					}
				}
				else
					pMasterFB = pMasterFB + (dwPitch * NumLines);
			}
		}
#endif
#endif

		if (nFixup)
			pSlave->lpDriverData = 0x0;

		// Copy the whole DAC
		pSrcIORegs = (SstIORegs *) pMaster->RegBase[HWINFO_SST_IOREGS_INDEX];
		pDstIORegs = (SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX];
		for (j = 0; j < 512; j++)
		{
			pSrcIORegs->dacAddr = j;
			pDstIORegs->dacAddr = j;
			dwGamma = pSrcIORegs->dacData;
			for (k = 0; k < MAX_RETRYS; k++)
			{
				pDstIORegs->dacData = dwGamma;
				foo = pDstIORegs->dacData;
				if (dwGamma == foo)
				{
					k = MAX_RETRYS;
				}
			}
		}
	}

#ifdef DO_MEMCPY
#ifndef WIN_CSIM
	if (pSaveSlave)
		SwapPhysFB (pSaveSlave, 0x0, dwMapSize, pSaveSlave->nPages << 12);
#endif
#endif

#ifdef DO_MEMCPY
#ifndef WIN_CSIM
	// Need to compress Master
	if (pRequest->ChipInfo.dwsliEn)
	{
		compareMask = 0x0 << N;
		pMasterFB = (uint8 *) (pMaster->LfbBase + dwOffset);
		pSlaveFB = (uint8 *) (pMaster->LfbBase + dwOffset);
		for (i = 0; i < dwY; i += NumLines)
		{
			NumLines = MIN (pRequest->ChipInfo.dwsli_nlines, dwY - i);
			if ((i & renderMask) == compareMask)
			{
				for (j = 0; j < NumLines; j++)
				{
					our_memcpy (pSlaveFB, pMasterFB, dwSize);
					pSlaveFB += dwPitch;
					pMasterFB += dwPitch;
				}
			}
			else
				pMasterFB = pMasterFB + (dwPitch * NumLines);
		}
	}
#endif
#endif

	// Need to Resync Master
	InitFifoFixup (pMaster->lpDriverData);

	ChipInfo.dwChips = pRequest->ChipInfo.dwChips;
	ChipInfo.dwsliEn = pRequest->ChipInfo.dwsliEn;
	ChipInfo.dwaaEn = pRequest->ChipInfo.dwaaEn;
	ChipInfo.dwaaSampleHigh = pRequest->ChipInfo.dwaaSampleHigh;
	ChipInfo.dwsliAaAnalog = pRequest->ChipInfo.dwsliAaAnalog;
	ChipInfo.dwsli_nlines = pRequest->ChipInfo.dwsli_nlines;
	ChipInfo.dwCfgSwapAlgorithm = pRequest->ChipInfo.dwCfgSwapAlgorithm;
	ChipInfo.dwMasterID = 0x0;
	ChipInfo.pDevTable = (uint32) pMaster;

#ifndef FAKE_IT
	H3_SETUP_SLI_AA (SLI_AA_ENABLE, &ChipInfo, &pRequest->MemInfo);
#endif

	return;
}

/*----------------------------------------------------------------------
Function name:  DisableSLIAA

Description:    Disable SLI and AA on Napalm

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
void RestoreMemConfig (uint32 lpDriverData, PCONF_SAVE pConf_Save);
void DisableSLIAA (PDEVTABLE pMaster, PSLI_AA_REQUEST pRequest)
{
	PDEVTABLE pSlave;
	uint32 dwOffset;
	uint32 dwPitch;
	uint32 dwSize;
	uint32 dwY;
	uint32 dwDiff;
#ifdef DO_MEMCPY
#ifndef WIN_CSIM
	int i;
	uint32 j;
	uint32 NumLines;
	uint8 *pSlaveFB;
#endif
#endif
	uint32 nFixup;
	uint32 compareMask;
	uint32 renderMask;
	uint32 N;
	uint8 *pMasterFB;
	PDEVTABLE pSaveSlave;
	uint32 dwMapSize;
	uint32 dwOldMapSize;
	CHIPINFO ChipInfo;
	uint32 dwReg;

#ifdef RD_ABORT_ERROR
	pMaster->SLIAARequest = *pRequest;
#endif
	ChipInfo.dwChips = pRequest->ChipInfo.dwChips;
	ChipInfo.dwsliEn = pRequest->ChipInfo.dwsliEn;
	ChipInfo.dwaaEn = pRequest->ChipInfo.dwaaEn;
	ChipInfo.dwaaSampleHigh = pRequest->ChipInfo.dwaaSampleHigh;
	ChipInfo.dwsliAaAnalog = pRequest->ChipInfo.dwsliAaAnalog;
	ChipInfo.dwsli_nlines = pRequest->ChipInfo.dwsli_nlines;
	ChipInfo.dwCfgSwapAlgorithm = pRequest->ChipInfo.dwCfgSwapAlgorithm;
	ChipInfo.dwMasterID = 0x0;
	ChipInfo.pDevTable = (uint32) pMaster;

#ifndef FAKE_IT
	H3_SETUP_SLI_AA (SLI_AA_DISABLE, &ChipInfo, &pRequest->MemInfo);
#else
	pRequest->ChipInfo.dwsliEn = 1;
#endif

	dwOffset = GetFBAddr (pMaster->lpDriverData);
	dwPitch = GetPitch (pMaster->lpDriverData);
	dwY = pMaster->Vpc.height;
	dwSize = pMaster->Vpc.width * GetBPP (pMaster->lpDriverData);
	dwDiff = dwPitch - dwSize;

	switch (pRequest->ChipInfo.dwsli_nlines)
	{
	case 2:
		N = 1;
		break;

	case 4:
		N = 2;
		break;

	case 8:
		N = 3;
		break;

	case 16:
		N = 4;
		break;

	case 32:
		N = 5;
		break;

	case 64:
		N = 6;
		break;

	case 128:
		N = 7;
		break;

	default:
		N = 7;
		break;
	}

	renderMask = (pRequest->ChipInfo.dwChips - 1) << N;

#ifdef DO_MEMCPY
#ifndef WIN_CSIM
	if (pRequest->ChipInfo.dwsliEn)
	{
		compareMask = 0x0 << N;
		pMasterFB = (uint8 *) (pMaster->LfbBase + dwOffset + dwPitch * dwY);
		pSlaveFB = (uint8 *) (pMaster->LfbBase + dwOffset);
		for (i = 0; i < (int) dwY; i += NumLines)
		{
			NumLines = MIN (pRequest->ChipInfo.dwsli_nlines, dwY - i);
			if ((i & renderMask) == compareMask)
				pSlaveFB += NumLines * dwPitch;
		}

		pSlaveFB -= dwPitch;
		for (i = dwY - NumLines; i >= 0; i -= NumLines)
		{
			NumLines = MIN (pRequest->ChipInfo.dwsli_nlines, dwY - i);
			if ((i & renderMask) == compareMask)
			{
				for (j = 0; j < NumLines; j++)
				{
					our_memcpy (pMasterFB, pSlaveFB, dwSize);
					pSlaveFB -= dwPitch;
					pMasterFB -= dwPitch;
				}
			}
			else
				pMasterFB = pMasterFB - (dwPitch * NumLines);
		}
	}
#endif
#endif

	pSaveSlave = NULL;
	dwMapSize = dwPitch * dwY;
	dwOldMapSize = 0x0;
	for (pSlave = pMaster->pSlave; pSlave != NULL; pSlave = pSlave->pSlave)
	{
		pMasterFB = (uint8 *) (pMaster->LfbBase + dwOffset);
		compareMask = pSlave->UnitNum << N;

#ifdef DO_MEMCPY
#ifndef WIN_CSIM
		if (pRequest->ChipInfo.dwsliEn)
		{
			if (1 == pSlave->UnitNum)
				pSaveSlave = pSlave;

			if (0x0 == dwOldMapSize)
				dwOldMapSize = pSlave->nPages << 12;
			else
				dwOldMapSize = dwMapSize + (dwOffset & 4095);

#ifndef WIN_CSIM
			pSlaveFB = SwapPhysFB (pSlave, dwOffset, dwOldMapSize, dwMapSize);
#else
			pSlaveFB = (uint8 *) (pSlave->LfbBase + dwOffset);
#endif

			for (i = 0; i < (int) dwY; i += NumLines)
			{
				NumLines = MIN (pRequest->ChipInfo.dwsli_nlines, dwY - i);
				if ((i & renderMask) == compareMask)
				{
					for (j = 0; j < NumLines; j++)
					{
						our_memcpy (pMasterFB, pSlaveFB, dwSize);
						pSlaveFB += dwPitch;
						pMasterFB += dwPitch;
					}
				}
				else
					pMasterFB = pMasterFB + (dwPitch * NumLines);
			}
		}
#endif
#endif

		nFixup = FALSE;
		if (0x0 == pSlave->lpDriverData)
		{
			pSlave->lpDriverData = (uint32) & DriverData;
			nFixup = TRUE;
		}

		// Restore Memory Configuration
		RestoreMemConfig (pSlave->lpDriverData, &pSlave->SaveMem);

		// if Slave had a mode then reset it
		if (0x0 != pSlave->Vpc.height)
		{
			// Disable Master
			PCI_Write_Config (pMaster->Bus, pMaster->DevFunc, SST_PCI_COMMAND_ID, 0x02);

			// Enable Slave
			PCI_Write_Config (pSlave->Bus, pSlave->DevFunc, SST_PCI_COMMAND_ID, 0x03);

			SetVideoMode (&pSlave->Vpc, pSlave);

			// Disable Slave
			PCI_Write_Config (pSlave->Bus, pSlave->DevFunc, SST_PCI_COMMAND_ID, 0x02);

			// Enable Master
			PCI_Write_Config (pMaster->Bus, pMaster->DevFunc, SST_PCI_COMMAND_ID, 0x03);
		}

		// Setup the CMDFIFO and 2D Engine <when though it is not used>
		InitRegswoDF (pSlave->lpDriverData, pSlave->lpDriverData);

		// Kill the slave since the DAC are tied together
		dwReg = ((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidProcCfg;
		dwReg &= ~SST_VIDEO_PROCESSOR_EN;
		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->vidProcCfg = dwReg;
		dwReg = SST_DAC_DPMS_ON_VSYNC | SST_DAC_DPMS_ON_HSYNC;
		((SstIORegs *) pSlave->RegBase[HWINFO_SST_IOREGS_INDEX])->dacMode = dwReg;

		if (nFixup)
			pSlave->lpDriverData = 0x0;
	}

#ifndef WIN_CSIM
	if (pSaveSlave)
		SwapPhysFB (pSaveSlave, 0x0, dwMapSize, pSaveSlave->nPages << 12);
#endif

	return;
}

#ifdef RD_ABORT_ERROR
/*----------------------------------------------------------------------
Function name:  SLI_Read_Enable

Description:    Enable SLI Reads

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
void SLI_Read_Enable (PDEVTABLE pMaster)
{
	PDEVTABLE pDev;
	uint32 cfgSliLfbCtrl;
	uint32 cfgAALfbCtrl;

	if (pMaster->SLIAARequest.ChipInfo.dwsliEn)
	{
		for (pDev = pMaster; NULL != pDev; pDev = pDev->pSlave)
		{
			// Turn on SLI Read
			cfgSliLfbCtrl = PCI_Read_Config (pDev->Bus, pDev->DevFunc, CFG_SLI_LFB_CTRL);
			cfgSliLfbCtrl |= (CFG_SLI_RD_EN);
			PCI_Write_Config (pDev->Bus, pDev->DevFunc, CFG_SLI_LFB_CTRL, cfgSliLfbCtrl);

			// Turn off AA if it is not enabled
			if (!pMaster->SLIAARequest.ChipInfo.dwaaEn)
			{
				cfgAALfbCtrl = PCI_Read_Config (pDev->Bus, pDev->DevFunc, CFG_AA_LFB_CTRL);
				cfgAALfbCtrl &= ~(CFG_AA_LFB_RD_EN);
				PCI_Write_Config (pDev->Bus, pDev->DevFunc, CFG_AA_LFB_CTRL, cfgAALfbCtrl);
			}
		}
	}
}

/*----------------------------------------------------------------------
Function name:  SLI_Read_Disable

Description:    Disable SLI Reads

Information:

Return:    Nothing     
                
----------------------------------------------------------------------*/
void SLI_Read_Disable (PDEVTABLE pMaster)
{
	PDEVTABLE pDev;
	uint32 cfgSliLfbCtrl;
	uint32 cfgAALfbCtrl;

	if (pMaster->SLIAARequest.ChipInfo.dwsliEn)
	{
		for (pDev = pMaster; NULL != pDev; pDev = pDev->pSlave)
		{
			// Turn off SLI Read
			cfgSliLfbCtrl = PCI_Read_Config (pDev->Bus, pDev->DevFunc, CFG_SLI_LFB_CTRL);
			cfgSliLfbCtrl &= ~(CFG_SLI_RD_EN);
			PCI_Write_Config (pDev->Bus, pDev->DevFunc, CFG_SLI_LFB_CTRL, cfgSliLfbCtrl);

			// Turn on AA Read
			cfgAALfbCtrl = PCI_Read_Config (pDev->Bus, pDev->DevFunc, CFG_AA_LFB_CTRL);
			cfgAALfbCtrl |= (CFG_AA_LFB_RD_EN);
			PCI_Write_Config (pDev->Bus, pDev->DevFunc, CFG_AA_LFB_CTRL, cfgAALfbCtrl);
		}
	}
}
#endif

/*----------------------------------------------------------------------
Function name:  our_mem_rdd

Description:    Read a uint32 from Memory Mapped Space

Information:

Return:    uint32
                
----------------------------------------------------------------------*/
uint32 our_mem_rdd (uint32 Addr)
{
	uint32 dwReturn;

	_asm mov ebx, Addr _asm mov eax,[ebx] _asm mov dwReturn, eax return dwReturn;
}

/*----------------------------------------------------------------------
Function name:  our_mem_wrd

Description:    Write a uint32 from Memory Mapped Space

Information:

Return:    uint32
                
----------------------------------------------------------------------*/
void our_mem_wrd (uint32 Addr, uint32 Value)
{
_asm mov ebx, Addr _asm mov eax, Value _asm mov[ebx], eax}
/*----------------------------------------------------------------------
Function name:  our_mem_wrd

Description:    Fast Memcpy

Information:

Return:    uint32
                
----------------------------------------------------------------------*/
	void our_memcpy (uint8 * pDest, uint8 * pSrc, uint32 dwSize)
{
	__asm pushad
		// Save Registers-----------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm mov edi, pDest
		// Get Destination----------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm mov esi, pSrc
		// Get Sourceation----------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm xor ecx, ecx
		// count=0rceation----------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm mov ebx, dwSize
		// Get Sizeceation----------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm shr ebx, 1
		// Convert to words---------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm adc cx, 0
		// Remainder? words---------------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm rep movsb
		// Move a uint8 if needed---------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm shr ebx, 1
		// Convert to uint32seded---------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm mov ecx, ebx
		// Countrt to uint32seded---------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm rep movsd
		// doittrt to uint32seded---------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm adc ecx, 0
		// any words? uint32seded---------------------------------------------*/ __asm cld // Go Forwardd Tile
		__asm rep movsw // move itds? uint32seded---------------------------------------------*/ __asm popad  // all done Tile }
#endif
