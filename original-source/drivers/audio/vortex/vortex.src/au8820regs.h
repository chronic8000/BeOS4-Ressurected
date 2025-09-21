#ifndef _AU8820REGS_H
#define _AU8820REGS_H

#define VORTEX_VDB_CHANNEL_COUNT					(16)
#define VORTEX_WT_CHANNEL_COUNT						(32)
#define VORTEX_A3D_CHANNEL_COUNT					(1)
#define VORTEX_A3D_HW_CHANNEL_COUNT					(0)

#define VORTEX_FIFO_SIZE							(32)
#define VORTEX_VDB_FIFO_SIZE						(32)
#define VORTEX_WT_FIFO_SIZE							(32)

#define VORTEX_VDB_DESTINATION_COUNT				(103)
#define VORTEX_VDB_HEADER_COUNT						(22)

#define VORTEX_SRC_HEADER_COUNT						(22)
#define VORTEX_SRC_CHANNEL_COUNT					(16)

#define VORTEX_CHANNEL_COUNT						(48)
#define VORTEX_HOST_3D_WAVE_MAXIMUM_CHANNEL_COUNT	(8)
#define VORTEX_HOST_3D_WAVE_NON_PII_CHANNEL_COUNT	(6)
#define VORTEX_HOST_3D_WAVE_NON_MMX_CHANNEL_COUNT	(6)
#define VORTEX_CODEC_COUNT							(8)

#define VORTEX_TOTAL_WT_BLOCKS						(1)
#define VORTEX_HW_VOICES_PER_BLOCK					(32)

#define VORTEX_CONFIG_SPACE_SIZE	(0x20000)		/* sizeof regs in bytes */


#define VDB_FIFO_CHANNEL_COUNT       16
#define VDB_SRC_CHANNEL_COUNT        16
#define VDB_MIXERSRC_CHANNEL_COUNT   16
#define VDB_MIXERDEST_CHANNEL_COUNT  32
#define VDB_FX_CHANNEL_COUNT          8
#define VDB_CODEC_CHANNEL_COUNT       8
#define VDB_SERIAL_CHANNEL_COUNT      2
#define VDB_WT_CHANNEL_COUNT          4
#define VDB_PARALLEL_CHANNEL_COUNT    1

	
/* Major Block offsets for memory mapped access */
typedef enum
{
    VORTEX_WAVETABLE_OFFSET = 0x00000000,
    VORTEX_MIXER_OFFSET     = 0x00008000,
    VORTEX_SRC_OFFSET       = 0x0000c000,
    VORTEX_FIFO_OFFSET      = 0x0000e000,
    VORTEX_DMA_OFFSET       = 0x00010000,
    VORTEX_VDB_OFFSET       = 0x00010800,
    VORTEX_GAME_OFFSET      = 0x00011000,
    VORTEX_SERIAL_OFFSET    = 0x00011800,
    VORTEX_SB_OFFSET        = 0x00012000,
    VORTEX_PCI_OFFSET       = 0x00012800,
    VORTEX_I2C_OFFSET       = 0x00013000,
    VORTEX_PARALLEL_OFFSET  = 0x00014000,
}VORTEX_BLOCK_ADDRESS;

/* Major Block IDs */
typedef enum
{
    VORTEX_WAVETABLE_ID = 1,
    VORTEX_MIXER_ID     = 4,
    VORTEX_SRC_ID       = 2,
    VORTEX_FIFO_ID      = 0,
    VORTEX_DMA_ID       = 3,
    VORTEX_VDB_ID       = 5,
    VORTEX_GAME_ID      = 6,
    VORTEX_SERIAL_ID    = 7,
    VORTEX_SB_ID        = 0,
    VORTEX_PCI_ID       = 9,
    VORTEX_I2C_ID       = 0, 
    VORTEX_PARALLEL_ID  = 8,
}VORTEX_BLOCK_ID;


/*******************************************************************************
 * Wavetable Registers 
 *******************************************************************************/

typedef union
{
    struct
    {
	unsigned volatile Enable      : 1;
	unsigned volatile SrateSelect : 5;
	unsigned volatile Headroom    : 3;
	unsigned volatile Mode49152   : 1;
	unsigned             : 22;
    }Bits;
    uint32 volatile dwValue;
}WTcontrol;

/* Possible values for wt_enable */
#define WT_DISABLE 0
#define WT_ENABLE  1

typedef union
{
    struct
    {
		uint16 volatile Value;
		uint16 volatile Slope;
    }Bits;
    uint32 volatile dwValue;
}WTramp;

typedef union
{
    struct
    {
	unsigned volatile StereoPair : 1;
	unsigned            : 31;
    } Bits;
    uint32 volatile dwValue;
}WTstereo;

typedef union
{
    struct
    {
	unsigned volatile MutePending : 1;
	unsigned            : 31;
    }Bits;
    uint32 volatile dwValue;
}WTmute;

typedef struct
{
    union
    {
        struct
        {
	    unsigned volatile State : 1;
	    unsigned volatile Pitch : 15;
	    unsigned volatile Gain  : 8;
	    unsigned volatile FC    : 8;
        }Bits;
        uint32 volatile dwValue;
    }vs0;

    union
    {
        struct
        {
	    unsigned volatile State : 1;
	    unsigned volatile Pitch : 15;
	    unsigned volatile Gain  : 8;
	    unsigned volatile FC    : 8;
        }Bits;
        uint32 volatile dwValue;
    }vs1;

    union
    {
        struct
        {
	    unsigned short volatile PitchPhase;
	    unsigned short volatile FilterFeedbackGain;
        }Bits;
        uint32 volatile dwValue;
    }vs2;

    union
    {
        struct
        {
	    unsigned volatile Reverb          : 7;
	    unsigned volatile Chorus          : 7;
	    unsigned volatile Pan             : 7;
	    unsigned volatile GainState       : 3;
	    unsigned volatile FilterInputGain : 8;
        }Bits;
        uint32 volatile dwValue;
    }vs3;
}WTvoicestate;

