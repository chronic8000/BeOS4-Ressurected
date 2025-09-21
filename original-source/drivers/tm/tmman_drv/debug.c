/*---------------------------------------------------------------------------- 
COPYRIGHT (c) 1998 by Philips Semiconductors

 THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED AND COPIED IN 
 ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH A LICENSE AND WITH THE 
 INCLUSION OF THE THIS COPY RIGHT NOTICE. THIS SOFTWARE OR ANY OTHER COPIES 
 OF THIS SOFTWARE MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER
 PERSON. THE OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED. 
 
  THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT ANY PRIOR NOTICE
  AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY Philips Semiconductor. 
  
   PHILIPS ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF THIS SOFTWARE
   ON PLATFORMS OTHER THAN THE ONE ON WHICH THIS SOFTWARE IS FURNISHED.
----------------------------------------------------------------------------*/
/*
HISTORY
960925	Tilakraj Roy  Organized and added all three types of debugging.
980604  Volker Schildwach  Ported to Windows CE
981021	Tilakraj Roy  Changes for integrating into common source base
990511  DTO Ported to pSOS
001108  Ported to Linux
*/

/*----------------------------------------------------------------------------
SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>

/*----------------------------------------------------------------------------
DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/

#include "tmmanlib.h"
#include "tm_platform.h"
#include "mmio.h"


void	debugOutput (  UInt8* pString );
void	debugTrace ( UInt8* szString );
void	debugProcessChar ( UInt8* pszBuffer, UInt32*pdwPosition, UInt8 Value  );
void	debugProcessDec ( UInt8* pszBuffer, UInt32* pdwPosition, UInt32 Value  );
void	debugProcessHex ( UInt8* pszBuffer, UInt32* pdwPosition, UInt32 Value  );
void	debugProcessStr ( UInt8* pszBuffer, UInt32* pdwPosition, UInt8* Value  );


Bool	debugInit ( debugParameters* Parameters  )
{
	DebugObject*	Debug = &TMManGlobal->Debug;
	
	Debug->DBGLevelBitmap = ((debugParameters*)Parameters)->LevelBitmap;
	Debug->DBGType        = ((debugParameters*)Parameters)->Type;
	Debug->TraceLength    = ((debugParameters*)Parameters)->TraceBufferSize ;
	Debug->TraceBuffer    = Debug->TraceBufferData;
	Debug->TracePosition  = 0;
	Debug->TraceWrapped   = False;
	return statusSuccess;
}



Bool	debugExit ( void )
{
	return statusSuccess;
}

Bool	debugCheckLevel ( UInt32 Level )
{
	DebugObject*	Debug = &TMManGlobal->Debug;
	if( (( Level) & Debug->DBGLevelBitmap)  )
	{
		return True;
	}
	return False;
}

UInt32	debugLevel ( UInt32 OptionBits )
{
	DebugObject*	Debug = &TMManGlobal->Debug;
	UInt32	        TempOptionBits;
	
	TempOptionBits        = Debug->DBGLevelBitmap;
	Debug->DBGLevelBitmap = OptionBits;
	return TempOptionBits;
}


// unsigned int for pSOS strings
void	debugPrintf( 
					String FormatString,    //DL
					... )
{
	UInt16	  Idx;
	UInt32          StrIdx = 0;
	UInt8	          Char;
	UInt8           byteArg;
	UInt32          wordArg;
	DebugObject*	  Debug = &TMManGlobal->Debug;
	va_list         pArgument;
	
	// this function is ment for a 32 bit environment.
	// modify pointer increment values for 32 bit stack.
	//((Pointer *)pArgument)++;
	va_start ( pArgument, FormatString );
	
	for( Idx = 0 ; FormatString[Idx] ; Idx ++ )
    {
		/* check if we are overflowing the temp buffer. */
		if ( ! ( StrIdx < constTMManStringLength ) )
		{
			StrIdx--;
			break;
		} 
		
		if( FormatString[Idx] == '%')
		{
			Char = FormatString[++Idx];
			switch( Char )
			{
				
			case 'd':
				wordArg = va_arg ( pArgument, UInt32 );
				debugProcessDec ( 
					Debug->DBGBuffer, 
					&StrIdx, 
					wordArg);
				break;
				
			case 's':
				wordArg = va_arg ( pArgument, UInt32 );
				debugProcessStr ( 
					Debug->DBGBuffer,
					&StrIdx, 
					(UInt8 *)wordArg);
				
				break;
				
			case 'c':
				byteArg = va_arg ( pArgument, UInt8 );
				debugProcessChar ( 
					Debug->DBGBuffer,
					&StrIdx, 
					byteArg);
				break;
				
			case 'x':
				wordArg = va_arg ( pArgument, UInt32 );
				debugProcessHex ( 
					Debug->DBGBuffer, 
					&StrIdx, 
					wordArg);
				break;
				
			default :
				Debug->DBGBuffer[StrIdx++] = ('%');
				Debug->DBGBuffer[StrIdx++] = (Char);
				break;
			}
		}
		else
		{
			Debug->DBGBuffer[StrIdx++] = FormatString[Idx];
			continue;
		}
    }
	
	Debug->DBGBuffer[StrIdx] = 0;
	
	va_end ( pArgument );
	
	switch ( Debug->DBGType )
    {
    case constTMManDebugTypeTrace :
		debugTrace ( Debug->DBGBuffer );
		break;
		
    case constTMManDebugTypeOutput :
		debugOutput ( Debug->DBGBuffer );
		break;
		
    case constTMManDebugTypeNULL :	
    case constTMManDebugTypeCOM :	
    case constTMManDebugTypeLPT :	
    default :
		break;
    }
}

