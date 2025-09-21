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
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\ntasm.c_v  $											*/
/* 
/*    Rev 1.12   03 Jun 1998 14:37:28   BARBARA
/* ifdef NDIS_MINIPORT_DRIVER added.
/* 
/*    Rev 1.11   24 Mar 1998 09:24:06   BARBARA
/* Debug Color print added.
/* 
/*    Rev 1.10   10 Apr 1997 17:54:08   BARBARA
/* DebugLog changed to IF_LOG(DebugLog).
/* 
/*    Rev 1.9   15 Jan 1997 18:01:16   BARBARA
/* OutHexD, OutHexW added.
/* 
/*    Rev 1.8   11 Oct 1996 11:15:02   BARBARA
/* Unused variables were taken away.
/* 
/*    Rev 1.7   15 Jul 1996 14:40:52   TERRYL
/* 
/*    Rev 1.6   14 Jun 1996 16:50:42   JEAN
/* Removed the ^Z at the end of the file.
/* 
/*    Rev 1.5   28 May 1996 16:12:30   TERRYL
/* Added in OutCharAt(), OutHexAt() and WriteStatusLine() for win95
/* 
/*    Rev 1.4   12 May 1996 17:41:08   TERRYL
/* Added OutHex() and DelayUs() routines
/* 
/*    Rev 1.3   16 Apr 1996 14:34:22   TERRYL
/* Fixed a bug on OutChar so that the color code doesn't override the 
/* debugging character
/* 
/*    Rev 1.2   16 Apr 1996 11:29:50   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.1   30 Mar 1996 13:36:50   TERRYL
/* Added LLDDebugColor 
/* 
/*    Rev 1.0   04 Mar 1996 11:06:28   truong
/* Initial revision.
/*
/*   Rev 1.3   01 Feb 1996 17:09:08   truong
/*Changed calculation for NdisStallExecution assuming it needs the parameter
/*in microseconds.
/* 
/*    Rev 1.2   20 Oct 1995 18:38:30   BARBARA
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
#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#include "hw.h"
#endif

#include	"constant.h"
#include "keywords.h"
#include	"lld.h"
#include	"pstruct.h"
#include "hardware.h"
#include "port.h"
#include	"asm.h"
#include "lldpack.h"
#include	"lldinit.h"
#include	"lldisr.h"
#include "css.h"
#include "pcmcia.h"
#include "blld.h"
#include	"bstruct.h"
#include "lldqueue.h"

#ifdef NDIS_MINIPORT_DRIVER
#include "sw.h"
#endif


extern unsigned_8 LLDDebugColor; 		

#ifdef MULTI
#include "bstruct.h"
#include "pstruct.h"
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8					LLDDebugFlag;
#endif


#define	COLOR			1
#define  STATUS_LINE_ROW		1
#define  MASTER_NAME_COLUMN 	50
#define 	RESET_COUNT_COLUMN 	(MASTER_NAME_COLUMN + NAMELENGTH + 2)

#define	MinCursor	80 * 10 * 2
#define	MaxCursor   80 * 42 * 2						  

extern unsigned_8	LLDMSTASyncName [NAMELENGTH+1];
extern unsigned_32 LLDErrResetCnt;

unsigned_16		Cursor = MinCursor;
unsigned_16		ScreenColumn;
char 			   *VideoBase;



/***************************************************************************/
/*																									*/
/*	Delay (millitime)																			*/
/*																									*/
/*	INPUT:	millitime -	Number of milliseconds to delay. 						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine calls NDISStallExecution function to delay    */
/*              the required number of milliseconds.							   */
/*																									*/
/***************************************************************************/
void Delay (unsigned_16 Time)

{	
   //TTL - Believe NdisStallExecution to take us instead of ms as parameter
	NdisStallExecution ((UINT)Time * 1000);
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
   //TTL - Believe NdisStallExecution to take us instead of ms as parameter
	NdisStallExecution ((UINT)microtime);
}


