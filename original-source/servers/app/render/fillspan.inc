
#undef WritePixel
#undef WritePixelSpan
#undef DrawPixel
#undef DrawPixelAtXY
#undef DrawPixelSpan
#undef DeclareModeSpecificVars
#undef DeclarePatternVars
#undef DeclareVars
#undef PointInRect
#undef PixSize
#undef PerPixel
#undef PerScanLine
#undef BlendForePix
#undef BlendBackPix
#undef AlphaOp
#undef BlendVarDeclarations

#if (DestBits == 8)
	#define DestAlignment 1
#elif (DestBits == 32)
	#define DestAlignment 4
#else
	#define DestAlignment 2
#endif

#undef SourceFormatBits
#undef SourceFormatEndianess
#undef DestFormatBits
#undef DestFormatEndianess

#define SourceFormatBits 32
#define SourceFormatEndianess HostEndianess
#define DestFormatBits DestBits
#define DestFormatEndianess DestEndianess

#include "pixelConvert.inc"

#if (AlphaFunction == ALPHA_FUNCTION_COMPOSITE)
	#define AlphaOp(a,b,c)	AlphaBlend(a,b,c)
	#define BlendVarDeclarations uint32 dstAlpha,srcFactor,dstFactor;
	#define BlendForePix context->foreColorARGB
	#define BlendBackPix context->backColorARGB
#else
	#define AlphaOp(a,b,c)	HalfAlphaBlend(a,b,c)
	#define BlendVarDeclarations
	#define BlendForePix cache->foreAlphaPix
	#define BlendBackPix cache->backAlphaPix
#endif