void	debugOutput ( UInt8* szString )
{
	//printf("%s", szString);
	DPF(0,("%s",szString));
}

void	debugTrace ( UInt8* szString )
{
	// It can dump the output both to monochrone and write it
	// to the debug buffer - for offline retrieval
	DebugObject*	Debug = &TMManGlobal->Debug;
	UInt32          Idx;
	
    
	for ( Idx = 0; szString[Idx]; Idx++ )
    {
		Debug->TraceBufferData[Debug->TracePosition] = szString[Idx];
		
		if ( ++Debug->TracePosition >= Debug->TraceLength )
		{
			Debug->TracePosition = 0;
			Debug->TraceWrapped = 1;
		}
    }
}

// get the debug buffers on the target(other)processor
Bool	debugDPBuffers (
						UInt32	HalHandle,
						UInt8 **ppFirstHalfPtr, 
						UInt32 *pFirstHalfSize, 
						UInt8 **ppSecondHalfPtr, 
						UInt32 *pSecondHalfSize )
{
	UInt8*	      SDRAMPtr;
	UInt32	      SDRAMMapped;
	UInt32	      SDRAMLength;
	UInt32	      SDRAMPhysical;
	UInt32	      MMIOMapped;
	UInt32	      MMIOLength;
	UInt32	      MMIOPhysical;
	DebugControl*	      DebugPtr;
	UInt8*	      BufferPtr;
	UInt32              Idx;
	UInt8	              MagicString[constDebugMagicSize];
	UInt32              MagicLen;
	UInt32*             pAddr;	
	UInt32              NumLWords;
	/*
	unused variable
	UInt32             *pTemp;
	UInt32              i;
	*/
	
	/* initialize them to NULL in case we don't find the MAGIC header */
	
	*ppFirstHalfPtr  = NULL;
	*ppSecondHalfPtr = NULL;
	*pFirstHalfSize  = 0;
	*pSecondHalfSize = 0;
	
	halGetSDRAMInfo ( 
		HalHandle,
		(Pointer*)&SDRAMPhysical, 
		(Pointer*)&SDRAMMapped, 
		&SDRAMLength );
	
	halGetMMIOInfo ( 
		HalHandle,
		(Pointer*)&MMIOPhysical, 
		(Pointer*)&MMIOMapped, 
		&MMIOLength );
	
	SDRAMPtr = (UInt8*)SDRAMMapped;
	
	strcpy ( MagicString, constDebugMagic );
	
	MagicString[0] = 'T';
	MagicString[1] = 'M';
	MagicString[2] = '-';
	MagicString[3] = 'S';
	MagicString[4] = 'o';
	MagicString[5] = 'f';
	MagicString[6] = 't';
	
	// Byte swap the Magic String
	MagicLen = strlen(MagicString) / 4;
	
	/* DL    for (Idx = 0; Idx < MagicLen; Idx++)
	{
	pTemp = (UInt32 *)(&(MagicString[(Idx*4)]));
	*pTemp = BYTE_SWAP4(*pTemp);
	}
	*/
	
	
	pAddr     = (UInt32 *)SDRAMMapped;
	NumLWords = SDRAMLength / sizeof(UInt32);
	
	DPF(8, ("AO_FREQ: 0x%lx\n", *(UInt32*)(MMIOMapped + AO_FREQ)));
	DPF(8, ("INT_CTL: 0x%lx\n", *(UInt32*)(MMIOMapped + INT_CTL)));
	
	
	// search the entire SDRAM for the magic string 
	for ( Idx = 0 ; 
	Idx < SDRAMLength ;
	Idx += constDebugMagicSize, SDRAMPtr += constDebugMagicSize )
    {
		if ( memcmp( SDRAMPtr, MagicString, MagicLen ) == 0 )
		{
			DPF(8, ("Found: 0x%lx\n", (UInt32)SDRAMPtr));
			break;
		}
    }
	
	if ( Idx >= SDRAMLength )
    {
		return False;
    }
	
	
	DebugPtr = (DebugControl *)SDRAMPtr;
	
	DPF(8, ("  debug.c: debugDPBuffers: DP Buffer Details:\n"));
	DPF(8, ("\tdebug.c: debugDPBuffers: Magic:     %s\n"  , DebugPtr->szMagic));
	DPF(8, ("\tdebug.c: debugDPBuffers: Wrapped:   0x%lx\n", DebugPtr->Wrapped));
	DPF(8, ("\tdebug.c: debugDPBuffers: AllocBase: 0x%p\n", DebugPtr->AllocBase));
	DPF(8, ("\tdebug.c: debugDPBuffers: BufLen:    0x%lx\n", DebugPtr->BufLen));
	DPF(8, ("\tdebug.c: debugDPBuffers: BufPos:    0x%lx\n", DebugPtr->BufPos));
	DPF(8, ("\tdebug.c: debugDPBuffers: Buffer:    0x%p\n", DebugPtr->Buffer));
	
	/* convert the physical TM address to mapped MIPS address */
	BufferPtr = (UInt8*)
		((halAccess32(HalHandle , (volatile UInt32)DebugPtr->Buffer) - 
		SDRAMPhysical) + SDRAMMapped );
	
	if ( halAccess32( HalHandle, DebugPtr->Wrapped ) ) // wrap around has occured 
    {
		*pFirstHalfSize = 
			halAccess32( HalHandle, DebugPtr->BufLen ) - 
			halAccess32( HalHandle, DebugPtr->BufPos );
		
		*ppFirstHalfPtr = BufferPtr + halAccess32( HalHandle, DebugPtr->BufPos );
    }
	
	*ppSecondHalfPtr = BufferPtr;
	*pSecondHalfSize = halAccess32( HalHandle, DebugPtr->BufPos );
	
	return True;
}

