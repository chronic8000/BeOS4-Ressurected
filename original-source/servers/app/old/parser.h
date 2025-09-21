//******************************************************************************
//
//	File:		Parser.h
//
//			Benoit Schillings.		1997		new today.
//
//
//******************************************************************************

#if !defined(PARSER)

#define PARSER

#include <InterfaceDefs.h>
#include <Object.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//-----------------------------------------------------------------

typedef struct {
	float left;
	float top;
	float right;
	float bottom;
} 	float_rect;

//-----------------------------------------------------------------

typedef struct {
	long left;
	long top;
	long right;
	long bottom;
} 	integer_rect;

//-----------------------------------------------------------------

typedef struct {
	long h;
	long v;
} integer_point;

//-----------------------------------------------------------------


class Parser : public BObject {
public:

				Parser();
				~Parser();

		void 		DoParse();
		long 		get_long();
		float 		get_float();
		float_rect 	get_rect();
		float_rect 	get_irect();
		void 		get_data(long size, void *ptr);

		long	Read(void *p, long len);

virtual	void	DoMoveTo(		float pen_h,
				 				float pen_v);
virtual	void	DoLineTo(		float pen_h,
				 				float pen_v);
virtual	void	DoRectFill(		float_rect a_rect,	
								pattern    p);
virtual	void	DoRectInvert(	float_rect a_rect);
virtual	void	DoSetMode(		long mode);
virtual	void	DoSetPenSize(	float pensize);
virtual	void	DoEllipseFrame(	float_rect a_rect,
					   			pattern p);
virtual	void	DoEllipseFill(	float_rect a_rect,
					  			pattern p);
virtual	void	DoArcFrame(		float_rect a_rect,
				   				float start_angle,
				   				float end_angle,
				   				pattern p);
virtual	void	DoArcFill(		float_rect a_rect,
				  				float start_angle,
				  				float end_angle,
				  				pattern p);
virtual	void	DoRoundRectFrame(float_rect a_rect,
						 		float xRadius,
						 		float yRadius,
						 		pattern p);
virtual	void	DoRoundRectFill(float_rect a_rect,
								float xRadius,
								float yRadius,
								pattern p);
virtual	void	DoSetForeColor(	float red,
					   			float green,
					   			float blue);
virtual	void	DoSetBackColor(	float red,
					   			float green,
					   			float blue);
virtual	void	DoLineToPattern(float x,
								float y,
								pattern p);
virtual	void	DoSetFontSize(	float size);
virtual	void	DoSetFontShear(	float shear);
virtual	void	DoSetFontRotate(float angle);
virtual	void	DoMoveBy(		float dx,
				 				float dy);
virtual	void	DoSetScale(		float scale);
virtual	void	DoRectFramePat(	float_rect a_rect,
					   			pattern p);
virtual	void	DoRectFrame	(	float_rect a_rect);
virtual	void	DoFillPoly(		integer_point *pt_list,
				   				long num_pt,
				   				pattern p);
virtual	void	DoFramePoly(	integer_point *pt_list,
								long num_pt,
								pattern p,
								bool closed);			
virtual void	DoBitmap(		float_rect src_rect,
						 		float_rect dst_rect,
						 		long 		ptype,
						 		long 		bit_per_pixel,
						 		long 		rowbyte,
						 		float_rect bbox,
						 		char 		*p);
virtual	void	DoLineArray(	float x0,
								float y0,
								float x1,
								float y1,
								float red,
								float green,
								float blue);
virtual	void	DoEndPicture();
virtual	void	DoSubPicture();
virtual	void	DoSubFrame(		float ddx,
								float ddy);
};


//-----------------------------------------------------------------

static long PTSIZE = sizeof(integer_point);

//--------------------------------------------------------------------

#endif