typedef struct
{
    uint32   z1;
    uint32   z2;
    uint32   z3;
    uint32   z4;
}WTUnitDelay;

typedef struct
{
    WTcontrol Control;
    WTramp Sramp;
    uint32 reserved1;
    WTramp Mramp;
    uint32 GainMode;
    WTramp Aramp;
    uint32 reserved2[0x1a];
    WTstereo Stereo[(VORTEX_HW_VOICES_PER_BLOCK/2)];
    uint32 reserved3[16];
    WTmute Mute[VORTEX_HW_VOICES_PER_BLOCK];
    uint32 Running[VORTEX_HW_VOICES_PER_BLOCK];
    WTvoicestate VoiceState[VORTEX_HW_VOICES_PER_BLOCK];
    WTUnitDelay UnitDelay[VORTEX_HW_VOICES_PER_BLOCK];
    char filler[0x7a00];
}WTRegs;


/*******************************************************************************
 * Mixer Registers 
 *******************************************************************************/

#define NEXTVALID 0x10

typedef union
{
    struct
    {
	unsigned volatile Mantissa     : 3;
	unsigned volatile Exponent     : 5;
	unsigned volatile PreviousSign : 1;
	unsigned              : 23;
    }Bits;
    uint32 volatile dwValue;
}MixerGainSignReg; /* Table 9-3 */

typedef union
{
    struct
    {
	unsigned volatile EnableBit0 : 1;
	unsigned volatile EnableBit1 : 1;
	unsigned volatile EnableBit2 : 1;
	unsigned volatile EnableBit3 : 1;
    }Bits;
    uint32 volatile dwValue;
}MixerEnableReg; /* Table 9-5 */

typedef union
{
    struct
    {
    unsigned volatile SaturationFlag0      : 1;
    unsigned volatile SaturationFlag1      : 1;
    unsigned volatile SaturationFlag2      : 1;
    unsigned volatile SaturationFlag3      : 1;
    unsigned volatile SaturationFlag4      : 1;
    unsigned volatile SaturationFlag5      : 1;
    unsigned volatile SaturationFlag6      : 1;
    unsigned volatile SaturationFlag7      : 1;
    unsigned volatile SaturationFlag8      : 1;
    unsigned volatile SaturationFlag9      : 1;
    unsigned volatile SaturationFlag10     : 1;
    unsigned volatile SaturationFlag11     : 1;
    unsigned volatile SaturationFlag12     : 1;
    unsigned volatile SaturationFlag13     : 1;
    unsigned volatile SaturationFlag14     : 1;
    unsigned volatile SaturationFlag15     : 1;
    unsigned volatile AccOverflow          : 1;
    unsigned volatile OutputGainSaturation : 1;
    unsigned                      : 14;
    }Bits;
    uint32 volatile dwValue;
}MixerSaturationReg; /* Table 9-7 */

typedef union
{
    struct
    {
	unsigned volatile BlockChannel : 4;
	unsigned volatile NextValid    : 1;
    }Bits;
    uint32 volatile dwValue;
}MixerRAMLinkReg;

/* Indexes into the header tag array */
typedef enum
{
    MIXER_SR_SRC0 = 0,
    MIXER_SR_SRC1,
    MIXER_SR_SRC2,
    MIXER_SR_SRC3,
    MIXER_SR_SRC4,
    MIXER_SR_SRC5,
    MIXER_SR_SRC6,
    MIXER_SR_SRC7,
    MIXER_SR_SRC8,
    MIXER_SR_SRC9,
    MIXER_SR_SRC10,
    MIXER_SR_SRC11,
    MIXER_SR_SRC12,
    MIXER_SR_SRC13,
    MIXER_SR_SRC14,
    MIXER_SR_SRC15,
    MIXER_SR_FX,
    MIXER_SR_CODEC,
    MIXER_SR_AC97,
    MIXER_SR_SERIAL,
    MIXER_SR_SSTEP,
    MIXER_SR_DSPDMA,
}MIXER_SAMPLE_RATES;

typedef union
{
    struct
    {
	unsigned volatile SRC0   : 1;
	unsigned volatile SRC1   : 1;
	unsigned volatile SRC2   : 1;
	unsigned volatile SRC3   : 1;
	unsigned volatile SRC4   : 1;
	unsigned volatile SRC5   : 1;
	unsigned volatile SRC6   : 1;
	unsigned volatile SRC7   : 1;
	unsigned volatile SRC8   : 1;
	unsigned volatile SRC9   : 1;
	unsigned volatile SRC10  : 1;
	unsigned volatile SRC11  : 1;
	unsigned volatile SRC12  : 1;
	unsigned volatile SRC13  : 1;
	unsigned volatile SRC14  : 1;
	unsigned volatile SRC15  : 1;
	unsigned volatile FX     : 1;
	unsigned volatile CODEC  : 1;
	unsigned volatile AC97   : 1;
	unsigned volatile SERIAL : 1;
	unsigned volatile SSTEP  : 1;
	unsigned volatile DSPDMA : 1;
	unsigned        : 10;
    }Bits;
    uint32 volatile dwValue;
}MixerSampleRateActiveReg;


typedef union
{
    struct
    {
	unsigned volatile Data : 16;
	unsigned      : 16;
    }Bits;
    uint32 volatile dwValue;
}MixerDataReg; /* Table 9-6 */

typedef struct
{
    MixerGainSignReg CurrentInputGain[512];
    MixerGainSignReg CurrentOutputGain[16];
    char reserved0[0x07c0];
    MixerGainSignReg TargetInputGain[512];
    MixerGainSignReg TargetOutputGain[16];
    char reserved1[0x01c0];
    MixerEnableReg Enable[128];
    MixerDataReg Sample[96];
    char reserved2[0x0080];
    MixerRAMLinkReg ChannelLink[38];
    char reserved3[0x0068];
    MixerSampleRateActiveReg SRActive;
    char reserved4[0x7c];
    MixerSaturationReg Saturation;
    char filler[0x207c];
}MixerRegs; /* Table 9-2 */


