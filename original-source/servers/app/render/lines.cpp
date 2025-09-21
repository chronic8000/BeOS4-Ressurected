
//******************************************************************************
//
//	File:		lines.cpp
//
//	Description:	Utility routines to generate polygonal thick lines
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <Debug.h>
#include <math.h>
#include "renderInternal.h"
#include "render.h"
#include "renderLock.h"
#include "edges.h"
#include "lines.h"
#include "shape.h"
#include "bezier.h"
#include "fastmath.h"
#include "pointInlines.h"

#define MAX_ROUND_CAP_ERROR 0.4

#define PI M_PI

fpoint ComputeDelta(fpoint p0, fpoint p1, float penSize)
{
	fpoint d,delta;
	float len,_len;
		
	d = sub(p1,p0);
	len = sqrt(d.h*d.h + d.v*d.v);
	_len = 1.0/len;
	d.h *= _len;
	d.v *= _len;
	delta.h = d.v*penSize;
	delta.v = -d.h*penSize;

	return delta;
};

void SThickener::GenerateRoundCap(
	SThickener *shape,
	fpoint p, fpoint delta)
{
	float thetaCount = 0,c,s;
	fpoint newDelta;
	
	thetaCount += shape->m_theta;

	shape->m_points.AddItem(add(p,delta));

	int steps=0,maxSteps=shape->m_numThetaSteps;
	while ((thetaCount < PI) && (steps < maxSteps)) {
		c = shape->m_cosThetaSteps[steps];
		s = shape->m_sinThetaSteps[steps];
		newDelta.h = c*delta.h - s*delta.v;
		newDelta.v = c*delta.v + s*delta.h;
		shape->m_points.AddItem(add(p,newDelta));

		thetaCount += shape->m_theta;
		steps++;
	};

	shape->m_points.AddItem(sub(p,delta));
};

void SThickener::GenerateSquareCap(
	SThickener *shape,
	fpoint p, fpoint delta)
{	
	fpoint halfPen;
	
	halfPen.h = -delta.v;
	halfPen.v = delta.h;
	halfPen = add(p,halfPen);
	
	shape->m_points.AddItem(add(halfPen,delta));
	shape->m_points.AddItem(sub(halfPen,delta));
};

void SThickener::GenerateButtCap(
	SThickener *shape,
	fpoint p, fpoint delta)
{

	fpoint halfPen;
	float len = 1.0/sqrt((delta.h*delta.h) + (delta.v*delta.v));
	halfPen.h = (-delta.v * len) * 0.5;
	halfPen.v = (delta.h * len) * 0.5;
	halfPen = add(p,halfPen);
		
	shape->m_points.AddItem(add(halfPen,delta));
	shape->m_points.AddItem(sub(halfPen,delta));
};

void SThickener::GenerateRoundCorner(
	SThickener *shape,
	fpoint corner, fpoint delta1, fpoint delta2)
{
	shape->m_points.AddItem(add(corner,delta1));
	
	float targetCos,c,s;
	fpoint newDelta;
	
	targetCos = (delta1.h*delta2.h + delta1.v*delta2.v)*shape->m_penSize2_;
	int steps = shape->m_numThetaSteps;
	
	while (steps && (targetCos > shape->m_cosThetaSteps[steps-1])) steps--;		
	shape->m_points.AddItem(add(corner,delta1));

	if (steps) {
		for (int i=0;i<steps;i++) {
			c = shape->m_cosThetaSteps[i];
			s = shape->m_sinThetaSteps[i];
			newDelta.h = c*delta1.h - s*delta1.v;
			newDelta.v = c*delta1.v + s*delta1.h;
			shape->m_points.AddItem(add(corner,newDelta));	
		};
	};

	shape->m_points.AddItem(add(corner,delta2));
};

