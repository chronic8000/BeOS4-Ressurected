/*****************************************************************************
 *
 * Copyright (C) 1998-1999 IC Ensemble, Inc. All Rights Reserved.
 *
 ****************************************************************************/

#ifndef _EEPROM_H_
#define _EEPROM_H_

/* BE helpful */
#define DWORD uint32
#define BYTE  uint8
#define WORD  uint16


// Note: Customers should not change the definition of this EEPROM image.
// However, customer can append to the structure below any vendor specific
// fields. Please also adjust the value of "bSize" accordingly.

#define MAX_EEPROM_SIZE     256     // in bytes

// definition for version 1 EEPROM

#pragma pack(1)                     // assume byte packing

typedef struct _EEPROM {
    DWORD   dwSubVendorID;  // PCI[2C-2F], in BIG ENDIAN format
    BYTE    bSize;          // size of EEPROM image in bytes
    BYTE    bVersion;       // version equal 1 for this structure.
    BYTE    bCodecConfig;   // PCI[60]
    BYTE    bACLinkConfig;  // PCI[61]
    BYTE    bI2SID;         // PCI[62]
    BYTE    bSpdifConfig;   // PCI[63]
    BYTE    bGPIOInitMask;  // Corresponding bits need to be init'ed
                            // 0 means write bit and 1 means cannot write
    BYTE    bGPIOInitState; // Initial state of GPIO pins
    BYTE    bGPIODirection; // GPIO Direction State
    WORD    wAC97MainVol;
    WORD    wAC97PCMVol;
    WORD    wAC97RecVol;
    BYTE    bAC97RecSrc;
    BYTE    bDACID[4];      // I2S IDs for DACs
    BYTE    bADCID[4];      // I2S IDs for ADCs
} EEPROM, *PEEPROM;

#define EEPROM_SIZE    sizeof(EEPROM)
#define EEPROM_SIZE_OFFSET  4   // offset of size

typedef union {
    EEPROM      Eeprom;        
    BYTE        Bytes[EEPROM_SIZE];
} EEPROM_U;

// I2S converter properties:
// Since each I2S Codec is unique in its bit width, 96k sampling rate support,
// and volume control support, the following structure is used to store the
// various properties.
// A global table is built, and each codec is assigned an I2S id which is used 
// as an index to this table. In a particular implementation, the I2S id is 
// programmed in EEPROM for driver to retrieve.

typedef struct {
    WORD    wBitWidth;      // max. bit width of codec
    WORD    w96kSupport;    // enum. of 96k support
    WORD    wVolSupport;    // enum. of volume support
} I2S_CODEC;

// I2S IDs: I2SID_vvv_ppp
// where vvv is vendor name, and ppp specifies part no.

// TT: this doesn't seem to be used anywhere
typedef enum {
    I2SID_None,         // ID 0 is not assigned
    I2SID_AKM_AK4393,   // DAC   
    I2SID_AKM_AK5393,   // ADC
    I2SID_AKM_AK4524,	// Stereo CODEC used in some third party devices
    I2SID_MAX
} I2S_ID;

// 
typedef enum {
    I2S_96K_NONE = 0,       // no 96k support
    I2S_96K_GPIO,           // 96k sample rate is programmed via GPIO pins
                            // pin 0 = 1 when SR = 96k, pin 0 = 0 otherwise
    I2S_96K_MAX
} I2S_96K_SUPPORT;

typedef enum {
    I2S_VOL_NONE = 0,       // no volume control
    I2S_VOL_MAX
} I2S_VOL_SUPPORT;

#pragma pack()              // revert to default packing

#endif // _EEPROM_H_
