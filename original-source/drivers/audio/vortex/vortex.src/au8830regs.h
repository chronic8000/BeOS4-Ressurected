#ifndef _AU8830REGS_H
#define _AU8830REGS_H

#define VORTEX_VDB_CHANNEL_COUNT					(32)
#define VORTEX_WT_CHANNEL_COUNT						(64)
#define VORTEX_A3D_CHANNEL_COUNT					(76)
#define VORTEX_A3D_HW_CHANNEL_COUNT					(16)

#define VORTEX_FIFO_SIZE							(32)
#define VORTEX_VDB_FIFO_SIZE						(64)
#define VORTEX_WT_FIFO_SIZE							(64)

#define VORTEX_VDB_DESTINATION_COUNT				(173)
#define VORTEX_VDB_HEADER_COUNT						(24)

#define VORTEX_SRC_HEADER_COUNT						(22)
#define VORTEX_SRC_CHANNEL_COUNT					(16)

#define VORTEX_CHANNEL_COUNT						(96)

// wavetable
#define VORTEX_TOTAL_WT_BLOCKS						(2)
#define VORTEX_HW_VOICES_PER_BLOCK					(32)

// to be modified
#define VORTEX_HOST_3D_WAVE_MAXIMUM_CHANNEL_COUNT	(8)
#define VORTEX_HOST_3D_WAVE_NON_PII_CHANNEL_COUNT	(6)
#define VORTEX_HOST_3D_WAVE_NON_MMX_CHANNEL_COUNT	(0)

#define VORTEX_CONFIG_SPACE_SIZE	(0x40000)		/* sizeof regs in bytes */


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
    VORTEX_WAVETABLE0_OFFSET	= 0x00000000,
    VORTEX_WAVETABLE1_OFFSET	= 0x00008000,
    VORTEX_FIFO_OFFSET			= 0x00010000,
    VORTEX_A3D_OFFSET			= 0x00018000,
    VORTEX_MIXER_OFFSET			= 0x00020000,
    VORTEX_PARALLEL_OFFSET		= 0x00022000,
	VORTEX_XTALK_OFFSET			= 0x00024000,
    VORTEX_SRC_OFFSET			= 0x00026000,
    VORTEX_DMA_OFFSET			= 0x00027000,
    VORTEX_VDB_OFFSET			= 0x00028000,
    VORTEX_GAME_OFFSET			= 0x00028800,
    VORTEX_SERIAL_OFFSET		= 0x00029000,
    VORTEX_SB_OFFSET			= 0x00029800,
    VORTEX_PCI_OFFSET			= 0x0002A000,
    VORTEX_I2C_OFFSET			= 0x0002A800,
    VORTEX_EQ_OFFSET			= 0x0002B000,
}VORTEX_BLOCK_ADDRESS;

// to be modified (not used)
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
	unsigned volatile Enable		: 1;
	unsigned volatile SrateSelect	: 5;
	unsigned volatile Headroom		: 3;
	unsigned volatile Mode49152		: 1;
	unsigned volatile DsHead		: 3;		
	unsigned volatile Reserved		: 19;
    } Bits;
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
    } Bits;
    uint32 volatile dwValue;
}WTramp;

typedef union
{
    struct
    {
		uint32 volatile OpSelect;
    } Bits;
    uint32 volatile dwValue;
}WTdsreg;

// values for OP select
#define WT_DSSTREAM_ON				0x1L
#define WT_DSSTREAM_OFF				0x0L


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
    } Bits;
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
        } Bits;
        uint32 volatile dwValue;
    }Vs0;

    union
    {
        struct
        {
	    unsigned volatile State : 1;
	    unsigned volatile Pitch : 15;
	    unsigned volatile Gain  : 8;
	    unsigned volatile FC    : 8;
        } Bits;
        uint32 volatile dwValue;
    }Vs1;

    union
    {
        struct
        {
	    unsigned short volatile PitchPhase;
	    unsigned short volatile FilterFeedbackGain;
        } Bits;
        uint32 volatile dwValue;
    }Vs2;

    union
    {
        struct
        {
	    unsigned volatile Reverb          : 7;
	    unsigned volatile Chorus          : 7;
	    unsigned volatile Pan             : 7;
	    unsigned volatile GainState       : 3;
	    unsigned volatile FilterInputGain : 8;
        } Bits;
        uint32 volatile dwValue;
    }Vs3;
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
    WTdsreg Dsreg;
    WTramp Mramp;
    uint32 GainMode;
    WTramp Aramp;
    uint32 reserved1[0x1a];
    WTstereo Stereo[(VORTEX_HW_VOICES_PER_BLOCK/2)];
    uint32 reserved2[16];
    WTmute Mute[VORTEX_HW_VOICES_PER_BLOCK];
    uint32 Running[VORTEX_HW_VOICES_PER_BLOCK];
    WTvoicestate VoiceState[VORTEX_HW_VOICES_PER_BLOCK];
    WTUnitDelay UnitDelay[VORTEX_HW_VOICES_PER_BLOCK];
    char filler[0x7a00];
}WTRegs;	// 10.2.1

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
    } Bits;
    uint32 volatile dwValue;
}MixerGainSignReg;

typedef union
{
    struct
    {
	unsigned volatile Mantissa     : 3;
	unsigned volatile Exponent     : 5;
	unsigned              : 24;
    } Bits;
    uint32 volatile dwValue;
}MixerGainUnsignReg;

typedef union
{
    struct
    {
	unsigned volatile EnableBit0 : 1;
	unsigned volatile EnableBit1 : 1;
	unsigned volatile EnableBit2 : 1;
	unsigned volatile EnableBit3 : 1;
	unsigned				: 28;
    } Bits;
    uint32 volatile dwValue;
}MixerEnableReg;

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
    } Bits;
    uint32 volatile dwValue;
}MixerSaturationReg;

typedef union
{
    struct
    {
	unsigned volatile BlockChannel : 4;
	unsigned volatile NextValid    : 1;
    } Bits;
    uint32 volatile dwValue;
}MixerRAMLinkReg;