/*******************************************************************************
 * Sample Rate Converter Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile data : 16;
	unsigned      : 16;
    }Bits;
    uint32 volatile dwValue;
}SRCData;

typedef union
{
    struct
    {
	unsigned volatile NextChannel  : 4;
	unsigned volatile ChannelValid : 1;
	unsigned              : 27;
    }Bits;
    uint32 volatile dwValue;
}SRCNextChannel;

typedef union
{
    struct
    {
	unsigned volatile Header       : 4;
	unsigned volatile ChannelValid : 1;
	unsigned              : 27;
    }Bits;
    uint32 volatile dwValue;
}SRCSrHeader;

typedef union
{
    struct
    {
	unsigned volatile CorrelatedChannel : 4;
	unsigned volatile SamplesPerWing    : 4;
	unsigned volatile DriftThrottle     : 1;
	unsigned volatile DriftLockSpeed    : 1;
    }cp0;
    struct
    {
	unsigned volatile ConversionRatio : 16;
	unsigned                 : 16;
    }cp1;
    struct
    {
	unsigned volatile DriftErrorAcc : 19;
	unsigned               : 13;
    }cp2;
    struct
    {
	unsigned volatile DriftLastError : 19;
	unsigned                : 13;
    }cp3;
    struct
    {
	unsigned volatile CurrentTimeFraction : 14;
	unsigned                     : 18;
    }cp4;
    struct
    {
	unsigned volatile DriftOutCount : 7;
	unsigned               : 25;
    }cp5;
    struct
    {
	unsigned volatile FIFOWritePointer      : 8;
	unsigned volatile FIFOReadPointer       : 8;
	unsigned volatile ThrottleDirectionFlag : 1;
	unsigned volatile FIFOTargetDepth       : 1;
	unsigned                       : 14; 
    }cp6;
    uint32 volatile dwValue;
}SRCChannelParams;

/* Possible values for ThrottleDirectionFlag */
#define THROTTLE_OUT 0
#define THROTTLE_IN  1

typedef struct
{
    SRCData InputFifo[16][32];
    SRCData OutputBuffer[16][2];
    char reserved0[0x380];
    SRCNextChannel NextChannel[VORTEX_SRC_CHANNEL_COUNT];
    SRCSrHeader SrHeader[VORTEX_SRC_HEADER_COUNT];
    char reserved1[0x28];
    uint32 SRActive;
    uint32 ThrottleSource;
    uint32 ThrottleCountSize;
    char reserved2[0x134];
    SRCChannelParams ChannelParams[7][16];
    char reserved3[0x40];
    char filler[0x1000];
}SRCRegs;


/*******************************************************************************
 * FIFO Registers 
 *******************************************************************************/

typedef union
{
	struct
	{
	unsigned volatile data   :16;
	unsigned        :16;
	}Bits;
	uint32 volatile dwValue;
}FIFOChannelDataReg; /* 5.6.2 */

typedef union
{
    struct
    {
	unsigned volatile SentDMAReq   :1;
	unsigned volatile Stereo       :1;
	unsigned volatile Priority     :2;
	unsigned volatile ChannelValid :1;
	unsigned volatile Empty        :1;
	unsigned volatile VDBPointer   :5;
	unsigned volatile DMAPointer   :5;
	unsigned volatile Wait4DMA     :1;
	unsigned volatile HadUnderrun  :1;
	unsigned volatile RTModFlag    :1;
	unsigned              :13;
    }Bits;
    uint32 volatile dwValue;
}FIFOVDBPointersAndControlReg; /* 5.6.4 */

/* Possible values for Stereo */
#define FIFO_MONO_DATA (0)
#define FIFO_STEREO_DATA (1)

typedef union
{
    struct
    {
	unsigned volatile SentDMAReq   :1;
	unsigned volatile DataInROM    :1;
	unsigned volatile Priority     :2;
	unsigned volatile ChannelValid :1;
	unsigned volatile Empty        :1;
	unsigned volatile WTPointer    :5;
	unsigned volatile DMAPointer   :5;
	unsigned volatile HadUnderrun  :1;
	unsigned volatile RTModFlag    :1;
	unsigned              :14;
    }Bits;
    uint32 volatile dwValue;
}FIFOWTPointersAndControlReg; /* 5.6.3 */

/* Possible values for Priority */
#define FIFO_LOW_PRIORITY (0)
#define FIFO_HIGH_PRIORITY (1)

typedef union
{
    struct
    {
	unsigned volatile WTTriggerOpt    :1;
	unsigned volatile VDBTriggerOpt   :1;
	unsigned volatile WTTriggerLevel  :5;
	unsigned volatile VDBTriggerLevel :5;
    }Bits;
    uint32 volatile dwValue;
}FIFOTriggerControlReg; /* 5.6.1 */


typedef struct
{
    FIFOChannelDataReg VDBData[ VORTEX_VDB_CHANNEL_COUNT ][ VORTEX_VDB_FIFO_SIZE ];
    FIFOChannelDataReg WTData[ VORTEX_WT_CHANNEL_COUNT ][ VORTEX_WT_FIFO_SIZE ];
    FIFOVDBPointersAndControlReg VDBPtrCtl[ VORTEX_VDB_CHANNEL_COUNT ];
    FIFOWTPointersAndControlReg WTPtrCtl[ VORTEX_WT_CHANNEL_COUNT ];
    FIFOTriggerControlReg trigger;
    char filler[0x073c];
}FIFORegs;

/*******************************************************************************
 * DMA Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile CurrentPosition :12;
	unsigned volatile InterruptEnable :1;
	unsigned volatile ReadWrite       :1;
	unsigned volatile DataFormat      :4;
	unsigned volatile DataInROM       :1;
	unsigned volatile SubbufError     :1;
	unsigned volatile SubbufErrorChk  :1;
	unsigned volatile CurrentSubbuf   :2;
	unsigned                 :9;
    }Bits;
    uint32 volatile dwValue;
}DmaWTChannelModeReg; /* 4.4.1 */
    
/* Possible values for DataInROM */
#define DMA_DATA_IN_MEM 0
#define DMA_DATA_IN_ROM 1

