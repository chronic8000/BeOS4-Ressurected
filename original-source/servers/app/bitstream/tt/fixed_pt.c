/*
 *	fixed_pt.c:	fixed point arithmetic functions
*/

/*
 *	Revision Control Information

 $Header: //bliss/user/archive/rcs/true_type/fixed_pt.c,v 14.0 97/03/17 17:53:12 shawn Release $
 $Log:	fixed_pt.c,v $
 * Revision 14.0  97/03/17  17:53:12  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.2  97/01/14  17:34:44  shawn
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 10.1  95/11/17  09:27:34  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 *
 * Revision 10.0  95/02/15  16:22:12  roberte
 * Release
 * 
 * Revision 9.1  95/01/04  16:29:11  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:48:08  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:25:53  roberte
 * Release
 * 
 * Revision 7.1  94/04/26  13:08:15  tamari
 * Removed USE_FLOAT code in Mul26Dot6() and F26Dot6() (with roberte's
 * blessing) because it sometimes returned a value that overflowed the
 * four byte limit of an F26Dot6.
 * 
 * Revision 7.0  94/04/08  11:56:39  roberte
 * Release
 * 
 * Revision 6.92  93/10/01  17:04:04  roberte
 * Added #if PROC_TRUETYPE conditional around whole file.
 * 
 * Revision 6.91  93/08/30  14:50:04  roberte
 * Release
 * 
 * Revision 6.47  93/06/24  17:38:48  roberte
 * Left #define USE_FLOAT as default option. It is faster and does not
 * mandate a floating point library.
 * Also removed some dead code.
 * 
 * Revision 6.46  93/06/15  14:04:32  roberte
 * Repaired USE_FLOAT logic.
 * 
 * Revision 6.45  93/05/18  09:35:25  roberte
 * Speedup work.  Added new fixed point math routines. Replaced some fixed math routines
 * with simple floating point operations. These actually execute slightly faster than the
 * laborious equivalents. The "faster" fixed routines really only
 * give better performance when a floating point chip is present, as odd as that seems.
 * 
 * Revision 6.44  93/03/15  13:07:00  roberte
 * Release
 * 
 * Revision 6.8  93/01/19  11:31:00  davidw
 * Removed outdated code
 * 80 column cleanup
 * fixed bug in FracDiv() function (return 0 when x=0, not NEGINFINITY)
 * 
 * Revision 6.7  93/01/18  14:25:56  davidw
 * Replaced FracDiv() fixed point module with floating point temp patch.
 * 
 * Revision 6.6  92/12/29  12:47:41  roberte
 * Now includes "spdo_prv.h" first.
 * 
 * Revision 6.5  92/11/24  13:37:36  laurar
 * include fino.h
 * 
 * Revision 6.4  92/07/28  11:54:25  davidw
 * cleaned up comments, removed debugging messages
 * 
 * Revision 6.3  92/07/28  11:51:53  davidw
 * fixed bugs in new FracDiv() routine
 * 
 * Revision 6.1  91/08/14  16:44:45  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:25:56  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:38:46  mark
 * added RCS control strings
 * 
*/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/true_type/fixed_pt.c,v 14.0 97/03/17 17:53:12 shawn Release $";
#endif

#include "spdo_prv.h"
#if PROC_TRUETYPE
#include "fino.h"
#include "fixed.h"


LOCAL_PROTO void CompMul(long src1, long src2, long *dst);
LOCAL_PROTO long CompDiv(long src1, long *src2);


/*  Here we define Fixed [16.16] and Fract [2.30] precision 
 *  multiplication and division functions and a Fract square root 
 *  function which are compatible with those in the Macintosh toolbox.
 *
 *  The division functions load the 32-bit numerator into the "middle"
 *  bits of a 64-bit numerator, then call the 64-bit by 32-bit CompDiv()
 *  function defined above, which can return a NEGINFINITY or POSINFINITY
 *  overflow return code.
 *
 *  The multiply functions call the 32-bit by 32-bit CompMul() function
 *  defined above which produces a 64-bit result, then they extract the
 *  "interesting" 32-bits from the middle of the 64-bit result and test 
 *  for overflow.
 *
 *  The GET32(a,i) macro defined below extracts a 32-bit value with "i" 
 *  bits of fractional precision from the 64-bit value in "a", a 2-element
 *  array of longs.
 *
 *  The CHKOVF(a,i,v) macro tests the most significant bits of the 
 *  64-bit value in "a", a 2-element array of longs, and tests the 
 *  32-bit result "v" for overflow.  "v" is defined as having "i" bits
 *  of fractional precision.
 *
 *  BIT() and OVFMASK() are "helper" macros used by GET32() and CHKOVF().
 *
 *  BIT(i) returns a mask with the "i"-th bit set.
 *  OVFMASK(i) returns a mask with the most-significant "32-i" bits set.
 */

