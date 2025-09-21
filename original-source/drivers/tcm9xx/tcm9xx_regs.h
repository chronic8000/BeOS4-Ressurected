#ifndef _TCM9XX_REGS_H
#define _TCM9XX_REGS_H

/* There are eight register windows, with the Command and IntStatus registers
 * available in each. */
#define Command						0x0e
#define IntStatus					0x0e
#define SET_WINDOW(win_num)			write16(device->base + Command, SelectWindow | (win_num))

/*   The top five bits written to the Command register are a command, the lower
 * 11 bits are the parameter, if applicable.
 *   Note that 11 parameters bits was fine for ethernet, but the new chip
 * can handle FDDI length frames (~4500 octets) and now parameters count
 * 32-bit 'Dwords' rather than octets. */
enum _Command {
    TotalReset = 0 << 11, SelectWindow = 1 << 11, StartCoax = 2 << 11,
    RxDisable = 3 << 11, RxEnable	= 4 << 11, RxReset	= 5 << 11,
    UpStall = 6 << 11, UpUnstall = ( 6 << 11 ) + 1, DownStall = ( 6 << 11 ) + 2,
    DownUnstall = ( 6 << 11 ) + 3, RxDiscard = 8 << 11, TxEnable = 9 << 11,
    TxDisable = 10 << 11, TxReset = 11 << 11, FakeIntr = 12 << 11,
    AckIntr = 13 << 11, SetIntrEnb = 14 << 11, SetStatusEnb = 15 << 11,
    SetRxFilter = 16 << 11, SetRxThreshold = 17 << 11, SetTxThreshold = 18 << 11,
    SetTxStart = 19 << 11, StartDMAUp = 20 << 11, StartDMADown = ( 20 << 11 ) + 1,
    StatsEnable = 21 << 11, StatsDisable = 22 << 11, StopCoax = 23 << 11,
    SetFilterBit = 25 << 11,
};

/* The SetRxFilter command accepts the following classes: */
enum _RxFilter
{
    RxStation	= 1, RxMulticast	= 2,
    RxBroadcast	= 4, RxProm	= 8
};

/* Bits in the general status register. */
enum _IntStatus {
    IntLatch = 0x0001, HostError = 0x0002,
    TxComplete = 0x0004, TxAvailable = 0x0008,
    RxComplete = 0x0010, RxEarly = 0x0020,
    IntReq = 0x0040, StatsFull	= 0x0080,
    DMADone = 1 << 8,  	/* Definition doesn't match 90x manual */
    LinkEvent	= 1 << 8, DownComplete	= 1 << 9,
    UpComplete = 1 << 10,
    DMAInProgress = 1 << 11, 	/* DMA controller is still busy.*/
    CmdInProgress = 1 << 12, 	/* Command register is still busy.*/
};

/* Register window 1 offsets, the window used in normal operation.
 * On the Vortex this window is always mapped at offsets 0x10-0x1f. */
enum _Window1 {
    TX_FIFO = 0x10, RX_FIFO = 0x10,
    RxErrors = 0x14, RxStatus = 0x18,
    Timer	= 0x1A, TxStatus = 0x1B,
    TxFree = 0x1C, 		/* Remaining free bytes in Tx buffer. */
};

enum _Window0 {
    Wn0EepromCmd = 10, 		/* Window 0: EEPROM command register. */
    Wn0EepromData = 12, 		/* Window 0: EEPROM results register. */
    IntrStatus	= 0x0E, 		/* Valid in all windows. */
};
enum _Win0_EEPROM_bits {
    EEPROM_Read = 0x80,
    EEPROM_WRITE = 0x40,
    EEPROM_ERASE = 0xC0,
    EEPROM_EWENB = 0x30, 		/* Enable erasing/writing for 10 msec. */
    EEPROM_EWDIS = 0x00, 		/* Disable EWENB before 10 msec timeout. */
};

/* EEPROM locations. */
enum _EEPROM_offsets {
    PhysAddr01	= 0, PhysAddr23	= 1, PhysAddr45	= 2, ModelID	= 3,
    EtherLink3ID = 7, IFXcvrIO	= 8, IRQLine	= 9, NodeAddr01	= 10,
    NodeAddr23	= 11, NodeAddr45	= 12, DriverTune	= 13, Checksum	= 15
};

