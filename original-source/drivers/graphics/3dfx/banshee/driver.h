/*
   	This contains the structures needed to interface
   	with the pseudo-driver
*/

#ifndef __THDFX_DRIVER_H
#define __THDFX_DRIVER_H

#include <KernelExport.h>

#include <graphics_p/3dfx/common/bena4.h>
#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/common/MemMgr/MemMgr.h>

/*****************************************************************************
 * Definitions.
 */
 
#define MAX_CARDS		4

#define INT_CHECK_TIMEOUT 500000

/*
 * GRAPHICS_DEVICE_PATH will be set to non-NULL in the Makefile for
 * debug/development builds.
 */
#ifndef	GRAPHICS_DEVICE_PATH
#define	GRAPHICS_DEVICE_PATH	""
#endif

/****************************************************************************
 * Structure definitions.
 */
typedef struct devinfo {
	struct timer	int_check_timer;	/* This MUST come first for the Kernel Timer handler to succeed !! - nasty typecasting ! */
	pci_info	PCI;
	area_id		BaseAddr0ID; 		/* Banshee 2D/3D & AGP Memory Mapped Registers */
	area_id		BaseAddr1ID; 		/* Banshee Memory Mapped Frame Buffer */
	
	area_id		ci_AreaID; 
	thdfx_card_info	*ci;

	area_id		MemMgr_AreaID; 
	__mem_Area	*MemMgr;

	uint32		SlotNum;
	uint32		io_base;
	
	uint32		Opened;
	int32		IRQEnabled;
	uint32		NInterrupts;	// DEBUG
	uint32		NVBlanks;	// DEBUG
	uint32		LastIrqMask;	// DEBUG
	char		Name[B_OS_NAME_LENGTH];
	
	uint32		PrimXRes;
	uint32		PrimYRes;
	uint32		PrimRefresh;
	uint32		PrimLoacClut;
	int32		PrimOverideID;
	
	int32		engine_3d_lock;
} devinfo;

typedef struct driverdata {
	Bena4		DriverLock;					/*  Driver-wide benaphore	*/
	uint32		NDevs;						/*  # of devices found		*/
	uint32		NInterrupts;				/*  # of IRQs from this device	*/
	uint32		NCalls;						/*  # of IRQs from this device	*/
	char		*DevNames[MAX_CARDS + 1];	/*  For export  */
	pci_module_info *pci_bus;
	devinfo		DI[MAX_CARDS];				/* Device-specific stuff	*/
} driverdata;

extern driverdata _V3_dd;

extern uint8 _V3_SetVideoMode( devinfo *di,
	uint32 xRes,    // xResolution in pixels
	uint32 yRes,    // yResolution in pixels
	uint32 refresh, // refresh rate in Hz
	uint32 loadClut); // really a bool, should we load the lookup table

extern void _V3_WriteReg_16( devinfo *di, uint32 reg, uint16 data );
extern uint16 _V3_ReadReg_16( devinfo *di, uint32 reg );
extern void _V3_WriteReg_8( devinfo *di, uint32 reg, uint8 data );
extern uint8 _V3_ReadReg_8( devinfo *di, uint32 reg );


#endif 

