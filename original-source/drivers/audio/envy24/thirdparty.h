/*=============================================================================
   FILE:         terratec.h
-------------------------------------------------------------------------------
   COPYRIGHT:    (c) 2000, TerraTec Electronic GmbH. All Rights Reserved.

                 Information in this file is the intellectual property of
                 TerraTec Electronic GmbH, and contains trade secrets
                 that must be stored and viewed confidentially.
                 By viewing, using, modifying and compiling 
                 the programming code below you acknowledge you have read,
                 accepted and executed the Software licensing agreement 
                 your company has established with TerraTec Electronic GmbH.
=============================================================================*/

#ifndef _TERRATEC_H_
#define _TERRATEC_H_

#include "envy24.h"

/*=============================================================================
     CONSTANT DEFINITIONS
=============================================================================*/

// IIC addresses: +1 for read operations
//-----------------------------------------------------------------------------
#define PCF8574_SPDIF_IIC_ADDRESS 0x40	    // CS8404 (S/PDIF Transmitter control)
#define PCF8574_CODEC_IIC_ADDRESS 0x48	    // Codec control (chip selects)
#define PCF8574_INPUT_IIC_ADDRESS 0x46	    // Input Level switch control

#define PCF_EWSADAT_DIGITAL_IO    0x40      // EWS 88D PCF8575


#define ICE_VENDOR_ID	    0x1412		// IC Ensemble PCI vendor ID
#define ICE_DEVICE_ID	    0x1712		// IC Ensemble PCI vendor ID


// ICE1712 GPIO PIN MASK (as used in registers CCI20-22)
//-----------------------------------------------------------------------------
#define ICE_PIN_F0    0x01       // connected to CS8414 receiver
#define ICE_PIN_F1    0x02       // connected to CS8414 receiver
#define ICE_PIN_F2    0x04       // connected to CS8414 receiver
#define ICE_PIN_RW    0x08       // to signal a read from IIC port
		                         // HIGH: Write: LOW: read 
#define ICE_PIN_SDA   0x10       // connected to PCF8574
#define ICE_PIN_SCL   0x20       // connected to PCF8574

/* NOTE: The PCF8574s and the AK4525 share pins for different serial protocols */
#define ICE_PIN_CDTI  ICE_PIN_SDA // connected to AK4524 CODECS
#define ICE_PIN_CCLK  ICE_PIN_SCL // connected to AK4524 CODECS

#define kTerraTec_EWS88MT_ID 0x3b151511 // dev. 1115, vend. 153b
#define THIRD_PARTY_ID kTerraTec_EWS88MT_ID
/*=============================================================================
     Function Declarations
=============================================================================*/

status_t ak4524_reset(envy24_dev* card, int32 codec_index);
status_t ak4524_set_double_mode(envy24_dev* card, bool double_mode);
status_t ak4524_set_deemphasis(envy24_dev* card, bool on);
status_t ak4524_set_input_level(envy24_dev* card, uint8 channel_mask, uint8 level);
status_t ak4524_set_output_level(envy24_dev* card, uint8 channel_mask, uint8 level);
status_t ak4524_init(envy24_dev* card);

#endif // #ifndef _TERRATEC_H_
