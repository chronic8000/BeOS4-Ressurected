


/*****************************************************************************
*                                                                            *
*  Copyright 1991-1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                         U.S. Patent No 4,785,391                           *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/


/**************************** S P M _ F N S . C ******************************
 *                                                                           *
 * This module contains the new functions required to support mutability     *
 * in Speedo-M fonts                                                         *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1)  1 May 91  jsc  Created                                               *
 *                                                                           *
 ****************************************************************************/
/********************* Revision Control Information **********************************
*                                                                         
*     $Header: //bliss/user/archive/rcs/speedo/spm_fns.c,v 33.0 97/03/17 17:48:22 shawn Release $                                                                       *
*                                                                                    
*     $Log:	spm_fns.c,v $
 * Revision 33.0  97/03/17  17:48:22  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.1  96/07/02  12:04:27  shawn
 * Changed NULL to BTSNULL.
 * 
 * Revision 32.0  95/02/15  16:40:10  shawn
 * Release
 * 
 * Revision 31.2  95/01/12  11:32:23  roberte
 * Updated Copyright header.
 * 
 ************************************************************************************/


#include "spdo_prv.h"           /* General definitions for Speedo   */

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#if PROC_SPEEDO



#if INCL_MUTABLE

FUNCTION void sp_set_int_specs(SPGLOBPTR2 fix31 coeffs[])
/*
 * Called by the host software to set the interpolation coefficients 
 * for a Speedo-M font.
 * The array coeffs must contain the same number of coefficients
 * as there are design axes in the currently selected font.
 * Each coefficient represents a signed fixed-point number in 16.16 format
 * coeff[0] corresponds to the first design axis.
 * Generates an adjustment vector coefficient for each term in the font
 * interpolation term mask.
 * Also initializes a pointer to first constraint table adjustment vector.
 */
{
fix15   ii, jj;
ufix8   tmpufix8;
fix15   tmpfix15;
ufix16  pub_hdr_size;           /* Public header size (bytes) */
ufix16  pvt_hdr_size;           /* Private header size (bytes) */
ufix8   nDesignAxes;            /* Number of design axes */
fix31   offset;                 /* Offset to start of interpolation term mask data */
ufix8  *p1;                     /* Pointer to interpolation term mask data */
fix15   n1;                     /* Number of interpolation terms */
fix15   mult;                   /* Adjustment vector coeff accumulator */

if (!sp_globals.specs_valid)            /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);                   /* Report font not specified */
    return;
    }

pub_hdr_size = sp_globals.processor.speedo.hdr2_org - sp_globals.processor.speedo.font_org;
pvt_hdr_size = 
    (pub_hdr_size >= (FH_PVHSZ + 2))?
    sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_PVHSZ):
    FH_NBYTE + 3;
if ((pub_hdr_size < FH_NDSNA + 1) ||    /* Not a Speedo-M font? */
    (pvt_hdr_size < FH_OFITM + 3))      /* No interpolation term mask? */
    {
    sp_globals.processor.speedo.nIntTerms = 0;           /* Inhibit adjustment vector execution */
    return;
    }

nDesignAxes = *(sp_globals.processor.speedo.font_org + FH_NDSNA);
if (nDesignAxes == 0)                   /* Not a Speedo-M font? */
    {
    sp_globals.processor.speedo.nIntTerms = 0;           /* Inhibit adjustment vector execution */
    return;
    }

offset = sp_read_long(PARAMS2 sp_globals.processor.speedo.font_org + pub_hdr_size + FH_OFITM);
p1 = sp_globals.processor.speedo.font_org + offset;      /* Point to start of interpolation term mask */
n1 = NEXT_BYTE(p1);                     /* Number of active interpolation terms */
sp_globals.processor.speedo.nIntTerms = n1;
for (ii = 0; ii < n1; ii++)             /* For each interpolation term... */
    {
    tmpufix8 = NEXT_BYTE(p1);           /* Read interpolation term mask */
    mult = COEFF_ONE;
    for (jj = 0; jj < 8; jj++)          /* For each possible coefficient... */
        {
        if (tmpufix8 & BIT0)            /* Coefficient active in term? */
            {
            tmpfix15 = ((coeffs[jj] + (COEFF_DIV >> 1)) / COEFF_DIV) - COEFF_RND;
            mult = (((fix31)mult * (fix31)tmpfix15) + COEFF_RND) >> COEFF_SHIFT;
            }
        tmpufix8 >>= 1;
        }
    sp_globals.processor.speedo.c[ii] = mult;            /* Adjustment vector coefficient */
    }

if (pvt_hdr_size < FH_OFCAD + 3)        /* No offset to constr adjustment data in font? */
    {
    sp_globals.processor.speedo.constr.adj_org = BTSNULL;   /* Inhibit constr adjustment vector exection */
    }
