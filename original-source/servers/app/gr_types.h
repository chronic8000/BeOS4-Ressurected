//******************************************************************************
//
//	File:			gr_types.h
//	Description:	Structure declarations and assorted constants
//
//	Copyright 1992-1998, Be Incorporated
//
//******************************************************************************/

#ifndef	GR_TYPES_H
#define	GR_TYPES_H

#include <AppDefs.h>
#include <GraphicsDefs.h>
#include <SupportDefs.h>

#ifdef __cplusplus
#include <Font.h>
#include <IRegion.h>
#endif

#include "basic_types.h"
#include "messages.h"
#include "font_defs.h"

#include "shared_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------*/

#define	WS_COUNT		9	/* number of workspaces */
#define MAX_WORKSPACE	32	/* maximum number of workspaces */

/*----------------------------------------------------------------*/

#define FAKE_SCREEN_WIDTH			12	/* if no video card, fake a screen */
#define FAKE_SCREEN_HEIGHT			12	/* if no video card, fake a screen */
#define FAKE_SCREEN_BITS_PER_PIXEL	8	/* if no video card, fake a screen */

/*----------------------------------------------------------------*/

typedef struct {
	bigtime_t last_answer_time;
	float     polling_delay;
	uchar     used_channels;
	uchar     buttons[3];
	uchar     keys[48];	
} game_client;

/*----------------------------------------------------------------*/

typedef struct {
	ulong	res;
	float	rate;
} workspace_res;

/*----------------------------------------------------------------*/

typedef struct {
	long		h;
	long		v;
	long		modifiers;
	long		buttons;
	bigtime_t	time;
	long		clicks;
} button_event;

/*----------------------------------------------------------------*/

/* 	This struct must be kept in sync with the color_map struct on
	the client side, except for the last entry which is private to
	the server side. */

typedef	struct {
	long		uniq_id;
	rgb_color	color_list[256];
	uchar		invert_map[256];
	uchar		inverted[32768];
	uint32		index2ARGB[256];
} server_color_map;

/*----------------------------------------------------------------*/
/* window manager flags for windows prop.			 			  */
/*----------------------------------------------------------------*/

#define	NO_MOVE			0x00000001	/* public flag */
#define	NO_SIZE			0x00000002	/* public flag */
#define	ONLY_VSIZE		0x00000004	/* public flag */
#define ONLY_HSIZE		0x00000008	/* public flag */
#define FIRST_CLICK		0x00000010	/* public flag */
#define	NO_GOAWAY		0x00000020	/* public flag */
#define NO_ZOOM			0x00000040	/* public flag */
#define DISLIKE_FRONT	0x00000080	/* public flag */
#define	NO_WS_SWITCH	0x00000100	/* public flag */
#define	NO_LIVE_SIZE	0x00001000	/* public flag */
#define	DISLIKE_FOCUS	0x00002000	/* public flag */
#define NO_MINIMIZE		0x00004000	/* public flag */
#define WS_WINDOW		0x00008000	/* private flag */
#define NO_DRAWING		0x00010000	/* private flag */
#define	MOVE_TO_WS		0x00020000	/* public flag */
#define LOCK_FOCUS		0x00040000	/* private flag */
#define ASYNC_CONTROLS	0x00080000	/* public flag */
#define BLOCK_WS_SWITCH	0x00100000	/* public flag */
#define VIEWS_OVERLAP	0x00200000	/* public flag */

#define REQUIRE_KILL	0x40000000	/* private flag */
#define SENT_TO_DEBUG	0x80000000	/* private flag */

/*----------------------------------------------------------------*/
/* window wdef dimensions										  */
/*----------------------------------------------------------------*/

#define CLOSE_ICON_WIDTH		14
#define	CLOSE_ICON_HEIGHT		14
#define ZOOM_ICON_WIDTH			14
#define	ZOOM_ICON_HEIGHT		14
#define MINIMIZE_ICON_WIDTH		14
#define MINIMIZE_ICON_HEIGHT	14
#define LEFT_TITLE_GAP			17
#define RIGHT_TITLE_GAP			14
#define ICON_MARGIN				4

/*----------------------------------------------------------------*/
/* window feels													  */
/*----------------------------------------------------------------*/

#define NORMAL					0		/* public value */
#define MODAL_TEAM				1		/* public value */
#define MODAL_LIST				2		/* public value */
#define MODAL_SYSTEM			3		/* public value */
#define FLOATING_TEAM			4		/* public value */
#define FLOATING_LIST			5		/* public value */
#define FLOATING_SYSTEM			6		/* public value */
#define DESKTOP_WINDOW			1024	/* private value */
#define	MENU_WINDOW				1025	/* private value */
#define	WINDOW_SCREEN			1026	/* app_server private */

/*----------------------------------------------------------------*/
/* window alignment												  */
/*----------------------------------------------------------------*/

#define	WINDOW_BYTE_ALIGNMENT	0
#define	WINDOW_PIXEL_ALIGNMENT	1

/*----------------------------------------------------------------*/
/* Global server attributes                                       */
/*----------------------------------------------------------------*/

#define	SECOND_BUTTON_SEND_BACK		0x00000001
#define	FOCUS_FOLLOWS_MOUSE			0x00000002

/*----------------------------------------------------------------*/

#define wind_active		0x0001
#define	look_active		0x0002
#define	known_active	0x0004

/*----------------------------------------------------------------*/

typedef struct
{
	long	firstchar;
	long	lastchar;
	long	width_max;
	long	font_height;
	long	ascent;
	long	descent;
	long	leading;
	long	bm_rowbyte;
	long	default_width;
	uchar	*bitmap;
	short	*offsets;
	char	proportional;
	char	filler1;
} font_desc;

/*----------------------------------------------------------------*/

struct server_edge_info {
	float	left;
	float	right;
};

/*----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