typedef union
{
    struct
    {
	unsigned volatile Data : 16;
	unsigned      : 16;
    } Bits;
    uint32 volatile dwValue;
}MixerDataReg; /* Table 11-6 */

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
    } Bits;
    uint32 volatile dwValue;
}MixerSampleRateActiveReg;

typedef struct
{
    MixerGainSignReg CurrentInputGain[VDB_MIXERSRC_CHANNEL_COUNT * VDB_MIXERDEST_CHANNEL_COUNT];
    MixerGainSignReg CurrentOutputGain[VDB_MIXERSRC_CHANNEL_COUNT];
    char reserved0[0x07c0];
    MixerGainUnsignReg TargetInputGain[VDB_MIXERSRC_CHANNEL_COUNT * VDB_MIXERDEST_CHANNEL_COUNT];
    MixerGainUnsignReg TargetOutputGain[VDB_MIXERSRC_CHANNEL_COUNT];
    char reserved1[0x01c0];
    MixerEnableReg Enable[128];
    MixerDataReg Sample[96];
    char reserved2[0x0080];
    MixerRAMLinkReg ChannelLink[38];
    char reserved3[0x0068];
    MixerSampleRateActiveReg SRActive;
    char reserved4[0x7c];
    MixerSaturationReg Saturation;
    char filler[0x007c];
}MixerRegs;


/*******************************************************************************
 * Sample Rate Converter Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile data : 16;
	unsigned      : 16;
    } Bits;
    uint32 volatile dwValue;
}SRCData;

typedef union
{
    struct
    {
	unsigned volatile NextChannel  : 4;
	unsigned volatile ChannelValid : 1;
	unsigned              : 27;
    } Bits;
    uint32 volatile dwValue;
}SRCNextChannel;

typedef union
{
    struct
    {
	unsigned volatile Header       : 4;
	unsigned volatile ChannelValid : 1;
	unsigned              : 27;
    } Bits;
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
    SRCData InputFifo[VORTEX_SRC_CHANNEL_COUNT][32];
    SRCData OutputBuffer[VORTEX_SRC_CHANNEL_COUNT][2];
    char reserved0[0x380];
    SRCNextChannel NextChannel[VORTEX_SRC_CHANNEL_COUNT];
    SRCSrHeader SrHeader[VORTEX_SRC_HEADER_COUNT];
    char reserved1[0x28];
    uint32 SRActive;
    uint32 ThrottleSource;
    uint32 ThrottleCountSize;
    char reserved2[0x134];
    SRCChannelParams ChannelParams[7][VORTEX_SRC_CHANNEL_COUNT];
    char reserved3[0x40];
}SRCRegs;
// unchanged

/*******************************************************************************
 * FIFO Registers 
 *******************************************************************************/

typedef union
{
	struct
	{
	unsigned volatile data   :16;
	unsigned        :16;
	} Bits;
	uint32 volatile dwValue;
}FIFOChannelDataReg; /* 4.6.3 */

// changed
typedef union
{
    struct
    {
	unsigned volatile SentDMAReq	:1;
	unsigned volatile Stereo		:1;
	unsigned volatile Priority		:2;
	unsigned volatile ChannelValid	:1;
	unsigned volatile Empty			:1;
	unsigned volatile WTVDBPointer	:6;
	unsigned volatile DMAPointer	:6;
	unsigned volatile Wait4DMA		:1;
	unsigned volatile HadUnderrun	:1;
	unsigned volatile FreezePtrs	:1;
	unsigned volatile FlushFIFO		:1;
	unsigned volatile MaskSentDMAReq:1;
	unsigned volatile MaskStereo	:1;
	unsigned volatile MaskPriority	:1;
	unsigned volatile MaskValid		:1;
	unsigned volatile MaskEmpty		:1;
	unsigned volatile MaskWTVDBPtr	:1;
	unsigned volatile MaskDMAPtr	:1;
	unsigned volatile MaskWait		:1;
	unsigned volatile MaskUnderrun	:1;
	unsigned volatile MaskFlushFreeze	:1;
    } Bits;
    uint32 volatile dwValue;
}FIFOPointersAndControlReg; /* 4.6.4 */

/* Possible values for Stereo */
#define FIFO_MONO_DATA (0)
#define FIFO_STEREO_DATA (1)

/* Possible values for Priority */
#define FIFO_LOW_PRIORITY (0)
#define FIFO_HIGH_PRIORITY (1)

// changed
typedef union
{
    struct
    {
	unsigned volatile EnableTriggerOpt	:1;
	unsigned volatile TriggerLevel		:6;
	unsigned							:25;
    } Bits;
    uint32 volatile dwValue;
}FIFOTriggerControlReg; /* 4.6.2 */


// changed
typedef struct
{
    FIFOChannelDataReg			WTData[VORTEX_WT_CHANNEL_COUNT][VORTEX_WT_FIFO_SIZE];
    FIFOChannelDataReg			VDBData[VORTEX_VDB_CHANNEL_COUNT][VORTEX_VDB_FIFO_SIZE];
    FIFOPointersAndControlReg	WTPtrCtl[VORTEX_WT_CHANNEL_COUNT];
    FIFOPointersAndControlReg	VDBPtrCtl[VORTEX_VDB_CHANNEL_COUNT];
	char						reserved1[0x0e80];
    FIFOTriggerControlReg		WT0trigger;
    FIFOTriggerControlReg		WT1trigger;
    FIFOTriggerControlReg		VDBtrigger;
    char						reserved2[0x0ff4];
}FIFORegs;

/*******************************************************************************
 * DMA Registers 
 *******************************************************************************/
// changed
typedef union
{
    struct
    {
	unsigned volatile CurrentPosition	:12;
	unsigned volatile InterruptEnable	:1;
	unsigned volatile ReadWrite			:1;
	unsigned volatile DataFormat		:4;
	unsigned volatile SubbufError		:1;
	unsigned volatile SubbufErrorChk	:1;
	unsigned volatile CurrentSubbuf		:2;
	unsigned							:10;
    } Bits;
    uint32 volatile dwValue;
}DmaChannelModeReg; /* 3.4.2 */