#if (DestAlignment==1)
	#define PixSize 0
	#if (RenderMode == DEFINE_OP_COPY) || (RenderMode == DEFINE_OP_OVER) || (RenderMode == DEFINE_OP_ERASE)
		#define WritePixel(pixelLocation) *(pixelLocation) = pixel;
		#define WritePixelSpan(pixelLocation,count) 										\
			while ((count > 0) && ((uint32)pixelLocation)&0x3) { 							\
				*(pixelLocation++) = pixel; pixelLocation++; count--; 						\
			}; 																				\
			while (count >= 4) { 															\
				*((uint32*)(pixelLocation)) = longPixel; 									\
				count -= 4;  pixelLocation += 4; 											\
			}; 																				\
			while (count-- > 0) { 															\
				*(pixelLocation++) = pixel; 												\
			};
	#endif
	#if (RenderMode==DEFINE_OP_COPY)
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 												\
				uint32 bothPixels[2]; 														\
				bothPixels[0] = cache->backColorCanvasFormat;								\
				bothPixels[1] = cache->foreColorCanvasFormat; 								\
				uint8 pixel;		
		#else
			#define DeclareModeSpecificVars 												\
				uint32 longPixel = *(&cache->foreColorCanvasFormat + context->solidColor);	\
				uint8 pixel = longPixel & 0x000000FF;        		
		#endif
	#elif ((RenderMode==DEFINE_OP_OVER) || (RenderMode==DEFINE_OP_ERASE))
		#define DeclareModeSpecificVars 													\
			uint32 longPixel = 																\
			((context->drawOp==OP_OVER)?cache->foreColorCanvasFormat 						\
									 :cache->backColorCanvasFormat) & 0x000000FF; 			\
			uint8 pixel = longPixel & 0x000000FF;
	#elif (RenderMode==DEFINE_OP_INVERT)
		#define WritePixel(pixelLocation) 													\
			*(pixelLocation) = invertMap[*(pixelLocation)];											
		#define WritePixelSpan(pixelLocation,count) 										\
			while ((count > 0) && ((uint32)pixelLocation)&0x3) { 							\
				*(pixelLocation) = invertMap[*(pixelLocation)]; 							\
				pixelLocation++; count--;													\
			}; 																				\
			while (count >= 4) { 															\
				longPixel = *((uint32*)(pixelLocation)); 									\
				*((uint32*)(pixelLocation)) = 	(invertMap[longPixel>>24]<<24) | 			\
												(invertMap[(longPixel>>16)&0xFF]<<16) | 	\
												(invertMap[(longPixel>>8)&0xFF]<<8) | 		\
												(invertMap[longPixel&0xFF]); 				\
				count -= 4;  pixelLocation += 4; 											\
			}; 																				\
			while (count-- > 0) { 															\
				*(pixelLocation) = invertMap[*(pixelLocation)]; 							\
				pixelLocation++; 															\
			};
		#define DeclareModeSpecificVars 													\
			uint8 *invertMap = canvas->pixels.colorMap->invert_map; 						\
			uint32 longPixel;
	#elif (RenderMode==DEFINE_OP_BLEND)
	    #define WritePixel(pixelLocation) 													\
			dstPixel = *((uint8*)(pixelLocation));											\
			AlphaOp(srcPixel,dstPixel,srcAlpha);											\
			*((uint8*)(pixelLocation)) = dstPixel;
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				uint32 bothPixels[2]; 															\
				uint32 bothAlphas[2]; 															\
				bothPixels[0] = BlendBackPix; 													\
				bothPixels[1] = BlendForePix; 													\
				bothAlphas[0] = cache->backSrcAlpha; 											\
				bothAlphas[1] = cache->foreSrcAlpha; 											\
				uint32 *index2ARGB = (uint32*)canvas->pixels.colorMap->index2ARGB;				\
				uint8 *RGB2Index = (uint8*)canvas->pixels.colorMap->inverted;					\
				uint32 tmpVar1,dstPixel,srcAlpha,srcPixel;
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					srcPixel = bothPixels[pat & 1];												\
					srcAlpha = bothAlphas[pat & 1];												\
					WritePixel(pixelLocation); 													\
					pixelLocation++; 															\
					PerPixel; 																	\
				};
		#else
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					WritePixel(pixelLocation); 													\
					pixelLocation++; 															\
					PerPixel;																	\
				};
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				uint32 *index2ARGB = (uint32*)canvas->pixels.colorMap->index2ARGB;				\
				uint8 *RGB2Index = (uint8*)canvas->pixels.colorMap->inverted;					\
				uint32 tmpVar1,dstPixel;														\
				uint32 srcAlpha = cache->foreSrcAlpha;											\
				uint32 srcPixel = BlendForePix;													
		#endif
	#else /* (RenderMode == OP_FUNCTION) */
	    #define WritePixel(pixelLocation) 														\
			*((uint8*)(pixelLocation)) = colorOp(pixel,*((uint8*)(pixelLocation)),context, port); 
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 bothPixels[2]; 															\
				bothPixels[0] = context->backColorARGB; 										\
				bothPixels[1] = context->foreColorARGB; 										\
				uint32 pixel; 															
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					pixel = bothPixels[pat & 1];												\
					WritePixel(pixelLocation); 													\
					pixelLocation++; 															\
					PerPixel; 																	\
				};
		#else
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					WritePixel(pixelLocation); 													\
					pixelLocation++; 															\
					PerPixel;																	\
				};
			#define DeclareModeSpecificVars 													\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 pixel = context->foreColorARGB; 							
		#endif
	#endif
