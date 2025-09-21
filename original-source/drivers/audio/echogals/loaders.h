//---------------------------------------------------------------------------
//   Loaders.H
//---------------------------------------------------------------------------
//	Copyright Echo Speech Corporation (c) 1996  All Rights Reserved.
//	Portions Copyright (c) 1993 Microsoft Corporation.  All Rights Reserved.
//---------------------------------------------------------------------------
//  Purpose:
//     DSP load function Headers.   
//---------------------------------------------------------------------------

//status_t LoadASIC(PMONKEYINFO pMI, PBYTE pCode, DWORD dwSize);
status_t	LoadDSP(PMONKEYINFO pMI, PWORD pCode) ;
status_t 	LoadFirmware( PMONKEYINFO pMI );
status_t	SetCommPage( PMONKEYINFO pMI );

// **** End Of Loaders.h ****