/* Possible values for InterruptEnable */
#define DMA_INTERRUPT_OFF	(0)
#define DMA_INTERRUPT_ON	(1)

/* Possible values for ReadWrite */
#define DMA_VORTEX_TO_HOST	(0)
#define DMA_HOST_TO_VORTEX	(1)

/* Possible values for DataFormat */
#define DMA_16_BIT_LINEAR	(8)
#define DMA_8_BIT_LINEAR	(1)
#define DMA_8_BIT_ULAW		(2)
#define DMA_8_BIT_ALAW		(3)
#define DMA_16_BIT_AIFF		(9)

// changed
typedef union
{
    struct
    {
	unsigned volatile SubbufPosition	:12;
	unsigned volatile CurrentSubbuf     :2;
	unsigned volatile NumMTFIFOS		:7;
	unsigned volatile FIFOValid			:1;
	unsigned volatile Empty				:1;
	unsigned volatile DMAInProgress		:1;
	unsigned volatile XferBytes			:8;
    } Bits;
    uint32 volatile dwValue;
}DmaChannelGetAbsolutePositionReg; /* 3.4.3 */

// changed
typedef union
{
    struct
    {
	unsigned long volatile address;
    } Bits;
    uint32 volatile dwValue;
}DmaSubbufAddressReg;

typedef struct
{
    DmaSubbufAddressReg subbuf[4];
}DmaChannelAddressReg;		/* 3.4.4 */


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
    } Bits;
    uint32 volatile dwValue;
}DmaChannelSizeAndGotoReg; /* 3.4.5 */

typedef struct
{
	DmaChannelSizeAndGotoReg sb0_1;
	DmaChannelSizeAndGotoReg sb2_3;
}DmaChannelSizeGotoReg;


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
    } Bits;
    uint32 volatile dwValue;
}DmaStatusReg; /* 3.4.6 */


typedef union
{
    struct
    {
	unsigned volatile AllBurstSize16		:1;
	unsigned volatile NewDmaReqID			:7;
	unsigned volatile NewDmaReqIDValid		:1;
	unsigned volatile CurrentDMAReqID		:7;
	unsigned volatile CurrentDMAReqIDValid	:1;
	unsigned volatile MaxWords				:6;
	unsigned volatile CacheBurst			:1;
	unsigned volatile MaxWordsBurst			:1;
	unsigned volatile AlwaysReadMultiple	:1;
	unsigned volatile MaybeReadMultiple		:1;
	unsigned volatile CachelineSize			:4;
	unsigned volatile WriteAllControl		:1;
    } Bits;
    uint32 volatile dwValue;
}DmaEngineControlReg; /* 3.4.7 */

typedef union
{
    struct
    {
	unsigned volatile Prio1Status :11;
	unsigned volatile Prio0Status :11;
	unsigned             :10;
    } Bits;
    uint32 volatile dwValue;
}DmaQueueLinkedListPointersReg; /* 3.4.8 */


typedef union
{
    struct
    {
	unsigned volatile NextAddr      :5;
	unsigned volatile ListItemValid :1;
	unsigned volatile Prio0Status   :11;
	unsigned volatile Prio1Status   :11;
	unsigned               :4;
    } Bits;
    uint32 volatile dwValue;
}DmaRequestQueuePointerReg; /* 3.4.9 */
 
// changed
typedef struct
{
    DmaChannelAddressReg				WTAddr[VORTEX_WT_CHANNEL_COUNT];
    DmaChannelAddressReg                VDBAddr[VORTEX_VDB_CHANNEL_COUNT];
    DmaChannelSizeGotoReg				WTSizeGoto[VORTEX_WT_CHANNEL_COUNT];
    DmaChannelSizeGotoReg				VDBSizeGoto[VORTEX_VDB_CHANNEL_COUNT];
    DmaChannelModeReg					WTMode[VORTEX_WT_CHANNEL_COUNT];
    DmaChannelModeReg					VDBMode[VORTEX_VDB_CHANNEL_COUNT];
    DmaStatusReg                        WTStat[ VORTEX_WT_CHANNEL_COUNT/16 ];
    DmaStatusReg                        VDBStat[ VORTEX_VDB_CHANNEL_COUNT/16 ];
	char								reserved1[80];
    DmaEngineControlReg                 EngCtl;
    DmaQueueLinkedListPointersReg		WTReqQue [ VORTEX_WT_CHANNEL_COUNT/16 ];
    DmaQueueLinkedListPointersReg		VDBReqQue;
	uint32								WTSetStat[VORTEX_WT_CHANNEL_COUNT];
	uint32								VDBSetStat[VORTEX_VDB_CHANNEL_COUNT];
	char								reserved2[128];
	DmaChannelGetAbsolutePositionReg	WTGAP[VORTEX_WT_CHANNEL_COUNT];
	DmaChannelGetAbsolutePositionReg	VDBGAP[VORTEX_VDB_CHANNEL_COUNT];
	DmaRequestQueuePointerReg			WTReqQuePtr[VORTEX_WT_CHANNEL_COUNT];
	DmaRequestQueuePointerReg			VDBReqQuePtr[VORTEX_VDB_CHANNEL_COUNT];
}DmaRegs; /* 3.3 */

// same for 8820 and 8830
typedef	DmaChannelModeReg			DmaVDBChannelModeReg;
typedef	DmaChannelSizeGotoReg		DmaVDBChannelSizeGoto;
typedef	DmaChannelSizeAndGotoReg	DmaVDBChannelSizeAndGotoReg;

/*******************************************************************************
 * VDB Registers 
 *******************************************************************************/
typedef union
{
    struct
    {
	unsigned volatile DestAddr :8;
	unsigned volatile SrcAddr  :8;
	unsigned          :16;
    } Bits;
    uint32 volatile dwValue;
}VDBRamLink; /* Table 5.12 */

