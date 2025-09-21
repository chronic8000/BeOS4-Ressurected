//******************************************************************************
//
//	File:		blit.c
//
//	Description:	1 bit blitting and scrolling.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/
 
 
#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef	DISPATCH_H
#include "dispatch.h"
#endif

#if 0

/*--------------------------------------------------------------------*/

void	switch_blit(port *from_port, port *to_port, rect *f_rect, rect *t_rect, long mode)
{
	switch(mode & 0xff) {
		case op_copy	:
			blit_copy(from_port, to_port, f_rect, t_rect);
			break;

		case op_or	:
			blit_or(from_port, to_port, f_rect, t_rect);
			break;

		case op_and	:
			blit_and(from_port, to_port, f_rect, t_rect);
			break;
			
		case op_xor	:
			blit_xor(from_port, to_port, f_rect, t_rect);
			break;

		case op_blend	:
			blit_blend(from_port, to_port, f_rect, t_rect);
			break;
		
		case op_sub	:
			blit_sub(from_port, to_port, f_rect, t_rect);
			break;
	
		case op_add	:
			blit_add(from_port, to_port, f_rect, t_rect);
			break;

		case op_min	:
			blit_min(from_port, to_port, f_rect, t_rect);
			break;
	
		case op_max	:
			blit_max(from_port, to_port, f_rect, t_rect);
			break;
	}
}


/*--------------------------------------------------------------------*/

void blit_complex(port *from_port, port *to_port, rect f_rect, rect t_rect, long mode)
{
	short	i;
	long	count;
	rect	r1;
	rect	r2;
	rect	r3;
	region	*tmp_region1;
	region	*clip;
	long	dh;
	long	dv;

	if (from_port->base != to_port->base) {
		if (!rect_in_region(to_port->draw_region, &t_rect))
			return;
	}


/* if the source and destination ports are not the same	*/
/* then we've got no problems with overlapping areas	*/

	dh = t_rect.left - f_rect.left;
	dv = t_rect.top -  f_rect.top;

	if ((mode & 0xffffff00) == 0) {
		tmp_region1 = newregion();
		clip = newregion();

/* get a copy of the current clip in tmp_region	*/

		if (from_port == to_port) {
			copy_region(from_port->draw_region, tmp_region1);
		}
		else
			set_region(tmp_region1, &(from_port->bbox));

/* offset it by the scrolling dh, dv		*/

		offset_region(tmp_region1, dh, dv);

/* and, and it with the clipping		*/
/* After that, clip should be the target area	*/
/* for the blit.				*/

		and_region(to_port->draw_region, tmp_region1, clip);
		kill_region(tmp_region1);
	}
	else {
		clip = newregion();
		copy_region(to_port->draw_region, clip);
	}	

	count = clip->count;

	if (from_port->base != to_port->base) {

/* scan all the rectangles of the destination region */

		for (i = 0; i < count; i++) {
			r1 = ra(clip, i);
			sect_rect(r1, t_rect, &r2);

/* If any intersection with the target rect */

			if (!empty_rect(r2)) {

/* size a source rect matching the current destination rect */

				r1 = f_rect;
				r1.left  += (r2.left - t_rect.left);
				r1.top   += (r2.top  - t_rect.top);
				r1.bottom -= (t_rect.bottom - r2.bottom);
				r1.right -= (t_rect.right - r2.right);
		

/* and blit according to the transfer mode */

				switch_blit(from_port, to_port, &r1, &r2, mode);

			}
		}
		kill_region(clip);
		return;
	}


/* now, this is the hard case when source and destinations can overlap	*/
/* make it as a block so that the stack allocation won't always be done */
/* !! Benoit, it seems that the allocation is always done !!		*/

	if (!empty_region(clip)) {
		rect	*dests;
		long	r_count;
		long	again;
		long	j, i;

		dests = (rect *)gr_malloc(count * sizeof(rect));

		r_count = 0;

/* hint hint, it could be efficient to scan the list in reverse order	*/
/* when the dest in y greater than src					*/
/* &&& Benoit, check how long this stuff takes in hard cases, also 	*/
/* check to see if this algorithm always works and never locks		*/

/* Since we backup one scan line at a time, we can only work on vertical*/
/* overlap problems.							*/
/* Here are the two cases for up or down scrolling			*/
/* Where we put rectangles in the sort list in a pre order that will be */
/* more efficient for each of the two cases.				*/


		if (dv < 0) {

/* for all the rectangle in the blit dst region				*/

			for (i = 0; i < count; i++) {
				r1 = ra(clip, i);
/* sect with the target rectangle					*/
				sect_rect(r1, t_rect, &r2);
/* if sect, add it to the list						*/
				if (!empty_rect(r2)) {
					dests[r_count] = r2;
					r_count++;
				}
			}
		}
/* Same things but reverse order					*/
		else {
			for (i = (count - 1); i >= 0; i--) {
				r1 = ra(clip, i);
				sect_rect(r1, t_rect, &r2);
				if (!empty_rect(r2)) {
					dests[r_count] = r2;
					r_count++;
				}
			}
		}


/* swap list elements until there is no overlap		*/

		again = 1;
		while (again != 0) {
			again = 0;


			for (i = 0; i < r_count; i++) {

				r1 = dests[i];

				for (j = (i + 1); j < r_count; j++) {

					r2 = dests[j];
					offset_rect(&r2, -dh, -dv);

					sect_rect(r1, r2, &r3);

					if (!empty_rect(r3)) {
						r3 = dests[i + 1];
						dests[i + 1] = dests[i];
						dests[i] = r3;
						again = 1;
						goto out;
					}
				}
				out:;
			}
		}	   	



		for (i = 0; i < r_count; i++) {
			r1 = dests[i];
			r2 = r1;
			
			offset_rect(&r2, -dh, -dv);

			switch_blit(from_port, to_port, &r2, &r1, mode);

		}
		gr_free((char *)dests);
	}
	kill_region(clip);
}

