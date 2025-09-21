#include <Application.h>
#include <View.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Bitmap.h>
#include <stdio.h>
#include <Autolock.h>
#include <stdlib.h>
#include <unistd.h>

BMediaFile *fMediaFile;
BMediaTrack *fAudioTrack;
void *fBuffer;
int32 fFrameRate;
thread_id fPlayerThread;
bool fStopFlag;
int32 fBufferSize;
int		outf;

status_t Open(entry_ref *ref)
{
	fAudioTrack = 0;
	fMediaFile = new BMediaFile(ref);
	if (fMediaFile->InitCheck() < B_OK) {
		printf("can't find handler for this file\n");
		return fMediaFile->InitCheck();
	}

	// Loop through tracks, looking for video.		
	media_format format;
	for (int32 index = 0; index < fMediaFile->CountTracks(); index++) {
		printf("Track %d\n", index);
		
		// Get information about the encoded format.
		status_t err = fMediaFile->TrackAt(index)->EncodedFormat(&format);
		if (err != B_OK) {
			printf("Couldn't get track format\n");
			delete fMediaFile;
			fMediaFile = 0;
			return err;
		}
		
		if (format.type == B_MEDIA_ENCODED_AUDIO || format.type == B_MEDIA_RAW_AUDIO) {
			fFrameRate = (int32) format.u.raw_video.field_rate;
			fAudioTrack = fMediaFile->TrackAt(index);

			// Fill in the rest of the format data for the decoder.
			format.u.raw_audio.frame_rate = 44100;
			format.u.raw_audio.channel_count = 2;
			format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
			format.u.raw_audio.buffer_size = 0x1000;
			format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;

			// Pass the requested format to the decoder.
			err = fAudioTrack->DecodedFormat(&format);
		
			fBufferSize = format.u.raw_audio.buffer_size;
			fBuffer = malloc(fBufferSize);
			if (err != B_OK) {
				printf("Couldn't get decoder for this track\n");
				fAudioTrack = 0;
				delete fMediaFile;
				fMediaFile = 0;
				return err;
			}

			return B_OK;
		}
	}

	return B_ERROR;
}

int32 PlayThread(void*)
{
	while (!fStopFlag) {
		media_header header;
		int64 numReadFrames = 1;
		if (fAudioTrack->ReadFrames((char*) fBuffer, &numReadFrames, &header) != B_OK)
			break;
		if(numReadFrames == 0)
			break;
		
		int tw = fBufferSize;
	again:
		int ww = write(outf, (char *)fBuffer+fBufferSize-tw, tw);
		if (ww < 1) break;
		tw -= ww;
		if (tw > 0) goto again;
	}
}

void Play()
{
	fStopFlag = false;
	fPlayerThread = spawn_thread(PlayThread, "player_thread", B_URGENT_PRIORITY, 0);
	resume_thread(fPlayerThread);
}

void Stop()
{
	fStopFlag = true;
	int32 status;
	wait_for_thread(fPlayerThread, &status);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: player soundfile [outputfile]\n");
		return 1;
	}

	entry_ref ref;
	if (get_ref_for_path(argv[1], &ref) != B_OK) {
		printf("invalid filename\n");
		return 1;
	}

	if(argv[2])	{
		outf = open(argv[2], O_WRONLY);
		if(outf == -1) {
			printf("couldn't open file %s\n", argv[2]);
			return 1;
		}
	}
	else
		outf = fileno(stdout);

	BApplication app("application/x-vnd.Be.Application");
	if (Open(&ref) != B_OK) {
		printf("couldn't open file %s\n", argv[1]);
		return 1;
	}
	
	Play();
	app.Run();
	
	close(outf);
	return 0;
}
