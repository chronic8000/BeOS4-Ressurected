
#include <stdlib.h>
#include <stdio.h>
#include <game_audio.h>
#include <errno.h>
#include <algorithm>

#include "gamedev.h"
#include "miniautolock.h"
#include "mixfunc.h"
#include "minimix.h"
#include "minibinder.h"

using namespace std;

int g_gameBufCnt = 2;

static const char * SETTINGS_FILE_NAME = "/boot/home/config/settings/game_audio_setup.bin";

// ------------------------------------------------------------------------ //

class game_dev::cloned_buffer_set
{
public:
	cloned_buffer_set(
		game_dev& _dev,
		int32 _channel_id,
		int32 _buffer_count,
		bool _dev_owned=true) :
		dev(_dev),
		channel_id(_channel_id),
		buffer_count(_buffer_count),
		dev_owned(_dev_owned),
		next_buffer_index(0),
		buffers(0),
		next(0)
	{
		buffers = new cloned_buffer[buffer_count];
		for (int32 n = 0; n < buffer_count; n++)
			buffers[n].id = GAME_NO_ID;		
	}

	~cloned_buffer_set()
	{
		for (int32 n = 0; n < buffer_count; n++)
		{
			if (buffers[n].id == GAME_NO_ID) continue;

			if (dev_owned)
			{
				// ask driver to clean up buffer
#ifdef GAME_CURRENT_API_VERSION
				H<game_close_stream_buffer> gcsb;
				gcsb.buffer = buffers[n].id;
				G<game_close_stream_buffers> gcsbs;
				gcsbs.in_request_count = 1;
				gcsbs.buffers = &gcsb;
				if (ioctl(dev.m_fd, GAME_CLOSE_STREAM_BUFFERS, &gcsbs) < 0)
				{
					perror("error: ioctl(GAME_CLOSE_STREAM_BUFFERS)");
				}
#else
				G<game_close_stream_buffer> gcsb;
				gcsb.buffer = buffers[n].id;
				if (ioctl(dev.m_fd, GAME_CLOSE_STREAM_BUFFER, &gcsb) < 0)
				{
					perror("error: ioctl(GAME_CLOSE_STREAM_BUFFER)");
				}
#endif
			}
			// account for other chunks of this buffer
			int32 id = buffers[n].id;
			for (int32 forward = n; forward < buffer_count; forward++)
			{
				if (buffers[forward].id == id) buffers[forward].id = GAME_NO_ID;
			}
				
			// clean up locally allocated/cloned area
			area_id clone = buffers[n].local_area;
			if (clone)
			{
				for (int32 forward = n; forward < buffer_count; forward++)
				{
					if (buffers[forward].local_area == clone)
						buffers[forward].local_area = 0;
				}
				delete_area(clone);
			}
		}
		delete [] buffers;
	}
	
	game_dev&			dev;
	const int32			channel_id;
	const int32			buffer_count;
	
	bool				dev_owned;
	cloned_buffer*		buffers;
	cloned_buffer_set*	next;
	int32				next_buffer_index;

private:
	cloned_buffer_set(const cloned_buffer_set&);
	const cloned_buffer_set& operator=(const cloned_buffer_set& other);
};

// ------------------------------------------------------------------------ //

game_dev::game_dev() :
	m_fd(-1),
	m_dacStreamSem(-1),
	m_dacStreamID(GAME_NO_ID),
	m_dacBufferFrames(0),
	m_mixBufferSets(0),
	m_nextMixBufferID(0),
	m_mixdownBuffer(0),
	m_dacBufferDuration(0LL),
	m_adcStreamSem(-1),
	m_adcStreamID(GAME_NO_ID),
	m_adcBufferFrames(0),
#ifdef GAME_CURRENT_API_VERSION
	m_dacLocalArea(-1),
	m_dacPtr(0),
	m_dacBufferID(GAME_NO_ID),
#endif
	m_captureLock("game_dev::m_captureLock"),
	m_captureThread(-1),
	m_runCaptureThread(false),
	m_nextCaptureChannelID(0),
	m_nextBufferIndex(0),
	m_captureBufferSet(0),
	m_captureChannels(0)
{
#ifndef GAME_CURRENT_API_VERSION
	memset(m_dacAreaInfo, -1, sizeof(m_dacAreaInfo));
	memset(m_dacPtr, -1, sizeof(m_dacPtr));
	memset(m_dacBuffer, -1, sizeof(m_dacBuffer));
#endif
	memset(&m_dacMixerInfo, -1, sizeof(m_dacMixerInfo));
	memset(&m_mainLevel, -1, sizeof(m_mainLevel));
	memset(&m_modemLevel, -1, sizeof(m_modemLevel));

	memset(&m_adcMixerInfo, -1, sizeof(m_adcMixerInfo));
	memset(&m_adcLevel, -1, sizeof(m_adcLevel));
	memset(&m_micBoostEnable, -1, sizeof(m_micBoostEnable));
	memset(&m_adcMux, -1, sizeof(m_adcMux));
	memset(m_adcMuxValues, 0, sizeof(m_adcMuxValues));

	m_serverState.event_gain = 0.5;
	m_serverState.event_mute = false;
	const char * bcstr = find_xarg("SND_BUF_COUNT");
	if (bcstr && (atoi(bcstr) > 1)) {
		g_gameBufCnt = atoi(bcstr);
		if (g_gameBufCnt > MAXGAMEBUFCNT) g_gameBufCnt = MAXGAMEBUFCNT;
		fprintf(stderr, "info: using %d buffers\n", g_gameBufCnt);
	}
}


game_dev::~game_dev()
{
	free_all_dac_buffers();
	if (m_dacStreamSem)
	{
		delete_sem(m_dacStreamSem);

#ifdef GAME_CURRENT_API_VERSION
		if (m_dacLocalArea >= 0) delete_area(m_dacLocalArea);
#else
		for (int ix=0; ix<GAMEBUFCNT; ix++) delete_area(m_dacAreaInfo[ix].clone);
#endif
	}
	stop_adc_stream();
	close_adc_stream();
	close(m_fd);
}


#ifdef GAME_CURRENT_API_VERSION
bigtime_t
game_dev::do_mix()
{
	// cyclic do_mix()

static bool running = false;
static int pre_queue = GAMEBUFCNT;
static int no_cnt = 0;
static int buf_ix = 0;

//	fprintf(stderr, "dm: pre(%ld), no(%ld), buf(%ld)\n", pre_queue, no_cnt, buf_ix);

	status_t err;
	memset(m_mixdownBuffer, 0, m_dacFormat.buffer_size*2); /* int32 instead of int16 */
	bool has = g_mix.Mix(m_mixdownBuffer, m_dacBufferFrames);
	if (!has) {
		if (no_cnt++ > MAX_NO_MIX) {
			// input has run out; stop service for now.
			H<game_run_stream> grs;
			grs.stream = m_dacStreamID;
			grs.state = GAME_STREAM_STOPPED;
			G<game_run_streams> grss;
			grss.in_stream_count = 1;
			grss.streams = &grs;
			if (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) {
				fprintf(stderr, "error: GAME_RUN_STREAMS: %s\n", strerror(errno));
			}
			while (acquire_sem_etc(m_dacStreamSem, 1, B_TIMEOUT, 0LL) == B_OK) {}
//			fprintf(stderr, "*** game/STOP\n",);
			running = false;
			pre_queue = GAMEBUFCNT;
			buf_ix = 0;
			return B_INFINITE_TIMEOUT;
		}
	}
	else no_cnt = 0;

	bigtime_t then = system_time();

	if (pre_queue) --pre_queue;
	else if ((err = acquire_sem_etc(m_dacStreamSem, 1, B_TIMEOUT, SND_TIMEOUT)) < 0) {
		fprintf(stderr, "warning: game_dev::do_mix(): acquire_sem_etc(): %s\n", strerror(err));
		return B_INFINITE_TIMEOUT;
	}
	int16* nextChunk = (int16*)((int8*)m_dacPtr + buf_ix * m_dacFormat.buffer_size);
#if DITHER
	convert_32_16_dither(m_mixdownBuffer, nextChunk, m_dacBufferFrames, m_dithers);
#else
	(*g_convert_func)(m_mixdownBuffer, nextChunk, m_dacBufferFrames);
#endif

	if (!running && !pre_queue) {
		// get things rolling
		G<game_set_stream_buffer> gssb;
		gssb.stream = m_dacStreamID;
		gssb.flags = GAME_BUFFER_PING_PONG;
		gssb.buffer = m_dacBufferID;
		gssb.page_frame_count = m_dacBufferFrames;
		gssb.frame_count = gssb.page_frame_count * GAMEBUFCNT;
		if (ioctl(m_fd, GAME_SET_STREAM_BUFFER, &gssb) < 0) {
			fprintf(stderr, "GAME_SET_STREAM_BUFFER: %s\n", strerror(errno));
		}
		
		H<game_run_stream> grs;
		grs.stream = m_dacStreamID;
		grs.state = GAME_STREAM_RUNNING;
		G<game_run_streams> grss;
		grss.in_stream_count = 1;
		grss.streams = &grs;
		if (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) {
			fprintf(stderr, "GAME_RUN_STREAMS: %s\n", strerror(errno));
		}
//		fprintf(stderr, "*** game/START\n");
		running = true;
	}

	buf_ix = (buf_ix + 1) % GAMEBUFCNT;
	return then+m_dacBufferDuration/2;
}

#else // GAME_CURRENT_API_VERSION

bigtime_t
game_dev::do_mix()
{
	// crusty old queueing do_mix()

static bool first = true;
static bigtime_t last_time = 0;
static int no_cnt = 0;
static bigtime_t that_time = 0;
static int buf_ix = 0;

//	audio_buffer_header * abh = (audio_buffer_header *)o_ptr;
	status_t err;
	memset(m_mixdownBuffer, 0, m_dacFormat.buffer_size * 2); /* int32 instead of int16 */
	bool has = g_mix.Mix(m_mixdownBuffer, m_dacBufferFrames);
	if (!has) {
		if (no_cnt++ > MAX_NO_MIX) {
//	actually, we don't need to stop the stream...
//			G<game_run_streams> grss;
//			H<game_run_stream> grs;
//			grss.in_stream_count = 1;
//			grss.streams = &grs;
//			grs.stream = m_dacStreamID;
//			grs.state = GAME_STREAM_PAUSED;
//			first = true;
//			if (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) {
//				err = errno;
//				fprintf(stderr, "error: ioctl(GAME_RUN_STREAMS pause %d): %s\n", m_dacStreamID, strerror(err));
//				return B_INFINITE_TIMEOUT;
//			}
			return B_INFINITE_TIMEOUT;
		}
	}
	else {
		no_cnt = 0;
	}
	bigtime_t then = system_time();
	game_queue_stream_buffer gqsb;
	if (first) {
		//	prime the driver with an empty buffer to make sure 
		//	it will have enough double-buffering data to take us
		//	to the next buffer.
#if DITHER
		m_dithers[0] = m_dithers[1] = 0;
#endif
		for (int ix=0; ix<GAMEBUFCNT-1; ix++) {
			(void)acquire_sem(m_dacStreamSem);
			memset(m_dacPtr[buf_ix], 0, m_dacFormat.buffer_size);
			last_time = system_time();
			memset(&gqsb, 0, sizeof(gqsb));
			gqsb.info_size = sizeof(gqsb);
			gqsb.stream = m_dacStreamID;
			gqsb.flags = GAME_RELEASE_SEM_WHEN_DONE;
			gqsb.buffer = m_dacBuffer[buf_ix];
			gqsb.frame_count = m_dacBufferFrames;
			(void)ioctl(m_fd, GAME_QUEUE_STREAM_BUFFER, &gqsb);
			that_time = bigtime_t(m_dacBufferDuration*1.25)+2000LL;	//	acceptable jitter
			first = false;
			buf_ix = (buf_ix + 1) % GAMEBUFCNT;
		}
		G<game_run_streams> grss;
		H<game_run_stream> grs;
		grss.in_stream_count = 1;
		grss.streams = &grs;
		grs.stream = m_dacStreamID;
		grs.state = GAME_STREAM_RUNNING;
		if (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) {
			err = errno;
			fprintf(stderr, "error: ioctl(GAME_RUN_STREAMS run %d): %s\n", m_dacStreamID, strerror(err));
			return B_INFINITE_TIMEOUT;
		}
	}
	if ((err = acquire_sem_etc(m_dacStreamSem, 1, B_TIMEOUT, SND_TIMEOUT)) < 0) {
		fprintf(stderr, "warning: waiting for 'game' card: acquire_sem_etc(): %s\n", strerror(err));
		return B_INFINITE_TIMEOUT;
	}
#if DITHER
	convert_32_16_dither(m_mixdownBuffer, (int16 *)m_dacPtr[buf_ix], m_dacBufferFrames, m_dithers);
#else
	(*g_convert_func)(m_mixdownBuffer, (int16 *)m_dacPtr[buf_ix], m_dacBufferFrames);
#endif
	memset(&gqsb, 0, sizeof(gqsb));
	gqsb.info_size = sizeof(gqsb);
	gqsb.stream = m_dacStreamID;
	gqsb.flags = GAME_RELEASE_SEM_WHEN_DONE;
	gqsb.buffer = m_dacBuffer[buf_ix];
	gqsb.frame_count = m_dacBufferFrames;
	if (ioctl(m_fd, GAME_QUEUE_STREAM_BUFFER, &gqsb) < 0) {
		err = errno;
		if (err == B_INTERRUPTED) {
			fprintf(stderr, "warning: ioctl(SOUND_WRITE): B_INTERRUPTED (mostly OK)\n");
		}
		else {
			fprintf(stderr, "warning: ioctl(SOUND_WRITE): %s\n", strerror(err));
		}
	}
	if (then-last_time > that_time) {
		fprintf(stderr, "warning: then-last_time is %Ld\n", then-last_time);
	}
	last_time = then;
	buf_ix = (buf_ix + 1) % GAMEBUFCNT;
	return then+m_dacBufferDuration/2;
}
#endif // GAME_CURRENT_API_VERSION

