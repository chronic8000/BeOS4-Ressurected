/*
	Resource.h

	russ 5/11/99
 
	(c) 1997-99 Be, Inc. All rights reserved

*/

#ifndef	_RESOURCE_H
#define	_RESOURCE_H

/************* Hamachi resources ************/

static const char *k_DeviceName = "net/pkgig/";
 
enum DescriptionStatusBits{
	DescOwn = 0x8000,
	DescEndPacket = 0x4000,
	DescEndRing = 0x2000,
	DescIntr = 0x1000,
};

/* Hamachi register offsets */
enum HamachiMemoryOffsets{
	ChipRev = 0x068,
	MacAddr = 0x0D2,
	AutoNegCtrl = 0x0E0,
	AutoNegStatus = 0x0E2,
	AutoNegChngCtrl = 0x0E4,
	AutoNegAdvertise = 0x0E8,
	ChipReset = 0x06B,
	RxPtr = 0x028,
	TxPtr = 0x08,
	FIFOcfg = 0x0F8,
	RxChecksum = 0x076,
	MACCnfg = 0x0A0,
	FrameGap0 = 0x0A2,
	FrameGap1 = 0x0A4,
	MACCnfg2 = 0x0B0,
	FlowCtrl = 0x0BC,
	MaxFrameSize = 0x0CE,
	LEDCtrl = 0x06C,
	TxIntrCtrl = 0x078,
	RxIntrCtrl = 0x07C,
	AddrMode = 0x0D0,
	InterruptEnable = 0x080,
	EventStatus = 0x08C,
	RxDMACtrl = 0x020,
	TxDMACtrl = 0x00,
	RxCmd = 0x024,
	TxCmd = 0x04,
	TxStatus = 0x06,
	RxStatus = 0x026,
	InterruptClear = 0x084,	
};

/* Interrupt status bit masks after reading
   hamachi_offsets::InterruptClear. */
enum InterruptClearMask{
	IntrRxDone = 0x01,
	IntrTxDone = 0x0100,
	LinkChange = 0x010000,
	NegotiationChange = 0x02000,
};

/* IO Control messages. */
enum IOControl {
	ETHER_GETADDR =  B_DEVICE_OP_CODES_END,
	ETHER_INIT,
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE						/* get frame size */
};


enum ManufactureID {
	VendorID = 0x1318,
	DeviceID = 0x911,
};


/* The hamachi expects separate transmit (tx) and receives (rx)
   descriptors with the a pointer the transfer data. I've organized
   as a ring buffers. The following sets the ring size. */
const uint16 k_packetLen = 1536;

/* The rings must be a power of 2! There is a var ringsizemask 
   that uses powers of 2. */
const uint16 k_rxRingSize = 32;
const uint16 k_txRingSize = 16;

#endif /*_RESOURCE_H */


