
//******************************************************************************
//
//	File:			wdef.cpp
//	Description:	WDEF support stuff
//	Written by:		Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//******************************************************************************

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef	WINDOW_H
#include "window.h"
#endif

#ifndef	_TEXT_H
#include "text.h"
#endif

#include "rect.h"

/*---------------------------------------------------------------*/

extern bool	are_frames_colored[2]	= {
	FALSE,	/* non-modal */
	FALSE	/* modal */
};

extern bool	use_default_frame_colors[2]	= {
	TRUE,	/* non-modal */
	TRUE	/* modal */	/* modals don't have titles so this is a no-op */
};

rgb_color	custom_frame_colors[2] = {
	{170,170,120,0},		/* non-modal */
	{255 ,150, 102, 0}		/* modal */
};

extern bool	are_titles_colored = TRUE;
extern bool use_default_title_color = TRUE;
rgb_color	custom_title_colors[2] = {
	{170,170,120,0},		/* non-minimized */
	{200 ,150, 102, 0}		/* minimized */
};

#define def_mini_bkg			{102, 203, 203, 0}

// default colors for an active frame drawn in black & white
#define def_bw_bkg_light		{255, 255, 255, 0}
#define def_bw_bkg				{216, 216, 216, 0}
#define def_bw_bkg_dark			{184, 184, 184, 0}

// default colors for an active frame drawn in our standard yellow
#define def_bkg_light			{255, 255, 102, 0}
#define def_bkg					{255, 203, 0, 0}
#define def_bkg_dark			{255, 152, 0, 0}

// default colors for an active alert frame drawn in our standard red
#define def_modal_bkg_light		{255, 152, 152, 0}
#define def_modal_bkg			{255, 102, 102, 0}
#define def_modal_bkg_dark		{203, 51, 51, 0}

// default colors for an inactive frame drawn
#define def_ia_bkg_light		{255, 255, 255, 0}
#define def_ia_bkg				{232, 232, 232, 0}
#define def_ia_bkg_dark			{216, 216, 216, 0}

// default colors for an active frame drawn in our standard yellow
#define def_mini_bkg_light		{203, 255, 255, 0}
#define def_mini_bkg			{102, 203, 203, 0}
#define def_mini_bkg_dark		{51, 152, 152, 0}

// window_type X is_colored X active X intensitiy
//		window_type	:	0 is non-modal, 1 is modal
//		is_colored	:	0 is not colored, 1 is colored
//		active		:	0 is not active, 1 is active
//		intensity	:	0 is light version, 1 is standard, 2 is dark version

const rgb_color	default_color_scheme[2][2][2][3] = {
	// non-modal windows
	{
		// not colored
		{
			// not active
			{def_ia_bkg_light,		def_ia_bkg,		def_ia_bkg_dark},
			// active
			{def_bw_bkg_light,		def_bw_bkg,		def_bw_bkg_dark}
		},
		// colored
		{
			// not active
			{def_ia_bkg_light,		def_ia_bkg,		def_ia_bkg_dark},
			// active
			{def_bkg_light,			def_bkg,		def_bkg_dark}
		}
	},
	// modal windows
	{
		// not colored
		{
			// not active
			{def_ia_bkg_light,		def_ia_bkg,		def_ia_bkg_dark},
			// active
			{def_bw_bkg_light,		def_bw_bkg,		def_bw_bkg_dark}
		},
		// colored
		{
			// not active
			{def_ia_bkg_light,		def_ia_bkg,		def_ia_bkg_dark},
			// active
			{def_modal_bkg_light,	def_modal_bkg,	def_modal_bkg_dark}
		}
	}
};

// default color scheme for minimized windows.
const rgb_color	default_mini_color_scheme[2][3] = {
	{def_bw_bkg_light,			def_bw_bkg,			def_bw_bkg_dark},
	{def_mini_bkg_light,		def_mini_bkg,		def_mini_bkg_dark}
};

/*---------------------------------------------------------------*/

static uchar shift_component(uchar val, float percent)
{
	if (percent >= 1.0)
		return (val * (2.0 - percent));
	else
		return (255 - (percent * (255-val)));
}

/* ----------------------------------------------------------- */

rgb_color	shift_color(rgb_color c, float percent)
{
	rgb_color n;

	n.red = shift_component(c.red, percent);
	n.green = shift_component(c.green, percent);
	n.blue = shift_component(c.blue, percent);

	return n;
}

/* ---------------------------------------------------------------- */

rgb_color	FrameColor(float percent, TWindow *window)
{
	bool		active = window->looks_active();
//	bool		modal = ((window->flags & MODAL) != 0);
	bool		modal = 0;
	rgb_color	c;

	if (use_default_frame_colors[modal] || !active || !are_frames_colored[modal]) {
		// window_type X is_colored X active X intensitiy
		long intensity = 1;
		if (percent < 0.99)
			intensity = 0;
		else if (percent > 1.01)
			intensity = 2;

		bool colored = are_frames_colored[modal];
		c = default_color_scheme[modal][colored][active][intensity];
	} else {
		c = shift_color(custom_frame_colors[modal], percent);
	}

	return c;
}

/*---------------------------------------------------------------*/

rgb_color	TitleColor(float percent, TWindow *window)
{
	bool		active = window->looks_active();
//	bool		modal = ((window->flags & MODAL) != 0);
	bool		modal = 0;
	bool		mini = window->is_mini;

	rgb_color	c;

	if (mini) {
		active = TRUE;
	}

	if (use_default_title_color || !active || !are_titles_colored) {
		// window_type X is_colored X active X intensitiy
		long intensity = 1;
		if (percent < 0.99)
			intensity = 0;
		else if (percent > 1.01)
			intensity = 2;

		bool colored = are_titles_colored;
		if (mini) 
			c = default_mini_color_scheme[colored][intensity];
		else
			c = default_color_scheme[modal][colored][active][intensity];
	} else {
		c = shift_color(custom_title_colors[mini], percent);
	}

	return c;
}
