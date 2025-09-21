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

/*---------------------------------------------------------------------*/

void cursor_blit(port *from_port, port *to_port, rect *srcrect, rect *dstrect);

/*---------------------------------------------------------------------*/

void xblit(port *from_port, port *to_port, rect *srcrect, rect *dstrect, long mode)
{
	rect	f_rect;
	rect	xf_rect;
	rect	t_rect;
	rect	d_rect;
	long	delta;

	
	f_rect = *srcrect;
	offset_rect(&f_rect, from_port->origin.h, from_port->origin.v);
	t_rect = *dstrect;
	offset_rect(&t_rect, to_port->origin.h, to_port->origin.v);

	delta = f_rect.top - from_port->bbox.top;

	if (delta < 0) {
		f_rect.top -= delta;
		t_rect.top -= delta;
	}

	delta = f_rect.left - from_port->bbox.left;

	if (delta<0) {
		f_rect.left -= delta;
		t_rect.left -= delta;
	}

	delta = f_rect.bottom - from_port->bbox.bottom;

	if (delta > 0) {
		f_rect.bottom -= delta;
		t_rect.bottom -= delta;
	}

	delta = f_rect.right - from_port->bbox.right;

	if (delta > 0) {
		f_rect.right -= delta;
		t_rect.right -= delta;
	}

	if ((f_rect.left > f_rect.right) ||
		(f_rect.top > f_rect.bottom))
		return;
	
/* ??? Benoit: wouldn't bound be ok here? Eric */
	sect_rect(t_rect,to_port->draw_region->data[0],&d_rect);
	if (empty_rect(d_rect))
		return;
	
	f_rect.left  += (d_rect.left - t_rect.left);
	f_rect.top   += (d_rect.top - t_rect.top);
	f_rect.bottom -= (t_rect.bottom - d_rect.bottom);
	f_rect.right -= (t_rect.right - d_rect.right);
	
	sect_rect(f_rect,from_port->draw_region->data[0],&xf_rect);

	t_rect.left  += (xf_rect.left - f_rect.left);
	t_rect.top   += (xf_rect.top - f_rect.top);
	t_rect.bottom -= (f_rect.bottom - xf_rect.bottom);
	t_rect.right -= (f_rect.right - xf_rect.right);
	
	switch_blit(from_port, to_port, &xf_rect, &d_rect, mode);
	/*
	switch(mode) {
		case op_copy:
			blit_copy(from_port, to_port, &f_rect, &d_rect);
			break;
		case op_and:
			blit_and(from_port, to_port, &f_rect, &d_rect);
			break;
		case op_or:
			blit_or(from_port, to_port, &f_rect, &d_rect);
			break;
	}
	*/
}

/*----------------------------------------------------------------*/

