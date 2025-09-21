//******************************************************************************
//
//	File:		mwindow.cpp
//
//	Description:	client server windows views etc... communication
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	may 94		update to new event model	BGS
//******************************************************************************/
 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#undef DEBUG
#define	DEBUG 0
#include <Debug.h>

#include <OS.h>
#include <Errors.h>
#include <shared_defs.h>

#include "as_debug.h"

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif
#ifndef	MACRO_H
#include "macro.h"
#endif
#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef	BEZIER_H
#include "bezier.h"
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
#ifndef	RENDER_H
#include "render.h"
#endif
#ifndef	SCROLL_BAR_H
#include "scroll_bar.h"
#endif
#ifndef	_MESSAGES_H
#include "messages.h"
#endif
#ifndef NEWPICTURE_H
#include "newPicture.h"
#endif
#ifndef	BITMAP_H
#include "bitmap.h"
#endif


#include <direct_window_priv.h>
#include "debug_update.h"
#include "rect.h"
#include "off.h"
#include "cursor.h"
#include "./token.h"

#undef assert
//#define assert(condition)	my_assert(condition)
#define assert(condition)
#define my_assert(condition) ((condition) ? ((void) 0) : debugger(0))
#define as_long(v) *((long *)&v)

extern	HashTable 		serverHeapHash;
extern 	"C" long		cur_switch;
extern	TWindow 		*front_limit;
extern	TWindow 		*last_top_window;


/*-------------------------------------------------------------*/

#include "app.h"
#include "path.h"
#include "input.h"
#include "eventport.h"
#include "dwindow.h"
#include "mwindow.h"

/*-------------------------------------------------------------*/
 
inline point npoint(int32 h, int32 v) { point p; p.h = h; p.v = v; return p; };
inline fpoint nfpoint(float h, float v) { fpoint p; p.h = h; p.v = v; return p; };

/*-------------------------------------------------------------*/

void MWindow::gr_pick_view()
{
	int32	token,result;
	short	a_type;
	TView	*a_view;
	
	a_session->sread(4,&token);

	if (token == last_view_token) return;

	if (last_view) grGiveUpLocks(last_view->renderContext);

	if (token < 0) {
		last_view = NULL;
		last_view_token = -1;
		return;
	}
	
	result = tokens->get_token(token, &a_type, (void **)&a_view);
	if ((result < 0) || (a_type != TT_VIEW)) {
		DebugPrintf(("bad view token %d\n",token));
		last_view = NULL;
		last_view_token = -1;
		return;
	}

	last_view_token = token;
	last_view = a_view;
}

/*-------------------------------------------------------------*/
pattern	_black	=
{
	{
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff
	}
};

/*-------------------------------------------------------------*/

MWindow::MWindow(
	SApp *application,
	char *name,
	rect *bound,
	uint32 design,
	uint32 behavior,
	ulong flags,
	ulong ws,
	int32 id,
	port_id eventPort,
	int32 clientToken,
	TSession *session)
	: TWindow(application, name, bound, design, behavior, flags, ws, id,
			  eventPort, clientToken, session)
{
	last_view = NULL;
	last_view_token = -1;
}

/*-------------------------------------------------------------*/

MWindow::MWindow(SApp *app) : TWindow(app)
{
}

/*-------------------------------------------------------------*/

void MWindow::gr_open_view_transaction()
{
	atomic_add(&m_updateDisabledCount,1);
	atomic_add(&m_viewTransactionCount,1);
};

void MWindow::gr_close_view_transaction()
{
	int32 result = atomic_add(&m_viewTransactionCount,-1);

	if (result < 1) {
		atomic_add(&m_viewTransactionCount,1);
		_sPrintf("Mismatched CloseViewTransaction()!\n");
		return;
	};

	if (result == 1) {
		region *newInval = newregion();
		LockRegionsForWrite();
		top_view->ViewTransaction(newInval,NULL,true);
		Invalidate(newInval);
		UnlockRegionsForWrite();
		kill_region(newInval);
	};

	if (atomic_add(&m_updateDisabledCount,-1) < 1) {
		atomic_add(&m_updateDisabledCount,1);
		_sPrintf("Mismatched CloseViewTransaction() (someone did an EnableUpdates()!\n");
		return;
	};

	// REVISIT: This fake mouse moved is extremely bad.
	// find a way to remove it when not needed
	// We could send it only if the mouse is actually in this window
	the_server->force_mouse_moved();
};

/*-------------------------------------------------------------*/

void MWindow::update_off()
{
	atomic_add(&m_updateDisabledCount,1);
}

void MWindow::update_on()
{
	if (atomic_add(&m_updateDisabledCount,-1) < 1) {
		atomic_add(&m_updateDisabledCount,1);
		_sPrintf("Mismatched EnableUpdates()!\n");
		return;
	};
}

/*-------------------------------------------------------------*/

