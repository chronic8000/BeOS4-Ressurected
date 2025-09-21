/*------------------------------------------------------------*/
//
//	File:		CDDBSupport.cpp
//
//	based on Jukebox by Chip Paul
//
//	Copyright 2000, Be Incorporated
//
/*------------------------------------------------------------*/

#include <errno.h>
#include <fs_index.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NetAddress.h>
#include <Path.h>
#include <SupportDefs.h>

#include "CDDBSupport.h"
#include "cd.h"


const int kTerminatingSignal = SIGINT; // SIGCONT;

struct ConvertedToc
{
	int32 min;
	int32 sec;
	int32 frame;
};

static int32 cddb_sum(int n)
{
	char	buf[12];
	int32	ret = 0;
	
	sprintf(buf, "%u", n);
	for (const char *p = buf; *p != '\0'; p++)
		ret += (*p - '0');
	return ret;
}

static void DoNothing(int)
{
}


/*=================================================================*/

template <class InitCheckable>

void InitCheck(InitCheckable* item)
{
	if (!item)
		throw B_ERROR;
	status_t error = item->InitCheck();
	if (error != B_OK)
		throw error;
}

/*-----------------------------------------------------------------*/

inline void ThrowOnError(status_t error)
{
	if (error != B_OK)
		throw error; 
}

/*-----------------------------------------------------------------*/

inline void ThrowIfNotSize(ssize_t size)
{
	if (size < 0)
		throw (status_t)size; 
}


/*=================================================================*/

CDDBQuery::CDDBQuery(const char *server, int32 port, bool log)
	: fLog(log),
	  fServerName(server),
	  fPort(port),
	  fConnected(false),
	  fDiscID(-1),
	  fState(eInitial)
{
}

/*-----------------------------------------------------------------*/