// changed		Table 5.3
/* Possible values for SrcAddr */
typedef enum
{
    VDB_SRC_FIFO	= 0,
    VDB_SRC_SRC		= 0x20,
    VDB_SRC_MIXER	= 0x30,
	VDB_SRC_WT0		= 0x40,
    VDB_SRC_XTALK	= 0x66,
	VDB_SRC_FX		= 0x68,
    VDB_SRC_CODEC	= 0x70,
    VDB_SRC_SERIAL	= 0x78,
    VDB_SRC_SPDIF	= 0x7a,
    VDB_SRC_AC98	= 0x7c,
    VDB_SRC_EQ		= 0x7e,
    VDB_SRC_WT1		= 0x80,
    VDB_SRC_A3D		= 0xa6,
    VDB_SRC_NULL	= 0xff,
}VDB_SRC_ADDR;

//changed		Table 5.4
/* Possible values for DestAddr */
typedef enum
{
    VDB_DEST_FIFO		= 0,
    VDB_DEST_FIFOA		= 0x20,
    VDB_DEST_SRC		= 0x40,
    VDB_DEST_MIXER		= 0x50,
    VDB_DEST_A3D		= 0x70,
    VDB_DEST_FX			= 0x80,
    VDB_DEST_CODEC		= 0x88,
    VDB_DEST_SERIAL		= 0x90,
    VDB_DEST_SPDIF		= 0x92,
    VDB_DEST_AC98		= 0x94,
    VDB_DEST_XTALK		= 0x96,
    VDB_DEST_EQ			= 0xa0,
	VDB_DEST_PIF		= 0xa4,
    VDB_DEST_NULL		= 0xff,
}VDB_DEST_ADDR;

/* Indexes into the header tag array */
// 5-2
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
    VDB_SR_SPDIF,
    VDB_SR_DSPDMA,
    VDB_SR_SRC22,
    VDB_SR_SRC23,
}VDB_SAMPLE_RATES;

// pg123 5.2.2.2
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
	unsigned volatile SRC22	 : 1;
	unsigned volatile SRC23	 : 1;
	unsigned            :8;
    } Bits;
    uint32 volatile dwValue;
}VDBSampleRateActiveReg; /* Table 5-16 */


typedef struct
{
    VDBRamLink DestLink[VORTEX_VDB_DESTINATION_COUNT];		// 173*4	= 692	(0 ~ 2b0)
    VDBRamLink HeadLink[VORTEX_VDB_HEADER_COUNT];			// 24*4		= 96	(2b4 ~ 310)
    uint32 reserved1[59];									// 236*1	= 236	(314 ~ 3fc)
    VDBSampleRateActiveReg SRActive;						// 4		= 4		(400)
    char filler[1020];										//					(404 ~ 7fc)
}VDBRegs; /* Table 5-11 */


/*******************************************************************************
 * Game Registers 
 *******************************************************************************/
// bits unclear in spec!  (no change)
// 18.2.1
typedef union
{
    struct
    {
	unsigned volatile MIDIData	: 8;
	unsigned					: 24;
    } Bits;
    uint32 volatile dwValue;
}GAMEMidiDataReg;


//18.2.2
typedef union
{
    struct
    {
	unsigned volatile MPU401Mode    : 1;
	unsigned						: 5;
	unsigned volatile WriteOK       : 1;
	unsigned volatile MIDIDataValid : 1;
	unsigned						: 24;
    } Bits;
    uint32 volatile dwValue;
}GAMEMidiCmdStatReg;

// 18.2.3
typedef union
{
    struct
    {
	unsigned volatile JoystickAxis		: 4;
	unsigned volatile JoystickButtons	: 4;
	unsigned							: 24;
    } Bits;
    uint32 volatile dwValue;
}GAMEJoystickPortReg;

// 18.2.4
typedef union
{
    struct
    {
	unsigned volatile MIDIDataValid		: 1;
	unsigned volatile MIDIRcvOverflow	: 1;
	unsigned volatile MIDIRcvReady		: 1;
	unsigned volatile MIDIWriteOK		: 1;
	unsigned volatile MIDIMode			: 2;
	unsigned volatile JoystickMode		: 1;
	unsigned							: 1;
	unsigned volatile ClockDivider		: 8;
	unsigned							: 16;
    } Bits;
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

// changed
typedef union
{
    struct
    {
	unsigned volatile FrameMode			: 1;
	unsigned volatile SampleRateSelect	: 2;
	unsigned volatile SCLKDirection		: 1;
	unsigned volatile SampleEdge		: 1;
	unsigned volatile SyncEvent			: 1;
	unsigned volatile SyncDirection		: 1;
	unsigned volatile ResetPort			: 1;
	unsigned							: 22;
	unsigned volatile VDB_SRB16event	: 1;
	unsigned volatile VDB_SRB16			: 1;
    } Bits;
    uint32 volatile dwValue;
}SERFXCtlReg; /* 16.3.2 */

// changed
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
	unsigned							   : 2;
	unsigned volatile AC97CodecRdyOverride : 1;
	unsigned volatile AC97ModemEnaOverride : 1;
	unsigned volatile AC97WarmReset        : 1;
	unsigned volatile AC97ColdReset        : 1;
	unsigned volatile AC97CodecTag         : 13;
	unsigned                      		   : 1;
	unsigned volatile VDB_SRB17event       : 1;
	unsigned volatile VDB_SRB17            : 1;
    } Bits;
    uint32 volatile dwValue;
}SERCodecIntCtlReg;/* 13.3.3 */

/* Possible values for SCLKdirection */
#define SER_CODEC_INPUT  0
#define SER_CODEC_OUTPUT 1

typedef union
{
    struct
    {
	unsigned volatile RightCmdStat : 16;
	unsigned volatile LeftCmdStat  : 16;
    } Bits;
    uint32 volatile dwValue;
}SERCodecCmdStatReg; /* 13.3.7 */

typedef union
{
    struct
    {
	unsigned volatile AC97CmdStatData   : 16;
	unsigned volatile AC97CmdStatAddr   : 7;
	unsigned volatile AC97CmdStatRWStat : 1;
	unsigned                   : 8;
    } Bits;
    uint32 volatile dwValue;
}SERAC97CmdStatReg; /* 13.3.7 */

