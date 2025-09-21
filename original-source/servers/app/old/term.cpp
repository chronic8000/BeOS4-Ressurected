#if 0
#include <stdio.h> 
#include <OS.h>
#include <algobase.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif
#ifndef	MACRO_H
#include "macro.h"
#endif
#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef	RESOURCE_H
#include "resource.h"
#endif
#ifndef	WINDOW_H
#include "window.h"
#endif
#ifndef	SERVER_H
#include "server.h"
#endif
#ifndef	VIEW_H
#include "view.h"
#endif
#ifndef	SCROLL_BAR_H
#include "scroll_bar.h"
#endif
#ifndef	MENU_BAR_H
#include "menu_bar.h"
#endif
#ifndef	_MESSAGES_H
#include "messages.h"
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _TEXT_H
#include <text.h>
#endif

/*-------------------------------------------------------------*/

#include "term.h"

/*-------------------------------------------------------------*/

#define	INHERITED	TWindow
extern	TWindow		*console;

/*-------------------------------------------------------------*/

		TTermWindow::TTermWindow(char *name, rect *bound, long kind)
	:	TWindow(name, bound, kind, NO_GOAWAY)
{}

/*-------------------------------------------------------------*/

void	TTermWindow::handle_message(message *m)
{
	switch(m->what) {
		case (COUT) :
			the_server->user_action();
			the_term->term_write((char *)&(m->parm2), m->parm1, 0);
			break;
		
		case (TITLE) :
			the_term->get_owner()->set_name((char *)&(m->parm2));
			break;
		
		default :
			INHERITED::handle_message(m);
			break;
	}
}

/*-------------------------------------------------------------*/

char	TTermWindow::handle_key_down(message *m)
{
	write_port(key_port, 0, (char*)m, 64);
	return(1);
}

/*-------------------------------------------------------------*/

void	TTermWindow::do_close()
{
	bpdelete(key_port);
	TWindow::do_close();
}

/*-------------------------------------------------------------*/

	TTerm::TTerm(rect *r)	:	TView(r, 1, 0)
{
	long	i;

	xpos = 0;
	ypos = 0;
	sequence = 0;
	out_cnt = 0;
	sq_cnt = 0;
	c_attr = v_normal;

	for (i = 0; i < LNNUM; i++)
		sc_buf[i][0] = '\0';
}

/*-------------------------------------------------------------*/

TWindow *new_term()
{
	rect		r1;
	TTermWindow	*the_window;
	TTerm		*a_view;
	long		caller_id;

	r1.top = 30;
	r1.bottom = 450;
	r1.left = 30;
	r1.right = 560;
	the_window = new TTermWindow("Terminal", &r1, K_STD_WIND); 

	set_rect(r1, (r1.bottom - r1.top) - 350,
		     0,
		     r1.bottom - r1.top,
		     r1.right - r1.left);

	a_view = new TTerm(&r1);
	
	the_window->add_view(a_view);
	the_window->the_term = a_view;
	the_window->show();
	return(the_window);
}

/*-------------------------------------------------------------*/

void TTerm::draw()
{
	long	i;
	rect	r;
	rect	update_rect;
	long	vp;
	
	get_bound(&r);
	get_owner()->get_update_rect(&update_rect);
	update_rect.top -= 15;
	update_rect.bottom += 15;
	for (i=0 ; i<LNNUM ; i++) {
		vp = TXTYORG + (i * TXTHEIGHT) + TXTASCENT;
		if ((vp > update_rect.top) && (vp < update_rect.bottom))
			draw_string_screen_only(sc_buf[i],
									b_font_plain,
									op_copy,
									TXTXORG,
									(TXTYORG + i * TXTHEIGHT) + TXTASCENT,
									rgb_black,
									rgb_white);
	}
}

/*-------------------------------------------------------------*/

void	TTerm::scroll_text_rect(long left, long top, long right, long bot, long xD, long yD)
{
	rect	srcrect, dstrect;
	rect	r1, r2;
	region	*tmp_region1;
	region	*tmp_region2;
	region	*tmp_region3;
	
	xD *= TXTWIDTH;
	yD *= TXTHEIGHT;


	set_rect(srcrect, TXTYORG+top*TXTHEIGHT, TXTXORG+left*TXTWIDTH,
		TXTYORG+(bot+1)*TXTHEIGHT-1, TXTXORG+(right+1)*TXTWIDTH-1);
	dstrect.left = srcrect.left + xD;
	dstrect.top = srcrect.top + yD;
	dstrect.right = srcrect.right + xD;
	dstrect.bottom = srcrect.bottom + yD;

	if ((srcrect.top >= srcrect.bottom) || (srcrect.left >= srcrect.right))
		return;

	do_cursor();

	move_bits(&srcrect, &dstrect);
	

	if (yD > 0) {
		r1.bottom = dstrect.top - 1;
		r1.top = srcrect.top;
	}
	else 
		if (yD) {
			r1.top = dstrect.bottom + 1;
			r1.bottom = srcrect.bottom;
		}
		else {
			r2.top = srcrect.top;
			r2.bottom = srcrect.bottom;
		}
	if (xD < 0) {
		r2.left = dstrect.right + 1;
		r2.right = srcrect.right;
	}
	else
		if (xD) {
			r2.left = srcrect.left;
			r2.right = dstrect.left - 1;
		}
		else {
			r1.left = srcrect.left;
			r1.right = srcrect.right;
		}

	if (yD)
		rect_fill(&r1, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);
	if (xD)
		rect_fill(&r2, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);

	do_cursor();
}

