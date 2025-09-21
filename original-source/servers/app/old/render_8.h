//******************************************************************************
//
//	File:		render.h
//
//	Description:	8 bit proto
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//			new oct 30 1992 bgs
//
//******************************************************************************/
 
#include <stdio.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

/*--------------------------------------------------------------*/

#define	write_long(ptr, what)			\
	*((long *)ptr) = what;
#define	write_double(ptr, what)			\
	*((double *)ptr) = what;
#define	read_long(ptr, what)			\
	what = *(long *)ptr;
#define	read_double(ptr, what)			\
	what = *(double *)ptr;


#define	as_long(v)				\
	*((long *)&v)
#define	as_ulong(v)				\
	*((ulong *)&v)

/*--------------------------------------------------------------*/
/* util from the color manager					*/

uchar	color_2_index(register server_color_map *a_cs, rgb_color c);
uchar 	calc_max_8(server_color_map *a_cs, rgb_color c1, rgb_color c2);
uchar 	calc_hilite_8(server_color_map *a_cs, uchar old_color, rgb_color fore_color, rgb_color back_color);
uchar 	calc_min_8(server_color_map *a_cs, rgb_color c1, rgb_color c2);
uchar 	calc_blend1_8(server_color_map *a_cs, rgb_color c1, rgb_color c2);
uchar 	calc_blend_8(server_color_map *a_cs, rgb_color c1, rgb_color c2);
uchar 	calc_add_8(server_color_map *a_cs, rgb_color c1, rgb_color c2);
/*--------------------------------------------------------------*/


void horizontal_line_copy_pattern_8(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);

void horizontal_line_copy_8(port *a_port,
				    		long x1,
				    		long x2,
				    		long y1,
				    		rect *cliprect);

void horizontal_line_other_pattern_8(port *a_port,
				    				long x1,
				    				long x2,
				    				long y1,
				    				rect *cliprect,
				    				pattern *the_pattern,
				    				long phase_x,
				    				long phase_y, 
				    				short mode);

/*--------------------------------------------------------------*/

void vertical_line_copy_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);

void vertical_line_or_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);

void vertical_line_and_pattern_8(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);

/*--------------------------------------------------------------*/

void	pixel_copy_8(port *a_port, long x, long y);

void	pixel_other_8(port *a_port, long x, long y, short mode);

/*--------------------------------------------------------------*/

void	blit_copy_s_eq_d_8(port *src_port,
			 port *dst_port,
			 rect *source_rect,
			 rect *dest_rect);

void	copy_block_1to8(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode);

void	copy_other_1to8(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode);
