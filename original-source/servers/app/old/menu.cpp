#if 0
//******************************************************************************
//
//	File:		menu.cpp
//
//	Description:	menubar class.
//			Implements menu data structure.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	4/27/92		bgs	new today
//
//******************************************************************************

#include <string.h>
#include "gr_types.h"

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef	MENU_H
#include "menu.h"
#endif

#ifndef	TEXT_H
#include "text.h"
#endif

#ifndef	SERVER_H
#include "server.h"
#endif

#define INHERITED TView

//------------------------------------------------------------

	TMenu::TMenu(char *name)
{
	strcpy(title, name);
	
	item_count = 0;
	items = 0;
	next_menu = 0;
	hit_range_left = 0;
	hit_range_right = 0;
	has_mark = 0;
}

//------------------------------------------------------------

menu_item *TMenu::find_item(long message)
{
	menu_item	*tmp_item;
	
	tmp_item = items;	

	while (tmp_item) {
		if (tmp_item->message == message)
			return(tmp_item);

		tmp_item = (menu_item *)tmp_item->next;
	}
	return(0);
}	

//------------------------------------------------------------

void	TMenu::add_item(char *name, long message, char fkey, char enable)
{
	menu_item	*the_item;
	menu_item	*tmp_item;

	the_item = (menu_item *)gr_malloc(sizeof(menu_item));
	strcpy(the_item->name, name);
	the_item->message = message;	
	
	the_item->f_enable = enable;

	if (strcmp(name, "-") == 0)
		the_item->f_enable = 0;
	else
		the_item->f_enable = enable;

	the_item->f_key = fkey;
	the_item->f_mark = 0;

	the_item->next = 0;
	
	tmp_item = items;	
	item_count++;

	if (tmp_item == 0) {
		items = the_item;
		return;
	}

	while (tmp_item->next) {
		tmp_item = (menu_item *)tmp_item->next;
	}
	tmp_item->next = the_item;
}

//------------------------------------------------------------

menu_item *TMenu::get_item(long i)
{
	menu_item	*tmp_item;

	if (i >= item_count)
		return(0);

	tmp_item = items;

	while(i > 0) {
		i--;
		tmp_item = (menu_item *)tmp_item->next;
	}
	return(tmp_item);
}

//------------------------------------------------------------

char	TMenu::enable(long i)
{
	return(get_item(i)->f_enable);
}

//------------------------------------------------------------

void	TMenu::menu_draw()
{
	port		*old_port;
	menu_item	*the_item;
	long		i;
	long		vp;
	long		hs;
	rect		tmp_rect;
	long		h;
	long		v;
	char        table_char[2];

	tmp_rect.top = 0;
	tmp_rect.left = 0;
	tmp_rect.bottom = 1000;
	tmp_rect.right = 1000;
	rect_fill(&tmp_rect, get_pat(BLACK), op_copy, 1, rgb(220, 220, 220), rgb_white);

	hs = current_bound.right - current_bound.left;

	for (i = 0; i < item_count; i++) {
		vp = V_TOP + (i * V_SPACE);
		the_item = get_item(i);
		if (strcmp(the_item->name, "-") == 0) {
			set_rect(tmp_rect, vp+4, 0, vp+4, hs);
			rect_fill(&tmp_rect, get_pat(GRAY), op_copy, 1, rgb_black, rgb_white);
		}
		else {
			if (has_mark)
				h = V_LEFT + MARK_SPACE;
			else
				h = V_LEFT;
		
			if (!(the_item->f_enable))
				draw_string_screen_only(the_item->name,
										b_font_bold,
										op_copy,
										h,
										vp + b_font_bold->ascent(),
										rgb_gray,
										rgb_white);	
			else
				draw_string_screen_only(the_item->name,
										b_font_bold,
										op_copy,
										h,
										vp + b_font_bold->ascent(),
										rgb_black,
										rgb_white);	

			if (the_item->f_mark) {
				table_char[0] = the_item->f_mark;
				table_char[1] = 0;
				draw_string_screen_only(table_char,
										b_font_bold,
										op_copy,
										V_LEFT,
										vp + b_font_bold->ascent(),
										rgb_black,
										rgb_white);
			}
			if (the_item->f_key) {
				table_char[0] = '#';
				table_char[1] = 0;
				draw_string_screen_only(table_char,
										b_font_bold,
										op_copy,
										width - V_RIGHT,
										vp + b_font_bold->ascent(),
										rgb_black,
										rgb_white);
				table_char[0] = the_item->f_key;
				table_char[1] = 0;
				draw_string_screen_only(table_char,
										b_font_bold,
										op_copy,
										width - V_RIGHT + 7,
										vp + b_font_bold->ascent(),
										rgb_black,
										rgb_white);
			}
			/*
			if (!(the_item->f_enable)) {
				tmp_rect.top = vp - 2;
				tmp_rect.bottom = tmp_rect.top + V_SPACE;
				tmp_rect.left = 0;
				tmp_rect.right = current_bound.right - current_bound.left;
				rect_fill(&tmp_rect, get_pat(LT_GRAY), op_and, 1, rgb_black, rgb_white);
			}
			*/
		}
	}

}

