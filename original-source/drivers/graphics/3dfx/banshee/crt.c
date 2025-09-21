#include <Drivers.h>
#include <OS.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/3dfx/common/debug.h>

#include <stdio.h>

#include "driver.h"


//#include <add-ons/graphics/GraphicsCard.h>
//#include <add-ons/graphics/Accelerant.h>

uint16 mode_table[][24] =
{ 
#include "modetable.h"
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        
};

uint8 vgaattr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x00, 0x00, 0x00, 0x00, 
                  0x00, 0x01, 0x00, 0x0F, 0x00};

/*----------------------------------------------------------------------
Function name:  FindVideoMode

Description:    Find the video mode in the mode table based on the
                xRes, yRes, and refresh rate.
Information:

Return:         (uint16 *)   Ptr to the entry in the mode table,
                            NULL if failure.
----------------------------------------------------------------------*/
uint16 *
FindVideoMode(uint32 xRes,
	          uint32 yRes,
              uint32 refresh)
{
    int i = 0;

    while (mode_table[i][0] != 0)
    {
        if ((mode_table[i][0] == xRes) &&
            (mode_table[i][1] == yRes) &&
            (mode_table[i][2] == refresh))
        {
            return &mode_table[i][3];
        }
        if ((mode_table[i][0] == xRes) &&
            (mode_table[i][1] == yRes) &&
            (mode_table[i+1][2] > refresh) &&
						(mode_table[i][2] < refresh))
        {
					// We may have a partial match where the refresh is not 
					// exactly identical but 'would do'. For example if the 3dfx
					// card supports 70 or 75 Hz but we were asked for 72 Hz then
					// 70 'would do' so return that
//				  dprintf(("3dfx_V3_Krnl: FindVideoMode(%d, %d, %d), returning entry %d, real xRes = %d, yRes = %d, refresh = %d\n", xRes, yRes, refresh, i, mode_table[i][0],mode_table[i][1],mode_table[i][2]));
					return  &mode_table[i][3];
				}
        i += 1;
    }

    return NULL;
}





