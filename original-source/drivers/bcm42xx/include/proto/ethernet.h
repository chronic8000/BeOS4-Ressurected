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
 * $Id: ethernet.h,v 9.9 1999/12/16 03:54:06 nn Exp $
 * From FreeBSD 2.2.7: Fundamental constants relating to ethernet.
 ******************************************************************************/

#ifndef _NET_ETHERNET_H_ /* use native BSD ethernet.h when available */
#define _NET_ETHERNET_H_

#include "typedefs.h"

#if defined(__arm)&& defined(__GNUC__)
#define	PACKED	__attribute__((packed))
#else
#define	PACKED
#endif

/*
 * The number of bytes in an ethernet (MAC) address.
 */
#define	ETHER_ADDR_LEN		6

/*
 * The number of bytes in the type field.
 */
#define	ETHER_TYPE_LEN		2

/*
 * The number of bytes in the trailing CRC field.
 */
#define	ETHER_CRC_LEN		4

/*
 * The length of the combined header.
 */
#define	ETHER_HDR_LEN		(ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)

/*
 * The minimum packet length.
 */
#define	ETHER_MIN_LEN		64

/*
 * The maximum packet length.
 */
#define	ETHER_MAX_LEN		1518

/*
 * A macro to validate a length with
 */
#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)


#ifndef __INCif_etherh /* Quick and ugly hack for VxWorks */
/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	uint8	ether_dhost[ETHER_ADDR_LEN];
	uint8	ether_shost[ETHER_ADDR_LEN];
	uint16	ether_type;
} PACKED ;
#endif

/*
 * Structure of a 48-bit Ethernet address.
 */
struct	ether_addr {
	uint8 octet[ETHER_ADDR_LEN];
} PACKED ;

/*
 * Takes a pointer, returns true if a 48-bit multicast address
 * (including broadcast, since it is all ones)
 */
#define ETHER_ISMULTI(ea) (((uint8 *)(ea))[0] & 1)

/*
 * Takes a pointer, returns true if a 48-bit broadcast (all ones)
 */
#define ETHER_ISBCAST(ea) ((((uint8 *)(ea))[0] &		\
			    ((uint8 *)(ea))[1] &		\
			    ((uint8 *)(ea))[2] &		\
			    ((uint8 *)(ea))[3] &		\
			    ((uint8 *)(ea))[4] &		\
			    ((uint8 *)(ea))[5]) == 0xff)

static const struct ether_addr ether_bcast = {{255, 255, 255, 255, 255, 255}};

/*
 * Takes a pointer, returns true if a 48-bit null address (all zeros)
 */
#define ETHER_ISNULLADDR(ea) ((((uint8 *)(ea))[0] |		\
			    ((uint8 *)(ea))[1] |		\
			    ((uint8 *)(ea))[2] |		\
			    ((uint8 *)(ea))[3] |		\
			    ((uint8 *)(ea))[4] |		\
			    ((uint8 *)(ea))[5]) == 0)

#undef PACKED

#endif /* _NET_ETHERNET_H_ */
