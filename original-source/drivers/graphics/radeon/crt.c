/******************************************************************************
 * Radeon sample code                                                         *
 *                                                                            *
 * nbmode.c - This module sets a display mode without using the video         *
 * BIOS.  The screen is cleared.                                              *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc. All rights reserved.              *
 ******************************************************************************/
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <stdio.h>
#include <math.h>

#include <Accelerant.h>
#include <graphics_p/video_overlay.h>
#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>

#include "driver.h"

extern pci_module_info *pci_bus;


//#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_SUPPORTS_OVERLAYS)
#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS)
#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)

static const int32 mode_hz[] = {
	60,60, 70,70, 75,75, 85,85,	// 640
	60,60, 75,75, 72,72, 85,85,	// 800
	60,60, 70,70, 75,75, 85,85,	// 1024
	70,70, 75,75, 85,85,		// 1152
	60,60, 75,75, 85,85,		// 1280
	60,60, 65,65, 70,70, 75,75, 80,80, 85,85 //1600
	};
	
	

/*
 * This table is formatted for 132 columns, so stretch that window...
 * Stolen from Trey's R4 Matrox driver and reformatted to improve readability
 * (after a fashion).  Some entries have been updated with Intel's "official"
 * numbers.
 */
 static const display_mode	mode_list[] = {
/* Now the standard App-server modes */
/* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
 {	{  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
 {	{  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
/* 640X480X60Hz */
// {	{  27500,  640,  672,  768,  800,  480,  488,  494,  530, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
// {	{  27500,  640,  672,  768,  800,  480,  488,  494,  530, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
/* SVGA_640X480X60HzNI */
// {	{  30500,  640,  672,  768,  800,  480,  517,  523,  588, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
// {	{  30500,  640,  672,  768,  800,  480,  517,  523,  588, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  768,  800,  480,  489,  492,  520, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
 {	{  31500,  640,  672,  768,  800,  480,  489,  492,  520, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  736,  840,  480,  481,  484,  500, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
 {	{  31500,  640,  672,  736,  840,  480,  481,  484,  500, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
 {	{  36000,  640,  712,  768,  832,  480,  481,  484,  509, 0},			B_RGB16_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
 {	{  36000,  640,  712,  768,  832,  480,  481,  484,  509, 0},			B_RGB32_LITTLE,  640,  480, 0, 0, MODE_FLAGS},
 
/* SVGA_800X600X56HzNI */
// {	{  38100,  800,  832,  960, 1088,  600,  602,  606,  620, 0},			B_RGB16_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
// {	{  38100,  800,  832,  960, 1088,  600,  602,  606,  620, 0},			B_RGB32_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
 {	{  40000,  800,  856,  984, 1056,  600,  601,  605,  628, T_POSITIVE_SYNC},	B_RGB16_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
 {	{  40000,  800,  856,  984, 1056,  600,  601,  605,  628, T_POSITIVE_SYNC},	B_RGB32_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
 {	{  49500,  800,  832,  912, 1056,  600,  601,  604,  625, T_POSITIVE_SYNC},	B_RGB16_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
 {	{  49500,  800,  832,  912, 1056,  600,  601,  604,  625, T_POSITIVE_SYNC},	B_RGB32_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
 {	{  50000,  800,  832,  912, 1056,  600,  637,  643,  666, T_POSITIVE_SYNC},	B_RGB16_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
 {	{  50000,  800,  832,  912, 1056,  600,  637,  643,  666, T_POSITIVE_SYNC},	B_RGB32_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
 {	{  56250,  800,  848,  912, 1048,  600,  601,  604,  631, T_POSITIVE_SYNC},	B_RGB16_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
 {	{  56250,  800,  848,  912, 1048,  600,  601,  604,  631, T_POSITIVE_SYNC},	B_RGB32_LITTLE,  800,  600, 0, 0, MODE_FLAGS},
 
/* SVGA_1024X768X43HzI */
// {	{  46600, 1024, 1088, 1216, 1312,  384,  385,  388,  404, B_TIMING_INTERLACED},	B_RGB16_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
// {	{  46600, 1024, 1088, 1216, 1312,  384,  385,  388,  404, B_TIMING_INTERLACED},	B_RGB32_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
 {	{  65000, 1024, 1064, 1200, 1344,  768,  771,  777,  806, 0},			B_RGB16_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
 {	{  65000, 1024, 1064, 1200, 1344,  768,  771,  777,  806, 0},			B_RGB32_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
 {	{  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806, 0},			B_RGB16_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
 {	{  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806, 0},			B_RGB32_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
 {	{  78750, 1024, 1056, 1152, 1312,  768,  769,  772,  800, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
 {	{  78750, 1024, 1056, 1152, 1312,  768,  769,  772,  800, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
 {	{  94500, 1024, 1088, 1184, 1376,  768,  769,  772,  808, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
 {	{  94500, 1024, 1088, 1184, 1376,  768,  769,  772,  808, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1024,  768, 0, 0, MODE_FLAGS},
 
/* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
 {	{  94200, 1152, 1184, 1280, 1472,  864,  865,  868,  914, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
 {	{  94200, 1152, 1184, 1280, 1472,  864,  865,  868,  914, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
 {	{ 108000, 1152, 1216, 1344, 1600,  864,  865,  868,  900, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
 {	{ 108000, 1152, 1216, 1344, 1600,  864,  865,  868,  900, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
 {	{ 121500, 1152, 1216, 1344, 1568,  864,  865,  868,  911, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
 {	{ 121500, 1152, 1216, 1344, 1568,  864,  865,  868,  911, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1152,  864, 0, 0, MODE_FLAGS},
 
/* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
 {	{ 108000, 1280, 1344, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
 {	{ 108000, 1280, 1344, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
 {	{ 135000, 1280, 1312, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
 {	{ 135000, 1280, 1312, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
 {	{ 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
 {	{ 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS},
 
/* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
 {	{ 162000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 162000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
 {	{ 175500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 175500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
 {	{ 189000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 189000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
 {	{ 202500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 202500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
 {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
 {	{ 229500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB16_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS},
 {	{ 229500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC},	B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS}
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))

// conversion of 'depth' passed to 'setmode' to uint8s/pixel
static uint16 mode_bytpp[7] = { 0, 0, 1, 2, 2, 3, 4 };
static uint16 mode_bpp[7] = { 0, 0, 8, 15, 16, 24, 32 };

// amount of extra pixels to add to hsyncstart value as func of uint8s/pixel
static uint16 hsync_adj_tab[5] = { 0, 0x12, 9, 6, 5 };

// Table of register offsets and values to set them to that are common for
// all modes.  These registers are cleared so that they do not interfere
// with the setting of a display mode.
offset_value_struct common_regs_table[] =
    {
    { OVR_CLR,      0 },        // (ext'd) overscan color=0
    { OVR_WID_LEFT_RIGHT,   0 },// no (ext'd) overscan border
    { OVR_WID_TOP_BOTTOM,   0 },
    { OV0_SCALE_CNTL,   0 },    // disable overlay
    { SUBPIC_CNTL,      0 },    // disable subpic
    { VIPH_CONTROL,     0 },    // disable viph
    { I2C_CNTL_1,       0 },    // disable I2C
    { GEN_INT_CNTL,     0 },    // disable interrupts
    { CAP0_TRIG_CNTL,   0 }     // disable cap0
    };

offset_value_struct common_regs_table_m6[] =
    {
    { OVR_CLR,      0 },        // (ext'd) overscan color=0
    { OVR_WID_LEFT_RIGHT,   0 },// no (ext'd) overscan border
    { OVR_WID_TOP_BOTTOM,   0 },
    { OV0_SCALE_CNTL,   0 },    // disable overlay
    { SUBPIC_CNTL,      0 },    // disable subpic
    { DVI_I2C_CNTL_1,   0 },    // disable I2C m6-a 
    { GEN_INT_CNTL,     0 },    // disable interrupts
    { CAP0_TRIG_CNTL,   0 }     // disable cap0
    };

const int len_common_regs_table = sizeof (common_regs_table)/sizeof (common_regs_table[0]);
const int len_common_regs_table_m6 = sizeof (common_regs_table_m6)/sizeof (common_regs_table_m6[0]);

static pllinfo PLL_INFO;


static uint8 Radeon_PLLReadUpdateComplete ( CardInfo *ci );
static void Radeon_PLLWriteUpdate ( CardInfo *ci );
static uint8 GetPostDividerBitValue (uint8 PostDivider);
static uint32 Radeon_VClockValue (DeviceInfo *di);


/******************************************************************************
 * Function:   void Radeon_SetDPMS (WORD dpms_state)                            *
 *  Purpose:   set DPMS (Display Power Mgmt Signaling) mode                   *
 *    Input:   dpms_state: 0 = active                                         *
 *                         1 = standby                                        *
 *                         2 = suspend                                        *
 *                         3 = off                                            *
 *                         4 = blank the display                              *
 *                             (this is not a DPMS state;                     *
 *                             the DPMS state is left alone)                  *
 *  Returns:    none.                                                         *
 ******************************************************************************/
void Radeon_SetDPMS( CardInfo *ci, uint16 dpms_state)
{
	if (dpms_state > 4)
		return;
	
	// map the passed dpms_state values to hsync disable, vsync disable,
	// and blank bits in byte 1 of the CRTC_EXT_CNTL reg
	// bits<2:0> = Blank, VSyncDis, HSyncDis
	// (set other bits of this byte=0)
	
	// set blanking if dpms_state <> 0
	if (dpms_state == 4)
	{
		dpms_state = (READ_REG_8(CRTC_EXT_CNTL + 1) & 3) | 4;
	}
	else if (dpms_state != 0)
	{
		dpms_state |= 4;
	}

	if ( ci->isMobility )
		dpms_state |= 0x80;
	
	WRITE_REG_8 (CRTC_EXT_CNTL + 1, dpms_state);    // write the reg byte
} // Radeon_SetDPMS ()



/******************************************************************************
 * Function:   WORD Radeon_GetDPMS (void)                                       *
 *  Purpose:   Get the current DPMS mode                                      *
 *    Input:   none                                                           *
 *  Returns:   Current DPMS mode                                              *
 *                 0 = active                                                 *
 *                 1 = standby                                                *
 *                 2 = suspend                                                *
 *                 3 = off                                                    *
 ******************************************************************************/
uint32 Radeon_GetDPMS( CardInfo *ci )
{
    return READ_REG_8(CRTC_EXT_CNTL+1) & 3;
} // Radeon_GetDPMS ()

/******************************************************************************
 * MinBitsRequired (uint32 Value)                                              *
 *  Function: deterines the miniumum bits required to hold a particular       *
 *            value                                                           *
 *    Inputs: uint32 Value - value to be stored                                *
 *   Outputs: number of bits required to hold Value                           *
 ******************************************************************************/
static uint32 MinBitsRequired (uint32 Value)
{
    uint32 BitsReqd = 0;

    if (Value == 0)
    {
        BitsReqd = 1; // Take care for 0 value
    }

    while (Value)
    {
        Value >>= 1;  // Shift right until Value == 0
        BitsReqd++;
    }

    return (BitsReqd);
}   // MinBitsRequired ()

/******************************************************************************
 * Maximum (uint32 Value1, uint32 Value2)                                       *
 *  Function: determines the greater value of the two, and returns it.        *
 *    Inputs: uint32 Value1 - the first value to compare                       *
 *            uint32 Value2 - the second value to compare                      *
 *   Outputs: the greater of Value1 and Value2                                *
 ******************************************************************************/
static uint32 Maximum (uint32 Value1, uint32 Value2)
{

    if (Value1 >= Value2)
    {
        return Value1;
    }
    else
    {
        return Value2;
    }
}   // Maximum ()

/******************************************************************************
 * Minimum (uint32 Value1, uint32 Value2)                                       *
 *  Function: determines the lesser value of the two, and returns it.         *
 *    Inputs: uint32 Value1 - the first value to compare                       *
 *            uint32 Value2 - the second value to compare                      *
 *   Outputs: the lesser of Value1 and Value2                                 *
 ******************************************************************************/
static uint32 Minimum (uint32 Value1, uint32 Value2)
{
    if (Value1 <= Value2)
    {
        return Value1;
    }
    else
    {
        return Value2;
    }
}   // Minimum ()


/******************************************************************************
 * RoundDiv (uint32 Numerator, uint32 Denominator)                              *
 *  Function: returns the division of Numerator/Denominator with rounding.    *
 *    Inputs: uint32 Numerator - value to be divided                           *
 *            uint32 Denominator - value to be divided                         *
 *   Outputs: the rounded result of Numerator/Denominator                     *
 ******************************************************************************/
static uint32 RoundDiv (uint32 Numerator, uint32 Denominator)
{
    return (Numerator + (Denominator / 2)) / Denominator;
}   // RoundDiv ()

/******************************************************************************
 *  PLL_regw (BYTE index, DWORD data)                                         *
 *                                                                            *
 *  Function: This function will provide write access to the memory           *
 *            mapped PLL registers. Each register is 32 bits wide.            *
 *    Inputs: index - Register byte index into PLL registers                  *
 *            data - 32 bit data value to fill register.                      *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void PLL_regw (CardInfo *ci, uint8 index, uint32 data)
{
	WRITE_REG_8 (CLOCK_CNTL_INDEX, (index & CLOCK_CNTL_INDEX__PLL_ADDR_MASK)
		| CLOCK_CNTL_INDEX__PLL_WR_EN);
	WRITE_REG_32 (CLOCK_CNTL_DATA, data);
}

/***********************************************************************************
 *  PLL_regr (BYTE regindex)                                                       *
 *                                                                                 *
 *  Function: This function will provide read access to the PLL memory             *
 *            mapped registers. Each register is 32 bits wide.                     *
 *    Inputs: regindex - Register byte offset into PLL register block.             *
 *   Outputs: data - 32 bit contents of register.                                  *
 ***********************************************************************************/
uint32 PLL_regr (CardInfo *ci, uint8 regindex)
{
	WRITE_REG_8 (CLOCK_CNTL_INDEX, regindex & CLOCK_CNTL_INDEX__PLL_ADDR_MASK);
	return (READ_REG_32 (CLOCK_CNTL_DATA));
}

/******************************************************************************
 * Radeon_PLLGetDividers (uint16 Frequency)                                       *
 *  Function: generates post and feedback dividers for the desired pixel      *
 *            clock frequency. The resulting values are stored in the global  *
 *            PLL_INFO structure.                                             *
 *    Inputs: uint16 Frequency - the desired pixel clock frequency, in units    *
 *                             of 10 kHz.                                     *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void Radeon_PLLGetDividers (DeviceInfo *di, uint16 Frequency)
{
    uint32 FeedbackDivider;                    // Feedback divider value
    uint32 OutputFrequency;                    // Desired output frequency
    uint8 PostDivider = 0;                     // Post Divider for VCLK

    //
    // The internal clock generator uses a PLL feedback system to produce the desired frequency output
    // according to the following equation:
    //
    // OutputFrequency = (REF_FREQ * FeedbackDivider) / (REF_DIVIDER *  PostDivider)
    //
    // Where REF_FREQ is the reference crystal frequency, FeedbackDivider is the feedback divider
    // (from 128 to 255 inclusive), and REF_DIVIDER is the reference frequency divider.
    // PostDivider is the post-divider value (1, 2, 3, 4, 6, 8, or 12).
    //
    // The required feedback divider can be calculated as:
    //
    // FeedbackDivider = (PostDivider * REF_DIVIDER * OutputFrequency) / REF_FREQ
    //

    if (Frequency > di->ci->pll.PCLK_max_freq)
    {
        Frequency = (uint16)di->ci->pll.PCLK_max_freq;
    }

    if (Frequency * 12 < di->ci->pll.PCLK_min_freq)
    {
        Frequency = (uint16)di->ci->pll.PCLK_min_freq / 12;
    }

    OutputFrequency = 1 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 1;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 2 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 2;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 3 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 3;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 4 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 4;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 6 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 6;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 8 * Frequency;

    if ((OutputFrequency >= di->ci->pll.PCLK_min_freq) && (OutputFrequency <= di->ci->pll.PCLK_max_freq))
    {
        PostDivider = 8;
        goto _PLLGetDividers_OK;
    }

    OutputFrequency = 12 * Frequency;
    PostDivider = 12;

_PLLGetDividers_OK:


    //
    // OutputFrequency now contains a value which the PLL is capable of generating.
    // Find the feedback divider needed to produce this frequency.
    //

    FeedbackDivider = RoundDiv (di->ci->pll.PCLK_ref_divider * OutputFrequency, di->ci->pll.PCLK_ref_freq);

    PLL_INFO.fb_div   = (uint16)FeedbackDivider;
    PLL_INFO.post_div = (uint8)PostDivider;

    return;

}   // Radeon_PLLGetDividers()

/******************************************************************************
 * Radeon_ProgramPLL (void)                                                     *
 *  Function: programs the related PLL registers for setting a display mode.  *
 *            The values used have been calculated in previous functions.     *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void Radeon_ProgramPLL (DeviceInfo *di)
{
	CardInfo *ci = di->ci;
    uint8   Address;
    uint32  Data, Temp, Mask;

    //
    // Select Clock 3 to be the clock we will be programming (bits 8 and 9).
    //
    Temp  = READ_REG_32 (CLOCK_CNTL_INDEX);
    Temp |= CLOCK_CNTL_INDEX__PPLL_DIV_SEL_MASK;	// Select Clock 3

    WRITE_REG_32 (CLOCK_CNTL_INDEX, Temp);

    //
    // Reset the PPLL.
    // But there is no need to set SRC_SEL to CPUCLK (David Glen).
    //
    Address = PPLL_CNTL;
    Data    = PPLL_CNTL__PPLL_RESET | PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN 
            | PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN;
    Mask    = ~(PPLL_CNTL__PPLL_RESET | PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN 
            | PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN);

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

    //
    // Write the reference divider.
    //
    Address = PPLL_REF_DIV;
    Data    = (uint32)di->ci->pll.PCLK_ref_divider;
    Mask    = ~PPLL_REF_DIV__PPLL_REF_DIV_MASK;

    while (Radeon_PLLReadUpdateComplete(ci) == FALSE);

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

    Radeon_PLLWriteUpdate (ci);

    //
    // Write to the feedback divider.
    //
    Address = PPLL_DIV_3;
    Data    = PLL_INFO.fb_div;
    Mask    = ~PPLL_DIV_3__PPLL_FB3_DIV_MASK;

    while (Radeon_PLLReadUpdateComplete (ci) == FALSE);

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

    Radeon_PLLWriteUpdate (ci);

    //
    // Load the post divider value program according to CRTC
    //
    Address = PPLL_DIV_3;
    Data    = (uint32)GetPostDividerBitValue ((uint8)PLL_INFO.post_div);
    Data    = Data << PPLL_DIV_3__PPLL_POST3_DIV__SHIFT;
    Mask    = ~PPLL_DIV_3__PPLL_POST3_DIV_MASK;

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

    Radeon_PLLWriteUpdate (ci);

    while (Radeon_PLLReadUpdateComplete (ci) == FALSE);

    //
    // Zero out HTOTAL_CNTL.
    //
    Address = HTOTAL_CNTL;
    Data    = 0x0;

    while (Radeon_PLLReadUpdateComplete (ci) == FALSE);

    PLL_regw (ci, Address, Data);

    Radeon_PLLWriteUpdate (ci);

    //
    // No need to set the SRC_SEL to PPLCLK
    // Just clear the reset (just in case).
    //
    Address = PPLL_CNTL;
    Data    = 0;
    Mask    = ~PPLL_CNTL__PPLL_RESET;

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

    return;

}   // Radeon_ProgramPLL ()


/******************************************************************************
 * Radeon_PLLReadUpdateComplete (void)                                          *
 *  Function: polls the PPLL_ATOMIC_UPDATE_R bit to ensure the PLL has        *
 *            completed the atomic update.                                    *
 *    Inputs: NONE                                                            *
 *   Outputs: TRUE if PLL update complete                                     *
 *            FALSE if PLL update still pending                               *
 ******************************************************************************/
static uint8 Radeon_PLLReadUpdateComplete ( CardInfo *ci )
{
    uint8 Address;
    uint32 Result;

    Address = PPLL_REF_DIV;
    Result  = PLL_regr (ci, Address);

    if (PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_R & Result)
    {
        return (FALSE); // Atomic update still pending do not write
    }
    else
    {
        return (TRUE);  // Atomic update complete, ready for next write
    }
}   // Radeon_PLLReadUpdateComplete ()

/******************************************************************************
 * Radeon_PLLWriteUpdate (void)                                                 *
 *  Function: sets the PPLL_ATOMIC_UPDATE_W bit to inform the PLL that a      *
 *            write has taken place.                                          *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
static void Radeon_PLLWriteUpdate ( CardInfo *ci )
{
    uint8 Address;
    uint32 Data, Temp, Mask;

    Address = PPLL_REF_DIV;
    Data    = PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_W;
    Mask    = ~PPLL_REF_DIV__PPLL_ATOMIC_UPDATE_W;

    while (Radeon_PLLReadUpdateComplete (ci) == FALSE);

    Temp = PLL_regr (ci, Address);
    Temp &= Mask;
    Temp |= Data;
    PLL_regw (ci, Address, Temp);

}   // Radeon_PLLWriteUpdate ()

/******************************************************************************
 * GetPostDividerBitValue (uint8 PostDivider)                                  *
 *  Function: returns the actual value that should be written to the register *
 *            for a given post divider value.                                 *
 *    Inputs: NONE                                                            *
 *   Outputs: value to be written to the post divider register.               *
 ******************************************************************************/
static uint8 GetPostDividerBitValue (uint8 PostDivider)
{
    uint8 Result;

    switch (PostDivider)
    {
        case 1:     Result = 0x00;
                    break;
        case 2:     Result = 0x01;
                    break;
        case 3:     Result = 0x04;
                    break;
        case 4:     Result = 0x02;
                    break;
        case 6:     Result = 0x06;
                    break;
        case 8:     Result = 0x03;
                    break;
        case 12:    Result = 0x07;
                    break;
        default:    Result = 0x02;
    }

    return Result;
}   // GetPostDividerBitValue()


/******************************************************************************
 * Radeon_ProgramDDAFifo (uint32 BitsPerPixel)                                   *
 *  Function: programs the DDA_ON_OFF and DDA_CONFIG registers based on the   *
 *            the current mode, pixel and memory clock.                       *
 *    Inputs: uint32 BitsPerPixel - bpp of the display mode to be set.         *
 *   Outputs: TRUE if successful                                              *
 *            FALSE if a value is out of range                                *
 ******************************************************************************/
uint8 Radeon_ProgramDDAFifo (DeviceInfo *di, uint32 BitsPerPixel)
{
	CardInfo *ci = di->ci;
    uint32 DisplayFifoWidth = 128;
    uint32 DisplayFifoDepth = 32;
    uint32 XclksPerTransfer;
    uint32 MinBits;
    uint32 UseablePrecision;
    uint32 VclkFreq;
    uint32 XclkFreq;

    uint32 ML, MB, Trcd, Trp, Twr, CL, Tr2w, LoopLatency, DspOn;
    uint32 Ron, Roff, XclksPerXfer, Rloop, Temp;

    // adjust for 15 bpp (really 16 bpp!)
    if (BitsPerPixel == 15)
    {
        BitsPerPixel = 16;
    }

    //
    // Read CONFIG_STAT0 to find about the memory type for Display loop latency values
    //
//j    Temp  = regr (MEM_CNTL);
    Temp  = READ_REG_32 (MEM_SDRAM_MODE_REG);

    //
    // Check the type of memory and fill in the default values accordingly.
    //
    switch ((MEM_SDRAM_MODE_REG__MEM_CFG_TYPE & Temp) >> 30)
    {
        case 0:  // SDR SGRAM (2:1)

            ML          = 4;
            MB          = 4;
            Trcd        = 1;
            Trp         = 2;
            Twr         = 1;
            CL          = 2;
            Tr2w        = 1;
            LoopLatency = 16;
            DspOn       = 24;
            Rloop       = 12 + ML;
            break;

        case 1:  // DDR SGRAM

            ML          = 4;
            MB          = 4;
            Trcd        = 3;
            Trp         = 3;
            Twr         = 2;
            CL          = 3;
            Tr2w        = 1;
            LoopLatency = 16;
            DspOn       = 31;
            Rloop       = 12 + ML;
            break;

        default:

            //
            // Default to 64-bit, SDR-SGRAM values.
            //
            ML          = 4;
            MB          = 8;
            Trcd        = 3;
            Trp         = 3;
            Twr         = 1;
            CL          = 3;
            Tr2w        = 1;
            LoopLatency = 17;
            DspOn       = 46;
            Rloop       = 12 + ML + 1;
    }

    //
    // Determine the XCLK and VCLK values in MHz.
    //
    VclkFreq = Radeon_VClockValue ( di );
    XclkFreq = di->ci->pll.XCLK;

    //
    // Calculate the number of XCLKS in a transfer.
    //
    XclksPerTransfer = RoundDiv (XclkFreq * DisplayFifoWidth, VclkFreq * BitsPerPixel);

    //
    // Calculate the minimum number of bits needed to hold the integer
    // portion of XclksPerTransfer.
    //
    MinBits = MinBitsRequired (XclksPerTransfer);

    //
    // Calculate the useable precision.
    //
    UseablePrecision = MinBits + 1;

    //
    // Calculate the number of XCLKS in a transfer without precision loss.
    //
    XclksPerXfer = RoundDiv ((XclkFreq * DisplayFifoWidth) << (11 - UseablePrecision),
                            VclkFreq * BitsPerPixel);

    //
    //  Display FIFO on point.
    //
    Ron = 4 * MB +
            3 * Maximum (Trcd - 2, 0) +
            2 * Trp +
            Twr +
            CL +
            Tr2w +
            XclksPerTransfer;

    Ron = Ron << (11 - UseablePrecision);     // Adjust on point.

    //
    // Display FIFO off point.
    //
    Roff = XclksPerXfer * (DisplayFifoDepth - 4);

    //
    // Make sure that ulRon + ulRloop < ulRoff otherwise the mode is not
    //  guarenteed to work.
    //

    if ((Ron + Rloop) >= Roff)
    {
        // Value if out of range.
        return (FALSE);
    }

    //
    // Write values to registers.
    //

    Temp  = UseablePrecision << 16;
    Temp |= Rloop << 20;
    Temp |= XclksPerXfer;

//j    regw (DDA_CONFIG, Temp);

    Temp  = Ron << 16;
    Temp |= Roff;

//j    regw (DDA_ON_OFF, Temp);

    return TRUE;
}   // Radeon_ProgramDDAFifo ()


/******************************************************************************
 * Radeon_VClockValue (void)                                                    *
 *  Function: return the pixel clock value (in kHz) of the requested          *
 *            display mode.                                                   *
 *    Inputs: NONE                                                            *
 *   Outputs: pixel clock frequency, in kHz                                   *
 *            0 if an error condition is found (CRTC is off)                  *
 ******************************************************************************/
static uint32 Radeon_VClockValue (DeviceInfo *di)
{
	CardInfo *ci = di->ci;
    uint8   Address;
    uint32  Temp, VClock;
    uint8   VClockDivider []  = {1,2,4,8,3,0,6,12};

    pllinfo VClockPLLDividers;

    //
    // Read the CRTC_GEN_CNTL Register
    //
    Temp = READ_REG_32 (CRTC_GEN_CNTL);

    //
    // Make sure that the CRTC is enabled
    //
    if (!(Temp & CRTC_GEN_CNTL__CRTC_EN))
    {
        return 0;
    }

    //
    // Read the feedback- and post- divider.
    //
    Address = PPLL_DIV_3;
    Temp    = PLL_regr (ci, Address);

    VClockPLLDividers.fb_div = (uint16)Temp & PPLL_DIV_3__PPLL_FB3_DIV_MASK;

    VClockPLLDividers.post_div = VClockDivider[(Temp
                               & PPLL_DIV_3__PPLL_POST3_DIV_MASK) >> 16];

    //
    // Get the reference divider value lower [0-7] bit
    //
    Address = PPLL_REF_DIV;
    Temp    = PLL_regr (ci, Address);

    di->ci->pll.PCLK_ref_divider = (uint16)Temp & PPLL_REF_DIV__PPLL_REF_DIV_MASK;

    if (VClockPLLDividers.post_div == 0 ||
        VClockPLLDividers.fb_div   == 0)
    {
        return 0;
    }

    VClock = RoundDiv ((uint32)di->ci->pll.PCLK_ref_freq * (uint32)VClockPLLDividers.fb_div,
             (uint32)di->ci->pll.PCLK_ref_divider * (uint32)VClockPLLDividers.post_div);

    return (VClock);
}   // Radeon_VClockValue ()

/****************************************************************************
 * Radeon_DisableAtomicUpdate (void)                                          *
 *  Function: clears the ATOMIC_UPDATE_ENABLE bits in PPLL_CNTL             *
 *    Inputs: NONE                                                          *
 *   Outputs: NONE                                                          *
 ****************************************************************************/
void Radeon_DisableAtomicUpdate (CardInfo *ci)
{
    uint32 v;

    v = PLL_regr (ci, PPLL_CNTL);
    v &= ~(PPLL_CNTL__PPLL_ATOMIC_UPDATE_EN | PPLL_CNTL__PPLL_VGA_ATOMIC_UPDATE_EN);
    PLL_regw (ci, PPLL_CNTL, v);
    return;
} // Radeon_DisableAtomicUpdate ()...


/******************************************************************************
 * void set_regs_common (void)                                                *
 *  Function: Clears common registers that may interfere with the setting     *
 *            of a display mode.                                              *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
void set_regs_common (CardInfo * ci)
{
	int i;
	if ( ci->isMobility )
	{
		for (i=0; i<len_common_regs_table_m6; ++i)
		{
			WRITE_REG_32 (common_regs_table_m6[i].offset, common_regs_table_m6[i].value);
		}
	}
	else
	{
		for (i = 0; i < len_common_regs_table; ++i)
		{
			WRITE_REG_32 (common_regs_table[i].offset, common_regs_table[i].value);
		}
	}
}

/******************************************************************************
 * uint8 Radeon_SetModeNB (uint16 xres, uint16 yres, uint16 depth,                   *
 *                      CRTCInfoBlock *paramtable)                            *
 *  Function: Sets a display mode without calling the BIOS.                   *
 *    Inputs: uint16 xres - requested X resolution                              *
 *            uint16 yres - requested Y resolution                              *
 *            uint16 depth - requested pixel depth, in Radeon format            *
 *                         2 = 8bpp, 3 = 15bpp (555), 4 = 16bpp (565),        *
 *                         6 = 32bpp (xRGB)                                   *
 *            CRTCInfoBlock *paramtable - the CRTC parameters for the         *
 *                                        requested mode.  See MAIN.H for     *
 *                                        the full definition.                *
 *   Outputs: Returns 1 on successful mode set,                               *
 *            0 on a failure.                                                 *
 ******************************************************************************/
uint8 Radeon_SetModeNB (DeviceInfo *di, display_mode *paramtable, uint16 depth )
{
	uint16 bytpp;
	uint32 v, w;
	uint32 bpp;
	CardInfo *ci = di->ci;
	
	//Radeon_GetPLLInfo ();

	if (depth < 2 || depth > 6)
	{
		dprintf(("Radeon:  SetModeNB bad depth \n" ));
		return (0);					  //ret fail if invalid depth
	}

	bytpp = mode_bytpp[depth];	  //set bytpp=uint8s/pixel of mode
	bpp = mode_bpp[depth];

	// check if specified horiz total and vert total are within range
	// that will fit in the register fields (ret fail if not)
	// [note: we won't bother checking all the other input parameters]
	if (paramtable->timing.h_total / 8 - 1 > 0x1FF || paramtable->timing.v_total - 1 > 0x7FF)
	{
		dprintf(("Radeon: SetModeNB bad parmtable \n" ));
		return (0);
	}

	Radeon_SetDPMS( ci, 4 );					  // blank the screen
	
WRITE_REG_32( MC_FB_LOCATION, 0 | (0x3ff << 16) );
WRITE_REG_32( MC_AGP_LOCATION, 0x8000 | (0x83ff << 16) );
	
	set_regs_common (ci);			  // set registers that are common for all modes

	// get value for CRTC_GEN_CNTL reg:
	v = CRTC_GEN_CNTL__CRTC_EXT_DISP_EN | CRTC_GEN_CNTL__CRTC_EN
		| (depth << CRTC_GEN_CNTL__CRTC_PIX_WIDTH__SHIFT);
	// set extended display mode, crtc enbl,
	// cursor disabled, pixel width=depth,
	// no composite sync

//	if (paramtable->timing.flags & CI_DBLSCAN)
//	{
//		v |= CRTC_GEN_CNTL__CRTC_DBL_SCAN_EN;	//set doublescan bit
//	}

	if (paramtable->timing.flags & B_TIMING_INTERLACED)
	{
		v |= CRTC_GEN_CNTL__CRTC_INTERLACE_EN;	// set interlace bit
	}
	
	if( ci->showCursor )
		v |= CRTC_GEN_CNTL__CRTC_CUR_EN;

	WRITE_REG_32 (CRTC_GEN_CNTL, v);	  //set CRTC_GEN_CNTL reg

	// set CRTC_EXT_CNTL reg:
	// preserve hsync dis, vsync dis, blank bits,
	// set ati_linear, ext'd crtc display addr counter
	// (also disables flat panel outputs)
	v = READ_REG_32 (CRTC_EXT_CNTL);
	v &= (CRTC_EXT_CNTL__CRTC_HSYNC_DIS | CRTC_EXT_CNTL__CRTC_VSYNC_DIS
			| CRTC_EXT_CNTL__CRTC_DISPLAY_DIS);
	v |= (CRTC_EXT_CNTL__VGA_XCRT_CNT_EN | CRTC_EXT_CNTL__VGA_ATI_LINEAR);
v |= 0x8000;
	WRITE_REG_32 (CRTC_EXT_CNTL, v);

	// set DAC_CNTL reg:
	// preserve low 3 bits,
	// set dac 8-bit en, disable dac tvo, disable dac vga adr access
	// set dac mask=FF
	// [note: for depth>8bpp, must set dac_8bit_en; for depth=8bpp, may
	// or may not want dac set to 8 (instead of 6) bit
	v = READ_REG_32 (DAC_CNTL);
	v &= (DAC_CNTL__DAC_8BIT_EN | DAC_CNTL__DAC_RANGE_CNTL_MASK | DAC_CNTL__DAC_BLANKING);
	if (bpp == 8)
	{
		v |= DAC_CNTL__DAC_MASK_MASK;
	}
	else
	{
		v |= (DAC_CNTL__DAC_MASK_MASK | DAC_CNTL__DAC_8BIT_EN);
	}

	// enable DAC_VGA_ADDR_EN
	v |= DAC_CNTL__DAC_VGA_ADR_EN;

	WRITE_REG_32 (DAC_CNTL, v);

	// set CRTC_H_TOTAL_DISP reg:
	// (high uint16 = hdisp, low uint16 = htotal)
	WRITE_REG_32 (CRTC_H_TOTAL_DISP, ((uint32) (paramtable->virtual_width / 8 - 1)
									  << CRTC_H_TOTAL_DISP__CRTC_H_DISP__SHIFT) |
			(((paramtable->timing.h_total / 8) - 1) & 0xFFFFL));

	// get value for CRTC_H_SYNC_STRT_WID reg:
	// (high uint16=hsyncwid/polarity, low uint16=hsyncstart)
	// w = hsyncwidth = hsyncend - hsyncstart (/8 to convert to chars)
	w = (paramtable->timing.h_sync_end - paramtable->timing.h_sync_start) / 8;

	if (w == 0)
		w = 1;						  // make sure w at least 1 char

	if (w > 0x3F)
		w = 0x3F;					  // if > max size, set=max size

	// set bit 7 of w to corresponding horizontal polarity
	if ( !(paramtable->timing.flags & B_POSITIVE_HSYNC) )
	{
		w |= 0x80;
	}

	// v = hsyncstart(pixels)-8, then adjusted depending on uint8s/pixel
	v = (paramtable->timing.h_sync_start - 8) + hsync_adj_tab[bytpp];
	WRITE_REG_32 (CRTC_H_SYNC_STRT_WID, (w << CRTC_H_SYNC_STRT_WID__CRTC_H_SYNC_WID__SHIFT) | (v & 0xFFFFL));

	// set CRTC_V_TOTAL_DISP register:
	// (high uint16 = vdisp, low uint16 = vtotal)
	v = paramtable->virtual_height;

//	if (paramtable->timing.flags & CI_DBLSCAN)
//	{
//		v *= 2;						  // Double scan enabled.
//	}
	WRITE_REG_32 (CRTC_V_TOTAL_DISP, ((v - 1) << CRTC_V_TOTAL_DISP__CRTC_V_DISP__SHIFT)
			| ((paramtable->timing.v_total - 1) & 0xFFFFL));

	// get value for CRTC_V_SYNC_STRT_WID register:
	// (high uint16 = vsyncwid / polarity, low uint16 = vsyncstart)

	// w = vsyncwidth = vsyncend - vsyncstart
	w = (paramtable->timing.v_sync_end - paramtable->timing.v_sync_start);

	if (w == 0)
	{
		w = 1;						  // make sure w is at least 1 char
	}

	if (w > 0x1F)
	{
		w = 0x1F;					  // if w > max size, set to max size
	}

	// set bit 7 of w to corresponding vertical polarity
	if ( !(paramtable->timing.flags & B_POSITIVE_VSYNC) )
	{
		w |= 0x80;
	}

	WRITE_REG_32 (CRTC_V_SYNC_STRT_WID, (w << CRTC_V_SYNC_STRT_WID__CRTC_V_SYNC_WID__SHIFT)
			| ((paramtable->timing.v_sync_start - 1) & 0xFFFFL));

	WRITE_REG_32 (CRTC_OFFSET, 0);		  // set CRTC_OFFSET = 0
	WRITE_REG_32 (CRTC_OFFSET_CNTL, 0);  // set CRTC_OFFSET_CNTL = 0
	WRITE_REG_32 (CRTC_PITCH, paramtable->virtual_width >> 3);	// set CRTC_PITCH = xres / 8

	// set the PLL parms, display DDA parms, clock select
	// (also ecp div, htotal_cntl)
	Radeon_PLLGetDividers (di, paramtable->timing.pixel_clock /10);
	Radeon_ProgramPLL (di);
	Radeon_ProgramDDAFifo (di, (uint32) bpp);

	Radeon_DisableAtomicUpdate (ci);

	if (bpp == 8)
	{
//		Radeon_InitPalette ();
	}
	else
	{
//		Radeon_InitGamma (di, di->gamma_R, di->gamma_G, di->gamma_B );
	}

	Radeon_SetDPMS(ci,0);					  // turn screen back on

	return (1);

}										  // Radeon_SetModeNB ()

uint8 Radeon_SetDisplayMode( DeviceInfo *di, display_mode *dm )
{
	int32 ct;
	int32 bpp;
//	int32 refreshrate = ((dm->timing.pixel_clock * 1000) / (dm->timing.h_total * dm->timing.v_total)) + 1;
	
dprintf(( "Radeon_SetDisplayMode  %ld X %ld \n", dm->virtual_width, dm->virtual_height ));
	switch( dm->space )
	{
	default:
		return 0;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		bpp = 6; break;
		
	case B_RGB16_LITTLE:
	case B_RGB16_BIG:
		bpp = 4; break;

	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
		bpp = 3; break;
	}
dprintf(( "Radeon_SetDisplayMode  bpp = %ld \n", bpp ));
	
	for( ct=0; ct<NMODES; ct++ )
	{
		if( (mode_list[ct].virtual_width == dm->virtual_width) &&
			(mode_list[ct].virtual_height == dm->virtual_height) )
			break;
	}
	if( ct == NMODES )
		return 0;
dprintf(( "Radeon_SetDisplayMode  mode=%ld \n", ct ));
		
	
	Radeon_SetModeNB ( di, dm, bpp );
//	if( !Radeon_SetModeNB ( di, ct, bpp ) )
//		return 0;

	di->ci->FBStride = dm->virtual_width;
dprintf(( "Radeon_SetDisplayMode  stride=%ld \n", di->ci->FBStride ));
	switch( dm->space )
	{
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		di->ci->FBStride *= 4; break;
		
	case B_RGB16_LITTLE:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		di->ci->FBStride *= 2; break;
	}
		
dprintf(( "Radeon_SetDisplayMode  stride=%ld \n", di->ci->FBStride ));
	return 1;
}

uint32 Radeon_GetModeCount( DeviceInfo *di )
{
dprintf(( "Radeon_GetModeCount returning 0x%x \n", NMODES ));
	return NMODES;
}

const void * Radeon_GetModeList( DeviceInfo *di )
{
	return mode_list;
}

extern uint8 Radeon_Set2NdMode( DeviceInfo *di, int32 w, int32 h, int32 hz, int32 is32bpp )
{
	int32 ct;
	int32 bpp;
	int32 close_hz_mode = NMODES;
	int32 close_hz = 0;
	
dprintf(( "RADEON: Radeon_Set2NdMode w=%ld  h=%ld  hz=%ld  32bpp=%ld\n", w, h, hz, is32bpp ));
	for( ct=0; ct<NMODES; ct++ )
	{
		if( (mode_list[ct].virtual_width == w) && (mode_list[ct].virtual_height == h) )
		{
			if( mode_hz[ct] == hz )
			{
				close_hz_mode = ct;
				break;
			}
			
			if( abs(mode_hz[ct] - hz) < abs( mode_hz[ct] - close_hz ) )
			{
				close_hz = mode_hz[ct];
				close_hz_mode = ct;
			}
		}
	}

	ct = close_hz_mode;
dprintf(( "RADEON: Radeon_Set2NdMode mode # %ld\n", ct ));
	if( ct == NMODES )
		return 0;
		
	if( is32bpp )
		bpp = 6;
	else
		bpp = 4;
	
	Radeon_SetModeNB ( di, &mode_list[ct], bpp );

	return 1;
}


