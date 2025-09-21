//******************************************************************************
//
//	File:		fregion.cpp
//
//	Description:	Floating-point (scalable) regions
//	
//	Written by:	George Hoffman
//
//	Copyright 1993, Be Incorporated
//
//******************************************************************************/

#include "fregion.h"
#include <math.h>

fregion::fregion() : m_rects(10)
{
};

fregion::fregion(frect *fr, int32 rectCount) : m_rects(rectCount)
{
	Copy(fr,rectCount);
};

void fregion::Copy(frect *fr, int32 rectCount)
{
	frect bound;

	m_rects.SetItems(0);
	bound.top = 1e30;
	bound.left = 1e30;
	bound.bottom = -1e30;
	bound.right = -1e30;
	for (int32 i=0;i<rectCount;i++) {
		if (bound.top > fr->top) bound.top = fr->top;
		if (bound.left > fr->left) bound.left = fr->left;
		if (bound.bottom < fr->bottom) bound.bottom = fr->bottom;
		if (bound.right < fr->right) bound.right = fr->right;
		m_rects.AddItem(*fr);
		fr++;
	};
	m_bounds = bound;
};

fregion::fregion(fregion *reg) : m_rects(reg->m_rects.CountItems())
{
	Copy(reg);
};

void fregion::Copy(fregion *reg)
{
	m_rects.SetItems(reg->m_rects.CountItems());
	memcpy(m_rects.Items(),reg->m_rects.Items(),reg->m_rects.CountItems()*sizeof(frect));
	m_bounds = reg->m_bounds;
	m_signature = reg->m_signature;
};

fregion::fregion(region *reg) : m_rects(reg->CountRects())
{
	Copy(reg);
};

void fregion::Copy(region *reg)
{
	frect fr,bound;
	rect r;
	
	const int32 clipCount = reg->CountRects();
	const rect* clipRects = reg->Rects();
	
	m_rects.SetItems(0);
	bound.top = 1e30;
	bound.left = 1e30;
	bound.bottom = -1e30;
	bound.right = -1e30;
	for (int32 i=0;i<clipCount;i++) {
		r = clipRects[i];
		fr.top = r.top;
		fr.left = r.left;
		fr.bottom = r.bottom+1;
		fr.right = r.right+1;
		if (bound.top > fr.top) bound.top = fr.top;
		if (bound.left > fr.left) bound.left = fr.left;
		if (bound.bottom < fr.bottom) bound.bottom = fr.bottom;
		if (bound.right < fr.right) bound.right = fr.right;
		m_rects.AddItem(fr);
	};
	m_bounds = bound;
};

fregion::~fregion()
{
};

frect & fregion::Bounds()
{
	return m_bounds;
};

frect & fregion::RectAt(int32 index)
{
	return m_rects[index];
};

int32 fregion::RectCount()
{
	return m_rects.CountItems();
};

region * fregion::IRegion(float scale)
{
	rect r;
	frect fr;

	region *reg = newregion();
	for (int32 i=0;i<m_rects.CountItems();i++) {
		fr = m_rects[i];
		fr.top *= scale;
		fr.bottom *= scale;
		fr.left *= scale;
		fr.right *= scale;
		r.top = (int32)floor(fr.top+0.5);
		r.left = (int32)floor(fr.left+0.5);
		r.bottom = (int32)floor(fr.bottom+0.5);
		r.right = (int32)floor(fr.right+0.5);
		r.bottom--;
		r.right--;
		add_rect(reg,r);
	};
	return reg;
};
