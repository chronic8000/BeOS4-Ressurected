//******************************************************************************
//
//	File:		picture.cpp
//
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1995, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _PICTURE_H
#include "picture.h"
#endif
#ifndef	_MESSAGES_H
#include "messages.h"
#endif
#ifndef	VIEW_H
#include "view.h"
#endif
#ifndef	GLUE_H
#include "glue.h"
#endif
#ifndef	_STDLIB_H
#include <stdlib.h>
#endif
#ifndef	_STRING_H
#include <string.h>
#endif
#ifndef	_TEXT_H
#include "text.h"
#endif
#include <math.h>

//----------------------------------------------------------------------------
extern "C" void	*
		sh_realloc(void *p, long size);
//----------------------------------------------------------------------------

void	TPicture::init_state()
{
	state.pen_size = -1;
	state.h = -2234567.22;
	state.v = -2243274.33;
	state.mode = -1;
	state.front = rgb(22,33,88);
	state.back = rgb(22,44,33);
	state.f_state.init_no_state();
}


//----------------------------------------------------------------------------
#define	as_long(v)				\
	*((long *)&v)
//----------------------------------------------------------------------------

void	TPicture::resync_state(MView *a_view)
{
	state.h = a_view->pen_h;
	state.v = a_view->pen_v;
}

//----------------------------------------------------------------------------

void	TPicture::compare_state(MView *a_view)
{
	long	    col1, col2;
	fc_context  *ctxt;

	if (state.pen_size != a_view->pen_size) {
		add_opcode(0, PIC_SET_PEN_SIZE);
		add_float(a_view->pen_size);
		state.pen_size = a_view->pen_size;
	}

	if ((state.h != a_view->pen_h) || (state.v != a_view->pen_v)) {
		add_opcode(0, PIC_MOVETO);
		add_float(a_view, a_view->pen_h, a_view->pen_v);
		state.h = a_view->pen_h;
		state.v = a_view->pen_v;
	}

	if (state.mode != a_view->draw_mode) {
		add_opcode(0, PIC_SET_DRAW_MODE);
		add_long(a_view->draw_mode);
		state.mode = a_view->draw_mode;
	}
	
	if (as_long(state.front) != as_long(a_view->fore_color)) {
		add_opcode(0, PIC_FORE_COLOR);
		add_data(&a_view->fore_color, 4);
		state.front = a_view->fore_color;
	}

	if (as_long(state.back) != as_long(a_view->back_color)) {
		add_opcode(0, PIC_BACK_COLOR);
		add_data(&a_view->back_color, 4);
		state.back = a_view->back_color;
	}

	ctxt = a_view->font_context;
	
	if (state.f_state.escape_size != ctxt->escape_size()) {
		add_opcode(0, PIC_SET_FONT_SIZE);
		add_float(ctxt->escape_size());
		state.f_state.escape_size = ctxt->escape_size();
	}	

	if (state.f_state.family_id != ctxt->family_id()) {
		char 	*tmp;
		long	length;
		
		tmp = fc_get_family_name(ctxt->family_id());
		if (tmp) {
			add_opcode(0, PIC_SET_FONT_FAMILY);
			length = strlen(tmp) + 1;
			add_long(length);
			add_data((void*)tmp, length);
			state.f_state.family_id = ctxt->family_id();
		}
	}

	if (state.f_state.style_id != ctxt->style_id()) {
		char 	*tmp;
		long	length;
		
		tmp = fc_get_style_name(ctxt->family_id(), ctxt->style_id());
		if (tmp) {
			add_opcode(0, PIC_SET_FONT_STYLE);
			length = strlen(tmp) + 1;
			add_long(length);
			add_data((void*)tmp, length);
			state.f_state.style_id = ctxt->style_id();
		}
	}

	if (state.f_state.encoding != ctxt->encoding()) {
		add_opcode(0, PIC_SET_FONT_ENCODING);
		add_long(ctxt->encoding());
		state.f_state.encoding = ctxt->encoding();
	}

	if (state.f_state.shear != ctxt->shear()) {
		add_opcode(0, PIC_SET_FONT_SHEAR);
		add_float(ctxt->shear());
		state.f_state.shear = ctxt->shear();
	}

	if (state.f_state.rotation != ctxt->rotation()) {
		add_opcode(0, PIC_SET_FONT_ROTATE);
		add_float(ctxt->rotation());
		state.f_state.rotation = ctxt->rotation();
	}

	if (state.f_state.spacing_mode != ctxt->spacing_mode()) {
		add_opcode(0, PIC_SET_FONT_SPACING);
		add_long(ctxt->spacing_mode());
		state.f_state.spacing_mode = ctxt->spacing_mode();
	}

	if (state.f_state.bits_per_pixel != ctxt->spacing_mode()) {
		add_opcode(0, PIC_SET_FONT_BPP);
		add_long(ctxt->bits_per_pixel());
		state.f_state.bits_per_pixel = ctxt->bits_per_pixel();
	}

	if (state.f_state.flags != ctxt->flags()) {
		add_opcode(0, PIC_SET_FONT_FLAGS);
		add_long(ctxt->flags());
		state.f_state.flags = ctxt->flags();
	}

	if (state.f_state.faces != ctxt->faces()) {
		add_opcode(0, PIC_SET_FONT_FACES);
		add_long(ctxt->faces());
		state.f_state.faces = ctxt->faces();
	}
}

