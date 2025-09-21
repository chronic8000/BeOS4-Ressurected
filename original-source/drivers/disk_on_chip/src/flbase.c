/*
 * $Log:   V:/Flite/archives/FLite/src/FLBASE.C_V  $
 * 
 *    Rev 1.4   12 Mar 2000 16:40:10   dimitrys
 * Get rid of warnings (short -> unsigned short, long->
 *   unsigned long)
 * 
 *    Rev 1.3   Jan 13 2000 18:15:06   vadimk
 * TrueFFS OSAK 4.1
 * 
 *    Rev 1.2   06 Oct 1997 19:42:18   danig
 * Changed LEushort\long & Unaligned\4 to arrays
 * 
 *    Rev 1.1   10 Sep 1997 16:32:08   danig
 * Got rid of generic names
 * 
 *    Rev 1.0   28 Aug 1997 16:42:00   danig
 * Initial revision.
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-99              */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#include "flbase.h"

#if BIG_ENDIAN

/*----------------------------------------------------------------------*/
/*         Little / Big - Endian Conversion Routines			*/
/*----------------------------------------------------------------------*/

void toLEushort(unsigned char FAR0 *le, unsigned short n)
{
  le[1] = (unsigned char)(n >> 8);
  le[0] = (unsigned char)n;
}


unsigned short fromLEushort(unsigned char const FAR0 *le)
{
  return ((unsigned short)le[1] << 8) + le[0];
}


void toLEulong(unsigned char FAR0 *le, unsigned long n)
{
  le[3] = (unsigned char)(n >> 24);
  le[2] = (unsigned char)(n >> 16);
  le[1] = (unsigned char)(n >> 8);
  le[0] = (unsigned char)n;
}


unsigned long fromLEulong(unsigned char const FAR0 *le)
{
  return ((unsigned long)le[3] << 24) +
	 ((unsigned long)le[2] << 16) +
	 ((unsigned long)le[1] << 8) +
	 le[0];
}

extern void copyShort(unsigned char FAR0 *to, unsigned char const FAR0 *from)
{
  to[0] = from[0];
  to[1] = from[1];
}

extern void copyLong(unsigned char FAR0 *to, unsigned char const FAR0 *from)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
}


#else

void toUNAL(unsigned char FAR0 *unal, unsigned short n)
{
  unal[1] = (unsigned char)(n >> 8);
  unal[0] = (unsigned char)n;
}


unsigned short fromUNAL(unsigned char const FAR0 *unal)
{
  return ((unsigned short)unal[1] << 8) + unal[0];
}


void toUNALLONG(Unaligned FAR0 *unal, unsigned long n)
{
  toUNAL(unal[0],(unsigned short)n);
  toUNAL(unal[1],(unsigned short)(n >> 16));
}


unsigned long fromUNALLONG(Unaligned const FAR0 *unal)
{
  return fromUNAL(unal[0]) +
	 ((unsigned long)fromUNAL(unal[1]) << 16);
}

#endif /* BIG_ENDIAN */