#elif (DestAlignment==2)
	#define PixSize 1
	#if (RenderMode == DEFINE_OP_COPY) || (RenderMode == DEFINE_OP_OVER) || (RenderMode == DEFINE_OP_ERASE)
		#define WritePixel(pixelLocation) 														\
			*((uint16*)pixelLocation) = pixel; 													
		#define WritePixelSpan(pixelLocation,count) 											\
			if ((count>0) && (((uint32)pixelLocation)&0x03)) { 									\
				*((uint16*)pixelLocation) = pixel;  pixelLocation+=2; 							\
			};																					\
			while (count>=2) {																	\
				*((uint32*)pixelLocation) = pixel;  pixelLocation+=4; count-=2; 				\
			};																					\
			if (count>0) { 																		\
				*((uint16*)pixelLocation) = pixel;												\
			};
	#endif
	#if (RenderMode == DEFINE_OP_COPY)
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				uint32 bothPixels[2]; 															\
				bothPixels[0] = cache->backColorCanvasFormat; 									\
				bothPixels[1] = cache->foreColorCanvasFormat; 									\
				uint32 pixel;
		#else
			#define DeclareModeSpecificVars 													\
				uint32 pixel = *(&cache->foreColorCanvasFormat + context->solidColor);
		#endif
	#elif ((RenderMode == DEFINE_OP_OVER) || (RenderMode == DEFINE_OP_ERASE))
		#define DeclareModeSpecificVars 														\
			uint32 pixel = 																		\
				((context->drawOp==OP_OVER)?cache->foreColorCanvasFormat 						\
										 :cache->backColorCanvasFormat);
	#elif (RenderMode == DEFINE_OP_INVERT)
		#define WritePixel(pixelLocation) 														\
			*((uint16*)(pixelLocation)) = invert - *((uint16*)(pixelLocation)); 				
		#define WritePixelSpan(pixelLocation,count) 											\
			while ((count>0) && (((uint32)pixelLocation)&0x03)) { 								\
				*((uint16*)(pixelLocation)) = invert - *((uint16*)(pixelLocation)); 			\
				pixelLocation += 2; count--;													\
			};																					\
			while (count>=8) { 																	\
				tmp1 = *((uint32*)(pixelLocation));												\
				tmp2 = *((uint32*)(pixelLocation+4));											\
				tmp3 = *((uint32*)(pixelLocation+8));											\
				tmp4 = *((uint32*)(pixelLocation+12));											\
				*((uint32*)(pixelLocation)) = invert - tmp1; 									\
				*((uint32*)(pixelLocation+4)) = invert - tmp2; 									\
				*((uint32*)(pixelLocation+8)) = invert - tmp3; 									\
				*((uint32*)(pixelLocation+12)) = invert - tmp4; 								\
				pixelLocation += 16; 															\
				count -= 8;																		\
			};																					\
			while (count>=2) { 																	\
				*((uint32*)(pixelLocation)) = invert - *((uint32*)(pixelLocation)); 			\
				pixelLocation += 4;	count-=2;													\
			};																					\
			while (count--) { 																	\
				*((uint16*)(pixelLocation)) = invert - *((uint16*)(pixelLocation)); 			\
				pixelLocation += 2;																\
			};																					
		#define DeclareModeSpecificVars 														\
			uint32 invert = 0xFFFFFFFF;															\
			uint32 tmp1,tmp2,tmp3,tmp4;
	#elif (RenderMode==DEFINE_OP_BLEND)
	    #define WritePixel(pixelLocation) 														\
			dstPixel = *((uint16*)(pixelLocation));												\
			AlphaOp(srcPixel,dstPixel,srcAlpha);												\
			*((uint16*)(pixelLocation)) = dstPixel;
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 bothPixels[2]; 															\
				uint32 bothAlphas[2]; 															\
				bothPixels[0] = BlendBackPix; 													\
				bothPixels[1] = BlendForePix; 													\
				bothAlphas[0] = cache->backSrcAlpha; 											\
				bothAlphas[1] = cache->foreSrcAlpha; 											\
				uint32 tmpVar1,dstPixel,srcAlpha,srcPixel;
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					srcPixel = bothPixels[pat & 1];												\
					srcAlpha = bothAlphas[pat & 1];												\
					WritePixel(pixelLocation); 													\
					pixelLocation+=2; 															\
					PerPixel; 																	\
				};
		#else
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					WritePixel(pixelLocation); 													\
					pixelLocation+=2; 															\
					PerPixel;																	\
				};
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 tmpVar1,dstPixel;														\
				uint32 srcAlpha = cache->foreSrcAlpha;											\
				uint32 srcPixel = BlendForePix;
		#endif
	#else
		#define WritePixel(pixelLocation) 														\
			*((uint16*)(pixelLocation)) = colorOp(pixel,*((uint16*)(pixelLocation)),context, port); 	
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 bothPixels[2]; 															\
				bothPixels[0] = context->backColorARGB; 										\
				bothPixels[1] = context->foreColorARGB; 										\
				uint32 pixel;
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					if (pat & 1) WritePixel(pixelLocation); 									\
					pixelLocation += 2; 														\
					PerPixel; 																	\
				};
		#else
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					WritePixel(pixelLocation); 													\
					pixelLocation += 2; 														\
					PerPixel; 																	\
				};
			#define DeclareModeSpecificVars 													\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 pixel = context->foreColorARGB; 							
		#endif
	#endif	