int32  CDDBQuery::GetDiscID(const scsi_toc* toc)
{
	int32	tmpDiscID;
	int32	tmpDiscLength;
	int32	tmpNumTracks;
	BString	tmpFrameOffsetString;
	BString	tmpDiscIDStr;
	
	GetDiscID(toc, tmpDiscID, tmpNumTracks, tmpDiscLength, tmpFrameOffsetString,
		tmpDiscIDStr);
	
	return tmpDiscID;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::GetSites(bool (*eachFunc)(const char* site, int port, const char* latitude,
						 const char* longitude, const char* description, void* state),
						 void* passThru)
{
	// ToDo:
	// add interrupting here

	Connector connection(this);
	BString tmp;
	
	tmp = "sites\n";
	if (fLog)
		printf(">%s", tmp.String());

	ThrowIfNotSize(fSocket.Send(tmp.String(), tmp.Length()));
	ReadLine(tmp);

	if (tmp.FindFirst("210") == -1)
		throw (status_t)B_ERROR;

	for (;;)
	{
		int32	sitePort;
		BString	site;
		BString latitude;
		BString longitude;
		BString description;

		ReadLine(tmp);
		if (tmp == ".")
			break;
		const char *scanner = tmp.String();

		scanner = GetToken(scanner, site);
		BString portString;
		scanner = GetToken(scanner, portString);
		sitePort = atoi(portString.String());
		scanner = GetToken(scanner, latitude);
		scanner = GetToken(scanner, longitude);
		description = scanner;
		
		if (eachFunc(site.String(), sitePort, latitude.String(), longitude.String(),
			description.String(), passThru))
			break;
	}
}

/*-----------------------------------------------------------------*/

void CDDBQuery::GetTitles(scsi_toc* toc, BMessage* msg, BHandler* target)
{
	fMessage = msg;
	fTarget = target;

	GetDiscID(toc, fDiscID, fNumTracks, fDiscLength, fFrameOffsetString, fDiscIDStr);

	// ToDo:
	// add interrupting here

	fResult = B_OK;
	fState = eReading;
	fThread = spawn_thread(&CDDBQuery::LookupBinder, "CDDBLookup", B_NORMAL_PRIORITY, this);
	if (fThread >= 0)
		resume_thread(fThread);
	else
	{
		fState = eError;
		fResult = fThread;
	}
}

/*-----------------------------------------------------------------*/

void  CDDBQuery::SetToSite(const char *server, int32 port)
{
	Disconnect();
	fServerName = server;
	fPort = port;
	fState = eInitial;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::Connect()
{
	BNetAddress*	address = NULL;

	address = new BNetAddress(fServerName.String(), fPort);
	if ((address) && (address->InitCheck() == B_NO_ERROR))
	{
		ThrowOnError(fSocket.Connect(*address));
		fConnected = true;

		BString	tmp;
		ReadLine(tmp);
		IdentifySelf();
		delete address;
	}
}

/*-----------------------------------------------------------------*/

void CDDBQuery::CreateTmpFile(BFile* file, entry_ref* ref)
{
	BPath			path;

	ThrowOnError(find_directory(B_COMMON_TEMP_DIRECTORY, &path, true));
	
	BDirectory	dir(path.Path());
	BString		name("CDContent");
	for (int32 index = 0; ;index++)
	{
		if (dir.CreateFile(name.String(), file, true) != B_FILE_EXISTS)
		{
			BEntry	entry(&dir, name.String());

			entry.GetRef(ref);
			break;
		}
		name = "CDContent";
		name << index;
	}
}

/*-----------------------------------------------------------------*/

void CDDBQuery::Disconnect()
{
	fSocket.Close();
	fConnected = false;
}

/*-----------------------------------------------------------------*/

void  CDDBQuery::GetDiscID(const scsi_toc* toc, int32 &id, int32 &numTracks,
						   int32 &length, BString &frameOffsetsString,
						   BString &discIDString)
{
	ConvertedToc	tocData[100];

	// figure out the disc ID
	for (int index = 0; index < 100; index++)
	{
		tocData[index].min = toc->toc_data[9 + 8*index];
		tocData[index].sec = toc->toc_data[10 + 8*index];
		tocData[index].frame = toc->toc_data[11 + 8*index];
	}
	numTracks = toc->toc_data[3] - toc->toc_data[2] + 1;

	int32 sum1 = 0;
	int32 sum2 = 0;
	for (int index = 0; index < numTracks; index++)
	{
		sum1 += cddb_sum((tocData[index].min * 60) + tocData[index].sec);
		// the following is probably running over too far
		sum2 +=	(tocData[index + 1].min * 60 + tocData[index + 1].sec) -
			(tocData[index].min * 60 + tocData[index].sec);
	}
	id = ((sum1 % 0xff) << 24) + (sum2 << 8) + numTracks;
	discIDString = "";

	sprintf(discIDString.LockBuffer(10), "%08lx", id);
	discIDString.UnlockBuffer();

	// compute the total length of the CD.
	length = tocData[numTracks].min * 60 + tocData[numTracks].sec;

	frameOffsetsString = "";
	for (int index = 0; index < numTracks; index++) 
		frameOffsetsString << tocData[index].min * 4500 + tocData[index].sec * 75
			+ tocData[index].frame << ' ';
}

/*-----------------------------------------------------------------*/

const char* CDDBQuery::GetToken(const char* stream, BString& result)
{
	result = "";
	while ((*stream) && (*stream <= ' '))
		stream++;
	while ((*stream) && (*stream > ' '))
		result += *stream++;

	return stream;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::IdentifySelf()
{
	char	hostname[MAXHOSTNAMELEN + 1];
	char	username[256];
	BString	tmp;

	//if (!getusername(username,256))
		strcpy(username, "unknown");

	//if (gethostname(hostname, MAXHOSTNAMELEN) == -1)
		strcpy(hostname, "unknown");

	tmp << "cddb hello " << username << " " << hostname << " CDButton v1.0\n";

	if (fLog)
		printf(">%s", tmp.String());

	ThrowIfNotSize(fSocket.Send(tmp.String(), tmp.Length()));
	ReadLine(tmp);
}

/*-----------------------------------------------------------------*/

bool CDDBQuery::IsConnected() const
{
	return fConnected;
}

/*-----------------------------------------------------------------*/

int32 CDDBQuery::LookupBinder(void* castToThis)
{
	CDDBQuery*	query = (CDDBQuery*)castToThis;
	char*		offset;
	char*		track_list;
	BFile		file;
	BMessage*	msg = query->fMessage;
	BHandler*	target = query->fTarget;
	entry_ref	ref;

	try
	{
		signal(kTerminatingSignal, DoNothing);

		query->CreateTmpFile(&file, &ref);
		Connector	connection(query);

		query->ReadFromServer(&file);
		file.Seek(0, SEEK_SET);

		query->ParseResult(&file);
		query->fState = eDone;
		query->fResult = B_OK;

		BString	newTitle(query->fTitle);
		int32	length = newTitle.Length();
		for (int32 index = 0; index < length; index++)
		{
			if (newTitle[index] == '/' || newTitle[index] == ':')
				newTitle[index] = '-';
		}

		file.Sync();
		BEntry entry(&ref);
		entry.Remove();

		int32 len = strlen(query->fTitle.String()) + 1;
		for (std::vector<BString>::iterator iterator = query->fTrackNames.begin();
			iterator != query->fTrackNames.end(); iterator++)
			len += strlen((*iterator).String()) + 1;
		len++;

		track_list = (char*)malloc(len);
		sprintf(track_list, "%s\n", query->fTitle.String());
		offset = track_list + strlen(query->fTitle.String()) + 1;

		for (std::vector<BString>::iterator iterator = query->fTrackNames.begin();
			iterator != query->fTrackNames.end(); iterator++)
		{
			sprintf(offset, "%s\n", (*iterator).String());
			offset += strlen((*iterator).String()) + 1;
		}
		track_list[len - 1] = 0;
		msg->AddString(kCD_DAEMON_PLAYER_TRACK_LIST, track_list);
		be_app->PostMessage(msg, target);
		free(track_list);
	}
	catch (status_t error)
	{
		query->fState = eError;
		query->fResult = error;

		// the cached file is messed up, remove it
		BEntry	entry(&ref);
		BPath	path;

		entry.GetPath(&path);
		//printf("removing bad CD content file %s\n", path.Path());
		entry.Remove();
	}
	query->fThread = -1;
	delete msg;
	return 0;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::ParseResult(BDataIO* source)
{
	fTitle = "";
	fTrackNames.clear();

	for (;;)
	{
		BString	tmp;

		ReadLine(source, tmp);
		if (tmp == ".")
			break;
			
		if (tmp == "")
			throw (status_t)B_ERROR;
		
		if (tmp.FindFirst("DTITLE=") == 0)
			fTitle = tmp.String() + sizeof("DTITLE");
		else if (tmp.FindFirst("TTITLE") == 0)
		{
			int32	afterIndex = tmp.FindFirst('=');
			if (afterIndex > 0) {
				BString	trackName(tmp.String() + afterIndex + 1);
				fTrackNames.push_back(trackName);
			}
		}
	}
	fState = eDone;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::ReadFromServer(BDataIO* stream)
{
	// Format the query
	BString	query;
	query << "cddb query " << fDiscIDStr << ' ' << fNumTracks << ' ' 
		// Add frame offsets
		<< fFrameOffsetString << ' '
		// Finish it off with the total CD length.
		<< fDiscLength << '\n';

	if (fLog)
		printf(">%s", query.String());

	// Send it off.
	ThrowIfNotSize(fSocket.Send(query.String(), query.Length()));

	BString	tmp;
	ReadLine(tmp);
	if (tmp.FindFirst("200") != 0)
		return;

	BString	category;
	GetToken(tmp.String() + 3, category);
	if (!category.Length())
		category = "misc";

	query = "";
	query << "cddb read " << category << ' ' << fDiscIDStr << '\n' ;
	ThrowIfNotSize(fSocket.Send(query.String(), query.Length()));

	for (;;)
	{
		BString tmp;
		ReadLine(tmp);
		tmp += '\n';
		ThrowIfNotSize( stream->Write(tmp.String(), tmp.Length()) );
		if (tmp == "." || tmp == ".\n")
			break;
	}
	fState = eDone;
}

/*-----------------------------------------------------------------*/

void CDDBQuery::ReadLine(BString& buffer)
{
	buffer = "";
	char ch;
	for (;;)
	{
		if (fSocket.Receive(&ch, 1) <= 0)
			break;
		if (ch >= ' ')
			buffer += ch;
		if (ch == '\n')
			break;
	}
	if (fLog)
		printf("<%s\n", buffer.String());
}

/*-----------------------------------------------------------------*/

void CDDBQuery::ReadLine(BDataIO* stream, BString& buffer)
{
	// this is super-lame, should use better buffering
	// ToDo:
	// redo using LockBuffer, growing the buffer by 2k and reading into it

	buffer = "";
	char ch;
	for (;;)
	{
		ssize_t result = stream->Read(&ch, 1);

		if (result < 0)
			// read error
			throw (status_t)result;
		if (result == 0)
			break;
		if (ch >= ' ')
			buffer += ch;
		if (ch == '\n')
			break;
	}
}
