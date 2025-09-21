/******************************************************************************

  File Name:			OsNVM.c

  File Description:		This file implements the Os dependent part of the NVM Manager.

  Function list:

  GLOBAL:

  STATIC:

*******************************************************************************/
#include "ostypedefs.h"
#include "comtypes.h"
#include "configtypes.h"
#include "osmemory_ex.h"
#include "testdebug.h"
#include "countries.h"

static const PROFILE_DATA g_FactoryProfile = 
{
    1,      // Echo
    1,      // Volume
    1,      // Speaker control
    0,      // Pulse
    0,      // Quiet
    1,      // Verbose
    4,      // report Level
	0,		// Connect message
    1,      // AmperC
    0,      // AmperD
    0,      // AmperG
    0,      // AmperT
    0,      // S0
    0,      // S1;
    43,     // S2;
    13,     // S3;
    10,     // S4;
    8,      // S5;
    2,      // S6;
    50,     // S7;
    2,      // S8;
    14,     // S10;
    50,     // S12;
    0,      // S16;
    0,      // S18;
    70      // S29;
};
static OEMFlagStructure OEM_Flags = {
      1,                               // OEMFlag0:  fUseTIES
      1,                               //            fAnalogSpeaker
      0,                               //            fDataFaxRemoteTAM
      0,                               //            fDataFaxVoiceView
      0,                               //            fLCSPresent
      1,                               //            f3VoltIA
      0,                               //            fRemHangupPresent
      0,                               //            fSpkMuteHWPresent

      0,                               // OEMFlag1:  fReadCountryFromSwitch
      0,                               //            fLegacy
      0,                               //            fPME_Enable
      0,                               //            fAlternateConnectMSG
      0,                               //            fCountryNoneSelect
      0,                               //            fSpkMuteHWPolarity
      0,                               //            fLocalSpkBoostTAM
      0,                               //            fMicSpkControl

      0,                               // OEMFlag2:  fNEC_ACPI
      0,                               //            fDisForceLAPMWhenNoV8bis
      0,                               //            fDosSupportDisabled
      0,                               //            fDosCheckBoxSupport
      0,                               //            fCIDHandsetMode
      0,                               //            fExtOffHookHWPresent
      0,                               //            fLAN_Modem_Support
      0,                               //            fPortCloseConnected

      0,                               // OEMFlag3:  fFullGPIO
      0,                               //            fD3CIDCapable
      0,                               //            fLineInUsed:
      0,                               //            fFlag3Spare3
      0,                               //            fFlag3Spare4
      0,                               //            fV23AnswerDis
      0,                               //            fDialSkipS6
      0                                //            fFxCallCadFlag
};

static OEMFilterStructure OEMFilters = 
{
	{
		0x0001,
		0x01B3, 0xFC9C, 0x01B4, 0xC147, 0x48C6, 
		0x01B3, 0x0097, 0x01B4, 0xC147, 0x4897,
		0x7E63,
		0x02DF,
		0x1C00,
		0x1C00,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0
	},
	{
		0x0002,
		0x01E8, 0xFC32, 0x01E9, 0xC147, 0xDF4F, 
		0x01E8, 0x03E4, 0x01E9, 0xC147, 0xDF19,
		0x7F30,
		0x00CF,
		0x1600,
		0x0A00,
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000,
		1
	},
	{
		0x0001,
		0x0205, 0xFBF9, 0x0206, 0xC147, 0xD22D, 
		0x0205, 0x0380, 0x0206, 0xC147, 0xD1F8,
		0x7E67,
		0x02DF,
		0x1C00,
		0x1C00,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0
	},
	{
		0x0001,
		0x054D, 0xF54B, 0x0569, 0xC1E8, 0x4A5F, 
		0x064D, 0x02C6, 0x0569, 0xC1E8, 0x463A,
		0x7E67,
		0x02DF,
		0x1100,
		0x1100,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0
	},
	{
		0x0002,
		0x0244, 0xFB7B, 0x0245, 0xC147, 0x35A7, 
		0x0244, 0x00CA, 0x0245, 0xC147, 0x3574,
		0x7F30,
		0x00CF,
		0x1000,
		0x1000,
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000,
		1
	},
	{
		0x0002,
		0x0C84, 0x0000, 0x0CC5, 0xC289, 0xE2B8, 
		0x0C84, 0x0E80, 0x0CC5, 0xC289, 0xDC60,
		0x7F30,
		0x00CF,
		0xC000,
		0xC000,
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x3FFF, 0x0000, 0x0000,
		1
	},
	{
		0x0001,
		0x0205, 0xFBF9, 0x0206, 0xC147, 0xD22D, 
		0x0205, 0x0380, 0x0206, 0xC147, 0xD1F8,
		0x7E67,
		0x02DF,
		0x3300,
		0x3300,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0
	}
};