//	this function is called inside a separate thread to actually save info
status_t
game_dev::pref_save()
{
	acquire_sem_etc(g_prefSnooze, 1, B_TIMEOUT, 6000000LL);
	if (g_debug & 8) fprintf(stderr, "starting to save game_dev\n");
	MiniAutolock lock(g_prefLock);
	if (m_mixerState.get(m_fd) < 0)
	{
		fprintf(stderr, "m_mixerState: cannot get() state\n");
		return B_ERROR;
	}
	g_prefThread = -1;
	int fd = open(SETTINGS_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fd = errno;
		perror("error: game_audio_setup.bin");
		return fd;
	}
	int32 version = CUR_VERSION;
	write(fd, &version, sizeof(version));
	version = m_mixerState.save_size();
	write(fd, &version, sizeof(version));
	void * data = malloc(version);
	if (!data) {
		close(fd);
		perror("error: saving prefs malloc()");
		return B_NO_MEMORY;
	}
	status_t err = m_mixerState.save(data, version);
	if (err < 0) {
		close(fd);
		free(data);
		fprintf(stderr, "error: m_mixerState.save(): %s\n", strerror(err));
		return err;
	}
	write(fd, data, version);
	free(data);
	version = sizeof(m_serverState);
	write(fd, &version, sizeof(version));
	write(fd, &m_serverState, version);
	close(fd);
	if (g_debug & 8) fprintf(stderr, "info: game_dev: saved settings\n");
	return B_OK;
}

//	this function loads the info, if any, into the struct given
void
game_dev::pref_load()
{
	int fd;
	int32 version;
	if (g_debug & 8) fprintf(stderr, "info: game_dev: loading settings\n");
	fd = open(SETTINGS_FILE_NAME, O_RDONLY);
	if (fd >= 0) {
		read(fd, &version, sizeof(version));
		if (version == CUR_VERSION) {
			read(fd, &version, sizeof(version));
			if (version < 4 || version > 65536)
			{
				fprintf(stderr, "pref_load(): bad pref size %ld\n", version);
				goto error;
			}
			void * data = malloc(version);
			if (data) {
				read(fd, data, version);
			}
			if (data && (m_mixerState.load(data, version) >= 0)) {
				if (g_debug & 8) fprintf(stderr, "info: loaded mixer setup\n");
				m_mixerState.set(m_fd);
			}
			else
			{
				fprintf(stderr, "warning: mixer state not restored\n");
			}
			free(data);
		}
		else
		{
			if (g_debug & 8) fprintf(stderr, "pref_load(): bad version %ld\n", version);
		}
error:
		close(fd);
	}
	else
	{
		perror(SETTINGS_FILE_NAME);
	}
}

static uint32 sr_from_mask(uint32 mask);

status_t 
game_dev::init_adc(const media_multi_audio_format& adc_format)
{
	status_t err;
	if (m_fd <= 0) return m_fd;

	// avoid messy situations
	if (GAME_IS_STREAM(m_adcStreamID)) {
		fprintf(stderr,
			"game_dev::init_adc():\n\t"
			"abort: the ADC stream is already running.\n");
		return B_ERROR;
	}
	
	// we base ADC settings on the DAC, so ensure that it's set up first:
	if (m_dacStreamID < 0) {
		fprintf(stderr,
			"game_dev::init_adc():\n\t"
			"abort: the DAC must be configured first.\n");
		return B_ERROR;
	}
	
	// fetch current DAC format
#ifdef GAME_CURRENT_API_VERSION
	G<game_get_codec_infos> ggdis;
	H<game_codec_info> gdi;
	ggdis.info = &gdi;
	ggdis.in_request_count = 1;
	gdi.codec_id = GAME_MAKE_DAC_ID(0);
	ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggdis);
#else
	G<game_get_dac_infos> ggdis;
	H<game_dac_info> gdi;
	ggdis.info = &gdi;
	ggdis.in_request_count = 1;
	gdi.dac_id = GAME_MAKE_DAC_ID(0);
	ioctl(m_fd, GAME_GET_DAC_INFOS, &ggdis);
#endif

	// fetch current ADC format
#ifdef GAME_CURRENT_API_VERSION
	G<game_get_codec_infos> ggais;
	H<game_codec_info> gai;
	ggais.info = &gai;
	ggais.in_request_count = 1;
	gai.codec_id = GAME_MAKE_ADC_ID(0);
	ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggais);
#else
	G<game_get_adc_infos> ggais;
	H<game_adc_info> gai;
	ggais.info = &gai;
	ggais.in_request_count = 1;
	gai.adc_id = GAME_MAKE_ADC_ID(0);
	ioctl(m_fd, GAME_GET_ADC_INFOS, &ggais);
#endif

	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	uint32 channelCount = adc_format.channel_count;
	if (channelCount != w.channel_count &&
		channelCount != 2) {
		channelCount = 2;
		fprintf(stderr, "warning: game_dev: unsupported ADC channel count %ld clamped to %ld\n",
			adc_format.channel_count, channelCount);
	}
	uint32 format = adc_format.format;
	if (format != w.format && format != media_multi_audio_format::B_AUDIO_SHORT) {
		fprintf(stderr, "warning: game_dev: ignoring unsupported ADC format %ld\n", format);
		format = media_multi_audio_format::B_AUDIO_SHORT;
	}
	if (adc_format.frame_rate != w.frame_rate && adc_format.frame_rate != m_dacFormat.frame_rate) {
		fprintf(stderr, "warning: game_dev: ignoring requested frame_rate %.2f; using DAC rate %.2f\n",
			adc_format.frame_rate, m_dacFormat.frame_rate);
	}
	size_t bufferFrames = (adc_format.buffer_size != w.buffer_size) ?
		adc_format.buffer_size / ((format & 0x0f) * channelCount) :
		m_dacBufferFrames;

	// build a matching ADC format
	G<game_set_codec_formats> gscfs;
	H<game_codec_format> gcf;
	gscfs.formats = &gcf;
	gscfs.in_request_count = 1;
	gcf.codec = GAME_MAKE_ADC_ID(0);
	gcf.flags = GAME_CODEC_CLOSEST_OK;
	if (gai.cur_chunk_frame_count != bufferFrames) {
		gcf.flags |= GAME_CODEC_SET_CHUNK_FRAME_COUNT;
		gcf.chunk_frame_count = bufferFrames;
		if (g_debug & 1) fprintf(stderr, "info: adc: requesting chunk size %ld frames\n", gcf.chunk_frame_count);
	}
	if (gai.cur_frame_rate != gdi.cur_frame_rate) {
		gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
		gcf.frame_rate = gdi.cur_frame_rate;
		if (g_debug & 1) fprintf(stderr, "info: adc: requesting frame rate 0x%hx\n", gcf.frame_rate);
	}
	if ((gdi.cur_frame_rate & B_SR_CVSR) &&
		gai.cur_cvsr != gdi.cur_cvsr) {
		gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
		gcf.cvsr = gdi.cur_cvsr;
		if (g_debug & 1) fprintf(stderr, "info: adc: requesting cvsr %.02f\n", gcf.cvsr);
	}
	if (gai.cur_format != gdi.cur_format) {
		gcf.flags |= GAME_CODEC_SET_FORMAT;
		gcf.format = gdi.cur_format;
		if (g_debug & 1) fprintf(stderr, "info: adc: requesting sample format 0x%hx\n", gcf.format);
	}
	if (gai.cur_channel_count != gdi.cur_channel_count) {
		gcf.flags |= GAME_CODEC_SET_CHANNELS;
		gcf.channels = channelCount;
		if (g_debug & 1) fprintf(stderr, "info: adc: requesting %d channels\n", gcf.channels);
	}
	// see if the codec agrees
	ioctl(m_fd, GAME_SET_CODEC_FORMATS, &gscfs);
	if (gcf.out_status < 0) {
		fprintf(stderr, "error: adc: set_codec_formats(): %s\n", strerror(gcf.out_status));
	}
#ifdef GAME_CURRENT_API_VERSION
	ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggais);
#else
	ioctl(m_fd, GAME_GET_ADC_INFOS, &ggais);
#endif
	if (gai.cur_frame_rate & B_SR_CVSR) m_adcFormat.frame_rate = gai.cur_cvsr;
	else m_adcFormat.frame_rate = sr_from_mask(gai.cur_frame_rate);
	if (m_adcFormat.frame_rate < 4000.0) {
		fprintf(stderr, "warning: adc: device returns poor sample rate value %.2f (assuming 48 kHz)\n",
			m_adcFormat.frame_rate);
		m_adcFormat.frame_rate = 48000.0f;
	}
	else if (g_debug & 1) {
		fprintf(stderr, "info: adc: device returns sample rate %.2f\n", m_adcFormat.frame_rate);
	}
	m_adcFormat.channel_count = gai.cur_channel_count;
	m_adcFormat.format = media_multi_audio_format::B_AUDIO_SHORT;
	m_adcFormat.byte_order = B_MEDIA_HOST_ENDIAN;
	m_adcBufferFrames = gai.cur_chunk_frame_count;
	m_adcFormat.buffer_size = (m_adcFormat.format & 0x0f) * m_adcFormat.channel_count * m_adcBufferFrames;
	fprintf(stderr, "info: adc: using sound buffer size %d frames (%d buffers)\n", m_adcBufferFrames, GAMEBUFCNT);

	// init the mixer
	err = scan_codec_mixer(gcf.codec, &m_adcMixerInfo);

	if (m_adcMixerInfo.mixer_id == GAME_NO_ID) {
		fprintf(stderr, "warning: adc: snd_server needs to find out which mixer to use\n");
	}
	else {
		if (m_adcLevel.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: adc: mixer has no 'input gain' capability\n");
		}
		if (m_micBoostEnable.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: adc: mixer has no 'mic boost' capability\n");
		}
		if (m_adcMux.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: adc: mixer has no 'input source mux' capability\n");
		}
	}

	return B_OK;
}

