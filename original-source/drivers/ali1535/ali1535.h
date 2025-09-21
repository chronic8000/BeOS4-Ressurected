//*******************************************************************
//  "REG4620.H"--CS4620 PCI audio chip information.
//               Copyright (c) 1998 Crystal Semiconductor Corp.
//               Ver 0.92 5feb99--minor changes for compliance
//               with "PCI Audio Design Guide for Embedded Systems"
//               ver 0.95 3mar99  -- changes for capture.
//               ver 0.96 25mar99 -- Improve capture quality.
//
//*******************************************************************

#ifndef ali1535_h
#define ali1535_h

#include <SupportDefs.h>
#include <KernelExport.h>
#include <Drivers.h>

#define PCI_VENDOR_ID_ALI				0x10b9
#define PCI_DEVICE_ID_ALI_M1535_AUDIO	0x5451
#define ALI_EXTENT_ENH					0x100
#define ALI_START						0x80
#define ALI_STOP						0x84
#define ALI_GC_CIR						0xa0
#define ALI_CH_CSO_ALPHA_FMS			0xe0
#define ALI_CH_LBA						0xe4
#define ALI_CH_ESO_DELTA				0xe8
#define ALI_CH_GVSEL_PAN_VOL_CTRL_EC	0xf0
#define ALI_ACR0_AC97_W					0x40
#define ALI_ACR0_AC97_WD				0x42
#define ALI_ACR0_AC97_R					0x44
#define ALI_ACR0_AC97_RD				0x46
#define ALI_AC97_BUSY					0x8000
#define ALI_SCTRL						0x48
#define ALI_AINTEN						0xa4
#define ALI_AINT						0x98
#define ALI_MPUR0						0x20
#define ALI_MPUR1						0x21
#define ALI_MPUR2						0x22
#define ALI_MPUR3						0x23
#define ALI_GLOBAL_CONTROL				0xd4
#define ALI_MUSICVOL_WAVEVOL			0xa8
#define ALI_EBUF1						0xf4
#define ALI_EBUF2						0xf8
#define ALI_NPLAYCHANNELS				31
#define ALI_NRECCHANNELS				1
#define ALI_PLAYBACK_CHANNEL			0
#define ALI_RECORD_CHANNEL				31
#define ALI_MISCINT						0xb0
#define ALI_STIMER						0xc8

#define ALI_FMT_16BITU					0x8000
#define ALI_FMT_SIGNED					0x2000
#define ALI_FMT_STEREO					0x4000
#define ALI_FMT_16BIT					0xa000

#define ALI_FMT_MASK					0xe000

#define ALI_MIDLP_ENDLP_IE				0x30
#define ALI_STILL_MODE					0x30000000
#define ALI_PCM_IN_ENABLE				0x80000000
#define ALI_AINTEN_DISABLE				0x00000000
#define ALI_AINT_RESET					0xffffffff
#define ALI_MIX_RECORD_GAIN_0dB			0x0000
#define ALI_MIX_PCBEEP_MUTE				0x8000
#define ALI_MIX_RECSEL_MIC				0x0000
#define ALI_MUSICVOL_WAVEVOL_0dB		0x00000000

#define ALI_CENABLE_PPE					0x4
#define ALI_CENABLE_RE					0x2
#define ALI_CENABLE_PE					0x1
#define ALI_CENABLE_CLEAR				0x8

#endif  // ali1535_h
