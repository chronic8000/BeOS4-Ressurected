/*
 * Modified for BeOS
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 */
/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.
*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
*/
/*
 * BeOS-specific driver definitions for
 * Broadcom BCM42XX InsideLine(TM) Home Phoneline Networking Adapter.
 *
 * Copyright(c) 2000 Broadcom Corp.
 * $Id: il_linux.h,v 13.1 2000/04/25 06:19:18 nn Exp $
 */

#ifndef _il_beos_h_
#define _il_beos_h_
#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <ByteOrder.h>


/* tunables */
#define	NTXD		256		/* # tx dma ring descriptors (must be ^2) */
#define	NRXD		256		/* # rx dma ring descriptors (must be ^2) */
#define	NRXBUFPOST	16		/* try to keep this # rbufs posted to the chip */
#define	BUFSZ		2048	/* packet data buffer size -- should divide B_PAGE_SIZE evenly */
#define	HWRXOFF		30		/* chip rx buffer offset */
#define	RXBUFSZ		(BUFSZ - 256)	/* receive buffer size */
#define	RXOFF		(HWRXOFF + ILINE_HDR_LEN) /* make ip packet aligned(int) */
#define	TXOFF		(ILINE_HDR_LEN + 8)	/* transmit buffer offset */
#define	NSCB		32		/* # station control blocks in cache */
#define NREFID		64		/* # mac address blocks in cache */
#define	MAXDEBUGSTR	256		/* max size of debug string */

/* printf and sprintf */
#define	printf	dprintf
int   sprintf(char *s, const char *format, ...);

/* memory mapped bus access */
#define read8(address)   				(*((volatile uint8*)(address)))
#define read16(address)  				(*((volatile uint16*)(address)))
#define read32(address) 				(*((volatile uint32*)(address)))
#define write8(address, data)  			(*((volatile uint8 *)(address)) = data)
#define write16(address, data) 			(*((volatile uint16 *)(address)) = (data))
#define write32(address, data) 			(*((volatile uint32 *)(address)) = (data))

/* register access macros */
#define	R_REG(r)	((sizeof *(r) == sizeof (uint32))? read32((uint32*)r): read16((uint16*)r))
#define	W_REG(r, v)	((sizeof *(r) == sizeof (uint32))? write32(r, v): write16(r, v))
#define	AND_REG(r, v)	W_REG((r), R_REG(r) & (v))
#define	OR_REG(r, v)	W_REG((r), R_REG(r) | (v))

/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)    memcpy(dst, src, len)
#define	bcmp(b1, b2, len)       memcmp(b1, b2, len)
#define	bzero(b, len)           memset(b, '\0', len)

/* shared memory access macros */
#define	R_SM(r)		*(r)
#define	W_SM(r, v)	(*(r) = (v))
#define	BZERO_SM(r, len)	memset(r, '\0', len)
#define	FLUSH(a, n, f)
// clear_caches(a,n, B_FLUSH_DCACHE)
#endif	/* _il_beos_h_ */