/***************************************************************************/
/*																									*/
/*	WriteStatusLine()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the status line							*/
/*																									*/
/***************************************************************************/

void WriteStatusLine(void)
{
	int i;

	for (i = 0; i < NAMELENGTH + 1; i ++)
	{
		OutCharAt((unsigned_8)LLDMSTASyncName[i], YELLOW, MASTER_NAME_COLUMN + i);
	}		

	OutHexAt(
		(unsigned_8)(LLDErrResetCnt >> 0x00), RED, RESET_COUNT_COLUMN);
}

/***************************************************************************/
/*																									*/
/*	OutChar (Ch, ColorCode)																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the character to the screen			*/
/*																									*/
/***************************************************************************/

void OutChar (unsigned_8 Character, unsigned_8 ColorCode)

{
	/*
	 * If LLDDebugColor is SET, output the debug characters and colors onto 
	 * the screen. Otherwise output the debug characters onto a buffer.
	 */
	if(LLDDebugColor)
	{
		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), Character);
		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), ColorCode);

		if ((Cursor += 2) > MaxCursor)
			Cursor = MinCursor;

		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), '*');
		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), WHITE+BLINK);
	}
	
	else
	{
//		DebugLog(Character);
		IF_LOG( DebugLog(Character);)
	}
}

/***************************************************************************/
/*																									*/
/*	OutCharAt (Ch, ColorCode, Col)																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the character to the screen			*/
/*																									*/
/***************************************************************************/

void OutCharAt(unsigned_8 Character, unsigned_8 ColorCode, int Col)

{
	int Cursor;

	/*
	 * If LLDDebugColor is SET, output the debug characters and colors onto 
	 * the screen. Otherwise output the debug characters onto a buffer.
	 */
	if(LLDDebugColor)
	{
		Cursor = (STATUS_LINE_ROW * 80 + Col) * 2;
		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), Character);
		NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), ColorCode);
	}
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
/*		frequency.																				*/
/*																									*/
/***************************************************************************/

void Beep (unsigned_16 Frequency)

{ 
/*
	__asm		push	ax
	__asm		push	bx
	__asm		push	dx
	__asm		pushf
	__asm		cli
	__asm		mov	bx, Frequency
	__asm		mov	al, 0B6h
	__asm		out	043h, al
	__asm		xor	ax, ax
	__asm		cmp	bx, 20
	__asm		jb		Sound_On_1
	__asm		mov	dx, 012h
	__asm		mov	ax, 034DEh
	__asm		div	bx

	Sound_On_1:
	__asm		out	042h, al
	__asm		mov	al, ah
	__asm		push	ax
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		pop	ax
	__asm		out	042h, al
	__asm		in		al, 061h
	__asm		or		al, 3
	__asm		push	ax
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		pop	ax
	__asm		out	061h, al
	__asm		popf
	__asm		pop	dx
	__asm		pop	bx
	__asm		pop	ax

	Delay (5000);

	__asm		push 	ax
	__asm		pushf
	__asm		cli
	__asm		in	  	al, 061h
	__asm		and	al, 11111100b
	__asm		push	ax
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		in	al, 61h
	__asm		pop	ax
	__asm		out	061h, al
	__asm		popf
	__asm		pop	ax
*/
}



/***************************************************************************/
/*																									*/
/*	PreserveFlag		Preserv the flag													*/
/*																									*/
/*	ENTRY:			 																			*/
/*																									*/
/*	EXIT:					AX = Flag saved													*/
/*																									*/
/***************************************************************************/
FLAGS PreserveFlag (void)
{

	return (0);
}


/***************************************************************************/
/*																									*/
/*	RestoreFlag			Restore previously preserved flag                     */
/*																									*/
/*	ENTRY:				Previously preserved flag										*/
/*																									*/
/*	EXIT:				 																			*/
/*																									*/
/***************************************************************************/
void RestoreFlag (FLAGS OrigFlag)
{
}



