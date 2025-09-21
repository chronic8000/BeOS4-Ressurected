//---------------------------------------------------------------------------
//   Loaders.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1997  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     DSP loader functions.   
//---------------------------------------------------------------------------

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include "monkey.h"
#include "56301.h"
#include "commpage.h"
#include "loaders.h"
#include "util.h"

//
//	DSP and ASIC code arrays
//
extern BYTE pbLaylaASIC[];

#define LAYLA_ASIC_SIZE		32385


//--------------------------------------------------------------------------
// Call this after loading the DSP or the ASIC for Layla.  Checks the
// return value from the DSP and sets the pMI flag accordingly
//--------------------------------------------------------------------------
void CheckAsicStatus( PMONKEYINFO pMI )
{
	DWORD		dwAsicStatus;
	status_t	rval;
	int			read_attempts;

	// Only check the ASIC for Layla
	if ( pMI->wCardType != LAYLA )
		return;
		
	DPF2(("DGL: CheckAsicStatus\n"));

	//
	// If this is a Layla, the DSP will return a value to 
	// indicate whether or not the ASIC is currently loaded
	//
	// Since it can take a while for the ASIC value to come back, give it 
	// a couple of chances
	//
	read_attempts = 10;
	rval = B_ERROR;
	while ( (read_attempts > 0) &&
			(B_ERROR == rval))
	{
		rval = VMONK_Read_DSP( pMI, &dwAsicStatus);	
		read_attempts--;	
	}
	
	if (FAIL == rval)
	{
		pMI->bASICLoaded = FALSE;
		DPF2(("DGL:\tFailed on reading the DSP\n"));
		
#ifdef DEBUG
		kernel_debugger("Failed on reading the DSP for ASIC status\n");
#endif	
		return;
	}


#ifdef DEBUG	
	if ( dwAsicStatus != LAYLA_ASIC_ALREADY_LOADED &&
		  dwAsicStatus != LAYLA_ASIC_NOT_LOADED )
	{	
		DEBUGBREAK; 
		DPF2(("DGL:\tBad value for ASIC status 0x%x\n",dwAsicStatus));
		kernel_debugger("Bad value for ASIC status");
	}
#endif
	
	if ( dwAsicStatus == LAYLA_ASIC_ALREADY_LOADED )
		pMI->bASICLoaded = TRUE;
	else
	{
		pMI->bASICLoaded = FALSE;
		pMI->dwFlags |= PMI_FLAG_ASIC_WAIT;
	}
		
	DPF2(("DGL:\tASIC status is 0x%x\n",dwAsicStatus));
	
} // end of CheckAsicStatus


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
BOOL	LoadASIC(PMONKEYINFO pMI, PBYTE pCode, DWORD dwSize)
{
	DWORD 		i;
		
	DPF2(("LoadASIC\n"));

	if (pMI->bASICLoaded == TRUE)
		return TRUE;

	// Send the "Here comes the ASIC" command
	if (VMONK_Write_DSP(pMI, LOAD_LAYLA_ASIC) == FAIL)
		return FALSE;

	// Write length of ASIC file in bytes
	if (VMONK_Write_DSP(pMI, dwSize) == FAIL)
		return FALSE;

	for (i=0; i<dwSize; i++)
	{
		if (VMONK_Write_DSP(pMI,pCode[i]) == FAIL)
			return FALSE;
	}
	//
	// Check if ASIC is alive and well.
	//
	CheckAsicStatus( pMI );
	return pMI->bASICLoaded;

}	// LoadASIC


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
status_t	LoadDSP(PMONKEYINFO pMI, PWORD pCode)
{
	PDWORD		pMonkeyBase;
	DWORD		dwAddress;
	DWORD		dwIndex,dwTemp;
	int			iNum;
	int			i;
	DWORD		dwReturn;
	bigtime_t	timeout;

	DPF2(("DGL: LoadDSP\n"));
	pMonkeyBase = pMI->pMonkeyBase;

	// Send software reset command	
	if (B_ERROR == SendVector(pMI,0x8001 | 0xfe))
	{
		DPF2(("Unable to perform software reset!\n"));
		return B_ERROR;
	}
	
	// Delay 10ms
	timeout = system_time() + 10000;
	while (system_time() < timeout) 
		;

	// Wait for HF3 to indicate that software reset is complete	
	timeout = system_time() + 10000;
	while (system_time() < timeout) 
	{
		dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
		if ((dwTemp & CHI32_STATUS_REG_HF3))
			break;
	}
	if (system_time() > timeout)
	{
		DPF2(("Timed out waiting for software reset acknowledge\n"));
		DEBUGBREAK;
		return B_ERROR;
	}
	
	if ( NULL == pMI->pCommPage )
	{
		DPF2( ("LoadDSP: No comm page allocated, load aborted\n") );
		DEBUGBREAK;
		return B_ERROR;
	}

	// Set DSP format bits for 24 bit mode now that soft reset is done
	dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_CONTROL_REG]);
	dwTemp |= 0x900;
	pMonkeyBase[CHI32_CONTROL_REG] = B_HOST_TO_LENDIAN_INT32(dwTemp);
	__eieio();

	DPF2(("DGL: \tSoftware reset done\n"));

	//---------------------------------------------------------------------------
	// Main loader loop
	//---------------------------------------------------------------------------
	dwIndex = pCode[0];

	for (;;)
	{
		int	iBlockType;
		int	iMemType;

		// Total Block Size
		dwIndex++;
		
		// Block Type
		iBlockType = pCode[dwIndex];
		if (iBlockType == 4)  // We're finished
			break;

		dwIndex++;

		// Memory Type  P=0,X=1,Y=2
		iMemType = pCode[dwIndex]; 
		dwIndex++;
		
		// Block Code Size
		iNum = pCode[dwIndex];
		dwIndex++;
		if (iNum == 0)  // We're finished
			break;
	
 		// Start Address
		dwAddress = (pCode[dwIndex]<<16)+pCode[dwIndex+1];
//		DPF2(("\ndwAddress %lX", dwAddress));
		dwIndex+=2;
		
		dwReturn = VMONK_Write_DSP(pMI,(DWORD)iNum);
		if (dwReturn != 0)
		{
			DPF2( ("LoadDSP: Failed to write DSP word count\n") );
			DPF2(("LoadDSP: dwIndex is %d\n",dwIndex));
			return B_ERROR;
		}

		dwReturn = VMONK_Write_DSP(pMI,(DWORD)dwAddress);
		if (dwReturn != 0)
		{
			DPF2(("LoadDSP: Failed to write DSP address\n"));
			return B_ERROR;
		}

		dwReturn = VMONK_Write_DSP(pMI,(DWORD)iMemType);
		if (dwReturn != 0)
		{
			DPF2(("LoadDSP: Failed to write DSP memory type\n"));
			return B_ERROR;
		}

		// Code
		for ( i = 0; i < iNum; i++) // 
		{
			DWORD	dwData;

			dwData = (pCode[dwIndex]<<16)+pCode[dwIndex+1];
			dwReturn = VMONK_Write_DSP(pMI,dwData);
			if (dwReturn != 0)
			{
				DPF2(("LoadDSP: Failed to write DSP code\n"));
				return B_ERROR;
			}

			dwIndex+=2;
		}
//		DPF2(("\nEnd Code Block\n"));
	}
	dwReturn = VMONK_Write_DSP(pMI,0); // We're done!!!
	if (dwReturn != 0)
	{
		DPF2(("LoadDSP: Failed to write final zero to DSP\n"));
		return B_ERROR;
	}

	timeout = system_time() + 500000; // Half a second timeout
	while (system_time() < timeout)
	{
		dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
		if (dwTemp & CHI32_STATUS_REG_HF4)
		{
			// Set for 32 bit format now that load is done
			dwTemp = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_CONTROL_REG]);
			dwTemp &=~0x1b00;
			pMonkeyBase[CHI32_CONTROL_REG] = B_HOST_TO_LENDIAN_INT32(dwTemp);
			__eieio();

			if (pMI->wCardType == LAYLA)
			{
				CheckAsicStatus(pMI);
			}

			if (B_OK != SetCommPage(pMI))
			{
				return B_ERROR;
			}

			pMonkeyBase[ CHI32_VECTOR_REG ] = B_HOST_TO_LENDIAN_INT32(CONFIRM_XFER_STATE);
			__eieio();
			
			return B_OK;
		}
	}

	return B_ERROR;

}	// DWORD	LoadDSP(PMONKEYINFO pMI, PWORD pCode)


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
status_t SetCommPage(PMONKEYINFO pMI)
{
	DWORD	dwReturn;
	int		j;
	DWORD	dwSerialTemp;
	
	dwReturn = VMONK_Write_DSP( pMI,SET_COMMPAGE_ADDR );
	if (dwReturn != 0)
	{
		dprintf("SetCommPage: Failed to write SET_COMMPAGE_ADDR\n");
		return B_ERROR;
	}

	dwReturn = VMONK_Write_DSP( pMI,pMI->dwCommPagePhys );
	if (dwReturn != 0)
	{
		dprintf("SetCommPage: Failed to write comm page address\n");
		return B_ERROR;
	}

	// Get the serial number via slave mode
	// This is triggered by the SET_COMMPAGE_ADDR command
	// Serial number not used in BeOS, so throw it away
	for (j=0; j<5; j++)
	{
		dwReturn = VMONK_Read_DSP( pMI, &dwSerialTemp);
		if (dwReturn != 0)
		{
			dprintf("LoadDSP: Failed to read serial number word %d\n",j);
			return B_ERROR;
		}

	}
	
	DPF2(("DGL: SetCommPage for %s to 0x%x\n",pMI->szCardBaseName,pMI->dwCommPagePhys));

	return B_OK;
			
} // end of SetCommPage
			
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
status_t LoadFirmware( PMONKEYINFO pMI )
{
	PWORD		pwDspCode = NULL;
	status_t	rval;

	DPF2(("DGL: LoadFirmware\n"));

	//
	//	If we failed in the Plug and Play configuration, then we're dead
	//	so no point in trying to load DSP or ASIC.
	//
	if ( pMI->dwFlags & PMI_FLAG_BADBOARD )
	{
		DPF2(("DGL: \tBad board; not loading\n"));	
		return B_ERROR;
	}

	pwDspCode = CT->pwDspCode;
	rval = LoadDSP( pMI, pwDspCode );
	if (B_ERROR == rval)
	{
		pMI->dwFlags |= PMI_FLAG_BADBOARD;
		return B_ERROR;
	}
	
	// Load the ASIC if this is Layla and the DSP load succeeded
	if ( ( pMI->wCardType == LAYLA ) && 
		  ( ( pMI->dwFlags & PMI_FLAG_BADBOARD ) == 0 ) )
	{		
		BOOL	bASICLoad;

		bASICLoad = LoadASIC(pMI, pbLaylaASIC, LAYLA_ASIC_SIZE);
		if ( FALSE == bASICLoad )
		{
			pMI->dwFlags |= PMI_FLAG_BADBOARD;
			return B_ERROR;
		}
	}

	return rval;
	
}	// BOOL LoadFirmware( PMONKEYINFO pMI )

//	end file Loaders.c