//------------------------------------------------------------

long	TMenu::calc_width()
{
	long		max;
	long		c_width;
	long		i;
	menu_item	*the_item;

	max = 0;
	has_mark = 0;

	for (i = 0; i < item_count; i++) {
		the_item = get_item(i);
		c_width = (int)(fc_get_string_width(b_font_bold,
											(uchar*)the_item->name)+0.5);
		if (the_item->f_key)
			c_width += 25;
		if (the_item->f_mark) {
			c_width += 20;
			has_mark = 1;
		}
		if (c_width > max)
			max = c_width;
	}
	max += (V_LEFT * 2) + 15;
	return(max);
}

//------------------------------------------------------------

void	TMenu::hilite_n(long n)
{
	frect	tmp_rect;

	if (enable(n)) {
		tmp_rect.top = n * V_SPACE;
		tmp_rect.bottom = tmp_rect.top + V_SPACE;
		tmp_rect.left = 0;
		tmp_rect.right = current_bound.right - current_bound.left;
		rect_invert(&tmp_rect, 1, 1.0);
	}
}

//------------------------------------------------------------

char	TMenu::other_menu(rect *start_rect, long right_most)
{
	long	h,v;
	long	dv;
	rect	tmp_rect;

	snooze(20000);
	the_server->get_mouse(&h, &v);

	if (point_in_rect(start_rect, h, v))
		return 2;
	
	tmp_rect = *start_rect;
	tmp_rect.left = tmp_rect.right + 1;
	tmp_rect.right = right_most;
	
	if (point_in_rect(&tmp_rect, h, v))
		return 1;

	tmp_rect = *start_rect;
	tmp_rect.right = tmp_rect.left - 1;
	tmp_rect.left = 0;

	if (point_in_rect(&tmp_rect, h, v))
		return 1;
	return(0);
}

//------------------------------------------------------------

char	TMenu::track_inside(rect *start_rect, long right_most)
{
	long	h,v;
	long	dv;
	rect	tmp_rect;

	snooze(20000);
	the_server->get_mouse(&h, &v);
	
	if (point_in_rect(&current_bound, h, v)) {
		dv = v - current_bound.top - 3;
		dv /= V_SPACE;
		if (dv > (item_count -1))
			dv = item_count - 1;

		if (dv != cur_item) {
			if (cur_item >= 0) {
				hilite_n(cur_item);
			}
			cur_item = dv;
			hilite_n(cur_item);
		}
	}
	else {
		if (cur_item >= 0) {
			hilite_n(cur_item);
			cur_item = -1;
		}
	}

	return 1;
}

//------------------------------------------------------------

long	TMenu::menu_track(rect *start_rect, long right_most, char* name)
{
	port		*old_port;
	long		i;
	char		last=0;
	char		done;
	char		button;
	char		part;
	long		tmp_port_id;

	old_port = get_port();
	width = calc_width();
	if (width < (start_rect->right - start_rect->left)) 
		width = (start_rect->right - start_rect->left);

	width += 2;
	
	cur_item = -1;

	current_bound.top = start_rect->bottom + 1;
	current_bound.bottom = current_bound.top + (V_SPACE * item_count) + 2;
	current_bound.left = start_rect->left;
	current_bound.right = current_bound.left + width;
	
	wait_regions();
	menu_window = new TWindow("menu", &current_bound, WDEF_MENU, 0);
	menu_window->no_update = 1;
	signal_regions();
	menu_window->set();
	menu_draw();

	done = 0;

	while(1) {
		button = the_server->test_button();
		track_inside(start_rect, right_most);
		if (button) {
			if (other_menu(start_rect, right_most) == 1) {
				cur_item = -1;
				goto exit;
			}
		}
		else {
			if (cur_item > 0)
				goto exit;
			else
				goto had_mouse_up;
		}
	}
had_mouse_up:
	part = other_menu(start_rect, right_most);
	if (part == 2)
	while(1) {
		button = the_server->test_button();
		track_inside(start_rect, right_most);
		/*
		if (other_menu(start_rect, right_most) == 1) {
			cur_item = -1;
			goto exit;
		}
		*/
		if (button) {
			goto exit;
		}
	}
	

exit:	

	if ((last != 0) && (cur_item >= 0)) {
		for (i = 0; i < 3; i++) {
			snooze(60000);
			hilite_n(cur_item);
		}
	}

	tmp_port_id = menu_window->fport_id;
	menu_window->close();
	snooze(30000);

	set_port(old_port);
	set_rect(current_bound, 0, 0, -1, -1);

	if ((last != 0) && (cur_item >= 0)) {
		strcpy(name, get_item(cur_item)->name);
		return(get_item(cur_item)->message);
	}
	else {
		return(-1);
	}
}

//------------------------------------------------------------
#endif