typedef union
{
	union
	{
	    SERCodecCmdStatReg Codec;
	    SERAC97CmdStatReg AC97;
	}CtlStat;
    uint32 volatile dwValue;
}SERCmdStatReg;	/* 13.3.7 */

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
    } Bits;
    uint32 volatile dwValue;
}SERSPORTIntCtlReg; /* 15.3.2 */

// changed
typedef union
{
    struct
    {
	unsigned volatile FXCh0Ena		: 1;
	unsigned volatile FXCh1Ena		: 1;
	unsigned volatile FXCh2Ena		: 1;
	unsigned volatile FXCh3Ena		: 1;
	unsigned volatile FXCh4Ena		: 1;
	unsigned volatile FXCh5Ena		: 1;
	unsigned volatile FXCh6Ena		: 1;
	unsigned volatile FXCh7Ena		: 1;
	unsigned volatile CodecCh0Ena	: 1;
	unsigned volatile CodecCh1Ena	: 1;
	unsigned volatile CodecCh2Ena	: 1;
	unsigned volatile CodecCh3Ena	: 1;
	unsigned volatile CodecCh4Ena	: 1;
	unsigned volatile CodecCh5Ena	: 1;
	unsigned volatile CodecCh6Ena	: 1;
	unsigned volatile CodecCh7Ena	: 1;
	unsigned volatile SportCh0Ena	: 1;
	unsigned volatile SportCh1Ena	: 1;
	unsigned volatile SPDIFCh0Ena	: 1;
	unsigned volatile SPDIFCh1Ena	: 1;
	unsigned volatile AC98Slot11Ena : 1;
	unsigned volatile AC98Slot12Ena : 1;
	unsigned             : 10;
    } Bits;
    uint32 volatile dwValue;
}SERChannelEnableReg; /* 13.3.8 */

// not clear in specs yet - might have to update
typedef union
{
    struct
    {
	unsigned volatile SCLKRSelect		: 2;
	unsigned volatile CRCEnable	        : 1;
	unsigned volatile OutputEnable		: 1;
	unsigned						    : 3;
	unsigned volatile ResetPort	        : 1;
	unsigned							: 22;
	unsigned volatile VDB_SRB20event    : 1;
	unsigned volatile VDB_SRB20         : 1;
    } Bits;
    uint32 volatile dwValue;
}SPDIFCtlReg; /* 14.3.1 */

typedef union
{
    struct
    {
	unsigned volatile SubsampleCount : 8;
	unsigned volatile SampleCount    : 24;
    } Bits;
    uint32 volatile dwValue;
}SERSampleCounterReg; /* 13.4.1 */

typedef union
{
    struct
    {
	unsigned volatile TimerCounterValue : 18;
	unsigned                   : 13;
	unsigned volatile TimerIntStat      : 1;
    } Bits;
    uint32 volatile dwValue;
}SERSampleTimerReg; /* 13.4.2 */

typedef union
{
    struct
    {
	unsigned volatile SecondSDISelect	: 10;
	unsigned volatile SRB23Ctl			: 1;
	unsigned volatile SRB22Ctl			: 1;
	unsigned volatile SRB18Ctl			: 1;
	unsigned volatile SecondCodecTag	: 13;
	unsigned volatile VDB_SRB23event    : 1;
	unsigned volatile VDB_SRB23         : 1;
	unsigned volatile VDB_SRB22event    : 1;
	unsigned volatile VDB_SRB22         : 1;
	unsigned volatile VDB_SRB18event    : 1;
	unsigned volatile VDB_SRB18         : 1;
    } Bits;
    uint32 volatile dwValue;
}SecondCodecCtlReg; /* 13.3.4 */


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

typedef union
{
    struct
    {
	unsigned volatile PriSDIPMEStat		: 1;
	unsigned volatile PriSDIPMEEna		: 1;
	unsigned volatile PriSDIPMEIrqEna	: 1;
	unsigned		: 1;
	unsigned volatile SecSDIPMEStat		: 1;
	unsigned volatile SecSDIPMEEna		: 1;
	unsigned volatile SecSDIPMEIrqEna	: 1;
	unsigned		: 1;
	unsigned volatile GPIOIrqPending    : 1;
	unsigned volatile GPIOIrqEna	    : 1;
	unsigned		: 2;
	unsigned volatile GPIOReserved	    : 4;
	unsigned		: 16;
    } Bits;
    uint32 volatile dwValue;
}GPIOCtlReg; /* 13.3.6 */

typedef union
{
    struct
    {
	unsigned volatile Slot12			: 16;
	unsigned		: 16;
    } Bits;
    uint32 volatile dwValue;
}GPIODoubleBuffAliasReg; /* 13.3.5 */

/*
typedef union
{
    struct
    {
	unsigned 		: 16;
	unsigned		: 16;
    } Bits;
    uint32 volatile dwValue;
}ModemSDOCtlReg; // dunno
*/

// First dword of the 24 byte status bits
typedef union
{
    struct
    {
		unsigned volatile Mode				: 1;
		unsigned volatile AudioMode			: 1;
		unsigned volatile CopyRight			: 1;
		unsigned volatile Emphasis			: 1;
		unsigned                   			: 4;
		unsigned volatile CategoryCode		: 7;
		unsigned volatile GenerationStatus 	: 1;
		unsigned                   		   	: 8;
		unsigned volatile SampleRate		: 4;
		unsigned							: 4;
    }consumer_mode;

    struct
    {
		unsigned volatile Mode				: 1;
		unsigned volatile AudioMode			: 1;
		unsigned                    		: 1;
		unsigned volatile Emphasis			: 1;
		unsigned                    		: 2;
		unsigned volatile SampleRate		: 2;
		unsigned							: 24;
    }pro_mode;

    struct	// Screwy mapping requires us to write half of the uint32 at a time
    {
		unsigned volatile Low			: 16;
		unsigned volatile High			: 16;
    } Word;

    uint32 volatile dwValue;
}SPDIFStatusBitsReg; /* errata */

