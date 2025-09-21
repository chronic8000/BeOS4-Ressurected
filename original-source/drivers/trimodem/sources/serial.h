/*==============================================================================================
File:       serial.h

Purpose:    BeOS function for DDAL

Author:     Bryan Pong, April 2000
            Copyright (C) 2000 by Trisignal Communications Inc.
                            All Rights Reserved
===============================================================================================*/
#ifndef SERIAL_H
#define SERIAL_H

#define USELOCK			// Uses locks to protect the hook functions
//#define USESEM			// use semaphores to protect open, close, read, write and control functions. 
//#define USEALLOC			// use memory module allocs (static vars if not defined)
#define USEINITHW			// use hardware check in init hardware
//#define USEFORCEDEBUG	    // use to activate debugger when modem active
#define USEDEBUG			// use to execute dprintfs

/* definitions */
/* register offsets for the 16550A UART */
#define	RBR	0
#define	THR	0
#define	DLL	0
#define	DLH	1
#define	IER	1
#define	ISR	1
#define	FCR	2
#define	IIR	2
#define	LCR	3
#define	MCR	4
#define	LSR	5
#define	MSR	6

/* register bits */
#define	LSR_THRE          (1 << 5)
#define	LSR_TSRE          (1 << 6)
#define LCR_7BIT          0x02
#define LCR_8BIT          0x03
#define LCR_2STOP         (1 << 2)
#define LCR_PARITY_ENABLE (1 << 3)
#define LCR_PARITY_EVEN   (1 << 4)
#define LCR_PARITY_MARK	  (1 << 5)
#define LCR_BREAK         (1 << 6)
#define	LCR_DLAB          (1 << 7)
#define MCR_DTR           (1 << 0)
#define MCR_RTS           (1 << 1)
#define MCR_IRQ_ENABLE    (1 << 3)
#define	MSR_DCTS          (1 << 0)
#define	MSR_DDCD          (1 << 3)
#define	MSR_CTS           (1 << 4)
#define	MSR_DSR           (1 << 5)
#define	MSR_RI            (1 << 6)
#define	MSR_DCD           (1 << 7)
/*
#define	IER_RDR           (1 << 0)
#define	IER_THRE          (1 << 1)
#define	IER_LS            (1 << 2)
#define	IER_MS            (1 << 3)
#define	FCR_ENABLE        (1 << 0)
#define	FCR_RX_RESET      (1 << 1)
#define	FCR_TX_RESET      (1 << 2)
#define	FCR_RX_TRIGGER_8  0x80
*/
/* external functions */
extern uint32 transmit_chars(unsigned char* gmcBuffer, uint32 maxByte);
extern uint32 get_num_chars_in_txbuf(void);
extern uint32 receive_chars(unsigned char* gmcBuffer, uint32 gmcBufSize);

extern unsigned char get_register_value(uint16 offset);
extern void set_register_value(uint16 offset, unsigned char val);

extern void setBaudRate(uint16 brate);

extern uint32 getControlMode(void);

#endif
