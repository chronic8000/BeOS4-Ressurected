/*
	(c) Philips Semiconductors - TriMedia Product Group 1998

	TriMedia Manager Status Codes

	FILE	error.c

	Author : T.Roy (Tilakraj.Roychoudhury@sv.sc.philips.com)


    990511  DTO Ported to pSOS
*/

#include "TMDownLoader.h"
#include "tmmanerr.h"


String  errorGetErrorString (
	UInt32 ErrorCode )
{
	switch ( ErrorCode )
	{

		case	statusSuccess :	return "statusSuccess";

		case	statusInvalidHandle : return "statusInvalidHandle";

		case	statusObjectAllocFail : return "statusObjectAllocFail";

		case	statusObjectListAllocFail : return "statusObjectListAllocFail";

		case	statusObjectInsertFail : return "statusObjectInsertFail";

		case	statusNotImplemented : return "statusNotImplemented";

		case	statusUnsupportedOnThisPlatform : return "statusUnsupportedOnThisPlatform";

		case	statusDSPNumberOutofRange : return "statusDSPNumberOutofRange";

		case	statusMajorVersionError : return "statusMajorVersionError";

		case	statusMinorVersionError : return "statusMinorVersionError";

		case	statusDeviceIoCtlFail : return "statusDeviceIoCtlFail";

		case	statusImageMemoryAllocationFail : return "statusImageMemoryAllocationFail";

		case	statusUnknownComponent : return "statusUnknownComponent";

		case	statusCriticalSectionCreateFail : return "statusCriticalSectionCreateFail";

		case	statusExecutableFileWrongEndianness : return "statusExecutableFileWrongEndianness";

		case	statusSynchronizationObjectCreateFail : return "statusSynchronizationObjectCreateFail";

		case	statusQueueObjectCreateFail : return "statusQueueObjectCreateFail";

		case	statusPCIConfigAccessFail : return "statusPCIConfigAccessFail";

		case	statusStringBufferOverflow : return "statusStringBufferOverflow";

		case	statusIncorrectParameter : return "statusIncorrectParameter";



		case	statusHalInitializationFail : return "statusHalInitializationFail";

		case	statusHalAllocInterruptFail : return "statusHalAllocInterruptFail";

		case	statusHalConnectInterruptFail : return "statusHalConnectInterruptFail";

		case	statusEventInterruptRegistrationFail : return "statusEventInterruptRegistrationFail";

		case	statusMemoryUnavailable : return "statusMemoryUnavailable";

		case	statusMessageQueueEmptyError : return "statusMessageQueueEmptyError";

		/* Downloader Error Codes */

		case	statusDebugNoDebugInformation : return "statusDebugNoDebugInformation";

		case	statusNameSpaceNoMoreSlots : return "statusNameSpaceNoMoreSlots";

		case	statusNameSpaceLengthExceeded : return "statusNameSpaceLengthExceeded";

		case	statusNameSpaceNameConflict : return "statusNameSpaceNameConflict";

		case	statusNameSpaceNameNonExistent : return "statusNameSpaceNameNonExistent";

		case	statusVIntrObjectAllocFail : return "statusVIntrObjectAllocFail";

		case	statusSGBufferNoMoreEntries : return "statusSGBufferNoMoreEntries";

		case	statusSGBufferOffsetOutOfRange : return "statusSGBufferOffsetOutOfRange";

		case	statusSGBufferSizeOutOfRange : return "statusSGBufferSizeOutOfRange";

		case	statusSGBufferTempPageAllocFail : return "statusSGBufferTempPageAllocFail";

		case	statusSGBufferPageLockingFail : return "statusSGBufferPageLockingFail";

		case	statusSGBufferPageTableSizeFail	 : return "statusSGBufferPageTableSizeFail";

		case	statusSGBufferInvalidPageTable : return "statusSGBufferInvalidPageTable";

		case	statusChannelInterruptRegistrationFail : return "statusChannelInterruptRegistrationFail";

		case	statusChannelMailboxFullError : return "statusChannelMailboxFullError";

		case	statusUnknownErrorCode : 

		default :
#ifdef	TMMAN_HOST
		if ( ( ErrorCode >= statusDownloaderErrorStart ) && ( ErrorCode <= statusDownloaderErrorEnd ) )
		{
			return TMDwnLdr_get_last_error (tmmanTMManStatusToDownloaderStatus(ErrorCode) );
		}
#endif
		return "statusUnknownErrorCode";

	}
}
