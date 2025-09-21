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
 * Driver-specific socket ioctls
 * used by BSD, Linux, and PSOS
 * BCM42XX PCI iLine10(TM) device drivers.
 *
 * Copyright 1999, Broadcom Corporation
 * All Rights Reserved.
 *
 * $Id: ilsockio.h,v 13.9 2000/03/08 05:09:50 abagchi Exp $
 */

#ifndef _ilsockio_h_
#define _ilsockio_h_

#define HWRXOFF		30

/* THESE MUST BE CONTIGUOUS AND CONSISTENT WITH VALUES IN ILC.H */


#if defined(linux)
#define	SIOCSILTXDOWN		(SIOCDEVPRIVATE + 0)
#define	SIOCSILLOOP		(SIOCDEVPRIVATE + 1)
#define	SIOCGILDUMP		(SIOCDEVPRIVATE + 2)
#define	SIOCGILDUMPSCB		(SIOCDEVPRIVATE + 3)
#define	SIOCSILTXBPB		(SIOCDEVPRIVATE + 4)
#define	SIOCSILTXPRI		(SIOCDEVPRIVATE + 5)
#define	SIOCSILSETMSGLEVEL	(SIOCDEVPRIVATE + 6)
#define	SIOCSILLINKINT		(SIOCDEVPRIVATE + 7)
#define	SIOCSILHPNAMODE		(SIOCDEVPRIVATE + 8)
#define	SIOCSILCSA		(SIOCDEVPRIVATE + 9)
#define	SIOCSILCSAHPNAMODE	(SIOCDEVPRIVATE + 10)
#define	SIOCGILLARQDUMP		(SIOCDEVPRIVATE + 11)
#define	SIOCSILLARQONOFF	(SIOCDEVPRIVATE + 12)
#define	SIOCSILPROMISCTYPE	(SIOCDEVPRIVATE + 13)
#define	SIOCGILDUMPPES		(SIOCDEVPRIVATE + 14)
#define	SIOCSILPESSET		(SIOCDEVPRIVATE + 15)

#elif defined(__BEOS__)
#include <Drivers.h>
#include <sys/sockio.h>

#define SIOCDEVPRIVATE		(B_DEVICE_OP_CODES_END + 100)
#define	SIOCSILTXDOWN		(SIOCDEVPRIVATE + 0)
#define	SIOCSILLOOP		(SIOCDEVPRIVATE + 1)
#define	SIOCGILDUMP		(SIOCDEVPRIVATE + 2)
#define	SIOCGILDUMPSCB		(SIOCDEVPRIVATE + 3)
#define	SIOCSILTXBPB		(SIOCDEVPRIVATE + 4)
#define	SIOCSILTXPRI		(SIOCDEVPRIVATE + 5)
#define	SIOCSILSETMSGLEVEL	(SIOCDEVPRIVATE + 6)
#define	SIOCSILLINKINT		(SIOCDEVPRIVATE + 7)
#define	SIOCSILHPNAMODE		(SIOCDEVPRIVATE + 8)
#define	SIOCSILCSA		(SIOCDEVPRIVATE + 9)
#define	SIOCSILCSAHPNAMODE	(SIOCDEVPRIVATE + 10)
#define	SIOCGILLARQDUMP		(SIOCDEVPRIVATE + 11)
#define	SIOCSILLARQONOFF	(SIOCDEVPRIVATE + 12)
#define	SIOCSILPROMISCTYPE	(SIOCDEVPRIVATE + 13)
#define	SIOCGILDUMPPES		(SIOCDEVPRIVATE + 14)
#define	SIOCSILPESSET		(SIOCDEVPRIVATE + 15)

#else	/* !linux */

#define	SIOCSILTXDOWN		_IOWR('i', 130, struct ifreq)
#define	SIOCSILLOOP		_IOWR('i', 131, struct ifreq)
#define	SIOCGILDUMP		_IOWR('i', 132, struct ifreq)
#define	SIOCGILDUMPSCB		_IOWR('i', 133, struct ifreq)
#define	SIOCSILTXBPB		_IOWR('i', 134, struct ifreq)
#define	SIOCSILTXPRI		_IOWR('i', 135, struct ifreq)
#define	SIOCSILSETMSGLEVEL	_IOWR('i', 136, struct ifreq)
#define	SIOCSILLINKINT		_IOWR('i', 137, struct ifreq)
#define	SIOCSILHPNAMODE		_IOWR('i', 138, struct ifreq)
#define	SIOCSILCSA		_IOWR('i', 139, struct ifreq)
#define	SIOCSILCSAHPNAMODE	_IOWR('i', 140, struct ifreq)
#define	SIOCGILLARQDUMP		_IOWR('i', 141, struct ifreq)
#define	SIOCSILLARQONOFF	_IOWR('i', 142, struct ifreq)
#define	SIOCSILPROMISCTYPE	_IOWR('i', 143, struct ifreq)
#define	SIOCGILDUMPPES		_IOWR('i', 144, struct ifreq)
#define	SIOCSILPESSET		_IOWR('i', 145, struct ifreq)

/* non-contiguous freebsd-specific ones */
#define	SIOCSILBPFEXT		_IOWR('i', 160, struct ifreq)

#endif

#endif