//----------------------------------------------------------------------------

	TPicture::TPicture()
{
	data_ptr = (char *)sh_malloc_g(SIZE_INC);
	avail_size = SIZE_INC;
	used_size = 0;
	cur_ptr = data_ptr;
	init_state();
}

//----------------------------------------------------------------------------

	TPicture::TPicture(long size)
{
	if (size > 0 && (size < 25000000)) {
		data_ptr = (char *)sh_malloc_g(size);
		avail_size = size;
		used_size = 0;
		cur_ptr = data_ptr;
	}
	else
		data_ptr = 0;
	init_state();
}

//----------------------------------------------------------------------------

char	TPicture::ok()
{
	if (data_ptr)
		return(1);
	else
		return(0);
}

//----------------------------------------------------------------------------

long TPicture::get_size()
{
	return(used_size);
}

//----------------------------------------------------------------------------

	TPicture::~TPicture()
{
	sh_free_g((char *)data_ptr);
}

//----------------------------------------------------------------------------

void *TPicture::get_shared_data_ptr()
{
	return((void*)data_ptr);
}

//----------------------------------------------------------------------------

void	TPicture::start_record()
{
	used_size = 0;
}

//----------------------------------------------------------------------------

void	TPicture::end_record()
{
}

//----------------------------------------------------------------------------

void	TPicture::add_data(void *data, long length)
{
	long	new_size;

	if (used_size+length > avail_size) {
		new_size = (used_size*1.3)+length+SIZE_INC;
		data_ptr = (char *)sh_realloc(data_ptr, new_size);
		avail_size = new_size;
		cur_ptr = data_ptr + used_size;
	}

	if (data_ptr) {
		if (length == 4) {
			*(long*)cur_ptr = *(long*)data;
		}
		else
			memcpy(cur_ptr, (char*)data, length);
		used_size += length;
		cur_ptr += length;
	}
}

//----------------------------------------------------------------------------

void	TPicture::done_with_port(port *a_port)
{
	kill_region(a_port->draw_region);
	gr_free((char *)a_port);
}

//----------------------------------------------------------------------------

