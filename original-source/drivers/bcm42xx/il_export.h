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
 * Required functions exported by the port-specific (os-dependent) driver
 * to common (os-independent) driver code.
 *
 * Copyright(c) 1999 Broadcom, Inc.
 * $Id: il_export.h,v 13.6 2000/04/25 06:19:17 nn Exp $
 */

#ifndef _il_export_h_
#define _il_export_h_

/* misc callbacks */
extern int il_up(void *ilh);
extern int il_down(void *ilh, int reset);
extern void il_init(void *ilh);
extern void il_reset(void *ilh);
extern uint il_sendctl(void *ilh, void *p, int txpri, int txbpb);
extern void il_txintr(void *ilh);
extern void il_rxintr(void *ilh);
extern void il_txreclaim(void *ilh, bool forceall);
extern void il_rxreclaim(void *ilh);
extern void il_sendup(void *ilh, void *p, uint pri, uint pcrc);
extern uint il_ms(void);
extern void il_delay(uint us);
extern void il_link_up(void *ilh);
extern void il_link_down(void *ilh);
extern void il_dump(void *ilh, uchar *buf, uint len);
extern void *il_malloc(uint size);
extern void il_mfree(void *addr, uint size);

/* canonical packet primitives */
extern void *il_pget(void *ilh, bool send, uint len, uchar **va, uchar **pa);
extern void il_pfree(void *ilh, void *p, bool send);
extern uchar *il_ppush(void *ilh, void *p, uint bytes);
extern uchar *il_ppull(void *ilh, void *p, uint bytes);
extern uint il_plen(void *p);
extern void il_psetlen(void *p, uint len);
extern void* il_pdup(void *ilh, void *p);


#endif	/* _il_export_h_ */
