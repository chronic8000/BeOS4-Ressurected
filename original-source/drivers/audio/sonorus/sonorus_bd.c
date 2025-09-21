#include <PCI.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <OS.h>

#include "sonorus.h"

#if __POWERPC__
#define EIEIO() __eieio()
#undef	NASM_SUPPORT
#else
#define EIEIO()
#define NASM_SUPPORT 1   
#endif

extern sonorus_dev cards[MAX_STUDIO_BD];

/* explicit declarations */
void xfer_32_32(int32 *, int32 *, uint32);
void xfer_32_shr_32(int32 *, int32 *, uint32 );
void xfer_32_shl_32(int32 *, int32 *, uint32 );

/* These routines should track the ones in StudioBd.c */
/* Note that these are NOT endian independent         */
/* We will need to add "pciAlign" or its equivalent   */

void
set_host_ctrl(
	int ix,
	uint32 data)
{
	EIEIO();
	cards[ix].dsp->hctr = data;
	EIEIO();
	spin(1);
}

uint32
get_host_ctrl(
	int ix)
{	
	uint32	ctrl;
	
	EIEIO();
	ctrl = cards[ix].dsp->hctr;
	EIEIO();
	return ctrl;
}

void
set_host_ctrl_bits(
	int ix,
	uint32 mask,
	uint32 bits)
{
	uint32	data;
	
	data = get_host_ctrl(ix);
	data &= ~mask;
	data |=  bits;
	set_host_ctrl( ix, data );
}

uint32
get_host_stat(
	int ix)
{
	uint32 stat;

	EIEIO();
	stat = cards[ix].dsp->hstr;
	EIEIO();
	return stat;
}

bool
can_send_host_cmd(
	int ix)
{
	uint32 hcvr;
	EIEIO();
	hcvr = cards[ix].dsp->hcvr;
	EIEIO();
	return (hcvr & M_HCVR_HC) == 0;
}

void
send_host_cmd(
	int ix,
	uint32 cmd)
{
	uint32	timeout = 0;

	while (!can_send_host_cmd(ix)) {
		if (++timeout > 256) {
			kprintf("sonorus: timeout waiting to send command %ld\n", cmd);
			spin(1000);
			break;
		}
	}
	EIEIO();
	cmd &= M_HCVR_HVMSK;
	cmd |= M_HCVR_HC;
	cards[ix].dsp->hcvr = cmd;
	EIEIO();
}

void
send_host_cmd_nmi(
	int ix,
	uint32 cmd)
{
	EIEIO();
	cmd &= M_HCVR_HVMSK;
	cmd |= M_HCVR_HC | M_HCVR_HNMI;
	cards[ix].dsp->hcvr = cmd;
	EIEIO();
}

bool
can_write_host(
	int ix)
{
	return (get_host_stat(ix) & M_HSTR_HTRQ) != 0;
}

void
write_host_word(
	int ix,
	uint32 word)
{
	if (!can_write_host(ix)) {
		dprintf("sonorus: host FIFO overrun!\n");
		spin(1000);
		if (!can_write_host(ix)) {
		dprintf("sonorus: host FIFO overrun (fatal)\n");		
#ifdef DEBUG
			debugger("ack");
#endif
		}	
	}
	cards[ix].dsp->hdat[0] = word;
//	cards[ix].hdata[0] = word;
	// write count would go here
	EIEIO();
}

#define BSWAP(x) (((uint32) (x)) >> 24) | (((uint32) (x)) << 24) | ((((uint32) (x)) >> 8) & 0x0000FF00L) | ((((uint32) (x)) << 8) & 0x00FF0000L)

void
write_host_buf(
	int ix,
	void* buf,
	uint32 len,
	studXferType_t xferType,
	void* args )
{
#ifndef NASM_SUPPORT
	uint32*				lpbuf = (uint32*) buf;
#endif
	volatile uint32*	tp = (uint32*) cards[ix].hdata;

	switch (xferType)
	{
	case kStudXfer32:
#ifdef NASM_SUPPORT
#ifdef NO_SHIFT
		xfer_32_32((int32 * ) tp , (int32*) buf , len );
#else
		xfer_32_shr_32((int32 * ) tp , (int32*) buf , len );
#endif
#else
//don't use	memcpy with hardware
		while (len-- > 0)
		{
			*(tp++) = *(lpbuf++); // >>8;
		}
#endif
		break;
		
	case kStudXfer16:
	case kStudXfer16Gain:
	case kStudXfer16Packed:
	case kStudXferFltGain:
	default:
		dprintf( "sonorus: Unknown/unsupported xfer type in write_host_buf\n" );
		break;
	}
	EIEIO();
	// write count would go here
}

bool
can_read_host(
	int ix)
{
	return (get_host_stat(ix) & M_HSTR_HRRQ) != 0;
}

uint32
read_host_word(
	int ix)
{
	uint32	data, timeout;
	
	timeout = 0;
	while (!can_read_host(ix)) {
		if (timeout++ > 256) {
			kprintf("sonorus: read_host_word timeout!\n");
			spin(1000);
			break;
		}
	}
	data = cards[ix].dsp->hdat[0];
//	data = cards[ix].hdata[0];
	EIEIO();
	// read count would go here
	return data;
}

void
read_host_buf(
	int ix,
	void* buf,
	uint32 len,
	studXferType_t xferType,
	void* args )
{
#ifndef NASM_SUPPORT
	uint32*				lpbuf = (uint32*) buf;
#endif
	volatile uint32*	tp = (uint32*) cards[ix].hdata;
	
	switch (xferType)
	{
	case kStudXfer32:
	
#ifdef NASM_SUPPORT	
#ifdef NO_SHIFT
		xfer_32_32((int32 * ) buf , (int32*) tp , len );
#else
		xfer_32_shl_32((int32 * ) buf , (int32*) tp , len );
#endif

#else
//don't use	memcpy with hardware
		while (len-- > 0)
		{
			*(lpbuf++) = *(tp++); //  <<8;
		}
#endif
		break;
	case kStudXfer16:
	case kStudXfer16Gain:
	case kStudXfer16Packed:
	case kStudXferFltGain:
	default:
		kprintf( "sonorus: Unknown or unsupported xfer type (read_host_buf)\n" );
	}
	
	EIEIO();
	// read count would go here
}