#elif (DestAlignment==4)
	#define PixSize 2
	#if (RenderMode == DEFINE_OP_COPY) || (RenderMode == DEFINE_OP_OVER) || (RenderMode == DEFINE_OP_ERASE)	
		#define WritePixel(pixelLocation) 													\
			*((uint32*)pixelLocation) = pixel; 												
		#define WritePixelSpan(pixelLocation,count) 										\
			while (count--) { 																\
				*((uint32*)pixelLocation) = pixel;  pixelLocation+=4; 						\
			};
	#endif
	#if (RenderMode == DEFINE_OP_COPY)
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 												\
				uint32 bothPixels[2]; 														\
				bothPixels[0] = cache->backColorCanvasFormat; 								\
				bothPixels[1] = cache->foreColorCanvasFormat; 								\
				uint32 pixel;		
		#else		
			#define DeclareModeSpecificVars 												\
				uint32 pixel = *(&cache->foreColorCanvasFormat + context->solidColor);
		#endif
	#elif ((RenderMode == DEFINE_OP_OVER) || (RenderMode == DEFINE_OP_ERASE))	
		#define DeclareModeSpecificVars 													\
			uint32 pixel = 																	\
			((context->drawOp==OP_OVER)?cache->foreColorCanvasFormat 						\
			:cache->backColorCanvasFormat);	
	#elif (RenderMode == DEFINE_OP_INVERT)
		#define WritePixel(pixelLocation) 													\
			*((uint32*)(pixelLocation)) = invert - *((uint32*)(pixelLocation)); 			
		#define WritePixelSpan(pixelLocation,count) 										\
			while (count>=4) { 																\
				tmp1 = *((uint32*)(pixelLocation));											\
				tmp2 = *((uint32*)(pixelLocation+4));										\
				tmp3 = *((uint32*)(pixelLocation+8));										\
				tmp4 = *((uint32*)(pixelLocation+12));										\
				*((uint32*)(pixelLocation)) = invert - tmp1; 								\
				*((uint32*)(pixelLocation+4)) = invert - tmp2; 								\
				*((uint32*)(pixelLocation+8)) = invert - tmp3; 								\
				*((uint32*)(pixelLocation+12)) = invert - tmp4; 							\
				pixelLocation += 16; 														\
				count -= 4;																	\
			};																				\
			while (count--) { 																\
				*((uint32*)(pixelLocation)) = invert - *((uint32*)(pixelLocation)); 		\
				pixelLocation += 4; 														\
			};		
		#define DeclareModeSpecificVars 													\
			uint32 invert = 0xFFFFFFFF;														\
			uint32 tmp1,tmp2,tmp3,tmp4;
	#elif (RenderMode==DEFINE_OP_BLEND)
	    #define WritePixel(pixelLocation) 													\
			dstPixel = *((uint32*)(pixelLocation));											\
			AlphaOp(srcPixel,dstPixel,srcAlpha);											\
			*((uint32*)(pixelLocation)) = dstPixel;
		#if (RenderPattern==1)
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 bothPixels[2]; 															\
				uint32 bothAlphas[2]; 															\
				bothPixels[0] = BlendBackPix; 													\
				bothPixels[1] = BlendForePix; 													\
				bothAlphas[0] = cache->backSrcAlpha; 											\
				bothAlphas[1] = cache->foreSrcAlpha; 											\
				uint32 tmpVar1,dstPixel,srcAlpha,srcPixel;
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					srcPixel = bothPixels[pat & 1];												\
					srcAlpha = bothAlphas[pat & 1];												\
					WritePixel(pixelLocation); 													\
					pixelLocation+=4; 															\
					PerPixel; 																	\
				};
		#else
			#define WritePixelSpan(pixelLocation,count) 										\
				while ((count)--) { 															\
					WritePixel(pixelLocation); 													\
					pixelLocation+=4; 															\
					PerPixel;																	\
				};
			#define DeclareModeSpecificVars 													\
				BlendVarDeclarations;															\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;									\
				uint32 tmpVar1,dstPixel;														\
				uint32 srcAlpha = cache->foreSrcAlpha;											\
				uint32 srcPixel = BlendForePix;
		#endif
	#else
		#define WritePixel(pixelLocation) 													\
			*((uint32*)(pixelLocation)) = colorOp(pixel,*((uint32*)(pixelLocation)),context, port);
		#if (RenderPattern==1)		
			#define DeclareModeSpecificVars 												\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;								\
				uint32 bothPixels[2];														\
				bothPixels[0] = context->backColorARGB; 									\
				bothPixels[1] = context->foreColorARGB; 									\
				uint32 pixel;
			#define WritePixelSpan(pixelLocation,count) 									\
				while ((count)--) { 														\
					if (pat & 1) WritePixel(pixelLocation); 								\
					pixelLocation += 4;														\
					PerPixel; 																\
				};		
		#else
			#define WritePixelSpan(pixelLocation,count) 									\
				while ((count)--) { 														\
					WritePixel(pixelLocation); 												\
					pixelLocation += (1 << PixSize);										\
					PerPixel; 																\
				};
			#define DeclareModeSpecificVars 												\
				ColorOpFunc colorOp = cache->fColorOpTransFunc;								\
				uint32 pixel = context->foreColorARGB; 							
		#endif
	#endif
