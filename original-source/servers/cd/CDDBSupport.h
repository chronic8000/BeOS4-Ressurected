/*------------------------------------------------------------*/
//
//	File:		CDDBSupport.h
//
//	based on Jukebox by Chip Paul
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#ifndef CDDB_SUPPORT
#define CDDB_SUPPORT

#include <vector>

#include <NetEndpoint.h>
#include <String.h>
#include <scsi.h>


/*=================================================================*/

class CDDBQuery
{
	public:
							CDDBQuery			(const char*,
												 int32 port = 888,
												 bool log = false);
		int32				CurrentDiscID		() const
													{ return fDiscID; }
		static int32		GetDiscID			(const scsi_toc*);
		void				GetSites			(bool (*)
													(const char*,
													 int,
													 const char*,
													 const char*,
													 const char*,
													 void*),
												 void *);
		void				GetTitles			(scsi_toc*,
												 BMessage*,
												 BHandler*);
		bool				Ready				() const
													{ return fState == eDone; }
		void				SetToSite			(const char*,
												 int32);

	private:
		void				Connect				();
		void				CreateTmpFile		(BFile*,
												 entry_ref*);
		void				Disconnect			();
		static void			GetDiscID			(const scsi_toc*,
												 int32&,
												 int32&,
												 int32&,
												 BString&,
												 BString&);
		const char*			GetToken			(const char*,
												 BString&);
		void				IdentifySelf		();
		bool				IsConnected			() const;
		static int32		LookupBinder		(void*);
		void				ParseResult			(BDataIO*);
		void				ReadFromServer		(BDataIO*);
		void				ReadLine			(BString&);
		static void			ReadLine			(BDataIO*,
												 BString&);

	class Connector
	{
		public:
								Connector		(CDDBQuery* client)
								: fClient(client),
								  fWasConnected(fClient->IsConnected())
								{
									if (!fWasConnected)
										fClient->Connect();
								}

								~Connector()
								{
									if (!fWasConnected)
										fClient->Disconnect();
								}

		private:
			CDDBQuery*			fClient;
			bool				fWasConnected;
	};

		bool				fLog;

		// connection description
		BString				fServerName;
		BNetEndpoint		fSocket;
		int32				fPort;
		bool				fConnected;

		// disc identification
		int32				fDiscID;
		int32				fDiscLength;
		int32				fNumTracks;
		BString				fFrameOffsetString;
		BString				fDiscIDStr;

		// target message
		BMessage*			fMessage;
		BHandler*			fTarget;

		// cached retrieved data
		enum State
		{
			eInitial,
			eReading,
			eDone,
			eInterrupting,
			eError
		};

		thread_id			fThread;
		State				fState;
		BString				fTitle;
		std::
		vector<BString>		fTrackNames;
		status_t			fResult;

		friend class		Connector;
};
#endif
