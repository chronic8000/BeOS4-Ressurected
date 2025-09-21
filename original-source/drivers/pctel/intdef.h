/****
*	Module	:	intdef.h
*
*	    Copyright (C) 1994-1998, PCtel Incorporation. All rights reserved.
*		Purpose         :
*		Author			: hcy
*		History (most recent first):
*				Dec	19	97	(EC) added INTDEF__H constant for multiple includes
*				03-27-94:	hcy created 
*
****/
	
		

/* ********************************************************************* */
/*  			ATTENTION TO ALL PCTEL'S CUSTOMERS  */
/*		DO NOT ATTEMPT TO MODIFY ANY OF THE FOLLOWING PARAMETERS */
/* ********************************************************************* */

#ifndef INTDEF__H
#define INTDEF__H

typedef	int		FlushGnuSymbolType1;
typedef	int		FlushGnuSymbolType2;


#ifdef __GNU__
#if 0
typedef char __int8;
typedef short __int16;
typedef long __int32;
#else
#define __int8 char
#define __int16 short
#define __int32 long
#endif

typedef struct { long msl; long lsl; }	__int64;
#endif

#define pcStatic		static
typedef	char			int8; 
typedef	short			int16;
typedef	long			int32;
typedef	unsigned char	USint8;
typedef	unsigned short	USint16;
typedef unsigned long	USint32;
typedef	unsigned long	Boolean;


typedef struct
{
	float	i,q;
}fPoint;
typedef struct
{
	int16	i,q;
}iPoint;   


#endif /* INTDEF__H */