/*----------------------------------------------------------------------
Function name:  SetVideoMode

Description:    Set the video mode.

Information:

Return:         FxBool  FXTRUE  if success or,
                        FXFALSE if failure.
----------------------------------------------------------------------*/
uint8 _V3_SetVideoMode(
	devinfo *di,
    uint32 xRes,    // xResolution in pixels
    uint32 yRes,    // yResolution in pixels
    uint32 refresh, // refresh rate in Hz
  uint32 loadClut) // really a bool, should we load the lookup table
{
    uint16 i, j;
    uint8 garbage;
    uint16 *rs = FindVideoMode(xRes, yRes, refresh);
    uint32 vidProcCfg;
    uint32 temp;

    if (rs == NULL)
    {
        dprintf(("3dfx_V3_Krnl: SetVideoMode - mode %d x %d @ %dHz is not supported\n", xRes, yRes, refresh));
        return 0;
    }
        
//    dprintf(("3dfx_V3_Krnl: SetVideoMode(%d, %d, %d)\n", xRes, yRes, refresh));

    //
    // MISC REGISTERS
    // This register gets programmed first since the Mono/ Color
    // selection needs to be made.
    //
    // Sync polarities
    // Also force the programmable clock to be used with bits 3&2
    //

    _V3_WriteReg_8( di, 0xc2, rs[16] | BIT(0));
    //
    // CRTC REGISTERS
    // First Unlock CRTC, then program them 
    //
    
    // Mystical VGA Magic
    _V3_WriteReg_16( di, 0x0d4,  0x11);
    _V3_WriteReg_16( di, 0x0d4,  (rs[0] << 8) | 0x00);
    _V3_WriteReg_16( di, 0x0d4,  (rs[1] << 8) | 0x01);
    _V3_WriteReg_16( di, 0x0d4,  (rs[2] << 8) | 0x02);
    _V3_WriteReg_16( di, 0x0d4,  (rs[3] << 8) | 0x03);
    _V3_WriteReg_16( di, 0x0d4,  (rs[4] << 8) | 0x04);

#ifdef H3_A0
//
// The problem with 800x600 is due to a
// synthesis bug (which A1 does not have)
//
    if (rs[14] & 0x80)
    {
        _V3_WriteReg_16( di, 0xd4, ((rs[5] & 0xe0) << 8) | 0x05);
    }
    else
    {
        _V3_WriteReg_16( di, 0x0d4,  (rs[5] << 8) | 0x05);
    }
#else // #ifdef H3_A0
    _V3_WriteReg_16( di, 0x0d4,  (rs[5] << 8) | 0x05);
#endif // #ifdef H3_A0    

    _V3_WriteReg_16( di, 0x0d4,  (rs[6] << 8) | 0x06);
    _V3_WriteReg_16( di, 0x0d4,  (rs[7] << 8) | 0x07);
    _V3_WriteReg_16( di, 0x0d4,  (rs[8] << 8) | 0x09);
    _V3_WriteReg_16( di, 0x0d4,  (rs[9] << 8) | 0x10);
    _V3_WriteReg_16( di, 0x0d4,  (rs[10] << 8) | 0x11);
    _V3_WriteReg_16( di, 0x0d4,  (rs[11] << 8) | 0x12);
    _V3_WriteReg_16( di, 0x0d4,  (rs[12] << 8) | 0x15);
    _V3_WriteReg_16( di, 0x0d4,  (rs[13] << 8) | 0x16);
#ifdef H3_A0
    _V3_WriteReg_16( di, 0x0d4,  (rs[14] << 8) | 0x101a);
    _V3_WriteReg_16( di, 0x0d4,  (rs[15] << 8) | 0x101b);
#else  // #ifdef H3_A0
    _V3_WriteReg_16( di, 0x0d4,  (rs[14] << 8) | 0x1a);
    _V3_WriteReg_16( di, 0x0d4,  (rs[15] << 8) | 0x1b);
#endif // #ifdef H3_A0
    
    //
    // Enable Sync Outputs
    //
    _V3_WriteReg_16( di, 0x0d4, (0x80 << 8) | 0x17);

    //
    // VIDCLK (32 bit access only!)
    // Set the Video Clock to the correct frequency
    //
	_V3_WriteReg_NC( di->ci, V3_VID_PLL_CTRL_0, (rs[19] << 8) | rs[18]);

    //
    // dacMode (32 bit access only!)
    // (sets up 1x mode or 2x mode)
    //
	_V3_WriteReg_NC( di->ci, V3_VID_DAC_MODE, rs[20]);

    //
    // the 1x / 2x bit must also be set in vidProcConfig to properly
    // enable 1x / 2x mode
    //
    vidProcCfg = _V3_ReadReg( di->ci, V3_VID_PROC_CFG );
    vidProcCfg &= ~(SST_VIDEO_2X_MODE_EN | SST_HALF_MODE);
    if (rs[20])
        vidProcCfg |= SST_VIDEO_2X_MODE_EN;
#if defined(H3VDD)
    if (scanlinedouble)
       vidProcCfg |= SST_HALF_MODE;
#endif
		_V3_WriteReg_NC( di->ci, V3_VID_PROC_CFG, vidProcCfg);

    //
    // SEQ REGISTERS
    // set run mode in the sequencer (not reset)
    //
    // make sure bit 5 == 0 (i.e., screen on)
    //
    _V3_WriteReg_16( di, 0x0c4, ((rs[17] & ((uint16) ~BIT(5))) << 8) | 0x1 );
    _V3_WriteReg_16( di, 0x0c4, ( 0x3 << 8) | 0x0 );

    //
    // turn off VGA's screen refresh, as this function only sets extended
    // video modes, and the VGA screen refresh eats up performance
    // (10% difference in screen to screen blits!).   This code is not in
    // the perl, but should stay here unless specifically decided otherwise
    //

	temp = _V3_ReadReg( di->ci, V3_VID_VGA_INIT_0 );
	_V3_WriteReg_NC( di->ci, V3_VID_VGA_INIT_0, temp | BIT(12) );

    //
    // Make sure attribute index register is initialized
    //
    garbage = _V3_ReadReg_8( di, 0x0da); // return value not used

    for (i = 0; i <= 19; i++)
    {
        _V3_WriteReg_8( di, 0xc0, i);
        _V3_WriteReg_8( di, 0xc0, vgaattr[i]);
    }

    _V3_WriteReg_8( di, 0xc0, 0x34);

    _V3_WriteReg_8( di, 0xda, 0);

    //
    // Initialize VIDEO res & proc mode info...
    //

#if defined(H3VDD)
    if (scanlinedouble)
			_V3_WriteReg_NC( di->ci, SSTIOADDR(vidScreenSize), (yRes << 13) | (xRes & 0xfff));
    else
			_V3_WriteReg_NC( di->ci, SSTIOADDR(vidScreenSize), (yRes << 12) | (xRes & 0xfff));
#else
	_V3_WriteReg_NC( di->ci, V3_VID_SCREEN_SIZE, (yRes << 12) | (xRes & 0xfff));
#endif

	_V3_WriteReg_NC( di->ci, V3_VID_OVERLAY_START_COORDS, 0);

    // I think "xFixRes" is obsolete since we're not supporting 1800
    // modes (only 1792 or 1808), so I won't use it here.   -KMW
    //
		_V3_WriteReg_NC( di->ci, V3_VID_OVERLAY_END_SCREEN_COORD, (((yRes - 1) << 12) |
                                      ((xRes - 1) & 0xfff)));
    //
    // Load CLUTs with an inverted ramp (undo inversion with new hardware!)
    //
    // Put in another routines.
    //
    //if (loadClut)
    {
		uint32 t[64];
        for( i=0; i <= 0x1ff; i++)
        {
			atomic_add( &t[0], 1 );
			atomic_add( &t[8], 1 );
			atomic_add( &t[16], 1 );
			atomic_add( &t[24], 1 );
			atomic_add( &t[32], 1 );
			_V3_WriteReg_NC( di->ci, V3_VID_DAC_ADDR, i);

            // miscInit1[0], is set 
            _V3_ReadReg( di->ci, V3_VID_DAC_ADDR );
            j = i & 0xff;

			atomic_add( &t[0], 1 );
			atomic_add( &t[8], 1 );
			atomic_add( &t[16], 1 );
			atomic_add( &t[24], 1 );
			atomic_add( &t[32], 1 );

#ifdef H3_A0
			_V3_WriteReg_NC( di->ci, V3_VID_DAC_DATA,  ~((j<<16) | (j<<8) | j));
#else  // #ifdef H3_A0
			_V3_WriteReg_NC( di->ci, V3_VID_DAC_DATA,   (j<<16) | (j<<8) | j);
#endif // #ifdef H3_A0
            _V3_ReadReg( di->ci, V3_VID_DAC_DATA );
        }
    }

    return 1; /* success! */

} // h3InitSetVideoMode

