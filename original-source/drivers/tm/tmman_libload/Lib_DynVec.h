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
 *  Module name              : Lib_DynVec.h    1.4
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : DynVec
 *
 *  Last update              : 08:42:37 - 97/03/19
 *
 *  Description              : Dynamic vectors
 *
 *	This module provides a simple vector package that is
 *	useful for maintaining small vectors of objects.
 *
 *	Elements may be pushed sequentially and retrieved either
 *	randomly by (0-based) index, or by popping the last element.
 *
 *	Lib_DynVec_count	return the population of a vector.
 *	Lib_DynVec_get		return the element at the index in the vector.
 *	Lib_DynVec_new		create a new DynVec.
 *	Lib_DynVec_pop		pop the last element from the vector and
 *				return it.
 *	Lib_DynVec_push		push an element onto the end of the vector.
 *	Lib_DynVec_set		set the value of the element at the index.
 *	Lib_DynVec_size		change the size of the vector as specified.
 */

#ifndef	_DynVec_h
#define	_DynVec_h

typedef struct _DynVec	DynVec;

int		Lib_DynVec_count(DynVec *vec);
void *		Lib_DynVec_get(DynVec *vec, int index);
DynVec *	Lib_DynVec_new(int initsz);
void *		Lib_DynVec_pop(DynVec *vec);
void		Lib_DynVec_push(DynVec *vec, void *elt);
void		Lib_DynVec_set(DynVec *vec, int index, void *elt);
void		Lib_DynVec_size(DynVec *vec, int size);

#endif
