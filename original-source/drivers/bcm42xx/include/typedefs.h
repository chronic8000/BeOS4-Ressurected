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
 * Copyright 1997 Epigram, Inc.
 *
 * $Id: typedefs.h,v 1.26 2000/02/03 23:22:33 gmo Exp $
 *
 */

#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

/* Compiler dependent boolean type */
#ifdef __cplusplus

#ifndef FALSE
#define FALSE	false
#endif
#ifndef TRUE
#define TRUE	true
#endif

#else /* !__cplusplus */

#if defined(_WIN32)|| defined(__klsi__)

typedef	unsigned char	bool;

#elif !defined(__MWERKS__) && !defined(__BEOS__)

typedef	int	bool;

#endif

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1

#ifndef NULL
#define	NULL 0
#endif

#endif

#endif /* __cplusplus */

/* Compiler dependent types */

#ifdef __BEOS__
#include <SupportDefs.h>
#else
typedef unsigned char uchar;
#endif

#if defined(_WIN32)|| defined(_C6X_) || defined(__klsi__)

typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;

#else

/* pick up ushort & uint from standard types.h */
#ifdef linux
#include <linux/types.h>	/* sys/types.h and linux/types.h are oil and water */
#else
#include <sys/types.h>	
#if !defined(TARGETENV_sun4) && !defined(__BEOS__)
typedef unsigned long	ulong;
#endif /* TARGETENV_sun4 */
#endif
#if defined(TARGETOS_vxWorks)
typedef unsigned int	uint;
#endif

#endif /* WIN32 || _C6X_ */

#if defined(__klsi__)

typedef signed char	int8;
typedef signed short	int16;
typedef signed long	int32;

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;

#ifndef UCHAR
#define UCHAR	uint8
#endif

#ifndef ULONG
#define ULONG	uint32
#endif

typedef struct {
	int32	hi;
	int32	lo;
} int64;

typedef struct {
	uint32	hi;
	uint32	lo;
} uint64;

#else /* !klsi */

/* Compiler independent types */
#ifndef __BEOS__
typedef signed char	int8;
typedef signed short	int16;
typedef signed int	int32;

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
#endif

#if !defined(_C6X_)

/* no floating point or int64s on the C6x */
typedef float		float32;
typedef double		float64;

/*
 * abstracted floating point type allows for compile time selection of
 * single or double precision arithmetic.  Compiling with -DFLOAT32
 * selects single precision; the default is double precision.
 */

#if defined(FLOAT32)
typedef float32 float_t;
#else /* default to double precision floating point */
typedef float64 float_t;
#endif /* FLOAT32 */

#if defined(__MWERKS__)
/* Note: MetroWerks defines an old _MSC_VER, so the test for __MWERKS__
 * should come before the test for _MSC_VER. */
#if __option(longlong)
typedef signed long long int64;
typedef unsigned long long uint64;
#endif /* option longlong */

#elif defined(_MSC_VER) /* MicroSoft C */
typedef signed __int64	int64;
typedef unsigned __int64 uint64;

#elif defined(__GNUC__)&& !defined(__STRICT_ANSI__) && !defined(__BEOS__)
/* gcc understands signed/unsigned 64 bit types, but complains in ANSI mode */
typedef signed long long int64;
typedef unsigned long long uint64;

#elif defined(__ICL)&& !defined(__STDC__)
/* ICL accepts unsigned 64 bit type only, and complains in ANSI mode */
typedef unsigned long long uint64;

#endif /* __MWERKS__ */

#endif /* !defined(_C6X_) */

#endif /* klsi */

#define	PTRSZ	sizeof (char*)

/* compiler dependent return type qualifier INLINE */
#ifndef INLINE

#if defined(_MSC_VER)

#define INLINE __inline

#elif defined(__GNUC__)

#define INLINE __inline__

#elif defined(_TMS320C6X)

#define INLINE inline

#elif defined(__klsi__)

#define INLINE

#else

#error Define INLINE for your environment

#endif /* defined(_MSC_VER) */

#endif /* INLINE */

#endif /* _TYPEDEFS_H_ */