status_t 
game_dev::open_adc_stream()
{
	status_t err;
	// sanity checks
	if (m_adcStreamID != GAME_NO_ID) return B_ERROR;
	if (!m_adcFormat.buffer_size || !m_adcBufferFrames) {
		fprintf(stderr, "abort: adc: stream not configured\n",
			strerror(m_adcStreamSem));
		return B_ERROR;
	}
	if (m_captureChannels) {
		fprintf(stderr, "warning: adc: deleting old capture channels\n");
		capture_channel* next;
		for (capture_channel* c = m_captureChannels; c; c = next)
		{
			next = c->next;
			delete c;
		}
		m_captureChannels = 0;
	}
	// make sure the capture thread is dead
	stop_adc_stream();
	// init stream sem
	if (m_adcStreamSem) delete_sem(m_adcStreamSem);
	m_adcStreamSem = create_sem(0, "game_dev::m_adcStreamSem");
	if (m_adcStreamSem < B_OK) {
		fprintf(stderr, "abort: adc: create_sem(): %s\n", strerror(m_adcStreamSem));
		return m_adcStreamSem;
	}
	// open stream
	G<game_open_stream> gos;
#ifdef GAME_CURRENT_API_VERSION
	gos.codec_id = GAME_MAKE_ADC_ID(0);
#else
	gos.adc_dac_id = GAME_MAKE_ADC_ID(0);
#endif
	gos.request = 0;
	gos.stream_sem = m_adcStreamSem;
	err = (ioctl(m_fd, GAME_OPEN_STREAM, &gos) < 0) ? errno : B_OK;
	if (err < B_OK) {
		fprintf(stderr, "abort: adc: GAME_OPEN_STREAM: %s\n", strerror(err));
		return err;
	}
	m_adcStreamID = gos.out_stream_id;
	// alloc buffers (don't clone, since this team never needs to read the
	// recorded data.)
	const int bufferCount = g_gameBufCnt;
	cloned_buffer_set* s = new cloned_buffer_set(*this, -1, bufferCount);
	if (m_captureBufferSet) delete m_captureBufferSet;
	m_captureBufferSet = s;
	// allocate looping/pingpong buffer
#ifdef GAME_CURRENT_API_VERSION
	H<game_open_stream_buffer> gosb;
	gosb.stream = m_adcStreamID;
	gosb.byte_size = m_adcFormat.buffer_size * bufferCount;
	G<game_open_stream_buffers> gosbs;
	gosbs.in_request_count = 1;
	gosbs.buffers = &gosb;
	err = (ioctl(m_fd, GAME_OPEN_STREAM_BUFFERS, &gosbs) < 0) ? errno : B_OK;
#else
	G<game_open_stream_buffer> gosb;
	gosb.stream = m_adcStreamID;
	gosb.byte_size = m_adcFormat.buffer_size * g_gameBufCnt;
	err = (ioctl(m_fd, GAME_OPEN_STREAM_BUFFER, &gosb) < 0) ? errno : B_OK;
#endif
	if (err < B_OK)
	{
		perror("error: adc: ioctl(GAME_OPEN_STREAM_BUFFER)");
		close_adc_stream();
		return err;
	}
	size_t offset = 0;
	for (int n = 0; n < bufferCount; n++, offset += m_adcFormat.buffer_size)
	{
		s->buffers[n].id = gosb.buffer;
		s->buffers[n].area = gosb.area;
		s->buffers[n].offset = gosb.offset + offset;
		s->buffers[n].size = m_adcFormat.buffer_size;
		s->buffers[n].local_area = -1;
		s->buffers[n].data = 0;
		s->buffers[n].size_used = 0;
		s->buffers[n].next_queued = 0;
	}
	return B_OK;
}

status_t 
game_dev::close_adc_stream()
{
	status_t err;
	if (m_adcStreamID == GAME_NO_ID) return B_ERROR;
	// wait for capture thread to stop if necessary
	stop_adc_stream();
	// close stream
	G<game_close_stream> gcs;
	gcs.stream = m_adcStreamID;
	gcs.flags = GAME_CLOSE_DELETE_SEM_WHEN_DONE;
	if (ioctl(m_fd, GAME_CLOSE_STREAM, &gcs) < 0) {
		perror("ioctl(GAME_CLOSE_STREAM)");
	}
	if (m_captureBufferSet) {
		delete m_captureBufferSet;
		m_captureBufferSet = 0;
	}
	m_adcStreamID = GAME_NO_ID;
	m_adcStreamSem = -1;
	return B_OK;
}

status_t 
game_dev::start_adc_stream()
{
	status_t err;
	if (!GAME_IS_STREAM(m_adcStreamID)) {
		fprintf(stderr, "abort: adc: start_adc_stream(): no stream.\n");
		return B_ERROR;
	}
	if (!m_captureBufferSet) {
		fprintf(stderr, "abort: adc: start_adc_stream(): no buffers.\n");
		return B_ERROR;
	}
	if (m_runCaptureThread)  {
		fprintf(stderr, "abort: adc: start_adc_stream(): already running.\n");
		return B_ERROR;
	}
	// wait for capture thread to stop if necessary
	stop_adc_stream();
	// init buffer
#ifdef GAME_CURRENT_API_VERSION
	G<game_set_stream_buffer> gssb;
	gssb.stream = m_adcStreamID;
	gssb.flags = GAME_BUFFER_PING_PONG;
	gssb.buffer = m_captureBufferSet->buffers[0].id;
	gssb.frame_count = m_adcBufferFrames * m_captureBufferSet->buffer_count;
	gssb.page_frame_count = m_adcBufferFrames;
	err = (ioctl(m_fd, GAME_SET_STREAM_BUFFER, &gssb) < 0) ? errno : B_OK;
	if (err < B_OK) {
		fprintf(stderr, "abort: adc: GAME_SET_STREAM_BUFFER: %s\n", strerror(err));
		return err;
	}
#else
	G<game_queue_stream_buffer> gqsb;
	gqsb.stream = m_adcStreamID;
	gqsb.flags = GAME_BUFFER_LOOPING | GAME_BUFFER_PING_PONG;
	gqsb.buffer = m_captureBufferSet->buffers[0].id;
	gqsb.frame_count = m_adcBufferFrames * m_captureBufferSet->buffer_count;
	gqsb.page_frame_count = m_adcBufferFrames;
	err = (ioctl(m_fd, GAME_QUEUE_STREAM_BUFFER, &gqsb) < 0) ? errno : B_OK;
	if (err < B_OK) {
		fprintf(stderr, "abort: adc: GAME_QUEUE_STREAM_BUFFER: %s\n", strerror(err));
		return err;
	}
#endif
	// start the stream
	H<game_run_stream> grs;
	grs.stream = m_adcStreamID;
	grs.state = GAME_STREAM_RUNNING;
	G<game_run_streams> grss;
	grss.in_stream_count = 1;
	grss.streams = &grs;
	err = (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) ? errno : B_OK;
	if (err < B_OK) {
		fprintf(stderr, "abort: adc: GAME_RUN_STREAMS: %s\n", strerror(err));
		return err;
	}
	// start the thread
	m_runCaptureThread = true;
	m_captureThread = spawn_thread(capture_thread_entry, "game_dev::capture_thread()", 115, this);
	if (m_captureThread < B_OK) {
		fprintf(stderr, "abort: adc: spawn_thread(): %s\n", strerror(m_captureThread));
		grs.state = GAME_STREAM_STOPPED;
		ioctl(m_fd, GAME_RUN_STREAMS, &grss);
		return m_captureThread;
	}
	return resume_thread(m_captureThread);
}

status_t 
game_dev::stop_adc_stream()
{
	thread_id tid = m_captureThread;
	if (tid > B_OK)
	{
		m_runCaptureThread = false;
		status_t err;
		while (wait_for_thread(tid, &err) == B_INTERRUPTED) {}
	}
	m_nextBufferIndex = 0;
	return B_OK;
}

status_t 
game_dev::capture_thread_entry(void *cookie)
{
	((game_dev*)cookie)->capture_thread();
	return B_OK;
}

void 
game_dev::capture_thread()
{
	status_t err;
	ASSERT(m_captureBufferSet);
	const uint16 bufferCount = m_captureBufferSet->buffer_count;

	while (m_runCaptureThread)
	{
		// wait for a fresh input buffer
		err = acquire_sem_etc(m_adcStreamSem, 1, B_TIMEOUT, SND_TIMEOUT);
		if (err < B_OK)
		{
			fprintf(stderr,
				"!!! game_dev::capture_thread():\n\t"
				"ABORT: acquire_sem_etc(m_input_sem): %s\n", strerror(err));
			break;
		}
		// broadcast notice
		{
			MiniAutolock _l(m_captureLock);
			capture_channel** c = &m_captureChannels;
			while (*c)
			{
				err = release_sem((*c)->sem);
				if (err < B_OK)
				{
					fprintf(stderr,
						"game_dev::capture_thread():\n\t"
						"flushing dead channel 0x%x\n",
						(*c)->id);
					capture_channel* d = *c;
					*c = d->next;
					delete d;
				}
				else
				{
					c = &(*c)->next;
				}
			}
			m_nextBufferIndex = (m_nextBufferIndex+1) % bufferCount;	
			if (!m_captureChannels) break;
		}
	}

	// stop the stream
	H<game_run_stream> grs;
	grs.stream = m_adcStreamID;
	grs.state = GAME_STREAM_STOPPED;
	G<game_run_streams> grss;
	grss.in_stream_count = 1;
	grss.streams = &grs;
	err = (ioctl(m_fd, GAME_RUN_STREAMS, &grss) < 0) ? errno : B_OK;
	if (err < B_OK) {
		fprintf(stderr, "warning: adc: capture_thread(): GAME_RUN_STREAMS(stop): %s\n",
			strerror(err));
	}
	// tidy affairs
	m_captureThread = -1;
	m_runCaptureThread = false;
}

// look for a mixer likely to match the given codec (either explicitly linked,
// or containing control(s) matching the codec type.)  in the event of a match,
// call handle_xxx_control() for each control.
status_t 
game_dev::scan_codec_mixer(int32 codecID, game_mixer_info *outMixerInfo)
{
	if (!GAME_IS_DAC(codecID) && !GAME_IS_ADC(codecID)) return B_BAD_VALUE;
	status_t err;
	int32 linkedMixer = GAME_NO_ID;

#ifdef GAME_CURRENT_API_VERSION
	G<game_get_codec_infos> ggcis;
	H<game_codec_info> gci;
	ggcis.info = &gci;
	ggcis.in_request_count = 1;
	gci.codec_id = codecID;
	err = (ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggcis) < 0) ? errno : B_OK;
	linkedMixer = gci.linked_mixer_id;
#else
	if (GAME_IS_DAC(codecID)) {
		G<game_get_dac_infos> ggdis;
		H<game_dac_info> gdi;
		ggdis.info = &gdi;
		ggdis.in_request_count = 1;
		gdi.dac_id = codecID;
		err = (ioctl(m_fd, GAME_GET_DAC_INFOS, &ggdis) < 0) ? errno : B_OK;
		linkedMixer = gdi.linked_mixer_id;
	} else {
		G<game_get_adc_infos> ggais;
		H<game_adc_info> gai;
		ggais.info = &gai;
		ggais.in_request_count = 1;
		gai.adc_id = codecID;
		err = (ioctl(m_fd, GAME_GET_ADC_INFOS, &ggais) < 0) ? errno : B_OK;
		linkedMixer = gai.linked_mixer_id;
	}
