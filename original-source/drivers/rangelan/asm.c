/***************************************************************************/
/*																									*/
/*						  	PROXIM RANGELAN2 LOW LEVEL DRIVER							*/
/*																									*/
/*		PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the 		*/
/*		sole property of PROXIM, INC.  Reproduction or utilization of			*/
/*		this source in whole or in part is forbidden without the written		*/
/*		consent of PROXIM, INC.																*/
/*																									*/
/*																									*/
/*							(c) Copyright PROXIM, INC. 1994								*/
/*									All Rights Reserved										*/
/*																									*/
/***************************************************************************/

/***************************************************************************/
/*																									*/
/*	$Header: I:/mks/RL2Drivers/clld/rcs/ASM.C 1.1 1998/10/26 23:20:11 BARBARA Exp $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log: ASM.C $											*/
/*	Revision 1.1  1998/10/26 23:20:11  BARBARA											*/
/*	Initial revision											*/
/* 
/*    Rev 1.20   27 Sep 1996 09:03:38   JEAN
/* 
/*    Rev 1.19   26 Sep 1996 10:54:00   JEAN
/* Changed a parameter type for OutString().
/* 
/*    Rev 1.18   09 Sep 1996 16:28:30   JEAN
/* Fixed a null string comparison.
/* 
/*    Rev 1.17   23 Aug 1996 08:25:46   JEAN
/* Added three new functions:  OutString, OutHexW, and OutHexL.
/* 
/*    Rev 1.16   14 Jun 1996 16:15:26   JEAN
/* Comment changes.
/* 
/*    Rev 1.15   16 Apr 1996 11:29:34   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.14   06 Feb 1996 14:23:24   JEAN
/* Comment changes.
/* 
/*    Rev 1.13   31 Jan 1996 13:06:50   JEAN
/* Changed the ordering of some header include files and modified
/* some comments.
/* 
/*    Rev 1.12   19 Dec 1995 18:08:00   JEAN
/* Added a header include file.
/* 
/*    Rev 1.11   14 Dec 1995 15:15:50   JEAN
/* Added some header include files.
/* 
/*    Rev 1.10   17 Nov 1995 16:35:36   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   16 Oct 1995 16:05:28   JEAN
/* Made the in-line assembly code 8086/8088 compatible.
/* 
/*    Rev 1.8   12 Oct 1995 11:50:32   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.7   23 May 1995 09:42:48   tracie
/* 
/*    Rev 1.6   24 Apr 1995 15:49:48   tracie
/* 
/*    Rev 1.5   13 Apr 1995 08:48:26   tracie
/* XROMTEST version
/* 																								*/
/*    Rev 1.4   16 Mar 1995 16:15:26   tracie										*/
/* Added support for ODI																	*/
/* 																								*/
/*    Rev 1.3   29 Nov 1994 12:45:02   hilton										*/
/* 																								*/
/*    Rev 1.2   22 Sep 1994 10:56:16   hilton										*/
/* 																								*/
/*    Rev 1.1   20 Sep 1994 16:03:08   hilton										*/
/* No change.																					*/
/* 																								*/
/*    Rev 1.0   20 Sep 1994 11:00:26   hilton										*/
/* Initial revision.																			*/
/*																									*/
/*																									*/
/***************************************************************************/

#include <KernelExport.h>
#include	<stdio.h>
#include "constant.h"
#include "lld.h"
#include	"asm.h"

#ifdef MULTI
#include "bstruct.h"
#include "pstruct.h"
#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#endif
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8					LLDDebugFlag;
#endif


#define	COLOR			1
#define	MinCursor	80 * 27 * 2
#define	MaxCursor   80 * 48 * 2

unsigned_16		Cursor = MinCursor;
unsigned_16		ScreenColumn;



/***************************************************************************/
/*																									*/
/*	Delay (millitime)																			*/
/*																									*/
/*	INPUT:	millitime -	Number of milliseconds to delay. 						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine calls a BIOS function to delay the required	*/
/*              number of milliseconds.												*/
/*																									*/
/***************************************************************************/

void Delay (unsigned_16 millitime)
{
	snooze( ((bigtime_t)millitime) * 1000 );
	//spin( ((bigtime_t)millitime) * 1000 );
//	__asm		push ax
//	__asm		push cx
//	__asm		push dx
//
//	/*	Convert the milliseconds value into microseconds. */
//	__asm		mov  ax, millitime 
//	__asm		mov  cx, 1000
//	__asm		mul  cx
//
//	/*----------------------------------------------------*/
//	/* The time interval is a 32-bit microseconds value.	*/
//	/* Load the lower word of the time interval into DX 	*/
//	/* and the upper word into CX.								*/
//	/*----------------------------------------------------*/
//	__asm		mov  cx, dx
//	__asm		mov  dx, ax
//
//	/*----------------------------------------------*/
//	/* Call the BIOS function Wait Until Time			*/
//	/* Interval Has Elapsed.  This is function		*/
//	/* number 86.  BIOS functions are called			*/
//	/* using interrupt 15.  This function requires	*/
//	/* the carry to be 0.									*/
//	/*----------------------------------------------*/
//	__asm		mov  ah, 86h
//	__asm		clc
//	__asm		int  15h
//
//	__asm		pop  dx
//	__asm		pop  cx
//	__asm		pop  ax
}


