
/*
 *	Definition for a game_audio device driver.
 *	Copyright 2000 Be, Incorporated. All rights reserved.
 */

/*
 *	TODO: should we remove queue_stream_data and require user
 *	to always use allocated buffers? That would make the driver
 *	model simpler, and avoid a who-copies-what-when problem.
 */

#if !defined(game_audio_h)
#define game_audio_h

#include <Drivers.h>
//#include <Debug.h>
#include <audio_base.h>

#define B_GAME_DRIVER_BASE (B_AUDIO_DRIVER_BASE+60)

#if !defined(__cplusplus)
typedef enum game_audio_opcode game_audio_opcode;
typedef enum game_dac_info_stream_flags game_dac_info_stream_flags;
typedef enum game_codec_format_flags game_codec_format_flags;
typedef enum game_mixer_control_type game_mixer_control_type;
typedef enum game_mixer_control_flags game_mixer_control_flags;
typedef enum game_mixer_level_info_flags game_mixer_level_info_flags;
typedef enum game_mixer_level_info_type game_mixer_level_info_type;
typedef enum game_mixer_mux_info_flags game_mixer_mux_info_flags;
typedef enum game_mixer_mux_info_type game_mixer_mux_info_type;
typedef enum game_mixer_enable_info_flags game_mixer_enable_info_flags;
typedef enum game_mixer_level_value_flags game_mixer_level_value_flags;
typedef enum game_mixer_control_value_status game_mixer_control_value_status;
typedef enum game_open_stream_request game_open_stream_request;
typedef enum game_stream_state game_stream_state;
typedef enum game_stream_description_mapping game_stream_description_mapping;
typedef enum game_queue_stream_data_flags game_queue_stream_data_flags;
typedef enum game_close_stream_flags game_close_stream_flags;
typedef enum game_open_stream_buffer_flags game_open_stream_buffer_flags;
#endif

typedef struct game_get_info game_get_info;
typedef struct game_dac_info game_dac_info;
typedef struct game_get_dac_infos game_get_dac_infos;
typedef struct game_adc_info game_adc_info;
typedef struct game_get_adc_infos game_get_adc_infos;
typedef struct game_codec_format game_codec_format;
typedef struct game_set_codec_formats game_set_codec_formats;
typedef struct game_mixer_info game_mixer_info;
typedef struct game_get_mixer_infos game_get_mixer_infos;
typedef struct game_mixer_control game_mixer_control;
typedef struct game_get_mixer_controls game_get_mixer_controls;
typedef struct game_get_mixer_level_info game_get_mixer_level_info;
typedef struct game_mux_item game_mux_item;
typedef struct game_get_mixer_mux_info game_get_mixer_mux_info;
typedef struct game_get_mixer_enable_info game_get_mixer_enable_info;
typedef struct game_mixer_level_value game_mixer_level_value;
typedef struct game_mixer_mux_value game_mixer_mux_value;
typedef struct game_mixer_enable_value game_mixer_enable_value;
typedef struct game_mixer_control_value game_mixer_control_value;
typedef struct game_get_mixer_control_values game_get_mixer_control_values;
typedef struct game_set_mixer_control_values game_set_mixer_control_values;
typedef struct game_open_stream game_open_stream;
typedef struct game_stream_timing game_stream_timing;
typedef struct game_get_stream_timing game_get_stream_timing;
typedef struct game_get_stream_description game_get_stream_description;
typedef struct game_stream_controls game_stream_controls;
typedef struct game_get_stream_controls game_get_stream_controls;
typedef struct game_set_stream_controls game_set_stream_controls;
typedef struct game_queue_stream_data game_queue_stream_data;
typedef struct game_queue_stream_buffer game_queue_stream_buffer;
typedef struct game_run_stream game_run_stream;
typedef struct game_run_streams game_run_streams;
typedef struct game_close_stream game_close_stream;
typedef struct game_open_stream_buffer game_open_stream_buffer;
typedef struct game_close_stream_buffer game_close_stream_buffer;
typedef struct game_get_device_state game_get_device_state;
typedef struct game_set_device_state game_set_device_state;
typedef struct game_get_interface_info game_get_interface_info;
typedef struct game_get_interfaces game_get_interfaces;