// get the debug TMMan internal buffers on the target(other)processor
Bool	debugTargetBuffers (
							UInt32	HalHandle,
							UInt8 **ppFirstHalfPtr, 
							UInt32 *pFirstHalfSize, 
							UInt8 **ppSecondHalfPtr, 
							UInt32 *pSecondHalfSize )
{
	UInt8*			SDRAMPtr;
	UInt32			SDRAMMapped;
	UInt32			SDRAMLength;
	UInt32			SDRAMPhysical;
	DebugControl*	                DebugPtr;
	UInt8*			BufferPtr;
	UInt32			Idx;
	UInt8		        	MagicString[constDebugMagicSize];
	UInt32                        MagicLen;
	/*
	unused variable
	UInt32                        NumLWords;	
	UInt32                        *pTemp;
	*/
	
	/* initialize them to NULL in case we don't find the MAGIC header */
	
	*ppFirstHalfPtr  = NULL;
	*ppSecondHalfPtr = NULL;
	*pFirstHalfSize  = 0;
	*pSecondHalfSize = 0;
	
	halGetSDRAMInfo ( 
		HalHandle,
		(Pointer*)&SDRAMPhysical, 
		(Pointer*)&SDRAMMapped, 
		&SDRAMLength );
	
	SDRAMPtr = (UInt8*)SDRAMMapped;
	
	strcpy ( MagicString, constDebugTMManMagic );
	
	MagicString[0] = 'T';
	MagicString[1] = 'i';
	MagicString[2] = 'l';
	MagicString[3] = 'a';
	MagicString[4] = 'k';
	MagicString[5] = 'r';
	MagicString[6] = 'a';
	MagicString[7] = 'j';
	
	// Byte swap the Magic String
	MagicLen = strlen(MagicString) / 4;
	
	/* DL for (Idx = 0; Idx < MagicLen; Idx++)
	{
	pTemp  = (UInt32 *)(&(MagicString[(Idx*4)]));
	*pTemp = BYTE_SWAP4(*pTemp);
	}
	*/
	
	// search the entire SDRAM for the magic string 
	DPF(8, ("Searching for '%s' from 0x%lx\n", MagicString, SDRAMMapped));
	for ( Idx = 0 ; 
	Idx < SDRAMLength ;
	Idx += constDebugMagicSize, SDRAMPtr += constDebugMagicSize )
    {
		if ( memcmp( SDRAMPtr, MagicString, (MagicLen * 4) ) == 0 )
		{
			DPF(8, ("Found: 0x%lx\n", (UInt32)SDRAMPtr));
			break;
		}
    }
	
	if ( Idx >= SDRAMLength )
    {
		return False;
    }
	
	
	DebugPtr = (DebugControl*)SDRAMPtr;
	
	DPF(8, ("  debug.c: debugTargetBuffers: DebugTarget Buffer Details:\n"));
	DPF(8, ("\tdebug.c: debugTargetBuffers: Magic:     %s\n"  , DebugPtr->szMagic));
	DPF(8, ("\tdebug.c: debugTargetBuffers: Wrapped:   0x%lx\n", DebugPtr->Wrapped));
	DPF(8, ("\tdebug.c: debugTargetBuffers: AllocBase: 0x%p\n", DebugPtr->AllocBase));
	DPF(8, ("\tdebug.c: debugTargetBuffers: BufLen:    0x%lx\n", DebugPtr->BufLen));
	DPF(8, ("\tdebug.c: debugTargetBuffers: BufPos:    0x%lx\n", DebugPtr->BufPos));
	DPF(8, ("\tdebug.c: debugTargetBuffers: Buffer:    0x%p\n", DebugPtr->Buffer));
	
	
	/* convert the physical address to mapped address */
	BufferPtr = (UInt8*)
		( ( halAccess32( HalHandle 
		, (volatile UInt32)DebugPtr->Buffer ) - SDRAMPhysical ) + 
		SDRAMMapped );
	
	if ( halAccess32( HalHandle, DebugPtr->Wrapped ) ) // wrap around has occured 
    {
		*pFirstHalfSize = 
			halAccess32( HalHandle, DebugPtr->BufLen ) - 
			halAccess32( HalHandle, DebugPtr->BufPos );
		*ppFirstHalfPtr = BufferPtr + halAccess32( HalHandle, DebugPtr->BufPos );
    }
	
	*ppSecondHalfPtr = BufferPtr;
	*pSecondHalfSize = halAccess32( HalHandle, DebugPtr->BufPos );
	
	
	return True;
							}

							
							// get the debug buffer on the host(this)processor
							Bool	debugHostBuffers (
								UInt8 **ppFirstHalfPtr, 
								UInt32 *pFirstHalfSize, 
								UInt8 **ppSecondHalfPtr, 
								UInt32 *pSecondHalfSize )
							{
								DebugObject*	Debug = &TMManGlobal->Debug;
								UInt8*	BufferPtr;
								
								*ppFirstHalfPtr  = NULL;
								*ppSecondHalfPtr = NULL;
								*pFirstHalfSize  = 0;
								*pSecondHalfSize = 0;
								
								
								BufferPtr = Debug->TraceBuffer;
								
								if (Debug->TraceWrapped) // wrap around has occured 
								{
									*pFirstHalfSize = Debug->TraceLength - Debug->TracePosition;
									*ppFirstHalfPtr = BufferPtr + Debug->TracePosition;
								}
								
								*ppSecondHalfPtr = BufferPtr;
								*pSecondHalfSize = Debug->TracePosition;
								
								return True;
							}
							
							
							
							void	debugProcessChar ( UInt8* pszBuffer, UInt32* pdwPosition, UInt8 Value  )
							{
								pszBuffer[(*pdwPosition)++] = Value;
							}
							
							void	debugProcessDec ( UInt8* pszBuffer, UInt32* pdwPosition, UInt32 Value  )
							{
								UInt32	Divisor;
								
								for( Divisor = 1 ; (Value / Divisor) >= 10 ;
								Divisor *= 10);
								do
								{
									pszBuffer[(*pdwPosition)++] = (UInt8)( (Value / Divisor) + '0');
									
									Value = (UInt32)(Value % Divisor);
									Divisor /= 10;
								} while ( Divisor > 0);
							}
							
							void	debugProcessHex ( UInt8* pszBuffer, UInt32* pdwPosition, UInt32 Value  )
							{
								UInt8  *Hex;
								UInt32	Divisor;
								
								Hex = "0123456789ABCDEF";
								for( Divisor = 1 ; (Value / Divisor) >= 16 ;
								Divisor *= 16);
								
								do
								{
									pszBuffer[(*pdwPosition)++] = (Hex[(Value / Divisor)]);
									Value = (UInt32)(Value % Divisor);
									Divisor /= 16;
								} while ( Divisor > 0);
							}
							
							void	debugProcessStr ( UInt8* pszBuffer, UInt32* pdwPosition, UInt8* Value  )
							{
								UInt32 BufIdx;
								for ( BufIdx = 0 ; Value[BufIdx];	BufIdx++ )
									pszBuffer[(*pdwPosition)++]  = Value[BufIdx];
							}
							
							
							
							
							
							
							
							
							
							
							
