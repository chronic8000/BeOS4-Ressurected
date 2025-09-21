/* Based on work by F.M.Birth. */

/****************************************************************************
* Information in this file is the intellectual property of                  *
* TerraTec Electronic GmbH, and contains trade secrets that must be         *
* stored and viewed confidentially.                                         *
*                                                                           *
* Copyright (C) 1999 TerraTec Electronic GmbH. All Rights Reserved.         *
****************************************************************************/

#include "iic.h"
#include "thirdparty.h"

/*=============================================================================
     CONSTANTS
=============================================================================*/
// PCF8574 pin usage as connected at PCF8574
  #define	PCF8574_P0_CODEC1_CS 0x01	// AK4525 Codec 1 chip select
  #define	PCF8574_P1_CODEC2_CS 0x02	// AK4525 Codec 2 chip select
  #define	PCF8574_P2_CODEC3_CS 0x04	// AK4525 Codec 3 chip select
  #define	PCF8574_P3_CODEC4_CS 0x08	// AK4525 Codec 4 chip select
//#define	PCF8574_P4_RESERVED  0x10	// unused	
//#define	PCF8574_P5_RESERVED  0x20	// unused	
  #define	PCF8574_P6_OUT_SEL   0x40
//#define	PCF8574_P7_RESERVED	 0x80    // unused

// Register offsets
enum {
	AK4525_REG_POWER,
	AK4525_REG_RESET,
	AK4525_REG_FORMAT,
	AK4525_REG_DEEMPHASIS,
	AK4525_REG_LEFT_INPUT,
	AK4525_REG_RIGHT_INPUT,
	AK4525_REG_LEFT_OUTPUT,
	AK4525_REG_RIGHT_OUTPUT
};

#define AK4524_SMUTE_SETTINGS 0x01 // emphasis off, Timeout = 256fs

static status_t ak4524_set_chip_select(envy24_dev* card, uint8 chip_select_mask)
{
	status_t err;
	uint8 data_byte;
	
	// check validity
	if (chip_select_mask > 0x0F)
		return B_BAD_VALUE;
		
	// Format data byte
	data_byte = (~chip_select_mask) & 0x0F;
	// FMB PCF8574 HIGH means -10dbV, 
	//     OutputLevelSwitch is 1 for +4dbu
	//if (!(+4dbu selected))
	data_byte += PCF8574_P6_OUT_SEL;
	
	// Set Chip select to LOW for selected Codecs (via IIC)
	err = iic_write_byte(card, PCF8574_CODEC_IIC_ADDRESS, data_byte);
	
	ddprintf(("ak4524_set_chip_select(%p) returning 0x%08lx\n", card, err));

	return err;
}
	
static status_t ak4524_write_register(envy24_dev* card, uint8 codec_mask,
	uint8 register_index, uint8 data)
{
	status_t err;
	uint16 command;
	uint8 i;
	uint8 temp;	
	
	// format buffer
	command = 	0xA000 +							// chip address + R/W
				(((uint16)register_index) << 8) +	// register address
				data;
				
	// start write cycle
	err = ak4524_set_chip_select(card, codec_mask);
	if (err < B_OK) goto exit;
	
	for (i = 0; i < 16; i++) {
		temp = (command & 0x8000) ? 0x01 : 0x00;
		iic_set_iic(card, temp, 0x00);
		iic_set_iic(card, temp, 0x01);	// data is clocked in on rising edge of CCLK
		command <<= 1;
	}
	
	// leave data line HIGH (default for IIC)
	iic_set_iic(card, temp, 0); 
	
	// end write cycle
	err = ak4524_set_chip_select(card, 0x00);
	if (err < B_OK) goto exit;
	
	// default
	iic_set_iic(card, 0x01, 0x01);	// data is clocked in on rising edge of CCLK

exit:
	ddprintf(("ak4524_write_register(%p) returning 0x%08lx\n", card, err));

	return err;
}

// TT: unfinished. Useful?
uint8 ak4524_read_register(envy24_dev* card, uint8 codec_mask,
	uint8 register_index)
{
	status_t err;
	uint8 i;
	uint8 value = 0xBE;
	
	(void)register_index;	// unused
	
	// start read cycle
	err = ak4524_set_chip_select(card, codec_mask);
	if (err < B_OK) goto exit;
	
	for (i = 0; i < 16; i++) {
	}	

exit:
	return value;
}

