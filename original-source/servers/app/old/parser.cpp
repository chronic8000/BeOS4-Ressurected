#include "parser.h"


//----------------------------------------------------------------------------

typedef struct {
		float x0;
		float y0;
		float x1;
		float y1;
	rgb_color col;
} a_line;

typedef enum { PIC_MOVETO = 100,		//100
       	       PIC_LINETO,				//101
       	       PIC_RECTFILL,			//102
       	       PIC_RECT_INVERT,			//103
       	       PIC_SET_DRAW_MODE,		//104
       	       PIC_DRAW_STRING,			//105
			   PIC_SET_PEN_SIZE,		//106
			   PIC_ELLIPSE_FRAME,		//107
			   PIC_ELLIPSE_FILL,		//108
			   PIC_ARC_FRAME,			//109
			   PIC_ARC_FILL,			//110
			   PIC_ROUND_RECT_FRAME,	//111
			   PIC_ROUND_RECT_FILL,		//112
			   PIC_FORE_COLOR,			//113
			   PIC_BACK_COLOR,			//114
			   PIC_LINETO_PAT,			//115
			   PIC_SET_FONT_NAME,		//116
			   PIC_SET_FONT_SIZE,		//117
			   PIC_SET_FONT_SHEAR,		//118
   			   PIC_SET_FONT_ROTATE,		//119
			   PIC_MOVEBY,				//120
			   PIC_SET_SCALE,			//121
			   PIC_SET_FONT_SET,		//122
			   PIC_RECTFRAME,			//123
			   PIC_RECTFRAME_PAT,		//124
			   PIC_FILLPOLY,			//125
			   PIC_FRAMEPOLY,			//126
			   PIC_BLIT,				//127
			   PIC_VARRAY,				//128
			   PIC_END_PICTURE,			//129
			   PIC_OFFSET_FRAME,		//130
			   PIC_SUB_PICTURE			//131
			} pic_type;


//----------------------------------------------------------------------------
//
//	Parser()
//
//	Initialize a new Parser object, without any reference to a file
//
//
//----------------------------------------------------------------------------

Parser::Parser()
{
}

//----------------------------------------------------------------------------
//
//	~Parser()
//
//	Destructor.
//
//			Do nothing
//
//----------------------------------------------------------------------------

Parser::~Parser()
{
}

//----------------------------------------------------------------------------

long	Parser::Read(void *p, long size)
{
	return 0;
}

//----------------------------------------------------------------------------

long	Parser::get_long()
{
	long val;
	
	this->Read(&val,sizeof(long));
	return val;
}

//----------------------------------------------------------------------------

float Parser::get_float()
{
	float val;
	
	this->Read(&val,sizeof(float));
	return val;
}

//----------------------------------------------------------------------------

float_rect Parser::get_rect()
{
	float_rect rect;
	
	this->Read(&(rect.left),sizeof(float));
	this->Read(&(rect.top),sizeof(float));
	this->Read(&(rect.right),sizeof(float));
	this->Read(&(rect.bottom),sizeof(float));
	
	return rect;
}

//----------------------------------------------------------------------------
//
//	frect get_irect 	(
//						)
//		
//	Read an frect structure in the BPicture file and return it
//  The structure is read using a long->float conversion
//		
//----------------------------------------------------------------------------

float_rect Parser::get_irect()
{
	float_rect 	rect;
	long		left,top,right,bottom;
	
	this->Read(&left,sizeof(long));
	this->Read(&top,sizeof(long));
	this->Read(&right,sizeof(long));
	this->Read(&bottom,sizeof(long));
	
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

	return rect;
}

//----------------------------------------------------------------------------
//
//	void get_data 	(
//					)
//		
//	Read "size" byte of data and return it
//		
//----------------------------------------------------------------------------

void Parser::get_data(long size,void *ptr)
{
	this->Read(ptr,size);
	return;
}


//----------------------------------------------------------------------------
//
//	DoParse 	(
//				)
//
//		
//----------------------------------------------------------------------------