/***************************************************************************/
/*																									*/
/*	EnableInterrupt	Enable the interrupt												*/
/*																									*/
/*	ENTRY:			 																			*/
/*																									*/
/*	EXIT:				 																			*/
/*																									*/
/***************************************************************************/
void EnableInterrupt (void)
{
}



/***************************************************************************/
/*																									*/
/*	DisableInterrupt	Disable the interrupt											*/
/*																									*/
/*	ENTRY:				NONE																	*/
/*																									*/
/*	EXIT:					AX = Flag saved													*/
/*																									*/
/***************************************************************************/
void DisableInterrupt (void)
{
}


/***************************************************************************/
/*																									*/
/*	OutHex (HexByte, ColorCode)  	 														*/
/*																									*/
/*	INPUT:		 																				*/
/*																									*/
/*	OUTPUT:		 Hex number of the input HexByte										*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHex (unsigned_8 HexByte, unsigned_8 ColorCode)
{
	unsigned_8 HexArray[2];
	unsigned_8 RemainValue;
	unsigned_8 i;

	for (i = 0; i < 2; i++)
	{

		RemainValue = HexByte;

		/* Get the remainder */
		RemainValue %= 16;

		/* Store the remainder */
		HexArray[1 - i] = 
			(RemainValue < 10) ? (RemainValue + '0') : (RemainValue - 10 + 'A');

		/* Get the quotient */
		HexByte /= 16;
	}
		
	if (LLDDebugColor)
	{
		for (i = 0; i < 2; i++)
		{
			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), HexArray[i]);
			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), ColorCode);

			if ((Cursor += 2) > MaxCursor)
				Cursor = MinCursor;

			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), '*');
			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), WHITE+BLINK);
		}
	}
	else
	{
		IF_LOG(DebugLog(HexArray[0]);)
		IF_LOG(DebugLog(HexArray[1]);)
	}
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
	OutHex ((unsigned_8)(HexWord >>  8), ColorCode);
	OutHex ((unsigned_8)(HexWord >>  0), ColorCode);

}

/***************************************************************************/
/*																									*/
/*	OutHexD (HexDouble, ColorCode)   														*/
/*																									*/
/*	INPUT:  The 32-bit hexadecimal number we want displayed.						*/
/*         The number's color.															*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHexD (unsigned_32 HexDouble, unsigned_8 ColorCode)

{
	OutHexW ((unsigned_16)(HexDouble >>  16), ColorCode);
	OutHexW ((unsigned_16)(HexDouble >>  0), ColorCode);

}

/***************************************************************************/
/*																									*/
/*	OutHexAt (HexByte, ColorCode, Col)  	 														*/
/*																									*/
/*	INPUT:		 																				*/
/*																									*/
/*	OUTPUT:		 Hex number of the input HexByte										*/
/*																									*/
/*	DESCRIPTION: This routine will print the Hex number to the screen			*/
/*																									*/
/***************************************************************************/

void OutHexAt (unsigned_8 HexByte, unsigned_8 ColorCode, int Col)
{
	unsigned_8 HexArray[2];
	unsigned_8 RemainValue;
	unsigned_8 i;
	int Cursor;

	for (i = 0; i < 2; i++)
	{

		RemainValue = HexByte;

		/* Get the remainder */
		RemainValue %= 16;

		/* Store the remainder */
		HexArray[1 - i] = 
			(RemainValue < 10) ? (RemainValue + '0') : (RemainValue - 10 + 'A');

		/* Get the quotient */
		HexByte /= 16;
	}
	
	Cursor = (STATUS_LINE_ROW * 80 + Col) * 2;
		
	if (LLDDebugColor)
	{
		for (i = 0; i < 2; i++)
		{
			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor), HexArray[i]);
			NdisWriteRegisterUchar((PUCHAR)(VideoBase+Cursor+1), ColorCode);
			Cursor += 2;
		}
	}
}