status_t ak4524_reset(envy24_dev* card, int32 codec_index)
{
	status_t err;
	uint8 codec_mask = 0;
	
	if (codec_index == -1) {
		// select all codecs
		codec_mask = 0x0F;
	} else {
		codec_mask = 0x01 << codec_index;	
	}
	
	// reset DACs & ADCs
	err = ak4524_write_register(card, codec_mask, AK4525_REG_RESET,
		0x00);
	if (err < B_OK) goto exit;
		
	// set to use IIS, 256 fsn, normal speed
	err = ak4524_write_register(card, codec_mask, AK4525_REG_FORMAT,
		0x60);
	if (err < B_OK) goto exit;
		
	// set DACs & ADCs to normal operation
	err = ak4524_write_register(card, codec_mask, AK4525_REG_RESET,
		0x03);
	if (err < B_OK) goto exit;
		
	// turn power on
	err = ak4524_write_register(card, codec_mask, AK4525_REG_POWER,
		0x07);
	if (err < B_OK) goto exit;
		
	// set soft mute timeout to short (?)
	err = ak4524_write_register(card, codec_mask, AK4525_REG_DEEMPHASIS,
		AK4524_SMUTE_SETTINGS);
	if (err < B_OK) goto exit;
		
exit:
	ddprintf(("ak4524_reset(%p) returning 0x%08lx\n", card, err));
	
	return err;
}

status_t ak4524_set_double_mode(envy24_dev* card, bool double_mode)
{
	status_t err;
	uint8 value = 0x60;	// always IIS format and 256 fs
	
	if (double_mode) {
		value |= 0x01;	// double speed
		// value |= 0x04; // 512 fsn
	}
	
	// 0x0F -> set mode for all codecs
	err = ak4524_write_register(card, 0x0F, AK4525_REG_FORMAT, value);
	
	ddprintf(("ak4524_set_double_mode(%p, %d) returning %ld\n",
		card, double_mode, err));

	return err;
}

/*=============================================================================
  Function    : HW_AK4525_SetDeemphasis
-------------------------------------------------------------------------------
  Description : Switches Deemphasis on the DACs
  Returns     : UCHAR  -> TRUE / FALSE
  Parameters  : PCUST_HW_INSTANCE_DATA pDevExt -> 
              : UCHAR fOn -> 
-------------------------------------------------------------------------------
  Notes       : - Deemphasis is supported only for 32,44.1,48 kHz
                  For other samplerates deemphasisis is disabled
                - This command is always sent to all codecs!

                Register 03h layout:
                +-----------------------------------------------+
                | D7    D6    D5    D4    D3    D2    D1    D0  |
                +-----+-----+-----+-----+-----------+-----------+
                |SMUTE|  0     0  |ZCEI | ZTM1 ZTM0 | DEM1 DEM0 |
                +-----+-----+-----+-----+-----------+-----------+
                   - SMUTE - Soft mute: normal operation    =  0
                   - ZCEI  - ADC IPGA zero crossing enable  =  1
                   - ZTM   - Zero crossing timeout: 1024 fs = 10
=============================================================================*/

status_t ak4524_set_deemphasis(envy24_dev* card, bool on)
{
	status_t err;
	uint8 value = AK4524_SMUTE_SETTINGS; // zero crossing enable, timeout = 256 fs
	
	value &= ~0x03;	// clear the emphasis bits first
	
	if (on) {
		switch (card->sample_rate) {
		case 32000:
			value |= 0x03;
			break;
		case 44100:
			value |= 0x00;
			break;
		case 48000:
			value |= 0x02;
			break;
		default:
			value |= 0x01;	// switch deemphasis off
		}
	} else {
		value |= 0x01;	// switch deemphasis off
	}
	
	// 0x0F -> set mode for all codecs
	err = ak4524_write_register(card, 0x0F, AK4525_REG_DEEMPHASIS, value);
	
	return err;
}

/*=============================================================================
    FUNCTION:  HW_AK4525_SetInputLevel
-------------------------------------------------------------------------------
	PURPOSE:    
    PARAMETERS: 
		PDEVICE_EXTENSION pDevExt - Device extension for port address
		UCHAR bChannelMask        - a one in this bitfield selects a channel,
		                            0xFF for all channel
		UCHAR bLevel              - input level in range - 0..255 (-72..+18 dB)
		                            127 = 0 dB
                
	RETURNS:    
-------------------------------------------------------------------------------
	NOTES:        
		- The channel mask is not the codec mask!!!
=============================================================================*/

