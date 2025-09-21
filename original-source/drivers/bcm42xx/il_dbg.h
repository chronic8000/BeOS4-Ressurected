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
 * Minimal debug/trace/assert driver definitions for
 * Broadcom BCM42XX InsideLine(TM) Home Phoneline Networking Adapter.
 *
 * Copyright(c) 2000 Broadcom Corp.
 * $Id: il_dbg.h,v 1.30 2000/04/28 15:40:21 nn Exp $
 */

#ifndef _il_dbg_
#define _il_dbg_

/*
 * Just include the appropriate port-specific header file.
 *
 * Each port-specific header file is responsibible for defining:
 * - tunables
 * - printf() and sprintf()
 * - register access macros (R_REG, W_REG, AND_REG, OR_REG)
 * - bcopy(), bcmp(), and bzero()
 * - shmem access macros (R_SM, W_SM, BZERO_SM)
 */
#ifdef ILSIM /* software simulation */
#include <il_sim.h>
#elif NDIS
#include <il_ndis.h>
#elif linux
#include <il_linux.h>
#elif __FreeBSD__
#include <il_bsd.h>
#elif defined(__BEOS__)
#include "beos/il_beos.h"
#else
#error
#endif

#ifdef DBG
#define	IL_ERROR(args)	if (!(il_msg_level & 1)) ; else printf args
#define	IL_TRACE(args)	if (!(il_msg_level & 2)) ; else printf args
#define	IL_PRHDRS(i, s, h, p, n, l) \
	if (!(il_msg_level & 4)) ; else ilc_dumphdrs(i, s, h, p, n, l)
#define	IL_PRPKT(m, b, n)	if (!(il_msg_level & 8)) ; else ilc_prhex(m, b, n)
/* il_msg_level value 16 used to print rxhdr */
#define IL_CN_TRACE(args)	if (!(il_msg_level & 32)) ; else printf args
extern int il_msg_level;
extern void il_assert(char *exp, char *file, unsigned int line);
extern void ilc_prhex(char *msg, unsigned char *buf, unsigned int nbytes);
#undef ASSERT
#define ASSERT(exp)     if (exp) ; else il_assert(#exp, __FILE__, __LINE__)
#else	/* DBG */
#define	IL_ERROR(args)
#define	IL_TRACE(args)
#define	IL_PRHDRS(i, s, h, p, n, l)
#define	IL_PRPKT(m, b, n)
#define	IL_CN_TRACE(args)
#undef ASSERT
#define	ASSERT(exp)
#endif	/* DBG */

#endif /* _il_dbg_ */