/***************************************************************************/
/*																									*/
/*	Delay_uS (microtime)																		*/
/*																									*/
/*	INPUT:	microtime -	Number of microseconds to delay. 				  		*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine calls a BIOS function to delay the required	*/
/*              required number of microseconds.									*/
/*																									*/
/***************************************************************************/

void Delay_uS (unsigned_16 microtime)
{
	//spin( (bigtime_t)microtime );
	snooze( microtime );
//	__asm		push ax
//	__asm		push cx
//	__asm		push dx
//
//	/*----------------------------------------------------*/
//	/* The time interval is a 32-bit microseconds value.	*/
//	/* Load the lower word of the time interval into DX 	*/
//	/* and the upper word into CX.								*/
//	/*----------------------------------------------------*/
//	__asm		xor  cx, cx
//	__asm		mov  dx, microtime
//
//	/*----------------------------------------------*/
//	/* Call the BIOS function Wait Until Time			*/
//	/* Interval Has Elapsed.  This is function		*/
//	/* number 86.  BIOS functions are called			*/
//	/* using interrupt 15.  This function requires	*/
//	/* the carry to be 0.									*/
//	/*----------------------------------------------*/
//	__asm		mov  ah, 86h
//	__asm		clc
//	__asm		int  15h
//
//	__asm		pop  dx
//	__asm		pop  cx
//	__asm		pop  ax
}


/***************************************************************************/
/*																									*/
/*	OutString (*string, ColorCode)					 									*/
/*																									*/
/*	INPUT:  string - The null terminated string of characters we want			*/
/*                  displayed.															*/
/*         ColorCode - The character's color.										*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the character on the screen.			*/
/*																									*/
/***************************************************************************/

void OutString (char *string, unsigned_8 ColorCode)
{
//	dprintf( "%s(%d)", string, (uint16)ColorCode );
//	while (*string)
//	{
//		OutChar (*string, ColorCode);
//		*string++;
//	}
	
}

/***************************************************************************/
/*																									*/
/*	OutChar (Character, ColorCode)					 									*/
/*																									*/
/*	INPUT:  Character - The character we want displayed.							*/
/*         ColorCode - The character's color.										*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the character on the screen.			*/
/*																									*/
/***************************************************************************/

void OutChar (unsigned_8 Character, unsigned_8 ColorCode)

{
//	dprintf( "%c(%d)", Character, (uint16)ColorCode );
//	if (LLDDebugFlag == COLOR)
//	{	
//		__asm		pushf
//		__asm		push	es
//		__asm		push	di
//		__asm		push	ax
//		__asm		push	bx
//
//		__asm		cld
//
//		__asm		mov	ax, 0B800H
//		__asm		mov	es, ax
//
//		__asm		mov	di, Cursor
//		__asm		mov	al, Character
//		__asm		stosb
//		__asm		mov	bl, ColorCode
//		__asm		mov	es: byte ptr [di], bl
//		__asm		inc	di
//		__asm		cmp	di, MaxCursor
//		__asm		jb		DontDecrement
//		__asm		mov	di, MinCursor
//
//	DontDecrement:
//		__asm		mov	Cursor, di
//		__asm		mov	es: byte ptr [di], '*'
//		__asm		mov	es: byte ptr [di + 1], 0f4h
//
//		__asm		pop	bx
//		__asm		pop	ax
//		__asm		pop	di
//		__asm		pop	es
//		__asm		popf
//	}
}



/***************************************************************************/
/*																									*/
/*	Beep (Frequency)																			*/
/*																									*/
/*	INPUT:	Frequency	-	Frequency to beep at.									*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will turn the speaker on at the specified		*/
/*              frequency.		  															*/
/*																									*/
/***************************************************************************/

void Beep (unsigned_16 Frequency)

{
//	__asm		push	ax
//	__asm		push	bx
//	__asm		push	dx
//	__asm		pushf
//	__asm		cli
//	__asm		mov	bx, Frequency
//	__asm		mov	al, 0B6h
//	__asm		out	043h, al
//	__asm		xor	ax, ax
//	__asm		cmp	bx, 20
//	__asm		jb		Sound_On_1
//	__asm		mov	dx, 012h
//	__asm		mov	ax, 034DEh
//	__asm		div	bx
//
//	Sound_On_1:
//	__asm		out	042h, al
//	__asm		mov	al, ah
//	__asm		push	ax
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		pop	ax
//	__asm		out	042h, al
//	__asm		in		al, 061h
//	__asm		or		al, 3
//	__asm		push	ax
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		pop	ax
//	__asm		out	061h, al
//	__asm		popf
//	__asm		pop	dx
//	__asm		pop	bx
//	__asm		pop	ax
//
//	Delay (84);
//
//	__asm		push 	ax
//	__asm		pushf
//	__asm		cli
//	__asm		in	  	al, 061h
//	__asm		and	al, 11111100b
//	__asm		push	ax
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		in	al, 61h
//	__asm		pop	ax
//	__asm		out	061h, al
//	__asm		popf
//	__asm		pop	ax
}