#define BIT(i)          (1L<<(i))
#define OVFMASK(i)	( ~0L ^ ( ((unsigned long)BIT(i)) - 1 ) )
#define CHKOVF(a,i,v)   (\
        ( ((a)[0] & OVFMASK(i))==0)          ? ( (v)>=0 ?(v) :POSINFINITY) : \
        ( ((a)[0] & OVFMASK(i))==OVFMASK(i)) ? ( (v)<=0 ?(v) :NEGINFINITY) : \
        ( ((a)[0] & BIT(31))                 ? POSINFINITY   :NEGINFINITY)   \
	)
#define GET32(a,i) \
((((a)[0]<<(32-(i))) | ((unsigned long)((a)[1])>>(i))) + !!((a)[1] & BIT((i)-1)))

FUNCTION Fixed FixMul(Fixed fxA, Fixed fxB)
{
    long alCompProd[2];
    Fixed fxProd;

    if  (fxA == 0 || fxB == 0)
        return 0;

    CompMul (fxA, fxB, alCompProd);
    fxProd = GET32 (alCompProd,16);
    return CHKOVF(alCompProd,16,fxProd);
}

FUNCTION Fixed FixDiv (Fixed fxA, Fixed fxB)
{
    long alCompProd[2];
    
    alCompProd[0] = fxA >> 16;
    alCompProd[1] = fxA << 16;
    return CompDiv (fxB, alCompProd);
}

FUNCTION Fract FracMul (Fract frA, Fract frB)
{
    long alCompProd[2];
    Fract frProd;

    if  (frA == 0 || frB == 0)
        return 0;

    CompMul (frA,frB,alCompProd);
    frProd = GET32 (alCompProd,30);
    return CHKOVF(alCompProd,30,frProd);
}

FUNCTION Fract FracDiv (Fract frA, Fract frB)
{
    long alCompProd[2];

    alCompProd[0] = frA >> 2;
    alCompProd[1] = frA << 30;
    return CompDiv (frB, alCompProd);
}

/* 
   Fract FracSqrt (Fract xf)
   Input:  xf           2.30 fixed point value
   Return: sqrt(xf)     2.30 fixed point value
*/

FUNCTION Fract FracSqrt (Fract xf)
{
    Fract b = 0L;
    unsigned long c, d, x = xf;
    
    if (xf < 0) return (NEGINFINITY);

    /*
    The algorithm extracts one bit at a time, starting from the
    left, and accumulates the square root in b.  The algorithm 
    takes advantage of the fact that non-negative input values
    range from zero to just under two, and corresponding output
    ranges from zero to just under sqrt(2).  Input is assigned
    to temporary value x (unsigned) so we can use the sign bit
    for more precision.
    */
    
    if (x >= 0x40000000)
    {
        x -= 0x40000000; 
        b  = 0x40000000; 
    }

    /*
    This is the main loop.  If we had more precision, we could 
    do everything here, but the lines above perform the first
    iteration (to align the 2.30 radix properly in b, and to 
    preserve full precision in x without overflow), and afterward 
    we do two more iterations.
    */
    
    for (c = 0x10000000; c; c >>= 1)
    {
        d = b + c;
        if (x >= d)
        {
            x -= d; 
            b += (c<<1); 
        }
        x <<= 1;
    }

    /*
    Iteration to get last significant bit.
    
    This code has been reduced beyond recognition, but basically,
    at this point c == 1L>>1 (phantom bit on right).  We would
    like to shift x and d left 1 bit when we enter this iteration,
    instead of at the end.  That way we could get phantom bit in
    d back into the word.  Unfortunately, that may cause overflow
    in x.  The solution is to break d into b+c, subtract b from x,
    then shift x left, then subtract c<<1 (1L).
    */
    
    if (x > (unsigned long)b) /* if (x == b) then (x < d).  We want to test (x >= d). */
    {
        x -= b;
        x <<= 1;
        x -= 1L;
        b += 1L; /* b += (c<<1) */
    }
    else
    {
        x <<= 1;
    }

    /* 
    Final iteration is simple, since we don't have to maintain x.
    We just need to calculate the bit to the right of the least
    significant bit in b, and use the result to round our final answer.
    */
    
    return ( b + (x>(unsigned long)b) );
}

