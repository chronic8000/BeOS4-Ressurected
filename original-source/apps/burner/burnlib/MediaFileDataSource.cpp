//
// MediaFileDataSource.cpp
//
//   See MediaFileDataSource.h
//
//   by Nathan Schrenk (nschrenk@be.com)
//

#include <Debug.h>
#include <Entry.h>
#include <media/MediaDefs.h>
#include <MediaTrack.h>
#include <String.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "MediaFileDataSource.h"

//#include <resampler_sinc.h>			// from media_p
#include <resampler_cubic.h>		// from media_p
#define resampler resampler_cubic


static const int32 kBytesPerOutputFrame		= 4;
static const float kOutputFrameRate 		= 44100.0f;

MediaFileDataSource::MediaFileDataSource(entry_ref *ref)
{
	BEntry entry(ref);
	Init(&entry);
}

MediaFileDataSource::MediaFileDataSource(BEntry *entry)
{
	Init(entry);
}

MediaFileDataSource::MediaFileDataSource(BMessage *archive)
{
	BEntry entry;
	const char *archivePath = archive->FindString("path");
	const char *projectPath = archive->FindString("project_path");
	if (archivePath && projectPath) {
		BString path;
		// Convert relative paths to absolute...
		if(*archivePath != '/')
			path << projectPath << "/" << archivePath;
		else
			path << archivePath;
				
		entry.SetTo(path.String(), true);
		Init(&entry);
	}
}

MediaFileDataSource::~MediaFileDataSource()
{
	if (fInitCode == B_OK) {
		fFile->ReleaseTrack(fTrack);
	}
	delete fFile;
	delete fResampler;
	if (fBuffer != NULL) {
		free(fBuffer);
//		area_id id = area_for(fBuffer);
//		delete_area(id);
	}
}

BArchivable *MediaFileDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "MediaFileDataSource")) {
		return new MediaFileDataSource(archive);
	} else {
		return NULL;
	}
}

status_t MediaFileDataSource::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString("class", "MediaFileDataSource");
	
	const char *projectPath = NULL;
	archive->FindString("project_path", &projectPath);
	
	BString trackPath(fPath.Path());
	
	if(trackPath.Compare(projectPath, strlen(projectPath)) == 0) {
		// Store as relative
		trackPath.RemoveFirst(projectPath);
		// Strip any leading slashes...
		if(trackPath.ByteAt(0) == '/')
			trackPath.RemoveFirst("/");

		archive->AddString("path", trackPath.String());
	} else {
		// Store as absolute
		archive->AddString("path", fPath.Path());
	}
	return B_OK;
}


void MediaFileDataSource::Init(BEntry *entry)
{
	fResampler = NULL;
	fBuffer = NULL;
	fFile = NULL;
	fResamplingRoundOff = 0.0f;

	if (!entry->Exists()) {
		fInitCode = B_ERROR;
		return;
	}
	
	entry->GetPath(&fPath);
	entry_ref ref;
	entry->GetRef(&ref);

	fFile = new BMediaFile(&ref, B_MEDIA_FILE_NO_READ_AHEAD);
	if ((fInitCode = fFile->InitCheck()) == B_OK) {
		// init ok, look for an audio track
		int32 tracks = fFile->CountTracks();
		bool foundTrack = false;

		for (int32 i = 0; i < tracks; i++) {
//			printf("Looking at track %ld\n", i);
			fTrack = fFile->TrackAt(i);
			status_t code = fTrack->EncodedFormat(&fFormat);
			if (code != B_OK || !fFormat.IsAudio()) {
				fFile->ReleaseTrack(fTrack);
				continue;
			}
			// set the format to raw, and set params to wildcards
			fFormat.type = B_MEDIA_RAW_AUDIO;
			memset(&(fFormat.u), sizeof(fFormat.u), 0);
			fFormat.u.raw_audio.buffer_size = 4096;
			
			code = fTrack->DecodedFormat(&fFormat);
			if (code == B_OK && fFormat.IsAudio()) {
				fByteSwap = (fFormat.u.raw_audio.byte_order != B_MEDIA_LITTLE_ENDIAN);
				fBufferSize = fFormat.u.raw_audio.buffer_size;
				fLeftOver = 0;
				fCurrFrame = 0;
				fBuffer = (uchar *)malloc(fBufferSize);
//				fBuffer = (uchar *)0xc0080000;
//				int32 areaSize = (fBufferSize % B_PAGE_SIZE == 0) ? fBufferSize : ((fBufferSize / B_PAGE_SIZE) + 1) * B_PAGE_SIZE;
//				create_area("mfdatasource buffer area", (void **)&fBuffer, B_EXACT_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

				ConfigureResampler();
				foundTrack = true;
				break;
			} else {
				fFile->ReleaseTrack(fTrack); // release track that we can't use			
			}
		}
		if (!foundTrack) {
			delete fFile;
			fFile = NULL;
			fInitCode = -1;
		}
	} else {
		delete fFile;
		fFile = NULL;
	}
}