void MWindow::screen_changed(long h_size, long v_size, long server_color_map)
{
	int32		delta_h, delta_v;
	frect 		r;
	BParcel		an_event(B_SCREEN_CHANGED);

	if (ClientRegion()->Bounds().left > (h_size-8))
		delta_h = (h_size-16)-ClientRegion()->Bounds().left;
	else
		delta_h = 0;
	if (ClientRegion()->Bounds().top > (v_size-8))
		delta_v = (v_size-16)-ClientRegion()->Bounds().top;
	else
		delta_v = 0;
	if ((delta_h != 0) || (delta_v != 0)) {
		move(delta_h, delta_v, 1);
/*
		if (the_server->m_dragWindow == this) {
			the_server->m_dragPt.h += delta_h;
			the_server->m_dragPt.v += delta_v;
  		}
*/
  	}
		
	set_rect(r, 0, 0, v_size-1, h_size-1);

	an_event.AddRect("frame",r);
	an_event.AddInt32("mode",server_color_map);
	an_event.AddInt64("when",system_time());
	m_eventPort->SendEvent(&an_event);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_pen_loc()
{
	fpoint		penLoc;

	if (last_view)
		grGetPenLocation(last_view->renderContext,&penLoc);
	else {
		penLoc.h = 0.0;
		penLoc.v = 0.0;
	}

	a_session->swrite(sizeof(penLoc), &penLoc);
	a_session->sync();
}

/*-------------------------------------------------------------*/
//done

void	MWindow::gr_moveto()
{
	fpoint	penLoc;
	a_session->sread(sizeof(fpoint), &penLoc);

	if (!last_view) return;
	grSetPenLocation(last_view->renderContext,penLoc);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_no_clip()
{
	if (!last_view) return;
	grClearClip(last_view->renderContext);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_start_picture()
{
	if (!last_view) return;
	grStartRecording(last_view->renderContext);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_restart_picture()
{
	int32	result, pictureToken;
	SPicture *pic;

	a_session->sread(4,&pictureToken);
	if (!last_view) return;

	result = tokens->grab_atom(pictureToken,(SAtom**)&pic);

	if (result >= 0) {
		SPicture *newPic = new SPicture();
		newPic->Restart(pic);
		grContinueRecording(last_view->renderContext,newPic);
		tokens->delete_atom(pictureToken, pic);
	};
}

/*-------------------------------------------------------------*/

void	MWindow::gr_play_picture()
{
	int32		picToken,result;
	fpoint		playAt;
	SPicture *	pic;

	a_session->sread(4,&picToken);
	a_session->sread(sizeof(fpoint),&playAt);

	if (!last_view) return;

	result = tokens->grab_atom(picToken,(SAtom**)&pic);

	if (result >= 0) {
		grPlayPicture(last_view->renderContext,pic,playAt);
		pic->ServerUnlock();
	} else {
		_sPrintf("picture not here!\n");
	};
}

/*-------------------------------------------------------------*/

void MWindow::gr_clip_to_picture()
{
	int32		picToken,inverse,result;
	fpoint		offset;
	SPicture *	pic;

	a_session->sread(4,&picToken);
	a_session->sread(sizeof(fpoint),&offset);
	a_session->sread(4,&inverse);

	if (!last_view) return;

	result = tokens->grab_atom(picToken,(SAtom**)&pic);

	if (result >= 0) {
		grClipToPicture(last_view->renderContext,pic,offset,inverse);
		pic->ServerUnlock();
	} else {
		DebugPrintf(("picture not here!\n"));
	};
};

/*-------------------------------------------------------------*/

void	MWindow::gr_end_picture()
{
	int32	picToken=0;

	if (last_view) {
		SPicture *p = grStopRecording(last_view->renderContext);
		if (p) {
			picToken = tokens->new_token(team_id,TT_PICTURE|TT_ATOM,p);
			p->ClientLock();
			p->SetToken(picToken);
		} else {
			DebugPrintf(("Wasn't recording?\n"));
		};
	};
	
	a_session->swrite_l(picToken);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_userclip()
{
	region	*a_region;

	a_region = newregion();
	a_session->read_region(a_region);

	if (!last_view) {
		kill_region(a_region);
		return;
	}

	grClipToRegion(last_view->renderContext,a_region);
	kill_region(a_region);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_pattern()
{
	pattern p;	
	a_session->sread(8,&p);
	
	if (!last_view) return;
	grSetStipplePattern(last_view->renderContext,&p);
};

/*-------------------------------------------------------------*/

void	MWindow::gr_moveby()
{
	fpoint	penLocDelta;

	a_session->sread(sizeof(fpoint), &penLocDelta);

	if (!last_view) return;
	grMovePenLocation(last_view->renderContext,penLocDelta);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_line()
{
	fpoint	fromPoint,toPoint;

	a_session->sread(sizeof(fpoint), &fromPoint);
	a_session->sread(sizeof(fpoint), &toPoint);

	if (!last_view) return;

	grStrokeLine(last_view->renderContext,fromPoint,toPoint);
	grSetPenLocation(last_view->renderContext,toPoint);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_lineto()
{
	fpoint	fromPoint,toPoint;

	a_session->sread(sizeof(fpoint), &toPoint);

	if (!last_view) return;

	grGetPenLocation(last_view->renderContext,&fromPoint);
	grStrokeLine(last_view->renderContext,fromPoint,toPoint);
	grSetPenLocation(last_view->renderContext,toPoint);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_rect_invert()
{
	frect	fr;
	int16	oldOp;
	
	a_session->sread(sizeof(fr), &fr);
	
	if (!last_view) return;
	
	grGetDrawOp(last_view->renderContext,&oldOp);
	grSetDrawOp(last_view->renderContext,OP_INVERT);
	grFillRect(last_view->renderContext,fr);
	grSetDrawOp(last_view->renderContext,oldOp);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_polyframe()
{
	fpoint		verticesStack[8];		
	fpoint *	vertices=verticesStack;
	short		numPts,closed;

	a_session->sread(2, &closed);
	a_session->sread(2, &numPts);

	if (numPts < 0) return;
	if (numPts > 8) {
		vertices = (fpoint *)grMalloc(numPts * sizeof(fpoint), "gr_polyframe tmp buf",MALLOC_MEDIUM);
		if (!vertices) {
			a_session->drain(sizeof(fpoint)*numPts);
			return;
		};
	};
	
	if ((a_session->sread(sizeof(fpoint)*numPts,vertices,false) == B_OK) && last_view)
		grStrokePolygon(last_view->renderContext,vertices,numPts,closed);
	
	if (vertices != verticesStack) grFree(vertices);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_polyfill()
{
	short		numPts;
	fpoint		verticesStack[8];
	fpoint *	vertices = verticesStack;
	
	a_session->sread(2, &numPts);
	
	if (numPts < 0) return;
	if (numPts > 8) {
		vertices = (fpoint *)grMalloc(numPts * sizeof(fpoint), "gr_polyfill tmp buf",MALLOC_MEDIUM);
		if (!vertices) {
			a_session->drain(sizeof(fpoint)*numPts);
			return;
		};
	};

	if ((a_session->sread(sizeof(fpoint)*numPts,vertices,false) == B_OK) && last_view)
		grFillPolygon(last_view->renderContext,vertices,numPts);

	if (vertices != verticesStack) grFree(vertices);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_fill_region()
{
	region	*a_region = newregion();
	a_session->read_region(a_region);
	
	if (last_view) {
		const int32 N = a_region->CountRects();
		const rect* r = a_region->Rects();
		for (int32 i=0;i<N;i++) {
			grFillIRect(last_view->renderContext,r[i]);
		};
	};

	kill_region(a_region);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_arc(long msg)
{
	float	start_angle,arc_angle;
	union { frect r; struct { fpoint center,radius; } p; } u;
	
	a_session->sread(16, &u);
	a_session->sread(4, &start_angle);
	a_session->sread(4, &arc_angle);

	if (!last_view) return;

	switch (msg) {
		case GR_ARC_STROKE:
			grStrokeArc(last_view->renderContext,u.p.center,u.p.radius,start_angle,arc_angle);
			break;
		case GR_ARC_FILL:
			grFillArc(last_view->renderContext,u.p.center,u.p.radius,start_angle,arc_angle);
			break;
		case GR_ARC_INSCRIBE_STROKE:
			grInscribeStrokeArc(last_view->renderContext,u.r,start_angle,arc_angle);
			break;
		case GR_ARC_INSCRIBE_FILL:
			grInscribeFillArc(last_view->renderContext,u.r,start_angle,arc_angle);
			break;
	};
}

void	MWindow::gr_ellipse(long msg)
{
	union { frect r; struct { fpoint center,radius; } p; } u;
	
	a_session->sread(16, &u);
	
	if (!last_view) return;

	switch (msg) {
		case GR_ELLIPSE_STROKE:
			grStrokeEllipse(last_view->renderContext,u.p.center,u.p.radius);
			break;
		case GR_ELLIPSE_FILL:
			grFillEllipse(last_view->renderContext,u.p.center,u.p.radius);
			break;
		case GR_ELLIPSE_INSCRIBE_STROKE:
			grInscribeStrokeEllipse(last_view->renderContext,u.r);
			break;
		case GR_ELLIPSE_INSCRIBE_FILL:
			grInscribeFillEllipse(last_view->renderContext,u.r);
			break;
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_round_rect_frame()
{
	frect	fr;
	fpoint	radius;
	
	a_session->sread(sizeof(frect), &fr);
	a_session->sread(sizeof(float), &radius.h);
	a_session->sread(sizeof(float), &radius.v);

	if (!last_view) return;

	grStrokeRoundRect(last_view->renderContext,fr,radius);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_round_rect_fill()
{
	frect	fr;
	fpoint	radius;
	
	a_session->sread(sizeof(frect), &fr);
	a_session->sread(sizeof(float), &radius.h);
	a_session->sread(sizeof(float), &radius.v);
	
	if (!last_view) return;

	grFillRoundRect(last_view->renderContext,fr,radius);
}

/*-------------------------------------------------------------*/

void	MWindow::get_pointer()
{
	bool	is_area;
	long	data;

	is_area = false;
	data = 0;
	a_session->swrite(sizeof(bool), &is_area);
	a_session->swrite_l(data);
	a_session->swrite_l(0);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_draw_bitmap(char async)
{
	fpoint		location;
	int32		bitmap_token, result;
	SAtom *		atom;
	SBitmap *	bitmap;
	frect		src_rect, dst_rect;
	
	a_session->sread(sizeof(fpoint),&location);
	a_session->sread(4, &bitmap_token);

	if (last_view) {
		result = tokens->grab_atom(bitmap_token, &atom);
		if (result == 0) {
			bitmap = dynamic_cast<SBitmap*>(atom);
			if (!bitmap) {
				TOff *off = dynamic_cast<TOff*>(atom);
				if (!off) {
					atom->ServerUnlock();
					goto done;
				};
				bitmap = off->Bitmap();
				bitmap->ServerLock();
				off->ServerUnlock();
			};
		
			src_rect.top = src_rect.left = 0;
#if ROTATE_DISPLAY
			if (bitmap->Canvas()->pixels.pixelIsRotated) {
				src_rect.bottom = bitmap->Canvas()->pixels.w-1;
				src_rect.right = bitmap->Canvas()->pixels.h-1;
			} else
#endif
			{
				src_rect.bottom = bitmap->Canvas()->pixels.h-1;
				src_rect.right = bitmap->Canvas()->pixels.w-1;
			}

			dst_rect = src_rect;
			dst_rect.top = src_rect.top + location.v;
			dst_rect.left = src_rect.left + location.h;
			dst_rect.bottom = src_rect.bottom + location.v;
			dst_rect.right = src_rect.right + location.h;
		
			/*	This grLock around the bitmap lock prevents a deadlock.
				region_sem must always be taken before local area locks,
				halleluah amen. */
			bitmap->PreloadPixels(src_rect);
			grLock(last_view->renderContext);
				bitmap->LockPixels();
					grDrawPixels(last_view->renderContext,src_rect,dst_rect,&bitmap->Canvas()->pixels);
				bitmap->UnlockPixels();
			grUnlock(last_view->renderContext);
			
			bitmap->ServerUnlock();
		};
	};

	done:
	
	if (async == 0) {
		a_session->swrite_l(0);	/* we need sync */
		a_session->sync();
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_move_bits()
{
	rect	r1,r2;	
	frect	fr1,fr2;	

	a_session->sread(sizeof(fr1), &fr1);
	a_session->sread(sizeof(fr2), &fr2);
	
	r1 = scale_rect(fr1,1.0);
	r2 = scale_rect(fr2,1.0);

	if (last_view) {
		LockRegionsForWrite();
		last_view->move_bits(&r1, &r2);
		UnlockRegionsForWrite();
	};
	
	a_session->swrite_l(0);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_scale_bitmap1(char async)
{
	int32		bitmap_token;
	int32		result;
	SAtom *		atom;
	SBitmap *	bitmap;
	frect		src_rect, dst_rect;
	
	a_session->sread(sizeof(src_rect), &src_rect);
	a_session->sread(sizeof(dst_rect), &dst_rect);
	a_session->sread(4, &bitmap_token);
	
	if (last_view) {
		result = tokens->grab_atom(bitmap_token, &atom);
		if (result == 0) {
			bitmap = dynamic_cast<SBitmap*>(atom);
			if (!bitmap) {
				TOff *off = dynamic_cast<TOff*>(atom);
				if (!off) {
					atom->ServerUnlock();
					goto done;
				};
				bitmap = off->Bitmap();
				bitmap->ServerLock();
				off->ServerUnlock();
			};

#if ROTATE_DISPLAY
			if (bitmap->Canvas()->pixels.pixelIsRotated) {
				frect old = src_rect;
				src_rect.left = old.top;
				src_rect.top = old.left;
				src_rect.right = old.bottom;
				src_rect.bottom = old.right;
			}
#endif
			bitmap->PreloadPixels(src_rect);
			grLock(last_view->renderContext);
				bitmap->LockPixels();
					grDrawPixels(last_view->renderContext,src_rect, dst_rect,&bitmap->Canvas()->pixels);
				bitmap->UnlockPixels();
			grUnlock(last_view->renderContext);
		
			bitmap->ServerUnlock();
		}
	}
	
done:
	if (async == 0) {
		a_session->swrite_l(0);	/* we need sync */
		a_session->sync();
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_scale_bitmap(char async)
{
	int32		bitmap_token;
	int32		result;
	SAtom *		atom;
	SBitmap *	bitmap;
	frect		src_rect, dst_rect;
	
	a_session->sread(sizeof(dst_rect), &dst_rect);
	a_session->sread(4, &bitmap_token);

	if (last_view) {
		result = tokens->grab_atom(bitmap_token, &atom);
		if (result == 0) {
			bitmap = dynamic_cast<SBitmap*>(atom);
			if (!bitmap) {
				TOff *off = dynamic_cast<TOff*>(atom);
				if (!off) {
					atom->ServerUnlock();
					goto done;
				};
				bitmap = off->Bitmap();
				bitmap->ServerLock();
				off->ServerUnlock();
			};

			src_rect.top = src_rect.left = 0;
#if ROTATE_DISPLAY
			if (bitmap->Canvas()->pixels.pixelIsRotated && !bitmap->Canvas()->pixels.isCompressed) {
				src_rect.bottom = bitmap->Canvas()->pixels.w-1;
				src_rect.right = bitmap->Canvas()->pixels.h-1;
			} else
#endif
			{
				src_rect.bottom = bitmap->Canvas()->pixels.h-1;
				src_rect.right = bitmap->Canvas()->pixels.w-1;
			}

			bitmap->PreloadPixels(src_rect);
			grLock(last_view->renderContext);
				bitmap->LockPixels();
					grDrawPixels(last_view->renderContext,src_rect, dst_rect,&bitmap->Canvas()->pixels);
				bitmap->UnlockPixels();
			grUnlock(last_view->renderContext);
		
			bitmap->ServerUnlock();
		}
	}
	
done:	
	if (async == 0) {
		a_session->swrite_l(0);	/* we need sync */
		a_session->sync();
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_rectframe()
{
	frect	fr;
	a_session->sread(sizeof(fr), &fr);
	
	if (!last_view) return;
	
	grStrokeRect(last_view->renderContext,fr);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_rectfill()
{
	frect	fr;	
	a_session->sread(sizeof(fr), &fr);

	if (!last_view) return;

	grFillRect(last_view->renderContext,fr);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_inval()
{
	rect r;
	a_session->sread(sizeof(r), &r);

	if (!last_view) return;
	
	region *reg = newregion();
	set_region(reg,&r);
	InvalInView(last_view,reg);
	kill_region(reg);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_inval_region()
{
	region *a_region = newregion();
	a_session->read_region(a_region);

	if (last_view) InvalInView(last_view,a_region);
	kill_region(a_region);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_bezier(long msg)
{
	fpoint controlPoints[4];
	a_session->sread(sizeof(fpoint)*4, controlPoints);
	
	if (!last_view) return;

	if (msg==GR_FILL_BEZIER)
		grFillBezier(last_view->renderContext,controlPoints);
	else
		grStrokeBezier(last_view->renderContext,controlPoints);
}

/*-------------------------------------------------------------*/

void MWindow::gr_shape(int32 msg)
{
	int32 opCount,ptCount;

	a_session->sread(4,&opCount);
	a_session->sread(4,&ptCount);

	if ((opCount <= 0) || (opCount > 65536) ||
		(ptCount <= 0) || (ptCount > 16*65536)) return;

	SPath *path = new SPath(last_view?last_view->renderContext:NULL,opCount,ptCount);
	path->Ops()->SetItems(opCount);
	path->Points()->SetItems(ptCount);

	if ((a_session->sread(sizeof(uint32)*opCount,path->Ops()->Items(),false) == B_OK) &&
		(a_session->sread(sizeof(fpoint)*ptCount,path->Points()->Items(),false) == B_OK) &&
		last_view) {
		switch (msg) {
			case GR_STROKE_SHAPE:	grStrokePath(last_view->renderContext,path);	break;
			case GR_FILL_SHAPE:		grFillPath(last_view->renderContext,path);		break;
		};
	};

	delete path;
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_pen_size()
{
	float	size;
	a_session->sread(sizeof(float), &size);

	if (!last_view) return;

	grSetPenSize(last_view->renderContext,size);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_scale()
{
	float	a_scale;	
	a_session->sread(sizeof(float), &a_scale);

	if (!last_view) return;

	grSetScale(last_view->renderContext,a_scale);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_origin()
{
	fpoint	origin;
	a_session->sread(sizeof(fpoint),&origin);

	if (!last_view) return;
		
	grSetOrigin(last_view->renderContext,origin);
}

void	MWindow::gr_get_origin()
{
	fpoint	origin;

	if (last_view)
		grGetOrigin(last_view->renderContext,&origin);
	else {
		origin.h = 0.0;
		origin.v = 0.0;
	}
	
	a_session->swrite(sizeof(fpoint),&origin);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_push_state()
{
	if (!last_view) return;
	grPushState(last_view->renderContext);
}

void	MWindow::gr_pop_state()
{
	if (!last_view) return;
	grPopState(last_view->renderContext);
}

/*-------------------------------------------------------------*/

void MWindow::gr_set_line_mode()
{
	short	modes[2];
	float	miterLimit;
	a_session->sread(sizeof(short)*2, modes);
	a_session->sread(sizeof(float), &miterLimit);

	if (!last_view) return;

	grSetLineMode(last_view->renderContext,modes[0],modes[1],miterLimit);
};

/*-------------------------------------------------------------*/

void MWindow::gr_get_line_mode(long msg)
{
	short	capRule,joinRule;
	float	miterLimit;
	void *	data;
	int32 	len;

	if (last_view)
		grGetLineMode(last_view->renderContext,&capRule,&joinRule,&miterLimit);
	else {
		capRule = 0;
		joinRule = 0;
		miterLimit = 0.0;
	}
	
	data = NULL;
	if (msg == GR_GET_CAP_MODE) {
		data = &capRule;
		len = sizeof(short);
	} else if (msg == GR_GET_JOIN_MODE) {
		data = &joinRule;
		len = sizeof(short);
	} else if (msg == GR_GET_MITER_LIMIT) {
		data = &miterLimit;
		len = sizeof(float);
	} else {
		DebugPrintf(("Protocol failed\n"));
		return;
	};

	if (data)
		a_session->swrite(len,data);
};

/*-------------------------------------------------------------*/

void	MWindow::gr_get_pen_size()
{
	float	size;
	
	if (last_view)
		grGetPenSize(last_view->renderContext,&size);
	else
		size = 1.0;

	a_session->swrite(sizeof(size), &size);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_fore_color()
{
	rgb_color	foreColor;
	a_session->sread(4, &foreColor);

	if (!last_view) return;

	grSetForeColor(last_view->renderContext,foreColor);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_font_flags()
{
	uint32 flags;
	a_session->sread(sizeof(flags), &flags);
	if (last_view) grForceFontFlags(last_view->renderContext, flags);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_view_bitmap()
{
	SAtom *atom;
	int32 result;
	bool returnColor=false;
	rgb_color key;
	SBitmap *bitmap;
	struct {	int32 token;
				frect srcRect, dstRect;
				uint32 followFlags, options; } protocol;

	if (!last_view) return;
	a_session->sread(sizeof(protocol),&protocol);
	
	result = tokens->grab_atom(protocol.token, &atom);
	if (result >= 0) {
		bitmap = dynamic_cast<SBitmap*>(atom);
		if (!bitmap) {
			TOff *off = dynamic_cast<TOff*>(atom);
			bitmap = off->Bitmap();
			bitmap->ServerLock();
			off->ServerUnlock();
		};

		LockRegionsForWrite();

		result = last_view->SetViewBitmap(
			bitmap,
			protocol.srcRect,protocol.dstRect,
			protocol.followFlags, protocol.options);

		if (protocol.options & AS_OVERLAY_BITMAP) {
			if (result == B_OK) key = bitmap->OverlayFor()->ColorKey();
			returnColor = true;
		};

		UnlockRegionsForWrite();

		bitmap->ServerUnlock();
	} else {
		/*	The bitmap token is bad.  That's okay... it's our
			cue to remove the view's background bitmap. */
		LockRegionsForWrite();
		last_view->ClearViewBitmap();
		UnlockRegionsForWrite();
	};
	a_session->swrite(4, &result);
	if (returnColor) a_session->swrite(sizeof(rgb_color), &key);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_view_color()
{
	rgb_color	a_color;
	a_session->sread(4, &a_color);

	if (last_view) last_view->view_color = a_color;
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_fore_color()
{
	rgb_color	foreColor;
	
	if (last_view)
		grGetForeColor(last_view->renderContext,&foreColor);
	else {
		foreColor.blue = 0;
		foreColor.green = 0;
		foreColor.red = 0;
		foreColor.alpha = 0;
	}
	
	a_session->swrite(sizeof(foreColor), &foreColor);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_back_color()
{
	rgb_color backColor;
	a_session->sread(4, &backColor);

	if (!last_view) return;

	grSetBackColor(last_view->renderContext,backColor);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_back_color()
{
	rgb_color backColor;
	
	if (last_view)
		grGetBackColor(last_view->renderContext,&backColor);
	else {
		backColor.blue = 0;
		backColor.green = 0;
		backColor.red = 0;
		backColor.alpha = 0;
	}
	
	a_session->swrite(sizeof(backColor), &backColor);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void MWindow::gr_set_blending_mode()
{
	int16 srcAlphaSrc,alphaFunction;
	a_session->sread(2, &srcAlphaSrc);
	a_session->sread(2, &alphaFunction);

	if (!last_view) return;

	grSetBlendingMode(last_view->renderContext,srcAlphaSrc,alphaFunction);
};

void MWindow::gr_get_blending_mode()
{
	int16 srcAlphaSrc=0,alphaFunction=0;
	if (last_view)
		grGetBlendingMode(last_view->renderContext,&srcAlphaSrc,&alphaFunction);

	a_session->swrite(2, &srcAlphaSrc);
	a_session->swrite(2, &alphaFunction);
	a_session->sync();
};

/*-------------------------------------------------------------*/

void MWindow::gr_set_draw_mode()
{
	short	drawOp;
	a_session->sread(2, &drawOp);

	if (!last_view) return;

	grSetDrawOp(last_view->renderContext,drawOp);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_need_update()
{
	long	result;

	result = update_pending();
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_sync()
{
	a_session->swrite_l(0);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_draw_mode()
{
	short drawOp=0;
	
	if (last_view) grGetDrawOp(last_view->renderContext,&drawOp);

	a_session->swrite(2, &drawOp);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_drawstring()
{
	short	string_length;
	float	spaces[2];
	uchar	buffer[256],*d_buffer=buffer;
	
	a_session->sread(8, (void*)spaces);
	a_session->sread(2, &string_length);

	if (string_length < 0) return;
	
	if (string_length > 256) {
		d_buffer = (uchar*)grMalloc(string_length,"gr_drawstring tmp buf",MALLOC_MEDIUM);
		if (!d_buffer) {
			a_session->drain(string_length);
			return;
		};
	};
	
	if ((a_session->sread(string_length, d_buffer, false) == B_OK) && last_view)
		grDrawString(last_view->renderContext,d_buffer,spaces);
	
	if (d_buffer != buffer)
		grFree(d_buffer);
};

/*-------------------------------------------------------------*/

void MWindow::set_font_context()
{
	if (last_view) {
		grSetFontFromSession(last_view->renderContext, a_session, true);
	} else {
		// There is no view, but we still need to read the data out
		// of the session.
		SFont font;
		font.ReadPacket(a_session, true);
	}
}

void MWindow::get_font_context()
{
	SFont				font;
	
	if (last_view) {
		grGetFont(last_view->renderContext, &font);
	} else {
		SFont::GetStandardFont(B_PLAIN_FONT, &font);
	};

	font.WritePacket(a_session);
	a_session->sync();
	
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_location(bool screwedUp)
{
	rect	tmp_rect;	

	if (last_view) {
		LockViewsForRead();
			last_view->GetFrameInParent(&tmp_rect,screwedUp);
		UnlockViewsForRead();
	} else {
		tmp_rect.left = 0;
		tmp_rect.top = 0;
		tmp_rect.right = -1;
		tmp_rect.bottom = -1;
	}

	a_session->swrite(16, &tmp_rect);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_pt_to_screen()
{
	float	h, v;	

	a_session->sread(sizeof(float), &h);
	a_session->sread(sizeof(float), &v);
	
	LockViewsForRead();
		if (last_view)
			last_view->LocalToWindow(&h, &v);
		else if (top_view)
			top_view->LocalToWindow(&h, &v);
//		LockRegionsForRead();
			h += ClientBound()->left;
			v += ClientBound()->top;
//		UnlockRegionsForRead();
	UnlockViewsForRead();
	
	a_session->swrite(sizeof(float), &h);
	a_session->swrite(sizeof(float), &v);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_pt_from_screen()
{
	float	h, v;
	
	a_session->sread(sizeof(float), &h);
	a_session->sread(sizeof(float), &v);
		
	LockViewsForRead();
//		LockRegionsForRead();
			h -= ClientBound()->left;
			v -= ClientBound()->top;
//		UnlockRegionsForRead();
		if (last_view)
			last_view->WindowToLocal(&h, &v);
		else if (top_view)
			top_view->WindowToLocal(&h, &v);
	UnlockViewsForRead();
	
	a_session->swrite(sizeof(float), &h);
	a_session->swrite(sizeof(float), &v);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_rect_to_screen()
{
	frect r;
	a_session->sread(sizeof(frect), &r);

	LockViewsForRead();
		if (last_view)
			last_view->LocalToWindow(&r);
		else if (top_view)
			top_view->LocalToWindow(&r);
//		LockRegionsForRead();
			r.left += ClientBound()->left;
			r.top += ClientBound()->top;
			r.right += ClientBound()->left;
			r.bottom += ClientBound()->top;
//		UnlockRegionsForRead();
	UnlockViewsForRead();

	a_session->swrite(sizeof(r), &r);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_rect_from_screen()
{
	frect 	r;
	a_session->sread(sizeof(rect), &r);
	
	LockViewsForRead();
//		LockRegionsForRead();
			r.left -= ClientBound()->left;
			r.top -= ClientBound()->top;
			r.right -= ClientBound()->left;
			r.bottom -= ClientBound()->top;
//		UnlockRegionsForRead();
		if (last_view)
			last_view->WindowToLocal(&r);
		else if (top_view)
			top_view->WindowToLocal(&r);
	UnlockViewsForRead();
	
	a_session->swrite(sizeof(r), &r);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_view_bound()
{
	rect	r;
	frect	fr;
	if (last_view) {
		LockViewsForRead();
		last_view->get_bound(&r);
		fr = rect_to_frect(r);
		UnlockViewsForRead();
	} else {
		fr.left = 0.0;
		fr.top = 0.0;
		fr.right = -1.0;
		fr.bottom = -1.0;
	}

	a_session->swrite(sizeof(fr), &fr);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_view_mouse()
{
	long	val[3];
	the_server->get_mouse(&val[0], &val[1]);
	LockRegionsForRead();
	val[0] -= ClientBound()->left;
	val[1] -= ClientBound()->top;
	UnlockRegionsForRead();

	val[2] = the_server->test_button();

	if (last_view) {
		LockViewsForRead();
		last_view->WindowToLocal(&val[0], &val[1]);
		UnlockViewsForRead();
	};

	a_session->swrite(12, val);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void MWindow::gr_send_to_back_window()
{
	long	result;
	TWindow	*a_window;
	long	token;
	short	w_type;

	a_session->sread(4, &token);

	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;

	a_window->send_to_back(true);
}

/*-------------------------------------------------------------*/

void MWindow::gr_select_window()
{
	long	result;
	TWindow	*a_window;
	long	token;
	short	w_type;

	a_session->sread(4, &token);

	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;

	a_window->bring_to_front();
}

/*-------------------------------------------------------------*/

void	MWindow::private_unlink()
{
	tokens->remove_token(server_token);

	a_session->sclose();
	delete a_session;
}

/*-------------------------------------------------------------*/

void	MWindow::gr_close_window()
{
	int32 token;
	a_session->sread(4, &token);
	do_close();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_hide_window()
{
	long	token;
	long	result;
	short	w_type;
	TWindow	*a_window;
	
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;

	a_window->do_hide();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_show_window()
{
	long	token, result;
	short	w_type;
	TWindow	*a_window;
	
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;
	a_window->do_show();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_show_window_sync()
{
	uint32		result = 0;

	gr_show_window();
	a_session->swrite(sizeof(uint32), &result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void MWindow::gr_set_window_picture()
{
	int32		picToken,result;
	uint32		inverse;
	fpoint		offset;
	SPicture *	pic;

	a_session->sread(4,&picToken);
	a_session->sread(sizeof(fpoint),&offset);
	a_session->sread(4,&inverse);

	LockRegionsForWrite();

	if (picToken == NO_TOKEN) {
		if (m_clipToPicture) m_clipToPicture->ServerUnlock();
		m_clipToPicture = NULL;
	} else {
		result = tokens->grab_atom(picToken,(SAtom**)&pic);
		if (result >= 0) {
			if (m_clipToPicture) m_clipToPicture->ServerUnlock();
			m_clipToPicture = pic;
			m_clipToPictureOffset = offset;
			m_clipToPictureFlags = inverse;
		} else {
			DebugPrintf(("picture not here for gr_set_window_picture!\n"));
		};
	};

	recalc_full();

	UnlockRegionsForWrite();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_window_flags()
{
	long		token, result;
	short		w_type;
	uint32		theDesign;
	uint32		theBehavior;	
	uint32		theFlags;
	TWindow		*a_window;

	a_session->sread(4, &token);
	a_session->sread(4, &theDesign);
	a_session->sread(4, &theBehavior);
	a_session->sread(4, &theFlags);

	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;
	a_window->set_flags(theDesign, theBehavior, theFlags);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_send_behind()
{
	long		token, token_front, result;
	short		w_type;
	TWindow		*a_window, *f_window;
	
	a_session->sread(4, &token);
	a_session->sread(4, &token_front);
	
	wait_regions();
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		result = B_ERROR;
	else {
		result = tokens->get_token(token_front, &w_type, (void **)&f_window);
		if ((result < 0) || !(w_type & TT_WINDOW))
			result = B_ERROR;
		else {
			a_window->bring_just_behind(f_window);
			result = B_NO_ERROR;
		}
	}
	signal_regions();
	a_session->swrite(sizeof(uint32), &result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_maximize()
{
	long	token;
	TWindow	*a_window;
	short	w_type;
	long	result;
	
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;
		
	the_server->no_zoom = 1;
	a_window->do_wind_maximize();
	the_server->no_zoom = 0;
}

/*-------------------------------------------------------------*/

void	MWindow::gr_minimize()
{
	long	token;
	TWindow	*a_window;
	short	w_type;
	long	result;
	
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;

	the_server->no_zoom = 1;
	a_window->do_wind_minimize();
	the_server->no_zoom = 0;
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_ws()
{
	long	token;
	long	result;
	short	w_type;
	ulong	count;
	TWindow	*a_window;
	
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result >= 0) && (w_type & TT_WINDOW))
		count = a_window->get_ws();
	else
		count = 0;

	a_session->swrite(4, &count);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_ws()
{
	long	token;
	long	result;
	short	w_type;
	ulong	count;
	TWindow	*a_window;
	
	a_session->sread(4, &count);
	a_session->sread(4, &token);
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result >= 0) && (w_type & TT_WINDOW))
		a_window->set_workspace(count);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_remove_subwindow()
{
	long	token_master;
	long	token_slave;
	long	result;
	TWindow	*master_window;
	TWindow	*slave_window;
	short	w_type;

	a_session->sread(4, &token_master);
	a_session->sread(4, &token_slave);

	wait_regions();
	result = tokens->get_token(token_master, &w_type, (void **)&master_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;
	result = tokens->get_token(token_slave, &w_type, (void **)&slave_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;
	result = master_window->remove_subwindow(slave_window);
	goto exit;
error:
	result = B_ERROR;
exit:
	signal_regions();
	a_session->swrite_l(result);
	a_session->sync();
}


/*-------------------------------------------------------------*/

void	MWindow::gr_add_subwindow()
{
	long	token_master;
	long	token_slave;
	long	result;
	TWindow	*master_window;
	TWindow	*slave_window;
	short	w_type;

	a_session->sread(4, &token_master);
	a_session->sread(4, &token_slave);

	wait_regions();
	result = tokens->get_token(token_master, &w_type, (void **)&master_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;
	result = tokens->get_token(token_slave, &w_type, (void **)&slave_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;
	result = master_window->add_subwindow(slave_window);
	goto exit;
error:
	result = B_ERROR;
exit:
	signal_regions();
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void MWindow::gr_add_window_child()
{
	long result;
	long token;
	long child_token;
	short w_type;
	TWindow	*window;
	TWindow	*child_window;

	a_session->sread(4, &token);
	a_session->sread(4, &child_token);

	wait_regions();
	result = tokens->get_token(token, &w_type, (void **)&window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;

	result = tokens->get_token(child_token, &w_type, (void **)&child_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;

	result = window->insert_child(child_window);

error:
	signal_regions();
	a_session->swrite_l(result);
	a_session->sync();
}

void MWindow::gr_remove_window_child()
{
	long result;
	long token;
	short w_type;
	TWindow	*window;

	a_session->sread(4, &token);

	wait_regions();
	result = tokens->get_token(token, &w_type, (void **)&window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;

	result = window->remove_self();

error:
	signal_regions();
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_movesize_window(long message)
{
	long	result;
	TWindow	*a_window;
	long	val[2];
	long	token;
	short	w_type;

	a_session->sread(4, &token);
	a_session->sread(8, val);

	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		return;

	// filter out complete garbage (protocol failure).
	if (val[0] < -1048576)	val[0] = -1048576;
	if (val[0] > 1048576)	val[0] = 1048576;
	if (val[1] < -1048576)	val[1] = -1048576;
	if (val[1] > 1048576)	val[1] = 1048576;

	switch (message) {
		case GR_MOVE_WINDOW :
			a_window->move(val[0], val[1], 1);
			break;
		case GR_MOVETO_WINDOW :
			a_window->moveto_w(val[0], val[1]);
			break;
		case GR_SIZE_WINDOW :
			a_window->size(val[0], val[1]);
			break;
		case GR_SIZETO_WINDOW :
			a_window->size_to(val[0], val[1]);
			break;
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_remove_view()
{
	long	result;
	TView	*a_view;
	long	token;
	short	v_type;
	rect	bnd;

	a_session->sread(4, &token);

	result = tokens->get_token(token, &v_type, (void **)&a_view);
	if ((result < 0) || (v_type != TT_VIEW))
		return;
	
	LockRegionsForWrite();

//	LockViewsForWrite();
	a_view->GetFrameInWindow(&bnd);
	TView* parent=a_view->parent;
	a_view->do_close();
	if (parent) parent->TweakHierarchy(); else top_view->TweakHierarchy();
//	UnlockViewsForWrite();

	Invalidate(&bnd);

	UnlockRegionsForWrite();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_view_flags()
{
	long	val;

	a_session->sread(4, &val);
	if (last_view) last_view->set_flags(val & (~CLIENT_DRAW));

	a_session->swrite_l(0);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_movesize_view(long message)
{
	long	result;
	TView	*a_view;
	long	val[2];
	long	token;
	short	v_type;

	a_session->sread(4, &token);
	a_session->sread(8, val);

	result = tokens->get_token(token, &v_type, (void **)&a_view);
	if ((result < 0) || (v_type != TT_VIEW)) return;

	// filter out complete garbage (protocol failure).
	if (val[0] < -1048576)
		val[0] = -1048576;
	if (val[0] > 1048576)
		val[0] = 1048576;
	if (val[1] < -1048576)
		val[1] = -1048576;
	if (val[1] > 1048576)
		val[1] = 1048576;

	LockRegionsForWrite();
	if (!m_viewTransactionCount) {
		region *newInval = newregion();
	//	LockViewsForWrite();
		switch (message) {
			case GR_MOVE_VIEW:
				a_view->Move(val[0], val[1], newInval);
				break;
			case GR_MOVETO_VIEW:
				a_view->MoveTo(val[0], val[1], newInval,false);
				break;
			case GR_SIZE_VIEW:
				a_view->Resize(val[0], val[1], newInval);
				break;
			case GR_SIZETO_VIEW:
				a_view->ResizeTo(val[0], val[1], newInval);
				break;
			case GR_SCREWED_UP_MOVETO_VIEW:
				a_view->MoveTo(val[0], val[1], newInval);
				break;
		}
		if (a_view->parent) a_view->parent->TweakHierarchy(); else a_view->TweakHierarchy();
	//	UnlockViewsForWrite();
	
		Invalidate(newInval);
		kill_region(newInval);
	} else {
		switch (message) {
			case GR_MOVE_VIEW:
				a_view->Move(val[0], val[1], NULL);
				break;
			case GR_MOVETO_VIEW:
				a_view->MoveTo(val[0], val[1], NULL, false);
				break;
			case GR_SIZE_VIEW:
				a_view->Resize(val[0], val[1], NULL);
				break;
			case GR_SIZETO_VIEW:
				a_view->ResizeTo(val[0], val[1], NULL);
				break;
			case GR_SCREWED_UP_MOVETO_VIEW:
				a_view->MoveTo(val[0], val[1], NULL);
				break;
		}
		if (a_view->parent) a_view->parent->TweakHierarchy(); else a_view->TweakHierarchy();
	};
	UnlockRegionsForWrite();
}


/*-------------------------------------------------------------*/

void	MWindow::gr_init_direct_window()
{
	long			token;
	long			result;
	void			*area_base;
	short			w_type;
	TWindow			*window;
	dw_init_info	info;
	
	a_session->sread(4, &token);

	wait_regions();
	result = tokens->get_token(token, &w_type, (void **)&window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;

	info.clipping_area = Area::CreateArea("Direct Window area", &area_base,
									B_ANY_ADDRESS, B_PAGE_SIZE, B_NO_LOCK,
									B_READ_AREA | B_WRITE_AREA);

	if (info.clipping_area < 0)
		goto error;
		
	info.disable_sem = create_sem(0, "dd disable sem");
	if (info.disable_sem < 0) {
		Area::DeleteArea(info.clipping_area);
		goto error;
	}
		
	info.disable_sem_ack = create_sem(0, "dd disable sem ack");
	if (info.disable_sem_ack < 0) {
		Area::DeleteArea(info.clipping_area);
		delete_sem(info.disable_sem);
		goto error;
	}
	
	// The resource allocation succeded. Time to init the window object itself.
	window->dw = new DWindow(&info, this);
	
	result = B_NO_ERROR;
	goto exit;
error:
	result = B_ERROR;
exit:
	signal_regions();
	a_session->swrite(sizeof(dw_init_info), (void*)&info);
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_fullscreen()
{
	long			token;
	uint32			enable;
	long			result;
	short			w_type;
	TWindow			*window;
	
	a_session->sread(4, &token);
	a_session->sread(4, &enable);

	wait_regions();
	result = tokens->get_token(token, &w_type, (void **)&window);
	if ((result < 0) || !(w_type & TT_WINDOW))
		goto error;
	
	result = set_fullscreen(&enable);
	goto exit;
error:
	enable = false;
	result = B_ERROR;
exit:
	signal_regions();
	a_session->swrite_l(enable);
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_window_alignment()
{
	int32		mode, h, ho, dh, dho, v, vo, dv, dvo; 
	status_t	result;
	TWindow		*a_window;
	long		token;
	short		w_type;

	a_session->sread(4, &token);
	a_session->sread(4, &mode);
	a_session->sread(4, &h);
	a_session->sread(4, &ho);
	a_session->sread(4, &dh);
	a_session->sread(4, &dho);
	a_session->sread(4, &v);
	a_session->sread(4, &vo);
	a_session->sread(4, &dv);
	a_session->sread(4, &dvo);
	
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result >= 0) || (w_type & TT_WINDOW))
		result = a_window->set_aligner(mode, h, ho, dh, dho, v, vo, dv, dvo);
	else
		result = B_ERROR;
		
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_window_alignment()
{
	int32		mode, h, ho, dh, dho, v, vo, dv, dvo; 
	status_t	result;
	TWindow		*a_window;
	long		token;
	short		w_type;

	a_session->sread(4, &token);
	
	result = tokens->get_token(token, &w_type, (void **)&a_window);
	if ((result >= 0) || (w_type & TT_WINDOW))
		result = a_window->get_aligner(&mode, &h, &ho, &dh, &dho, &v, &vo, &dv, &dvo);
	else {
		mode = h = ho = dh = dho = v = vo = dv = dvo = 0;
		result = B_ERROR;
	}
		
	a_session->swrite_l(mode);
	a_session->swrite_l(h);
	a_session->swrite_l(ho);
	a_session->swrite_l(dh);
	a_session->swrite_l(dho);
	a_session->swrite_l(v);
	a_session->swrite_l(vo);
	a_session->swrite_l(dv);
	a_session->swrite_l(dvo);
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::do_drop(message *a_message)
{
	BParcel	*tb;
	long	global_h;
	long	global_v;
	long	offset_h;
	long	offset_v;
	long	what;
	char	*flat;
	long	flat_size;
	BParcel	an_event(_MESSAGE_DROPPED_);

	tb 	 = (BParcel *)a_message->parm2;
	global_h = a_message->parm3;
	global_v = a_message->parm4;
	offset_h = a_message->parm5;
	offset_v = a_message->parm6;

	LockRegionsForRead();
	global_to_local(&global_h, &global_v);
	UnlockRegionsForRead();

	an_event.AddInt64("when",system_time());
	an_event.AddPoint("where",nfpoint(global_h, global_v));
	an_event.AddPoint("offset",nfpoint(offset_h, offset_v));
	EventPort()->SendEvent(&an_event);
	
	// ??? Yikes. It is very strange, we're basically going into a nested
	// event loop here (handle_client_message), while we're waiting
	// for the client to ask for the dropped message (GET_DROP). We
	// should be able to send the message directly, instead of sending the
	// fake _MESSAGE_DROPPED_ message.
	do {
		a_session->sread(4, &what);
		if (what != GET_DROP)
			handle_client_message(what);
	} while (what != GET_DROP);

	tb->Flatten(&flat, &flat_size);
	a_session->swrite_l(flat_size);
	a_session->swrite(flat_size, flat);
	grFree((char *)flat);
	a_session->sync();

	tb->ServerUnlock();
}

/*-------------------------------------------------------------*/
// Forward events comming thru the event port of the
// window to the int. port of the client task.

void MWindow::handle_message(message *a_message)
{
	switch(a_message->what) {
		case _MESSAGE_DROPPED_ :
			do_drop(a_message);
			return;
	}	
	TWindow::handle_message(a_message);
}

/*---------------------------------------------------------------*/

void	MWindow::gr_varray()
{
	int32		size,count;
	a_line		stackArray[8];
	a_line		*p = stackArray;
	
	a_session->sread(4, &count);

	if (!last_view) return;
	if ((count <= 0) || (count > (1<<24))) return;

	size = count * sizeof(a_line);
	if (count > 8) {
		p = (a_line*)grMalloc(size,"linearray tmp buf",MALLOC_MEDIUM);
		if (!p) {
			a_session->drain(size);
			return;
		};
	};

	if (a_session->sread(size, p, false) == B_OK)
		grStrokeLineArray(last_view->renderContext,p,count);

	if (p != stackArray) grFree(p);
}

/*-------------------------------------------------------------*/

//void	MWindow::gr_get_modifiers()
//{
//	a_session->swrite_l(the_server->last_modifiers);
//	a_session->sync();
//}

extern int32 trackThread;

/*-------------------------------------------------------------*/
void MWindow::handle_client_message(long message)
{
	DEBUGTRACE((last_view,tfInfo,"msg= %s\n",LookupMsg(message)));

	#if AS_DEBUG
	if (getpid() == trackThread) {
		DebugPrintf(("track(%d): msg= 0x%x:%s\n",trackThread,message,LookupMsg(message)));
	};
	#endif

	if (last_view) {
		if ((message >= 0x600) && (message < 0x780)) {
			if (message < 0x700) {
				last_view->SetFlag(vfDrawnIn,true);
				m_statusFlags |= WFLAG_DRAWN_IN;
			};
		} else
			grGiveUpLocks(last_view->renderContext);
	};

	switch(message) {
		case GR_ADD_VIEW:					new_view();								break;
		case GR_MOVE_VIEW:
		case GR_MOVETO_VIEW:
		case GR_SIZE_VIEW:
		case GR_SIZETO_VIEW:				gr_movesize_view(message);				break;
		case GR_REMOVE_VIEW:				gr_remove_view();						break;
		case GR_FIND_VIEW:					gr_find_view();							break;
		case GR_GET_VIEW_BOUND:				gr_get_view_bound();					break;
		case GR_VIEW_FLAGS:					gr_view_flags();						break;
		case GR_VIEW_SET_EVENT_MASK:		gr_set_view_event_mask();				break;
		case GR_VIEW_AUGMENT_EVENT_MASK:	gr_augment_view_event_mask();			break;
		case GR_SCREWED_UP_MOVETO_VIEW:		gr_movesize_view(message);				break;
		case GR_SET_VIEW_CURSOR:			gr_set_view_cursor();					break;
		case GR_SET_VIEW_DOUBLE_BUFFER:		gr_set_view_double_buffer();			break;

		case GR_MOVE_WINDOW:
		case GR_MOVETO_WINDOW:
		case GR_SIZE_WINDOW:
		case GR_SIZETO_WINDOW:				gr_movesize_window(message);			break;
		case GR_SELECT_WINDOW:				gr_select_window();						break;
		case GR_CLOSE_WINDOW:				gr_close_window();						break;
		case GR_WGET_TITLE:					gr_get_window_title();					break;
		case GR_WSET_TITLE:					gr_set_window_title();					break;
		case GR_WGET_BOX:					gr_get_window_bound();					break;
		case GR_WIS_FRONT:					gr_window_is_front();					break;
		case GR_SEND_TO_BACK:				gr_send_to_back_window();				break;
		case GR_WGET_BOUND:					gr_get_window_bound();					break;
		case GR_HIDE:						gr_hide_window();						break;
		case GR_SHOW:						gr_show_window();						break;
		case GR_WINDOW_LIMITS:				do_window_limits();						break;
		case GR_IS_ACTIVE:					gr_window_is_active();					break;
		case GR_MINIMIZE:					gr_minimize();							break;
		case GR_MAXIMIZE:					gr_maximize();							break;
		case GR_SHOW_SYNC:					gr_show_window_sync();					break;
		case GR_SET_WINDOW_FLAGS:			gr_set_window_flags();					break;
		case GR_SEND_BEHIND:				gr_send_behind();						break;
		case GR_ADD_SUBWINDOW:				gr_add_subwindow();						break;
		case GR_REMOVE_SUBWINDOW:			gr_remove_subwindow();					break;
		case GR_SET_WINDOW_ALIGNMENT:		gr_set_window_alignment();				break;
		case GR_GET_WINDOW_ALIGNMENT:		gr_get_window_alignment();				break;
		case GR_GET_BASE_POINTER:			get_pointer();							break;
		case GR_PICK_VIEW:					gr_pick_view();							break;
		case GR_SET_WINDOW_PICTURE:			gr_set_window_picture();				break;
		case GR_WGET_DECOR_STATE:			gr_get_decor_state();					break;
		case GR_WSET_DECOR_STATE:			gr_set_decor_state();					break;
		case GR_ADD_WINDOW_CHILD:			gr_add_window_child();					break;
		case GR_REMOVE_WINDOW_CHILD:		gr_remove_window_child();				break;

		case GR_MOVETO:						gr_moveto();							break;
		case GR_MOVEBY:						gr_moveby();							break;
		case GR_LINE:						gr_line();								break;
		case GR_LINETO:						gr_lineto();							break;
		case GR_RECTFRAME:					gr_rectframe();							break;
		case GR_RECTFILL:					gr_rectfill();							break;
		case GR_ARC_INSCRIBE_STROKE:
		case GR_ARC_INSCRIBE_FILL:
		case GR_ARC_STROKE:
		case GR_ARC_FILL:					gr_arc(message);						break;
		case GR_ROUND_RECT_FRAME:			gr_round_rect_frame();					break;
		case GR_ROUND_RECT_FILL:			gr_round_rect_fill();					break;
		case GR_FRAME_REGION:				/*gr_frame_region();*/					break;
		case GR_FILL_REGION:				gr_fill_region();						break;
		case GR_POLYFRAME:					gr_polyframe();							break;
		case GR_POLYFILL:					gr_polyfill();							break;
		case GR_DRAW_BEZIER:
		case GR_FILL_BEZIER:				gr_bezier(message);						break;
		case GR_ELLIPSE_INSCRIBE_STROKE:
		case GR_ELLIPSE_INSCRIBE_FILL:
		case GR_ELLIPSE_STROKE:
		case GR_ELLIPSE_FILL:				gr_ellipse(message);					break;
		case GR_DRAW_BITMAP:				gr_draw_bitmap(0);						break;
		case GR_SCALE_BITMAP:				gr_scale_bitmap(0);						break;
		case GR_SCALE_BITMAP1:				gr_scale_bitmap1(0);					break;
		case GR_DRAW_BITMAP_A:				gr_draw_bitmap(1);						break;
		case GR_SCALE_BITMAP_A:				gr_scale_bitmap(1);						break;
		case GR_SCALE_BITMAP1_A:			gr_scale_bitmap1(1);					break;
		case GR_DRAW_STRING:				gr_drawstring();						break;
		case GR_RECT_INVERT:				gr_rect_invert();						break;
		case GR_MOVE_BITS:					gr_move_bits();							break;
		case GR_START_VARRAY:				gr_varray();							break;
		case GR_PLAY_PICTURE:				gr_play_picture();						break;
		case GR_STROKE_SHAPE:
		case GR_FILL_SHAPE:					gr_shape(message);						break;

		case GR_SET_DRAW_MODE:				gr_set_draw_mode();						break;
		case GR_VIEW_SET_ORIGIN:			gr_set_view_origin();					break;
		case GR_SET_PEN_SIZE:				gr_set_pen_size();						break;
		case GR_SET_VIEW_COLOR:				gr_set_view_color();					break;
		case GR_SET_LINE_MODE:				gr_set_line_mode();						break;
		case GR_SET_FONT_CONTEXT:			set_font_context();						break;
		case GR_FORE_COLOR:					gr_fore_color();						break;
		case GR_BACK_COLOR:					gr_back_color();						break;
		case GR_NO_CLIP:					gr_no_clip();							break;
		case GR_SET_CLIP:					gr_userclip();							break;
		case GR_SET_SCALE:					gr_set_scale();							break;
		case GR_SET_ORIGIN:					gr_set_origin();						break;
		case GR_PUSH_STATE:					gr_push_state();						break;
		case GR_POP_STATE:					gr_pop_state();							break;
		case GR_SET_PATTERN:				gr_set_pattern();						break;
		case GR_SET_VIEW_BITMAP:			gr_set_view_bitmap();					break;
		case GR_SET_FONT_FLAGS:				gr_set_font_flags();					break;
		case GR_CLIP_TO_SHAPE:				gr_clip_to_picture();					break;
		case GR_SET_BLENDING_MODE:			gr_set_blending_mode();					break;

		case GR_GET_PEN_SIZE:				gr_get_pen_size();						break;
		case GR_GET_FORE_COLOR:				gr_get_fore_color();					break;
		case GR_GET_BACK_COLOR:				gr_get_back_color();					break;
		case GR_GET_PEN_LOC:				gr_get_pen_loc();						break;
		case GR_GET_LOCATION:				gr_get_location(false);					break;
		case GR_GET_DRAW_MODE:				gr_get_draw_mode();						break;
		case GR_GET_CLIP:					gr_get_clip();							break;
		case GR_GET_CAP_MODE:
		case GR_GET_JOIN_MODE:
		case GR_GET_MITER_LIMIT:			gr_get_line_mode(message);				break;
		case GR_GET_ORIGIN:					gr_get_origin();						break;
		case GR_GET_BLENDING_MODE:			gr_get_blending_mode();					break;
		case GR_GET_FONT_CONTEXT:			get_font_context();						break;
		case GR_GET_SCREWED_UP_LOCATION:	gr_get_location(true);					break;

		case GR_ADD_SCROLLBAR:				gr_add_scroll_bar();					break;
		case GR_SCROLLBAR_SET_VALUE:		gr_scroll_bar_set_value();				break;
		case GR_SCROLLBAR_SET_RANGE:		gr_scroll_bar_set_range();				break;
		case GR_SCROLLBAR_SET_STEPS:		gr_scroll_bar_set_steps();				break;
		case GR_SCROLLBAR_SET_PROPORTION:	gr_scroll_bar_set_proportion();			break;

		case GR_PT_TO_SCREEN:				gr_pt_to_screen();						break;
		case GR_PT_FROM_SCREEN:				gr_pt_from_screen();					break;
		case GR_RECT_TO_SCREEN:				gr_rect_to_screen();					break;
		case GR_RECT_FROM_SCREEN:			gr_rect_from_screen();					break;
		case GR_GET_VIEW_MOUSE:				gr_get_view_mouse();					break;

		case GR_INVAL:						gr_inval();								break;
		case GR_UPDATE_OFF:					update_off();							break;
		case GR_UPDATE_ON:					update_on();							break;
		case GR_NEED_UPDATE:				gr_need_update();						break;
		case GR_SYNC:						gr_sync();								break;
		case GR_OPEN_VIEW_TRANSACTION:		gr_open_view_transaction();				break;
		case GR_CLOSE_VIEW_TRANSACTION:		gr_close_view_transaction();			break;
		case GR_INVAL_REGION:				gr_inval_region();						break;

		case GR_START_PICTURE:				gr_start_picture();						break;
		case GR_RESTART_PICTURE:			gr_restart_picture();					break;
		case GR_END_PICTURE:				gr_end_picture();						break;

		case GR_WORKSPACE:					gr_get_ws();							break;
		case GR_SET_WS:						gr_set_ws();							break;

		case GR_DATA:						gr_data();								break;
			
		case GR_INIT_DIRECTWINDOW:			gr_init_direct_window();				break;
		case GR_SET_FULLSCREEN:				gr_set_fullscreen();					break;
		case GR_LOOPER_THREAD:				a_session->sread(4, &client_task_id);	break;
		case GR_START_DISK_PICTURE:			gr_disk_picture();						break;
		case GR_PLAY_DISK_PICTURE:  		gr_play_disk_picture();					break;
		
		case E_START_DRAW_WIND:				gr_begin_client_update();				break;
		case E_END_DRAW_WIND:				gr_end_client_update();					break;
		case E_START_DRAW_VIEW:				gr_next_update_view();					break;

		default: _sPrintf("*** MWindow::handle_client_message: Unknown message=%d! ***\n",message);
	}
}

/*-------------------------------------------------------------*/

void	MWindow::do_window_limits()
{
	long	minh,maxh,minv,maxv;

	a_session->sread(4, &minh);
	a_session->sread(4, &maxh);
	a_session->sread(4, &minv);
	a_session->sread(4, &maxv);

	set_min_max(minh, maxh, minv, maxv);
	a_session->swrite_l(0);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_clip()
{
	region	*a_region;
	long	length;
	short	tmp;

	a_region = newregion();

	if (last_view)
		grGetUpdateRegion(last_view->renderContext,a_region);
	else
		clear_region(a_region);

	a_session->swrite(16, &(a_region->Bounds()));
	tmp = a_region->CountRects();
	a_session->swrite(2, &tmp);
	length = a_region->CountRects() * 16;
	a_session->swrite(length, a_region->Rects());
	a_session->sync();
	kill_region(a_region);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_find_view()
{
	long	h;
	long	v;
	TView	*a_view;

	a_session->sread(4, &h);
	a_session->sread(4, &v);

	a_view = (TView *)top_view->FindView(npoint(h,v));

	if (a_view)
		a_session->swrite_l(a_view->client_token);
	else
		a_session->swrite_l(-1);
		
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_data()
{
	int32	size;
	uint32	oldPtr,ptr;
	Heap *	serverHeap;

	a_session->sread(4, &size);
	a_session->sread(4, &oldPtr);

	serverHeap = m_application->RWHeap();
	if (oldPtr != NONZERO_NULL) serverHeap->Free(oldPtr);

	if ((size <= 0) || (size > 10*1024*1024))
		ptr = NONZERO_NULL;
	else
		ptr = serverHeap->Alloc(size);

	a_session->swrite(4, &ptr);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_window_is_active()
{
	char active;
	active = is_active();
	a_session->swrite(1, &active);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_window_is_front()
{
	char front;
	front = (this == last_top_window);
	a_session->swrite(1, &front);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_window_title()
{
	char*	title;
	long	len;
	
	title = get_name();
	len = strlen(title);
	
	a_session->swrite(4, &len);
	a_session->swrite(len + 1, title);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_window_title()
{
	char	*buf;
	long	len;
	
	a_session->sread(4, &len);
	if (len >= 0 && len < 32768) {
		buf = (char *)grMalloc(len + 1,"window name tmp buf",MALLOC_MEDIUM);
		if (!buf) {
			a_session->drain(len+1);
			return;
		};
		if (a_session->sread(len + 1, buf, false) == B_OK) set_name(buf);
		grFree((char *)buf);
	}
}

/*-------------------------------------------------------------*/

void	MWindow::gr_get_decor_state()
{
	BMessage state;
	get_decor_state(&state);
	
	BMallocIO io;
	state.Flatten(&io);
	
	int32 len = io.BufferLength();
	
	a_session->swrite(4, &len);
	a_session->swrite(len, const_cast<void*>(io.Buffer()));
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_decor_state()
{
	char	*buf;
	long	len;
	
	a_session->sread(4, &len);
	buf = (char *)grMalloc(len,"decor state tmp buf",MALLOC_MEDIUM);
	if (!buf) {
		a_session->drain(len);
		return;
	}
	if (a_session->sread(len, buf, false) == B_OK) {
		BMessage data;
		data.Unflatten(buf);
		if (!data.IsEmpty()) set_decor_state(&data);
	}
	grFree((char *)buf);
}

/*-------------------------------------------------------------*/


void	MWindow::gr_get_window_bound()
{
	rect r;

	LockRegionsForRead();
	r = *ClientBound();
	UnlockRegionsForRead();
	a_session->swrite(16, &r);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	MWindow::gr_set_view_origin()
{
	float val[2];

	a_session->sread(sizeof(float) * 2, val);
	if (last_view) {
		LockRegionsForWrite();
		last_view->do_set_origin(val[0],val[1]);
		last_view->TweakHierarchy();
		UnlockRegionsForWrite();
	};
}

/*-------------------------------------------------------------*/

void MWindow::gr_scroll_bar_set_range()
{
	long sb_server_token;
	long val[2];

	a_session->sread(4, &sb_server_token);
	a_session->sread(8, val);

	TScrollBar	*sb_data;
	long result = tokens->get_token(sb_server_token, (void **)&sb_data);
	if (result==0 && sb_data)
		sb_data->set_range(val[0], val[1]);
}

/*-------------------------------------------------------------*/

void MWindow::gr_scroll_bar_set_steps()
{
	long sb_server_token;
	long val[2];

	a_session->sread(4, &sb_server_token);
	a_session->sread(8, val);

	TScrollBar	*sb_data;
	long result = tokens->get_token(sb_server_token, (void **)&sb_data);
	if (result == 0 && sb_data)
		sb_data->set_steps(val[0], val[1]);
}

/*-------------------------------------------------------------*/

void MWindow::gr_scroll_bar_set_value()
{
	long sb_server_token;
	long new_value;

	a_session->sread(4, &sb_server_token);
	a_session->sread(4, &new_value);

	TScrollBar	*sb_data;
	long result = tokens->get_token(sb_server_token, (void **)&sb_data);
	if (result == 0 && sb_data) {
		sb_data->set_value(new_value, 0);
		sb_data->s_value = new_value;
	}
}

/*-------------------------------------------------------------*/

void MWindow::gr_scroll_bar_set_proportion()
{
	long sb_server_token;
	float new_value;

	a_session->sread(4, &sb_server_token);
	a_session->sread(sizeof(float), &new_value);

	TScrollBar	*sb_data;
	long result = tokens->get_token(sb_server_token, (void **)&sb_data);
	if (result == 0 && sb_data)
		sb_data->set_proportion(new_value);
}

/*-------------------------------------------------------------*/

void	MWindow::gr_add_scroll_bar()
{
	rect		v_bound;
	long		v_type;
	uint32		v_eventMask;
	c_token		v_client_token;
	s_token		v_parent_token;
	TScrollBar	*a_view;
	TView		*parent;
	long		result;
	int32		theToken = NO_TOKEN;

	char		*parent_data;
	short		parent_type;
	long		min;
	long		max;
	short		orientation;
	
	a_session->sread(4,  &min);
	a_session->sread(4,  &max);
	a_session->sread(2,  &orientation);
	a_session->sread(16, &v_bound);
	a_session->sread(4,  &v_type);
	a_session->sread(4,  &v_eventgäğhä hıÆğiíÆ jİ¨ğkÍ¨ lÆÅpm¶Ä€n¦§po–¦€p†‰pqvˆ€rfkpsVj€tFMpu6L€v/iğw.€xKğxÿK yï-ğzß- {Ïğ|¿ }®ñğ~ñ Óğ                                                                                                                                        ÿÿ¹° ÿÿ«  CDT CST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 TZif                                    ÿÿ¹°  EST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           TZif                                  €±8ğ¡à‚‘ğƒ€ıà„püğ…j`†Zp‡Iü`ˆ9ûp‰)Ş`Šİp‹	À`‹ù¿pŒé¢`Ù¡pÒ¾à¹ƒp² à‘¢Ÿğ’’‚à“‚ğ”rdà•bcğ–RFà—BEğ˜2(à™"'ğšE`›Dp›û'`œë&pÛ	`ËpŸºë` ªêp¡šÍ`¢ŠÌp£ƒéà¤j®p¥cËà¦SÊğ§C­à¨3¬ğ©#àªğ«qà«ópğ¬ãSà­ÓRğ®Ìp`¯³4ğ°¬R`±œQp²Œ4`³|3p´l`µ\p¶Kø`·;÷p¸+Ú`¹Ùpºöà»õğ»ôØà¼ä×ğ½Ôºà¾Ä¹ğ¿´œàÀ¤›ğÁ”~àÂ„}ğÃ}›`Äd_ğÅ]}`ÆM|pÇ=_`È-^pÉA`Ê@pÊı#`Ëí"pÌİ`ÍÍpÎÆ!àÏ¶ ğĞ¦àÑ–ğÒ…åàÓuäğÔeÇàÕUÆğÖE©à×5¨ğØ.Æ`ÙŠğÚ¨`Úş§pÛîŠ`ÜŞ‰pİÎl`Ş¾kpß®N`àMpá0`â~/pãwLàä^påW.àæG-ğç7àè'ğéòàêñğêöÔàëæÓğìÖ¶àíÆµğî¿Ó`ï¯ÒpğŸµ`ñ´pò—`óo–pô_y`õOxpö?[`÷/Zpø(wàù<púYàúøXğûè;àüØ:ğıÈàş¸ğÿ§ÿà —şğ‡áàwàğpş``ıpPà`@ßp0Â`p	5`	­”ğ
ğ†`à…pÙ¢àÀgp¹„à©ƒğ™fà‰eğyHàiGğY*àI)ğ9à)ğ")`íğ`ò
páí`ÑìpÁÏ`±Îp¡±` ‘°p!“`"q’p#j¯à$Z®ğ%J‘à&:ğ'*sà(rğ)
Uà)úTğ*ê7à+Ú6ğ,ÓT`-ºğ.³6`/£5p0“`1ƒp2rú`3bùp4RÜ`5BÛp62¾`7"½p8Úà9Ùğ9û¼à:ë»ğ;Ûà<Ëğ=»€à>«ğ?›bà@‹ağA„`BkCğCda`DT`pEDC`F4BpG$%`H$pI`IôpJãé`KÓèpLÍàM³ÊpN¬çàOœæğPŒÉàQ|ÈğRl«àS\ªğTLàU<ŒğV,oàWnğXŒ`Y‹pYõn`Zåmp[ÕP`\ÅOp]µ2`^¥1p_•``…pa~0àbdõpc^àdNğe=ôàf-óğgÖàhÕğhı¸àií·ğjİšàkÍ™ğlÆ·`m¶¶pn¦™`o–˜pp†{`qvzprf]`sV\ptF?`u6>pv/[àw px=àxÿ<ğyïàzßğ{Ïà|¿ ğ}®ãà~âğÅà                                                                                                                                        ÿÿÇÀ ÿÿ¹° EDT EST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 TZif                                    ÿÿs`  HST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           TZif                                    ÿÿ  MST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           TZif                                  €±U¡8 ‚‘7ƒ „q…j6€†Z5‡J€ˆ:‰)ú€Šù‹	Ü€‹ùÛŒé¾€Ù½ÒÛ ¹Ÿ²½ ‘¢¼’’Ÿ “‚”r •b€–Rc —Bb˜2E ™"Dša€›`›ûC€œëBÛ%€Ë$Ÿ»€ «¡šé€¢Šè£„ ¤jÊ¥cè ¦Sç§CÊ ¨3É©#¬ ª«« «ó¬ãp ­Óo®ÌŒ€¯³Q°¬n€±œm²ŒP€³|O´l2€µ\1¶L€·<¸+ö€¹õº »»ôõ ¼äô½Ô× ¾ÄÖ¿´¹ À¤¸Á”› Â„šÃ}·€Äd|Å]™€ÆM˜Ç={€È-zÉ]€Ê\Êı?€Ëí>Ìİ!€ÍÍ ÎÆ> Ï¶=Ğ¦  Ñ–Ò† ÓvÔeä ÕUãÖEÆ ×5ÅØ.â€Ù§ÚÄ€ÚşÃÛî¦€ÜŞ¥İÎˆ€Ş¾‡ß®j€àiáL€â~Kãwi ä^-åWK æGJç7- è',é êêöñ ëæğìÖÓ íÆÒî¿ï€ï¯îğŸÑ€ñĞò³€óo²ô_•€õO”ö?w€÷/vø(” ùXúv úøuûèX üØWıÈ: ş¸9ÿ¨  ˜‡ş wıq€aPü€@û0Ş€5	5ª€	­±
ğ¢€à¡Ù¿ Àƒ¹¡ © ™ƒ ‰‚ye idYG IF9) )("E€	
'€ò&â	€ÒÁë€±ê¡Í€ ‘Ì!¯€"q®#jÌ $ZË%J® &:­'* ()
r )úq*êT +ÚS,Óp€-º5.³R€/£Q0“4€1ƒ32s€3c4Rø€5B÷62Ú€7"Ù8÷ 9ö9ûÙ :ëØ;Û» <Ëº=» >«œ?› @‹~A„›€Bk`Cd}€DT|ED_€F4^G$A€H@I#€Iô"Jä€KÔLÍ" M³æN­ OPŒæ Q|åRlÈ S\ÇTLª U<©V,Œ W‹X¨€Y§YõŠ€Zå‰[Õl€\Åk]µN€^¥M_•0€`…/a~M bec^/ dN.e> f.gó hòhıÕ iíÔjİ· kÍ¶lÆÓ€m¶Òn¦µ€o–´p†—€qv–rfy€sVxtF[€u6Zv/x w<xZ xÿYyï< zß;{Ï |¿}¯  ~ÿâ                                                                                                                                         ÿÿ«  ÿÿ MDT MST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 TZif                                    ÿÿ€  PST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           TZif                                  €±c ¡F‚‘E ƒ(„q' …jD†ZC ‡J&ˆ:% ‰*Š ‹	ê‹ùé ŒéÌÙË Òé¹­ ²Ë‘¢Ê ’’­“‚¬ ”r•b –Rq—Bp ˜2S™"R šo›n ›ûQœëP Û3Ë2 Ÿ» « ¡š÷¢Šö £„¤jØ ¥cö¦Sõ §CØ¨3× ©#ºª¹ «œ«ó› ¬ã~­Ó} ®Ìš¯³_ °¬|±œ{ ²Œ^³|] ´l@µ\? ¶L"·<! ¸,¹ º!»  »õ¼å ½Ôå¾Ää ¿´ÇÀ¤Æ Á”©Â„¨ Ã}ÅÄdŠ Å]§ÆM¦ Ç=‰È-ˆ ÉkÊj ÊıMËíL Ìİ/ÍÍ. ÎÆLÏ¶K Ğ¦.Ñ–- Ò†Óv ÔeòÕUñ ÖEÔ×5Ó Ø.ğÙµ ÚÒÚşÑ Ûî´ÜŞ³ İÎ–Ş¾• ß®xàw áZâ~Y ãwwä^; åWYæGX ç7;è': éê êöÿëæş ìÖáíÆà î¿ıï¯ü ğŸßñŞ òÁóoÀ ô_£õO¢ ö?…÷/„ ø(¢ùf ú„úøƒ ûèfüØe ıÈHş¸G ÿ¨* ˜) ˆx q(a' Q
A	 0ìC 	5¸	­¿ 
ğ°à¯ ÙÍÀ‘ ¹¯©® ™‘‰ ysir YUIT 97)6 "S	 5ò4 âÒ Áù±ø ¡Û ‘Ú !½"q¼ #jÚ$ZÙ %J¼&:» '*( )
€)ú *êb+Úa ,Ó~-ºC .³`/£_ 0“B1ƒA 2s$3c# 4S5C 62è7"ç 89 9ûç:ëæ ;ÛÉ<ËÈ =»«>«ª ?›@‹Œ A„©Bkn Cd‹DTŠ EDmF4l G$OHN I1Iô0 JäKÔ LÍ0M³ô N­O PŒôQ|ó RlÖS\Õ TL¸U<· V,šW™ X¶Yµ Yõ˜Zå— [Õz\Åy ]µ\^¥[ _•>`…= a~[be c^=dN< e>f. gh  hıãiíâ jİÅkÍÄ lÆám¶à n¦Ão–Â p†¥qv¤ rf‡sV† tFiu6h v/†wJ xhxÿg yïJzßI {Ï,|¿+ }¯~Ÿ ğ                                                                                                                                        ÿÿ ÿÿ€ PDT PST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 TZif                                    ÿÿp  YST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           TZif                                  €±q0¡T ‚‘S0ƒ6 „q50…jR †ZQ°‡J4 ˆ:3°‰* Š°‹	ø ‹ù÷°ŒéÚ ÙÙ°Ò÷ ¹»°²Ù ‘¢Ø0’’» “‚º0”r •bœ0–R —B~0˜2a ™"`0š} ›|°›û_ œë^°ÛA Ë@°Ÿ»#  «"°¡› ¢‹°£„" ¤jæ°¥d ¦T0§Cæ ¨3å0©#È ªÇ0«ª «ó©0¬ãŒ ­Ó‹0®Ì¨ ¯³m0°¬Š ±œ‰°²Œl ³|k°´lN µ\M°¶L0 ·</°¸, ¹°º/ ».0»õ ¼å0½Ôó ¾Äò0¿´Õ À¤Ô0Á”· Â„¶0Ã}Ó Äd˜0Å]µ ÆM´°Ç=— È-–°Éy Êx°Êı[ ËíZ°Ìİ= ÍÍ<°ÎÆZ Ï¶Y0Ğ¦< Ñ–;0Ò† Óv0Ôf  ÕUÿ0ÖEâ ×5á0Ø.ş ÙÃ0Úà Úşß°ÛîÂ ÜŞÁ°İÎ¤ Ş¾£°ß®† à…°áh â~g°ãw… ä^I°åWg æGf0ç7I è'H0é+ ê*0ê÷ ëç0ìÖï íÆî0îÀ ï°
°ğŸí ñì°òÏ óoÎ°ô_± õO°°ö?“ ÷/’°ø(° ùt°ú’ úø‘0ûèt üØs0ıÈV ş¸U0ÿ¨8  ˜70ˆ x0q6 a5°Q A°0ú Q°	5Æ 	­Í0
ğ¾ à½°ÙÛ ÀŸ°¹½ ©¼0™Ÿ ‰0y i€0Yc Ib09E )D0"a 	&0C òB°â% Ò$°Â ²°¡é  ‘è°!Ë "qÊ°#jè $Zç0%JÊ &:É0'*¬ («0)
 )ú0*êp +Úo0,ÓŒ -ºQ0.³n /£m°0“P 1ƒO°2s2 3c1°4S 5C°62ö 7"õ°8 909ûõ :ëô0;Û× <ËÖ0=»¹ >«¸0?›› @‹š0A„· Bk|0Cd™ DT˜°ED{ F4z°G$] H\°I? Iô>°Jä! KÔ °LÍ> M´°N­  O0P Q}0Rlä S\ã0TLÆ U<Å0V,¨ W§0XÄ YÃ°Yõ¦ Zå¥°[Õˆ \Å‡°]µj ^¥i°_•L `…K°a~i be-°c^K dNJ0e>- f.,0g h0hıñ iíğ0jİÓ kÍÒ0lÆï m¶î°n¦Ñ o–Ğ°p†³ qv²°rf• sV”°tFw u6v°v/” wX°xv xÿu0yïX zßW0{Ï: |¿90}¯ ~Ÿ0ş                                                                                                                                         ÿÿ€ ÿÿp YDT YST                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  OD   D   D                                                      D      é  D  h  é      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé  w
  O   é  ¬  ¨      T
  #           ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé      ÿÿÿÿé                    Dano0                                                           Dano/0 Â©1991-2001 Be Incorporated                                                                                                                                                                                                                             €                  Dano0                                                           Dano/0 Â©1991-2001 Be Incorporated                                                                                                                                                                                                                              application/x-vnd.Be-elfexecutable VPPA       BEOS:APP_VERSION ÿÿÿÿÿÿÿÿSMIM      
 BEOS:TYPE ÿÿÿÿÿÿÿÿjgŸ                                                                                                                                                                                                                                                                                                                              ELF                  4   TW      4    (                  K  K            K   [   [  È
  @           XU  Xe  Xe              ƒ   º       C   ´          w   {   “   €   ±       ¬       }          ©   a   ”       1   t   ‰   2   œ           b              (   Ÿ   m   J   ·   ’   G   g   ¦   ;   !   —       ¸   9           ‚       ¡   ‘          h   ³   e   •   ¯   A   ®               v   £   s   Š      «   U       `   z   ¥                       _   °   4   x   ¹   –   š               §   *   ­   q             …   N   ¨           ˜   ¢   ²               y   \   c          k   µ          ¶   <       V   -   Œ       8   ™   ^   ›       ª   ¤                                                                                                                                                                                                                           &       "                                           B               '               D   $       7           :              P       =       )           F               %           R   0   Q              [   #   +       j       Z               ?   i   ]               5       H                                      f   ƒ   ~   r   Y       d   „   .   X   S   l   O   †       W   n   ,      /   L   E              ‹   T       >   ˆ   6   ‡          p   M   u   3                   K       o       I   @                  |                       ”                         0            ¤'            |(            „)            ,            L,            ì-       	     $.       
     €1            lI             I             [            `^            Ôc            Hd            Pd            Xd            Xe            èe             f                      n              i            Q            ò      j      )	            ,  ÀB      2  l8       `  x7  ó                 
            /            Ø            k            ¼      j                     Xe       ñÿs      	      (   g             *      ±      	                  Ù      +      `  (g       Ÿ            š      	      ~            «  à[    !  ¸            Ô	            õ
            n            ·            Ã      A        t@  "     ù            Ë              ÄH  5   "  o      5      b	            '  lD  B     U  @D  ,     <      3      ¹               6  X    z      B      "  @]    !      ì-       	 Ó                        5  0g       ›	            Ø            !  °@  -     2  À?  ³     è            È  ˜E  g     “                  ,      Ì      ¥      Ú             í  €H  A   "  X  È;      &   P2  >     @       µ      ·  4H  I   "  „  <g                   ğ              °D  °     :      +      A               2  >     Œ      Ë      æ            Ì                        6   Ğ2  >     ö      &      Ø  ÜC  =     i      ë      í  `E  8     C            U      ó     Œ            %      i                  G
            ¾      #            (      Ğ      	      0      /      ®  à>  à       ˜@       J             }      ‹     ï               G  |     ‚      5      
            À      ë      c  èe       ñÿö              1      	      ı   à@  f           ¾     H      
        ğG  A   "  ¯  Lg       é      R      _            3            v            0   lI        [      (      •  Tg       ~            y      W      Ô            =                  í      !      ³           
      «              B  >     Î  4I  5   "  Û  ¬F  q     \  èe       ñÿŠ      .      
   Xd       ñÿo  `g       ñÿì  HA  È     g   F  ª     ‰   3  >     ô      6      N      ²      ¸      +      –  :  »          H      È      L      c  PB  o     û            
      Š      ü      0     2            c      	      Ü       G      F  üH  5   "  E            o            »
            ù      â     ›            2            š      A      X             œ  D  $     =      o      ¯            ›   P3  Ï    Q      '      u            ‹   ]  @   !   _DYNAMIC _GLOBAL_OFFSET_TABLE_ _init _init_one _fini _fini_one __deregister_frame_info __register_frame_info get_image_symbol _init_two _fini_two __throw BeginPage__10BPclDriverPCQ219BPrinterRasterAddOn14print_bitmap_t malloc__13BPrinterAddOnUl memset generate_dither_256__10BPclDriver Settings__C13BPrinterAddOn DeviceXdpi__C17BPrintJobSettings sprintf SendOutput__10BPclDriverPCc Message__C17BPrintJobSettings FindInt32__C8BMessagePCcPl DeviceYdpi__C17BPrintJobSettings Ydpi__