void Parser::DoParse()
{
	long	opcode;
	float	pen_h,pen_v;
	bool	endOfPicture;
	float	x,y;

	endOfPicture = FALSE;
	
	while (!endOfPicture) {
		// Read opcode		
		if (this->Read(&opcode,sizeof(long)) != sizeof(long))
			break;

		switch(opcode) {						// Do what's required according to opcode
		
//-------------------------------------------------------------
// moveto absolute position.
//
// 4 bytes. GR_MOVETO
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_MOVETO :	
				{
					pen_h = get_float();
					pen_v = get_float();
					DoMoveTo(pen_h, pen_v);
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
				
					pen_h = get_float();
					pen_v = get_float();
					DoLineTo(pen_h, pen_v);
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
					float_rect	r;
					pattern		p;

					r = get_rect();
					get_data(sizeof(pattern), &p);
					DoRectFill(r, p);
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
					float_rect	r;

					r = get_rect();
					DoRectInvert(r);
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
					drawing_mode	mode;

					mode = (drawing_mode) get_long();
					DoSetMode(mode);
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
		
					DoSetPenSize(pensize);
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
					float_rect	r;
					pattern		p;

					r = get_rect();
					get_data(sizeof(pattern), &p);
					DoEllipseFrame(r, p);
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
					float_rect	r;
					pattern		p;

					r = get_rect();
					get_data(sizeof(pattern), &p);
					DoEllipseFill(r, p);
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
					float_rect	r;
					pattern		p;
					long	start_angle;
					long	end_angle;

					r = get_rect();
					start_angle = get_long();
					end_angle = get_long();
					get_data(sizeof(pattern), &p);

					DoArcFrame(r, start_angle, end_angle, p);
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
					float_rect	r;
					pattern		p;
					long		start_angle;
					long		end_angle;	

					r = get_rect();
					start_angle = get_long();
					end_angle = get_long();
					get_data(sizeof(pattern), &p);

					DoArcFill(r, start_angle, end_angle, p);
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
					float_rect	r;
					pattern		p;
					float		xRadius;
					float		yRadius;

					r = get_rect();
					xRadius = get_float();
					yRadius = get_float();
					get_data(sizeof(pattern), &p);

					DoRoundRectFrame(r, xRadius, yRadius, p);
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
					float_rect	r;
					pattern		p;
					float		xRadius;
					float		yRadius;

					r = get_rect();
					xRadius = get_float();
					yRadius = get_float();
					get_data(sizeof(pattern), &p);

					DoRoundRectFill(r, xRadius, yRadius, p);
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
					DoSetForeColor(a_color.red, a_color.green, a_color.blue);
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
					DoSetBackColor(a_color.red, a_color.green, a_color.blue);
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

					tmp1 = get_float();
					tmp2 = get_float();
					get_data(sizeof(pattern), &p);
					DoLineToPattern(tmp1,tmp2, p);
				}
				break;

//-------------------------------------------------------------
// set font name. 
//
// 4 bytes. PIC_SET_FONT_NAME
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_NAME :		//54b
			{
			}
			break;

//-------------------------------------------------------------
// set font size. 
//
// 4 bytes. PIC_SET_FONT_SIZE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SIZE :		//54c
			{
				float	size;

				size = get_long();
				size = size / 8.0;
				DoSetFontSize(size);
			}
			break;


//-------------------------------------------------------------
// set font shear. 
//
// 4 bytes. PIC_SET_FONT_SHEAR
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SHEAR :		//54d
			{
				long	shear;

				shear = get_long();
				DoSetFontShear(shear);
			}
			break;


//-------------------------------------------------------------
// set font rotate. 
//
// 4 bytes. PIC_SET_FONT_ROTATE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_ROTATE :			//54e
			{
				long angle;
				
				angle = get_long();
				DoSetFontRotate(angle);
			}
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
					DoMoveBy(get_float(), get_float());
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
				
					DoSetScale(scale);
				}
				break;

//-------------------------------------------------------------
// set font set. 
//
// 4 bytes. PIC_SET_FONT_SET
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_SET :			//570
			{
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
					float_rect	r;
					pattern		p;

					r = get_rect();
					get_data(sizeof(pattern), &p);
					DoRectFramePat(r, p);
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
					float_rect	r;

					r = get_rect();
					DoRectFrame(r);
				}
				break;

//-------------------------------------------------------------
// fill polygon 
//
// 4 bytes. PIC_FILLPOLY
//16 bytes. rectangle containing the polygon
// 4 bytes. number of points
// 8 bytes * number of points : points coordinates
// 8 bytes  pattern
// 
//-------------------------------------------------------------
			
		case PIC_FILLPOLY :	
				{
					float_rect		r;
					pattern			p;
					long			num_pt;
					long			i;
					integer_point	*pt_list;
					
					r = get_rect();
					num_pt = get_long();
					pt_list = (integer_point *)malloc(num_pt * PTSIZE);
					get_data(PTSIZE * num_pt, pt_list);
					get_data(sizeof(pattern), &p);
					
					// Alloc buffer containing path
					
					DoFillPoly(pt_list, num_pt, p);				


					free((char *)pt_list);
				}
				break;

//-------------------------------------------------------------
// stroke polygon 
//
// 4 bytes. PIC_FRAMEPOLY
//16 bytes. rectangle containing the polygon
// 8 bytes  pattern
// 4 bytes. flag closed or not
// 4 bytes. number of integer_points
// 8 bytes * number of integer_points : integer_points coordinates
// 
//-------------------------------------------------------------
			
			case PIC_FRAMEPOLY :	
			{
				float_rect			r;
				pattern				p;
				long				num_pt;
				long				i,closed;
				integer_point		*pt_list;
				float				x,y;

				r = get_rect();
				get_data(sizeof(pattern), &p);
				closed = get_long();
				num_pt = get_long();
				pt_list = (integer_point *)malloc(num_pt * PTSIZE);
				get_data(PTSIZE * num_pt, pt_list);
					
				// Alloc buffer containing path			
				DoFramePoly(pt_list, num_pt, p, closed);				
				
				free((char *)pt_list);
			}
			break;

//-------------------------------------------------------------
// blit bitmap 
//
//16 bytes. source rectangle
//16 bytes. dest rectangle
// 4 bytes. ????
// 4 bytes  bit per pixels
// 4 bytes. byte per row
//16 bytes. ????
// 4 bytes. size of datas
// datas
// 
//-------------------------------------------------------------

			case PIC_BLIT :
			{
				float_rect	src_rect;
				float_rect	dst_rect;
				long		ptype;
				long		bit_per_pixel;
				long		rowbyte;
				float_rect	bbox;
				long		bits_size;
				
				// Read description of the bitmap
				src_rect = get_rect();
				dst_rect = get_rect();

				ptype = get_long();
				bit_per_pixel = get_long();
				rowbyte = get_long();
				bbox = get_irect();
				bits_size = get_long();

				uchar *data = (uchar *) malloc(sizeof(uchar) * bits_size);
				get_data(bits_size,data); 
				
				DoBitmap(src_rect,
						 dst_rect,
						 ptype,
						 bit_per_pixel,
						 rowbyte,
						 bbox,
						 (char *)data);
				free(data);
			}
			break;
			

//-------------------------------------------------------------
// line arrays 
//
// 4 bytes. number of integer_points
// 		4 bytes  x0
// 		4 bytes  y0
// 		4 bytes  x1
// 		4 bytes  y1
// 		1 bytes  red
// 		1 bytes  green
// 		1 bytes  blue
// 
//-------------------------------------------------------------
			case PIC_VARRAY :
			{
				long		num_pt, i;
				a_line		the_line;

				num_pt = get_long();
				for (i = 0; i < num_pt; i++) {
					get_data(sizeof(a_line), &the_line);
					DoLineArray(the_line.x0,
								the_line.y0,
								the_line.x1,
								the_line.y1,
								the_line.col.red,
								the_line.col.green,
								the_line.col.blue);
				}
			}
			break;


//-------------------------------------------------------------
// End of picture 
//
// 4 bytes. PIC_END_PICTURE
//
//-------------------------------------------------------------

			case PIC_END_PICTURE :			//575
			{
				DoEndPicture();
			}

//-------------------------------------------------------------

			case PIC_SUB_PICTURE :
				DoSubPicture();
				break;

//-------------------------------------------------------------
// Offset frame 
//
// 4 bytes. PIC_OFFSET_FRAME
// 4 bytes. x offset
// 4 bytes. y offset
//
//-------------------------------------------------------------
			
			case PIC_OFFSET_FRAME :			//576
			{
				float	ddh, ddv;

				ddh = get_float();
				ddv = get_float();
	
				DoSubFrame(ddh, ddv);
			}
			break;

//-------------------------------------------------------------
// Bug or new feature????
//
//-------------------------------------------------------------
			
			default:
			break;
		}
	}
}

