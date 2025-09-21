
//******************************************************************************
//
//	File:			renderPixels.cpp
//	Description:	Bitmap blitting rendering commands
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <math.h>
#include <Debug.h>
#include <support/StreamIO.h>

#include "renderInternal.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderLock.h"
#include "accelPackage.h"
#include "newPicture.h"
#include "render.h"
#include "rect.h"
#include "display_rotation.h"
#include "bitmapcollection.h"
#include "DrawEngine.h"


static const int32 bitsForPixFormat[5] = { 1, 8, 16, 16, 32 };
extern BlitFunction BlitCopySameFormat;
#define STACK_BUFFER_SIZE 800

void grSwitchScaledBlit(RenderContext *context, Pixels *src, Pixels *dst, frect srcRect, rect dstRect, RenderSubPort *port)
{
	int32	offs,x,y,countY,countX,
			srcByteWidth,dstByteWidth,
			defaultSrcYInc,i;
	uint8	*srcPtr,*dstPtr;
	rect	r2;
	fpoint	subSrc;
	float 	tmpf;
	int32 	srcPixSize = 0;
	int32 	dstPixSize = 0;
	char	uncompressBufferStack[STACK_BUFFER_SIZE*4];
	void *	uncompressBuffer = uncompressBufferStack;
	BlitFunction *blits,blitFunc=NULL;

	region *clip = port->RealDrawingRegion();
	if (!rect_in_region(clip, &dstRect))
		return;

	srcPixSize = bitsForPixFormat[src->pixelFormat];
	if (dst->pixelFormat > PIX_8BIT) dstPixSize++;
	if (dst->pixelFormat >= PIX_32BIT) dstPixSize++;

	const int32 count = clip->CountRects();
	const rect* rects = clip->Rects();

	//	We are working in a strange coordinate system.  Here is how we have to
	//	interpret floating-point scaling values: the bounds of the source rect
	//	are assumed to designate the center of the whole outermost pixel I want
	//	to fully appear in my destination rect.  Said another way, the pixel-boundary
	//	(rather than pixel-center) coordinates of the rectangle I want to blit from
	//	is designated by (srcRect.left, srcRect.top, srcRect.right+1, srcRect.bottom+1)

	srcRect.right += 1.0;
	srcRect.bottom += 1.0;
	int32 dw=dstRect.right-dstRect.left+1,dh=dstRect.bottom-dstRect.top+1;
	float sw=srcRect.right-srcRect.left,sh=srcRect.bottom-srcRect.top;
	int32 fixedX,fixedY,xInc,yInc;
	float pixelX,pixelY;

	// First calculate the size of a destination pixel in the source scale
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		pixelX = sh/dw;
		pixelY = sw/dh;
		//	_sPrintf("pixelX = sh/dw : %f = %f/%d\n", pixelX, sh, dw);
	} else
#endif
	{
		pixelX = sw/dw;
		pixelY = sh/dh;
	}
	
	if ((pixelX < 0.0001) || (pixelY < 0.0001)) return;

	/*	Convert to fixed point.  This is how much
		we have to source step per destination pixel. */
	xInc = (int32)floor(pixelX * 65536.0);
	yInc = (int32)floor(pixelY * 65536.0);

#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		blits = port->cache->fScaledRotatedBlitTable;
	} else
#endif
	{
		blits = port->cache->fScaledBlitTable;
	}

	if (!blits) return;
	int32 srcPixFormat = src->pixelFormat;
	int32 srcEndianess = src->endianess;
	if (src->isCompressed) {
		srcPixFormat = PIX_32BIT;
		srcEndianess = LENDIAN;
		if (src->w > STACK_BUFFER_SIZE)
			uncompressBuffer = grMalloc(src->w*4, "uncompress", MALLOC_CANNOT_FAIL);
	}
	// If we have no renderer, make sure to free memory before returning
	if ((blitFunc = blits[srcPixFormat*2 + srcEndianess]) == NULL)
		goto free_and_exit;

	/*	Trim both source and dest rectangles based on how much
		source we actually have */

#if ROTATE_DISPLAY
	float dstPixels;
	if (port->canvasIsRotated) {
		tmpf = src->h - srcRect.bottom;
		if (tmpf < 0.0) {
			dstPixels = -tmpf/pixelX;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.left = (int32)(dstRect.left + dstPixels);
			srcRect.bottom = (int32)(srcRect.bottom - (dstPixels * pixelX));
		}
	
		tmpf = srcRect.left;
		if (tmpf < 0.0) {
			dstPixels = -tmpf/pixelY;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.top = (int32)(dstRect.top + dstPixels);
			srcRect.left = (int32)(srcRect.left + (dstPixels * pixelY));
		}
	
		tmpf = srcRect.top;
		if (tmpf < 0.0) {
			dstPixels = -tmpf/pixelX;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.right = (int32)(dstRect.right - dstPixels);
			srcRect.top = (int32)(srcRect.top + (dstPixels * pixelX));
		}
	
		tmpf = srcRect.right - src->w;
		if (tmpf > 0.0) {
			dstPixels = tmpf/pixelY;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.bottom = (int32)(dstRect.bottom - dstPixels);
			srcRect.right = (int32)(srcRect.right - (dstPixels * pixelY));
		}
	} else
