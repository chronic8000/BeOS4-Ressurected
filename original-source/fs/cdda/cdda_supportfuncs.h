
#ifndef _CDDA_SUPPORTFUNCS_H
#define _CDDA_SUPPORTFUNCS_H

#include <ByteOrder.h>
#include "iso.h"

#ifdef __cplusplus
extern "C" {
#endif
void LockFS(nspace *ns);
void UnlockFS(nspace *ns);
int IndexForFilename(nspace *ns, const char *filename);
const char *GetTrackName(nspace *ns, long tracknumber);
void GetCDDANames(nspace *ns, bool dorequest);
void DumpCDDANames(nspace *ns);
char *SanitizeString(char *ss);

#ifdef __cplusplus
}
#endif

class WAVHeader
{
	long riffheader;
	long length;
	long subtype;
	long fmtchunk;
	long fmtlength;
	int16 formattag;
	int16 numchannels;
	long samplespersec;
	long avgbytespersec;
	int16 blockalign;
	int16 bitspersample;
	long datachunk;
	long datalength;
public:
	WAVHeader(long dl);
};

const long WAV_HEADER_SIZE=sizeof(WAVHeader);

#endif