/*-------------------------------------------------------------*/

void	TTerm::erase_text_rect(long left, long top, long right, long bot)
{
	rect	arect;

	do_cursor();

	set_rect(arect, TXTYORG+top*TXTHEIGHT, TXTXORG+left*TXTWIDTH,
		TXTYORG+(bot+1)*TXTHEIGHT, TXTXORG+(right+1)*TXTWIDTH-1);

	if ((arect.top < arect.bottom) && (arect.left < arect.right))
		rect_fill(&arect, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);

	do_cursor();
}

/*-------------------------------------------------------------*/

void	TTerm::invert_text_rect(long left, long top, long right, long bot)
{
	rect	arect;
	frect	tmp;

	do_cursor();

	set_rect(arect, TXTYORG+top*TXTHEIGHT, TXTXORG+left*TXTWIDTH,
		TXTYORG+(bot+1)*TXTHEIGHT, TXTXORG+(right+1)*TXTWIDTH-1);

	if ((arect.top < arect.bottom) && (arect.left < arect.right)) {
		tmp = rect_to_frect(arect);
		rect_invert(&tmp, 1, 1.0);
	}

	do_cursor();
}

/*-------------------------------------------------------------*/

void 	TTerm::do_cursor()
{
	long	old_op;
	long	y;
	long	x1;
	long	x2;

	
	x1 = TXTXORG + xpos * TXTWIDTH;
	x2 = TXTXORG + (xpos+1) * TXTWIDTH - 1;
	y  = TXTYORG + (ypos * TXTHEIGHT) + 9;
	lineto(x1, y, x2, y, op_xor, 1, rgb_black, rgb_white);
}

/*-------------------------------------------------------------*/

void	TTerm::lopen(long where, long num)
{
	long	i;
	
	num = min(num, LNNUM-where);
	for (i=LNNUM-1 ; i>=where+num ; i--)
		strcpy(sc_buf[i], sc_buf[i-num]);
	for (i=where ; i<where+num ; i++)
		sc_buf[i][0] = '\0';

	scroll_text_rect(0, where, CLNUM-1, LNNUM-1-num, 0, num);
}

/*-------------------------------------------------------------*/

void	TTerm::ldelete(long where, long num)
{
	long	i;

	num = min(num, LNNUM-where);
	for (i=where+num ; i<LNNUM ; i++)
		strcpy(sc_buf[i-num], sc_buf[i]);
	for (i=LNNUM-num ; i<LNNUM ; i++)
		sc_buf[i][0] = '\0';

	scroll_text_rect(0, where+num, CLNUM-1, LNNUM-1, 0, -num);
} 

/*-------------------------------------------------------------*/

void	TTerm::cinsert(long wx, long wy, long num)
{
	long	i, len;

	len = min((long) strlen(sc_buf[wy])+num, (long) CLNUM);
	num = min(num, CLNUM-wx);
	for (i=len-1 ; i>=wx+num ; i--)
		sc_buf[wy][i] = sc_buf[wy][i-num];
	sc_buf[wy][len] = '\0';
	for (i=wx ; i<wx+num ; i++)
		sc_buf[wy][i] = ' ';

	scroll_text_rect(wx, wy, CLNUM-num-1, wy, num, 0);
}

/*-------------------------------------------------------------*/

void	TTerm::cdelete(long wx, long wy, long num)
{
	long	i, len;

	len = strlen(sc_buf[wy]);
	num = min(num, CLNUM-wx);
	if (wx+num < len) {
		for (i=wx+num ; i<=len ; i++)
			sc_buf[wy][i-num] = sc_buf[wy][i];
		scroll_text_rect(wx+num, wy, len-1, wy, -num, 0);
	} else {
		sc_buf[wy][wx] = '\0';
		erase_text_rect(wx, wy, len-1, wy);
	}
}		

/*-------------------------------------------------------------*/

void	TTerm::eolerase()
{
	sc_buf[ypos][xpos] = '\0';
	erase_text_rect(xpos, ypos, CLNUM-1, ypos);
}

/*-------------------------------------------------------------*/

void	TTerm::eoserase()
{
	long	i;

	eolerase();
	for (i=ypos+1 ; i<LNNUM ; i++)
		sc_buf[i][0] = '\0';
	erase_text_rect(0, ypos+1, CLNUM-1, LNNUM-1);
}

/*-------------------------------------------------------------*/