void MediaFileDataSource::ConfigureResampler()
{
	if (fResampler) {
		delete fResampler;
		fResampler = NULL;
	}
	
//	printf("Adding MediaFile [type = %s, rate = %f, channels = %ld, sample format = %ld, buffersize = %ld]\n",
//			fFormat.type == B_MEDIA_RAW_AUDIO ? "raw" : "not raw", fFormat.u.raw_audio.frame_rate, fFormat.u.raw_audio.channel_count,
//			fFormat.u.raw_audio.format, fFormat.u.raw_audio.buffer_size);
	
	if (fFormat.u.raw_audio.frame_rate == kOutputFrameRate
		&& fFormat.u.raw_audio.channel_count == 2
		&& fFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT)
	{
		// no conversion/resampling necessary
		fSampleBytes = 2;
	} else {
		// must resample
		switch (fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			fSampleBytes = 1;
			fResampler = new resampler<uint8, int16>
				(	fFormat.u.raw_audio.frame_rate,		// freq in
					kOutputFrameRate, 					// freq. out
					fFormat.u.raw_audio.channel_count	// channels
				);	
			break;
//		case media_raw_audio_format::B_AUDIO_CHAR:		
//			fSampleBytes = 1;
//			fResampler = new resampler<int8, int16>
//				(	fFormat.u.raw_audio.frame_rate,		// freq in
//					kOutputFrameRate, 					// freq. out
//					fFormat.u.raw_audio.channel_count	// channels
//				);
//			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			fSampleBytes = 2;
			fResampler = new resampler<int16, int16>
				(	fFormat.u.raw_audio.frame_rate,		// freq in
					kOutputFrameRate, 					// freq. out
					fFormat.u.raw_audio.channel_count	// channels
				);
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			fSampleBytes = 4;
			fResampler = new resampler<float, int16>
				(	fFormat.u.raw_audio.frame_rate,		// freq in
					kOutputFrameRate, 					// freq. out
					fFormat.u.raw_audio.channel_count	// channels
				);	
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			fSampleBytes = 4;
			fResampler = new resampler<int32, int16>
				(	fFormat.u.raw_audio.frame_rate,		// freq in
					kOutputFrameRate, 					// freq. out
					fFormat.u.raw_audio.channel_count	// channels
				);	
			break;
		default:
			ASSERT(!"Unhandled case encountered in MediaFileDataSource::ConfigureResampler()!");
			break;
		}
	}
}

status_t MediaFileDataSource::InitCheck()
{
	return fInitCode;
}

// An inefficient way to read (and resample, and do endian conversion) out of a BMediaTrack.
// The CDDataSource API requires exact byte-offset random reads, while the BMediaTrack API doesn't
// guarantee random seeking in this fashion, so there is a lot of inefficiency involved.  We could
// buffer reads in a smart way in the datasource, but currently we don't.