static UINT16 OEMSpeakerMuteDelay = 0x012C;
static UINT16 OEMReadMask = 0;
static OEMThresholdStruct OEMThreshold = 
{
	{0, 0}, //Modulation		V21;
	{0, 0}, //Modulation		V21fax;
	{0, 0}, //Modulation		V23;
	{0, 0}, //Modulation		V22;
	{0, 0}, //Modulation		V22b;
	{0, 0}, //Modulation		V32;
	{0, 0}, //Modulation		V32b;
	{0, 0}, //Modulation		V34;
	{0, 0}, //Modulation		K56;
	{0, 0}, //Modulation		V90;
	{0, 0}, //Modulation		V27;
	{0, 0}, //Modulation		V29;
	{0, 0}, //Modulation		V17;
};



HANDLE NVM_Open(UINT32 dwDevNode)
{
//open NVM file and read contents to global struct
	return (HANDLE) 1;
}

COM_STATUS NVM_Close(HANDLE hNVM)
{
    return COM_STATUS_SUCCESS;
}

COM_STATUS NVM_Read(HANDLE hNVM, CFGMGR_CODE eCode, PVOID pBuf, UINT32 *pdwSize)
{
	PVOID 	pData;
	UINT32	dwSize;
	
	// look up eCode in global struct and return value
    switch (eCode)
    {
		// special case - country parameters
    	case CFGMGR_COUNTRY_STRUCT:
			OsMemCpy(pBuf, (void*)&g_Country_Parms[0], sizeof(CtryPrmsStruct));
			return COM_STATUS_SUCCESS;
    		break;
    		
    	// special case - hardcoded Factory Default Profile
	    case CFGMGR_PROFILE_STORED:
    	case CFGMGR_PROFILE_FACTORY:
			TRACE(T__NVMMGR, ("NVM_Read(CFGMGR_PROFILE_FACTORY) - restoring default\n"));
			pData = (void*)&g_FactoryProfile;
			dwSize = sizeof(PROFILE_DATA);
    		break;

		case CFGMGR_OEM_FLAGS:
			pData = (void*)&OEM_Flags;
			dwSize = sizeof(OEMFlagStructure);
			break;
	    case CFGMGR_OEM_FILTERS:
	    	pData = (void*)&OEMFilters;
	    	dwSize = sizeof(OEMFilterStructure);
	    	break;
	    case CFGMGR_OEM_SPKR_MUTE_DELAY:
	    	pData = (void*)&OEMSpeakerMuteDelay;
	    	dwSize = sizeof(OEMSpeakerMuteDelay);
	    	break;
		case CFGMGR_OEM_READ_MASK:
			pData = (void*)&OEMReadMask;
			dwSize = sizeof(OEMReadMask);
			break;
	    case CFGMGR_OEM_THRESHOLD:
	    	pData = (void *)&OEMThreshold;
	    	dwSize = sizeof(OEMThreshold);
	    	break;
    
	    case CFGMGR_COUNTRY_CODE:
	    case CFGMGR_PREVIOUS_COUNTRY_CODE:
	    	*(int*)pBuf = 0xb5;
    		return COM_STATUS_SUCCESS;
	   		break;
	    case CFGMGR_COUNTRY_CODE_LIST:
	    	pData="B5";
	    	dwSize=3;
	   		break;

		case CFGMGR_POUND_UD:
			OsMemSet(pBuf, 0, *pdwSize);
		 	return COM_STATUS_SUCCESS;
			break;		
		case CFGMGR_LAST:
		// hard code these options and copy from their locations
//			pData = pBuf;
//			dwSize = 0;
//			break;
		default:  // efb We will need to address what params can be passed
	    	return COM_STATUS_FAIL;
	}

	*pdwSize = min(*pdwSize, dwSize);
	OsMemCpy(pBuf, pData, *pdwSize);
	return COM_STATUS_SUCCESS;
  }

