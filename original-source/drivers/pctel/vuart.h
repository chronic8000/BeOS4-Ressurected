/* ================================================================================
 *	File:		vuart.h
 *
 *	    Copyright (C) 1999, PCtel, Inc. All rights reserved.
 *
 *	Purpose:	Header file for vuart.c
 *
 *	Author:		William Hsu <william_hsu@pctel.com>
 *
 *	Date:		07/14/99
 *
 *	Revision History:
 *
 *	Date		Who		What
 *	----		---		----
 *  07/14/99	whsu	Creation
 * ================================================================================
 */
#ifndef VUART_H
#define VUART_H

/* these must be a power of 2 */
#define COM_TxQlen 512
#define COM_RxQlen 512

#define XMIT_FIFO_SIZE	64

/* definitions for vUART_events */
#define vUART_TX_CHANGED	0x01

typedef struct
{
  /* tx and rx queue */
  int COM_TxQin;
  int COM_TxQout;
  int COM_RxQin;
  int COM_RxQout;
  unsigned char COM_TxQ[COM_TxQlen];
  unsigned char COM_RxQ[COM_RxQlen];

  /* uart tracking stuff */
  int vUART_events;
  unsigned char vUART_IIR;
  unsigned char vUART_oldMSR;

} vuart_struct;

/* Defined in vuart.c */
extern unsigned char get_uart_rx(void);
extern unsigned char get_uart_ier(void);
extern unsigned char get_uart_iir(void);
extern unsigned char get_uart_lcr(void);
extern unsigned char get_uart_mcr(void);
extern unsigned char get_uart_lsr(void);
extern unsigned char get_uart_msr(void);
extern unsigned char get_uart_scr(void);
extern unsigned char get_uart_dll(void);
extern unsigned char get_uart_dlm(void);

extern void put_uart_tx(unsigned char c);
extern void put_uart_ier(unsigned char c);
extern void put_uart_iir(unsigned char c);
extern void put_uart_lcr(unsigned char c);
extern void put_uart_mcr(unsigned char c);
extern void put_uart_lsr(unsigned char c);
extern void put_uart_msr(unsigned char c);
extern void put_uart_scr(unsigned char c);
extern void put_uart_dll(unsigned char c);
extern void put_uart_dlm(unsigned char c);

extern void PctelInitVUartVars(void);

extern unsigned short COM_TxQcount;
extern unsigned short COM_RxQcount;
extern vuart_struct vuart_info;

extern unsigned char   COM_Vier;
extern unsigned char   COM_Viir;
extern unsigned char   COM_Vlcr;
extern unsigned char   COM_Vmcr;
extern unsigned char   COM_Vlsr;
extern unsigned char   COM_Vmsr;
extern unsigned char   COM_Vscr;
extern short		COM_Vdll;

#endif	/* VUART_H */
