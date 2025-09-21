
#ifndef __RADEON_DRIVER_H__
#define __RADEON_DRIVER_H__

#include <KernelExport.h>
#include <drivers/PCI.h>
#include <drivers/Drivers.h>
#include <graphic_driver.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>

#include <graphics_p/radeon/CardState.h>
#include <graphics_p/common/MemMgr/MemMgrKrnl.h>
#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#define	dprintf(x) dprintf x

#define MAX_PCI_DEVICES 32
#define MAX_CARDS 6

/*
 * GRAPHICS_DEVICE_PATH will be set to non-NULL in the Makefile for
 * debug/development builds.
 */
#ifndef	GRAPHICS_DEVICE_PATH
#define	GRAPHICS_DEVICE_PATH	""
#endif


typedef struct DeviceInfoRec
{
	pci_info PCI;
	
	area_id AreaFrameBuffer;
	area_id AreaRegs;
	area_id AreaCardInfo;
	area_id AreaBios;
	area_id AreaGart;

	__mem_Area *ma;
	
	uint32 SlotNum;
	int32 isMobility;

	AdapterInfo info;

	char Name[B_OS_NAME_LENGTH];

	_PLL_BLOCK pll;
	
	uint32 bpp[2];
	uint32 mode[2];
	
	display_mode mode_Primary;
	
	int32 Opened;
	int32 dpms_mode;

	CardInfo *ci;
	uint8 *bios;
	uint8 *biosInfo;
	uint32 *Gart;
	
	int32 videoOverideID;
	uint32 pciBufferMap[16];		// 512K chunks.
} DeviceInfo;


typedef struct DriverInfoRec
{
	sem_id LockSem;
	uint32 DeviceCount;
	char *DevNames[MAX_CARDS+1];
	DeviceInfo Device[MAX_CARDS];
} DriverInfo;

typedef struct GFX_MODES_REC
{
	int32 w;
	int32 h;
	int32 hz;
	CRTCInfoBlock info;
} GFX_MODES;

extern DriverInfo radeon_di;
extern const GFX_MODES gfx_modes[];

extern uint8 Card_Detect();
extern uint8 Radeon_Probe();
extern uint8 Radeon_TestBios( DeviceInfo *di );
extern uint8 Radeon_SetModeNB (DeviceInfo *di, display_mode *dm, uint16 depth );
extern uint8 Radeon_SetDisplayMode( DeviceInfo *di, display_mode *mode );
extern uint32 Radeon_GetModeCount( DeviceInfo *di );
extern const void * Radeon_GetModeList( DeviceInfo *di );
extern void Radeon_SetDPMS( CardInfo *ci, uint16 dpms_state);

extern uint8 Radeon_Set2NdMode( DeviceInfo *di, int32 w, int32 h, int32 hz, int32 is32bpp );

#endif