COM_STATUS NVM_Write(HANDLE hNVM, CFGMGR_CODE eCode, PVOID pBuf, PUINT32 pdwSize)
{
	switch (eCode)
	{
		case CFGMGR_PROFILE_STORED:
		case CFGMGR_POUND_UD:
		case CFGMGR_PREVIOUS_COUNTRY_CODE:
		case CFGMGR_COUNTRY_CODE:
			break;
		default:
			return COM_STATUS_FAIL;
	}
// update eCode position with pBuf and save global struct to file
//efb
    return COM_STATUS_SUCCESS;

}

//static COM_STATUS NVM_ReadCountry(HANDLE hNVM, UINT16 wCountry, CtryPrmsStruct* pStruct)
//{
//// read country parms in from file
//}

#ifdef WINDOWS
#include "TestDebug.h"

static char szRegistryPath[128];

static COM_STATUS NVM_ConvertCode2Key(char* dstSubKey, char* dstName, PUINT32 pdwType, CFGMGR_CODE srcCode);
static COM_STATUS NVM_ReadCountry(HANDLE hNVM, UINT16 wCountry, CtryPrmsStruct* pStruct);
static COM_STATUS NVM_ReadKey(VMMHKEY hSubKey, char* szName, PVOID pBuf, PUINT32 pdwSize);



/********************************************************************************
    NVM_Open
********************************************************************************/
HANDLE NVM_Open(UINT32 dwDevNode)
{

	if (CR_SUCCESS != CM_Get_DevNode_Key(dwDevNode, 
                                         NULL, 
                                         szRegistryPath, 
                                         MAX_VMM_REG_KEY_LEN, 
		                                 CM_REGISTRY_SOFTWARE))
    {
        return (HANDLE)0;    
    }

    return (HANDLE)szRegistryPath;
}

/********************************************************************************
    NVM_Close
********************************************************************************/
COM_STATUS NVM_Close(HANDLE hNVM)
{
    return COM_STATUS_SUCCESS;
}

/********************************************************************************
    NVM_Read
********************************************************************************/
COM_STATUS NVM_Read(HANDLE hNVM, CFGMGR_CODE eCode, PVOID pBuf, UINT32 *pdwSize)
{
COM_STATUS cs = COM_STATUS_SUCCESS;
VMMHKEY hKeyS, hSubKey;
char szSubKey[128], szName[128];
UINT32 dwType;

    // special case - country parameters
    if (CFGMGR_COUNTRY_STRUCT == eCode)
        return NVM_ReadCountry(hNVM, (UINT16)pdwSize, pBuf);

    // special case - hardcoded Factory Default Profile
    if( CFGMGR_PROFILE_FACTORY == eCode)
    {
        TRACE(T__NVMMGR, ("NVM_Read(CFGMGR_PROFILE_FACTORY) - restoring default\n"));
        *pdwSize = min(*pdwSize, sizeof(PROFILE_DATA));
        OsMemCpy(pBuf, (void*)&g_FactoryProfile, *pdwSize);
        return COM_STATUS_SUCCESS;
    }
        
    if (COM_STATUS_SUCCESS != NVM_ConvertCode2Key(szSubKey, szName, &dwType, eCode))
        return COM_STATUS_FAIL;

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, hNVM, (PVMMHKEY)&hKeyS) == ERROR_SUCCESS)
        {
        if (szSubKey[0])
            {
            if (_RegOpenKey(hKeyS, szSubKey, (PVMMHKEY)&hSubKey) == ERROR_SUCCESS)
                {
                cs = NVM_ReadKey(hSubKey, szName, pBuf, pdwSize);
                _RegCloseKey(hSubKey);
                }
            }
        else
            {
            cs = NVM_ReadKey(hKeyS, szName, pBuf, pdwSize);
            }
        _RegCloseKey(hKeyS);
        }
/*
    // special case - stored profile
    if (CFGMGR_PROFILE_STORED == eCode)
        {
        if (COM_STATUS_VALUE_NOT_FOUND == cs)
            {
            // If no data in the registry - just return Factory Defaults
            *pdwSize = min(*pdwSize, sizeof(PROFILE_DATA));
            OsMemCpy(pBuf, (void*)&g_FactoryProfile, *pdwSize);
            return COM_STATUS_SUCCESS;
            }
        }
*/


    return cs;
}