/************************************************************
 * long LongMulDiv(long a, long b, long c)
 *
 * Returns (a*b)/c
 *
 ************************************************************/

FUNCTION long LongMulDiv(long a, long b, long c)
{
        long temp[2];

        CompMul(a, b, temp);
        return CompDiv(c, temp);
}

/************************************************************
 * long ShortFracMul(long a, ShortFrac b)
 * 
 * Returns a * b, where a is a long int, and b is of type
 * ShortFrac, a 2.14 fixed-point number
 *
 ************************************************************/

FUNCTION long ShortFracMul(long a, ShortFrac b)
{
        int negative = FALSE;
        uint16 al, ah;
        uint32 lowlong, midlong, hilong;

        if (a < 0) { a = -a; negative = TRUE; }
        if (b < 0) { b = -b; negative ^= TRUE; }

        al = LOWORD(a); ah = HIWORD(a);

        midlong = USHORTMUL(ah, b);
        hilong = midlong & 0xFFFF0000;
        midlong <<= 16;
        midlong += 1 << 13;
        lowlong = USHORTMUL(al, b) + midlong;
        if (lowlong < midlong)
                hilong += ONEFIX;

        midlong = (lowlong >> 14) | (hilong << 2);
        return negative ? -(int32)midlong : midlong;
}

/************************************************************
 * int32 ShortMulDiv(int32 a, int16 b, int16 c)
 * 
 * Returns (a*b)/c
 *
 ************************************************************/

FUNCTION int32 ShortMulDiv(int32 a, int32 b, int32 c)
{
        return LongMulDiv((long)a, (long)b, (long)c);
}

/************************************************************
 * ShortFrac ShortFracDot(ShortFrac a, ShortFrac b)
 * 
 * Returns a * b, where a and b are 2.14 fixed-point numbers
 *
 ************************************************************/

FUNCTION ShortFrac ShortFracDot(ShortFrac a, ShortFrac b)
{
        return SHORTMUL(a,b) + (1 << 13) >> 14;
}

/************************************************************
 *
 * Internal functions
 *
 *   void CompMul(long src1, long src2, long dst[2])
 *   long CompDiv(long src1, long src2[2])
 *
 ************************************************************/

FUNCTION LOCAL_PROTO void CompMul(long src1, long src2, long *dst)
{
        int negative = (src1 ^ src2) < 0;
        register unsigned long dsthi, dstlo;

        if (src1 < 0)
                src1 = -src1;
        if (src2 < 0)
                src2 = -src2;
        {       unsigned short src1hi, src1lo;
                register unsigned short src2hi, src2lo;
                register unsigned long temp;
                src1hi = (unsigned short)(src1 >> 16);
                src1lo = (unsigned short)src1;
                src2hi = (unsigned short)(src2 >> 16);
                src2lo = (unsigned short)src2;
                temp = (unsigned long)src1hi * src2lo + (unsigned long)src1lo * src2hi;
                dsthi = (unsigned long)src1hi * src2hi + (temp >> 16);
                dstlo = (unsigned long)src1lo * src2lo;
                temp <<= 16;
                dsthi += (dstlo += temp) < temp;
                dst[0] = dsthi;
                dst[1] = dstlo;
        }
        if (negative)
                if (dstlo = (unsigned long)(-((long)dstlo)))
                        dsthi = ~dsthi;
                else
                        dsthi = (unsigned long)(-((long)dsthi));
        dst[0] = dsthi;
        dst[1] = dstlo;
}


/************************************************************/