enum game_audio_opcode {
	/* get general hardware (card) info */
	GAME_GET_INFO = B_GAME_DRIVER_BASE,
	/* get info on specific inputs/outputs, and configure them */
	GAME_GET_DAC_INFOS,
	GAME_GET_ADC_INFOS,
	GAME_SET_CODEC_FORMATS,
	/* mixer information */
	GAME_GET_MIXER_INFOS,
	GAME_GET_MIXER_CONTROLS,
	GAME_GET_MIXER_LEVEL_INFO,
	GAME_GET_MIXER_MUX_INFO,
	GAME_GET_MIXER_ENABLE_INFO,
	/*	we specifically omit specific control connection info for now	*/
	GAME_MIXER_UNUSED_OPCODE_1_,
	/* mixer control values */
	GAME_GET_MIXER_CONTROL_VALUES,
	GAME_SET_MIXER_CONTROL_VALUES,
	/* stream management */
	GAME_OPEN_STREAM,
	GAME_GET_STREAM_TIMING,
	GAME_GET_STREAM_DESCRIPTION,
	GAME_GET_STREAM_CONTROLS,
	GAME_SET_STREAM_CONTROLS,
	_GAME_QUEUE_UNUSED_OPCODE_2_,
	GAME_QUEUE_STREAM_BUFFER,
	GAME_RUN_STREAMS,
	GAME_CLOSE_STREAM,
	/* buffers tied to streams (looping buffer, typically) */
	GAME_OPEN_STREAM_BUFFER,
	GAME_CLOSE_STREAM_BUFFER,
	/* save/load state */
	GAME_GET_DEVICE_STATE,
	GAME_SET_DEVICE_STATE,
	/* extensions (interfaces) */
	GAME_GET_INTERFACE_INFO,
	GAME_GET_INTERFACES
};

/*
 *	DACs, ADCs, MIXERs, CONTROLs and BUFFERs all live in the same ID name
 *	space within the device.
 */
#define GAME_IS_DAC(x) (((x)&0xff00) == 0x4400)
#define GAME_DAC_ORDINAL(x) ((x)&0xff)
#define GAME_MAKE_DAC_ID(o) (((o)&0xff) | 0x4400)
#define GAME_MAX_DAC_COUNT 256

#define GAME_IS_ADC(x) (((x)&0xff00) == 0x4800)
#define GAME_ADC_ORDINAL(x) ((x)&0xff)
#define GAME_MAKE_ADC_ID(o) (((o)&0xff) | 0x4800)
#define GAME_MAX_ADC_COUNT 256

#define GAME_IS_MIXER(x) (((x)&0xff00) == 0x4c00)
#define GAME_MIXER_ORDINAL(x) ((x)&0xff)
#define GAME_MAKE_MIXER_ID(o) (((o)&0xff) | 0x4c00)
#define GAME_MAX_MIXER_COUNT 256

#define GAME_IS_CONTROL(x) (((x)&0xf000) == 0x0000)
#define GAME_CONTROL_ORDINAL(x) ((x)&0xfff)
#define GAME_MAKE_CONTROL_ID(o) (o)
#define GAME_MAX_CONTROL_COUNT 4096

#define GAME_IS_STREAM(x) (((x)&0xf000) == 0x2000)
#define GAME_STREAM_ORDINAL(x) ((x)&0xfff)
#define GAME_MAKE_STREAM_ID(o) ((o) | 0x2000)
#define GAME_MAX_STREAM_COUNT 4096

#define GAME_IS_BUFFER(x) (((x)&0xf000) == 0x6000)
#define GAME_BUFFER_ORDINAL(x) ((x)&0xfff)
#define GAME_MAKE_BUFFER_ID(o) ((o) | 0x6000)
#define GAME_MAX_BUFFER_COUNT 4096

#define GAME_NO_ID (-1)

struct game_get_info  {

	size_t	info_size;

	char	name[32];
	char	vendor[32];
	int16	ordinal;
	int16	version;

	int16	dac_count;
	int16	adc_count;
	int16	mixer_count;
	int16	buffer_count;		/* maximum number of buffers for device
								(0 if "until you run out of memory or buffer IDs") */
};