void SThickener::GenerateMiterCorner(
	SThickener *shape,
	fpoint corner, fpoint delta1, fpoint delta2)
{
	int32 err;
	float c;
	fpoint p0,p1,d0,d1,p;

	c = (delta1.h*delta2.h + delta1.v*delta2.v)*shape->m_penSize2_;
	
	if (c < shape->m_cosMiterLimit)
		goto hum;
		
	p0 = add(corner,delta1);
	p1 = add(corner,delta2);
	d0.h = -delta1.v;
	d0.v = delta1.h;
	d1.h = -delta2.v;
	d1.v = delta2.h;
	
	err = FindIntersection(p0,d0,p1,d1,p);
	if (!err) {
		shape->m_points.AddItem(p);
	} else {
		hum:
		shape->m_points.AddItem(add(corner,delta1));
		shape->m_points.AddItem(add(corner,delta2));
	};
};

void SThickener::GenerateBevelCorner(
	SThickener *shape,
	fpoint corner, fpoint delta1, fpoint delta2)
{
	shape->m_points.AddItem(add(corner,delta1));
	shape->m_points.AddItem(add(corner,delta2));
};

#if 0
void grButtLineToEdges(
	RenderContext *d,
	fpoint p1, fpoint p2,
	Edges &edges)
{
	float penSize = d->totalEffective_PenSize*0.5;
	fpoint delta = ComputeDelta(p1,p2,penSize);
	fpoint pt1,pt2,pt3,pt4;
	
	if (d->capRule == BUTT_CAP) {
		/*	Make butt caps actually use a 0.5 pixel squaring, to
			keep backwards binary visual compatibility. */
		fpoint halfPen;
		float len = (delta.h*delta.h) + (delta.v*delta.v);
		len = 1.0/sqrt(len);
		halfPen.h = (-delta.v * len) * 0.5;
		halfPen.v = (delta.h * len) * 0.5;
		pt2 = add(p2,halfPen);
		pt3 = sub(pt2,delta);
		pt2 = add(pt2,delta);
		pt4 = sub(p1,halfPen);
		pt1 = add(pt4,delta);
		pt4 = sub(pt4,delta);
	} else { //if (d->capRule == B_SQUARE_CAP) {
		fpoint halfPen;
		halfPen.h = -delta.v;
		halfPen.v = delta.h;
		pt2 = add(p2,halfPen);
		pt3 = sub(pt2,delta);
		pt2 = add(pt2,delta);
		pt4 = sub(p1,halfPen);
		pt1 = add(pt4,delta);
		pt4 = sub(pt4,delta);
	};
	
	edges.StartEdges(pt1,pt2);
	edges.AddEdge(pt3);
	edges.AddEdge(pt4);
	edges.CloseEdges();
};
#endif

void SThickener::OpenShapes(fpoint pt)
{
	m_curve.SetItems(1);
	m_curve[0] = pt;
};

void SThickener::MoveTo(fpoint pt, bool closeLast)
{
	ThickenBuffer(closeLast);
	m_curve.SetItems(1);
	m_curve[0] = pt;
};

void SThickener::AddLineSegments(fpoint *pt, int32 count)
{
	int32 oldCount = m_curve.CountItems();
	m_curve.SetItems(oldCount+count);
	for (fpoint *dst=m_curve.Items()+oldCount;count;count--) *dst++ = *pt++;
};

void SThickener::AddBezierSegments(fpoint *pt, int32 count)
{
	fpoint controlPoints[4];
	while (count--) {
		controlPoints[0] = m_curve[m_curve.CountItems()-1];
		controlPoints[1] = *pt++;
		controlPoints[2] = *pt++;
		controlPoints[3] = *pt++;
		TesselateClippedCubicBezier(controlPoints,m_curve,m_clipBounds,0.4);
	};
};

void SThickener::CloseShapes(bool closeLast)
{
	ThickenBuffer(closeLast);
	m_buildingOp = 0x70000000;
	m_ops.AddItem(m_buildingOp);
	m_buildingOp = 0;
};

