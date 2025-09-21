
#if !defined(olddev_h)
#define olddev_h

#include "snddev.h"
#include "minilocker.h"
#include <unistd.h>
#include <OS.h>

#include <map>

class old_dev : public snd_dev {
public:
	old_dev();
	~old_dev();
	bigtime_t do_mix();
	status_t pref_save();
	int setup(
		const char * dev,
		const media_multi_audio_format& dac_format,
		const media_multi_audio_format& adc_format);
	void set_volume(set_volume_info * info);
	void get_volume(get_volume_info * info);
	void set_adc_mux(int32 stream_id);
	int32 get_adc_mux();

	media_multi_audio_format dac_format();
	media_multi_audio_format adc_format();
	
	status_t validate_new_mix_channel(
		new_mixchannel_info* io_info);
	// o_buffers is an array of cloned_buffer pointers to be filled
	status_t alloc_mix_buffers(
		int32 channel_id,
		size_t size,
		int32 count,
		cloned_buffer ** o_buffers);
	// return the appropriate cloned_buffer pointer in o_buffer 
	status_t find_mix_buffer(
		int32 buffer_id,
		int32 * o_channel_id,
		cloned_buffer ** o_buffer);
	status_t free_mix_buffers(
		int32 channel_id);

	status_t new_capture_channel(
		new_capturechannel_info* io_info);
	status_t remove_capture_channel(
		int32 channel_id);

private:
	class cloned_buffer_set;
	class capture_channel;

	void pref_load(minisnd_setup * setup);
	void free_all_dac_buffers();

	static status_t capture_thread_entry(void * cookie);
	void capture_thread();
	// requires m_captureLock:
	status_t feed_capture_channel(capture_channel * channel, void * buffer, size_t size);
	status_t start_capture_thread();
	void wait_for_capture_thread();

	int m_fd;
	sem_id m_output_sem;
	area_id m_output_area;
	void * m_output_ptr;
	sem_id m_input_sem;
	area_id m_input_area;
	void * m_input_ptr;
	minisnd_setup m_setup;
	
	cloned_buffer_set *	m_mixBufferSets;
	// bufferID -> channelID
	typedef std::map<int32,int32> mix_channel_map;
	mix_channel_map		m_mixChannelMap;
	int32				m_nextMixBufferID;

	size_t				m_codecFrameSize;
	int32				m_codecBufferFrames; 
	int32 *				m_mixdownBuffer;
	bigtime_t			m_bufferDuration;

	volatile bool		m_runCaptureThread;
	volatile thread_id	m_captureThread;
	MiniLocker			m_captureLock;
	capture_channel *	m_captureChannels;
	int32				m_nextCaptureChannelID;
	int32				m_nextCaptureBufferID;
	
	media_multi_audio_format	m_codecFormat;
};


#endif	//	olddev_h
