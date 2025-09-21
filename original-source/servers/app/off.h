//******************************************************************************
//
//	File:		off.h
//
//	Description:	offscreen header file.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	8/05/92		bgs	new today
//
//******************************************************************************

#ifndef	OFF_H
#define	OFF_H

#include "mwindow.h"
#include "as_debug.h"

class SBitmap;

/*-------------------------------------------------------------*/

class	TOff : public MWindow {

	public:
					TOff(	SApp *application, char *name, int32 x, int32 y, 
							uint32 kind, uint32 wflags,
							uint32 bitmapFlags, int32 rowBytes,
							Heap *serverHeap=NULL,
							TSession *session=NULL);
					~TOff();

	virtual	void	move(long dh, long dv, char forced);
	virtual	void	moveto_w(long h, long v);
	virtual	void	size(long dh, long dv);
	virtual	void	size_to(long new_h, long new_v, char forced=0);
	virtual	char	is_front();
	virtual	void	bring_to_front(bool, bool);
	virtual	void	send_to_back(bool);
	virtual	void	do_close();
	virtual	void	get_pointer();

	inline SBitmap *Bitmap();

	#if AS_DEBUG
	virtual	void	DumpDebugInfo(DebugInfo *);
	virtual char * 	ObjectName() { return "TOff"; };
	#endif

#if ROTATE_DISPLAY
	DisplayRotater	rotater;
#endif			

	private:
	
	SBitmap *		bitmap;
};

SBitmap * TOff::Bitmap()
{
	return bitmap;
};

/*-------------------------------------------------------------*/

extern void	new_window_offscreen(TSession *a_session);

#include "bitmap.h"

#endif