void SThickener::ThickenBuffer(bool close)
{
	int32 vc,i;
	fpoint *sp=m_curve.Items()+1,*dp=sp;
	for (vc=i=1;i<m_curve.CountItems();i++) {
		if ((sp->h != (sp-1)->h) ||
			(sp->v != (sp-1)->v)) {
			*dp++ = *sp;
			vc++;
		};
		sp++;
	};
	sp = m_curve.Items();
	dp = m_curve.Items()+vc-1;
	if (close && ((sp->h == dp->h) && (sp->v == dp->v))) vc--;

	if (m_joinFunc)	JoinedLines(sp,vc,close);
	else			CappedLines(sp,vc,close);
};

void SThickener::UpdateClipBounds()
{
	rect ri = m_context->renderPort->drawingBounds;
	fpoint trans = m_context->totalEffective_Origin;
	float ps = (m_context->totalEffective_PenSize * 0.5)+1;
	m_clipBounds.left = ri.left - ps - trans.h;
	m_clipBounds.right = ri.right + ps - trans.h;
	m_clipBounds.top = ri.top - ps - trans.v;
	m_clipBounds.bottom = ri.bottom + ps - trans.v;
};

void SThickener::IgnoreClipBounds()
{
	m_clipBounds.left = -MAX_OUT;
	m_clipBounds.right = MAX_OUT;
	m_clipBounds.top = -MAX_OUT;
	m_clipBounds.bottom = MAX_OUT;
};

void SThickener::JoinedLines(fpoint *curve, int32 curveLen, bool closed)
{
	int32 limit,oldPtCount=m_points.CountItems();
	fpoint lastDelta,delta;
	m_buildingOp = 0x90000000;
	
	delta = ComputeDelta(curve[0],curve[1],m_halfPen);
	lastDelta = delta;

	limit = curveLen;
	if (closed) limit++;
	
	for (int32 i=2;i<limit;i++) {
		delta = ComputeDelta(curve[i-1],curve[i%curveLen],m_halfPen);
		if ((delta.h*lastDelta.v - delta.v*lastDelta.h) > 1) {
			m_points.AddItem(add(curve[i-1],lastDelta));
			m_points.AddItem(add(curve[i-1],delta));
		} else {
			m_joinFunc(this,curve[i-1],lastDelta,delta);
		};
		lastDelta = delta;
	};

	if (closed) {
		delta = ComputeDelta(curve[0],curve[1],m_halfPen);
		if ((delta.h*lastDelta.v - delta.v*lastDelta.h) > 1) {
			m_points.AddItem(add(curve[0],lastDelta));
			m_points.AddItem(add(curve[0],delta));
		} else {
			m_joinFunc(this,curve[0],lastDelta,delta);
		};
		m_buildingOp += m_points.CountItems()-oldPtCount-1;
		m_ops.AddItem(m_buildingOp);
		oldPtCount=m_points.CountItems();
		m_buildingOp = 0xD0000000;
	} else {
		delta = ComputeDelta(curve[curveLen-2],curve[curveLen-1],m_halfPen);
		m_capFunc(this,curve[curveLen-1],delta);
	};

	delta = ComputeDelta(curve[curveLen-1],curve[curveLen-2],m_halfPen);
	lastDelta = delta;
	
	limit = 0;
	if (closed) limit--;

	for (int32 i=curveLen-3;i>=limit;i--) {
		delta = ComputeDelta(curve[i+1],curve[(i<0)?(curveLen+i):i],m_halfPen);
		if ((delta.h*lastDelta.v - delta.v*lastDelta.h) > 1) {
			m_points.AddItem(add(curve[i+1],lastDelta));
			m_points.AddItem(add(curve[i+1],delta));
		} else {
			m_joinFunc(this,curve[i+1],lastDelta,delta);
		};
		lastDelta = delta;
	};

	if (closed) {
		delta = ComputeDelta(curve[curveLen-1],curve[curveLen-2],m_halfPen);
		if ((delta.h*lastDelta.v - delta.v*lastDelta.h) > 1) {
			m_points.AddItem(add(curve[curveLen-1],lastDelta));
			m_points.AddItem(add(curve[curveLen-1],delta));
		} else {
			m_joinFunc(this,curve[curveLen-1],lastDelta,delta);
		};
		m_buildingOp += m_points.CountItems()-oldPtCount-1;
		m_ops.AddItem(m_buildingOp);
		oldPtCount=m_points.CountItems();
	} else {
		delta = ComputeDelta(curve[1],curve[0],m_halfPen);
		m_capFunc(this,curve[0],delta);
		m_buildingOp += m_points.CountItems()-oldPtCount-1;
		m_ops.AddItem(m_buildingOp);
	};
};