#endif

#define WritePixelAndAdvance(pixelLocation) \
	WritePixel(pixelLocation); pixelLocation += (1 << PixSize);

#if (RenderPattern==1)
	#define DeclarePatternVars 																\
		int32 phaseX = port->phase_x; 													\
		int32 phaseY = port->phase_y; 													\
		uint8 pat,*pats = context->stipplePattern.data;
#else
	#define DeclarePatternVars
#endif

#define DeclareVars \
	DeclareModeSpecificVars \
	DeclarePatternVars

#define PointInRect(x,y) 																\
	((x >= rleft) && 																	\
	 (x <= rright) &&																	\
	 (y >= rtop) &&																	\
	 (y <= rbottom))

#define top_code 	0x01
#define bottom_code	0x02
#define left_code	0x04
#define right_code	0x08

#if (RenderPattern==1)
	#define PerPixel pat = (pat>>7)|(pat<<1);
	#define PerScanLine(x,y) 																\
	  	pat = pats[(phaseY+y)&0x07]; 														\
  		pat = (pat >> (7-((x+phaseX)&0x07))) | (pat << (8-(7-((x+phaseX)&0x07))));
	#if ((RenderMode==DEFINE_OP_OVER) || (RenderMode==DEFINE_OP_ERASE) || (RenderMode==DEFINE_OP_INVERT))
		#define DrawPixel(pixelLocation) 													\
			if (pat & 0x80) WritePixel(pixelLocation); 										\
			pixelLocation += (1<<PixSize);
		#define DrawPixelSpan(pixelLocation,count) 											\
			while ((count)--) { 															\
				if (pat & 0x80) WritePixel(pixelLocation); 									\
				pixelLocation+=(1<<PixSize); 												\
				PerPixel; 																	\
			};
		#define DrawPixelAtXY(pixelLocation,x,y) 											\
			if ((pats[(y+phaseY)&0x07]>>(7-((x+phaseX)&0x07))) & 1) { 						\
				WritePixel(pixelLocation);													\
			};
	#elif (RenderMode==DEFINE_OP_BLEND)
		#define DrawPixel(pixelLocation) 													\
			srcPixel = (pat & 0x80)>>7; 													\
			srcAlpha = bothAlphas[srcPixel]; 												\
			srcPixel = bothPixels[srcPixel]; 												\
			WritePixelAndAdvance(pixelLocation);
		#define DrawPixelSpan(pixelLocation,count) 											\
			while ((count)--) { 															\
				DrawPixel(pixelLocation); 													\
				PerPixel; 																	\
			};
		#define DrawPixelAtXY(pixelLocation,x,y) 											\
			srcPixel = (pats[(y+phaseY)&0x07]>>(7-((x+phaseX)&0x07))) & 1; 					\
			srcAlpha = bothAlphas[srcPixel]; 												\
			srcPixel = bothPixels[srcPixel]; 												\
			WritePixel(pixelLocation);
	#else
		#define DrawPixel(pixelLocation) 													\
			pixel = bothPixels[(pat & 0x80)>>7]; 											\
			WritePixelAndAdvance(pixelLocation);
		#define DrawPixelSpan(pixelLocation,count) 											\
			while ((count)--) { 															\
				DrawPixel(pixelLocation); 													\
				PerPixel; 																	\
			};
		#define DrawPixelAtXY(pixelLocation,x,y) 											\
			pixel = bothPixels[(pats[(y+phaseY)&0x07]>>(7-((x+phaseX)&0x07))) & 1]; 		\
			WritePixel(pixelLocation);
	#endif