/* Window 2. */
enum _Window2 {
    Wn2_ResetOptions = 12,
};

/* Window 3: MAC/config bits. */
enum _Window3 {
    Wn3_Config	= 0, Wn3_MAC_Ctrl	= 6,
    Wn3_Options	= 8,
};
union wn3_config {
    int i;
    struct w3_config_fields {
unsigned int ram_size:
3, ram_width:
1, ram_speed:
2, rom_size:
        2;
int pad8:
        8;
unsigned int ram_split:
2, pad18:
2, xcvr:
4, autoselect:
        1;
int pad24:
        7;
    }
    u;
};

/* Window 4: Xcvr/media bits. */
enum _Window4 {
    Wn4_FIFODiag	= 4, Wn4_NetDiag = 6, Wn4_PhysicalMgmt = 8, Wn4_Media = 10,
};
enum _Win4_Media_bits {
    Media_SQE = 0x0008, 	/* Enable SQE error counting for AUI. */
    Media_10TP = 0x00C0, 	/* Enable link beat and jabber for 10baseT. */
    Media_Lnk = 0x0080, 	/* Enable just link beat for 100TX/100FX. */
    Media_LnkBeat	= 0x0800,
};

/* Window 7: Bus Master control. */
enum _Window7 {
    Wn7_MasterAddr = 0, Wn7_MasterLen = 6, Wn7_MasterStatus	= 12,
};
/* Boomerang bus master control registers. */
enum MasterCtrl {
    PktStatus = 0x20, DownListPtr = 0x24, FragAddr = 0x28, 
	FragLen = 0x2c, DownPollRate = 0x2d, 
    TxFreeThreshold = 0x2f, UpPktStatus = 0x30,
    UpListPtr = 0x38, UpPollRate = 0x3d,
};
enum DmaCtrl_bits {
    upAltSeqDisable	= ( 1 << 16 ), defeatMWI	= ( 1 << 20 ), defeatMRL	= ( 1 << 21 ),
};


/* The Rx and Tx descriptor lists.  Note also the 8 byte alignment contraint on
 * tx_desc[] and rx_desc[]. */
#define LAST_FRAG  0x80000000			/* Last Addr/Len pair in descriptor. */

typedef struct _boom_rx_desc {
    uint32 next; 						/* Last entry points to 0.   */
    int32 status;
    uint32 addr; 						/* Up to 63 addr/len pairs possible. */
    int32 length; 						/* Set LAST_FRAG to indicate last pair. */
}
boom_rx_desc;
/* Values for the Rx status entry. */
enum rx_desc_status {
    RxDComplete	= 0x00008000,
    RxDError	= 0x00004000,
	RxDStalled	= 0x00002000,
    /* See boomerang_rx() for actual error bits */
    IPChksumErr	= 1 << 25,
    TCPChksumErr	= 1 << 26,
    UDPChksumErr	= 1 << 27,
    IPChksumValid	= 1 << 29,
    TCPChksumValid	= 1 << 30,
    UDPChksumValid	= 1 << 31,
};

typedef struct _boom_tx_desc {
    uint32 next; 						/* Last entry points to 0.   */
    int32 status; 						/* bits 0:12 length, others see below.  */
    uint32 addr;
    int32 length;
}
boom_tx_desc;
/* Values for the Tx status entry. */
enum tx_desc_status
{
    CRCDisable	= 0x00002000, TxDComplete	= 0x00008000,
    AddIPChksum	= 0x02000000, AddTCPChksum	= 0x04000000,
    AddUDPChksum	= 0x08000000,
    TxIntrUploaded	= 0x80000000, 		/* IRQ when in FIFO, but maybe not sent. */
};

/* Chip features we care about in device->capabilities, read from the EEPROM. */
enum _ChipCaps {
    CapBusMaster	= 0x20,
    CapPwrMgmt	= 0x2000
};

#endif // _TCM9XX_REGS_H
