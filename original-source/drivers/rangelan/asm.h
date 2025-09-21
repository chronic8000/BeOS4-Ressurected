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
/*	$Header:   J:\pvcs\archive\clld\asm.h_v   1.18   11 Aug 1998 11:46:56   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\asm.h_v  $											*/
/* 
/*    Rev 1.18   11 Aug 1998 11:46:56   BARBARA
/* include ndis.h eliminated.
/* 
/*    Rev 1.17   24 Mar 1998 09:18:02   BARBARA
/* Define for CopyMemory and SetMemory added.
/* 
/*    Rev 1.16   15 Jan 1997 17:24:02   BARBARA
/* OutHexD declaration was added.
/* 
/*    Rev 1.15   27 Sep 1996 09:17:20   JEAN
/* 
/*    Rev 1.14   26 Sep 1996 10:55:46   JEAN
/* Modified a function prototype.
/* 
/*    Rev 1.13   23 Aug 1996 08:26:18   JEAN
/* Added some new function prototypes.
/* 
/*    Rev 1.12   14 Jun 1996 16:41:22   JEAN
/* Added some function prototypes.
/* 
/*    Rev 1.11   28 May 1996 16:22:58   TERRYL
/* Added in OutCharAt(), OutHexAt(), and WriteStatusLine() prototypes.
/* 
/*    Rev 1.10   16 Apr 1996 11:40:48   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.9   29 Mar 1996 11:50:50   JEAN
/* Fixed some function prototype definitions.
/* 
/*    Rev 1.8   04 Mar 1996 15:57:48   truong
/* Added variable for debug color support under win95/nt
/* 
/*    Rev 1.7   06 Feb 1996 14:33:20   JEAN
/* Fixed a function prototype.
/* 
/*    Rev 1.6   31 Jan 1996 13:39:56   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   17 Nov 1995 16:43:44   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 12:05:14   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.3   16 Mar 1995 16:20:10   tracie
/* Added support for ODI
/* 
/*    Rev 1.2   22 Sep 1994 10:56:32   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:30   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:42   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "asm.c"							*/
/*																							*/
/*********************************************************************/

#ifndef ASM_H
#define ASM_H

#ifdef NDIS_MINIPORT_DRIVER
#define CopyMemory( Destination,Source,Length ) NdisMoveMemory( Destination,Source,(unsigned_32)Length )
#define SetMemory( Destination,Value,Length) NdisZeroMemory(Destination,Length)
#else
#define CopyMemory( Destination,Source,Length ) memcpy( Destination,Source,Length )
#define SetMemory( Destination,Value,Length) memset(Destination,Value,Length)
#endif


extern char *VideoBase;

void	Delay (unsigned_16 millitime);
void	Delay_uS (unsigned_16 microtime);
void	Beep (unsigned_16 Frequency);
void	OutChar (unsigned_8 Character, unsigned_8 ColorCode);
void  OutString (char *string, unsigned_8 ColorCode);
#ifdef NDIS_MINIPORT_DRIVER
void  OutCharAt(unsigned_8 Character, unsigned_8 ColorCode, int Col);
#endif
void	OutHex  (unsigned_8 HexByte, unsigned_8 ColorCode);
void  OutHexW (unsigned_16 HexWord, unsigned_8 ColorCode);
void  OutHexL (unsigned_32 HexLong, unsigned_8 ColorCode);
void  OutHexD (unsigned_32 HexDouble, unsigned_8 ColorCode);
#ifdef NDIS_MINIPORT_DRIVER
void  OutHexAt (unsigned_8 HexByte, unsigned_8 ColorCode, int Col);
void  WriteStatusLine(void);
#endif
FLAGS	PreserveFlag (void);
void	DisableInterrupt (void);
void	EnableInterrupt (void);
void	RestoreFlag (FLAGS OrigFlag);

#endif /* ASM_H */
