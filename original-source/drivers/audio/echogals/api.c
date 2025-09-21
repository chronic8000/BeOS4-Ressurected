//   API.C
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1997  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     Plug -n- Play interface functions.   
//---------------------------------------------------------------------------

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include "monkey.h"
#include "56301.h"
#include "commpage.h"
#include "util.h"
#include "api.h"


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
DWORD	 StartDSP(PMONKEYINFO pMI)
{
	PCOMMPAGE	pCommPage;
	PDWORD		pMonkeyBase;
	DWORD		dwStatus,dwStart;
	bigtime_t	timeout;
	
	DPF2(("\n\nDGL: StartDSP\n"));
	
	pCommPage = pMI->pCommPage;
	dwStart = B_LENDIAN_TO_HOST_INT32(pCommPage->dwStart);
	if (0 == dwStart)
	{
		DPF1(("DGL: \tStartDSP called with dwStart set to zero!\n"));
		return 0;
	}

	DPF2(("DGL:\tStartDSP called with dwStart set to 0x%x\n",dwStart));
	
	SendVector(pMI, START_TRANSFER);
	pMonkeyBase = pMI->pMonkeyBase;
	timeout = system_time() + 10000;
	dwStatus = 0;
	while (system_time() < timeout) 
	{
		dwStatus = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
		if ((dwStatus & CHI32_STATUS_WAVE_ACK) != 0)  
		{
			break;
		}
	}
		
	if ((dwStatus & CHI32_STATUS_WAVE_ACK) == 0)
	{
		DPF2(("\tSTART punked out waiting for HF4\n"));
		DPF2(("\tStatus is 0x%x\n",
				B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG])));
		DEBUGBREAK;
	}
	else
	{
		SendVector(pMI, CONFIRM_XFER_STATE);
	}

	pCommPage->dwStart = 0;			
			
	return MMSYSERR_NOERROR;
}	// StartDSP


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
DWORD	 StopDSP(PMONKEYINFO pMI)
{
	PCOMMPAGE	pCommPage;
	PDWORD		pMonkeyBase;
	DWORD		dwStatus,dwStop,dwReset;
	bigtime_t	timeout;

	DPF2(("DGL: StopDSP\n"));

	pCommPage = pMI->pCommPage;
	
	dwStop = B_LENDIAN_TO_HOST_INT32(pCommPage->dwStop);		
	dwReset = B_LENDIAN_TO_HOST_INT32(pCommPage->dwReset);
	if ((dwStop != 0) || (dwReset != 0))
	{
		SendVector(pMI, STOP_TRANSFER);
	}

	if ((dwStop != 0) || (dwReset != 0))
	{
		pMonkeyBase = pMI->pMonkeyBase;
		timeout = system_time() + 10000;
		dwStatus = 0;
		while (system_time() < timeout) 
		{
			dwStatus = B_LENDIAN_TO_HOST_INT32(pMonkeyBase[CHI32_STATUS_REG]);
			if ((dwStatus & CHI32_STATUS_WAVE_ACK) != 0)  
			{
				break;
			}
		}
		
		if ((dwStatus & CHI32_STATUS_WAVE_ACK) == 0)
		{
			DPF2(("\tSTOP punked out waiting for HF4\n"));
			DEBUGBREAK;
		}
		else
		{
			SendVector(pMI, CONFIRM_XFER_STATE);
		}

		pCommPage->dwStop = 0;
		pCommPage->dwReset = 0;
	}

	return MMSYSERR_NOERROR;
}	// StopDSP


// *** End api.c ***