// Possible values for Mode
#define SER_SPDIF_CONSUMER_MODE (0)
#define SER_SPDIF_PRO_MODE      (1)

// Possible values for AudioMode
#define SER_SPDIF_PCM_MODE  (0)
#define SER_SPDIF_DATA_MODE (1)

// Possible values for SampleRate in consumer mode
#define SER_SPDIF_CMODE_32KHZ  (3)
#define SER_SPDIF_CMODE_441KHZ (0)
#define SER_SPDIF_CMODE_48KHZ  (2)

// Possible values for SampleRate in pro mode
#define SER_SPDIF_PMODE_32KHZ  (3)
#define SER_SPDIF_PMODE_441KHZ (1)
#define SER_SPDIF_PMODE_48KHZ  (2)

// Possible values for CategoryCode in consumer mode
#define SER_SPDIF_CATCODE_GENERAL	(0)
#define SER_SPDIF_CATCODE_DIGITAL_SIGNAL_MIXER (0x12)

// Possible values for GenerationStatus in consumer mode
#define SER_SPDIF_GENSTAT_COPY     (0)
#define SER_SPDIF_GENSTAT_ORIGINAL (1)

typedef struct
{
	uint32 EffectsChannelData[32];
    uint32 CodecChannelData[32];
    uint32 Sport0ChannelData[8];
	uint32 SPDIFChannelDate[8];
	uint32 AC98ChannelData[8];
    uint32 reserved1[8];
    SERFXCtlReg FXCtl;
    SERCodecIntCtlReg CodecCtl;
    SERCmdStatReg CodecCmdStat;
    SERSPORTIntCtlReg SportCtl;
    SERChannelEnableReg ChannelEnable;
	SPDIFCtlReg	SPDIFCtl;
    SERSampleCounterReg SampleCounter;
    SERSampleTimerReg SampleTimer;
	SecondCodecCtlReg SecondCodecCtl;
	GPIOCtlReg GPIOCtl;
	GPIODoubleBuffAliasReg	GPIODoubleBuffAlias;
	uint32 ModemSDOCtl;
	uint32 reserved2[8];
	SPDIFStatusBitsReg	SPDIFChannelStatusData0;
	uint32 SPDIFChannelStatusData[11];
    char filler[0x600];
}SerRegs; /* 13.1  */


/*******************************************************************************
 * Sound Blaster Registers 
 *******************************************************************************/
// no changes

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
    } Bits;
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
    } Bits;
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
// codec request added - changed
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
	unsigned volatile CodecRequest           :1;
	unsigned volatile MasterFatalToSERR      :1;
	unsigned volatile MasterParityToSERR     :1;
	unsigned volatile RegbusTimeoutToSERR    :1;
	unsigned volatile FifoUnderrunToSERR     :1;
	unsigned volatile DMANoBufferToSERR      :1;
	unsigned                        :10;
    } Bits;
    uint32 volatile dwValue;
}PciInterruptSCReg; /* Table 2-11 */

// changed
typedef union
{
    struct
    {
	unsigned volatile InterruptAsserted			:1;
	unsigned volatile SystemErrorAsserted		:1;
	unsigned volatile TargetTransmitFIFOFull	:1;
	unsigned volatile TargetTransmitFIFOEmpty	:1;
	unsigned volatile TargetReceiveFIFOEmpty	:1;
	unsigned volatile MasterTransmitFIFOFull	:1;
	unsigned volatile MasterTransmitFIFOEmpty	:1;
	unsigned volatile MasterReceiveFIFOEmpty	:1;
	unsigned volatile DMAInProgress				:1;
	unsigned volatile DMARequestIsWrite			:1;
	unsigned volatile DMARequestPending			:1;
	unsigned volatile PMECapable				:1;
	unsigned volatile Disable_pci_done			:1;
	unsigned volatile OscillatorInputLevel		:1;
	unsigned volatile GotDisconnect				:1;
	unsigned volatile AlwaysReadMulti			:1;
	unsigned volatile UseDMARead				:1;
	unsigned									:15;
    } Bits;
    uint32 volatile dwValue;
}PciGlobalStatusReg; /* Table 2-12 */

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
	unsigned volatile vdbArbiterReset         :1;
	unsigned volatile VortexSoftReset          :1;
	unsigned volatile MailboxSlot             :1;
	unsigned                         :7;
    } Bits;
    uint32 volatile dwValue;
}PciGlobalControlReg; /* Table 2-14 */

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
}PciRegs; /* Table 2-10 */


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
    } Bits;
    uint32 volatile dwValue;
}I2CCommandPortReg;

typedef union
{
    struct
    {
	unsigned volatile VendorId    : 16;
	unsigned volatile SubSystemId : 16;
    } Bits;
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
    } Bits;
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
    } Bits;
    uint32 volatile dwValue;
}PIFDMAConfigReg;

typedef union
{
    struct
    {
	unsigned volatile DMAFSMState      : 4;
	unsigned volatile ExternalFSMState : 5;
	unsigned                  : 23;
    } Bits;
    uint32 volatile dwValue;
}PIFFSMstatus;

typedef union
{
    struct
    {
		unsigned volatile GPIO0Value             : 1;
		unsigned volatile GPIO0OutEnable         : 1;
		unsigned volatile GPIO1Value             : 1;
		unsigned volatile GPIO1OutEnable         : 1;
		unsigned volatile GPIO2Value             : 1;
		unsigned volatile GPIO2OutEnable         : 1;
		unsigned volatile GPIO3Value             : 1;
		unsigned volatile GPIO3OutEnable         : 1;
		unsigned volatile GPIO1OutputSelect      : 1;
		unsigned volatile reserved1			     : 3;
		unsigned volatile dsp_req_Select	     : 2;
		unsigned volatile dsp_irq_n_Select	     : 2;
		unsigned volatile modem_irq_Select	     : 2;
		unsigned volatile SPDIF_sclk_in_Select   : 2;
		unsigned volatile Sec_CODEC_SD1_Select   : 2;
		unsigned volatile DSP_CODEC_SD0_PassThru : 2;
		unsigned                                 : 8;
    } Bits;
    uint32 volatile dwValue;
}PIFGPIOControl;