FUNCTION LOCAL_PROTO long CompDiv(long src1, long *src2)
{
        register unsigned long src2hi = src2[0], src2lo = src2[1];
        int negative = (long)(src2hi ^ src1) < 0;

        if ((long)src2hi < 0)
                if (src2lo = (unsigned long)(-((long)src2lo)))
                        src2hi = ~src2hi;
                else
                        src2hi = (unsigned long)(-((long)src2hi));
        if (src1 < 0)
                src1 = -src1;
        {       register unsigned long src1hi, src1lo;
                unsigned long result = 0, place = 0x40000000;

                if ((src1hi = src1) & 1)
                        src1lo = 0x80000000;
                else
                        src1lo = 0;

                src1hi >>= 1;
                src2hi += (src2lo += src1hi) < src1hi;          /* round the result */

                if (src2hi > src1hi || src2hi == src1hi && src2lo >= src1lo)
                        if (negative)
                                return NEGINFINITY;
                        else
                                return POSINFINITY;
                while (place && src2hi)
                {       src1lo >>= 1;
                        if (src1hi & 1)
                                src1lo += 0x80000000;
                        src1hi >>= 1;
                        if (src1hi < src2hi)
                        {       src2hi -= src1hi;
                                src2hi -= src1lo > src2lo;
                                src2lo -= src1lo;
                                result += place;
                        }
                        else if (src1hi == src2hi && src1lo <= src2lo)
                        {       src2hi = 0;
                                src2lo -= src1lo;
                                result += place;
                        }
                        place >>= 1;
                }
                if (src2lo >= (unsigned long)src1)
                        result += src2lo/src1;
                if (negative)
                        return -(long)result;
                else
                        return result;
        }
}


/************************************************************
 * Total precision routine to multiply two 26.6 numbers <3>
 *
 *
 ************************************************************/

FUNCTION F26Dot6 Mul26Dot6(F26Dot6 a, F26Dot6 b)
{
        int negative = FALSE;
        uint16 al, bl, ah, bh;
        uint32 lowlong, midlong, hilong;

        if ((a <= FASTMUL26LIMIT) && (b <= FASTMUL26LIMIT) && (a >= -FASTMUL26LIMIT) && (b >= -FASTMUL26LIMIT))
                return a * b + (1 << 5) >> 6;                                           /* fast case */

        if (a < 0) { a = -a; negative = TRUE; }
        if (b < 0) { b = -b; negative ^= TRUE; }

        al = LOWORD(a); ah = HIWORD(a);
        bl = LOWORD(b); bh = HIWORD(b);

        midlong = USHORTMUL(al, bh) + USHORTMUL(ah, bl);
        hilong = USHORTMUL(ah, bh) + HIWORD(midlong);
        midlong <<= 16;
        midlong += 1 << 5;
        lowlong = USHORTMUL(al, bl) + midlong;
        hilong += lowlong < midlong;

        midlong = (lowlong >> 6) | (hilong << 26);
        return negative ? (F26Dot6)(-((int32)midlong)) : midlong;
}

/************************************************************
 * Total precision routine to divide two 26.6 numbers
 *
 *
 ************************************************************/

FUNCTION F26Dot6 Div26Dot6(F26Dot6 num, F26Dot6 den)
{
        int negative = FALSE;
        register uint32 hinum, lownum, hiden, lowden, result, place;

        if (den == 0) return (num < 0 ) ? NEGINFINITY : POSINFINITY;

        if ( (num <= FASTDIV26LIMIT) && (num >= -FASTDIV26LIMIT) )                      /* fast case */
                return (num << 6) / den;

        if (num < 0) { num = -num; negative = TRUE; }
        if (den < 0) { den = -den; negative ^= TRUE; }

        hinum = ((uint32)num >> 26);
        lownum = ((uint32)num << 6);
        hiden = den;
        lowden = 0;
        result = 0;
        place = HIBITSET;

        if (hinum >= hiden) return negative ? NEGINFINITY : POSINFINITY;

        while (place)
        {
                lowden >>= 1;
                if (hiden & 1) lowden += HIBITSET;
                hiden >>= 1;
                if (hiden < hinum)
                {
                        hinum -= hiden;
                        hinum -= lowden > lownum;
                        lownum -= lowden;
                        result += place;
                }
                else if (hiden == hinum && lowden <= lownum)
                {
                        hinum = 0;
                        lownum -= lowden;
                        result += place;
                }
                place >>= 1;
        }

        return negative ? (F26Dot6)(-((int32)result)) : result;
}


/* EOF: fixed_pt.c */

#endif /* PROC_TRUETYPE */