enum game_dac_info_stream_flags {
	GAME_STREAMS_HAVE_VOLUME = 0x1,
	GAME_STREAMS_HAVE_PAN = 0x2,
	GAME_STREAMS_HAVE_FRAME_RATE = 0x4,
	GAME_STREAMS_HAVE_FORMAT = 0x8,
	GAME_STREAMS_HAVE_CHANNEL_COUNT = 0x10,
	GAME_STREAMS_ALWAYS_MONO = 0x20
};
struct game_dac_info {

	int16	dac_id;
	int16	linked_adc_id;
	int16	linked_mixer_id;

	int16	max_stream_count;
	int16	cur_stream_count;
	uint16	stream_flags;		/*	STREAMS_HAVE_VOLUME and friends	*/

	size_t	min_chunk_frame_count;		/*	device is not more precise */
	int32	chunk_frame_count_increment;	/*	-1 for power-of-two,
												0 for fixed chunk size */
	size_t	max_chunk_frame_count;
	size_t	cur_chunk_frame_count;

	char	name[32];
	uint32	frame_rates;
	float	cvsr_min;
	float	cvsr_max;
	uint32	formats;
	uint32	designations;
	uint32	channel_counts;		/*	0x1 for mono, 0x2 for stereo,
									0x4 for three channels, ... */

	uint32	cur_frame_rate;
	float	cur_cvsr;
	uint32	cur_format;
	int16	cur_channel_count;
	int16	_reserved_2;

	uint32	_future_[4];
};
struct game_get_dac_infos {

	size_t	info_size;

	game_dac_info *	info;
	int16	in_request_count;
	int16	out_actual_count;
};

struct game_adc_info {

	int16	adc_id;
	int16	linked_dac_id;
	int16	linked_mixer_id;

	int16	max_stream_count;
	int16	cur_stream_count;
	uint16	stream_flags;		/*	STREAMS_HAVE_VOLUME and friends	*/

	size_t	min_chunk_frame_count;		/*	device is not more precise,
												0 for "same as output" */
	int32	chunk_frame_count_increment;	/*	-1 for power-of-two,
												0 for fixed chunk size */
	size_t	max_chunk_frame_count;
	size_t	cur_chunk_frame_count;

	char	name[32];
	uint32	frame_rates;
	float	cvsr_min;
	float	cvsr_max;
	uint32	formats;
	uint32	designations;
	uint32	channel_counts;		/*	0x1 for mono, 0x2 for stereo,
									0x4 for three channels, ... */

	uint32	cur_frame_rate;
	float	cur_cvsr;
	uint32	cur_format;
	int16	cur_channel_count;
	int16	_reserved_2;

	uint32	_future_[4];
};
struct game_get_adc_infos {

	size_t	info_size;

	game_adc_info *	info;
	int16	in_request_count;
	int16	out_actual_count;
};

enum game_codec_format_flags {
	GAME_CODEC_FAIL_IF_DESTRUCTIVE = 0x1,
	GAME_CODEC_CLOSEST_OK = 0x2,
	GAME_CODEC_SET_CHANNELS = 0x4,
	GAME_CODEC_SET_CHUNK_FRAME_COUNT = 0x8,
	GAME_CODEC_SET_FORMAT = 0x10,
	GAME_CODEC_SET_FRAME_RATE = 0x20,
	GAME_CODEC_SET_ALL = 0x3c
};
struct game_codec_format {

	int16	codec;
	uint16	flags;

	uint16	_reserved_1;
	int16	channels;
	size_t	chunk_frame_count;
	uint32	format;
	uint32	frame_rate;
	float	cvsr;

	status_t	out_status;
	uint32	_reserved_2;

	uint32	_future_[4];
};
struct game_set_codec_formats {

	size_t	info_size;

	game_codec_format *	formats;
	int16	in_request_count;
	int16	out_actual_count;
};

struct game_mixer_info {

	int16	mixer_id;
	int16	linked_codec_id;	/*	-1 for none or for multiple (in which
									case you have to check in the codecs) */

	char	name[32];
	int16	control_count;
	int16	_reserved_;

	uint32	_future_[4];
};
struct game_get_mixer_infos {

	size_t	info_size;

