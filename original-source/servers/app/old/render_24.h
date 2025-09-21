#include "gr_types.h"

/*#define	TRANSPARENT_LONG -1L
*/
#define	TRANSPARENT_LONG 0x77747300

typedef union {
		uchar	as_char[4];
		ushort	as_short[2];
		ulong	as_long;
} trick;

// prototypes

void horizontal_line_xor_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_or_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_copy_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_and_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_and_24(port *a_port,
			   long x1,
			   long x2,
			   long y1,
			   rect *cliprect);
void vertical_line_copy_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);

ulong calc_max_24(ulong c1, ulong c2);
ulong calc_min_24(ulong c1, ulong c2);
ulong calc_blend1_24(ulong c1, ulong c2);
ulong calc_blend_24(ulong c1, ulong c2);
ulong calc_add_24(ulong c1, ulong c2);
ulong calc_sub_24(ulong c1, ulong c2);
ulong calc_hilite_24(ulong old_value, ulong fore_color, ulong back_color);

void pixel_xor_24(port *a_port, long x, long y);
void pixel_and_24(port *a_port, long x, long y);
void pixel_copy_24(port *a_port, long x, long y);
void	pixel_add_24(port *a_port, long x, long y);
void	pixel_sub_24(port *a_port, long x, long y);
void	pixel_min_24(port *a_port, long x, long y);
void	pixel_max_24(port *a_port, long x, long y);
void	pixel_blend_24(port *a_port, long x, long y);
void	pixel_other_24(port *a_port, long x, long y, short mode);
void vertical_line_other_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y,
			        short mode);
void vertical_line_add_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_sub_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_min_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_max_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_blend_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_xor_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_or_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void vertical_line_and_pattern_24(port *a_port,
			        long x1,
			        long y1,
			        long y2,
			        rect *cliprect,
			        pattern *the_pattern,
			        long phase_x,
			        long phase_y);
void horizontal_line_other_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y, 
				    short mode);
void horizontal_line_add_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_sub_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_min_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_max_pattern_24(port *a_port,
				    long x1,
				    long x2,
				    long y1,
				    rect *cliprect,
				    pattern *the_pattern,
				    long phase_x,
				    long phase_y);
void horizontal_line_blend_pattern_24(	port *a_port,
					    long x1,
					    long x2,
					    long y1,
					    rect *cliprect,
					    pattern *the_pattern,
					    long phase_x,
					    long phase_y);

void copy_other_8to24(	port *from_port,
						port *dst_port,
						rect *from,
						rect *to,
						short mode);
void	copy_block_8to24copy_s  (port *from_port,
							   port *dst_port,
							   rect *from,
							   rect *to);
void	copy_block_8to24copy(port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to);
void	copy_block_8to24or  (port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to);
void	copy_block_1to241or(port *from_port,
			port *dst_port,
			rect *from,
			rect *to);
void	copy_block_1to241or_s  (port *from_port,
							   port *dst_port,
							   rect *from,
							   rect *to);
void	copy_block_1to241copy(port *from_port,
							 port *dst_port,
							 rect *from,
							 rect *to);
void	copy_block_1to241(port *from_port,
			port *dst_port,
			rect *from,
			rect *to,
			short mode);
void	blit_to_screen_24  (port *src_port,
			 			 port *dst_port,
			 			 rect *src_rect,
			 			 rect *dst_rect);
void	blit_copy_s_eq_d_24(port *src_port,
			 			 port *dst_port,
			 			 rect *source_rect,
			 			 rect *dest_rect);
long text8_24 (port *a_port,
	       	   long h,
	       	   long v,
	       	   char *str,
	       	   font_desc *desc,
	       	   rect clipbox,
	       	   short mode);

void	blit_xor_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_and_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_or_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_blend_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_add_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_sub_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_min_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_max_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);
void	blit_copy_24(port *src_port, port *dst_port, rect *src_rect, rect *dst_rect);

