/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : Lib_Local.c    1.6
 *
 *  Module type              : IMPLEMENTATION 
 *
 *  Title                    : Basic definitions  
 *
 *  Last update              : 08:42:42 - 97/03/19
 *
 *  Description              : 
 *
 */

/*----------------------------includes----------------------------------------*/

#include "Lib_Local.h"


/*----------------------------functions---------------------------------------*/



extern Address
Lib_Mem_MALLOC_fun( Int size ) { return Lib_Mem_MALLOC(size); }

extern void
Lib_Mem_FREE_fun( Address address ) { Lib_Mem_FREE(address); }



UInt GCD( UInt a, UInt b ) 
{
     while (True) {
       a -= b * (a/b);  if (a==0) { return b; }
       b -= a * (b/a);  if (b==0) { return a; }
     }
}


UInt LCM( UInt a, UInt b ) 
{
   UInt gcd= GCD(a,b);

   return (a/gcd) * b;
}