#endif

	if (err < B_OK) {
		fprintf(stderr, "warning: GAME_GET_CODEC_INFOS: %s\n", strerror(err));
		return err;
	}

	if (linkedMixer == GAME_NO_ID) {
		fprintf(stderr, "warning: no linked mixer for codec 0x%hx\n", codecID);
		
		// guess the appropriate mixer by looking for a representative level control
		uint32 targetLevelType = GAME_IS_DAC(codecID) ?
			GAME_LEVEL_AC97_MASTER :
			GAME_LEVEL_AC97_RECORD;

		for (int mixerIndex = 0; linkedMixer == GAME_NO_ID; mixerIndex++) {
			H<game_mixer_info> gmi;
			gmi.mixer_id = GAME_MAKE_MIXER_ID(mixerIndex);
			G<game_get_mixer_infos> ggmis;
			ggmis.in_request_count = 1;
			ggmis.info = &gmi;
			if (ioctl(m_fd, GAME_GET_MIXER_INFOS, &ggmis) < 0) break;

			G<game_get_mixer_controls> ggmc;
			ggmc.mixer_id = gmi.mixer_id;
			ggmc.control = (game_mixer_control*)alloca(sizeof(game_mixer_control) * gmi.control_count);
			if (!ggmc.control) continue;
			ggmc.in_request_count = gmi.control_count;
			if (ioctl(m_fd, GAME_GET_MIXER_CONTROLS, &ggmc) < 0) {
				fprintf(stderr, "warning: GAME_GET_MIXER_CONTROLS(0x%hx): %s\n",
					gmi.mixer_id, strerror(errno));
				continue;
			}
			
			for (int n = 0; n < gmi.control_count; n++) {
#ifdef GAME_CURRENT_API_VERSION
				if (ggmc.control[n].kind != GAME_MIXER_CONTROL_IS_LEVEL) continue;
#else
				if (ggmc.control[n].type != GAME_MIXER_CONTROL_IS_LEVEL) continue;
#endif
				G<game_get_mixer_level_info> ggmli;
				ggmli.mixer_id = gmi.mixer_id;
				ggmli.control_id = ggmc.control[n].control_id;
				if (ioctl(m_fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli) < 0) {
					fprintf(stderr,
						"warning: GAME_GET_MIXER_LEVEL_INFO(mixer 0x%hx, control 0x%hx): %s\n",
						gmi.mixer_id, ggmli.control_id, strerror(errno));
					continue;
				}
				if (ggmli.type == targetLevelType) {
					// found one
					linkedMixer = gmi.mixer_id;
					break;
				}
			}
		}
	}
	
	if (linkedMixer == GAME_NO_ID) {
		// give up
		fprintf(stderr, "warning: no mixer found for codec 0x%hx\n", codecID);
		return B_ERROR;
	}
	
	// populate mixer info
	outMixerInfo->mixer_id = linkedMixer;
	G<game_get_mixer_infos> ggmis;
	ggmis.in_request_count = 1;
	ggmis.info = outMixerInfo;
	if (ioctl(m_fd, GAME_GET_MIXER_INFOS, &ggmis) < 0) {
		fprintf(stderr, "error: GAME_GET_MIXER_INFOS(0x%hx): %s\n",
			outMixerInfo->mixer_id, strerror(errno));
		return errno;
	}

	// fetch controls
	G<game_get_mixer_controls> ggmc;
	ggmc.mixer_id = outMixerInfo->mixer_id;
	ggmc.control = (game_mixer_control*)alloca(sizeof(game_mixer_control) * outMixerInfo->control_count);
	if (!ggmc.control) return B_NO_MEMORY;
	ggmc.in_request_count = outMixerInfo->control_count;
	if (ioctl(m_fd, GAME_GET_MIXER_CONTROLS, &ggmc) < 0) {
		fprintf(stderr, "error: GAME_GET_MIXER_CONTROLS(0x%hx): %s\n",
			ggmc.mixer_id, strerror(errno));
		return errno;
	}
	
	// dispatch control descriptions to hooks
	for (int n = 0; n < outMixerInfo->control_count; n++) {
#ifdef GAME_CURRENT_API_VERSION
		switch (ggmc.control[n].kind)
#else
		switch (ggmc.control[n].type)
#endif
		{
			case GAME_MIXER_CONTROL_IS_LEVEL: {
				G<game_get_mixer_level_info> ggmli;
				ggmli.mixer_id = ggmc.mixer_id;
				ggmli.control_id = ggmc.control[n].control_id;
				if (ioctl(m_fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli) < 0) {
					fprintf(stderr,
						"error: GAME_GET_MIXER_LEVEL_INFO(mixer 0x%hx, control 0x%hx): %s\n",
						ggmc.mixer_id, ggmli.control_id, strerror(errno));
					continue;
				}
				if (g_debug & 2) {
					fprintf(stderr, " * level info: '%s' id %ld mixer %ld\n", ggmli.label,
						ggmli.control_id, ggmli.mixer_id);
					fprintf(stderr, "         flags: 0x%lx  values: %d  min: %d  max: %d\n",
						ggmli.flags, ggmli.value_count, ggmli.min_value, ggmli.max_value);
					fprintf(stderr, "         normal: %d  min_disp: %g  max_disp: %g\n",
						ggmli.normal_value, ggmli.min_value_disp, ggmli.max_value_disp);
					fprintf(stderr, "         format: '%s'  type: 0x%lx\n",
						ggmli.disp_format, ggmli.type);
				}
				handle_level_control(codecID, *outMixerInfo, ggmli);
				break;
			}
			case GAME_MIXER_CONTROL_IS_MUX: {
				G<game_get_mixer_mux_info> ggmmi;
				ggmmi.mixer_id = ggmc.mixer_id;
				ggmmi.control_id = ggmc.control[n].control_id;
				ggmmi.in_request_count = 16;
				ggmmi.items = (game_mux_item*)alloca(sizeof(game_mux_item) * ggmmi.in_request_count);
				memset(ggmmi.items, 0, sizeof(game_mux_item) * ggmmi.in_request_count);
				if (ioctl(m_fd, GAME_GET_MIXER_MUX_INFO, &ggmmi) < 0) {
					fprintf(stderr,
						"error: GAME_GET_MIXER_MUX_INFO(mixer 0x%hx, control 0x%hx): %s\n",
						ggmc.mixer_id, ggmmi.control_id, strerror(errno));
					continue;
				}
				handle_mux_control(codecID, *outMixerInfo, ggmmi);
				break;
			}
			case GAME_MIXER_CONTROL_IS_ENABLE: {
				G<game_get_mixer_enable_info> ggmei;
				ggmei.mixer_id = ggmc.mixer_id;
				ggmei.control_id = ggmc.control[n].control_id;
				if (ioctl(m_fd, GAME_GET_MIXER_ENABLE_INFO, &ggmei) < 0) {
					fprintf(stderr,
						"error: GAME_GET_MIXER_ENABLE_INFO(mixer 0x%hx, control 0x%hx): %s\n",
						ggmc.mixer_id, ggmei.control_id, strerror(errno));
					continue;
				}
				handle_enable_control(codecID, *outMixerInfo, ggmei);
				break;
			}
			default:
#ifdef GAME_CURRENT_API_VERSION
				fprintf(stderr, "warning: unknown type 0x%hx for mixer 0x%hx, control 0x%hx\n",
					ggmc.control[n].kind, ggmc.mixer_id, ggmc.control[n].control_id);
#else
				fprintf(stderr, "warning: unknown type 0x%hx for mixer 0x%hx, control 0x%hx\n",
					ggmc.control[n].type, ggmc.mixer_id, ggmc.control[n].control_id);
#endif
		}
	}
	
	return B_OK;
}

void 
game_dev::handle_level_control(int32 codecID, const game_mixer_info &mixer, const game_get_mixer_level_info &control)
{
	if (GAME_IS_DAC(codecID)) {
		if (control.type == GAME_LEVEL_AC97_MASTER) {
			m_mainLevel = control;
			if (g_debug & 1) fprintf(stderr,
				"info: master level id %d, '%s', range %d-%d (%d), flags 0x%x\n",
				control.control_id,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags);
		}
		else if (control.type == GAME_LEVEL_AC97_PHONE) {
			m_modemLevel = control;
			if (g_debug & 1) fprintf(stderr,
				"info: phone level id %d, '%s', range %d-%d (%d), flags 0x%x\n",
				control.control_id,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags);
		}
		else if (control.type == GAME_LEVEL_AC97_CD) {
			m_cdLevel = control;
			if (g_debug & 1) fprintf(stderr,
				"info: CD level id %d, '%s', range %d-%d (%d), flags 0x%x\n",
				control.control_id,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags);
		}
		else {
			if (g_debug & 1) fprintf(stderr,
				"info: ignoring level id %d, type %d, '%s', range %d-%d (%d), flags 0x%x (type 0x%x)\n",
				control.control_id,
				control.type,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags,
				control.type);
		}
	}
	else {
		if (control.type == GAME_LEVEL_AC97_RECORD) {
			m_adcLevel = control;
			if (g_debug & 1) fprintf(stderr,
				"info: adc: input gain level id %d, '%s', range %d-%d (%d), flags 0x%x\n",
				control.control_id,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags);
		}
		else {
			if (g_debug & 1) fprintf(stderr,
				"info: adc: ignoring level id %d, type %d, '%s', range %d-%d (%d), flags 0x%x (type 0x%x)\n",
				control.control_id,
				control.type,
				control.label,
				control.min_value,
				control.max_value,
				control.normal_value,
				control.flags,
				control.type);
		}
	}
}

void 
game_dev::handle_mux_control(int32 codecID, const game_mixer_info &mixer, const game_get_mixer_mux_info &control)
{
	if (GAME_IS_DAC(codecID)) {
		if (g_debug & 1) fprintf(stderr,
			"info: ignoring mux id %d, type %d, '%s', flags 0x%x\n",
			control.control_id,
			control.type,
			control.label,
			control.flags);
	}
	else {
		if (control.type == GAME_MUX_AC97_RECORD_SELECT) {
			if (g_debug & 1) fprintf(stderr,
				"info: adc: record mux id %d, '%s', flags 0x%x\n",
				control.control_id,
				control.label,
				control.flags);

			// match input sources by name
			m_adcMux = control;
			for (int n = 0; n < control.out_actual_count; n++) {
				if (!strncasecmp(control.items[n].name, "main", 4) ||
					!strncasecmp(control.items[n].name, "output", 6)) {
					if (g_debug & 1) fprintf(stderr, "\tguessing '%s':0x%x is ADC_MUX_MAIN_OUT\n",
						control.items[n].name, control.items[n].mask);
					m_adcMuxValues[ADC_MUX_MAIN_OUT] = control.items[n].mask;
				}
				else
				if (!strncasecmp(control.items[n].name, "cd", 2)) {
					if (g_debug & 1) fprintf(stderr, "\tguessing '%s':0x%x is ADC_MUX_CD_IN\n",
						control.items[n].name, control.items[n].mask);
					m_adcMuxValues[ADC_MUX_CD_IN] = control.items[n].mask;
				}
				else
				if (!strncasecmp(control.items[n].name, "line", 4)) {
					if (g_debug & 1) fprintf(stderr, "\tguessing '%s':0x%x is ADC_MUX_LINE_IN\n",
						control.items[n].name, control.items[n].mask);
					m_adcMuxValues[ADC_MUX_LINE_IN] = control.items[n].mask;
				}
				else
				if (!strncasecmp(control.items[n].name, "mic", 3)) {
					if (g_debug & 1) fprintf(stderr, "\tguessing '%s':0x%x is ADC_MUX_MIC_IN\n",
						control.items[n].name, control.items[n].mask);
					m_adcMuxValues[ADC_MUX_MIC_IN] = control.items[n].mask;
				}
				else
				if (g_debug & 1) fprintf(stderr,
					"\tignoring '%s':0x%x\n",
					control.items[n].name, control.items[n].mask);
			}
		}
		else {
			if (g_debug & 1) fprintf(stderr,
				"info: adc: ignoring mux id %d, type %d, '%s', flags 0x%x\n",
				control.control_id,
				control.type,
				control.label,
				control.flags);
		}
	}
}

void 
game_dev::handle_enable_control(int32 codecID, const game_mixer_info &mixer, const game_get_mixer_enable_info &control)
{
	if (GAME_IS_DAC(codecID)) {
		if (g_debug & 1) fprintf(stderr,
			"info: ignoring enable id %d, type %d, '%s', states( %s / %s )\n",
			control.control_id,
			control.type,
			control.label,
			control.enabled_label, control.disabled_label);
	}
	else {
		if (control.type == GAME_ENABLE_AC97_MIC_BOOST) {
			m_micBoostEnable = control;
			if (g_debug & 1) fprintf(stderr,
				"info: adc: mic boost enable id %d, '%s', states( %s / %s )\n",
				control.control_id,
				control.label,
				control.enabled_label, control.disabled_label);
		}
		else {
			if (g_debug & 1) fprintf(stderr,
				"info: adc: ignoring enable id %d, type %d, '%s', states( %s / %s )\n",
				control.control_id,
				control.type,
				control.label,
				control.enabled_label, control.disabled_label);
		}
	}
}


#define SM(x) { x, B_SR_##x }
static struct {
	uint32	sr;
	uint32	mask;
}
sr_masks[] = {
	SM(8000),
	SM(11025),
	SM(12000),
	SM(16000),
	SM(22050),
	SM(24000),
	SM(32000),
	SM(44100),
	SM(48000),
	SM(64000),
	SM(88200),
	SM(96000),
	SM(176400),
	SM(192000),
	SM(384000),
	SM(1536000),
};

static uint32
mask_from_sr(
	uint32 sr)
{
	for (int ix=0; ix<sizeof(sr_masks)/sizeof(sr_masks[0]); ix++) {
		if (sr == sr_masks[ix].sr) return sr_masks[ix].mask;
	}
	return 0;
}