// changed
typedef struct
{
    uint32				volatile p16550regs[8];
    uint32				volatile DSPRegs[8];
    PIFPortConfigReg	volatile PortConfig;
    PIFDMAConfigReg		volatile DMAConfig;
    uint32				volatile DMAactive;
    uint32				volatile Reset;
    uint32				volatile Pins;
    uint32				volatile SRout;
    PIFFSMstatus		volatile FSMstatus;
	PIFGPIOControl		volatile GPIOControl;
    char				volatile reserved0[0xfa0];
    uint32				volatile RegBusDMA;
    char filler[0xffc];
}PifRegs;


/*******************************************************************************
 * Crosstalk Canceller Registers 
 *******************************************************************************/

typedef union
{
    struct
    {
		unsigned volatile Enable			: 1;
		unsigned volatile SStepEna			: 1;		// not supported in AU8830
		unsigned volatile SStepState		: 1;		// not supported in AU8830
		unsigned volatile SRateSel			: 5;
		unsigned volatile RightDelay		: 5;
		unsigned volatile LeftDelay			: 5;
		unsigned volatile RightDelayPtr		: 5;
		unsigned volatile LeftDelayPtr		: 5;
		unsigned volatile BiquadDelayPtr	: 1;
		unsigned volatile CtlMask			: 3;
    } Bits;
    uint32 volatile dwValue;
}XTalkCtlReg;

typedef union
{
    struct
    {
		unsigned volatile RAMWriteEnable	: 1;
		unsigned volatile SelRAMDin			: 2;
		unsigned volatile SelCReg			: 5;
		unsigned volatile SelBReg			: 3;
		unsigned volatile SelAReg			: 3;
		unsigned volatile BiquadFSMState	: 5;
		unsigned volatile MainFSMState		: 5;
		unsigned volatile BiquadDelayPtr	: 1;
		unsigned volatile CurrentSrate		: 1;
		unsigned volatile MixFSMState		: 5;
    } Bits;
    uint32 volatile dwValue;
}XTalkDebugReg;

typedef struct
{
	uint32 volatile a1;
	uint32 volatile a2;
	uint32 volatile b0;
	uint32 volatile b1;
	uint32 volatile b2;
	uint32 volatile y1;
	uint32 volatile y2;
	uint32 volatile err1;
	uint32 volatile err2;
}CoeffReg;

typedef struct
{
	uint32 volatile LeftDelayLO[32];
	uint32 volatile LeftDelayHI[32];
	uint32 volatile RightDelayLO[32];
	uint32 volatile RightDelayHI[32];
	CoeffReg EQ[10];
	CoeffReg XT[10];
	uint32 volatile IpGain[10];
	uint32 volatile LeftEqX1Hi;
	uint32 volatile LeftEqX2Hi;
	uint32 volatile LeftEqX1Lo;
	uint32 volatile LeftEqX2Lo;
	uint32 volatile RightEqX1Hi;
	uint32 volatile RightEqX2Hi;
	uint32 volatile RightEqX1Lo;
	uint32 volatile RightEqX2Lo;
	uint32 volatile LeftXTX1Hi;
	uint32 volatile LeftXTX2Hi;
	uint32 volatile LeftXTX1Lo;
	uint32 volatile LeftXTX2Lo;
	uint32 volatile RightXTX1Hi;
	uint32 volatile RightXTX2Hi;
	uint32 volatile RightXTX1Lo;
	uint32 volatile RightXTX2Lo;
	uint32 volatile LeftEqK;
	uint32 volatile LeftEqShift;
	uint32 volatile RightEqK;
	uint32 volatile RightEqShift;
	uint32 volatile LeftXTK;
	uint32 volatile LeftXTShift;
	uint32 volatile RightXTK;
	uint32 volatile RightXTShift;
	uint32 volatile MixLeftHi;
	uint32 volatile MixLeftLo;
	uint32 volatile MixRightHi;
	uint32 volatile MixRightLo;
	uint32 volatile ScratchHi;
	uint32 volatile ScratchLo;
}XtalkRamReg;

typedef struct
{
	XtalkRamReg RAM;
	char reserved1[0x90];
	uint32 volatile VDBDest[20];
	uint32 volatile VDBSource[4];
	XTalkCtlReg Control;
	XTalkDebugReg Debug;
	char reserved2[0x1998];
}XTalkRegs;

/*******************************************************************************
 * Graphic Equalizer Registers 
 *******************************************************************************/

#define NUM_GRAPH_BANDS					(10)

typedef union
{
    struct
    {
	unsigned volatile Enable			: 1;
	unsigned volatile SingleStepState	: 1;		// not supported in AU8830
	unsigned volatile SingleStepEnable	: 1;		// not supported in AU8830
	unsigned volatile SrateSelect		: 5;
	unsigned volatile DelayPtr			: 1;
	unsigned volatile reserved1			: 2;
	unsigned volatile ControlMask		: 5;
	unsigned volatile reserved2			: 16;
    } Bits;
    uint32 volatile dwValue;
}EqControlReg;

typedef struct
{
	uint32 volatile a1;
	uint32 volatile a2;
	uint32 volatile b0;
	uint32 volatile b1;
	uint32 volatile b2;
	uint32 volatile outz1;
	uint32 volatile outz2;
	uint32 volatile errz1;
	uint32 volatile errz2;
	uint32 volatile peak;
	uint32 volatile currgain;
	uint32 volatile targain;
}BandCoeff;

typedef struct
{
	BandCoeff Left[NUM_GRAPH_BANDS];
	BandCoeff Right[NUM_GRAPH_BANDS];
	uint32 volatile LevelMeterChannel;
	uint32 volatile GainTC;
	uint32 volatile LevelTC;
	uint32 volatile BypPeakLeft;
	uint32 volatile BypCurrGainLeft;
	uint32 volatile BypTarGainLeft;
	uint32 volatile A3DPeakLeft;
	uint32 volatile A3DCurrGainLeft;
	uint32 volatile A3DTarGainLeft;
	uint32 volatile BypPeakRight;
	uint32 volatile BypCurrGainRight;
	uint32 volatile BypTarGainRight;
	uint32 volatile A3DPeakRight;
	uint32 volatile A3DCurrGainRight;
	uint32 volatile A3DTarGainRight;
	uint32 volatile InZ1Left;
	uint32 volatile InZ2Left;
	uint32 volatile InZ1Right;
	uint32 volatile InZ2Right;
}EqRAMReg;