status_t ak4524_set_input_level(envy24_dev* card, uint8 channel_mask, uint8 level)
{
	status_t err = B_OK;
	int i;
	int shift;
	uint8 cmp;
	uint8 codec_mask;
	
	// all channels selected
	if (channel_mask == 0x0F) {
		err = ak4524_write_register(card, 0x0F, AK4525_REG_LEFT_INPUT, level);
		if (err < B_OK) goto exit;
		err = ak4524_write_register(card, 0x0F, AK4525_REG_RIGHT_INPUT, level);
		goto exit;	// we're done
	}
	
	for (i = 0; i < 4; i++) {
		// FMB: The following stuff looks eloquent but leave it because
		// optimization does strange things with the shift operation else!!!
		shift = 2*i;
		cmp = channel_mask >> shift;
		codec_mask = 0x01 << i;
		
		if (cmp & 0x01) {	// codec i, left channel
			ddprintf(("setting left input of codec %x to %d\n",
				codec_mask, level));
			err = ak4524_write_register(card, codec_mask,
				AK4525_REG_LEFT_INPUT, level);
			if (err < B_OK) goto exit;
		}
		
		if (cmp & 0x02) {	// codec i, right channel
			ddprintf(("setting right input of codec %x to %d\n",
				codec_mask, level));
			err = ak4524_write_register(card, codec_mask,
				AK4525_REG_RIGHT_INPUT, level);
			if (err < B_OK) goto exit;
		}
	}
	
exit:
	return err;
}

/*=============================================================================
    FUNCTION:  HW_AK4525_SetOutputLevel
-------------------------------------------------------------------------------
	PURPOSE:    
    PARAMETERS: 
		PCUST_HW_INSTANCE_DATA pDevExt    - pointer to config info
		UCHAR bChannelMask        - a one in this bitfield selects a channel,
		                            0xFF for all channels
		UCHAR bLevel              - output level in range - 0..127 (-72..0 dB)
		                            127 = 0 dB
                
	RETURNS:    
-------------------------------------------------------------------------------
	NOTES:        
		- The channel mask is not the codec mask!!!
		- The control panel only allows all outputs synchronously
		  or not at all ( if the digital mixer output attenuation is used)
=============================================================================*/
// Register 0x06/7 - Output Level

status_t ak4524_set_output_level(envy24_dev* card, uint8 channel_mask, uint8 level)
{
	status_t err = B_OK;
	int i;
	int shift;
	uint8 cmp;
	uint8 codec_mask;
	
	// all channels selected
	if (channel_mask == 0xFF) {
		err = ak4524_write_register(card, 0x0F, AK4525_REG_LEFT_OUTPUT, level);
		if (err < B_OK) goto exit;
		err = ak4524_write_register(card, 0x0F, AK4525_REG_RIGHT_OUTPUT, level);
		goto exit;	// we're done
	}

	for (i = 0; i < 4; i++) {
		// FMB: The following stuff looks eloquent but leave it because
		// optimization does strange things with the shift operation else!!!
		shift = 2*i;
		cmp = channel_mask >> shift;
		codec_mask = 0x01 << i;
		
		if (cmp & 0x01) {	// codec i, left channel
			ddprintf(("setting left output of codec %x to %d\n",
				codec_mask, level));
			err = ak4524_write_register(card, codec_mask,
				AK4525_REG_LEFT_OUTPUT, level);
			if (err < B_OK) goto exit;
		}
		
		if (cmp & 0x02) {	// codec i, right channel
			ddprintf(("setting right output of codec %x to %d\n",
				codec_mask, level));
			err = ak4524_write_register(card, codec_mask,
				AK4525_REG_RIGHT_OUTPUT, level);
			if (err < B_OK) goto exit;
		}
	}

exit:
	return err;
}

/*=============================================================================
    FUNCTION:  HW_AK4525_Initialize
-------------------------------------------------------------------------------
	PURPOSE:    Initialize a hot-plugged I/O board to the current HW settings 
    PARAMETERS: 
		        PCUST_HW_INSTANCE_DATA pDevExt    - pointer to config info
	RETURNS:    TRUE/FALSE
-------------------------------------------------------------------------------
	NOTES:        
=============================================================================*/

status_t ak4524_init(envy24_dev* card)
{
	status_t err;
	int i;
	uint8 channel_mask;

	err = ak4524_reset(card, -1);	// resets AK4525 and sets I/O levels to 0dB
	if (err < B_OK) goto exit;
	
	// set startup values on the I/O board
	// set initial input & output levels
	// TT: 127 is 0dB; should use values in envy24_dev* card
	for (i = 0; i < 8; i++) {
		channel_mask = 0x01 << i;
		err = ak4524_set_input_level(card, channel_mask, 127);
		if (err < B_OK) goto exit;
	}
	
	for (i = 0; i < 8; i++) {
		channel_mask = 0x01 << i;
		err = ak4524_set_output_level(card, channel_mask, 127);
		if (err < B_OK) goto exit;
	}

	// set input & output switches
	// TT: do I have this code? Maybe in missing Hardware.c
	
exit:
	ddprintf(("ak4524_init(%p) returning 0x%08lx\n", card, err));
	
	return err;
}