static uint32
sr_from_mask(
	uint32 mask)
{
	for (int ix=0; ix<sizeof(sr_masks)/sizeof(sr_masks[0]); ix++) {
		if (mask == sr_masks[ix].mask) return sr_masks[ix].sr;
	}
	return 0;
}

int
game_dev::setup(
	const char * dev,
	const media_multi_audio_format& dac_format,
	const media_multi_audio_format& adc_format)
{
	status_t err;
	//	At some point, we could open devices on demand 
	//	and support multiple devices, since the name of 
	//	the requested device is now in the connect request 
	//	(although we ignore it there :-)
	//	(Actually, since we now scan for device and can't
	//	rely on AUDIO_DEVICE, that name becomes less useful.)

	m_fd = open(dev, O_RDWR);
	if (m_fd < 0) {
		perror(dev);
		return 1;
	}

	//	DAC format
#ifdef GAME_CURRENT_API_VERSION
	G<game_get_codec_infos> ggdis;
	H<game_codec_info> gdi;
	ggdis.info = &gdi;
	ggdis.in_request_count = 1;
	gdi.codec_id = GAME_MAKE_DAC_ID(0);
	fprintf(stderr, "GAME_GET_CODEC_INFOS(%ld)", ggdis.info_size);
	ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggdis);
#else
	G<game_get_dac_infos> ggdis;
	H<game_dac_info> gdi;
	ggdis.info = &gdi;
	ggdis.in_request_count = 1;
	gdi.dac_id = GAME_MAKE_DAC_ID(0);
	ioctl(m_fd, GAME_GET_DAC_INFOS, &ggdis);
#endif

	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	float frameRate = dac_format.frame_rate;
	if (frameRate == w.frame_rate) frameRate = 44100.0f;
	uint32 channelCount = dac_format.channel_count;
	if (channelCount != w.channel_count) {
		// +++++ should we reject 3 or 5 channel requests?
		if (channelCount < 2) channelCount = 2;
		else if (channelCount > 6) channelCount = 6;
		if (dac_format.channel_count != channelCount) {
			fprintf(stderr, "warning: game_dev: unsupported channel count %ld clamped to %ld\n",
				dac_format.channel_count, channelCount);
		}
	}
	if (dac_format.format != w.format && dac_format.format != media_multi_audio_format::B_AUDIO_SHORT) {
		fprintf(stderr, "warning: game_dev: unsupported DAC format %ld\n", dac_format.format);
	}
	
	int32 ss = dac_format.buffer_size/((dac_format.format & 0x0f) * channelCount);
	if ((gdi.cur_chunk_frame_count != ss) && (gdi.chunk_frame_count_increment != 0)) {
		if (gdi.max_chunk_frame_count <= ss) {
			ss = gdi.max_chunk_frame_count;
		}
		else {
			int32 cfc = gdi.min_chunk_frame_count;
			while (cfc < ss) {
				if (gdi.chunk_frame_count_increment <= 0) {
					cfc <<= 1;
				}
				else {
					cfc += gdi.chunk_frame_count_increment;
				}
			}
			if (cfc > gdi.max_chunk_frame_count) {
				cfc = gdi.max_chunk_frame_count;
			}
			ss = cfc;
		}
	}
	else {
		ss = gdi.cur_chunk_frame_count;
	}
	G<game_set_codec_formats> gscfs;
	H<game_codec_format> gcf;
	gscfs.formats = &gcf;
	gscfs.in_request_count = 1;
#ifdef GAME_CURRENT_API_VERSION
	gcf.codec = gdi.codec_id;
#else
	gcf.codec = gdi.dac_id;
#endif
	gcf.flags = GAME_CODEC_CLOSEST_OK;
	if (ss != gdi.cur_chunk_frame_count) {
		gcf.flags |= GAME_CODEC_SET_CHUNK_FRAME_COUNT;
		gcf.chunk_frame_count = ss;
		if (g_debug & 1) fprintf(stderr, "info: requesting chunk size %ld frames\n", ss);
	}
	if (fabs(gdi.cur_cvsr - frameRate) >= 1.0) {
		uint32 m = mask_from_sr(uint32(frameRate));
		if (gdi.frame_rates & m) {
			gcf.frame_rate = m;
			gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
			if (g_debug & 1) fprintf(stderr, "info: requesting sample rate %ld\n", m);
		}
		else
		if ((gdi.frame_rates & B_SR_CVSR) &&
			(gdi.cvsr_min <= frameRate) &&
			(gdi.cvsr_max >= frameRate)) {
			gcf.frame_rate = B_SR_CVSR;
			gcf.cvsr = frameRate;
			gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
			if (g_debug & 1) fprintf(stderr, "info: requesting sample rate %.1f Hz\n", frameRate);
		}
		else {
			fprintf(stderr, "warning: device appears uncapable of %.1f Hz playback\n", frameRate);
		}
	}
	if (!(gdi.cur_format & B_FMT_16BIT)) {
		if (!gdi.formats & B_FMT_16BIT) {
			fprintf(stderr, "error: device doesn't support 16-bit audio (you need another snd_server)\n");
			return 1;
		}
		gcf.format = B_FMT_16BIT;
		gcf.flags |= GAME_CODEC_SET_FORMAT;
		if (g_debug & 1) fprintf(stderr, "info: requesting sample format B_FMT_16BIT\n");
	}
	if (gdi.cur_channel_count != channelCount) {
		if (!(gdi.channel_counts & (1<<(channelCount-1)))) {
			fprintf(stderr, "error: device doesn't support %ld-channel audio (you need another snd_server)\n",
				channelCount);
			return 1;
		}
		gcf.channels = channelCount;
		gcf.flags |= GAME_CODEC_SET_CHANNELS;
		if (g_debug & 1) fprintf(stderr, "info: requesting channel_count %ld\n", gcf.channels);
	}
	ioctl(m_fd, GAME_SET_CODEC_FORMATS, &gscfs);
	if (gcf.out_status < 0) {
		fprintf(stderr, "error: set_codec_formats(): %s\n", strerror(gcf.out_status));
	}
#ifdef GAME_CURRENT_API_VERSION
	ioctl(m_fd, GAME_GET_CODEC_INFOS, &ggdis);
#else
	ioctl(m_fd, GAME_GET_DAC_INFOS, &ggdis);
#endif
	if (gdi.cur_frame_rate & B_SR_CVSR) m_dacFormat.frame_rate = gdi.cur_cvsr;
	else m_dacFormat.frame_rate = sr_from_mask(gdi.cur_frame_rate);
	if (m_dacFormat.frame_rate < 4000.0f) {
		fprintf(stderr, "warning: device returns poor sample rate value %.2f (assuming 48 kHz)\n", m_dacFormat.frame_rate);
		m_dacFormat.frame_rate = 48000.0f;
	}
	else if (g_debug & 1) {
		fprintf(stderr, "info: device returns sample rate %.2f\n", m_dacFormat.frame_rate);
	}
	m_dacFormat.channel_count = gdi.cur_channel_count;
	m_dacFormat.format = media_multi_audio_format::B_AUDIO_SHORT;
	m_dacFormat.byte_order = B_MEDIA_HOST_ENDIAN;
	m_dacBufferFrames = gdi.cur_chunk_frame_count;
	m_dacFormat.buffer_size = (m_dacFormat.format & 0x0f) * m_dacFormat.channel_count * m_dacBufferFrames;
	fprintf(stderr, "info: using sound buffer size %d frames (%d buffers)\n", m_dacBufferFrames, GAMEBUFCNT);

	ASSERT(m_dacFormat.frame_rate);
	m_dacBufferDuration = (1000000LL * bigtime_t(m_dacBufferFrames)) / bigtime_t(m_dacFormat.frame_rate);
	scan_codec_mixer(gcf.codec, &m_dacMixerInfo);

	if (m_dacMixerInfo.mixer_id == GAME_NO_ID) {
		fprintf(stderr, "warning: snd_server needs to find out which mixer to use\n");
	}
	else {
		if (m_mainLevel.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: mixer has no 'main level' capability\n");
		}
		if (m_modemLevel.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: mixer has no 'phone level' capability\n");
		}
		if (m_cdLevel.control_id == GAME_NO_ID) {
			fprintf(stderr, "warning: mixer has no 'CD level' capability\n");
		}
	}
	//	create stream
#ifdef GAME_CURRENT_API_VERSION
	m_dacStreamSem = create_sem(0, "buffer issue");
#else
	m_dacStreamSem = create_sem(GAMEBUFCNT, "buffer issue");
#endif
	if (m_dacStreamSem < 0) {
		fprintf(stderr, "error: create_sem(): %s\n", strerror(m_dacStreamSem));
		return m_dacStreamSem;
	}
	G<game_open_stream> gos;
#ifdef GAME_CURRENT_API_VERSION
	gos.codec_id = gdi.codec_id;
	gos.request = (gdi.stream_capabilities.capabilities & GAME_STREAMS_HAVE_VOLUME) ?
		GAME_STREAM_VOLUME : 0;
#else
	gos.adc_dac_id = gdi.dac_id;
	gos.request = GAME_STREAM_VOLUME;
#endif
	gos.stream_sem = m_dacStreamSem;
	gos.frame_rate = gdi.cur_frame_rate;
	gos.cvsr_rate = gdi.cur_cvsr;
	gos.format = B_FMT_16BIT;
	gos.channel_count = 2;
	if (ioctl(m_fd, GAME_OPEN_STREAM, &gos) < 0) {
		perror("error: ioctl(GAME_OPEN_STREAM)");
		return 1;
	}
	m_dacStreamID = gos.out_stream_id;
	
	// init ADC but don't bother opening a stream for now
	err = init_adc(adc_format);
	if (err < B_OK) perror("error: init_adc()");
	
	//	volume etc
	pref_load();

	//	create buffers
#ifdef GAME_CURRENT_API_VERSION
	// single pingpong buffer
	H<game_open_stream_buffer> gosb;
	gosb.stream = m_dacStreamID;
	gosb.byte_size = m_dacFormat.buffer_size * GAMEBUFCNT;
	G<game_open_stream_buffers> gosbs;
	gosbs.in_request_count = 1;
	gosbs.buffers = &gosb;
	err = ioctl(m_fd, GAME_OPEN_STREAM_BUFFERS, &gosbs);
	if (err < 0) {
		perror("error: ioctl(GAME_OPEN_STREAM_BUFFER)");
		return 1;
	}
	m_dacLocalArea = clone_area("dac.clone", &m_dacPtr,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gosb.area);
	if (m_dacLocalArea < 0) {
		fprintf(stderr, "error: clone_area(): %s\n", strerror(m_dacLocalArea));
		return 1;
	}
	m_dacBufferID = gosb.buffer;
	fprintf(stderr, "+++++ NeUSKO01 all0k.kmpl33t +++++\n");
#else
	for (int ix=0; ix<GAMEBUFCNT; ix++) {
		G<game_open_stream_buffer> gosb;
		gosb.stream = m_dacStreamID;
		gosb.byte_size = m_dacFormat.buffer_size;
		err = ioctl(m_fd, GAME_OPEN_STREAM_BUFFER, &gosb);
		if (err < 0) {
			perror("error: ioctl(GAME_OPEN_STREAM_BUFFER)");
			return 1;
		}
		bool gotit = false;
		for (int iy=0; iy<ix; iy++) {
			if (m_dacAreaInfo[iy].orig == gosb.area) {
				m_dacAreaInfo[ix] = m_dacAreaInfo[iy];
				gotit = true;
				break;
			}
		}
		if (!gotit) {
			m_dacAreaInfo[ix].orig = gosb.area;
			m_dacAreaInfo[ix].clone = clone_area("buffer clone", &m_dacAreaInfo[ix].base,
				B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, gosb.area);
			if (g_debug & 1) {
				fprintf(stderr, "info: m_dacAreaInfo[%d].orig = %ld\n", ix, m_dacAreaInfo[ix].orig);
				fprintf(stderr, "info: m_dacAreaInfo[%d].clone = %ld\n", ix, m_dacAreaInfo[ix].clone);
				fprintf(stderr, "info: m_dacAreaInfo[%d].base = %p\n", ix, m_dacAreaInfo[ix].base);
			}
			if (m_dacAreaInfo[ix].clone < 0) {
				fprintf(stderr, "error: clone_area(): %s\n", strerror(m_dacAreaInfo[ix].clone));
				return 1;
			}
		}
		m_dacPtr[ix] = ((char *)m_dacAreaInfo[ix].base) + gosb.offset;
		m_dacBuffer[ix] = gosb.buffer;
		if (g_debug & 1) {
			fprintf(stderr, "info: m_dacPtr[%d] = %p; m_dacBuffer[%d] = %d\n", ix, m_dacPtr[ix], ix, m_dacBuffer[ix]);
		}
	}