#endif
	{
		float dstPixels;
		tmpf = srcRect.left;
		if (tmpf < 0.0) {
			dstPixels = -tmpf/pixelX;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.left = (int32)(dstRect.left + dstPixels);
			srcRect.left = (int32)(srcRect.left + (dstPixels * pixelX));
		}
	
		tmpf = srcRect.top;
		if (tmpf < 0.0) {
			dstPixels = -tmpf/pixelY;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.top = (int32)(dstRect.top + dstPixels);
			srcRect.top = (int32)(srcRect.top + (dstPixels * pixelY));
		}
	
		tmpf = srcRect.right - src->w;
		if (tmpf > 0.0) {
			dstPixels = tmpf/pixelX;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.right = (int32)(dstRect.right - dstPixels);
			srcRect.right = (int32)(srcRect.right - (dstPixels * pixelX));
		}
	
		tmpf = srcRect.bottom - src->h;
		if (tmpf > 0.0) {
			dstPixels = tmpf/pixelY;			// dstPixels == number of destination pixels we have to cut
			dstPixels = ceil(dstPixels);		// we can only cut whole pixels, and have to be conservative
			dstRect.bottom = (int32)(dstRect.bottom - dstPixels);
			srcRect.bottom = (int32)(srcRect.bottom - (dstPixels * pixelY));
		}
	}

	if ((srcRect.left >= srcRect.right) || (srcRect.top >= srcRect.bottom))		goto free_and_exit;
	if ((dstRect.left > dstRect.right) || (dstRect.top > dstRect.bottom))		goto free_and_exit;
	
	srcByteWidth = src->bytesPerRow;
	dstByteWidth = dst->bytesPerRow;
	defaultSrcYInc = 0;
	if (yInc > 65536) {
		defaultSrcYInc = (yInc>>16);
#if ROTATE_DISPLAY
		if (port->canvasIsRotated)
			defaultSrcYInc *= (srcPixSize>>3);
		else
			defaultSrcYInc *= srcByteWidth;
#else
		defaultSrcYInc *= srcByteWidth;
#endif
		yInc &= 0x0000FFFF;
	};
	
	for (i=0; i<count; i++) {
#if ROTATE_DISPLAY
		if (port->canvasIsRotated) {
			sect_rect(rects[i], dstRect, &r2);
			if (!empty_rect(r2)) {
				/*	r2 is a subset of dstRect.  We have to fill it with samples
					based on our mapping.  First, figure out our UL corner in the source */
				subSrc.h = srcRect.bottom - ((r2.left - dstRect.left) + 0.5) * pixelX;
				subSrc.v = srcRect.left + ((r2.top - dstRect.top) + 0.5) * pixelY;
				
				/*	Convert this to a starting x and y, and fractional components thereof */
				fixedX = (int32)floor(subSrc.h*65536 + 0.5);
				fixedY = (int32)floor(subSrc.v*65536 + 0.5);
				x = fixedX>>16; fixedX = 65536-(fixedX & 0x0000FFFF);
				y = fixedY>>16; fixedY = 65536-(fixedY & 0x0000FFFF);
	
				/*	Compute the actual pixel positions */
				offs = (y*srcPixSize);
				srcPtr = src->pixelData + (x*srcByteWidth) + (offs>>3);
				dstPtr = dst->pixelData + (r2.top*dstByteWidth) + (r2.left<<dstPixSize);
				offs = offs & 0x07;
	
				/*	Go for it */
				if (!src->isLbxCompressed) {
					blitFunc(
						context,port, src,dst,
						srcPtr,srcByteWidth,
						dstPtr,dstByteWidth,
						r2.right-r2.left+1,fixedX,xInc,
						r2.bottom-r2.top+1,fixedY,yInc,defaultSrcYInc,
						srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
				} else { // LBX - rotated, scaled
					countX = r2.right-r2.left+1;
					countY = r2.bottom-r2.top+1;
					dstPtr = dst->pixelData + (r2.top*dstByteWidth) + (r2.left<<dstPixSize);
					lbx_pixel_data_t *p = (lbx_pixel_data_t *)src->pixelData;
					LBX_DrawEngine *draw = p->draw_engine;
					while (countX--) {
						draw->DrawFirstLine((uint32 *)uncompressBuffer, y, x, src->w);
						blitFunc(
							context,port, src,dst,
							(uint8*)uncompressBuffer,srcByteWidth,
							dstPtr,dstByteWidth,
							1,0,0,
							countY,fixedY,yInc,defaultSrcYInc,
							srcPixSize>>3, 0, TOP_TO_BOTTOM|LEFT_TO_RIGHT);		
						dstPtr += (1<<dstPixSize);
						fixedX += xInc;
						if (fixedX > 65535) {
							x -= fixedX >> 16;
							x = (x < 0) ? 0 : x;
							fixedX &= 0x0000FFFF;
						}
					}
				}
			}
		} else
#endif	// ROTATE_DISPLAY
		{
			sect_rect(rects[i], dstRect, &r2);
			if (!empty_rect(r2)) {
				/*	r2 is a subset of dstRect.  We have to fill it with samples
					based on our mapping.  First, figure out our UL corner in the source */
				subSrc.h = srcRect.left + (r2.left - dstRect.left + 0.5) * pixelX;
				subSrc.v = srcRect.top + (r2.top - dstRect.top + 0.5) * pixelY;
				
				/*	Convert this to a starting x and y, and fractional components thereof */
				fixedX = (int32)floor(subSrc.h*65536);
				fixedY = (int32)floor(subSrc.v*65536);
				x = fixedX>>16; fixedX &= 0x0000FFFF;
				y = fixedY>>16; fixedY = 65536-(fixedY & 0x0000FFFF);
	
				/*	Compute the actual pixel positions */
				offs = (x*srcPixSize);
				srcPtr = src->pixelData + (y*srcByteWidth) + (offs>>3);
				dstPtr = dst->pixelData + (r2.top*dstByteWidth) + (r2.left<<dstPixSize);
				offs = offs & 0x07;
	
				/*	Go for it */
				countX = r2.right-r2.left+1;
				countY = r2.bottom-r2.top+1;
	
				if (!src->isLbxCompressed) {
					blitFunc(
						context,port,src,dst,
						srcPtr,srcByteWidth,
						dstPtr,dstByteWidth,
						countX,fixedX,xInc,
						countY,fixedY,yInc,defaultSrcYInc,
						srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
				} else { // LBX - non rotated, scaled
					int32 horizError = fixedX;
					int32 horizInc = xInc;
					int32 horizCount = countX;
					int32 vertCount = countY;
					dstPtr = dst->pixelData + (r2.top*dstByteWidth) + (r2.left<<dstPixSize);
					lbx_pixel_data_t *p = (lbx_pixel_data_t *)src->pixelData;
					LBX_DrawEngine *draw = p->draw_engine;
					while (vertCount--)
					{
						draw->DrawFirstLine((uint32 *)uncompressBuffer, x, y, src->w);
						blitFunc(
							context,port,src,dst,
							(uint8*)uncompressBuffer,srcByteWidth,
							dstPtr,dstByteWidth,
							horizCount,horizError,horizInc,
							1,0,0,0,
							srcPixSize>>3, 0, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
	
						dstPtr += dstByteWidth;
						fixedY -= yInc;
						if ((fixedY < 0) || defaultSrcYInc) {
							y += defaultSrcYInc;
							if (fixedY < 0) {
								fixedY += 65536;
								y++;
							}
							fixedY &= 0x0000FFFF;
						}
					}
				}
			}
		}
	}
free_and_exit:
	if ((src->isCompressed) && (src->w > STACK_BUFFER_SIZE))
		grFree(uncompressBuffer);
}

void grSwitchNonscaledBlit(RenderContext *context, Pixels *src, Pixels *dst, rect srcRect, rect dstRect, RenderSubPort *port)
{
	int32	i,offs,k,top,bottom,clippedTop,clippedBottom;
	uint8	*srcPtr,*dstPtr;
	rect	r1,r2;
	int32 	srcPixSize = 0;
	int32 	dstPixSize = 0;
	BlitFunction blitFunc = NULL;
	char	uncompressBufferStack[STACK_BUFFER_SIZE*4];
	void *	uncompressBuffer = uncompressBufferStack;

	region *clip = port->RealDrawingRegion();
	top = dstRect.top;
	bottom = dstRect.bottom;
	if ((top > bottom) || (dstRect.left > dstRect.right) ||
		((i=find_span_between(clip,top,bottom))==-1)) return;

	srcPixSize = bitsForPixFormat[src->pixelFormat];
	if (dst->pixelFormat > PIX_8BIT) dstPixSize++;
	if (dst->pixelFormat >= PIX_32BIT) dstPixSize++;

	// LBX fast mode : B_PIXEL_ALPHA, B_ALPHA_OVERLAY, B_OP_ALPHA, 16 bits, no scaled, not rotated
	bool lbx_fast_mode = false;
	if (src->isLbxCompressed)
	{
 		lbx_pixel_data_t *p = (lbx_pixel_data_t *)src->pixelData;
		if ((context->srcAlphaSrc == B_PIXEL_ALPHA)	&&
			(context->alphaFunction == B_ALPHA_OVERLAY)	&&
			(context->drawOp == B_OP_ALPHA)	&&
			(dst->pixelFormat == PIX_16BIT))
		{
			#if ROTATE_DISPLAY
				lbx_fast_mode = (src->pixelIsRotated == dst->pixelIsRotated);
			#else
				lbx_fast_mode = (src->pixelIsRotated == false);
			#endif
		}
	}

	const int32 count = clip->CountRects();
	const rect* rects = clip->Rects();

	BlitFunction *blits;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		blits = port->cache->fRotatedBlitTable;
		if (!blits) blits = port->cache->fScaledRotatedBlitTable;
	} else
#endif
	{
		blits = port->cache->fBlitTable;
		if (!blits) blits = port->cache->fScaledBlitTable;
	}
	if (!blits) return;

	int32 srcPixFormat = src->pixelFormat;
	int32 srcEndianess = src->endianess;
	if ((src->isCompressed) && (!lbx_fast_mode)) {
		srcPixFormat = PIX_32BIT;
		srcEndianess = LENDIAN;
		if (src->w > STACK_BUFFER_SIZE)
			uncompressBuffer = grMalloc(src->w*4, "uncompress", MALLOC_CANNOT_FAIL);
	}
	// If we have no renderer, make sure to free memory before returning
	if ((blitFunc = blits[srcPixFormat*2 + srcEndianess]) == NULL)
		goto free_and_exit;


#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		for (i=0; i<count; i++) {
			r1 = rects[i];
			sect_rect(r1, dstRect, &r2);
			if (!empty_rect(r2)) {
				r1 = srcRect;
				r1.left  += (r2.top - dstRect.top);
				r1.top   -= (r2.right  - dstRect.right);
				r1.bottom += (dstRect.left - r2.left);
				r1.right -= (dstRect.bottom - r2.bottom);

				if (!src->isLbxCompressed) {
					offs =  (r1.left*srcPixSize);
					srcPtr = src->pixelData + (r1.bottom*src->bytesPerRow) + (offs>>3);
					dstPtr = dst->pixelData + (r2.top*dst->bytesPerRow) + (r2.left<<dstPixSize);
					offs = offs & 0x07;
					blitFunc(
						context,port, src,dst,
						srcPtr,src->bytesPerRow,
						dstPtr,dst->bytesPerRow,
						r2.right-r2.left+1,0,0x10000,
						r2.bottom-r2.top+1,0,0,srcPixSize>>3,
						srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
				} else { // LBX - rotated, non scaled
					int32 bytesPerRow = (r1.right - r1.left + 1) * 4;
					dstPtr = dst->pixelData + (r2.top*dst->bytesPerRow) + (r2.right<<dstPixSize);
					lbx_pixel_data_t *p = (lbx_pixel_data_t *)src->pixelData;
					LBX_DrawEngine *draw = p->draw_engine;		
					if (lbx_fast_mode) {
						draw->DrawRect((uint16 *)dstPtr, dst->bytesPerRow, r1.left, r1.top, r1.right+1,  r1.bottom+1);
					} else {
						draw->DrawFirstLine((uint32 *)uncompressBuffer, r1.left, r1.top, r1.right+1);
						blitFunc(
							context,port,src,dst,
							(uint8*)uncompressBuffer,bytesPerRow,
							dstPtr,dst->bytesPerRow,
							1,0,0x10000,
							r2.bottom-r2.top+1,0,0,srcPixSize>>3,
							srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
						dstPtr -= 1<<dstPixSize;
						for (k=r1.top+1 ; k<= r1.bottom ; k++) {
							draw->DrawNextLine((uint32 *)uncompressBuffer);
							blitFunc(
								context,port,src,dst,
								(uint8*)uncompressBuffer,bytesPerRow,
								dstPtr,dst->bytesPerRow,
								1,0,0x10000,
								r2.bottom-r2.top+1,0,0,srcPixSize>>3,
								srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
							dstPtr -= 1<<dstPixSize;
						}
					}
				}
			}
		}
	} else
#endif
	{
		for (i=0; i<count; i++) {
			r1 = rects[i];
			sect_rect(r1, dstRect, &r2);
			if (!empty_rect(r2)) {
				r1 = srcRect;
				r1.left  += (r2.left - dstRect.left);
				r1.top   += (r2.top  - dstRect.top);
				r1.bottom -= (dstRect.bottom - r2.bottom);
				r1.right -= (dstRect.right - r2.right);

				if (!src->isLbxCompressed) {
					offs =  (r1.left*srcPixSize);
					srcPtr = src->pixelData + (r1.top*src->bytesPerRow) + (offs>>3);
					dstPtr = dst->pixelData + (r2.top*dst->bytesPerRow) + (r2.left<<dstPixSize);
					offs = offs & 0x07;
					blitFunc(
						context,port,src,dst,
						srcPtr,src->bytesPerRow,
						dstPtr,dst->bytesPerRow,
						r1.right-r1.left+1,0,0x10000,
						r1.bottom-r1.top+1,0,0,src->bytesPerRow,
						srcPixSize>>3, offs, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
				} else { // Mathias : LBX Non rotated, non scaled
					const int32 bytesPerRow = (r1.right - r1.left + 1) * 4;
					lbx_pixel_data_t *p = (lbx_pixel_data_t *)src->pixelData;
					LBX_DrawEngine *draw = p->draw_engine;
					dstPtr = dst->pixelData + (r2.top*dst->bytesPerRow) + (r2.left<<dstPixSize);
					if (lbx_fast_mode) {
						draw->DrawRect((uint16 *)dstPtr, dst->bytesPerRow, r1.left, r1.top, r1.right+1,  r1.bottom+1);
					} else {
						draw->DrawFirstLine((uint32 *)uncompressBuffer, r1.left, r1.top, r1.right+1);					
						dstPtr = dst->pixelData + (r2.top * dst->bytesPerRow) + (r2.left << dstPixSize);
						blitFunc(
							context, port, src, dst,
							(uint8 *)uncompressBuffer, bytesPerRow,
							dstPtr, dst->bytesPerRow,
							r1.right - r1.left + 1, 0, 0x10000,
							1, 0, 0, bytesPerRow, srcPixSize>>3, 0, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
						r2.top++;
						for (k=r1.top+1 ; k<=r1.bottom ; k++) {
							draw->DrawNextLine((uint32 *)uncompressBuffer);
							dstPtr = dst->pixelData + (r2.top * dst->bytesPerRow) + (r2.left << dstPixSize);
							blitFunc(
								context, port, src, dst,
								(uint8 *)uncompressBuffer, bytesPerRow,
								dstPtr, dst->bytesPerRow,
								r1.right - r1.left + 1, 0, 0x10000,
								1, 0, 0, bytesPerRow, srcPixSize>>3, 0, TOP_TO_BOTTOM|LEFT_TO_RIGHT);
							r2.top++;
						}
					}
				}
			}
		}
	}
free_and_exit:
	if ((src->isCompressed) && (!lbx_fast_mode) && (src->w > STACK_BUFFER_SIZE))
		grFree(uncompressBuffer);
}


void grCopyPixels(RenderContext *context, rect f_rect, point to, bool clipSource)
{
	int32 count;
	int32 bytesPerRow;
	int32 srcPixSize;
	int32 blitCount;
	rect r1,r2,r3;
	rect stackDests[10];
	Pixels * pixels;
	BlitFunction blitFunc;
	blit_params blits[20];
	blit_params *blitPtr;

	const rect src_rect = f_rect;
	region *clip = newregion();
	region *tmpRegion = newregion();
	region *tmpRegion2 = newregion();

	RenderPort *ueberPort = grLockPort(context);
	grLockCanvi(ueberPort, NULL, hwRectCopy);
	
	RenderSubPort *port = ueberPort;
	while (port) {
		const int32 ox = (int32)floor(context->totalEffective_Origin.h + port->origin.h + 0.5);
		const int32 oy = (int32)floor(context->totalEffective_Origin.v + port->origin.v + 0.5);
		f_rect = src_rect;
		offset_rect(&f_rect, ox, oy);
		point port_to = to;
		port_to.h += ox;
		port_to.v += oy;

		rect t_rect;
		t_rect.top = port_to.v;
		t_rect.left = port_to.h;
		t_rect.bottom = t_rect.top + (f_rect.bottom - f_rect.top);
		t_rect.right = t_rect.left + (f_rect.right - f_rect.left);

		int32 dh, dv;
#if ROTATE_DISPLAY
		if (port->canvasIsRotated) {
			port->rotater->RotateRect(&t_rect, &t_rect);
			dh = port->rotater->RotateDV(port_to.v - f_rect.top);
			dv = port->rotater->RotateDH(port_to.h - f_rect.left);
		} else
#endif
		{
			dh = port_to.h - f_rect.left;
			dv = port_to.v - f_rect.top;
		}		

		set_region(tmpRegion2, &t_rect);
		if (clipSource) {
			copy_region(port->RealDrawingRegion(), clip);
			offset_region(clip, dh, dv);
			and_region(clip, port->RealDrawingRegion(), tmpRegion);
			and_region(tmpRegion, tmpRegion2, clip);
		} else {
			and_region(port->RealDrawingRegion(), tmpRegion2, clip);
		}

		blitFunc = NULL;
		if (!empty_region(clip)) {
			rect u = clip->Bounds();
			rect ut = u;
			offset_rect(&ut,-dh,-dv);
			union_rect(&u,&ut,&u);
			engine_token *accelToken;
			screen_to_screen_blit blit = port->canvas->accelPackage->rectCopy;
			if (blit) {
				#if AS_DEBUG
				if ((port->hardwarePending&hwRectCopyOp) == 0) {
					DebugPrintf(("*** grCopyPixels: [context=%p, port=%p, canvas=%p, accelPackage=%p]", context, port, port->canvas, port->canvas->accelPackage));
					debugger("hwRectCopy not set in hardwarePending!");
					blit = NULL;
				} else
				#endif
				accelToken = grLockAccel(context,port,port->canvas);
			} else {
				pixels = &port->canvas->pixels;
				int32 pixelFormat = pixels->pixelFormat;
				int32 endianess = pixels->endianess;
				srcPixSize = bitsForPixFormat[pixelFormat]>>3;
				bytesPerRow = pixels->bytesPerRow;
				blitFunc = BlitCopySameFormat;
			}
		
			const int32 clipCount = clip->CountRects();
			const rect* clipRects = clip->Rects();
			
			blitCount=0;
			blitPtr = blits;
			
			int32 i,index=0,inc=1,y,count,innerIndex,hinc=1;
			if (dv > 0) {
				index = clipCount-1;
				inc = -1;
			}
			if (dh > 0) hinc = -1;
	
			for (i = clipCount; i ;index += inc) {
				count = 1;
				y = clipRects[index].top;
				innerIndex = index;
				i--;
				while (i && (clipRects[index+inc].top == y)) {
					index += inc;
					count++; i--;
				};
				if (hinc != inc) innerIndex = index;
				for (;count;count--,innerIndex+=hinc) {
					r1 = clipRects[innerIndex];
					r2 = r1;
					offset_rect(&r2, -dh, -dv);
					if (blit) {
						blitPtr->src_left = r2.left;
						blitPtr->src_top = r2.top;
						blitPtr->dest_left = r1.left;
						blitPtr->dest_top = r1.top;
						blitPtr->width = r2.right-r2.left;
						blitPtr->height = r2.bottom-r2.top;
						blitPtr++; blitCount++;
						if (blitCount == 20) {
							blit(accelToken,blits,blitCount);
							blitCount = 0;
							blitPtr = blits;
						}
					} else if (blitFunc) {
						uint8 *srcPtr, *dstPtr;
						srcPtr = pixels->pixelData + (r2.top*bytesPerRow) + r2.left*srcPixSize;
						dstPtr = pixels->pixelData + (r1.top*bytesPerRow) + r1.left*srcPixSize;
	
						blitFunc(
							context,port,pixels,pixels,
							srcPtr,bytesPerRow,
							dstPtr,bytesPerRow,
							r1.right-r1.left+1,0,0x10000,
							r1.bottom-r1.top+1,0,0,bytesPerRow,
							srcPixSize, 0, ((dh<=0)?LEFT_TO_RIGHT:0)|((dv<=0)?TOP_TO_BOTTOM:0)
						);
					}
				}
			}
			if (blit) {
				if (blitCount) blit(accelToken,blits,blitCount);
				grUnlockAccel(context,port,port->canvas);
			}
		}
		port = port->next;
	}

	grUnlockCanvi(ueberPort);
	grUnlockPort(context);

	kill_region(clip);
	kill_region(tmpRegion);
	kill_region(tmpRegion2);
};


void grDrawPixelsRecordPart(
	RenderContext *context,
	rect pixelsNeeded, int32 bpp,
	frect srcRect, frect dstRect,
	Pixels *src)
{
	static const uint32 internalToExternal[] = {
		B_GRAY1, B_CMAP8, B_RGB15, B_RGB16, B_RGB32
	};
	int32 width = pixelsNeeded.right-pixelsNeeded.left+1;
	int32 height = pixelsNeeded.bottom-pixelsNeeded.top+1;
	int32 bpr = (((width*bpp)>>3)+3) & 0xFFFFFFFC;
	if (bpr != src->bytesPerRow) bpr = (width*bpp)>>3;

	context->record->BeginOp(SPIC_PIXELS);
	context->record->AddRect(srcRect);
	context->record->AddRect(dstRect);
	context->record->AddInt32(width);
	context->record->AddInt32(height);
	context->record->AddInt32(bpr);
	uint32 combined = internalToExternal[src->pixelFormat];
	if (src->endianess == BENDIAN) combined |= IS_BIG_ENDIAN;
	context->record->AddInt32(combined);

	uint32 flags = 0;
	if (src->isLbxCompressed) {
		// Right now we don't support LBX bitmap in BPictures
		// flags |= BITMAP_IS_LBX_COMPRESSED;
	}
#if ROTATE_DISPLAY
	if (src->pixelIsRotated) {
		// Right now we don't support Rotated bitmaps in Bpictures
		// flags |= B_BITMAP_IS_ROTATED;
	}
#endif

	context->record->AddInt32(flags);
	context->record->AddInt32(height*bpr);
	uint8 *ptr = (uint8*)src->pixelData;
	ptr += pixelsNeeded.top * src->bytesPerRow;
	ptr += pixelsNeeded.left * bpp / 8;
	if (bpr == src->bytesPerRow) {
		context->record->AddData(ptr,height*src->bytesPerRow);
	} else {
		for (int32 i=height;i>0;i--) {
			context->record->AddData(ptr,bpr);
			ptr += src->bytesPerRow;
		}
	}
	context->record->EndOp();
}

void grDrawPixels_Record(
	RenderContext *context,
	frect srcRect, frect dstRect,
	Pixels *src)
{
	// If the Pixels are LBX, ignore it
	if (src->isLbxCompressed)
		return;

	if (src->pixelFormat < 0) return;
	const int32 bpp = bitsForPixFormat[src->pixelFormat];
	
	/* First, let's convert to a coordinate system that makes sense... */
	rect pixelsNeeded;
	
	if (bpp == 1) {
		//	Don't bother for 1-bit bitmaps right now...
		//	too much of a pain in the ass
		pixelsNeeded.left = pixelsNeeded.top = 0;
		pixelsNeeded.right = src->w-1;
		pixelsNeeded.bottom = src->h-1;
	} else {
		pixelsNeeded.left = (int32)floor(srcRect.left+0.5);
		pixelsNeeded.top = (int32)floor(srcRect.top+0.5);
		pixelsNeeded.right = (int32)floor(srcRect.right+0.5);
		pixelsNeeded.bottom = (int32)floor(srcRect.bottom+0.5);
		if (pixelsNeeded.left < 0) pixelsNeeded.left = 0;
		if (pixelsNeeded.top < 0) pixelsNeeded.top = 0;
		if (pixelsNeeded.right >= src->w) pixelsNeeded.right = src->w-1;
		if (pixelsNeeded.bottom >= src->h) pixelsNeeded.bottom = src->h-1;
		if ((pixelsNeeded.left > pixelsNeeded.right) ||
			(pixelsNeeded.top > pixelsNeeded.bottom))
			return;
	}

	srcRect.left -= pixelsNeeded.left;
	srcRect.right -= pixelsNeeded.left;
	srcRect.top -= pixelsNeeded.top;
	srcRect.bottom -= pixelsNeeded.top;

	grSyncRecordingState(context);
	grDrawPixelsRecordPart(context,pixelsNeeded,bpp,srcRect,dstRect,src);
}

void grDrawPixels_FatThin( RenderContext *context, frect ff_rect, frect ft_rect, Pixels *src)
{
	RDebugPrintf((context,"grDrawPixels_FatThin({%f,%f,%f,%f},{%f,%f,%f,%f},0x%08x)\n",
		ff_rect.left,ff_rect.top,ff_rect.right,ff_rect.bottom,
		ft_rect.left,ft_rect.top,ft_rect.right,ft_rect.bottom,
		src));

	rect f_rect;
	f_rect.left		= (int32)floor(ff_rect.left+0.5);
	f_rect.top		= (int32)floor(ff_rect.top+0.5);
	f_rect.right	= (int32)floor(ff_rect.right+0.5);
	f_rect.bottom	= (int32)floor(ff_rect.bottom+0.5);

	rect t_rect;
	t_rect.left		= (int32)floor(context->totalEffective_Origin.h + (ft_rect.left-0.5)*context->totalEffective_Scale + 1);
	t_rect.top 		= (int32)floor(context->totalEffective_Origin.v + (ft_rect.top-0.5)*context->totalEffective_Scale + 1);
	t_rect.right	= (int32)floor(context->totalEffective_Origin.h + (ft_rect.right+0.5)*context->totalEffective_Scale);
	t_rect.bottom	= (int32)floor(context->totalEffective_Origin.v + (ft_rect.bottom+0.5)*context->totalEffective_Scale);

	RenderPort *ueberPort = grLockPort(context);

#if ROTATE_DISPLAY
	// This handle the case when the source is also rotated...
	if (src->pixelIsRotated)
	{
		// We don't know how to rotate the other way...
		if (!ueberPort->canvasIsRotated) {
			grUnlockPort(context);
			return;
		}
		
		//	Below that, we know that we need to use the rotated region and bounds,
		//	but the code path will be the one for non rotated case. So we will
		//	hardcode access to the rotated region, and disable the master flag
		//	temporarily.

		if (!src->isLbxCompressed) {
			// In the LBX case we don't have to reverse the bitmap's width/height
			// becausse gr_draw_bitmap has already done that for us
			const int32 top = f_rect.top;
			const int32 bottom = f_rect.bottom;
			f_rect.top = f_rect.left;
			f_rect.bottom = f_rect.right;
			f_rect.left = src->w-1-bottom;
			f_rect.right =  src->w-1-top;
		}

		ueberPort->rotater->RotateRect(&t_rect, &t_rect);
		
		if (((f_rect.bottom - f_rect.top) == (t_rect.bottom - t_rect.top)) &&
			((f_rect.right - f_rect.left) == (t_rect.right - t_rect.left)))
		{
			long delta;
			delta = f_rect.top;
			if (delta < 0) {
				f_rect.top -= delta;
				t_rect.top -= delta;
			}
		
			delta = f_rect.left;
			if (delta < 0) {
				f_rect.left -= delta;
				t_rect.left -= delta;
			}
		
			delta = src->h - f_rect.bottom - 1;
			if (delta < 0) {
				f_rect.bottom += delta;
				t_rect.bottom += delta;
			}
		
			delta = src->w - f_rect.right - 1;
			if (delta < 0) {
				f_rect.right += delta;
				t_rect.right += delta;
			}

			grLockCanvi(ueberPort, NULL, hwBlit);
			RenderSubPort *port = ueberPort;
			while (port) {
				rect tt_rect = t_rect;
				const int dh = port->rotater->RotateDV(port->origin.v);
				const int dv = port->rotater->RotateDH(port->origin.h);
				offset_rect(&tt_rect, dh, dv);
				Pixels *dst = &port->canvas->pixels;
				port->canvasIsRotated = false;
					port->SwapDrawingRegions();
						grSwitchNonscaledBlit(context, src, dst, f_rect, tt_rect, port);
					port->SwapDrawingRegions();
				port->canvasIsRotated = true;
				port = port->next;
			}
			grUnlockCanvi(ueberPort);
		} else {
			grLockCanvi(ueberPort, NULL, hwBlit);
			RenderSubPort *port = ueberPort;
			while (port) {
				rect tt_rect = t_rect;
				const int dh = port->rotater->RotateDV(port->origin.v);
				const int dv = port->rotater->RotateDH(port->origin.h);
				offset_rect(&tt_rect, dh, dv);
				Pixels *dst = &port->canvas->pixels;
				port->SwapDrawingRegions();
					port->canvasIsRotated = false;
						grSwitchScaledBlit(context, src, dst, ff_rect, tt_rect, port);
					port->canvasIsRotated = true;
				port->SwapDrawingRegions();
				port = port->next;
			}
			grUnlockCanvi(ueberPort);
		}
	} else
#endif
	{
		if (((f_rect.bottom - f_rect.top) == (t_rect.bottom - t_rect.top)) &&
			((f_rect.right - f_rect.left) == (t_rect.right - t_rect.left))) {
			long delta;
			delta = f_rect.top;
			if (delta < 0) {
				f_rect.top -= delta;
				t_rect.top -= delta;
			}
		
			delta = f_rect.left;
			if (delta < 0) {
				f_rect.left -= delta;
				t_rect.left -= delta;
			}
		
			delta = src->h - f_rect.bottom - 1;
			if (delta < 0) {
				f_rect.bottom += delta;
				t_rect.bottom += delta;
			}
		
			delta = src->w - f_rect.right - 1;
			if (delta < 0) {
				f_rect.right += delta;
				t_rect.right += delta;
			}
			
 			grLockCanvi(ueberPort, NULL, hwBlit);
			RenderSubPort *port = ueberPort;
			while (port) {
				rect tt_rect = t_rect;
				offset_rect(&tt_rect, port->origin.h, port->origin.v);
#if ROTATE_DISPLAY
				if (port->canvasIsRotated)
					port->rotater->RotateRect(&tt_rect, &tt_rect);
#endif
				Pixels *dst = &port->canvas->pixels;
				grSwitchNonscaledBlit(context, src, dst, f_rect, tt_rect, port);
				port = port->next;
			}
			grUnlockCanvi(ueberPort);
		} else {
			grLockCanvi(ueberPort, NULL, hwBlit);
			RenderSubPort *port = ueberPort;
			while (port) {
				rect tt_rect = t_rect;
				offset_rect(&tt_rect, port->origin.h, port->origin.v);
#if ROTATE_DISPLAY
				if (port->canvasIsRotated)
					port->rotater->RotateRect(&tt_rect, &tt_rect);
#endif
				Pixels *dst = &port->canvas->pixels;
				grSwitchScaledBlit(context, src, dst, ff_rect, tt_rect, port);
				port = port->next;
			}
			grUnlockCanvi(ueberPort);
		}
	}
	grUnlockPort(context);
}