void SThickener::CappedLines(fpoint *curve, int32 curveLen, bool closed)
{
	int32 limit,oldPtCount=m_points.CountItems();
	LineCapFunc capFunc = m_capFunc;
	int stop,i;
	fpoint delta;
	m_buildingOp = 0x90000000;

	if (closed) {
		capFunc = m_internalCapFunc;
		stop = curveLen;
	} else
		stop = curveLen - 1;
	
	delta = ComputeDelta(curve[1],curve[0],m_halfPen);
	capFunc(this,curve[0],delta);
	delta.h = -delta.h;
	delta.v = -delta.v;
	m_internalCapFunc(this,curve[1],delta);
	m_buildingOp += m_points.CountItems()-oldPtCount-1;
	m_ops.AddItem(m_buildingOp);
	oldPtCount = m_points.CountItems();
	m_buildingOp = 0xD0000000;
	
	for (i=2;i<stop;i++) {
		delta = ComputeDelta(curve[i],curve[i-1],m_halfPen);
		m_internalCapFunc(this,curve[i-1],delta);
		delta.h = -delta.h;
		delta.v = -delta.v;
		m_internalCapFunc(this,curve[i],delta);
		m_buildingOp += m_points.CountItems()-oldPtCount-1;
		m_ops.AddItem(m_buildingOp);
		oldPtCount = m_points.CountItems();
		m_buildingOp = 0xD0000000;
	};

	int32 imodded = i%curveLen;
	delta = ComputeDelta(curve[imodded],curve[i-1],m_halfPen);
	m_internalCapFunc(this,curve[i-1],delta);
	delta.h = -delta.h;
	delta.v = -delta.v;
	capFunc(this,curve[imodded],delta);
	m_buildingOp += m_points.CountItems()-oldPtCount-1;
	m_ops.AddItem(m_buildingOp);
};


LineCapFunc cap_funcs[5] =
{
	SThickener::GenerateRoundCap,
	SThickener::GenerateButtCap,
	SThickener::GenerateButtCap,
	SThickener::GenerateButtCap,
	SThickener::GenerateSquareCap
};

LineJoinFunc corner_funcs[5] =
{
	SThickener::GenerateRoundCorner,
	SThickener::GenerateMiterCorner,
	SThickener::GenerateBevelCorner,
	NULL,
	NULL
};

SThickener::SThickener(RenderContext *context) : SPath(context,4,32)
{
	m_numThetaSteps = 0;
	m_thetaStepsHighWater = 0;
	m_cosThetaSteps = NULL;
	m_sinThetaSteps = NULL;
	m_context = context;
};

SThickener::~SThickener()
{
	if (m_cosThetaSteps) {
		grFree(m_cosThetaSteps);
		grFree(m_sinThetaSteps);
	};
};