/********************************************************************************
    NVM_Write
********************************************************************************/
COM_STATUS NVM_Write(HANDLE hNVM, CFGMGR_CODE eCode, PVOID pBuf, PUINT32 pdwSize)
{
VMMHKEY hKeyS, hSubKey;
char szSubKey[128], szName[128];
UINT32 dwType;
COM_STATUS cs = COM_STATUS_SUCCESS;

    if ((CFGMGR_PROFILE_STORED != eCode) &&
        (CFGMGR_POUND_UD != eCode) &&
        (CFGMGR_PREVIOUS_COUNTRY_CODE != eCode) &&
		(CFGMGR_COUNTRY_CODE != eCode))
        return COM_STATUS_FAIL;

    if (COM_STATUS_SUCCESS != NVM_ConvertCode2Key(szSubKey, szName, &dwType, eCode))
        return COM_STATUS_FAIL;

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, hNVM,(PVMMHKEY)&hKeyS) == ERROR_SUCCESS)
        {
        if (szSubKey[0])
            {
            if (cs = _RegOpenKey(hKeyS, szSubKey,(PVMMHKEY)&hSubKey) != ERROR_SUCCESS)
                {
                TRACE(T__NVMMGR, ("NVM_Write(%s) open key failed\n", szName));
                if (cs = _RegCreateKey(hKeyS, szSubKey, (PVMMHKEY)&hSubKey) != ERROR_SUCCESS)
                    {
                    TRACE(T__NVMMGR, ("NVM_Write(%s) create key failed\n", szName));
                    }
                }

            if (cs = _RegSetValueEx(hSubKey, szName, 0, dwType, pBuf, *pdwSize) != ERROR_SUCCESS)
                {
                TRACE(T__NVMMGR, ("NVM_Write(%s) set value failed\n", szName));
                }
			_RegCloseKey(hSubKey);
            }
        else
            {
            cs = _RegSetValueEx(hKeyS, szName, 0, dwType, pBuf, *pdwSize);
            }
        _RegCloseKey(hKeyS);
        }

    return cs;
}



typedef struct tagRegTagName {
    CFGMGR_CODE     eCode;
    UINT32          dwType;
    char            szSubKey[128];
    char            szName[32];
    } REG_TAG_NAME;

static REG_TAG_NAME RegTagName[] = {
    {CFGMGR_COUNTRY_CODE,               REG_DWORD,      "Country",      "Current"       },
    {CFGMGR_PREVIOUS_COUNTRY_CODE,      REG_DWORD,      "Country",      "Previous"      },
    {CFGMGR_COUNTRY_CODE_LIST,          REG_SZ,         "Country",      "CountryList"   },
    {CFGMGR_PROFILE_STORED,             REG_BINARY,     "",             "Profile"       },
    {CFGMGR_POUND_UD,                   REG_BINARY,     "",             "PoundUD"       },
    {CFGMGR_LAST,                       0,              0,              0               },
    };

/********************************************************************************
    NVM_ConvertCode2Key
********************************************************************************/
static COM_STATUS NVM_ConvertCode2Key(char* dstSubKey, char* dstName, PUINT32 pdwType, CFGMGR_CODE srcCode)
{
int i;

    for (i = 0; ; i++)
        {
        if (CFGMGR_LAST == RegTagName[i].eCode)
            return COM_STATUS_FAIL;

        if (srcCode == RegTagName[i].eCode)
            {
            OsStrCpy(dstSubKey, RegTagName[i].szSubKey);
            OsStrCpy(dstName, RegTagName[i].szName);
            *pdwType = RegTagName[i].dwType;
            break;
            }
        }

    return COM_STATUS_SUCCESS;
}

