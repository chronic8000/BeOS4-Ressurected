/******************************************************************************
//
//	File:		token.h
//
//	Description:	token space class.
//			Implements token managment in the window server.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//	1/20/94		tdl	Added revNumber and portId token data members to support scripting.
//				Added new_token(long a_token, short type, void** data, long portId );
//				Added get_token(long a_token, void** data, long* portId );
//				Added set_token_port(long a_token, long portId );
//	5/22/92		bgs	new today
//
  *****************************************************************************/

#ifndef	_AS_TOKEN_H
#define _AS_TOKEN_H

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef ATOM_H
#include "atom.h"
#endif
#ifndef LOCK_H
#include "lock.h"
#endif


/*-------------------------------------------------------*/

typedef		long	s_token;
typedef		long	c_token;

#define		level_1_size	256
#define		level_2_size	128

#define		TOK_ITER_REMOVE	0x00000001
#define		TOK_ITER_STOP	0x00000002

#define		MAX_TOKEN_SPACE	((level_1_size * level_2_size) - 1)
#define		REV_NUMBER_MASK	(0x7FFF0000)	/* USES HIGH_ORDER s_token/c_token bits */
#define		REV_NUMBER_INC	(0x00010000)
#undef		NO_TOKEN
#define		NO_TOKEN		(0xFFFFFFFF)

#define		NO_USER		-1	/* if owner = NO_USER etc... 	*/
#define		ANY_USER	-2	/* Any user can use the token	*/
							/* Could be used for system data*/
							/* Etc...			*/
					
#define		NO_PORT		(-1)	/* No port assigned to token */

/*-------------------------------------------------------*/

typedef	struct {
	long	owner;
	short	type;
	long	size;
	void	*data;
	long	revNumber;
	long	portId;
} token_type;

/* one token takes 20 bytes */

/*-------------------------------------------------------*/

typedef	struct	{
	long		first_free;
	long		use_count;
	token_type	array[level_2_size];
} token_array;

/* one token array takes 2560 bytes */

/*-------------------------------------------------------*/

typedef uint32 (*token_iterator)(int32,int32,int16,void*,void *);

/*-------------------------------------------------------*/

class	TokenSpace {
public:
					TokenSpace();	
					~TokenSpace();	
		void		new_token_array(long level_1_index);
		long		new_token(long owner, short type, void *pointer);
		long		get_token(long a_token, short *type, void **data);
		long		get_token(long a_token, void **data);
		long		set_token_type(long a_token, short type);
		long		find_free_entry(long *level_1_index, long *level_2_index);
		void		adjust_free(long old_level_1_free, long old_level_2_free);
		void		remove_token(long a_token);
		void		full_search_adjust();
		void		dump_tokens();
		void		cleanup_dead(long target_owner);
		void		get_token_by_type(int *cookie, short type, long owner, void **data);	

		int32		grab_atom(int32 token, SAtom **atom);
		int32		delete_atom(int32 token, SAtom *atom);

		void		iterate_tokens(	int32 owner, short type,
									token_iterator tok, void *userData=NULL);

		long		first_free;
		Benaphore	tokenLock;
		CountedBenaphore	atomLock;
		token_array	*level_1[level_1_size];
};

/*-------------------------------------------------------*/

#define		TT_ANY			-2			/* matches any token type */
#define		TT_WINDOW		0x0001		/* window token type  */
#define		TT_VIEW			0x0002		/* view token type    */
#define		TT_FONT_CACHE	0x0004		/* font cache app setting   */
#define		TT_OFFSCREEN	0x0008
#define		TT_BITMAP		0x0010
#define		TT_PICTURE		0x0020
#define		TT_CURSOR		0x0040
#define		TT_LBX			0x0080
#define		TT_ATOM			0x0100

#endif
