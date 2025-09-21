/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                         U.S. Patent No 4,785,391                           *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/

/*
 *
 *  $Header: //bliss/user/archive/rcs/speedo/set_keys.c,v 33.0 97/03/17 17:47:37 shawn Release $
 *
 *  $Log:	set_keys.c,v $
 * Revision 33.0  97/03/17  17:47:37  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.1  96/08/20  13:12:13  shawn
 * Added RCS/Header and Log keywords.
 * 
 *
 */


#include "speedo.h"
#include "fino.h"

LOCAL_PROTO ufix8 next_key(ufix16 *pkey);

#if PROC_SPEEDO

FUNCTION void set_keys(ufix16 cust_no, ufix16 init_key, ufix8 *keys)
/* 
 * Sets encryption keys for font generation.
 * Note that key generation may only depend on the first 240 bytes of the generated font
 */
{
ufix16 key;

if (cust_no)                /* Customer number non-zero? */
    {
    key = init_key;         /* Use standard decryption key */

    keys[1] = next_key(&key);
    keys[2] = next_key(&key);
    keys[3] = next_key(&key) & 0x03;
    keys[4] = next_key(&key);
    keys[5] = next_key(&key);
    keys[6] = next_key(&key);
    keys[7] = next_key(&key);
    keys[8] = next_key(&key);
    }
else                        /* Zero customer number - no encryption */
    {
    keys[1] = keys[2] = keys[3] = keys[4] = keys[5] = keys[6] = keys[7] = keys[8] = 0; 
    }
}



FUNCTION ufix8 next_key(ufix16 *pkey)
/*
 * Scrambles *pkey in a pseudo-random way.
 * Returns a ufix8 key value extracted from *pkey.
 */
{
ufix32 key;
fix15  i;

key = *pkey;
key = key * 78231 + 23805;
*pkey = key;
return (ufix8)((key >> 3) & 0xff);
}

#endif /* if PROC_SPEEDO */