void SThickener::LineDependencies()
{
	int32 capRule;
	RenderContext *context = m_context;
	if ((context->joinRule == ROUND_JOIN) || (context->capRule == ROUND_CAP)) {
		/*	This computes theta such that drawing a circle of diameter penWidth
			with a polygonal approximation computed by stepping by theta around
			it's edge ensures a "flatness" error for each segment of no more than 
			MAX_ROUND_CAP_ERROR. */
		m_theta = 2*acos(1.0 - 2.0*(MAX_ROUND_CAP_ERROR/(context->totalEffective_PenSize)));
		
		/* Precompute terms of the rotation */
		m_numThetaSteps = (int32)floor(PI/m_theta);
		if (m_numThetaSteps > m_thetaStepsHighWater) {
			if (m_cosThetaSteps) {
				grFree(m_cosThetaSteps);
				grFree(m_sinThetaSteps);
			};
			m_cosThetaSteps = (float*)grMalloc(m_numThetaSteps*sizeof(float),"cosThetaSteps",MALLOC_CANNOT_FAIL);
			m_sinThetaSteps = (float*)grMalloc(m_numThetaSteps*sizeof(float),"sinThetaSteps",MALLOC_CANNOT_FAIL);
			m_thetaStepsHighWater = m_numThetaSteps;
		};
		for (int i=0;i<m_numThetaSteps;i++) {
			m_cosThetaSteps[i] = cos(m_theta*(i+1));
			m_sinThetaSteps[i] = sin(m_theta*(i+1));
		};
	};
	
	if (context->joinRule == MITER_JOIN) {
		float sinTheta_2 = 1.0/context->miterLimit;
		float theta = asin(sinTheta_2) * 2;
		m_cosMiterLimit = -cos(theta);
	};

	m_halfPen = context->totalEffective_PenSize*0.5;
	m_penSize2_ = 1.0/(m_halfPen*m_halfPen);
	
	capRule = context->capRule;
	if ((capRule == ROUND_CAP) && (context->totalEffective_PenSize < 3.0)) capRule = BUTT_CAP;
	m_capFunc = cap_funcs[capRule];
	m_internalCapFunc = cap_funcs[context->joinRule];
	m_joinFunc = corner_funcs[context->joinRule];
};

SThinLineRenderer::SThinLineRenderer(RenderContext *context)
{
	m_context = context;
};

SThinLineRenderer::~SThinLineRenderer()
{
};

void SThinLineRenderer::OpenShapes(fpoint pt)
{
	RenderPort *port = grLockPort(m_context);
	rect ri = m_context->renderPort->drawingBounds;
	fpoint trans = m_context->totalEffective_Origin;
	m_clipBounds.left = ri.left - trans.h - 1;
	m_clipBounds.right = ri.right - trans.h + 1;
	m_clipBounds.top = ri.top - trans.v - 1;
	m_clipBounds.bottom = ri.bottom - trans.v + 1;

	grLockCanvi(port,&port->drawingBounds,hwLine);

	m_curve.SetItems(1);
	m_curve[0] = pt;
};

void SThinLineRenderer::MoveTo(fpoint pt, bool closeLast)
{
	grDrawManyLines(m_context,m_curve.Items(),m_curve.CountItems(),
		m_context->totalEffective_Origin,closeLast,NULL);
	m_curve.SetItems(1);
	m_curve[0] = pt;
};

void SThinLineRenderer::AddLineSegments(fpoint *pt, int32 count)
{
	int32 oldCount = m_curve.CountItems();
	m_curve.SetItems(oldCount+count);
	for (fpoint *dst=m_curve.Items()+oldCount;count;count--) *dst++ = *pt++;
};

void SThinLineRenderer::AddBezierSegments(fpoint *pt, int32 count)
{
	fpoint controlPoints[4];
	while (count--) {
		controlPoints[0] = m_curve[m_curve.CountItems()-1];
		controlPoints[1] = *pt++;
		controlPoints[2] = *pt++;
		controlPoints[3] = *pt++;
		TesselateClippedCubicBezier(controlPoints,m_curve,m_clipBounds,0.4);
	};
};

void SThinLineRenderer::CloseShapes(bool closeLast)
{
	grDrawManyLines(m_context,m_curve.Items(),m_curve.CountItems(),
		m_context->totalEffective_Origin,closeLast,NULL);

	grUnlockCanvi(m_context->renderPort);
	grUnlockPort(m_context);
};