port  *TPicture::pget_port()
{
	long	bit_per_pixel;
	long	rowbyte;
	rect	bbox;
	long	bits_size;
	char	*base;
	port	*aport;
	region	*tmp_region;
	long	ptype;

	ptype = get_long();
	bit_per_pixel = get_long();
	rowbyte = get_long();
	bbox = get_irect();
	bits_size = get_long();
	base = src_ptr;
	src_ptr += bits_size;

	aport=(port *)gr_malloc(sizeof(port));
	
	aport->port_type		= ptype;
	aport->shared_port   	= 0;
	aport->bit_per_pixel	= bit_per_pixel;
	aport->base				= (uchar *)base;
	aport->rowbyte			= rowbyte;
	aport->bbox				= bbox;
	aport->origin.h			= bbox.left;
	aport->origin.v			= bbox.top;
	aport->fore_color		= rgb(0, 0, 0);
	aport->back_color		= rgb(255, 255, 255);
	aport->port_cs			= system_cs;
	aport->gr_procs			= gr_procs_8;
	aport->area				= 0;

	tmp_region=newregion();
	set_region(tmp_region,&bbox);
	aport->draw_region = tmp_region;
	return(aport);
}

//----------------------------------------------------------------------------

void  TPicture::add_port(port *a_port)
{
	long	rowbyte;
	long	bitmap_size;
	rect	bound;

	bound = a_port->bbox;

// Needs to plan the case of other bits per pixel

	//rowbyte = ((bound.right - bound.left + 4) >> 2) << 2;
	rowbyte = a_port->rowbyte;
	bitmap_size = ((bound.bottom - bound.top) + 1) * rowbyte;
	
	add_long(a_port->port_type);
	add_long(a_port->bit_per_pixel);
	add_long(a_port->rowbyte);
	add_irect(0, a_port->bbox);
	add_long(bitmap_size);
	add_data(a_port->base, bitmap_size);
}

//----------------------------------------------------------------------------

void	TPicture::get_data(long length, void *p)
{
	memcpy((char *)p, src_ptr, length);
	src_ptr += length;
}

//----------------------------------------------------------------------------

long	TPicture::get_long()
{
	long	a_long;

	memcpy((char *)&a_long, src_ptr, sizeof(long));
	src_ptr += sizeof(long);
	return(a_long);
}

//----------------------------------------------------------------------------

float	TPicture::get_float()
{
	float	a_float;

	memcpy((char *)&a_float, src_ptr, sizeof(float));
	src_ptr += sizeof(float);
	return(a_float);
}

//----------------------------------------------------------------------------

rect	TPicture::get_irect()
{
	rect	a_rect;

	memcpy((char *)&a_rect, src_ptr, sizeof(rect));
	src_ptr += sizeof(rect);
	return(a_rect);
}

//----------------------------------------------------------------------------

frect	TPicture::get_rect()
{
	frect	a_rect;

	memcpy((char *)&a_rect, src_ptr, sizeof(frect));
	src_ptr += sizeof(frect);
	return(a_rect);
}

//----------------------------------------------------------------------------

void	TPicture::add_opcode(MView *a_view, long a_long)
{
	
	if (a_view)
		compare_state(a_view);

	add_data(&a_long, sizeof(long));
}

//----------------------------------------------------------------------------
	
void	TPicture::add_long(long a_long)
{
	add_data(&a_long, sizeof(long));
}

//----------------------------------------------------------------------------

void	TPicture::add_long(long a_long1, long a_long2)
{
	long	tmp[2];

	tmp[0] = a_long1;
	tmp[1] = a_long2;
	
	add_data(tmp, 2*sizeof(long));
}

//----------------------------------------------------------------------------

void	TPicture::add_float(TView *a_view, float a_float1, float a_float2)
{
	float	tmp[2];

	tmp[0] = a_float1;
	tmp[1] = a_float2;
	if (a_view) {
		tmp[0] += a_view->pic_offset_h;
		tmp[1] += a_view->pic_offset_v;
	}	

	add_data(tmp, 2*sizeof(float));
}

//----------------------------------------------------------------------------