void	TTerm::cseek(long v, long h)
{
	if (v<0)
		v=0;
	if (h<0)
		h=0;

	do_cursor();
	xpos = h;
	ypos = v;
	do_cursor();
}

/*-------------------------------------------------------------*/

void	TTerm::set_attr(char a)
{
	c_attr = a;
}

/*-------------------------------------------------------------*/

void	TTerm::drawtext(char *out_buf, long cnt, fc_context *ctxt, long h, long v)
{
	char	buffer[100];
	long	i;

	if (cnt > 100)
		cnt = 100;

	for (i = 0; i < cnt; i++) 
		buffer[i] = *out_buf++;
	buffer[cnt] = 0x00;

	draw_string_screen_only(buffer,
							ctxt,
							op_copy,
							h,
							v,
							rgb_black,
							rgb_white); 
}

/*-------------------------------------------------------------*/

void	TTerm::flush_out_buf()
{
	long	i, len;
	char	*src;
	char	*dst;
	long	h;
	long	v;


	do_cursor();
	h = TXTXORG + xpos * TXTWIDTH;
	v = (TXTYORG + ypos * TXTHEIGHT)+TXTASCENT;
	
	if (c_attr != v_normal) {
		drawtext(out_buf, out_cnt, b_font_plain, h, v);
		h = TXTXORG + 1 + xpos * TXTWIDTH;
		v = (TXTYORG + ypos * TXTHEIGHT)+TXTASCENT;
		drawtext(out_buf, out_cnt, b_font_plain, h, v);
	}
	else {
		drawtext(out_buf, out_cnt, b_font_plain, h, v);
	}

	len = strlen(sc_buf[ypos]);
	dst = &sc_buf[ypos][len];
	for (i=len ; i<xpos ; i++)
		*dst++ = ' ';
		
	src = out_buf;
	dst = &sc_buf[ypos][xpos];
	for (i=0 ; i<out_cnt ; i++)
		*dst++ = *src++;
	xpos += out_cnt;
	if (xpos > len)
		*dst = '\0';
	out_cnt = 0;

	do_cursor();
}

/*-------------------------------------------------------------*/

void	TTerm::outchar(char c)
{
	char	prec;
	rect	r;

	switch (c) {
	case CR:
		flush_out_buf();
		cseek(ypos, 0);
		break;
	case LF:
		flush_out_buf();
		do_cursor();
		xpos = 0;
		ypos++;
		if (ypos >= LNNUM) {
			ldelete(0, 3);
			ypos = LNNUM-3;
		}
		do_cursor();
		break;
	case BS:
		flush_out_buf();
		do_cursor();
		xpos--;
		if (xpos < 0) {
			xpos = CLNUM-1;
			ypos--;
			if (ypos < 0)
				ypos = 0;
		}
		r.top = TXTYORG + ypos * TXTHEIGHT;
		r.bottom = r.top + (TXTHEIGHT - 1);
		r.left = TXTXORG + xpos * TXTWIDTH;
		r.right = r.right + 7;
		rect_fill(&r, get_pat(WHITE), op_copy, 1, rgb_black, rgb_white);

		do_cursor();
		break;
	default:
		if (c == '\007') {
			break;
		}
		out_buf[out_cnt++] = c;
		if (xpos+out_cnt >= CLNUM) {
			flush_out_buf();
			do_cursor();
			xpos = 0;
			ypos++;
			do_cursor();
		}
		if (ypos >= LNNUM) {
			ldelete(0, 3);
			do_cursor();
			ypos = LNNUM - 3;
			do_cursor();
		}
		if (out_cnt == OUTSIZE)
			flush_out_buf();
		break;
	}
}

/*-------------------------------------------------------------*/

long	TTerm::term_write(char *buf, long len, long pos)
{
	char	c;
	long	count;

	set();
	count = len;
	while (count--)
		if (((c = *buf++) == '\033') && !sequence) {
			flush_out_buf();
			sequence = 1;
			sq_cnt = 0;
		} else
			if (!sequence)
				outchar(c);
			else {
				sq_buf[sq_cnt++] = c;
				if (sq_cnt == SQSIZE)
					process_sq();
			}
	flush_out_buf();
	return (len);
}

/*-------------------------------------------------------------*/

void	TTerm::process_sq()
{

	sequence = 0;

	switch(sq_buf[0]) {
		case 'H':
			cseek(sq_buf[1],sq_buf[2]);
			break;
		case 'Q':
			cseek(sq_buf[1]-1,sq_buf[2]-1);
			break;

		case 'L':
			lopen(ypos, sq_buf[1]);
			break;
		case 'M':
			ldelete(ypos, sq_buf[1]);
			break;
		case 'J' :
			eoserase();
			break;
		case 'K':
			eolerase();
			break;
		case '@':
			cinsert(xpos, ypos, sq_buf[1]);
			break;
		case 'P':
			cdelete(xpos, ypos, sq_buf[1]);
			break;
		case 'n':
		case 'z':
		case 'e':
			set_attr(sq_buf[0]);
			break;
		default:
			break;
		}
}
#endif