#endif

	// allocate the 32-bit mixdown buffer (assuming 16-bit format!)
	m_mixdownBuffer = (int32*)malloc(m_dacFormat.buffer_size * 2);
	if (!m_mixdownBuffer) {
		fprintf(stderr, "error: no memory for m_mixdownBuffer\n");
		return 1;
	}

	return 0;
}

void
game_dev::set_volume(
	set_volume_info * info)
{
	if (g_debug & 1) {
		fprintf(stderr, "set_volume(%d, %g, %g)\n", info->i_id, info->i_vols[0], info->i_vols[1]);
	}
	if (info->i_id > 0) {
		status_t err = g_mix.SetChannelVolume(info->i_id, info->i_vols);
		if (err < 0) {
			fprintf(stderr, "error: SetChannelVolume(%d): %s\n", info->i_id, strerror(err));
		}
	}
	else
	if (info->i_id == -5 || info->i_id == -7) {
		// ADC control: mic and capture controls are synonymous for now, to be consistent
		// with olddev
		G<game_get_mixer_control_values> ggmcvs;
#ifdef GAME_CURRENT_API_VERSION
		ggmcvs.mixer_id = m_adcMixerInfo.mixer_id;
#endif
		ggmcvs.values = (game_mixer_control_value*)alloca(sizeof(game_mixer_control_value) * 2);
		memset(ggmcvs.values, 0, sizeof(game_mixer_control_value) * 2);
		ggmcvs.in_request_count = 2;
		ggmcvs.values[0].control_id = m_adcLevel.control_id;
		ggmcvs.values[1].control_id = m_micBoostEnable.control_id;
		if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
			if (g_debug & 1) {
				fprintf(stderr, "warning: adc: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
			}
			return;
		}

		if (!(info->i_flags & VOL_SET_VOL)) {
			if (g_debug & 2) {
				fprintf(stderr, "info->i_flags (0x%lx) & VOL_SET_VOL (0x%x) is 0; bailing\n",
					info->i_flags, VOL_SET_VOL);
			}
			return;
		}

		update_binder_property_vol(-5, info->i_vols[0], false);
		update_binder_property_vol(-7, info->i_vols[0], false);

		ggmcvs.values[1].enable.enabled = (info->i_vols[0] > 1.0 || info->i_vols[1] > 1.0) ? 1 : 0;
		if (info->i_vols[0] > 1.0) info->i_vols[0] = 1.0;
		if (info->i_vols[1] > 1.0) info->i_vols[1] = 1.0;
		if (m_adcLevel.min_value_disp > m_adcLevel.max_value_disp) {
			ggmcvs.values[0].level.values[0] = (int16)
				((1.0-info->i_vols[0])*(m_adcLevel.max_value-m_adcLevel.min_value) +
				m_adcLevel.min_value);
			ggmcvs.values[0].level.values[1] = (int16)
				((1.0-info->i_vols[1])*(m_adcLevel.max_value-m_adcLevel.min_value) +
				m_adcLevel.min_value);
		}
		else {
			ggmcvs.values[0].level.values[0] = (int16)
				(info->i_vols[0]*(m_adcLevel.max_value-m_adcLevel.min_value) +
				m_adcLevel.min_value);
			ggmcvs.values[0].level.values[1] = (int16)
				(info->i_vols[1]*(m_adcLevel.max_value-m_adcLevel.min_value) +
				m_adcLevel.min_value);
		}
		
		if (g_debug & 2) {
			fprintf(stderr, "\n==========> set mixer values: 0x%lx, 0x%lx\n\n",
				(int32)ggmcvs.values[0].level.values[0], (int32)ggmcvs.values[0].level.values[1]);
		}
		G<game_set_mixer_control_values> gsmcvs;
#ifdef GAME_CURRENT_API_VERSION
		gsmcvs.mixer_id = ggmcvs.mixer_id;
#endif
		gsmcvs.values = ggmcvs.values;
		gsmcvs.in_request_count = 2;
		if (ioctl(m_fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
			status_t err = errno;
			fprintf(stderr, "error: ioctl(GAME_SET_MIXER_CONTROL_VALUES) failed: %s\n", strerror(err));
			return;
		}
		else {
			queue_pref_save();
		}
		
	}
	else {	// DAC control
		G<game_get_mixer_control_values> ggmcvs;
#ifdef GAME_CURRENT_API_VERSION
		ggmcvs.mixer_id = m_dacMixerInfo.mixer_id;
#endif
		ggmcvs.values = (game_mixer_control_value*)alloca(sizeof(game_mixer_control_value) * 3);
		memset(ggmcvs.values, 0, sizeof(game_mixer_control_value) * 3);
		ggmcvs.in_request_count = 3;
		ggmcvs.values[0].control_id = m_mainLevel.control_id;
		ggmcvs.values[1].control_id = m_modemLevel.control_id;
		ggmcvs.values[2].control_id = m_cdLevel.control_id;
		if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
			if (g_debug & 7) {
				fprintf(stderr, "warning: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
			}
		}
		else {
			if (((info->i_id == -6) && (g_debug & 5)) ||
				((info->i_id != -6) && (g_debug & 2))) {
				fprintf(stderr, "\tGET VOLUMES:\n\t\tMAIN:  0x%04lx, 0x%04lx\n\t\tMODEM: 0x%04lx, 0x%04lx\n"
					"\t\tCD:    0x%04lx, 0x%04lx\n",
					(uint32)ggmcvs.values[0].level.values[0], (uint32)ggmcvs.values[0].level.values[1],
					(uint32)ggmcvs.values[1].level.values[0], (uint32)ggmcvs.values[1].level.values[1],
					(uint32)ggmcvs.values[2].level.values[0], (uint32)ggmcvs.values[2].level.values[1]);
			}
			bool muted = false;
			float volume = 0.f;
			bool update = true;
			switch (info->i_id) {
			case 0:
				if (g_debug & 2) {
					fprintf(stderr, "Change MAIN\n");
				}
				if (info->i_flags & VOL_SET_VOL) {
					if (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp) {
						ggmcvs.values[0].level.values[0] = (int16)
							((1.0-info->i_vols[0])*(m_mainLevel.max_value-m_mainLevel.min_value) +
							m_mainLevel.min_value);
						ggmcvs.values[0].level.values[1] = (int16)
							((1.0-info->i_vols[1])*(m_mainLevel.max_value-m_mainLevel.min_value) +
							m_mainLevel.min_value);
					}
					else {
						ggmcvs.values[0].level.values[0] = (int16)
							(info->i_vols[0]*(m_mainLevel.max_value-m_mainLevel.min_value) +
							m_mainLevel.min_value);
						ggmcvs.values[0].level.values[1] = (int16)
							(info->i_vols[1]*(m_mainLevel.max_value-m_mainLevel.min_value) +
							m_mainLevel.min_value);
					}
				}
				if (info->i_flags & VOL_SET_MUTE) {
					ggmcvs.values[0].level.flags |= GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_CLEAR_MUTE) {
					ggmcvs.values[0].level.flags &= ~GAME_LEVEL_IS_MUTED;
				}
#define STEP 4
				if (info->i_flags & VOL_SOFTER) {
					if (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp) {
						ggmcvs.values[0].level.values[0] =
							min((int)m_mainLevel.max_value, ggmcvs.values[0].level.values[0]+STEP);
						ggmcvs.values[0].level.values[1] =
							min((int)m_mainLevel.max_value, ggmcvs.values[0].level.values[1]+STEP);
					}
					else {
						ggmcvs.values[0].level.values[0] =
							max((int)m_mainLevel.min_value, ggmcvs.values[0].level.values[0]-STEP);
						ggmcvs.values[0].level.values[1] =
							max((int)m_mainLevel.min_value, ggmcvs.values[0].level.values[1]-STEP);
					}
				}
				if (info->i_flags & VOL_LOUDER) {
					if (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp) {
						ggmcvs.values[0].level.values[0] =
							max((int)m_mainLevel.min_value, ggmcvs.values[0].level.values[0]-STEP);
						ggmcvs.values[0].level.values[1] =
							max((int)m_mainLevel.min_value, ggmcvs.values[0].level.values[1]-STEP);
					}
					else {
						ggmcvs.values[0].level.values[0] =
							min((int)m_mainLevel.max_value, ggmcvs.values[0].level.values[0]+STEP);
						ggmcvs.values[0].level.values[1] =
							min((int)m_mainLevel.max_value, ggmcvs.values[0].level.values[1]+STEP);
					}
				}
				if (info->i_flags & VOL_TOGGLE_MUTE) {
					ggmcvs.values[0].level.flags ^= GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_MUTE_IF_ZERO) {
					int zero_value = (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp) ?
						m_mainLevel.max_value : m_mainLevel.min_value;
					if (ggmcvs.values[0].level.values[0] == zero_value)
					{
						ggmcvs.values[0].level.flags |= GAME_LEVEL_IS_MUTED;
					}
				}
				muted = (ggmcvs.values[0].level.flags & GAME_LEVEL_IS_MUTED) != 0;
				volume = (ggmcvs.values[0].level.values[0]-m_mainLevel.min_value)/
					(float)(m_mainLevel.max_value-m_mainLevel.min_value);
				if (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp)
				{
					volume = 1. - volume;
				}
				break;
			case -2:		//	CD
				if (g_debug & 2) {
					fprintf(stderr, "Change CD\n");
				}
				if (info->i_flags & VOL_SET_VOL) {
					if (m_cdLevel.min_value_disp > m_cdLevel.max_value_disp) {
						ggmcvs.values[2].level.values[0] = (int16)
							((1.0-info->i_vols[0])*(m_cdLevel.max_value-m_cdLevel.min_value) +
							m_cdLevel.min_value);
						ggmcvs.values[2].level.values[1] = (int16)
							((1.0-info->i_vols[1])*(m_cdLevel.max_value-m_cdLevel.min_value) +
							m_cdLevel.min_value);
					}
					else {
						ggmcvs.values[2].level.values[0] = (int16)
							(info->i_vols[0]*(m_cdLevel.max_value-m_cdLevel.min_value) +
							m_cdLevel.min_value);
						ggmcvs.values[2].level.values[1] = (int16)
							(info->i_vols[1]*(m_cdLevel.max_value-m_cdLevel.min_value) +
							m_cdLevel.min_value);
					}
				}
				if (info->i_flags & VOL_SET_MUTE) {
					ggmcvs.values[2].level.flags |= GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_CLEAR_MUTE) {
					ggmcvs.values[2].level.flags &= ~GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_MUTE_IF_ZERO) {
					int zero_value = (m_cdLevel.min_value_disp > m_cdLevel.max_value_disp) ?
						m_cdLevel.max_value : m_cdLevel.min_value;
					if (ggmcvs.values[2].level.values[0] == zero_value)
					{
						ggmcvs.values[2].level.flags |= GAME_LEVEL_IS_MUTED;
					}
				}
				muted = (ggmcvs.values[2].level.flags & GAME_LEVEL_IS_MUTED) != 0;
				volume = (ggmcvs.values[2].level.values[0]-m_mainLevel.min_value)/
					(float)(m_cdLevel.max_value-m_cdLevel.min_value);
				if (m_cdLevel.min_value_disp > m_cdLevel.max_value_disp)
				{
					volume = 1. - volume;
				}
				break;
			case -3:		//	Phone
				if (g_debug & 2) {
					fprintf(stderr, "Change PHONE\n");
				}
				if (info->i_flags & VOL_SET_VOL) {
					if (m_modemLevel.min_value_disp > m_modemLevel.max_value_disp) {
						ggmcvs.values[1].level.values[0] = (int16)
							((1.0-info->i_vols[0])*(m_modemLevel.max_value-m_modemLevel.min_value) +
							m_modemLevel.min_value);
						ggmcvs.values[1].level.values[1] = (int16)
							((1.0-info->i_vols[1])*(m_modemLevel.max_value-m_modemLevel.min_value) +
							m_modemLevel.min_value);
					}
					else {
						ggmcvs.values[1].level.values[0] = (int16)
							(info->i_vols[0]*(m_modemLevel.max_value-m_modemLevel.min_value) +
							m_modemLevel.min_value);
						ggmcvs.values[1].level.values[1] = (int16)
							(info->i_vols[1]*(m_modemLevel.max_value-m_modemLevel.min_value) +
							m_modemLevel.min_value);
					}
				}
				if (info->i_flags & VOL_SET_MUTE) {
					ggmcvs.values[1].level.flags |= GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_CLEAR_MUTE) {
					ggmcvs.values[1].level.flags &= ~GAME_LEVEL_IS_MUTED;
				}
				if (info->i_flags & VOL_MUTE_IF_ZERO) {
					int zero_value = (m_modemLevel.min_value_disp > m_modemLevel.max_value_disp) ?
						m_modemLevel.max_value : m_modemLevel.min_value;
					if (ggmcvs.values[1].level.values[0] == zero_value)
					{
						ggmcvs.values[1].level.flags |= GAME_LEVEL_IS_MUTED;
					}
				}
				muted = (ggmcvs.values[1].level.flags & GAME_LEVEL_IS_MUTED) != 0;
				volume = (ggmcvs.values[1].level.values[0]-m_modemLevel.min_value)/
					(float)(m_modemLevel.max_value-m_modemLevel.min_value);
				if (m_modemLevel.min_value_disp > m_modemLevel.max_value_disp)
				{
					volume = 1. - volume;
				}
				break;
			case -6:		//	System Events
				if (g_debug & 5) {
					fprintf(stderr, "Change SYSTEM EVENTS (%s%s%s) %g\n",
						(info->i_flags & VOL_SET_VOL) ? "setvol " : "",
						(info->i_flags & VOL_SET_MUTE) ? "mute " : "",
						(info->i_flags & VOL_CLEAR_MUTE) ? "unmute " : "");
				}
				if (info->i_flags & VOL_SET_VOL) {
					m_serverState.event_gain = info->i_vols[0];
				}
				if (info->i_flags & VOL_SET_MUTE) {
					m_serverState.event_mute = true;
				}
				if (info->i_flags & VOL_CLEAR_MUTE) {
					m_serverState.event_mute = false;
				}
				if (info->i_flags & VOL_MUTE_IF_ZERO) {
					if (m_serverState.event_gain == 0) {
						m_serverState.event_mute = true;
					}
				}
				muted = m_serverState.event_mute;
				volume = m_serverState.event_gain;
				break;
			default:
				update = false;
				fprintf(stderr, "error: set_volume(%d): unknown channel\n", info->i_id);
				return;
			}
			if (update)
			{
				if (g_debug & 8)
				{
					fprintf(stderr, "channel %d, volume %f, mute %s\n", info->i_id, volume,
						muted ? "true" : "false");
				}
				update_binder_property_vol(info->i_id, volume, muted);
			}
		}
		G<game_set_mixer_control_values> gsmcvs;
#ifdef GAME_CURRENT_API_VERSION
		gsmcvs.mixer_id = ggmcvs.mixer_id;
#endif
		gsmcvs.values = ggmcvs.values;
		gsmcvs.in_request_count = 3;
			if (((info->i_id == -6) && (g_debug & 5)) ||
				((info->i_id != -6) && (g_debug & 2))) {
			fprintf(stderr, "\tSET VOLUMES:\n\t\tMAIN:  0x%04lx, 0x%04lx\n\t\tMODEM: 0x%04lx, 0x%04lx\n"
				"\t\tCD:    0x%04lx, 0x%04lx\n",
				(uint32)ggmcvs.values[0].level.values[0], (uint32)ggmcvs.values[0].level.values[1],
				(uint32)ggmcvs.values[1].level.values[0], (uint32)ggmcvs.values[1].level.values[1],
				(uint32)ggmcvs.values[2].level.values[0], (uint32)ggmcvs.values[2].level.values[1]);
		}
		if (ioctl(m_fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
			status_t err = errno;
			fprintf(stderr, "error: ioctl(GAME_SET_MIXER_CONTROL_VALUES) failed: %s\n", strerror(err));
			return;
		}
		else {
			queue_pref_save();
		}
	}
}

void
game_dev::get_volume(
	get_volume_info * info)
{
	get_vols_reply gvr;
	memset(gvr.vols, 0, sizeof(gvr.vols));
	bool mute = false;
	status_t err = B_OK;
	if (info->i_id > 0) {
		err = g_mix.GetChannelVolume(info->i_id, gvr.vols);
		if (err < 0) {
			fprintf(stderr, "error: GetChannelVolume(%d): %s\n", info->i_id, strerror(err));
		}
	}
	else
	if (info->i_id == -5 || info->i_id == -7) {
		// ADC control: mic and capture controls are synonymous for now, to be consistent
		// with olddev
		G<game_get_mixer_control_values> ggmcvs;
		ggmcvs.values = (game_mixer_control_value*)alloca(sizeof(game_mixer_control_value) * 2);
		memset(ggmcvs.values, 0, sizeof(game_mixer_control_value) * 2);
#ifdef GAME_CURRENT_API_VERSION
		ggmcvs.mixer_id = m_adcMixerInfo.mixer_id;
#endif
		ggmcvs.in_request_count = 2;
		ggmcvs.values[0].control_id = m_adcLevel.control_id;
		ggmcvs.values[1].control_id = m_micBoostEnable.control_id;
		if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
			if (g_debug & 1) {
				fprintf(stderr, "warning: adc: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
			}
			err = B_ERROR;
		}
		else {
			gvr.vols[0] = (ggmcvs.values[0].level.values[0]-m_adcLevel.min_value) /
				(float)(m_adcLevel.max_value-m_adcLevel.min_value);
			gvr.vols[1] = (ggmcvs.values[0].level.values[1]-m_adcLevel.min_value) /
				(float)(m_adcLevel.max_value-m_adcLevel.min_value);
			mute = false;
			if (m_adcLevel.min_value_disp > m_adcLevel.max_value_disp) {
				gvr.vols[0] = 1. - gvr.vols[0];
				gvr.vols[1] = 1. - gvr.vols[1];
			}
			if (ggmcvs.values[1].enable.enabled) {
				gvr.vols[0] += 1.0;
				gvr.vols[1] += 1.0;
			}
		}
	}
	else {	// DAC control
		G<game_get_mixer_control_values> ggmcvs;
		ggmcvs.values = (game_mixer_control_value*)alloca(sizeof(game_mixer_control_value) * 3);
		memset(ggmcvs.values, 0, sizeof(game_mixer_control_value) * 3);
#ifdef GAME_CURRENT_API_VERSION
		ggmcvs.mixer_id = m_dacMixerInfo.mixer_id;
#endif
		ggmcvs.in_request_count = 3;
		ggmcvs.values[0].control_id = m_mainLevel.control_id;
		ggmcvs.values[1].control_id = m_modemLevel.control_id;
		ggmcvs.values[2].control_id = m_cdLevel.control_id;
		if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
			if (g_debug & 1) {
				fprintf(stderr, "warning: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
			}
			err = B_ERROR;
		}
		else {
			if (g_debug & 2) {
				fprintf(stderr, "\tGET VOLUMES:\n\t\tMAIN:  0x%04lx, 0x%04lx\n\t\tMODEM: 0x%04lx, 0x%04lx\n"
					"\t\tCD:    0x%04lx, 0x%04lx\n",
					(uint32)ggmcvs.values[0].level.values[0], (uint32)ggmcvs.values[0].level.values[1],
					(uint32)ggmcvs.values[1].level.values[0], (uint32)ggmcvs.values[1].level.values[1],
					(uint32)ggmcvs.values[2].level.values[0], (uint32)ggmcvs.values[2].level.values[1]);
			}
			switch (info->i_id) {
			case 0:
				gvr.vols[0] = (ggmcvs.values[0].level.values[0]-m_mainLevel.min_value)/
					(float)(m_mainLevel.max_value-m_mainLevel.min_value);
				gvr.vols[1] = (ggmcvs.values[0].level.values[1]-m_mainLevel.min_value)/
					(float)(m_mainLevel.max_value-m_mainLevel.min_value);
				mute = (ggmcvs.values[0].level.flags & GAME_LEVEL_IS_MUTED) ? true : false;
				if (m_mainLevel.min_value_disp > m_mainLevel.max_value_disp)
				{
					gvr.vols[0] = 1. - gvr.vols[0];
					gvr.vols[1] = 1. - gvr.vols[1];
				}
				if (g_debug & 2) {
					fprintf(stderr, "====> main volume = %g, %g\n", gvr.vols[0], gvr.vols[1]);
				}
				break;

			case -2:	//	CD
				gvr.vols[0] = (ggmcvs.values[2].level.values[0]-m_cdLevel.min_value)/
					(float)(m_mainLevel.max_value-m_cdLevel.min_value);
				gvr.vols[1] = (ggmcvs.values[2].level.values[1]-m_cdLevel.min_value)/
					(float)(m_cdLevel.max_value-m_cdLevel.min_value);
				mute = (ggmcvs.values[2].level.flags & GAME_LEVEL_IS_MUTED) ? true : false;
				if (m_cdLevel.min_value_disp > m_cdLevel.max_value_disp)
				{
					gvr.vols[0] = 1. - gvr.vols[0];
					gvr.vols[1] = 1. - gvr.vols[1];
				}
				if (g_debug & 2) {
					fprintf(stderr, "====> cd volume = %g, %g\n", gvr.vols[0], gvr.vols[1]);
				}
				break;

			case -3: // phone
				gvr.vols[0] = (ggmcvs.values[1].level.values[0]-m_modemLevel.min_value)/
					(float)(m_modemLevel.max_value-m_modemLevel.min_value);
				gvr.vols[1] = (ggmcvs.values[1].level.values[1]-m_modemLevel.min_value)/
					(float)(m_modemLevel.max_value-m_modemLevel.min_value);
				mute = (ggmcvs.values[1].level.flags & GAME_LEVEL_IS_MUTED) ? true : false;
				if (m_modemLevel.min_value_disp > m_modemLevel.max_value_disp)
				{
					gvr.vols[0] = 1. - gvr.vols[0];
					gvr.vols[1] = 1. - gvr.vols[1];
				}
				if (g_debug & 2) {
					fprintf(stderr, "====> phone volume = %g, %g\n", gvr.vols[0], gvr.vols[1]);
				}
				break;
			case -6:
				gvr.vols[0] = m_serverState.event_gain;
				gvr.vols[1] = m_serverState.event_gain;
				mute = m_serverState.event_mute;
				if (g_debug & 2) {
					fprintf(stderr, "====> event volume = %g, %g\n", gvr.vols[0], gvr.vols[1]);
				}
				break;
			default:
				if (g_debug & 1)
				{
					fprintf(stderr, "error: unknown stream id %d\n", info->i_id);
				}
				err = B_ERROR;
				break;
			}
		}
	}
	if (err < 0) {
		if (g_debug & 1) {
			fprintf(stderr, "Error getting volume.\n");
		}
		write_port_etc(info->i_port, -1, 0, 0, B_TIMEOUT, SND_TIMEOUT);
	}
	else {
		gvr.mute = mute;
		if (g_debug & 2) {
			fprintf(stderr, "mute is %s; returning to port %d.\n", mute ? "true" : "false", info->i_port);
		}
		write_port_etc(info->i_port, 0, &gvr, sizeof(gvr), B_TIMEOUT, SND_TIMEOUT);
	}
}

void 
game_dev::set_adc_mux(int32 stream_id)
{
	G<game_get_mixer_control_values> ggmcvs;
#ifdef GAME_CURRENT_API_VERSION
	ggmcvs.mixer_id = m_adcMixerInfo.mixer_id;
#endif
	H<game_mixer_control_value> gmcv;
	gmcv.control_id = m_adcMux.control_id;
	ggmcvs.in_request_count = 1;
	ggmcvs.values = &gmcv;
	if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
		if (g_debug & 1) {
			fprintf(stderr, "warning: adc: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
		}
		return;
	}
	update_binder_property_adc(stream_id);
	switch (stream_id) {
		case miniMainOut:
			gmcv.mux.mask = m_adcMuxValues[ADC_MUX_MAIN_OUT];
			break;
		case miniCDIn:
			gmcv.mux.mask = m_adcMuxValues[ADC_MUX_CD_IN];
			break;
		case miniLineIn:
			gmcv.mux.mask = m_adcMuxValues[ADC_MUX_LINE_IN];
			break;
		case miniMicIn:
			gmcv.mux.mask = m_adcMuxValues[ADC_MUX_MIC_IN];
			break;
		default:
			fprintf(stderr, "error: adc: bad mux stream %ld\n", stream_id);
			return;
	}
	G<game_set_mixer_control_values> gsmcvs;
#ifdef GAME_CURRENT_API_VERSION
	gsmcvs.mixer_id = ggmcvs.mixer_id;
#endif
	gsmcvs.values = &gmcv;
	gsmcvs.in_request_count = 1;
	if (ioctl(m_fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
		if (g_debug & 1) {
			fprintf(stderr, "warning: adc: GAME_SET_MIXER_CONTROL_VALUES failed.\n");
		}
		return;
	}
}

int32 
game_dev::get_adc_mux()
{
	G<game_get_mixer_control_values> ggmcvs;
#ifdef GAME_CURRENT_API_VERSION
	ggmcvs.mixer_id = m_adcMixerInfo.mixer_id;
#endif
	H<game_mixer_control_value> gmcv;
	gmcv.control_id = m_adcMux.control_id;
	ggmcvs.values = &gmcv;
	ggmcvs.in_request_count = 1;
	if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
		if (g_debug & 1) {
			fprintf(stderr, "warning: adc: GAME_GET_MIXER_CONTROL_VALUES failed.\n");
		}
		return LONG_MIN;
	}
	if (gmcv.mux.mask == m_adcMuxValues[ADC_MUX_MAIN_OUT]) return miniMainOut;
	else if(gmcv.mux.mask == m_adcMuxValues[ADC_MUX_CD_IN]) return miniCDIn;
	else if(gmcv.mux.mask == m_adcMuxValues[ADC_MUX_LINE_IN]) return miniLineIn;
	else if(gmcv.mux.mask == m_adcMuxValues[ADC_MUX_MIC_IN]) return miniMicIn;
	else {
		if (g_debug & 1) fprintf(stderr,
			"warning: adc: no mapping for adc mux mask 0x%x\n", gmcv.mux.mask);
		return LONG_MIN;
	}
}

media_multi_audio_format 
game_dev::dac_format()
{
	return m_dacFormat;
}

media_multi_audio_format 
game_dev::adc_format()
{
	return m_adcFormat;
}

status_t 
game_dev::validate_new_mix_channel(new_mixchannel_info *io_info)
{
	ASSERT(io_info);
	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	status_t err = B_OK;
	if (io_info->io_format.frame_rate == w.frame_rate)
	{
		io_info->io_format.frame_rate = m_dacFormat.frame_rate;
	}
	// wildcards for channel_count and format don't make sense
	if (io_info->io_format.channel_count < 1 ||
		io_info->io_format.channel_count > m_dacFormat.channel_count)
	{
		io_info->io_format.channel_count = m_dacFormat.channel_count;
		err = B_MEDIA_BAD_FORMAT;
	}
	if (io_info->io_format.format == w.format ||
		(io_info->io_format.format != media_multi_audio_format::B_AUDIO_UCHAR &&
		io_info->io_format.format != media_multi_audio_format::B_AUDIO_SHORT))
	{
		io_info->io_format.format = m_dacFormat.format;
		err = B_MEDIA_BAD_FORMAT;
	}
	if (io_info->io_format.byte_order != w.byte_order &&
		io_info->io_format.byte_order != B_MEDIA_HOST_ENDIAN)
	{
		err = B_MEDIA_BAD_FORMAT;
	}
	io_info->io_format.byte_order = B_MEDIA_HOST_ENDIAN;
	if (io_info->io_format.buffer_size == w.buffer_size)
	{
		io_info->io_format.buffer_size = m_dacFormat.buffer_size;
	}
	return err;
}


// ------------------------------------------------------------------------ //
// buffer management

status_t 
game_dev::alloc_mix_buffers(int32 channel_id, size_t data_size, int32 count, cloned_buffer **o_buffers)
{
	status_t err;
	if(count < 1)
		return B_BAD_VALUE;

#ifdef GAME_CURRENT_API_VERSION
	cloned_buffer_set* s = new cloned_buffer_set(*this, channel_id, count, false);

	size_t area_size = (data_size * count + B_PAGE_SIZE-1) & -B_PAGE_SIZE;
	void* base;
	area_id area = create_area(
		"mix.buffers", &base, B_ANY_ADDRESS, area_size,
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		fprintf(stderr, "error: create_area(): %s\n", strerror(area));
		return area;
	}
	int32 offset = 0;
	for(int32 n = 0; n < count; n++, offset += data_size)
	{
		// describe buffer
		s->buffers[n].id = m_nextMixBufferID++;
		s->buffers[n].area = area;
		s->buffers[n].local_area = area;
		s->buffers[n].offset = offset;
		s->buffers[n].size = data_size;
		s->buffers[n].size_used = 0;
		s->buffers[n].next_queued = 0;
		s->buffers[n].data = ((int8*)base) + offset;

		// map buffer ID to this channel		
		m_mixChannelMap.insert(
			mix_channel_map::value_type(s->buffers[n].id, channel_id));
		
		// return it
		o_buffers[n] = &s->buffers[n];
	}	
#else
	cloned_buffer_set* s = new cloned_buffer_set(*this, channel_id, count);

	for(int32 n = 0; n < count; n++)
	{
		// ask driver for a buffer
		G<game_open_stream_buffer> gosb;
		gosb.stream = GAME_NO_ID; //m_dacStreamID;
		gosb.byte_size = data_size;
		err = ioctl(m_fd, GAME_OPEN_STREAM_BUFFER, &gosb);

		if (err < 0)
		{
			perror("error: ioctl(GAME_OPEN_STREAM_BUFFER)");
			goto _bail;
		}

		// paranoia
		if(gosb.buffer == GAME_NO_ID)
		{
			fprintf(stderr, "alloc_mix_buffers(): ioctl(GAME_OPEN_STREAM_BUFFER): GAME_NO_ID\n");
			goto _bail;
		}

		// describe the buffer		
		s->buffers[n].id = gosb.buffer;
		s->buffers[n].area = gosb.area;
		s->buffers[n].offset = gosb.offset;
		s->buffers[n].size = data_size;
		s->buffers[n].size_used = 0;
		s->buffers[n].next_queued = 0;
		s->buffers[n].local_area = -1;

		// clone the area only if it hasn't already been cloned
		for(int32 prev = 0; prev < n; prev++)
		{
			if(s->buffers[prev].area == s->buffers[n].area)
			{
				s->buffers[n].local_area = s->buffers[prev].local_area;
				s->buffers[n].data = (int8*)s->buffers[prev].data +
					s->buffers[n].offset - s->buffers[prev].offset;
				break;
			}
		}

		if(s->buffers[n].local_area < 0)
		{
			void* base;
			area_id cloned = clone_area(
				"mixbuffer clone",
				&base,
				B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA,
				s->buffers[n].area);
			if(cloned < 0)
			{
				perror("error: alloc_mix_buffers(): clone_area()");
				goto _bail;
			}
			s->buffers[n].local_area = cloned;
			s->buffers[n].data = (int8*)base + s->buffers[n].offset;
		}

		// map buffer ID to this channel		
		m_mixChannelMap.insert(
			mix_channel_map::value_type(s->buffers[n].id, channel_id));
		
		// return it
		o_buffers[n] = &s->buffers[n];
	}
#endif

	s->next = m_mixBufferSets;
	m_mixBufferSets = s;

	return B_OK;

_bail:
	delete s;
	memset(o_buffers, 0, count * sizeof(cloned_buffer*));
	return B_ERROR;
}

status_t 
game_dev::find_mix_buffer(int32 buffer_id, int32 *o_channel_id, cloned_buffer **o_buffer)
{
	mix_channel_map::const_iterator it = m_mixChannelMap.find(buffer_id);
	if(it == m_mixChannelMap.end())
	{
		fprintf(stderr, "game_dev::find_mix_buffer(): buffer ID %ld not in map.\n",
			buffer_id);
		return B_BAD_VALUE;
	}
	
	*o_channel_id = (*it).second;
	
	cloned_buffer_set* s = m_mixBufferSets;
	for(; s && s->channel_id != *o_channel_id; s = s->next) {}
	if(!s)
	{
		fprintf(stderr, "game_dev::find_mix_buffer(): mix channel %ld not found.\n",
			*o_channel_id);
		return B_ERROR;
	}
	
	// linear search starting with the buffer immediately following the
	// last one fetched

	int32 next = s->next_buffer_index;
	
	for(int32 n = next; n < s->buffer_count; n++)
	{
		if(s->buffers[n].id == buffer_id)
		{
			s->next_buffer_index = n + 1;
			*o_buffer = &s->buffers[n];
			return B_OK;
		}
	}
	for(int32 n = 0; n < next; n++)
	{
		if(s->buffers[n].id == buffer_id)
		{
			s->next_buffer_index = n + 1;
			*o_buffer = &s->buffers[n];
			return B_OK;
		}
	}

	fprintf(stderr, "game_dev::find_mix_buffer(): buffer ID %ld not found.\n",
		buffer_id);
	return B_ERROR;
}

status_t 
game_dev::free_mix_buffers(int32 channel_id)
{
	cloned_buffer_set** b = &m_mixBufferSets;
	while(*b)
	{
		if((*b)->channel_id == channel_id)
		{
			for(int32 n = 0; n < (*b)->buffer_count; n++)
			{
				m_mixChannelMap.erase((*b)->buffers[n].id);
			}
			cloned_buffer_set* dead = *b;
			*b = dead->next;
			delete dead;
			return B_OK;
		}
		else
			b = &((*b)->next);
	}
	return B_BAD_VALUE;
}

void 
game_dev::free_all_dac_buffers()
{
	for(cloned_buffer_set* b = m_mixBufferSets; b;)
	{
		cloned_buffer_set* prev = b;
		b = b->next;
		delete prev;
	}
	m_mixBufferSets = 0;
	m_mixChannelMap.clear();
	if (m_mixdownBuffer) {
		free(m_mixdownBuffer);
		m_mixdownBuffer = 0;
	}
}

status_t 
game_dev::new_capture_channel(new_capturechannel_info *io_info)
{
	status_t err;	
	MiniAutolock _l(m_captureLock);

	bool needOpen = !GAME_IS_STREAM(m_adcStreamID);
	if (needOpen)
	{
		err = open_adc_stream();
		if (err < B_OK)
		{
			fprintf(stderr,
				"game_dev::new_capture_channel():\n\t"
				"open_adc_stream(): %s\n", strerror(err));
			return err;
		}
	}

	ASSERT(GAME_IS_STREAM(m_adcStreamID));
	ASSERT(m_captureBufferSet);

	bool needStart = needOpen || !m_runCaptureThread;
	
	// impose a format on our hapless client
	io_info->o_format = m_adcFormat;
	io_info->io_buffer_count = m_captureBufferSet->buffer_count;

	// describe stream buffers
	for (int32 n = 0; n < m_captureBufferSet->buffer_count; n++)
	{
		memcpy(&io_info->o_buffers[n], &m_captureBufferSet->buffers[n], sizeof(minisnd_buffer));
	}
	
	capture_channel* c = new capture_channel;
	c->id = m_nextCaptureChannelID++;
	c->sem = io_info->i_sem;
	c->next = m_captureChannels;
	m_captureChannels = c;

	io_info->o_id = c->id;
	io_info->o_first_buffer_index = m_nextBufferIndex;

	if (needStart)
	{
		err = start_adc_stream();
		if (err < B_OK)
		{
			fprintf(stderr,
				"game_dev::new_capture_channel():\n\t"
				"start_adc_stream(): %s\n", strerror(err));
		}
	}

	return B_OK;
}

status_t 
game_dev::remove_capture_channel(int32 channelID)
{
	MiniAutolock _l(m_captureLock);
	capture_channel** c = &m_captureChannels;
	while (*c)
	{
		if ((*c)->id == channelID)
		{
			capture_channel* d = *c;
			*c = d->next;
			delete d;
			return B_OK;
		}
		else
		{
			c = &(*c)->next;
		}
	}
	return B_BAD_INDEX;
}


