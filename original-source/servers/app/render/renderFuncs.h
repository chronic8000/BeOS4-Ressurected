
//******************************************************************************
//
//	File:			renderFuncs.h
//	Description:	Defines a function table format for rendering primitives
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef RENDERFUNCS_H
#define RENDERFUNCS_H

struct RenderFuncs {
	void (*fStrokeLineArray)(		RenderContext*,a_line*,int32);
	void (*fStrokeLine)(			RenderContext*,fpoint,fpoint);
	void (*fFillRegion)(			RenderContext*,region*);

	void (*fStrokeRect)(			RenderContext*,frect);
	void (*fFillRect)(				RenderContext*,frect);

	void (*fStrokeRoundRect)(		RenderContext*,frect,fpoint);
	void (*fFillRoundRect)(			RenderContext*,frect,fpoint);

	void (*fStrokeBezier)(			RenderContext*,fpoint*);
	void (*fFillBezier)(			RenderContext*,fpoint*);

	void (*fStrokePolygon)(			RenderContext*,fpoint*,int32,bool);
	void (*fFillPolygon)(			RenderContext*,fpoint*,int32);

	void (*fStrokePath)(			RenderContext*,SPath*);
	void (*fFillPath)(				RenderContext*,SPath*);

	void (*fStrokeArc)(				RenderContext*,fpoint,fpoint,float,float,bool);
	void (*fFillArc)(				RenderContext*,fpoint,fpoint,float,float,bool);

	void (*fInscribeStrokeArc)(		RenderContext*,frect,float,float);
	void (*fInscribeFillArc)(		RenderContext*,frect,float,float);

	void (*fStrokeEllipse)(			RenderContext*,fpoint,fpoint,bool);
	void (*fFillEllipse)(			RenderContext*,fpoint,fpoint,bool);

	void (*fInscribeStrokeEllipse)(	RenderContext*,frect);
	void (*fInscribeFillEllipse)(	RenderContext*,frect);

	void (*fDrawString)(			RenderContext*,const uchar*,float*);
	void (*fDrawPixels)(			RenderContext*,frect,frect,Pixels*);
};

#endif