typedef struct
{
	EqRAMReg EqRAM;
	char reserved1[0x004];
	uint32 volatile VDBDest[8];
	uint32 volatile VDBSource[4];
	EqControlReg Control;
	char reserved2[0x3B8];
}EqRegs;


/*******************************************************************************
 * A3D Registers 
 *******************************************************************************/

#define NUM_A3D_SOURCES					(4)
#define NUM_HRTF_PARAMS					(56)
#define NUM_A3D_SLICES					(4)

typedef union
{
    struct
    {
	unsigned volatile Enable			: 1;
	unsigned volatile SingleStepState	: 1;		// not supported in AU8830
	unsigned volatile SingleStepEnable	: 1;		// not supported in AU8830
	unsigned volatile SrateSelect		: 5;
	unsigned volatile TrackingCnt		: 6;
	unsigned volatile reserved			: 13;
	unsigned volatile ControlMask		: 5;
    } Bits;
    uint32 volatile dwValue;
}A3DControlReg;

typedef union
{
    struct
    {
	unsigned volatile BRAMWriteEnable	: 1;
	unsigned volatile ARAMWriteEnable	: 1;
	unsigned volatile SelARAMDin		: 2;
	unsigned volatile SelCReg			: 5;
	unsigned volatile SelBReg			: 3;
	unsigned volatile SelAReg			: 3;
	unsigned volatile BiquadFSMState	: 5;
	unsigned volatile MainFSMState		: 5;
	unsigned volatile BiquadDelayPtr	: 1;
	unsigned volatile CurrentSrate		: 1;
	unsigned volatile MixFSMState		: 5;
    } Bits;
    uint32 volatile dwValue;
}A3DDebugReg;

typedef union
{
    struct
    {
	unsigned volatile ITDPtr			: 6;
	unsigned volatile HRTFRightPtr		: 6;
	unsigned volatile HRTFLeftPtr		: 6;
	unsigned volatile BiquadDelayPtr	: 1;
	unsigned volatile reserved			: 9;
	unsigned volatile PointerMask		: 4;
    } Bits;
    uint32 volatile dwValue;
}A3DPtrReg;

typedef union
{
	struct
	{
	unsigned volatile ExVal	: 16;
	unsigned volatile Val	: 16;
	} Bits;
    uint32 volatile dwValue;
}RAMReg;


typedef struct
{
	RAMReg HrtfCurrent[NUM_HRTF_PARAMS];
	RAMReg GainCurrent;
	RAMReg GainTarget;
	RAMReg A12Current;
	RAMReg A21Target;
	RAMReg B01Current;
	RAMReg B10Target;
	RAMReg B2Current;
	RAMReg B2Target;
	RAMReg HrtfTarget[NUM_HRTF_PARAMS];
	RAMReg ITDCurrent;
	RAMReg ITDTarget;
	RAMReg HrtfDelayLine[NUM_HRTF_PARAMS];
}BRAMSourceReg;

typedef struct
{
	RAMReg HrtfCurrent[NUM_HRTF_PARAMS];
	RAMReg GainCurrent;
	RAMReg GainTarget;
	RAMReg A12Current;
	RAMReg A21Target;
	RAMReg B01Current;
	RAMReg B10Target;
	RAMReg B2Current;
	RAMReg B2Target;
	RAMReg HrtfTarget[NUM_HRTF_PARAMS];
	RAMReg ITDCurrent;
	RAMReg ITDTarget;
	RAMReg HrtfDelayLine[NUM_HRTF_PARAMS];
	RAMReg ITDDelayLine[45];
	RAMReg HrtfTrackTC;
	RAMReg GainTrackTC;
	RAMReg CoeffTrackTC;
	RAMReg ITDTrackTC;
	RAMReg x1;
	RAMReg x2;
	RAMReg y1;
	RAMReg y2;
	RAMReg HrtfOutL;
	RAMReg HrtfOutR;
}ARAMSourceReg;

typedef struct
{
	struct
	{
		ARAMSourceReg ARAMSource[NUM_A3D_SOURCES];
		char reserved0[0x170];
		BRAMSourceReg BRAMSource[NUM_A3D_SOURCES];
		char reserved1[0xe0];
		uint32 volatile VDBDest[8];
		uint32 volatile VDBSource[4];
		uint32 volatile ABReg;
		uint32 volatile CReg;
		A3DControlReg Control;
		A3DDebugReg DebugReserved;				// do not touch!!!
		A3DPtrReg Pointers;
		char reserved2[0x3BC];
	}A3DSlice[NUM_A3D_SLICES];
}A3DRegs;

/*******************************************************************************
 * BAR0 Memory Address Map
 *******************************************************************************/
// changed
// table 2-4
typedef struct
{
    WTRegs		wt[VORTEX_TOTAL_WT_BLOCKS];		// 32 * 2 = 64K
    FIFORegs	fifo;							// 32
	A3DRegs		a3d;							// 32
    MixerRegs	mixer;							// 8
    PifRegs		pif;							// 8
	XTalkRegs	xtalk;							// 8
    SRCRegs		src;							// 4
    DmaRegs		dma;							// 2
    VDBRegs		vdb;							// 2
    GameRegs	game;							// 2
    SerRegs		ser;							// 2
    SBRegs		sb;								// 2
    PciRegs		pci;							// 2
    I2CRegs		i2c;							// 2
	EqRegs		eq;								// 2
    char reserved[0x14800];
}au8830regs;


#define OFFSET(m)	((uint32)&(((au8830regs*)0)->m)) // returns offset of the specified register in au8830regs structure 

#endif 