void	TPicture::add_rect(TView *a_view, frect a_rect)
{
	if (a_view) {
		a_rect.top += a_view->pic_offset_v;
		a_rect.left += a_view->pic_offset_h;
		a_rect.bottom += a_view->pic_offset_v;
		a_rect.right += a_view->pic_offset_h;
	}
	add_data(&a_rect, sizeof(frect));
}

//----------------------------------------------------------------------------

void	TPicture::add_irect(TView *a_view, rect a_rect)
{
	if (a_view) {
		a_rect.top += a_view->pic_offset_v;
		a_rect.left += a_view->pic_offset_h;
		a_rect.bottom += a_view->pic_offset_v;
		a_rect.right += a_view->pic_offset_h;
	}
	add_data(&a_rect, sizeof(rect));
}

//----------------------------------------------------------------------------
	
void	TPicture::add_float(float a_float)
{
	add_data(&a_float, sizeof(float));
}

//----------------------------------------------------------------------------

void	TPicture::reset()
{
	src_ptr = data_ptr;
}

//----------------------------------------------------------------------------

void	TPicture::replay(MView *a_view, float hh, float vv)
{
	long	opcode;
	long	pid = getpid();
	char	buf[64];
	long	i=0;	
	float	stack_h[64];
	float	stack_v[64];
	long	stack_pointer;

	a_view->set();
	//a_view->scale = 4.0;
	stack_pointer = 0;

	reset();
	while(1) {
		opcode = get_long();	
		i++;
		//xprintf("opcode=%ld %lx\n", opcode, opcode);
		switch(opcode) {
//-------------------------------------------------------------
// moveto absolute position.
//
// 4 bytes. GR_MOVETO
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_MOVETO :			//501	//done
				{
				a_view->pen_h = hh+get_float();
				a_view->pen_v = vv+get_float();
				}
				break;

//-------------------------------------------------------------
// Line to absolute position.
//
// 4 bytes. PIC_LINETO
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_LINETO :		//502 done
				{
				float	tmp1;
				float	tmp2;
				
				tmp1 = hh+get_float();
				tmp2 = vv+get_float();
				lineto_p(pid, tmp1, tmp2, a_view);
				a_view->pen_h = tmp1;
				a_view->pen_v = tmp2;
				}
				break;

//-------------------------------------------------------------
// rect fill. 
//
// 4 bytes. PIC_RECTFILL
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_RECTFILL :		//507
				{
				frect	r;
				pattern	p;

				r = get_rect();
				offset_frect(&r, hh, vv);
				get_data(sizeof(pattern), &p);
				rect_fill_p(pid, &r, &p, a_view);
				}
				break;

//-------------------------------------------------------------
// rect invert. 
//
// 4 bytes. PIC_RECT_INVERT
//16 bytes. absolute rectangle
//-------------------------------------------------------------

			case PIC_RECT_INVERT :		//509
				{
				frect	r;

				r = get_rect();
				offset_frect(&r, hh, vv);
				rect_invert(&r, a_view->pen_size, a_view->scale);
				}
				break;


//-------------------------------------------------------------
// set draw mode. 
//
// 4 bytes. PIC_SET_DRAW_MODE
// 4 bytes. draw mode
//-------------------------------------------------------------


			case PIC_SET_DRAW_MODE :		//50a
				{
				long	tmp_mode;		

				tmp_mode = get_long();
				a_view->draw_mode = tmp_mode;
				}
				break;


//-------------------------------------------------------------
// draw string. 
//
// 4 bytes. PIC_DRAW_STRING
// 4 bytes. string length
// x bytes. string data
//-------------------------------------------------------------


			case PIC_DRAW_STRING :		//50c
				{
					long        str_length;
					char        *str_ptr;
					port        *a_port;
					float       s_factor, dnsp, dsp;
					fc_context  *ctxt;
					
					str_length = get_long();
					str_ptr = (char*)gr_malloc(str_length+1);
					get_data(str_length, str_ptr);
					dnsp = get_float();
					dsp = get_float();

					ctxt = a_view->font_context;

					s_factor = a_view->scale;
					if (s_factor != 1.0)
						ctxt->set_escape_size(ctxt->escape_size() * s_factor);
					
					//xprintf("ds %f %f %f %f %s\n", hh,vv,a_view->pen_h,a_view->pen_v,str_ptr);
					//ctxt->set_origin((hh+a_view->pen_h) * s_factor, (vv+a_view->pen_v) * s_factor);
					ctxt->set_origin((a_view->pen_h) * s_factor, (a_view->pen_v) * s_factor);
	
					wait_regions_hinted(pid);
	
					a_port = get_port_pid(pid);
					if (a_port->last_user != pid)
						port_change(a_port, pid);	
					a_port->fore_color = a_view->fore_color;
					a_port->back_color = a_view->back_color;

					draw_string(a_port, ctxt, (uchar*)str_ptr, a_view->draw_mode, dnsp, dsp);

					if (s_factor != 1.0) {
						s_factor = 1.0/s_factor;
						ctxt->set_escape_size(ctxt->escape_size() * s_factor);
					}
					a_view->pen_h = ctxt->origin_x()*s_factor;
					a_view->pen_v = ctxt->origin_y()*s_factor;
	
					signal_regions();
		
					gr_free(str_ptr);
				}
				break;

//-------------------------------------------------------------
// set pen size. 
//
// 4 bytes. PIC_SET_PEN_SIZE
// 4 bytes. new pen size
//-------------------------------------------------------------

			case PIC_SET_PEN_SIZE :		//522
				{
				float	pensize;
			
				pensize = get_float();
				a_view->pen_size = pensize;
				}
				break;


//-------------------------------------------------------------
// ellipse frame. 
//
// 4 bytes. PIC_ELLIPSE_FRAME
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ELLIPSE_FRAME :	//524
				{
				frect	r;
				pattern	p;

				r = get_rect();
				offset_frect(&r, hh, vv);
				get_data(sizeof(pattern), &p);
				ellipse_frame_p(pid, &p, &r, a_view);
				}
				break;


//-------------------------------------------------------------
// ellipse fill. 
//
// 4 bytes. PIC_ELLIPSE_FILL
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ELLIPSE_FILL :	//533
				{
				frect	r;
				pattern	p;

				r = get_rect();
				offset_frect(&r, hh, vv);
				get_data(sizeof(pattern), &p);
				ellipse_fill_p(pid, &p, &r, a_view);
				break;
				}



//-------------------------------------------------------------
// arc frame. 
//
// 4 bytes. PIC_ARC_FILL
//16 bytes. absolute rectangle
// 4 bytes. Start Angle
// 4 bytes. End Angle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ARC_FRAME :		//53a
				{
				frect	r;
				pattern	p;
				long	start_angle;
				long	end_angle;

				r = get_rect();
				offset_frect(&r, hh, vv);
				start_angle = get_long();
				end_angle = get_long();
				get_data(sizeof(pattern), &p);
				arc_frame_p(pid, &p, &r, start_angle, end_angle, a_view);
				}
				break;


//-------------------------------------------------------------
// arc fill. 
//
// 4 bytes. PIC_ARC_FILL
//16 bytes. absolute rectangle
// 4 bytes. Start Angle
// 4 bytes. End Angle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ARC_FILL :		//53b
				{
				frect	r;
				pattern	p;
				long	start_angle;
				long	end_angle;

				r = get_rect();
				offset_frect(&r, hh, vv);
				start_angle = get_long();
				end_angle = get_long();
				get_data(sizeof(pattern), &p);
				arc_fill_p(pid, &p, &r, start_angle, end_angle, a_view);
				}
				break;


//-------------------------------------------------------------
// round rect frame.
//
// 4 bytes. PIC_ROUND_RECT_FRAME
//16 bytes. absolute rectangle
// 4 bytes. x radius
// 4 bytes. y radius
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ROUND_RECT_FRAME :	//53c
				{
				frect	r;
				pattern	p;
				float	xRadius;
				float	yRadius;

				r = get_rect();
				offset_frect(&r, hh, vv);
				xRadius = get_float();
				yRadius = get_float();
				get_data(sizeof(pattern), &p);
				round_rect_frame_p(	pid,
					 		    	&r,
									xRadius,
									yRadius,
									&p,
					     	 		a_view->draw_mode,
					     			a_view->pen_size,
					     			a_view->fore_color,
					     			a_view->back_color,
									a_view->scale);
				}
				break;


//-------------------------------------------------------------
// round rect fill.
//
// 4 bytes. PIC_ROUND_RECT_FILL
//16 bytes. absolute rectangle
// 4 bytes. x radius
// 4 bytes. y radius
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ROUND_RECT_FILL :		//53d
				{
				frect	r;
				pattern	p;
				float	xRadius;
				float	yRadius;

				r = get_rect();
				offset_frect(&r, hh, vv);
				xRadius = get_float();
				yRadius = get_float();
				get_data(sizeof(pattern), &p);
				round_rect_fill_p (	pid,
					 		    	&r,
									xRadius,
									yRadius,
									&p,
					     	 		a_view->draw_mode,
					     			a_view->pen_size,
					     			a_view->fore_color,
					     			a_view->back_color,
									a_view->scale);
				}
				break;

//-------------------------------------------------------------
// set foreground color. 
//
// 4 bytes. PIC_FORE_COLOR
// 4 bytes. rgb color
//-------------------------------------------------------------

			case PIC_FORE_COLOR :	//540
				{
				rgb_color	a_color;		

				get_data(sizeof(rgb_color), &a_color);
				a_view->fore_color = a_color;
				}
				break;

//-------------------------------------------------------------
// set background color. 
//
// 4 bytes. PIC_BACK_COLOR
// 4 bytes. rgb color
//-------------------------------------------------------------

			case PIC_BACK_COLOR :		//541
				{
				rgb_color	a_color;		

				get_data(sizeof(rgb_color), &a_color);
				a_view->back_color = a_color;
				}
				break;



//-------------------------------------------------------------
// Line to absolute position.
//
// 4 bytes. PIC_LINETO_PAT
// 4 bytes. x position
// 4 bytes. y position
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_LINETO_PAT :	//546 //done
				{
				float	tmp1;
				float	tmp2;
				pattern	p;

				tmp1 = hh+get_float();
				tmp2 = vv+get_float();
				get_data(sizeof(pattern), &p);
				//lineto_p(pid, tmp1, tmp2, a_view);
				lineto_p_pat(pid,
					 		 a_view->pen_h,
					 		 a_view->pen_v,
					 		 tmp1,
					 		 tmp2,
					 		 a_view->draw_mode,
					 		 &p,
					 		 a_view->pen_size,
					 		 a_view->fore_color,
					 		 a_view->back_color,
					 		 a_view->scale);
				a_view->pen_h = tmp1;
				a_view->pen_v = tmp2;
				}
				break;

//-------------------------------------------------------------
// set font family. 
//
// 4 bytes. PIC_SET_FONT_FAMILY
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_FAMILY :		//54b
			{
				long		string_length;
				char		*str_ptr;
				int         id;

				string_length = get_long();
				str_ptr = (char *)gr_malloc(string_length);
				get_data(string_length, str_ptr);
				
				id = fc_get_family_id(str_ptr);
				if (id < 0)
					id = fc_get_default_family_id();
				a_view->font_context->set_family_id(id);

				gr_free((char *)str_ptr);
			}
			break;


//-------------------------------------------------------------
// set font style. 
//
// 4 bytes. PIC_SET_FONT_STYLE
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_STYLE :		//54b
			{
				long		string_length;
				char		*str_ptr;
				int         id;
				
				string_length = get_long();
				str_ptr = (char *)gr_malloc(string_length);
				get_data(string_length, str_ptr);
				
				id = fc_get_style_id(a_view->font_context->family_id(), str_ptr);
				if (id < 0)
					id = fc_get_default_style_id(a_view->font_context->family_id());
				a_view->font_context->set_style_id(id);

				gr_free((char *)str_ptr);
			}
			break;


//-------------------------------------------------------------
// set font size. 
//
// 4 bytes. PIC_SET_FONT_SIZE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SIZE :
				a_view->font_context->set_escape_size(get_float());	
				break;


//-------------------------------------------------------------
// set font encoding. 
//
// 4 bytes. PIC_SET_FONT_ENCODING
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_ENCODING :
				a_view->font_context->set_encoding(get_long());	
				break;


//-------------------------------------------------------------
// set font shear. 
//
// 4 bytes. PIC_SET_FONT_SHEAR
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SHEAR :		//54d
				a_view->font_context->set_shear(get_float());	
				break;


//-------------------------------------------------------------
// set font rotate. 
//
// 4 bytes. PIC_SET_FONT_ROTATE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_ROTATE :			//54e
				a_view->font_context->set_rotation(get_float());	
				break;


//-------------------------------------------------------------
// set font spacing. 
//
// 4 bytes. PIC_SET_FONT_SPACING
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SPACING :			//54e
				a_view->font_context->set_spacing_mode(get_long());	
				break;


//-------------------------------------------------------------
// set font bits_per_pixel. 
//
// 4 bytes. PIC_SET_FONT_BPP
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_BPP :			//54e
				a_view->font_context->set_bits_per_pixel(get_long());	
				break;


//-------------------------------------------------------------
// set font flags. 
//
// 4 bytes. PIC_SET_FONT_FLAGS
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_FLAGS :			//54e
				a_view->font_context->set_flags(get_long());	
				break;


//-------------------------------------------------------------
// set font faces. 
//
// 4 bytes. PIC_SET_FONT_FACES
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_FACES :			//54e
				a_view->font_context->set_faces(get_long());	
				break;


//-------------------------------------------------------------
// moveby relative position.
//
// 4 bytes. PIC_MOVEBY
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_MOVEBY :			//54f	//done
				{
				a_view->pen_h += get_float();
				a_view->pen_v += get_float();
				}	
				break;

//-------------------------------------------------------------
// set scale. 
//
// 4 bytes. PIC_SET_SCALE
// 4 bytes. float scale
//-------------------------------------------------------------

			case PIC_SET_SCALE :		//573
				{
				float	scale;
			
				scale = get_float();
				a_view->scale = scale;
				}
				break;

//-------------------------------------------------------------
// rect frame. 
//
// 4 bytes. PIC_RECTFRAME_PAT
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_RECTFRAME_PAT :	
				{
				frect	r;
				pattern	p;

				r = get_rect();
				offset_frect(&r, hh, vv);
				get_data(sizeof(pattern), &p);
				rect_frame_pat_p(pid, &r, &p,
								 a_view->draw_mode,
								 a_view->pen_size,
								 a_view->fore_color,
								 a_view->back_color,
								 a_view->scale);
				}
				break;

//-------------------------------------------------------------
// rect frame. 
//
// 4 bytes. PIC_RECTFRAME
//16 bytes. absolute rectangle
//-------------------------------------------------------------

			case PIC_RECTFRAME :	
				{
				frect	r;
				pattern	p;

				r = get_rect();
				offset_frect(&r, hh, vv);
				rect_frame_p(pid, &r, a_view);
				}
				break;

//-------------------------------------------------------------
			
			case PIC_FILLPOLY :	
				{
				frect	r;
				pattern	p;
				long	num_pt;
				long	i;
				point	*pt_list;
	
				r = get_rect();
				offset_frect(&r, hh, vv);
				num_pt = get_long();
				pt_list = (point *)gr_malloc(num_pt * PTSIZE);
				get_data(PTSIZE * num_pt, pt_list);
				get_data(sizeof(pattern), &p);

				for (i = 0; i < num_pt; i++) {
					pt_list[i].h += hh;
					pt_list[i].v += vv;
				}

				poly_fill_p(pid,
					    	&r,
					    	num_pt,
					    	pt_list,
					    	&p,
							a_view);

				gr_free((char *)pt_list);
				}
				break;

			case PIC_FRAMEPOLY :	
				{
				frect	r;
				pattern	p;
				long	num_pt;
				long	i,closed;
				point	*pt_list;

				r = get_rect();
				get_data(sizeof(pattern), &p);
				offset_frect(&r, hh, vv);
				closed = get_long();
				num_pt = get_long();
				pt_list = (point *)gr_malloc(num_pt * PTSIZE);
				get_data(PTSIZE * num_pt, pt_list);

				for (i = 0; i < num_pt; i++) {
					pt_list[i].h += hh;
					pt_list[i].v += vv;
				}

				if (a_view->pen_size > 1.0)
					/* go in a heavy generc routine to generate all frame poly except if the pen is 1 or less */
					poly_frame_pen_p(pid,
									 &p,
									 &r,
									 closed,
									 num_pt,
									 pt_list,
									 a_view);
				else
					/* optimized case for pen 1 */
					poly_frame_p(pid,
								 &p,
								 &r,
								 closed,
								 num_pt,
								 pt_list,
								 a_view);
	
				gr_free((char *)pt_list);
				}
				break;

//-------------------------------------------------------------

			case PIC_BLIT :
			{
				frect	src_rect;
				frect	dst_rect;
				port	*a_port;			
				port	*to_port;			

				src_rect = get_rect();
				dst_rect = get_rect();
				a_port = pget_port();
		
				offset_frect(&dst_rect, hh, vv);
				to_port = get_port_pid(pid);
				
				blit(a_port,
				     to_port,
				     &src_rect,
				     &dst_rect,
				     a_view->draw_mode,
					 a_view->scale);
				done_with_port(a_port);
				break;
			}


//-------------------------------------------------------------

			case PIC_VARRAY :
			{
				long		num_pt, i;
				float		x0, y0, x1, y1;
				rgb_color	a_color;
				a_line		the_line;
				long		ps;

				ps = scale_pen(a_view->pen_size, a_view->scale);
				num_pt = get_long();
			
				if (ps <= 1)
					get_screen();
				for (i = 0; i < num_pt; i++) {
					get_data(sizeof(a_line), &the_line);
					x0 = the_line.x0;
					x1 = the_line.x1;
					y0 = the_line.y0;
					y1 = the_line.y1;
					mt_lt_p1(pid,
						 	 x0 + hh,
						 	 y0 + vv,
						   	 x1 + hh,
						     y1 + vv,
						 	 a_view->draw_mode,
						 	 a_view->pen_size,
						 	 the_line.col,
						 	 a_view->back_color,
						 	 a_view->scale);
				}
				if (ps <= 1)
					release_screen();
				break;
			}


//-------------------------------------------------------------

			case PIC_END_PICTURE :			//575
			{
				if (stack_pointer == 0)
					return;
				stack_pointer--;
				hh = stack_h[stack_pointer];
				vv = stack_v[stack_pointer];
				break;
			}

//-------------------------------------------------------------

			case PIC_SUB_PICTURE :
				break;

//-------------------------------------------------------------
			
			case PIC_OFFSET_FRAME :			//576
			{
				float	ddh, ddv;
		
			
				stack_h[stack_pointer] = hh;
				stack_v[stack_pointer] = vv;
				stack_pointer++;
				ddh = get_long();
				ddv = get_long();
				hh += ddh;
				vv += ddv;
			}
			break;
		}
	}
}

