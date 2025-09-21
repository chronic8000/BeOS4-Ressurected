
//******************************************************************************
//
//	File:			renderUtil.h
//	Description:	Handy utility functions and constants related to rendering
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERUTIL_H
#define RENDERUTIL_H

#include "basic_types.h"

using namespace B::Support2;
using namespace B::Raster2;

enum {
	BLACK = 0,
	GRAY
};

extern const rgb_color	rgb_white;
extern const rgb_color	rgb_black;
extern const rgb_color	rgb_gray;
extern const rgb_color	rgb_dk_gray;

extern pattern usefulPatterns[];

inline pattern *get_pat(int32 idx) {
	return usefulPatterns+idx;
}

inline rgb_color rgb(int32 r, int32 g, int32 b) {
	rgb_color tmp;
	tmp.red = r;
	tmp.green = g;
	tmp.blue = b;
	return tmp;
}

int32 grPixelFormat2BitsPerPixel(int32 format);

#endif
