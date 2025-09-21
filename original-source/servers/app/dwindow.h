#ifndef _DWINDOW_H
#define	_DWINDOW_H

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef	SESSION_H
#include "session.h"
#endif

#ifndef	VIEW_H
#include "view.h"
#endif

#ifndef WORKSPACE_H
#include <workspace.h>
#endif

#ifndef _OS_H
#include <OS.h>
#endif

#include <direct_window_priv.h>

#include "parcel.h"

extern void		get_unblocked_screen(team_id bad_team, int32 *screen_index, int32 *ws_index);
extern void		send_into_debugger(TWindow *window, sem_id failed_acquisition);
extern void		acquire_direct_screen_and_region_sem(bool force_kill = false);
extern void		release_direct_screen_and_region_sem();
extern void		reset_direct_screen_sem();
extern TWindow	*current_window_screen();
extern TWindow	*current_direct_window();
extern int32	lock_direct_screen(int32 index, int32 state);

// direct window class.
class DWindow {
public:
		DWindow(dw_init_info *info, TWindow *window);
		~DWindow();
		
		void		Disconnect();
		void		Before();
		void		After();
		void		Acknowledge();
		void		NewMode();
		void		NewDriver();
		void		GetContentBound(rect *user_bound, rect *old_bound);
		void		SetRegion(region *clip, rect *bound);		
private:
		/* private enums */
		enum buf_state {
			B_DIRECT_MODE_MASK	= 15,
		
			B_DIRECT_START		= 0,
			B_DIRECT_STOP		= 1,
			B_DIRECT_MODIFY		= 2,
			
			B_CLIPPING_MODIFIED = 16,
			B_BUFFER_RESIZED 	= 32,
			B_BUFFER_MOVED 		= 64,
			B_BUFFER_RESETED 	= 128,
			B_BUFFER_FULLSCREEN	= 256
		};
		enum driv_state {
			B_DRIVER_CHANGED	= 0x0001,
			B_MODE_CHANGED		= 0x0002
		};

		/* Frame buffer access descriptor */
		typedef struct {
			buf_state			buffer_state;
			driv_state			driver_state;
			void				*bits;
			void				*pci_bits;
			int32				bytes_per_row;
			uint32				bits_per_pixel;
			color_space			pixel_format;
			buffer_layout		layout;
			buffer_orientation	orientation;
			uint32				_reserved[9];
			uint32				_dd_type_;
			uint32				_dd_token_;
			uint32				clip_list_count;
			rect				window_bounds;
			rect				clip_bounds;
			rect				clip_list[1];
		} direct_buffer_info;

		char				*pci0;
		char				*bits0;
		bool				need_acknowledge;
		bool				need_after;
		bool				multiple_access;
		bool				software_cursor;
		bool				was_screen;
		sem_id				disable_sem;
		sem_id				disable_sem_ack;
		TWindow				*owner;
		area_id				clipping_area;
		region				*bounds;
		region				*visible;
		region				*old_visible;
		region				*tempregion;
		direct_buffer_info	*buf_info;
		int32				change_state;
		int32				buffer_state;

		inline int32		fullscreen_mask() { return (owner->is_screen()?B_BUFFER_FULLSCREEN:0); }
};

#endif

