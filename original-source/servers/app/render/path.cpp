
//******************************************************************************
//
//	File:			path.cpp
//	Description:	Defines a Postscript-style path
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "path.h"

SPath::SPath(RenderContext *context)
{
	m_context = context;
};

SPath::~SPath()
{
};

SPath::SPath(RenderContext *context, int32 ops, int32 pts)
	: m_ops(ops), m_points(pts)
{
	m_context = context;
};

void SPath::Clear()
{
	m_buildingOp = 0;
	m_ops.SetItems(0);
	m_points.SetItems(0);
};

void SPath::OpenShapes(fpoint pt)
{
	m_buildingOp = 0x80000000;
	m_points.AddItem(pt);
};

void SPath::CloseShapes(bool closeLast)
{
	if (m_buildingOp) m_ops.AddItem(m_buildingOp);
	if (closeLast)
		m_ops.AddItem(0x70000000);
	else
		m_ops.AddItem(0x30000000);
};

void SPath::MoveTo(fpoint pt, bool closeLast)
{
	if (m_buildingOp) m_ops.AddItem(m_buildingOp);
	m_buildingOp = 0x80000000|(closeLast?0x40000000:0);
	m_points.AddItem(pt);
};

void SPath::AddLineSegments(fpoint *pt, int32 count)
{
	if ((m_buildingOp & 0x30000000) == 0x00000000)
		m_buildingOp |= 0x10000000;
	else if ((m_buildingOp & 0x30000000) != 0x10000000) {
		m_ops.AddItem(m_buildingOp);
		m_buildingOp = 0x10000000;
	};
	m_buildingOp+=count;
	while (count--) m_points.AddItem(*pt++);
};

void SPath::AddBezierSegments(fpoint *pt, int32 count)
{
	if ((m_buildingOp & 0x30000000) == 0x00000000)
		m_buildingOp |= 0x20000000;
	else if ((m_buildingOp & 0x30000000) != 0x20000000) {
		m_ops.AddItem(m_buildingOp);
		m_buildingOp = 0x20000000;
	};
	m_buildingOp+=count*3;
	while (count--) {
		m_points.AddItem(pt[0]);
		m_points.AddItem(pt[1]);
		m_points.AddItem(pt[2]);
		pt += 3;
	};
};

frect SPath::Bounds()
{
	frect r;
	r.top = r.left = MAX_OUT;
	r.bottom = r.right = -MAX_OUT;
	fpoint p;
	for (int32 i=0;i<m_points.CountItems();i++) {
		p = m_points[i];
		if (p.h < r.left) r.left = p.h;
		if (p.h > r.right) r.right = p.h;
		if (p.v < r.top) r.top = p.v;
		if (p.v > r.bottom) r.bottom = p.v;
	};
	return r;
};

void SPath::Copy(SPath *path, fpoint translate, float scale)
{
	m_ops.SetItems(0);
	m_ops.AddArray(&path->m_ops);

	int32 count = path->m_points.CountItems();
	m_points.SetItems(count);
	fpoint *src = path->m_points.Items();
	fpoint *dst = m_points.Items();
	for (int32 i=0;i<count;i++) {
		dst[i].h = (src[i].h + translate.h) * scale;
		dst[i].v = (src[i].v + translate.v) * scale;
	};
};

status_t SPath::Transfer(SShaper *shape)
{
	int32 opIndex;
	uint32 tmp,op,count,ptIndex=0;
	fpoint tmpPt[4];

	for (opIndex=0;opIndex<m_ops.CountItems();opIndex++) {
		tmp = m_ops[opIndex];
		op = tmp >> 28;
		count = tmp & 0x0FFFFFFF;

		if (op&0x08) {
			if (opIndex)	shape->MoveTo(m_points[ptIndex++],op&0x04);
			else			shape->OpenShapes(m_points[ptIndex++]);
		};

		switch (op&0x03) {
			case PATH_LINES:
				shape->AddLineSegments(m_points.Items()+ptIndex,count);
				ptIndex += count;
				break;
			case PATH_BEZIER:
				shape->AddBezierSegments(m_points.Items()+ptIndex,count/3);
				ptIndex += count;
				break;
			case PATH_END:
				shape->CloseShapes(op&0x04);
				return B_OK;
		};
	};
	if (opIndex) shape->CloseShapes(false);
	return B_OK;
};