typedef union
{
    struct
    {
	unsigned volatile CurrentPosition :12;
	unsigned volatile InterruptEnable :1;
	unsigned volatile ReadWrite       :1;
	unsigned volatile DataFormat      :4;
	unsigned volatile SubbufError     :1;
	unsigned volatile SubbufErrorChk  :1;
	unsigned volatile CurrentSubbuf   :2;
	unsigned                 :10;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBChannelModeReg; /* 4.4.1 */
    
/* Possible values for InterruptEnable */
#define DMA_INTERRUPT_OFF (0)
#define DMA_INTERRUPT_ON (1)

/* Possible values for ReadWrite */
#define DMA_VORTEX_TO_HOST (0)
#define DMA_HOST_TO_VORTEX (1)

/* Possible values for DataFormat */
#define DMA_16_BIT_LINEAR (8)
#define DMA_8_BIT_LINEAR  (1)
#define DMA_8_BIT_ULAW    (2)
#define DMA_8_BIT_ALAW    (3)

typedef union
{
    struct
    {
	unsigned volatile SubbufPosition     :12;
	unsigned volatile CurrentSubbuf      :2;
	unsigned volatile NumMTFIFOS         :5;
	unsigned volatile PendingDMAReqID    :4;
	unsigned volatile PendingDMAReqValid :1;
	unsigned volatile CurrentDMAReqID    :4;
	unsigned volatile CurrentDMAReqValid :1;
	unsigned                    :3;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBChannelGetAbsolutePositionReg; /* 4.4.2 */

typedef union
{
    struct
    {
	unsigned volatile address :12;
	unsigned         :20;
    }Bits;
    uint32 volatile dwValue;
}DmaWTSubbufAddressReg; /* 4.4 */

typedef union
{
    struct
    {
	unsigned long volatile address;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBSubbufAddressReg; /* 4.4 */

typedef struct
{
    DmaWTSubbufAddressReg subbuf[4];
}DmaWTChannelAddressReg;

typedef struct
{
    DmaVDBSubbufAddressReg subbuf[4];
}DmaVDBChannelAddressReg;

typedef union
{
    struct
    {
	unsigned                :12;
	unsigned volatile Subbuf0Size    :12;
	unsigned volatile Subbuf1Next    :2;
	unsigned volatile Subbuf1GotoEna :1;
	unsigned volatile Subbuf1IntEna  :1;
	unsigned volatile Subbuf0Next    :2;
	unsigned volatile Subbuf0GotoEna :1;
	unsigned volatile Subbuf0IntEna  :1;
    }Bits;
    uint32 volatile dwValue;
}DmaWTChannelSizeAndGotoReg; /* 4.4.4 */

typedef union
{
    struct
    {
	unsigned volatile Subbuf1Size    :12;
	unsigned volatile Subbuf0Size    :12;
	unsigned volatile Subbuf1Next    :2;
	unsigned volatile Subbuf1GotoEna :1;
	unsigned volatile Subbuf1IntEna  :1;
	unsigned volatile Subbuf0Next    :2;
	unsigned volatile Subbuf0GotoEna :1;
	unsigned volatile Subbuf0IntEna  :1;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBChannelSizeAndGotoReg; /* 4.4.4 */

typedef struct
{
	DmaWTChannelSizeAndGotoReg sb0_1;
	DmaWTChannelSizeAndGotoReg sb2_3;
}DmaWTChannelSizeGoto;

typedef struct
{
	DmaVDBChannelSizeAndGotoReg sb0_1;
	DmaVDBChannelSizeAndGotoReg sb2_3;
}DmaVDBChannelSizeGoto;

typedef union
{
    struct
    {
	unsigned volatile Channel15SubBuffer :2;
	unsigned volatile Channel14SubBuffer :2;
	unsigned volatile Channel13SubBuffer :2;
	unsigned volatile Channel12SubBuffer :2;
	unsigned volatile Channel11SubBuffer :2;
	unsigned volatile Channel10SubBuffer :2;
	unsigned volatile Channel9SubBuffer  :2;
	unsigned volatile Channel8SubBuffer  :2;
	unsigned volatile Channel7SubBuffer  :2;
	unsigned volatile Channel6SubBuffer  :2;
	unsigned volatile Channel5SubBuffer  :2;
	unsigned volatile Channel4SubBuffer  :2;
	unsigned volatile Channel3SubBuffer  :2;
	unsigned volatile Channel2SubBuffer  :2;
	unsigned volatile Channel1SubBuffer  :2;
	unsigned volatile Channel0SubBuffer  :2;
    }Bits;
    uint32 volatile dwValue;
}DmaStatusReg; /* 4.4.5 */


typedef union
{
    struct
    {
	unsigned volatile AllBurstSize16       :1;
	unsigned volatile NewDmaReqID          :6;
	unsigned volatile NewDmaReqIDValid     :1;
	unsigned volatile CurrentDMAReqID      :6;
	unsigned volatile CurrentDMAReqIDValid :1;
	unsigned volatile WriteAllControl      :1;
	unsigned                      :16;
    }Bits;
    uint32 volatile dwValue;
}DmaEngineControlReg; /* 4.4.6 */

typedef union
{
    struct
    {
	unsigned volatile Prio1Status :11;
	unsigned volatile Prio0Status :11;
	unsigned             :10;
    }Bits;
    uint32 volatile dwValue;
}DmaWTQueueLinkedListPointersReg; /* 4.4.7 */

typedef union
{
    struct
    {
	unsigned volatile Prio1Status :11;
	unsigned volatile Prio0Status :11;
	unsigned             :10;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBQueueLinkedListPointersReg; /* 4.4.7 */

typedef union
{
    struct
    {
	unsigned volatile NextAddr      :5;
	unsigned volatile ListItemValid :1;
	unsigned volatile Prio0Status   :11;
	unsigned volatile Prio1Status   :11;
	unsigned               :4;
    }Bits;
    uint32 volatile dwValue;
}DmaWTRequestQueuePointerReg; /* 4.4.8 */

typedef union
{
    struct
    {
	unsigned volatile NextAddr      :5;
	unsigned volatile ListItemValid :1;
	unsigned volatile Prio0Status   :11;
	unsigned volatile Prio1Status   :11;
	unsigned               :4;
    }Bits;
    uint32 volatile dwValue;
}DmaVDBRequestQueuePointerReg; /* 4.4.8 */

typedef struct
{
    DmaWTChannelAddressReg              WTAddr[ VORTEX_WT_CHANNEL_COUNT ];
    DmaVDBChannelAddressReg             VDBAddr[ VORTEX_VDB_CHANNEL_COUNT ];
    DmaWTChannelSizeGoto                WTSizeGoto[ VORTEX_WT_CHANNEL_COUNT ];
    DmaVDBChannelSizeGoto               VDBSizeGoto[ VORTEX_VDB_CHANNEL_COUNT ];
    char reserved0[ 128 ];
    DmaWTChannelModeReg                 WTMode[ VORTEX_WT_CHANNEL_COUNT ];
    DmaVDBChannelModeReg                VDBMode[ VORTEX_VDB_CHANNEL_COUNT ];
    DmaVDBChannelGetAbsolutePositionReg VDBGAP[ VORTEX_VDB_CHANNEL_COUNT ];
    DmaStatusReg                        WTStat[ VORTEX_WT_CHANNEL_COUNT/16 ];
    DmaStatusReg                        VDBStat[ VORTEX_VDB_CHANNEL_COUNT/16 ];
    DmaEngineControlReg                 EngCtl;
    DmaWTQueueLinkedListPointersReg     WTReqQue [ VORTEX_WT_CHANNEL_COUNT/16 ];
    DmaVDBQueueLinkedListPointersReg    VDBReqQue;
    DmaStatusReg                        WriteWTStat0_15;
    DmaStatusReg                        WriteWTStat16_31;
    DmaStatusReg                        WriteVDBStat;
    char reserved1[ 24 ];
    uint32                              WTSetStat[ VORTEX_WT_CHANNEL_COUNT ];
    uint32                              VDBSetStat[ VORTEX_VDB_CHANNEL_COUNT ];
    DmaWTRequestQueuePointerReg         WTReqQuePtr[ VORTEX_WT_CHANNEL_COUNT ];
    DmaVDBRequestQueuePointerReg        VDBReqQuePtr[ VORTEX_VDB_CHANNEL_COUNT ];
    char reserved2[ 64 ];
}DmaRegs; /* 4.4 */



/*******************************************************************************
 * VDB Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile DestAddr :7;
	unsigned volatile SrcAddr  :7;
	unsigned          :18;
    }Bits;
    uint32 volatile dwValue;
}VDBRamLink; /* Table 6-11 */

/* Possible values for SrcAddr */
typedef enum
{
    VDB_SRC_FIFO   = 0,
    VDB_SRC_SRC    = 0x10,
    VDB_SRC_MIXER  = 0x20,
    VDB_SRC_FX     = 0x40,
    VDB_SRC_CODEC  = 0x48,
    VDB_SRC_SERIAL = 0x50,
    VDB_SRC_WT     = 0x58,
    VDB_SRC_NULL   = 0x7f,
}VDB_SRC_ADDR;

/* Possible values for DestAddr */
typedef enum
{
    VDB_DEST_FIFO     = 0,
    VDB_DEST_SRC      = 0x10,
    VDB_DEST_FIFOA    = 0x20,
    VDB_DEST_MIXER    = 0x30,
    VDB_DEST_FX       = 0x50,
    VDB_DEST_CODEC    = 0x58,
    VDB_DEST_SERIAL   = 0x60,
    VDB_DEST_PARALLEL = 0x66,
    VDB_DEST_NULL     = 0x7f,
}VDB_DEST_ADDR;

/* Indexes into the header tag array */
typedef enum
{
    VDB_SR_SRC0 = 0,
    VDB_SR_SRC1,
    VDB_SR_SRC2,
    VDB_SR_SRC3,
    VDB_SR_SRC4,
    VDB_SR_SRC5,
    VDB_SR_SRC6,
    VDB_SR_SRC7,
    VDB_SR_SRC8,
    VDB_SR_SRC9,
    VDB_SR_SRC10,
    VDB_SR_SRC11,
    VDB_SR_SRC12,
    VDB_SR_SRC13,
    VDB_SR_SRC14,
    VDB_SR_SRC15,
    VDB_SR_FX,
    VDB_SR_CODEC,
    VDB_SR_AC97,
    VDB_SR_SERIAL,
    VDB_SR_SSTEP,
    VDB_SR_DSPDMA,
}VDB_SAMPLE_RATES;

typedef union
{
    struct
    {
	unsigned volatile SRC0   : 1;
	unsigned volatile SRC1   : 1;
	unsigned volatile SRC2   : 1;
	unsigned volatile SRC3   : 1;
	unsigned volatile SRC4   : 1;
	unsigned volatile SRC5   : 1;
	unsigned volatile SRC6   : 1;
	unsigned volatile SRC7   : 1;
	unsigned volatile SRC8   : 1;
	unsigned volatile SRC9   : 1;
	unsigned volatile SRC10  : 1;
	unsigned volatile SRC11  : 1;
	unsigned volatile SRC12  : 1;
	unsigned volatile SRC13  : 1;
	unsigned volatile SRC14  : 1;
	unsigned volatile SRC15  : 1;
	unsigned volatile FX     : 1;
	unsigned volatile CODEC  : 1;
	unsigned volatile AC97   : 1;
	unsigned volatile SERIAL : 1;
	unsigned volatile SSTEP  : 1;
	unsigned volatile DSPDMA : 1;
	unsigned            :10;
    }Bits;
    uint32 volatile dwValue;
}VDBSampleRateActiveReg; /* Table 6-12 */


typedef struct
{
    VDBRamLink DestLink[ VORTEX_VDB_DESTINATION_COUNT ];	// 103*4	= 412 (0-411)
    VDBRamLink HeadLink[ VORTEX_VDB_HEADER_COUNT ];			// 22*4		= 88  (412-499)
    uint32 reserved[3];										// 3*4		= 12	(500-511)
    VDBSampleRateActiveReg SRActive;						// 4		= 4		(512-515)
    uint32 SingleStep;										// 4		= 4		(516-519)
    char filler[1528];
}VDBRegs; /* Table 6-10 */


/*******************************************************************************
 * Game Registers 
 *******************************************************************************/

typedef union
{
    struct
    {
	unsigned volatile MIDIData : 8;
	unsigned          : 24;
    }Bits;
    uint32 volatile dwValue;
}GAMEMidiDataReg;

typedef union
{
    struct
    {
	unsigned volatile MPU401Mode    : 1;
	unsigned               : 5;
	unsigned volatile WriteOK       : 1;
	unsigned volatile MIDIDataValid : 1;
	unsigned               : 24;
    }Bits;
    uint32 volatile dwValue;
}GAMEMidiCmdStatReg;

typedef union
{
    struct
    {
	unsigned volatile JoystickAxis    : 4;
	unsigned volatile JoystickButtons : 4;
	unsigned                 : 24;
    }Bits;
    uint32 volatile dwValue;
}GAMEJoystickPortReg;

typedef union
{
    struct
    {
	unsigned volatile MIDIDataValid   : 1;
	unsigned volatile MIDIRcvOverflow : 1;
	unsigned volatile MIDIRcvReady    : 1;
	unsigned volatile MIDIWriteOK     : 1;
	unsigned volatile MIDIMode        : 2;
	unsigned volatile JoystickMode    : 1;
	unsigned                 : 1;
	unsigned volatile ClockDivider    : 8;
	unsigned                 : 16;
    }Bits;
    uint32 volatile dwValue;
}GAMEControlReg;

typedef struct
{
    GAMEMidiDataReg     MidiData;
    GAMEMidiCmdStatReg  CmdStat;
    GAMEJoystickPortReg Joystick;
    GAMEControlReg      Control;
    uint32               volatile Position0;
    uint32               volatile Position1;
    uint32               volatile Position2;
    uint32               volatile Position3;
    char filler[0x7e0];
}GameRegs;


/*******************************************************************************
 * Serial Registers 
 *******************************************************************************/

typedef union
{
    struct
    {
	unsigned volatile FrameMode        : 1;
	unsigned volatile SampleRateSelect : 2;
	unsigned volatile SCLKDirection    : 1;
	unsigned volatile SampleEdge       : 1;
	unsigned                  : 2;
	unsigned volatile ResetPort        : 1;
	unsigned                  : 22;
	unsigned volatile VDB_SRB16event   : 1;
	unsigned volatile VDB_SRB16        : 1;
    }Bits;
    uint32 volatile dwValue;
}SERFXCtlReg; /* 8.3.4 */

typedef union
{
    struct
    {
	unsigned volatile FrameMode            : 2;
	unsigned volatile SYNCinvert           : 1;
	unsigned volatile SampleEdge           : 1;
	unsigned volatile SCLKdirection        : 1;
	unsigned volatile CCLKselect           : 1;
	unsigned volatile CCLKenable           : 1;
	unsigned volatile ResetPort            : 1;
	unsigned volatile CmdWrtOK             : 1;
	unsigned volatile CmdType              : 1;
	unsigned                      : 2;
	unsigned volatile AC97CodecRdyOverride : 1;
	unsigned volatile AC97ModemEnaOverride : 1;
	unsigned volatile AC97WarmReset        : 1;
	unsigned volatile AC97ColdReset        : 1;
	unsigned volatile AC97CodecTag         : 11;
	unsigned                      : 1;
	unsigned volatile VDB_SRB18event       : 1;
	unsigned volatile VDB_SRB18            : 1;
	unsigned volatile VDB_SRB17event       : 1;
	unsigned volatile VDB_SRB17            : 1;
    }Bits;
    uint32 volatile dwValue;
}SERCodecIntCtlReg;/* 8.4.6 */

/* Possible values for SCLKdirection */
#define SER_CODEC_INPUT  0
#define SER_CODEC_OUTPUT 1

typedef union
{
    struct
    {
	unsigned volatile RightCmdStat : 16;
	unsigned volatile LeftCmdStat  : 16;
    }Bits;
    uint32 volatile dwValue;
}SERCodecCmdStatReg; /* 8.4.7 */

typedef union
{
    struct
    {
	unsigned volatile AC97CmdStatData   : 16;
	unsigned volatile AC97CmdStatAddr   : 7;
	unsigned volatile AC97CmdStatRWStat : 1;
	unsigned                   : 8;
    }Bits;
    uint32 volatile dwValue;
}SERAC97CmdStatReg; /* 8.4.7 */

typedef union
{
	union
	{
	    SERCodecCmdStatReg Codec;
	    SERAC97CmdStatReg AC97;
	}CtlStat;
    uint32 volatile dwValue;
}SERCmdStatReg;

typedef union
{
    struct
    {
	unsigned volatile FrameMode         : 2;
	unsigned volatile DataCCLKdirection : 1;
	unsigned volatile SYNCinvert        : 1;
	unsigned volatile SYNCdirection     : 1;
	unsigned volatile SampleEdge        : 1;
	unsigned volatile SCLKdirection     : 1;
	unsigned volatile ResetPort         : 1;
	unsigned volatile SampleRateSelect  : 2;
	unsigned volatile CCLKselect        : 1;
	unsigned                   : 19;
	unsigned volatile VDB_SRB19event    : 1;
	unsigned volatile VDB_SRB19         : 1;
    }Bits;
    uint32 volatile dwValue;
}SERSPORTIntCtlReg; /* 8.5.5 */

typedef union
{
    struct
    {
	unsigned volatile FXCh0Ena    : 1;
	unsigned volatile FXCh1Ena    : 1;
	unsigned volatile FXCh2Ena    : 1;
	unsigned volatile FXCh3Ena    : 1;
	unsigned volatile FXCh4Ena    : 1;
	unsigned volatile FXCh5Ena    : 1;
	unsigned volatile FXCh6Ena    : 1;
	unsigned volatile FXCh7Ena    : 1;
	unsigned volatile CodecCh0Ena : 1;
	unsigned volatile CodecCh1Ena : 1;
	unsigned volatile CodecCh2Ena : 1;
	unsigned volatile CodecCh3Ena : 1;
	unsigned volatile CodecCh4Ena : 1;
	unsigned volatile CodecCh5Ena : 1;
	unsigned volatile CodecCh6Ena : 1;
	unsigned volatile CodecCh7Ena : 1;
	unsigned volatile SportCh0Ena : 1;
	unsigned volatile SportCh1Ena : 1;
	unsigned             : 14;
    }Bits;
    uint32 volatile dwValue;
}SERChannelEnableReg; /* 8.6.1 */

typedef union
{
    struct
    {
	unsigned volatile SubsampleCount : 8;
	unsigned volatile SampleCount    : 24;
    }Bits;
    uint32 volatile dwValue;
}SERSampleCounterReg; /* 8.7.1 */

typedef union
{
    struct
    {
	unsigned volatile TimerCounterValue : 18;
	unsigned                   : 13;
	unsigned volatile TimerIntStat      : 1;
    }Bits;
    uint32 volatile dwValue;
}SERSampleTimerReg; /* 8.8.1 */

/* Offsets within CodecChannelData[] below */
typedef enum
{
    SER_FX_SRC = 0,
    SER_FX_DST = 2,
    SER_CODEC_SRC = 32,
    SER_CODEC_DST = 34,
    SER_SPORT_SRC = 64,
    SER_SPORT_DST = 66,
}SERDataOffset;

typedef struct
{
    uint32 EffectsChannelData[32];
    uint32 CodecChannelData[32];
    uint32 Sport0ChannelData[8];
    uint32 reserved1[24];
    SERFXCtlReg FXCtl;
    SERCodecIntCtlReg CodecCtl;
    SERCmdStatReg CodecCmdStat;
    SERSPORTIntCtlReg SportCtl;
    SERChannelEnableReg ChannelEnable;
    uint32 reserved2;
    SERSampleCounterReg SampleCounter;
    SERSampleTimerReg SampleTimer;
    char filler[0x660];
}SerRegs; /* 8.9 */


/*******************************************************************************
 * Sound Blaster Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile FMAddress         : 8;
	unsigned volatile FMNew             : 1;
	unsigned volatile FMVersionToReport : 1;
	unsigned volatile OPL3Enable        : 1;
	unsigned volatile Clock50mHz        : 1;
	unsigned                   : 20;       
    }Bits;
    uint32 volatile dwValue;
}FMCtrlStatReg;

typedef union
{
    struct
    {
	unsigned volatile Timer1Preset : 8;
	unsigned volatile Timer2Preset : 8;
	unsigned volatile Timer1Start  : 1;
	unsigned volatile Timer2Start  : 1;
	unsigned              : 3;
	unsigned volatile Timer2Mask   : 1;
	unsigned volatile Timer1Mask   : 1;
	unsigned              : 9;
    }Bits;
    uint32 volatile dwValue;
}FMTimerStatReg;

typedef struct
{
	uint32          volatile AddrStat;
	uint32          volatile DataPort;
	uint32          volatile AdvAddrPort;
	uint32          volatile AdvDataPort;
	uint32          volatile MixerAddrPort;
	uint32          volatile MixerDataPort;
	uint32          volatile Reset;
	uint32          volatile ResetAlias;
	uint32          volatile AddrStat2;
	uint32          volatile DataPort2;
	uint32          volatile ReadData;
	uint32          volatile ReadDataAlias;
	uint32          volatile CmdStat;
	uint32          volatile CmdStatAlias;
	uint32          volatile Status;
	uint32          volatile StatusAlias;
	uint32          volatile LACR;
	uint32          volatile LADCR;
	uint32          volatile LAMR;
	uint32          volatile LARR;
	uint32          volatile SBVer;
	FMCtrlStatReg  volatile CtrlStat;
	FMTimerStatReg volatile TimerStat;
	char           reserved0[0xA4];
    uint32          volatile FMRAM[0x40];
    char           pad[0x600];
}SBRegs;


/*******************************************************************************
 * PCI Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile MasterFatalError       :1;
	unsigned volatile MasterParityError      :1;
	unsigned volatile RegbusTimeoutError     :1;
	unsigned volatile FIFOUnderrunError      :1;
	unsigned volatile DMANoBufferError       :1;
	unsigned volatile DMAEndOfBuffer         :1;
	unsigned volatile FMTimerOverflow        :1;
	unsigned volatile SBMixerRequest         :1;
	unsigned volatile SBProRequest           :1;
	unsigned volatile ForcedInterrupt        :1;
	unsigned volatile DSPRequestPULSE        :1;
	unsigned volatile DSPRequestLEVEL        :1;
	unsigned volatile VortexMainTimerOverflow :1;
	unsigned volatile MIDIRequest            :1;
	unsigned volatile ModemRequest           :1;
	unsigned                        :1;
	unsigned volatile MasterFatalToSERR      :1;
	unsigned volatile MasterParityToSERR     :1;
	unsigned volatile RegbusTimeoutToSERR    :1;
	unsigned volatile FifoUnderrunToSERR     :1;
	unsigned volatile DMANoBufferToSERR      :1;
	unsigned                        :10;
    }Bits;
    uint32 volatile dwValue;
}PciInterruptSCReg; /* Table 3-12 */

typedef union
{
    struct
    {
	unsigned volatile InterruptAsserted       :1;
	unsigned volatile SystemErrorAsserted     :1;
	unsigned volatile TargetTransmitFIFOFull  :1;
	unsigned volatile TargetTransmitFIFOEmpty :1;
	unsigned volatile TargetReceiveFIFOEmpty  :1;
	unsigned volatile MasterTransmitFIFOFull  :1;
	unsigned volatile MasterTransmitFIFOEmpty :1;
	unsigned volatile MasterReceiveFIFOEmpty  :1;
	unsigned volatile DMAInProgress           :1;
	unsigned volatile DMARequestIsWrite       :1;
	unsigned volatile DMARequestPending       :1;
	unsigned volatile PMECapable              :1;
	unsigned volatile Disable_pci_done        :1;
	unsigned volatile DisableIOBar2           :1;
	unsigned volatile DisableIOBar3           :1;
	unsigned                         :17;
    }Bits;
    uint32 volatile dwValue;
}PciGlobalStatusReg; /* Table 3-13 */

typedef union
{
    struct
    {
	unsigned volatile MPU401Enable            :1;
	unsigned volatile SBEnable                :1;
	unsigned volatile AdlibEnable             :1;
	unsigned volatile JoystickEnable          :1;
	unsigned volatile COMEnable               :1;
	unsigned volatile MPU401Select            :2;
	unsigned volatile SBSelect                :2;
	unsigned volatile JoystickSelect          :3;
	unsigned volatile COMSelect               :2;
	unsigned volatile GlobalInterruptEnable   :1;
	unsigned volatile ForceInterrupt          :1;
	unsigned volatile FlushMasterFIFO         :1;
	unsigned volatile FlushTargetReceiveFIFO  :1;
	unsigned volatile FlushTargetTransmitFIFO :1;
	unsigned volatile AutoIncrementEnable     :1;
	unsigned volatile RegbusTimeoutEnable     :1;
	unsigned volatile ExternalReset           :1;
	unsigned volatile VDBArbiterReset         :1;
	unsigned volatile VortexSoftReset          :1;
	unsigned volatile MailboxSlot             :1;
	unsigned                         :7;
    }Bits;
    uint32 volatile dwValue;
}PciGlobalControlReg; /* Table 3-14 */

// Possible values for COMSelect
#define PCI_COM_SELECT_2e8 (0)
#define PCI_COM_SELECT_2f8 (1)
#define PCI_COM_SELECT_3e8 (2)
#define PCI_COM_SELECT_3f8 (3)

// Possible values for JoystickSelect
#define PCI_JOYSTICK_SELECT_200 (0)
#define PCI_JOYSTICK_SELECT_201 (1)
#define PCI_JOYSTICK_SELECT_202 (2)
#define PCI_JOYSTICK_SELECT_203 (3)
#define PCI_JOYSTICK_SELECT_204 (4)
#define PCI_JOYSTICK_SELECT_205 (5)
#define PCI_JOYSTICK_SELECT_206 (6)
#define PCI_JOYSTICK_SELECT_207 (7)

// Possible values for SBSelect
#define PCI_SB_SELECT_220 (0)
#define PCI_SB_SELECT_240 (1)
#define PCI_SB_SELECT_260 (2)
#define PCI_SB_SELECT_280 (3)

// Possible values for MPU401Select
#define PCI_MPU401_SELECT_300 (0)
#define PCI_MPU401_SELECT_310 (1)
#define PCI_MPU401_SELECT_320 (2)
#define PCI_MPU401_SELECT_330 (3)

typedef struct
{
    PciInterruptSCReg   isr;
    PciInterruptSCReg   icr;
    PciGlobalStatusReg  gsr;
    PciGlobalControlReg gcr;
    char filler[0x7f0];
}PciRegs; /* Table 3-11 */


/*******************************************************************************
 * I2C Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile PCIDone      : 1;
	unsigned volatile EEPROMexists : 1;
	unsigned volatile SlowFlag     : 1;
	unsigned volatile FastFlag     : 1;
	unsigned volatile SCLin        : 1;
	unsigned volatile SDAin        : 1;
	unsigned volatile SCLout       : 1;
	unsigned volatile SDAout       : 1;
	unsigned              : 24;
    }Bits;
    uint32 volatile dwValue;
}I2CCommandPortReg;

typedef union
{
    struct
    {
	unsigned volatile VendorId    : 16;
	unsigned volatile SubSystemId : 16;
    }Bits;
    uint32 volatile dwValue;
}I2CPCISubSysReg;

typedef struct
{
    I2CCommandPortReg Command;
    I2CPCISubSysReg   SubSys;
    uint32             PageReadMode;
    char filler[0x7f4];
}I2CRegs;


/*******************************************************************************
 * Parallel Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile ModemPulse    : 5;
	unsigned volatile HENwidth      : 5;
	unsigned volatile DSPWritePulse : 5;
	unsigned               : 17;
    }Bits;
    uint32 volatile dwValue;
}PIFPortConfigReg;

typedef union
{
    struct
    {
	unsigned volatile DMAwidth     : 1;
	unsigned volatile ThrottledDMA : 1;
	unsigned volatile DSPPolling   : 1;
	unsigned              : 29;
    }Bits;
    uint32 volatile dwValue;
}PIFDMAConfigReg;

typedef union
{
    struct
    {
	unsigned volatile DMAFSMState      : 4;
	unsigned volatile ExternalFSMState : 5;
	unsigned                  : 23;
    }Bits;
    uint32 volatile dwValue;
}PIFFSMstatus;

typedef struct
{
    uint32            	volatile p16550regs[8];
    uint32            	volatile DSPRegs[8];
    PIFPortConfigReg 	volatile PortConfig;
    PIFDMAConfigReg  	volatile DMAConfig;
    uint32            	volatile DMAactive;
    uint32            	volatile Reset;
    uint32            	volatile Pins;
    uint32            	volatile SRout;
    PIFFSMstatus     	volatile FSMstatus;
    char             	volatile reserved0[0xfa4];
    uint32            	volatile RegBusDMA;
    char filler[0xffc];
}PifRegs;

typedef struct
{
    WTRegs    wt[VORTEX_TOTAL_WT_BLOCKS];
    MixerRegs mixer;
    SRCRegs   src;
    FIFORegs  fifo;
    DmaRegs   dma;
    VDBRegs   vdb;
    GameRegs  game;
    SerRegs   ser;
    SBRegs    sb;
    PciRegs   pci;
    I2CRegs   i2c;
    char filler[0x0800];
    PifRegs   pif;
}au8820regs;

#define OFFSET(m)	((uint32)&(((au8820regs*)0)->m)) // returns offset of the specified register in au8820regs structure 

#endif