//-------------------------------------------------------------
					

//-------------------------------------------------------------

void	Parser::DoRectFill(float_rect a_rect,
						   pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoRectInvert(float_rect a_rect)
{
}

//-------------------------------------------------------------

void	Parser::DoSetMode(long mode)
{
}

//-------------------------------------------------------------

void	Parser::DoSetPenSize(float pensize)
{
}

//-------------------------------------------------------------

void	Parser::DoEllipseFrame(float_rect a_rect,
							   pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoEllipseFill(float_rect a_rect,
							  pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoArcFrame(float_rect a_rect,
						   float start_angle,
						   float end_angle,
						   pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoArcFill(float_rect a_rect,
						  float start_angle,
						  float end_angle,
						  pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoRoundRectFrame(float_rect a_rect,
								 float xRadius,
								 float yRadius,
								 pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoRoundRectFill(float_rect a_rect,
								float xRadius,
								float yRadius,
								pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoSetForeColor(float red,
							   float green,
							   float blue)
{
}

//-------------------------------------------------------------

void	Parser::DoSetBackColor(float red,
							   float green,
							   float blue)
{
}

//-------------------------------------------------------------

void	Parser::DoLineToPattern(float x,
								float y,
								pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoSetFontSize(float size)
{
}

//-------------------------------------------------------------

void	Parser::DoSetFontShear(float shear)
{
}

//-------------------------------------------------------------

void	Parser::DoSetFontRotate(float angle)
{
}

//-------------------------------------------------------------

void	Parser::DoMoveBy(float dx,
						 float dy)
{
}

//-------------------------------------------------------------

void	Parser::DoSetScale(float scale)
{
}

//-------------------------------------------------------------

void	Parser::DoRectFramePat(float_rect a_rect,
							   pattern p)
{
}

//-------------------------------------------------------------

void	Parser::DoRectFrame(float_rect a_rect)
{
}

//-------------------------------------------------------------

void	Parser::DoFillPoly(integer_point *pt_list,
						   long 		 num_pt,
						   pattern 		 p)
{
}				

//-------------------------------------------------------------

void	Parser::DoFramePoly(integer_point *pt_list,
							long 		  num_pt,
							pattern 	  p,
							bool 		  closed)				
{
}

//-------------------------------------------------------------

void	Parser::DoBitmap(float_rect src_rect,
						 float_rect dst_rect,
						 long 		ptype,
						 long 		bit_per_pixel,
						 long 		rowbyte,
						 float_rect bbox,
						 char 		*p)
{
}

//-------------------------------------------------------------

void	Parser::DoLineArray(float x0,
							float y0,
							float x1,
							float y1,
							float red,
							float green,
							float blue)
{
}

//-------------------------------------------------------------

void	Parser::DoEndPicture()
{
}

//-------------------------------------------------------------

void	Parser::DoSubPicture()
{
}

//-------------------------------------------------------------

void	Parser::DoSubFrame(float ddx, float ddy)
{
}

//-------------------------------------------------------------

void	Parser::DoMoveTo(float pen_h,
						 float pen_v)
{
}

//-------------------------------------------------------------

void	Parser::DoLineTo(float pen_h,
						 float pen_v)
{
}
