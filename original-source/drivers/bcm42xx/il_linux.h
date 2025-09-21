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
 * Linux-specific driver definitions for
 * Broadcom BCM42XX InsideLine(TM) Home Phoneline Networking Adapter.
 *
 * Copyright(c) 2000 Broadcom Corp.
 * $Id: il_linux.h,v 13.1 2000/04/25 06:19:18 nn Exp $
 */

#ifndef _il_linux_h_
#define _il_linux_h_

/* tunables */
#define	NTXD		256		/* # tx dma ring descriptors (must be ^2) */
#define	NRXD		256		/* # rx dma ring descriptors (must be ^2) */
#define	NRXBUFPOST	16		/* try to keep this # rbufs posted to the chip */
#define	BUFSZ		2048		/* packet data buffer size */
#define	HWRXOFF		30		/* chip rx buffer offset */
#define	RXBUFSZ		(BUFSZ - 256)	/* receive buffer size */
#define	RXOFF		(HWRXOFF + ILINE_HDR_LEN) /* make ip packet aligned(int) */
#define	TXOFF		(ILINE_HDR_LEN + 8)	/* transmit buffer offset */
#define	NSCB		32		/* # station control blocks in cache */
#define NREFID		64		/* # mac address blocks in cache */
#define	MAXDEBUGSTR	256		/* max size of debug string */

/* linux header files necessary for below */
#include <linux/config.h>
#ifdef MODULE
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#endif
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

/* printf and sprintf */
#define	printf	printk

/* register access macros */
#define	R_REG(r)	((sizeof *(r) == sizeof (uint32))? readl((uint32*)r): readw((uint16*)r))
#define	W_REG(r, v)	((sizeof *(r) == sizeof (uint32))? writel(v, r): writew(v, r))
#define	AND_REG(r, v)	W_REG((r), R_REG(r) & (v))
#define	OR_REG(r, v)	W_REG((r), R_REG(r) | (v))

/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	memcpy(dst, src, len)
#define	bcmp(b1, b2, len)	memcmp(b1, b2, len)
#define	bzero(b, len)		memset(b, '\0', len)

/* shared memory access macros */
#define	R_SM(r)		*(r)
#define	W_SM(r, v)	(*(r) = (v))
#define	BZERO_SM(r, len)	memset(r, '\0', len)

#endif	/* _il_linux_h_ */