else                                    /* Font contains offset to constr adjustment data? */
    {
    offset = sp_read_long(PARAMS2 sp_globals.processor.speedo.font_org + pub_hdr_size + FH_OFCAD);
    sp_globals.processor.speedo.constr.adj_org = sp_globals.processor.speedo.font_org + offset;
    sp_globals.processor.speedo.constr.data_valid = FALSE; /* Flag possible change to constraint table */
    }
}
#endif


#if INCL_MUTABLE

FUNCTION void sp_get_int_range_name(SPGLOBPTR2 fix15 axis, fix15 coeff, fix15 n, ufix8 *s)
/*
 * Called by the host software to access the name of the interpolation
 * range for the specified design axis in the currently selected font
 * Up to n - 1 characters of the name are copied to the specified string
 * which is then null-terminated. 
 */
{
fix15   ii, jj;
ufix16  pub_hdr_size;           /* Public header size (bytes) */
ufix16  pvt_hdr_size;           /* Private header size (bytes) */
ufix8   nDesignAxes;            /* Number of design axes */
fix31   offset;                 /* Offset to start of interpolation term mask data */
ufix8  *p0;
ufix8  *p1;
ufix8  *p2;
fix31   upper_limit;
fix15   str_size;

if (!sp_globals.specs_valid)            /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);                   /* Report font not specified */
    return;
    }

if (n < 1)                              /* No space for string? */
    {
    return;
    }

pub_hdr_size = sp_globals.processor.speedo.hdr2_org - sp_globals.processor.speedo.font_org;
pvt_hdr_size = 
    (pub_hdr_size >= (FH_PVHSZ + 2))?
    sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_PVHSZ):
    FH_NBYTE + 3;
if ((pub_hdr_size < FH_NDSNA + 1) ||    /* Not a Speedo-M font? */
    (pvt_hdr_size < FH_OFIRN + 3))      /* No interpolation range names? */
    {
    goto done;                          /* Return null string */
    }

nDesignAxes = *(sp_globals.processor.speedo.font_org + FH_NDSNA);
if ((nDesignAxes == 0) ||               /* Not a Speedo-M font? */
    (axis < 1) ||
    (axis > nDesignAxes))               /* Non-existent axis selected? */
    {
    goto done;                          /* Return null string */
    }

if ((coeff < 0) ||
    (coeff > (fix31)100 << 16))         /* Interpolation coefficient out of range? */
    {
    goto done;                          /* Return null string */
    }

offset = sp_read_long(PARAMS2 sp_globals.processor.speedo.font_org + pub_hdr_size + FH_OFIRN);
p0 = sp_globals.processor.speedo.font_org + offset;      /* Start of interpolation range names data */
p1 = p0 + ((axis - 1) << 1);            /* Index to range names for req'd axis */
p2 = p0 + NEXT_WORD(p1);                /* Start of req'd range name chain */
do
    {
    upper_limit = (fix31)((ufix8)NEXT_BYTE(p2)) << 16;
    str_size = (fix15)((ufix8)NEXT_BYTE(p2));
    if (coeff <= upper_limit)
        {
        if (str_size > (n - 1))         /* String exceeds maximum length? */
            str_size = n - 1;           /* Truncate it */
        for (jj = 0; jj < str_size; jj++)
            {
            s[jj] = NEXT_BYTE(p2);
            }
        s[jj] = '\0';                   /* Terminate string */
        return;
        }
    else
        {
        p2 += str_size;
        }
    }
while(upper_limit < 100);
done:
s[0] = '\0';
}
#endif


#if INCL_MUTABLE

FUNCTION fix15 sp_get_adjustment(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp2)
/*
 * Returns the total adjustment due to interpolation
 * The adjustment data is read from the font pointer *pp2.
 * Adjustment is zero if *pp2 = NULL
 * The pointer is incremented ready for the next adjustment.
 */
{
fix15   ii;
fix15   adj;                    /* Adjustment for each adjustment vector element */
fix31   total;                  /* Total adjustment */

if (*pp2 == BTSNULL)                       /* Interpolation inactive? */
    return (fix15)0;

total = 0;
for (ii = 0; ii < sp_globals.processor.speedo.nIntTerms; ii++)  /* For each interpolation term... */
    {
    adj = (fix15)((ufix8)NEXT_BYTE(*pp2));
    if (adj < 8)                        /* One to eight zeros? */
        {
        ii += adj;                      /* Skip 0 - 7 interpolation coefficients */
        continue;
        }
    else if (adj < 247)                 /* Single-byte format? */
        {
        adj -= 127;                     /* Map to range -119 to +119 (inclusive) */
        }
    else if (adj > 247)                 /* Two-byte format? */
        {
        adj = (((adj & 0x07) << 8) + NEXT_BYTE(*pp2)) - 1024;
        }
    else                                /* Three-byte format? */
        {
        adj = NEXT_WORD(*pp2);          /* Read 2-byte value */
        }
    total += adj * sp_globals.processor.speedo.c[ii];    /* Accumulate sum of products */
    }

return (fix15)((total + COEFF_RND) >> COEFF_SHIFT); /* Return result */
}
#endif
#endif /* if PROC_SPEEDO */
