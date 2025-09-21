#if 0
//******************************************************************************
//
//	File:		menu.h
//
//	Description:	TMenu header.
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

#ifndef	MENU_H
#define	MENU_H

#ifndef VIEW_H
#include "view.h"
#endif


//-----------------------------------------------------------------

#define	V_SPACE	13
#define	V_TOP	1
#define	V_LEFT	6
#define	V_RIGHT	20
#define MARK_SPACE 12

//-----------------------------------------------------------------

typedef	struct {
	void	*next;
	char	name[32];
	long	message;
	char	f_enable;
	char	f_key;
	char	f_mark;
}	menu_item;

//-----------------------------------------------------------------

class TMenu {

public:
		TWindow		*menu_window;
		TMenu		*next_menu;
		long		hit_range_left;
		long		hit_range_right;
		long		width;
		char		has_mark;
private:
		char		title[32];
		long		item_count;
		menu_item	*items;
		rect		current_bound;
		long		cur_item;
public:
					TMenu(char *name);
		void		add_item(char *name, long message, char fkey, char enable);
		long		menu_track(rect *, long, char*);
		void		menu_draw();
		char		track_inside(rect *, long);
		char		other_menu(rect *start_rect, long right_most);
		void		hilite_n(long);
		long		calc_width();
		menu_item	*get_item(long);
		menu_item	*find_item(long message);
		char		enable(long);
inline		char		*get_name() { return(title); }
inline		long		count() { return(item_count); }
inline		TMenu		*next() { return(next_menu); }

private:	
		void		local_to_screen(rect *);
};

#endif
#endif
