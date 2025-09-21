/******************************************************************************
//
//	File:		MWindow.h
//
//	Description:	MWindow testing area.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	5/19/92		bgs	new today
//	5/31/94		bgs	update to new event model
//
//*****************************************************************************/

#ifndef	MWINDOW_H
#define	MWINDOW_H

#include "window.h" 
#include "view.h"
#include "heap.h"
#include "as_debug.h"
#include "parcel.h"

#define	CLIENT_DRAW	0x20000000

/*-------------------------------------------------------------*/	

struct RenderContext;
struct ViewUpdateInfo;

class MWindow : public TWindow
{
	private:
				BArray<ViewUpdateInfo>	updateList;
				int32					updateListPtr;
				uint32					sharedUpdateList;
				region *				m_updateRegion;
				int32					m_updatePixelFormat;
				uint8					m_updatePixelEndianess;

				void					gr_begin_client_update();
				void					gr_end_client_update();
				void					gr_next_update_view();
				void					AddUpdateBuffers(TView *buffers);
				void					RemoveUpdateBuffers(TView *buffers);
				void					EndViewUpdate();

	public:
				long			caller;
				long			send_port;

								MWindow(SApp *application,
									char *name, rect *bound,
									uint32 design, uint32 behavior,
									uint32 flags, uint32 ws, int32 teamID,
									port_id eventPort, int32 clientToken,
									TSession *session=NULL);
								MWindow(SApp *application);

				void			new_view();
		virtual	void			handle_client_message(long message);
		virtual	void			update();
		virtual	void			handle_message(message *a_message);

		virtual	void			draw();
				void			do_drop(message *a_message);
				char			check_inter_port();
				bool			check_view(long token);
				void			get_update_state();
		virtual	void			get_pointer();
				void			update_off();
				void			update_on();
				void			screen_changed(long h_size, long v_size, long color_space);

				void			gr_pick_view();
				//void			gr_get_modifiers();
				void			gr_no_clip();
				void			gr_userclip();
				void			gr_view_flags();
				void			gr_get_pen_loc();
				void			gr_moveto();
				void			gr_line();
				void			gr_lineto();
				void			gr_moveby();
				void			gr_rectframe();
				void			gr_rectfill();
				void			gr_rect_invert();
				void			gr_polyframe();
				void			gr_polyfill();
				void			gr_fore_color();
				void			gr_set_view_color();
				void			gr_set_view_bitmap();
				void			gr_get_fore_color();
				void			gr_back_color();
				void			gr_get_back_color();
				void			gr_set_pattern();
				void			gr_set_draw_mode();
				void			gr_get_draw_mode();
				void			gr_get_view_mouse();
				void			gr_get_key_states();
				void			gr_drawstring();
				void			set_font_context();
				void			set_font_name();
				void			set_font_set();
				void			set_font_size();
				void			set_font_shear();
				void			set_font_rotate();
				void			gr_inval();
				void			gr_inval_region();
				void			gr_need_update();
				void			gr_sync();
				void			gr_movesize_view(long message);
				void			gr_remove_view();
				void			gr_movesize_window(long message);
				void			gr_add_menubar();
				void			gr_add_menu();
				void			gr_add_menu_item();
				void			gr_add_scroll_bar();
				void			gr_close_window();
				void			private_unlink();
				void			gr_select_window();
				void			gr_send_to_back_window();
				void			gr_varray();
				void			gr_scroll_bar_set_steps();
				void			gr_scroll_bar_set_range();
				void			gr_scroll_bar_set_value();
				void			gr_scroll_bar_set_proportion();
				void			gr_set_pen_size();
				void			gr_get_pen_size();
				void			gr_set_scale();
				void			gr_set_view_origin();
				void			gr_ellipse(long message);
				void			gr_arc(long message);
				void			gr_round_rect_frame();
				void			gr_round_rect_fill();
				void			gr_get_font_info();
				void			gr_measure_string();
				void			gr_get_char_escapements();
				void			gr_get_char_edges();
				void			gr_get_window_title();
				void			gr_set_window_title();
				void			gr_get_decor_state();
				void			gr_set_decor_state();
				void			gr_add_window_child();
				void			gr_remove_window_child();
				void			gr_get_window_box();
				void			gr_get_window_bound();
				void			gr_window_is_front();
				void			gr_window_is_active();
				void			gr_get_view_bound();
				void			gr_get_location(bool screwedUp=true);
				void			gr_set_menu_text();
				void			gr_set_menu_things(long message);
				void			gr_data();
				void			gr_draw_bitmap(char async);
				void			gr_scale_bitmap(char async);
				void			gr_scale_bitmap1(char async);
				void			gr_move_bits();
				void			gr_find_view();
				void			gr_get_clip();
				void			gr_pt_to_screen();
				void			gr_pt_from_screen();
				void			gr_rect_to_screen();
				void			gr_rect_from_screen();
				void			gr_show_window();
				void			gr_hide_window();
				void			gr_start_picture();
				void			gr_restart_picture();
				void			gr_end_picture();
				void			gr_play_picture();
				void			gr_clip_to_picture();
				void			do_window_limits();
				void			gr_maximize();
				void			gr_minimize();
				void			gr_get_ws();
				void			gr_set_ws();
				void			gr_remove_subwindow();
				void			gr_add_subwindow();
				void			gr_fill_region();
				void			gr_bezier(long msg);
				void			gr_set_line_mode();
				void			gr_get_line_mode(long msg);
				void			gr_show_window_sync();
				void			gr_set_window_flags();
				void			gr_send_behind();
				void			gr_set_origin();
				void			gr_get_origin();
				void			gr_push_state();
				void			gr_pop_state();
				void			gr_init_direct_window();
				void			gr_set_fullscreen();
				void			gr_set_window_alignment();
				void			gr_get_window_alignment();
				void			gr_disk_picture();
				void			gr_play_disk_picture();
				void			gr_set_font_flags();
				void			gr_shape(int32 msg);
				void			gr_set_view_event_mask();
				void			gr_augment_view_event_mask();
				void			gr_open_view_transaction();
				void			gr_close_view_transaction();
				void			gr_set_blending_mode();
				void			gr_get_blending_mode();
				void			get_font_context();
				void			gr_set_window_picture();
				status_t		gr_set_view_cursor();
				status_t		gr_set_view_double_buffer();
#if AS_DEBUG
		virtual	void			DumpDebugInfo(DebugInfo *);
#endif
};

/*-------------------------------------------------------------*/

void	new_window(TSession *a_session);
void	new_off_wind(TSession *a_session);
void	new_bitmap(TSession *a_session, Heap *serverHeap);
void	set_bits(TSession *a_session);
void	set_bits_24(TSession *a_session);
void	set_bits_24_tst(TSession *a_session);
void	remove_bitmap(TSession *a_session);
void	new_tt_font(TSession *a_session);

#endif