	game_mixer_info *	info;
	int16	in_request_count;
	int16	out_actual_count;
};

enum game_mixer_control_type {
	GAME_MIXER_CONTROL_IS_UNKNOWN = 0,
	GAME_MIXER_CONTROL_IS_LEVEL,
	GAME_MIXER_CONTROL_IS_MUX,
	GAME_MIXER_CONTROL_IS_ENABLE
};
enum game_mixer_control_flags {
	GAME_MIXER_CONTROL_ADVANCED = 0x1,	/*	show on "advanced settings" page	*/
	GAME_MIXER_CONTROL_AUXILIARY = 0x2	/*	put above or below channel strips	*/
};
struct game_mixer_control {

	int16	control_id;
	int16	mixer_id;

	int16	type;			/*	unknown, level, mux, enable	*/
	uint16	flags;			/*	advanced, auxiliary, ... (layout control) */
	int16	parent_id;		/*	or NO_ID for no parent	*/
	int16	_reserved_;

	uint32	_future_;
};
struct game_get_mixer_controls {

	size_t	info_size;

	int16	mixer_id;
	int16	from_ordinal;	/*	i e starting with the Nth control for this
								mixer */

	game_mixer_control *	control;
	int16	in_request_count;
	int16	out_actual_count;
};

/*
 *	The value-disp stuff works such that at min_value "physical" value,
 *	the min_value_disp value is fed to the sprintf() string for display.
 *	Intermediate values up to max_value/max_value_disp are interpolated.
 */
enum game_mixer_level_info_flags {
	GAME_LEVEL_HAS_MUTE = 0x1,
	GAME_LEVEL_VALUE_IN_DB = 0x2,
	GAME_LEVEL_HAS_DISP_VALUES = 0x4,
	GAME_LEVEL_ZERO_MEANS_NEGATIVE_INFINITY = 0x8,
	GAME_LEVEL_IS_PAN = 0x10,
	GAME_LEVEL_IS_EQ = 0x20
};
enum game_mixer_level_info_type {
	GAME_LEVEL_AC97_MASTER = 0x2,
	GAME_LEVEL_AC97_ALTERNATE = 0x4,
	GAME_LEVEL_AC97_MONO = 0x6,
	GAME_LEVEL_AC97_PCBEEP = 0xA,
	GAME_LEVEL_AC97_PHONE = 0xC,
	GAME_LEVEL_AC97_MIC = 0xE,
	GAME_LEVEL_AC97_LINE_IN = 0x10,
	GAME_LEVEL_AC97_CD = 0x12,
	GAME_LEVEL_AC97_VIDEO = 0x14,
	GAME_LEVEL_AC97_AUX = 0x16,
	GAME_LEVEL_AC97_PCM = 0x18,
	GAME_LEVEL_AC97_RECORD = 0x1C,
	GAME_LEVEL_FIRST_NONAC97 = 0x200
};
struct game_get_mixer_level_info {

	size_t	info_size;

	int16	control_id;
	int16	mixer_id;

	char	label[32];

	uint16	flags;		/*	has mute, value in dB, etc */
	int16	min_value;
	int16	max_value;
	int16	normal_value;
	float	min_value_disp;
	float	max_value_disp;
	char	disp_format[32];	/*	sprintf() format string taking one float */
	uint16	type;
	int16	value_count;		/* 2 for a stereo level, 4 for a quad, ... */
};

struct game_mux_item {

	uint32	mask;
	int16	control;
	int16	_reserved_;
	char	name[32];
	uint32	_future_[2];
};
enum game_mixer_mux_info_flags {
	GAME_MUX_ALLOWS_MULTIPLE_VALUES = 0x1
};
enum game_mixer_mux_info_type {
	GAME_MUX_AC97_RECORD_SELECT = 0x1A
};
struct game_get_mixer_mux_info {

	size_t	info_size;

	int16	control_id;
	int16	mixer_id;

	char	label[32];

	uint32	flags;		/*	multiple allowed, etc	*/
	uint32	normal_mask;	/*	default value(s)	*/
	int16	in_request_count;
	int16	out_actual_count;
	game_mux_item	*items;
	uint16	type;
};

