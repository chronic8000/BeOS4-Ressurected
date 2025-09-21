
#if !defined(gamedriver_h)
#define gamedriver_h

#include <PCI.h>
#include <ISA.h>
#include <USB.h>
#include <game_audio.h>
#include <gaplug.h>
#include <midi_driver.h>
#include <joystick_driver.h>

extern pci_module_info * g_pci;
extern isa_module_info * g_isa;
extern usb_module_info * g_usb;

typedef struct ga_loaded_module ga_loaded_module;
typedef struct ga_cookie ga_cookie;
typedef struct ga_midi_cookie ga_midi_cookie;
typedef struct ga_joy_cookie ga_joy_cookie;
typedef struct ga_active_stream_info ga_active_stream_info;
typedef struct ga_active_buffer_info ga_active_buffer_info;
typedef struct ga_mixer_info ga_mixer_info;

#define BLOCK_SHIFT			5
#define BUFFERS_IN_A_BLOCK	(1<<BLOCK_SHIFT)
#define BLOCK_MASK			((BUFFERS_IN_A_BLOCK)-1)

struct ga_loaded_module {
	struct ga_loaded_module *
							next;
	char					module[44];
	char					devpath[36];
	char					midipath[32];
	char					joypath[36];
	plug_api *				dev;
	bool					inited;
	int						adc_count;
	game_codec_info *		adc_infos;
	ga_active_stream_info **
							adc_streams;
	int						dac_count;
	game_codec_info *		dac_infos;
	ga_active_stream_info **
							dac_streams;
	ga_active_stream_info *	close_list;
	ga_cookie *				cookies;
	int						buffer_count;
	ga_active_buffer_info **
							buffers;
	int						mixer_count;
	ga_mixer_info *			mixer_infos;
	
	sem_id					sem;
	int32					ben;
	thread_id				locked;

	spinlock				spinlock;
	thread_id				spinlocked;
	cpu_status				cpu;

	int32					mpu401_open_count;
	void *					mpu401_device_id;
	plug_mpu401_info		mpu401_info;

	int32					gameport_open_count;
	void *					gameport_device_id;
	plug_gameport_info		gameport_info;
};

struct ga_cookie {
	ga_cookie *				next;
	ga_loaded_module *		module;
	plug_api *				dev;
	bool					write_ok;
};

struct ga_midi_cookie {
	void *					inner_cookie;
	ga_loaded_module *		module;
};

struct ga_joy_cookie {
	void *					inner_cookie;
	ga_loaded_module *		module;
};

struct ga_active_stream_info {
	game_open_stream		info;
	/*	Extra book-keeping comes here	*/
	ga_cookie *				owner;	/*	device cookie that owns this stream, or 0 for unallocated */
	void *					stream_cookie;	/*	allocated by plug-in	*/
	game_stream_state		state;
	ga_stream_data			feed;
	ga_active_buffer_info * buffer;
	uint32					page_frame_count;
	int32					frame_size;
	int32					release_count;
	sem_id					done_sem;
	ga_active_stream_info *	next_closing;
	game_stream_timing		timing;
	game_stream_controls	controls;
	game_stream_capabilities *
							capabilities;
};

struct ga_active_buffer_info {
	int32					id;
	int32					stream;
	area_id					area;
	void *					base;
	size_t					offset;
	size_t					size;
	ga_cookie *				owner;
	int						count;
};

struct ga_mixer_info {
	game_mixer_info			info;
	gaplug_get_mixer_controls
							controls;
	gaplug_mixer_control_values
							values;
	int32					hidden_ctl_count;						
};

#endif	//	gamedriver_h

