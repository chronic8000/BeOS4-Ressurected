/*----------------------------------------------------------

	webpad_private.h
	
	by Pierre Raynaud-Richard and Trey Boudreau

	Copyright (c) 1998 by Be Incorporated.
	All Rights Reserved.
	
----------------------------------------------------------*/

#if !defined(_DURANGO_PRIVATE_H_)
#define _DURANGO_PRIVATE_H_ 1

#include <Drivers.h>
#include <OS.h>
#include <Accelerant.h>
#include <video_overlay.h>
#include <surface/genpool.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the Media-GX video in the dt300 webpad.
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "a durango driver benaphore");  x.ben = 0;
#define ACQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define ACQUIRE_BEN_ON_ERROR(x, y) if((atomic_add(&(x.ben), 1)) >= 1) if (acquire_sem(x.sem) != B_OK) y;
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem); x.sem = -1; x.ben = 1;

#define DURANGO_PRIVATE_DATA_MAGIC	0x0203 /* a private driver rev, of sorts */
#define MAX_DURANGO_DEVICE_NAME_LENGTH 32

#define GKD_MOVE_CURSOR    0x00000001
#define GKD_PROGRAM_CLUT   0x00000002
#define GKD_SET_START_ADDR 0x00000004
#define GKD_SET_CURSOR     0x00000008
#define GKD_HANDLER_INSTALLED 0x80000000

enum {
	DURANGO_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	DURANGO_DEVICE_NAME,
	DURANGO_WRITE_CH7005,
	DURANGO_READ_CH7005,
	DURANGO_LOCK,
	DURANGO_ALLOC,
	DURANGO_FREE
};

typedef struct {
	vuchar	*regs;
	area_id	regs_area;
	void	*framebuffer;
	void	*framebuffer_dma;
	area_id	fb_area;
	vuchar	*dac;
	area_id	dac_area;
	uint32	mem_size;
	uint8	GCR;
#if GXLV_DO_VBLANK
	sem_id	vblank;
	int32
		flags,
		start_addr;
	uint16
		cursor_x,
		cursor_y,
		first_color,
		color_count;
	uint8
		color_data[3 * 256],
		cursor0[512],
		cursor1[512];
#endif
} shared_info;

typedef struct {
	int	used;	// non-zero if in use
	// these are used to reposition the window during virtual desktop moves
	const overlay_buffer *ob;	// current overlay_buffer for this overlay
	overlay_window ow;	// current overlay_window for this overlay
	overlay_view ov;	// current overlay_view for this overlay
} durango_overlay_token;

typedef struct {
	uint64		fifo_count;
	display_mode
				dm;
	frame_buffer_config
				fbc;
	benaphore	engine;
	struct
	{
		uint32	*data;
		uint32	black;
		uint32	white;
		int16	x, y;
		int16	hot_x, hot_y;
		bool	is_visible;
		BMemSpec cursor_spec;	// info about cursor memory allocation when using memtypes
	} cursor;
	BMemSpec fb_spec;		// frame buffer allocation
	BMemSpec compression_spec;	// frame buffer compression area spec, if req'd
	BMemSpec ovl_buffer_specs[4];	// overlay buffer allocations
	overlay_buffer ovl_buffers[4];	// overlay buffers
	durango_overlay_token ovl_tokens[1];// storage for each overlay
} accelerant_info;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;
} durango_set_bool_state;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	shared_info	*si;
} durango_get_private_data;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;
} durango_device_name;

typedef struct {
	uint32		magic;		/* magic number to make sure the caller groks us */
	BMemSpec	*ms;
} durango_alloc_mem;

#ifdef CH7005_SUPPORT
typedef struct {
	uint32	magic;
	uint8	count;
	uint8	index;
	uint8	*data;
} durango_ch7005_io;
#endif

enum {
	DURANGO_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif

#endif /* _DURANGO_PRIVATE_H_ */