enum game_mixer_enable_info_flags {
	GAME_ENABLE_OFF, GAME_ENABLE_ON
};
struct game_get_mixer_enable_info {

	size_t	info_size;

	int16	control_id;
	int16	mixer_id;

	char	label[32];

	char	normal;		/*	on or off	*/
	char	_reserved_[3];

	char	enabled_label[24];
	char	disabled_label[24];
};

enum game_mixer_level_value_flags {
	GAME_LEVEL_IS_MUTED = 0x1
};
struct game_mixer_level_value {
	int16	values[6];
	uint16	_reserved_;
	uint16	flags;
};
struct game_mixer_mux_value {
	uint32	mask;
	uint32	_reserved_[3];
};
struct game_mixer_enable_value {
	char	enabled;
	char	_reserved_1[3];
	uint32	_reserved_2[3];
};
struct game_mixer_control_value {

	int16	control_id;
	int16	mixer_id;
	int16	type;
	uint16	_reserved_;
	
	union {
		game_mixer_level_value	level;
		game_mixer_mux_value	mux;
		game_mixer_enable_value	enable;
	}
#if !defined(__cplusplus)
	u
#endif
	;
};
struct game_get_mixer_control_values {

	size_t	info_size;

	int16	in_request_count;
	int16	out_actual_count;
	game_mixer_control_value *	values;
};

struct game_set_mixer_control_values {

	size_t	info_size;

	int16	in_request_count;
	int16	out_actual_count;
	game_mixer_control_value *	values;
};

enum game_open_stream_request {
	GAME_STREAM_VOLUME = 0x1,
	GAME_STREAM_PAN = 0x2,
	GAME_STREAM_FR = 0x4,
	GAME_STREAM_FORMAT = 0x8,
	GAME_STREAM_CHANNEL_COUNT = 0x10
};
struct game_open_stream {

	size_t	info_size;

	int16	adc_dac_id;
	uint16	request;	/*	var-rate, var-volume, var-pan, ...	*/

	sem_id	stream_sem;	/*	< 0 if none	*/

	uint32	frame_rate;
	float	cvsr_rate;
	uint32	format;
	uint32	designations;
	int16	channel_count;

	int16	out_stream_id;
};

struct game_stream_timing {
	float	cur_frame_rate;
	uint32	frames_played;
	bigtime_t	at_time;
};
enum game_stream_state {
	GAME_STREAM_STOPPED,
	GAME_STREAM_PAUSED,
	GAME_STREAM_RUNNING
};
struct game_get_stream_timing {

	size_t	info_size;

	int16	stream;
	int16	state;		/*	stopped, paused, running	*/

	game_stream_timing	timing;
};

enum game_stream_description_mapping {
	GAME_VOLUME_IN_DB = 0x1,
	GAME_VOLUME_MULTIPLIER = 0x2,
	GAME_VOLUME_MIN_VALUE_IS_MUTE = 0x4
};
struct game_get_stream_description {

	size_t	info_size;

	int16	stream;
	int16	codec_id;
	int16	_reserved_;

	uint16	caps;				/*	STREAM_VOLUME, STREAM_INDIVIDUAL_FR, ... */
	uint16	volume_mapping;		/*	linear-in-dB, linear-in-mult, ...	*/
	int16	min_volume;
	int16	max_volume;
	int16	normal_point;

	float	min_volume_db;
	float	max_volume_db;
	float	nominal_frame_rate;
	uint32	format;
	uint32	designations;
};

/*
 *	Stream controls are different from mixer controls in that they are per
 *	individual stream (not per codec). They are also limited to a four-way
 *	pan, volume, and frame rate set.
 */
 struct game_stream_controls {
	int16	stream;
	uint16	caps;		/*	change-sr, change-volume, change-lr-pan, ... */

	int16	volume;
	int16	lr_pan;
	int16	fb_pan;
	int16	_reserved_1;
	float	frame_rate;

	uint32	_future_[2];
};
struct game_get_stream_controls {

	size_t	info_size;

	int16	in_request_count;
	int16	out_actual_count;
	game_stream_controls * controls;
};

struct game_set_stream_controls {

	size_t	info_size;

