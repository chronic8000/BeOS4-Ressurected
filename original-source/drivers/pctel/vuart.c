/* ================================================================================
 *	File:		vuart.c
 *
 *	    Copyright (C) 1999, PCtel, Inc. All rights reserved.
 *
 *	Purpose:
 *	Virtual UART emulation functions.  Interfaces with PTSERIAL (or
 *	similar Serial driver code to emulate 8250 UART register behavior
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

//#include "serial_reg.h"
#include "vuart.h"

#define UART_IER_RLSI	0x04	
#define UART_IER_THRI	0x02
#define UART_IER_MSI	0x08
#define UART_IIR_NO_INT	0x01	
#define UART_IIR_ID	0x06	
#define UART_IIR_MSI	0x00	
#define UART_IIR_THRI	0x02	
#define UART_IIR_RDI	0x04	
#define UART_IIR_RLSI	0x06
#define UART_LSR_TEMT	0x40	/* Transmitter empty */
#define UART_LSR_THRE	0x20
#define UART_LSR_DR	0x01
#define UART_IER_RDI	0x01
#define UART_MSR_TERI	0x04
#define UART_MSR_RI	0x40
#define UART_MSR_ANY_DELTA 0x0F

vuart_struct vuart_info;

unsigned char   COM_Vier = 0;
unsigned char   COM_Viir = 0;
unsigned char   COM_Vlcr = 0;
unsigned char   COM_Vmcr = 0;
unsigned char   COM_Vlsr = 0;
unsigned char   COM_Vmsr = 0;
unsigned char	COM_Vscr = 0;
short			COM_Vdll = 0;

unsigned short	COM_TxQcount;
unsigned short	COM_RxQcount;

unsigned char get_uart_rx(void)
{
  unsigned char c;

  if (COM_RxQcount == 0)
    {
      return vuart_info.COM_RxQ[vuart_info.COM_RxQout];
    }
  
  /* get one char from the RX queue */
  c = vuart_info.COM_RxQ[vuart_info.COM_RxQout++];

  /* handle wrap around; assume COM_RxQlen is a power of 2 */
  vuart_info.COM_RxQout &= (COM_RxQlen-1);

  COM_RxQcount--;

//	dprintf(" - get_uart_rx(0x%02x)\n", c);

  return c;
}

unsigned char get_uart_ier(void)
{
  return COM_Vier;
}

unsigned char get_uart_iir(void)
{
  static unsigned short com_viir;

  /* The COM_Viir register values are different than traditional UART.
     The bitmap is similar to Interrupt Enable Register */
#if 0	/* whsu debug tx */
  if ((vuart_info.vUART_events & vUART_TX_CHANGED) &&
      (COM_TxQcount <= 0))
#else
  if ((vuart_info.vUART_events & vUART_TX_CHANGED) &&
      (COM_TxQcount < XMIT_FIFO_SIZE))
#endif
    {
      COM_Viir |= UART_IER_THRI;
	  vuart_info.vUART_events &= ~vUART_TX_CHANGED;
    }
  else
    {
      COM_Viir &= ~UART_IER_THRI;
    }

  vuart_info.vUART_IIR &= 0xf0;

  com_viir = COM_Viir & COM_Vier;

  if (com_viir & UART_IER_RLSI)
    vuart_info.vUART_IIR |= UART_IIR_RLSI;
  else if (com_viir & UART_IER_RDI)
    vuart_info.vUART_IIR |= UART_IIR_RDI;
  else if (com_viir & UART_IER_THRI)
    vuart_info.vUART_IIR |= UART_IIR_THRI;
  else if (com_viir & UART_IER_MSI)
    vuart_info.vUART_IIR |= UART_IIR_MSI;
  else vuart_info.vUART_IIR |= UART_IIR_NO_INT;

  return vuart_info.vUART_IIR;
}

unsigned char get_uart_lcr(void)
{
  return COM_Vlcr;
}