/********************************************************************************
    NVM_ReadCountry
********************************************************************************/
static COM_STATUS NVM_ReadCountry(HANDLE hNVM, UINT16 wCountry, CtryPrmsStruct* pStruct)
{
char szCountryKeyPath[MAX_OEM_STR_LEN];
COM_STATUS cs = COM_STATUS_SUCCESS;
VMMHKEY hKeyS, hSubKey;

    OsSprintf(szCountryKeyPath, "Country\\%04X", wCountry);

    if (_RegOpenKey(HKEY_LOCAL_MACHINE, hNVM, (PVMMHKEY)&hKeyS) == ERROR_SUCCESS)
        {
        if (_RegOpenKey(hKeyS, szCountryKeyPath, (PVMMHKEY)&hSubKey) == ERROR_SUCCESS)
            {
            UINT32 dwSize;
            dwSize = sizeof(pStruct->Blacklisting);
            NVM_ReadKey(hSubKey, "BLACKLISTING", &pStruct->Blacklisting, &dwSize);
            dwSize = sizeof(pStruct->Cadence);
            NVM_ReadKey(hSubKey, "CADENCE", &pStruct->Cadence, &dwSize);
            dwSize = sizeof(pStruct->CallerID);
            NVM_ReadKey(hSubKey, "CALLERID", &pStruct->CallerID, &dwSize);
            dwSize = sizeof(pStruct->DTMF);
            NVM_ReadKey(hSubKey, "DTMF", &pStruct->DTMF, &dwSize);
            dwSize = sizeof(pStruct->Filter);
            NVM_ReadKey(hSubKey, "FILTER", &pStruct->Filter, &dwSize);
            dwSize = sizeof(pStruct->Flags);
            NVM_ReadKey(hSubKey, "FLAGS", &pStruct->Flags, &dwSize);
            dwSize = sizeof(pStruct->cIntCode);
            NVM_ReadKey(hSubKey, "INTCODE", &pStruct->cIntCode, &dwSize);
            dwSize = sizeof(pStruct->cInter);
            NVM_ReadKey(hSubKey, "NAME", &pStruct->cInter, &dwSize);
            dwSize = sizeof(pStruct->Pulse);
            NVM_ReadKey(hSubKey, "PULSE", &pStruct->Pulse, &dwSize);
            dwSize = sizeof(pStruct->Relays);
            NVM_ReadKey(hSubKey, "RELAYS", &pStruct->Relays, &dwSize);
            dwSize = sizeof(pStruct->Ring);
            NVM_ReadKey(hSubKey, "RING", &pStruct->Ring, &dwSize);
            dwSize = sizeof(pStruct->RLSD);
            NVM_ReadKey(hSubKey, "RLSD", &pStruct->RLSD, &dwSize);
            dwSize = sizeof(pStruct->AgressSpeedIndex);
            NVM_ReadKey(hSubKey, "SPEEDADJUST", &pStruct->AgressSpeedIndex, &dwSize);
            dwSize = sizeof(pStruct->SRegLimits);
            NVM_ReadKey(hSubKey, "SREG", &pStruct->SRegLimits, &dwSize);
            dwSize = sizeof(pStruct->T35Code);
            NVM_ReadKey(hSubKey, "T35CODE", &pStruct->T35Code, &dwSize);
            dwSize = sizeof(pStruct->Threshold);
            NVM_ReadKey(hSubKey, "THRESHOLD", &pStruct->Threshold, &dwSize);
            dwSize = sizeof(pStruct->Timing);
            NVM_ReadKey(hSubKey, "TIMING", &pStruct->Timing, &dwSize);
            dwSize = sizeof(pStruct->Tone);
            NVM_ReadKey(hSubKey, "TONE", &pStruct->Tone, &dwSize);
            dwSize = sizeof(pStruct->Txlevel);
            NVM_ReadKey(hSubKey, "TXLEVEL", &pStruct->Txlevel, &dwSize);

            _RegCloseKey(hSubKey);
            }
        _RegCloseKey(hKeyS);
        }
    return cs;
}

/********************************************************************************
    NVM_ReadKey
********************************************************************************/
static COM_STATUS NVM_ReadKey(VMMHKEY hSubKey, char* szName, PVOID pBuf, PUINT32 pdwSize)
{
UINT32 BufferSize;
COM_STATUS cs = COM_STATUS_SUCCESS;
LONG lRet;

    // Calling routines assume that this function will only return TRUE
    // if the buffer is large enough to hold the entire string!!!!!
    BufferSize = *pdwSize;
    if ((lRet = _RegQueryValueEx(hSubKey, szName, NULL, NULL, pBuf, &BufferSize)) == ERROR_SUCCESS)
        {
        if (BufferSize != *pdwSize)
            {
            TRACE(T__NVMMGR, ("NVM_ReadKey(%s) size mismatch (actual data is shorter) requested=%d, returned=%d\n", szName, *pdwSize, BufferSize));
//            cs = COM_STATUS_FAIL;
            }
        }
    else if (ERROR_MORE_DATA == lRet)
		{
        TRACE(T__NVMMGR, ("NVM_ReadKey(%s) size mismatch (actual data is longer) requested=%d, returned=%d\n", szName, *pdwSize, BufferSize));
		cs = COM_STATUS_FAIL;
		}
	else
        {
        TRACE(T__ERROR, ("NVM_ReadKey(%s) read failed\n", szName));
        cs = COM_STATUS_VALUE_NOT_FOUND;
        }

    *pdwSize = BufferSize;

    return cs;
}

#endif