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
/*******************************************************************************
 * $Id: bcmendian.h,v 1.14 2000/04/25 06:14:32 nn Exp $
 * local version of endian.h - byte order defines
 ******************************************************************************/

#ifndef _BCMENDIAN_H_
#define _BCMENDIAN_H_

#include "typedefs.h"

#if defined(__BEOS__)

#include <ByteOrder.h>

#elif defined(TARGETENV_freebsd)

#include <machine/endian.h>

#elif defined(TARGETENV_sun4)

#include <sys/byteorder.h>
#define	LITTLE_ENDIAN	4321
#define	BIG_ENDIAN	1234
#define	BYTE_ORDER	BIG_ENDIAN

#elif defined(TARGETENV_cygwin32)

#include <asm/byteorder.h>

#elif defined(TARGETENV_win32)|| defined(_WIN32) || defined(TARGETENV_klsi)

#define LITTLE_ENDIAN 4321
#define BIG_ENDIAN    1234
#define BYTE_ORDER    LITTLE_ENDIAN

#elif defined(TARGETOS_vxWorks)

#define	LITTLE_ENDIAN	_LITTLE_ENDIAN
#define	BIG_ENDIAN	_BIG_ENDIAN
#define	BYTE_ORDER	_BYTE_ORDER

#elif defined(_C6X_)

#define LITTLE_ENDIAN 4321
#define BIG_ENDIAN    1234
#if defined(_LITTLE_ENDIAN)
#define BYTE_ORDER    LITTLE_ENDIAN
#else
#define BYTE_ORDER    BIG_ENDIAN
#endif

#elif linux
#include <asm/byteorder.h>
#define	LITTLE_ENDIAN	__LITTLE_ENDIAN
#define	BIG_ENDIAN	__BIG_ENDIAN
#ifdef __LITTLE_ENDIAN
#define	BYTE_ORDER	LITTLE_ENDIAN
#else
#define	BYTE_ORDER	BIG_ENDIAN
#endif

#else
#error "endian.h did not determine the host arch"
#endif

/* Byte swap a 16 bit value */
static INLINE uint16
swap16(uint16 val)
{
	return (uint16)((val << 8) | (val >> 8));
}

/* Byte swap a 32 bit value */
static INLINE uint32
swap32(uint32 val)
{
	uint32 even_mask = 0x00FF00FF;

	val = ((val & even_mask) << 8) | ((val >> 8) & even_mask);
	val = val << 16 | val >> 16;

	return val;
}

/* buf	- start of buffer of shorts to swap		*/
/* len  - byte length of buffer					*/
static INLINE void
swap16_buf(uint16 *buf, uint len)
{	
	len = len/2;

	while(len--){
		*buf = swap16(*buf);
		buf++;
	}
}

#if BYTE_ORDER == LITTLE_ENDIAN
#define	hton16(i) swap16(i)
#define	hton32(i) swap32(i)
#define	ntoh16(i) swap16(i)
#define	ntoh32(i) swap32(i)
#define ltoh16(i) (i)
#define ltoh32(i) (i)
#define ltoh16_buf(buf, i) 
#else
#define	hton16(i) (i)
#define	hton32(i) (i)
#define	ntoh16(i) (i)
#define	ntoh32(i) (i)
#define	ltoh16(i) swap16(i)
#define	ltoh32(i) swap32(i)
#define ltoh16_buf(buf, i) swap16_buf((uint16*)buf, i)
#endif

#endif /* _BCMENDIAN_H_ */