	int16	in_request_count;
	int16	out_actual_count;
	game_stream_controls * controls;
	bigtime_t	ramp_time;
};

enum game_queue_stream_data_flags {
	GAME_RELEASE_SEM_WHEN_DONE = 0x1,
	GAME_GET_LAST_TIME = 0x2,				/*	get "timing" out	*/
	GAME_BUFFER_LOOPING = 0x4,			/*	play this buffer over and over again	*/
	GAME_BUFFER_PING_PONG = 0x8			/*	This flag means that the stream sem will	*/
										/*	be released after every page_frame_count frames played	*/
};
#if 0
struct game_queue_stream_data {

	size_t	info_size;

	int16	stream;
	uint16	flags;

	void *	data;
	size_t	frame_count;

	game_stream_timing	out_timing;
};
#endif
struct game_queue_stream_buffer {

	size_t	info_size;

	int16	stream;
	uint16	flags;
	int16	buffer;
	uint16	page_frame_count;	/* only used for PING_PONG */
	size_t	frame_count;

	game_stream_timing	out_timing;
};

/*
 *	When pausing and un-pausing, the stream state is kept (frames played etc).
 *	When stopping and re-starting, the stream is re-set (frames played starts
 *	at 0).
 */
struct game_run_stream {

	int16	stream;
	int16	state;		/*	stopped, paused, running	*/

	game_stream_timing	out_timing;		/*	timing info at which it took
										effect/will take effect (estimate) */
	uint32	_reserved_;
	uint32	_future_[2];
};
struct game_run_streams {

	size_t	info_size;

	int16	in_stream_count;
	int16	out_actual_count;
	game_run_stream *	streams;
};

enum game_close_stream_flags {
	GAME_CLOSE_BLOCK_UNTIL_COMPLETE = 0x1,
	GAME_CLOSE_DELETE_SEM_WHEN_DONE = 0x2,
	GAME_CLOSE_FLUSH_DATA = 0x4
};
struct game_close_stream {

	size_t	info_size;

	int16	stream;
	uint16	flags;		/*	block-until-complete, delete-stream-sem	*/
};

struct game_open_stream_buffer {

	size_t	info_size;

	int16	stream;
	uint16	_reserved_1;
	size_t	byte_size;

	area_id	area;			/*	user has to clone this area	*/
	size_t	offset;			/*	and find the buffer this far into it	*/
	int16	buffer;
	int16	_reserved_2;
};

struct game_close_stream_buffer {

	size_t	info_size;

	int16	buffer;
	int16	_reserved_;
};


/*
 *	GET_DEVICE_STATE and SET_DEVICE_STATE are for easy saving and restoring
 *	of the device state (mostly mixer levels and stuff) when starting/stopping
 *	the media server (or snd_server, as the case may be).
 *	The device should not change any mixer items when closed/unloaded; this is
 *	so that CD-through, telephone-through etc will still work if set.
 */
struct game_get_device_state {

	size_t	info_size;

	size_t	in_avail_size;
	int32	out_version;
	void *	data;
	size_t	out_actual_size;
};

struct game_set_device_state {

	size_t	info_size;

	size_t	in_avail_size;
	int32	in_version;
	void *	data;
};

/*
 *	For ioctl() interfaces not defined by Be (or even some extra ones defined
 *	later) you can ask whether a certain card supports it (and get information
 *	specific to that interface) by fourcc. You also get a name back, useful
 *	for informational purposes only (as the name may vary by implementation,
 *	but the fourcc is the same for one interface).
 *	"interface" in this context is just a defined set of ioctl() codes and
 *	the data/actions that go with those codes. User-defined codes start at
 *	100000 (or higher) and go up to 0x7fffffff.
 */
struct game_get_interface_info {

	size_t	info_size;

	uint32	fourcc;
	uchar	_reserved_[16];
	size_t	request_size;
	size_t	actual_size;
	void *	data;
	char	out_name[32];
	int		first_opcode;
	int		last_opcode;
};

/*
 *	List all interfaces this device implements (up to request_count items).
 */
struct game_get_interfaces {

	size_t	info_size;

	int16	request_count;
	int16	actual_count;
	uint32 *	interfaces;	/* fourcc codes */
};

#endif	/*	game_audio_h	*/