#else
	#define PerPixel
	#define PerScanLine(x,y)
	#define DrawPixel(pixelLocation) WritePixelAndAdvance(pixelLocation)
	#define DrawPixelAtXY(pixelLocation,x,y) WritePixel(pixelLocation)
	#define DrawPixelSpan(pixelLocation,count) WritePixelSpan(pixelLocation,count)
#endif

#ifdef FillRectsFunctionName

static void FillRectsFunctionName
(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount
)
{
	grAssertLocked(context, port);

	RenderCache *cache = port->cache;
	RenderCanvas *canvas = port->canvas;
	region *clipRegion = port->RealDrawingRegion();
	int32	rowBytes=port->canvas->pixels.bytesPerRow,rc;
	uint8	*p,*base=((uint8*)port->canvas->pixels.pixelData);
	int32	i,width,height,pixelInc,x1,x2,y1,y2;
	rect	*clipRect;
	uint16 	mode = context->drawOp;
	int32	clips;
	const rect	*curClipRect;
	point offset;
#if ROTATE_DISPLAY
	if (port->canvasIsRotated) {
		if (!context->renderPort->canvasIsRotated)
			return; // should not happen, but bullet proffing is better (hey, it's the app_server!)
		// the shape was rotated with another rotater! We must compensate.
		const int32 dh = port->rotater->mirrorIValue - context->renderPort->rotater->mirrorIValue;
		offset.h = dh + port->rotater->RotateDV(port->origin.v);
		offset.v = port->rotater->RotateDH(port->origin.h);
	} else
#endif
	{
		offset = port->origin;
	}
	DeclareVars;
	
	while (rectCount) {
		curClipRect = clipRegion->Rects();
		clips = clipRegion->CountRects();
		while (clips) {
			x1 = fillRects->left + offset.h;
			x2 = fillRects->right + offset.h;
			y1 = fillRects->top + offset.v;
			y2 = fillRects->bottom + offset.v;
			if (curClipRect->top > y1) y1 = curClipRect->top;
			if (curClipRect->bottom < y2) y2 = curClipRect->bottom;
			if (curClipRect->left > x1) x1 = curClipRect->left;
			if (curClipRect->right < x2) x2 = curClipRect->right;
			if ((y1 <= y2) && (x1 <= x2)) {
				height = (y2-y1+1);
				width = (x2-x1+1);
				pixelInc = rowBytes - (width<<PixSize);
				p = base+(y1*rowBytes)+(x1<<PixSize);
				while (y1<=y2) {
					i = width;
					PerScanLine(x1,y1);
					DrawPixelSpan(p,i);
					y1++; p += pixelInc;
				};
			};
			clips--;
			curClipRect++;
		};
		rectCount--;
		fillRects++;
	};
};

#endif

#ifdef DrawOneLineFunctionName

static void DrawOneLineFunctionName
(
 	RenderContext *context,
	RenderSubPort *port,
 	uint8 *base, int32 bpr,
	int32 numPixels,
	int32 x, int32 y,
	int32 xInc, int32 yInc
)
{
	RenderCache *cache = port->cache;
	RenderCanvas *canvas = port->canvas;
	uint8 *p;
	DeclareVars;

	grAssertLocked(context, port);
	
	while (numPixels--) {
		p = base + (bpr*(y>>16)) + ((x>>16)<<PixSize);
		DrawPixelAtXY(p,(x>>16),(y>>16));
		x += xInc; y += yInc;
	};
};

#endif

#undef DrawOneLineFunctionName
#undef FillRectsFunctionName
