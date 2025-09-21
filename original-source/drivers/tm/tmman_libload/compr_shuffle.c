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
 *  Module name              : compr_shuffle.c    1.4
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : shuffle
 *
 *  Author                   : 
 *
 *  Last update              : 08:37:20 - 97/03/19
 *
 *  Reviewed                 : 
 *
 *  Description              : see .h file
 */

#include "compr_shuffle.h"


#define get_word(ucp) ((unsigned long)(ucp)[0] << 24) \
	| ((unsigned long)(ucp)[1] << 16) \
	| ((unsigned long)(ucp)[2] <<  8) \
	| ((unsigned long)(ucp)[3]) 


extern int 
SHFL_shuffle(unsigned char *unshuffled, unsigned char *shuffled, unsigned long length)
{
	unsigned long	i, j, nrof256bits;
	unsigned char	*s_p, *u_p, stmp1, stmp2, stmp3, stmp4;
	unsigned long	u1, u2, u3, u4, u5, u6, u7, u8;
	
#ifndef NDEBUG
	if (unshuffled != shuffled 
	 && ( &unshuffled[length-1] > shuffled 
	   && unshuffled < &shuffled[length-1]))
		return -1;
	if ((unsigned long)unshuffled % 4 != 0)
		return -1;
	if (length % 32 != 0)
		return -1;
#endif

	nrof256bits = length / 32;
	
	u_p = unshuffled;
	s_p = shuffled;
	for (i = 0; i < nrof256bits; i++ ) {

		u1 = get_word(u_p     );
		u2 = get_word(u_p +  4);
		u3 = get_word(u_p +  8);
		u4 = get_word(u_p + 12);
		u5 = get_word(u_p + 16);
		u6 = get_word(u_p + 20);
		u7 = get_word(u_p + 24);
		u8 = get_word(u_p + 28);
		

		for (j = 0; j < 8; j++ ) {
		
			stmp1  =  (u1 >> (j + 24)) & 1;
			stmp2  =  (u1 >> (j + 16)) & 1;
			stmp3  =  (u1 >> (j +  8)) & 1;
			stmp4  =  (u1 >> (j +  0)) & 1;
			
			stmp1 |= ((u2 >> (j + 24)) & 1) << 1;
			stmp2 |= ((u2 >> (j + 16)) & 1) << 1;
			stmp3 |= ((u2 >> (j +  8)) & 1) << 1;
			stmp4 |= ((u2 >> (j +  0)) & 1) << 1;
			
			stmp1 |= ((u3 >> (j + 24)) & 1) << 2;
			stmp2 |= ((u3 >> (j + 16)) & 1) << 2;
			stmp3 |= ((u3 >> (j +  8)) & 1) << 2;
			stmp4 |= ((u3 >> (j +  0)) & 1) << 2;
			
			stmp1 |= ((u4 >> (j + 24)) & 1) << 3;
			stmp2 |= ((u4 >> (j + 16)) & 1) << 3;
			stmp3 |= ((u4 >> (j +  8)) & 1) << 3;
			stmp4 |= ((u4 >> (j +  0)) & 1) << 3;
			
			stmp1 |= ((u5 >> (j + 24)) & 1) << 4;
			stmp2 |= ((u5 >> (j + 16)) & 1) << 4;
			stmp3 |= ((u5 >> (j +  8)) & 1) << 4;
			stmp4 |= ((u5 >> (j +  0)) & 1) << 4;
			
			stmp1 |= ((u6 >> (j + 24)) & 1) << 5;
			stmp2 |= ((u6 >> (j + 16)) & 1) << 5;
			stmp3 |= ((u6 >> (j +  8)) & 1) << 5;
			stmp4 |= ((u6 >> (j +  0)) & 1) << 5;
			
			stmp1 |= ((u7 >> (j + 24)) & 1) << 6;
			stmp2 |= ((u7 >> (j + 16)) & 1) << 6;
			stmp3 |= ((u7 >> (j +  8)) & 1) << 6;
			stmp4 |= ((u7 >> (j +  0)) & 1) << 6;
			
			stmp1 |= ((u8 >> (j + 24)) & 1) << 7;
			stmp2 |= ((u8 >> (j + 16)) & 1) << 7;
			stmp3 |= ((u8 >> (j +  8)) & 1) << 7;
			stmp4 |= ((u8 >> (j +  0)) & 1) << 7;
			
			*(s_p +  0) = stmp1;
			*(s_p +  8) = stmp2;
			*(s_p + 16) = stmp3;
			*(s_p + 24) = stmp4;
			s_p++;
		}
		u_p = &unshuffled[(i+1) * 32];
		s_p = &shuffled[(i+1) * 32];
	}
	return 0;
}


