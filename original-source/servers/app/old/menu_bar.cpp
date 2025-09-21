#if 0
//******************************************************************************
//
//	File:		menubar.cpp
//
//	Description:	menubar class.
//			Implements menu bar UI elements.
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
#include <OS.h>
#include "gr_types.h"
#include "proto.h"
#include "macro.h"

#include "menu_bar.h"
#include "server.h"
#include "Container.h"
#include "text.h"

#define INHERITED TView

//------------------------------------------------------------

	TMenuBar::TMenuBar(rect *bound) :
        MView(bound, 1, rule(POS_TOP, POS_LEFT, POS_TOP, POS_RIGHT))
{
	first_menu = 0;
	right_most = 0;
	is_client_view = 0;
}

//------------------------------------------------------------

void	TMenuBar::hilite_range(TMenu *a_menu)
{
/*
	frect	a_rect;
	

	get_bound(&a_rect);
	a_rect.bottom--;
	a_rect.left = a_menu->hit_range_left;
	a_rect.right = a_menu->hit_range_right;
	rect_invert(&a_rect, 1, 1.0);
*/
}

//------------------------------------------------------------

void	TMenuBar::send_result(long result, char* item_name)
{
	MWindow	*mb_owner;
	BParcel	a_parcel(99);

	mb_owner = (MWindow *)get_owner();
	
/*
	message	a_message;

	a_message.parm1 = system_time();
	a_message.parm2 = client_token;
	a_message.parm3 = result;
	result = mb_owner->send_event(MENU_COMMAND, &a_message);
*/
	
	a_parcel.AddInt64(system_time(), "when");
	a_parcel.SetTarget(client_token);
	a_parcel.AddInt32(result, "command");
	a_parcel.AddString(item_name, "menuItem");
	mb_owner->send_event(&a_parcel);
}

//------------------------------------------------------------

char	TMenuBar::click(long h, long v)
{
	char	item_name[32];
	TMenu	*a_menu;
	TMenu	*current;
	long	i;
	rect	a_rect;
	long	offset_h;
	long	offset_v;
	long	total;
	long	message;
	TWindow	*an_owner;

	an_owner = get_owner();
	an_owner->make_active(true);
/*	if ((!an_owner->is_active()) && (an_owner->proc_id != WDEF_BACK)) {
		an_owner->bring_to_front();
		return(1);
	}*/
	
	current = 0;
	message = -1;

	do {
		i = 0;
		while(a_menu = get_menu(i)) {
			i++;
			if ((h >= a_menu->hit_range_left) &&
		    	(h <= a_menu->hit_range_right)) {
			
				if (a_menu != current) {
					if (current)
						hilite_range(current);
					
					hilite_range(a_menu);
					current = a_menu;

					get_bound(&a_rect);
					a_rect.bottom--;
					a_rect.left = a_menu->hit_range_left;
					a_rect.right = a_menu->hit_range_right;
					get_owner()->get_screen_offset(&offset_h, &offset_v);
					offset_rect(&a_rect, offset_h, offset_v);
					message = current->menu_track(&a_rect,
							 right_most + offset_h,
							 item_name);

					total = 0;
					do {
						snooze(4000);
						total += 4;
					} while (total < 30);
					set();
				}
				goto done;
			}
		}

	done:
		snooze(5000);
		the_server->get_mouse(&h, &v);
		global_to_local(&h, &v);
		get_bound(&a_rect);
		if (v > a_rect.bottom)
			goto exit;
		if (v < a_rect.top)
			goto exit;
		if (h < a_rect.left)
			goto exit;
		if (h > a_rect.right)
			goto exit;
	} while (the_server->test_button());
exit:
	if (current) {
		hilite_range(current);
	}

	if (message != -1) {
		send_result(message, item_name);
	}


	return(1);
}

//------------------------------------------------------------

TMenu	*TMenuBar::get_menu(long i)
{
	TMenu	*a_menu;

	a_menu = first_menu;

	while(i > 0) {
		if (a_menu == 0)
			return(a_menu);
		a_menu = a_menu->next_menu;
		i--;
	}
	return(a_menu);
}

//------------------------------------------------------------

void	TMenuBar::add_menu(TMenu *a_menu)
{
	TMenu	*tmp_menu;

	tmp_menu = first_menu;

	if (tmp_menu == 0) {
		first_menu = a_menu;
	}
	else {
		while(tmp_menu->next_menu)
			tmp_menu = tmp_menu->next_menu;

		tmp_menu->next_menu = a_menu;
	}

	recalc_ranges();

	inval();
}

//------------------------------------------------------------

void	TMenuBar::draw()
{
	long	hpos;
	long	i;
	rect	the_bound;
	TMenu	*a_menu;
	long	h;
	long	v;

	get_bound(&the_bound);

	the_bound.bottom--;

	rect_fill(&the_bound, get_pat(BLACK), op_copy, 1, desk_rgb_gray, desk_rgb_gray);


	get_bound(&the_bound);

	lineto(the_bound.left,
	       the_bound.bottom,
	       the_bound.right,
	       the_bound.bottom,
	       op_copy,
	       1,
	       rgb_black,
	       rgb_white);

	i = 0;
	hpos = offset_left;

	while(a_menu = get_menu(i)) {
		i++;
		h = a_menu->hit_range_left + (item_spacing / 2);
		v = the_bound.bottom - offset_bottom + b_font_bold->ascent() - 1;

		draw_string_screen_only(a_menu->get_name(),
								b_font_bold,
								op_copy,
								h,
								v,
								rgb_black,
								rgb_white);
	}
}

//------------------------------------------------------------

void	TMenuBar::recalc_ranges()
{
	long	hpos;
	long	width;
	long	i;
	TMenu	*a_menu;

	i = 0;
	hpos = offset_left;

	while(a_menu = get_menu(i)) {
		i++;
		width = (int)(fc_get_string_width(b_font_bold,
										  (uchar*)a_menu->get_name())+0.5);
		a_menu->hit_range_left = hpos - (item_spacing / 2);
		hpos = hpos + width + item_spacing;
		a_menu->hit_range_right = hpos - (item_spacing / 2); 
	}
	right_most = hpos - (item_spacing / 2);
}

//------------------------------------------------------------

void	TMenuBar::set_enable(long item_code, char enable)
{
	menu_item	*the_item;

	the_item = find_item(item_code);
	if (the_item) {
		the_item->f_enable = enable;
	}
}

//------------------------------------------------------------

void	TMenuBar::set_mark(long item_code, char mark)
{
	menu_item	*the_item;

	the_item = find_item(item_code);
	if (the_item) {
		the_item->f_mark = mark;
	}
}

//------------------------------------------------------------

void	TMenuBar::set_key(long item_code, char key)
{
	menu_item	*the_item;

	the_item = find_item(item_code);
	if (the_item) {
		the_item->f_key = key;
	}
}

//------------------------------------------------------------

void	TMenuBar::set_text(long item_code, char *text)
{
	menu_item	*the_item;

	the_item = find_item(item_code);
	if (the_item) {
		strcpy(the_item->name, text);
	}
}

//------------------------------------------------------------

menu_item *TMenuBar::find_item(long item_code)
{
	TMenu		*a_menu;
	menu_item	*the_item;

	a_menu = first_menu;
	while(a_menu) {
		the_item = a_menu->find_item(item_code);
		if (the_item)
			return(the_item);
		
		a_menu = a_menu->next_menu;
	}
	return(0);
}
#endif