/*---------------------------------------------------------------------*/

void	preload_bitmap(port *a_port, rect r)
{
	volatile uchar	*from;
	volatile uchar	*to;
	uchar			bid;

	offset_rect(&r, a_port->origin.h, a_port->origin.v);
	
	if (r.top < 0)
		return;
	if (r.bottom < 0)
		return;
	if (r.top > a_port->bbox.bottom)
		return;

	if (r.bottom > a_port->bbox.bottom)
		r.bottom = a_port->bbox.bottom;

	from = a_port->base + (a_port->rowbyte*r.top);
	to 	 = a_port->base + (a_port->rowbyte*r.bottom);


	while(from < to) {
		bid = *from;
		from += 4096;
	}
}

/*---------------------------------------------------------------------*/

void blit_ns(port *from_port, port *to_port, rect *srcrect, rect *dstrect, long mode)
{
	rect		f_rect;
	rect		t_rect;
	rect		tmp_rect;
	rect		tmp_rect1;
	long		delta;
	long		pid;
	char		from_on_screen;
	char		to_on_screen;
	
	preload_bitmap(from_port, *srcrect);
	pid = wait_regions();
//	port_change(from_port, pid);
	if (from_port != to_port) {
//		port_change(to_port, pid);
	}

	f_rect = *srcrect;
	offset_rect(&f_rect, from_port->origin.h, from_port->origin.v);
	t_rect = *dstrect;
	offset_rect(&t_rect, to_port->origin.h, to_port->origin.v);

	if (!rect_in_region(to_port->draw_region, &t_rect)) {
		signal_regions();
		return;
	}
	
	delta = f_rect.top - from_port->bbox.top;

	if (delta < 0) {
		f_rect.top -= delta;
		t_rect.top -= delta;
	}

	delta = f_rect.left - from_port->bbox.left;

	if (delta < 0) {
		f_rect.left -= delta;
		t_rect.left -= delta;
	}

	from_on_screen = (from_port->port_type & OFFSCREEN_PORT) == 0;
	to_on_screen = (to_port->port_type & OFFSCREEN_PORT) == 0;

	if (from_on_screen || to_on_screen) {
		get_screen();
	
		if (to_on_screen) {
			tmp_rect1 = to_port->draw_region->bound;
			tmp_rect.top  	= max(t_rect.top, tmp_rect1.top);
			tmp_rect.left 	= max(t_rect.left, tmp_rect1.left);
			tmp_rect.bottom = min(t_rect.bottom, tmp_rect1.bottom);
			tmp_rect.right  = min(t_rect.right, tmp_rect1.right);
 
			cursor_hide(tmp_rect);
		}
		if (from_on_screen) {
			tmp_rect1 = from_port->draw_region->bound;
			tmp_rect.top  	= min(f_rect.top, tmp_rect1.top);
			tmp_rect.left 	= min(f_rect.left, tmp_rect1.left);
			tmp_rect.bottom = max(f_rect.bottom, tmp_rect1.bottom);
			tmp_rect.right  = max(f_rect.right, tmp_rect1.right);
	
			cursor_hide(tmp_rect);
		}
	}	
	
	// if the blit has the same source and destination rectangle size					
	if (((f_rect.bottom - f_rect.top) == (t_rect.bottom - t_rect.top))
	    && ((f_rect.right - f_rect.left) == (t_rect.right - t_rect.left))) {
		// if we can special case for simple regions (one rectangle), do it
		if (((mode & 0xffffff00) == 0 && region_is_rect(to_port->draw_region) && region_is_rect(from_port->draw_region)) ||
		    ((mode & 0xffffff00) != 0 && region_is_rect(to_port->draw_region))) {
			rect	r1, r2;
			
			r1 = to_port->draw_region->data[0];

			/* If we're paying attention to source clipping here */
			if ((mode & 0xffffff00) == 0) {
				/* Offset the current source clip rect to match-up with the dest rect */
				
				if (from_port == to_port) {
					r2 = from_port->draw_region->data[0];

				}
				else
					r2 = from_port->bbox;

				offset_rect(&r2, t_rect.left - f_rect.left, t_rect.top -  f_rect.top);

				/* Get the intersection of the two clip rects to be the new clip rect */
				sect_rect(r1, r2, &r1);
			}
			
			/* Get intersection of dest rect and its cliprect */
			sect_rect(r1, t_rect, &r2);

			/* If there is any intersection to blit to */
			if (!empty_rect(r2)) {
				/* Size the source rect to match the current destination rect */
				r1 = f_rect;
				r1.left		+= (r2.left - t_rect.left);
				r1.top		+= (r2.top  - t_rect.top);
				r1.bottom	-= (t_rect.bottom - r2.bottom);
				r1.right	-= (t_rect.right - r2.right);
		
				/* We've got the smallest source and dest rects we know how to make
				   so blit according to the transfer mode */
				
				switch_blit(from_port, to_port, &r1, &r2, mode);

			}
		    
		}
		/* else the blit has a complex region */
		else
			blit_complex(from_port, to_port, f_rect, t_rect, mode);
	}
	/* else we have to do bit streching */
	else {
		if (from_on_screen || to_on_screen)
			release_screen();
	
		blit_scale(from_port, to_port, srcrect, dstrect, mode);
		signal_regions();
		return;
	}
	
	if (from_on_screen || to_on_screen)
		release_screen();
	signal_regions();
}

/*----------------------------------------------------------------*/

void blit(port *from_port, port *to_port, frect *srcrect, frect *dstrect, long mode, float scale)
{
	rect	dest;
	rect	src;

	dest = scale_rect(*dstrect, scale);
	src = scale_rect(*srcrect, 1.0);
	blit_ns(from_port, to_port, &src, &dest, mode);
}

#endif
