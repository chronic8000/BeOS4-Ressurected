
#if !defined(gamedev_h)
#define gamedev_h

#include "snddev.h"
#include "mixerinfo.h"
#include "miniapp.h"
#include "minilocker.h"

#include <map>
#include <limits.h>

#define GAMEBUFCNT g_gameBufCnt
extern int g_gameBufCnt;
#define MAXGAMEBUFCNT 6


template<class C> class G : public C {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class C> class H : public C {
public:
	H() { memset(this, 0, sizeof(*this)); }
};

class game_dev : public snd_dev {
public:
	game_dev();
	~game_dev();
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
	status_t alloc_mix_buffers(
		int32 channel_id,
		size_t size,
		int32 count,
		cloned_buffer ** o_buffers);
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

	void pref_load();
	void free_all_dac_buffers();
	status_t init_adc(const media_multi_audio_format& adc_format);
	status_t open_adc_stream();
	status_t close_adc_stream();
	status_t start_adc_stream();
	status_t stop_adc_stream();
	static status_t capture_thread_entry(void * cookie);
	void capture_thread();

	// look for a mixer likely to match the given codec (either explicitly linked,
	// or containing control(s) matching the codec type.)  in the event of a match,
	// call handle_xxx_control() for each control.
	status_t scan_codec_mixer(
		int32 codecID, game_mixer_info* outMixerInfo);
	void handle_level_control(
		int32 codecID, const game_mixer_info& mixer, const game_get_mixer_level_info& control);
	void handle_mux_control(
		int32 codecID, const game_mixer_info& mixer, const game_get_mixer_mux_info& control);
	void handle_enable_control(
		int32 codecID, const game_mixer_info& mixer, const game_get_mixer_enable_info& control);

	// state info in order of appearance in the settings file
	mixerstate				m_mixerState;
	game_setup				m_serverState;

	int						m_fd;

	// DAC state	

	sem_id					m_dacStreamSem;
	int						m_dacStreamID;
	int32					m_dacBufferFrames;
	media_multi_audio_format	m_dacFormat;

#ifdef GAME_CURRENT_API_VERSION
	area_id					m_dacLocalArea;
	void *					m_dacPtr;
	int32					m_dacBufferID;
#else
	struct {
		area_id orig;
		area_id clone;
		void * base;
	}						m_dacAreaInfo[MAXGAMEBUFCNT];
	void *					m_dacPtr[MAXGAMEBUFCNT];
	int32					m_dacBuffer[MAXGAMEBUFCNT];
#endif

	game_mixer_info				m_dacMixerInfo;
	game_get_mixer_level_info 	m_cdLevel;
	game_get_mixer_level_info 	m_mainLevel;
	game_get_mixer_level_info 	m_modemLevel;

	class cloned_buffer_set;
	friend class game_dev::cloned_buffer_set;
	cloned_buffer_set *		m_mixBufferSets;
	int32					m_nextMixBufferID;

	typedef std::map<int32,int32> mix_channel_map; // bufferID -> channelID
	mix_channel_map			m_mixChannelMap;
	
	int32 *					m_mixdownBuffer;
	bigtime_t				m_dacBufferDuration;

	// ADC state

	volatile sem_id			m_adcStreamSem;
	volatile int			m_adcStreamID;
	int32					m_adcBufferFrames;
	media_multi_audio_format	m_adcFormat;

	game_mixer_info				m_adcMixerInfo;
	game_get_mixer_level_info	m_adcLevel;
	game_get_mixer_enable_info	m_micBoostEnable;
	game_get_mixer_mux_info		m_adcMux;
	enum adc_mux_index_t {
		ADC_MUX_MAIN_OUT,
		ADC_MUX_CD_IN,
		ADC_MUX_LINE_IN,
		ADC_MUX_MIC_IN,
		ADC_MUX_INDEX_COUNT
	};
	uint32						m_adcMuxValues[ADC_MUX_INDEX_COUNT];

	MiniLocker				m_captureLock;
	volatile thread_id		m_captureThread;
	volatile bool			m_runCaptureThread;
	uint32					m_nextCaptureChannelID;
	uint32					m_nextBufferIndex;
	cloned_buffer_set *		m_captureBufferSet;
	struct capture_channel {
		capture_channel() : id(GAME_NO_ID), sem(-1), next(0) {}
		int32				id;
		sem_id				sem;
		capture_channel*	next;
	};
	capture_channel *		m_captureChannels;
};



#endif	//	gamedev_h

