//
// MediaFileDataSource.h
//
//   A CDDataSource that uses a BMediaFile as its underlying source.  Any file
//   that can be read with BMediaFile that has at least one audio track can
//   be adapted to a CDDataSource by this class.  The first audio track found
//   is the one used to supply the audio data.  A resampler is used on
//   any audio source that is not "stereo 16 bit 44100 Hz" format, which is
//   the expected output format of a CDDataSource.
//
//   by Nathan Schrenk (nschrenk@be.com)
//

#ifndef _MEDIA_FILE_DATA_SOURCE_H_
#define _MEDIA_FILE_DATA_SOURCE_H_

#include <MediaFile.h>
#include <Path.h>
#include "CDDataSource.h"

class BEntry;
class _resampler_base;

class MediaFileDataSource : public CDDataSource
{
public:
						MediaFileDataSource(entry_ref *ref);
						MediaFileDataSource(BEntry *entry);
						MediaFileDataSource(BMessage *archive);
						
	virtual				~MediaFileDataSource();
	
	virtual status_t	InitCheck();
	virtual status_t	Read(void *data, size_t len, off_t posn);
	virtual size_t		Length(void);
	virtual bool		IsAudio();
	virtual char 		*Description();

	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
private:
	void				Init(BEntry *entry);
	void				ConfigureResampler();
	void				EndianSwap(void *data, size_t len);
	
	BPath				fPath;			// absolute path to the media file
	media_format		fFormat;
	_resampler_base		*fResampler;	// resampler to use; can be NULL
	BMediaFile			*fFile;
	BMediaTrack			*fTrack;
	uchar				*fBuffer;		// used for buffering reads from MediaTrack
	int32				fBufferSize;
	int32				fLeftOver;		// indicates how many bytes in the buffer are
										// left over from the last read
	float				fResamplingRoundOff;	// keeps track of the round off error
												// when resampling
	int64				fCurrFrame;
	status_t			fInitCode;
	uint32				fSampleBytes;	// bytes in one sample
	bool				fByteSwap;		// whether or not to byteswap samples
};

#endif // _MEDIA_FILE_DATA_SOURCE_H_