status_t MediaFileDataSource::Read(void *data, size_t len, off_t posn)
{
	int32 bytes_per_frame = fFormat.u.raw_audio.channel_count * fSampleBytes;
	float rate_conversion = kOutputFrameRate / (float)fFormat.u.raw_audio.frame_rate;
	int64 maxPosn = (int64)(floor(fTrack->CountFrames() * rate_conversion) * kBytesPerOutputFrame) - 1;	

	if (posn > maxPosn) {
		return B_ERROR;
	} else if (posn + len > maxPosn) {
		// attempt to read past end, so bump it down
		len = maxPosn - posn;
	}
		
	int64 frame_posn = (int64)(posn / (rate_conversion * kBytesPerOutputFrame));
	uint32 framesFilled = 0;
	uint32 maxFrames = len / kBytesPerOutputFrame; // at most maxFrames frames can be read
	int64 framesRead;
	status_t result;
	
//	printf("fCurrFrame [%lld] - (frame_posn [%lld] + (fLeftOver [%ld] / [%ld])) = %Ld, total frames = %Ld\n",
//		fCurrFrame, frame_posn, fLeftOver, bytes_per_frame,  fCurrFrame - (frame_posn + (fLeftOver / bytes_per_frame)),
//		fTrack->CountFrames());

	if (abs(fCurrFrame - (frame_posn + (fLeftOver / bytes_per_frame))) > 1) {
		// seek to proper position since we're not there
		int64 seek_frame = frame_posn;
		result = fTrack->SeekToFrame(&seek_frame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (result != B_OK) {
			printf("MediaFileDataSource::Read() - SeekToFrame failed with error '%s'\n", strerror(result));
		}
//		printf("MediaFileDataSource::Read() - seek to %lld from %lld ended up at", frame_posn, fCurrFrame);
//		printf(" %lld... (thinks its %lld)\n", seek_frame, fTrack->CurrentFrame());
		fCurrFrame = seek_frame;
		fLeftOver = 0;
		fResamplingRoundOff = 0.0f;
//		int32 bogusReads = 0;
//		printf("\tfBufferSize = %ld\n", fBufferSize);
		while ((fCurrFrame + (fBufferSize / bytes_per_frame)) < frame_posn) {
//			bogusReads++;
//			printf(".");
			result = fTrack->ReadFrames(fBuffer, &framesRead);
			if (result != B_OK) {
				printf("MediaFileDataSource::Read() - ReadFrames() failed with error '%s' when trying to seek\n", strerror(result));
				break;
			} else if (framesRead <= 0) {
				printf("MediaFileDataSource::Read() - ReadFrames() returned B_OK, but did not fill any frames!\n");
				memset(fBuffer, 0, fBufferSize);
				break;				
			}
			fCurrFrame += framesRead;
		}
//		if (bogusReads > 0) { printf("\tperformed %ld \"unnecessary\" reads while seeking\n", bogusReads); }
		
//		printf("\tfCurrFrame = %lld, frame_posn = %lld\n", fCurrFrame, frame_posn);
		if (fCurrFrame < frame_posn) {
//			printf("\tfCurrFrame < frame_posn\n");
			int64 offset = (frame_posn - fCurrFrame) * bytes_per_frame;
			result = fTrack->ReadFrames(fBuffer, &framesRead);
			if (result != B_OK) {
				printf("MediaFileDataSource::Read() - ReadFrames() failed with error '%s'\n", strerror(result));
			} else if (framesRead <= 0) {
				printf("MediaFileDataSource::Read() - ReadFrames() returned B_OK, but did not fill any frames!\n");
				memset(fBuffer, 0, fBufferSize);			
			}
			if (framesRead > 0) {
				fCurrFrame += framesRead;			
				int32 goodBytes = (framesRead * bytes_per_frame) - offset;
				// copy left over data to beginning of buffer
//				printf("\tframesRead = %Ld, memmove(%p, %p + %Ld, %ld), buffer size = %ld\n", framesRead, fBuffer, fBuffer, offset, goodBytes, fBufferSize);
				memmove(fBuffer, fBuffer + offset, goodBytes);
				fLeftOver = goodBytes;
			} else {
				// we tried to read a partial buffer, but we failed!
				fLeftOver = 0;
			}
		} else if (fCurrFrame > frame_posn) {
//			printf("\tfCurrFrame > frame_posn\n");
			int32 neededBytes = (fCurrFrame - frame_posn) * bytes_per_frame;
			int32 offset = fBufferSize - neededBytes;
			// copy needed data to beginning of buffer
			memmove(fBuffer, fBuffer + offset, neededBytes);
			fLeftOver = neededBytes;
		}
	}

	size_t fill_len;
	size_t filled;
	int32 leftOverOffset = 0;
	
//	printf("Before loop - framesFilled = %ld, maxFrames = %ld\n", framesFilled, maxFrames);
	while (framesFilled < maxFrames) {
		if (fLeftOver > 0) {
			// printf("Using %ld bytes of left over data\n", fLeftOver);
			framesRead = fLeftOver / bytes_per_frame;
			fLeftOver = 0;
		} else {
			result = fTrack->ReadFrames(fBuffer, &framesRead);
			// ReadFrames will return an error (B_LAST_BUFFER_ERROR or for some codecs 
			// B_ERROR) but still read frames. This loop doesn't want to see an error 
			// until there are no frames read. 
			if (result != B_OK && framesRead > 0)
				result = B_OK;
			if (result != B_OK) {
				// read error, but don't announce EOF error
				if (result != B_LAST_BUFFER_ERROR) {
					printf("MediaFileDataSource::Read(): ReadFrames() failed with error '%s', returned %Ld frames\n",
							strerror(result), framesRead);
				}
				break;
			} else if (framesRead == 0) {
				printf("MediaFileDataSource::Read(): ReadFrames() returned B_OK, but did not fill any frames!\n");
				// no more data to read?
				break;
			}
			fCurrFrame += framesRead;
		}
		
		if (fResampler) {
			// samples must be in host byte order before resampling
			if ((B_HOST_IS_LENDIAN && fFormat.u.raw_audio.byte_order == B_MEDIA_BIG_ENDIAN)
				|| (B_HOST_IS_BENDIAN && fFormat.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN))
			{
				EndianSwap((void *)fBuffer, framesRead * fSampleBytes * fFormat.u.raw_audio.channel_count);
			}

			size_t in_frames = (size_t)framesRead;
			size_t out_frames = maxFrames - framesFilled;

			// resample

//			printf("First 20 bytes of input, followed by output: (in hex)\n\t");
//			for (int q = 0; q < 20; q++) {
//				printf("%02x ", fBuffer[q]);
//			}
//			printf("\n\t");
//			printf("Resampling %ld source frames into %ld output frames\n", in_frames, out_frames);			

			const size_t expectedOutFrames = out_frames;			
			const float gain[2] = {1.0f, 1.0f};
			fResampler->resample_gain(
				(void *)(fBuffer),	// input buffer 
				in_frames,	// # frames to read
				(void *)(((char *)data) + (framesFilled * kBytesPerOutputFrame)), // output buffer pointer
				out_frames,	// # frames to write
				gain,		// gain
				false);		// don't mix

			framesFilled += (expectedOutFrames - out_frames);
			fLeftOver = in_frames*kBytesPerOutputFrame;

//			for (int q = 0; q < 20; q++) {
//				printf("%d ", (((int16 *)(data)) + (framesFilled * 2))[q]);
//			}
//			printf("\n");
		} else {
			// no resampling needed, just copy the data
			filled = framesFilled * kBytesPerOutputFrame;
			fill_len = framesRead * kBytesPerOutputFrame;
			if (fill_len > (len - filled)) {
				// don't go past the end of the output buffer
				fLeftOver = filled + fill_len - len; 
				leftOverOffset = (framesRead * kBytesPerOutputFrame) - fLeftOver;
//				printf("straight path: output buffer would overflow... fLeftOver = %ld, offset = %ld\n", fLeftOver, leftOverOffset);				
				fill_len = len - filled;
			} else {
				fLeftOver = 0;
			}
			memcpy(((char *)data) + filled, fBuffer, fill_len);
			framesFilled += (fill_len / kBytesPerOutputFrame);
		}
		
		if (fLeftOver > 0) {
			// copy left over data to beginning of buffer
			memmove(fBuffer, fBuffer + leftOverOffset, fLeftOver);
		}
//		printf("\tloop - framesFilled = %ld, maxFrames = %ld\n", framesFilled, maxFrames);
	}
	
	filled = framesFilled * kBytesPerOutputFrame;
	if (filled < len) {
		memset(((char *)data) + filled, 0, len - filled); // write zeros into unfilled space
	}
	if ((B_HOST_IS_BENDIAN && fResampler) ||
		(!fResampler && fFormat.u.raw_audio.byte_order == B_MEDIA_BIG_ENDIAN))
	{
		// convert samples to little endian, either because they were converted to big
		// endian for resampling on a big endian host, or because they were originally
		// big endian in the file
		EndianSwap((void *)data, filled);
	}

//	if (framesFilled < maxFrames) {
//		printf("*** framesFilled [%ld] is less than maxFrames [%ld]!\n", framesFilled, maxFrames);
//	}
	return B_OK;
}

size_t MediaFileDataSource::Length()
{
	int64 frames = fTrack->CountFrames();
	float frame_conversion = kOutputFrameRate / fFormat.u.raw_audio.frame_rate;
	return (size_t)(frame_conversion * frames * kBytesPerOutputFrame);
}

bool MediaFileDataSource::IsAudio()
{
	return true;
}

char *MediaFileDataSource::Description()
{
	BString desc;
	media_file_format fileFormat;
	if(fFile != NULL) {
		fFile->GetFileFormatInfo(&fileFormat);
		desc << fPath.Leaf() << " (" << fileFormat.short_name << " format)";
		return strdup(desc.String());
	}
	return 0;
}

// Swaps the endianness of the len amount of sample data in the buffer, taking into account
// the sample format of this MediaFile 
void MediaFileDataSource::EndianSwap(void *data, size_t len)
{
	if (fSampleBytes < 2) {
		return;
	}
	
	int32 samplesToSwap = len / fSampleBytes;
	switch (fFormat.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_SHORT:
		{
			int16 *x_int16 = (int16 *)data;
			while (samplesToSwap > 0) {
				B_SWAP_INT16(*x_int16);
				x_int16++;
				samplesToSwap--;
			}
		}
		break;				
	case media_raw_audio_format::B_AUDIO_FLOAT:
		{
			float *x_float = (float *)data;
			while (samplesToSwap > 0) {
				B_SWAP_FLOAT(*x_float);
				x_float++;
				samplesToSwap--;
			}
		}
		break;
	case media_raw_audio_format::B_AUDIO_INT:
		{
			int32 *x_int32 = (int32 *)data;
			while (samplesToSwap > 0) {
				B_SWAP_INT32(*x_int32);
				x_int32++;
				samplesToSwap--;
			}
		}
	}
}