unsigned char get_uart_mcr(void)
{
  return COM_Vmcr;
}

unsigned char get_uart_lsr(void)
{
  if (COM_RxQcount == 0)
    {
      COM_Vlsr &= ~UART_LSR_DR;
      COM_Viir &= ~UART_IER_RDI;
    }

#if 0	/* whsu */
  if ((COM_TxQlen - COM_TxQcount) > 2)
#else
  if (COM_TxQcount < XMIT_FIFO_SIZE)
#endif
    {
      COM_Vlsr |= UART_LSR_TEMT | UART_LSR_THRE;
    }
  else
	{
	  COM_Vlsr &= ~(UART_LSR_TEMT | UART_LSR_THRE);
	}

  return COM_Vlsr;
}

unsigned char get_uart_msr(void)
{
  unsigned char msrDiff;

  COM_Vmsr &= 0xf0;

  msrDiff = (((COM_Vmsr ^ vuart_info.vUART_oldMSR) >> 4) & UART_MSR_ANY_DELTA);

  /* on the low to high transition of RING, disable RI line change */
  if ((msrDiff & UART_MSR_TERI) & (COM_Vmsr & UART_MSR_RI))
    msrDiff &= ~UART_MSR_TERI;

  if (msrDiff & (COM_Vier & UART_IER_MSI))
    COM_Viir |= UART_IER_MSI;

  vuart_info.vUART_oldMSR = COM_Vmsr | msrDiff;
  
  return vuart_info.vUART_oldMSR;
}

unsigned char get_uart_scr(void)
{
  return COM_Vscr;
}

unsigned char get_uart_dll(void)
{
  return (COM_Vdll & 0xff);
}

unsigned char get_uart_dlm(void)
{
  return ((COM_Vdll >> 8) & 0xff);
}


void put_uart_tx(unsigned char c)
{
//	dprintf(" - put_uart_tx(0x%02x)\n", c);
  /* check for overflow condition; drop chars if true */
  if ((COM_TxQlen - COM_TxQcount) <= 2)
	{
	  return;
	}

  vuart_info.COM_TxQ[vuart_info.COM_TxQin++] = c;

  /* handle wrap around; assume COM_TxQlen is a power of 2 */
  vuart_info.COM_TxQin &= (COM_TxQlen-1);

  COM_TxQcount++;

  vuart_info.vUART_events |= vUART_TX_CHANGED;

  /* printk("put:%c\n", c); */
}

void put_uart_ier(unsigned char c)
{
  COM_Vier = c;
}

void put_uart_iir(unsigned char c)
{
  /* READ ONLY register, do nothing */
  return;
}

void put_uart_lcr(unsigned char c)
{
  COM_Vlcr = c;
}

void put_uart_mcr(unsigned char c)
{
  COM_Vmcr = c;
}

void put_uart_lsr(unsigned char c)
{
  /* Not sure why we want to do this */
  COM_Vlsr = c;
}

void put_uart_msr(unsigned char c)
{
  /* Not sure why we want to do this */
  COM_Vmsr = c;
}

void put_uart_scr(unsigned char c)
{
  COM_Vscr = c;
}

void put_uart_dll(unsigned char c)
{
  COM_Vdll &= 0xff00;
  COM_Vdll |= c;
}

void put_uart_dlm(unsigned char c)
{
  COM_Vdll &= 0x00ff;
  COM_Vdll |= c << 8;
}

void PctelInitVUartVars(void)
{
  COM_TxQcount = 0;
  vuart_info.COM_TxQin = 0;
  vuart_info.COM_TxQout = 0;
  COM_RxQcount = 0;
  vuart_info.COM_RxQin = 0;
  vuart_info.COM_RxQout = 0;

  vuart_info.vUART_events |= vUART_TX_CHANGED;
  vuart_info.vUART_IIR = 0;
  vuart_info.vUART_oldMSR = 0;
}