/***************************************************************************/
/*																									*/
/*	PreserveFlag ()																			*/
/*																									*/
/*	INPUT:			 																			*/
/*																									*/
/*	OUTPUT:				Current flags														*/
/*																									*/
/* DESCRIPTION:		This routine returns the current processor flags.		*/
/*																									*/
/***************************************************************************/

FLAGS PreserveFlag (void)
{
	return 0;
	//return disable_interrupts();
//	FLAGS	oldflag;
//
//	__asm		pushf
//	__asm		pop	ax	
//	__asm		mov	oldflag, ax
//
//	return (oldflag);
}


/***************************************************************************/
/*																									*/
/*	RestoreFlag ()																				*/
/*																									*/
/*	INPUT:				Previously preserved flag										*/
/*																									*/
/*	OUTPUT:			 																			*/
/*																									*/
/* DESCRIPTION:		Restore the previously preserved processor flags.     */
/*																									*/
/***************************************************************************/

void RestoreFlag (FLAGS OrigFlag)
{
	//restore_interrupts( OrigFlag );
//	__asm		push	ax
//
//	__asm		mov	ax, OrigFlag
//	__asm		push	ax
//	__asm		popf		
//
//	__asm		pop	ax
}



/***************************************************************************/
/*																									*/
/*	EnableInterrupt ()																		*/
/*																									*/
/*	INPUT:			 																			*/
/*																									*/
/*	OUTPUT: 			 																			*/
/*																									*/
/*	DESCRIPTION: This function enables the processor's interrupts.				*/
/*																									*/
/***************************************************************************/

void EnableInterrupt (void)
{
	//__asm		sti
}



/***************************************************************************/
/*																									*/
/*	DisableInterrupt ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This function disables the processor's interrupts.			*/
/*																									*/
/***************************************************************************/

void DisableInterrupt (void)
{
	//__asm		cli 

}

/***************************************************************************/
/*																									*/
/*	OutHexL (HexLong, ColorCode)    														*/
/*																									*/
/*	INPUT:  The 32-bit hexadecimal number we want displayed.						*/
/*         The number's color.															*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHexL (unsigned_32 HexLong, unsigned_8 ColorCode)

{
//	dprintf( "%lX(%d)", HexLong, (uint16)ColorCode );
//	OutHex ((unsigned_8)(HexLong >> 24), ColorCode);
//	OutHex ((unsigned_8)(HexLong >> 16), ColorCode);
//	OutHex ((unsigned_8)(HexLong >>  8), ColorCode);
//	OutHex ((unsigned_8)(HexLong >>  0), ColorCode);
}

/***************************************************************************/
/*																									*/
/*	OutHexW (HexWord, ColorCode)   														*/
/*																									*/
/*	INPUT:  The 16-bit hexadecimal number we want displayed.						*/
/*         The number's color.															*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHexW (unsigned_16 HexWord, unsigned_8 ColorCode)

{
//	dprintf( "%hX(%d)", HexWord, (uint16)ColorCode );
//	OutHex ((unsigned_8)(HexWord >>  8), ColorCode);
//	OutHex ((unsigned_8)(HexWord >>  0), ColorCode);

}


/***************************************************************************/
/*																									*/
/*	OutHex (HexByte, ColorCode)  	 														*/
/*																									*/
/*	INPUT:  The 8-bit hexadecimal number we want displayed.		 				*/
/*         The number's color.															*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHex (unsigned_8 HexByte, unsigned_8 ColorCode)

{
//	dprintf( "%hX(%d)", (uint16)HexByte, (uint16)ColorCode );
	
//	unsigned_8	Abyte;
//
//	if (LLDDebugFlag == COLOR)
//	{	
//		__asm		push	ax
//
//		__asm		mov	al, HexByte
//		__asm		push	ax							
//
//		__asm		and	al, 011110000b	
//
//		__asm		shr	al, 1
//		__asm		shr	al, 1
//		__asm		shr	al, 1
//		__asm		shr	al, 1
//
//		__asm		cmp	al, 9
//		__asm		jbe	LLDISR_NP1							
//		__asm		add	al, 'A'-10		
//		__asm		jmp	LLDISR_NP1a
//
//	LLDISR_NP1:
//		__asm		add	al, 030h								
//
//	LLDISR_NP1a:
//		__asm		mov	Abyte, al
//
//		OutChar (Abyte, ColorCode);
//
//		__asm		pop	ax
//
//		__asm		and	al, 00001111b
//		__asm		cmp	al, 9
//		__asm		jbe	LLDISR_NP2
//		__asm		add	al, 'A'-10
//		__asm		jmp	LLDISR_NP2a
//
//	LLDISR_NP2:
//		__asm		add	al, 030h
//
//	LLDISR_NP2a:
//		__asm		mov	Abyte, al
//
//		OutChar (Abyte, ColorCode);
//
//		__asm		pop	ax
//	}

}

