
/*	This mess is an exhaustive set of conversions from source to
	destination pixel formats, in all host, source, and destination 
	endianesses. */

#undef SrcPixelIsTransparent
#undef PixelConversion
#undef DoSlowCopyScanLine
#undef DoFastCopyScanLine
#undef DoCopyScanLine
#undef DoScaledCopyScanLine
#undef DoScaledScanLine
#undef DoOverScanLine
#undef DoScanLine
#undef SourceAlignment
#undef DestAlignment
#undef SourceType
#undef DestType
#undef DestPixelDeclarations
#undef WriteGatherDeclarations
#undef CMapDeclarations
#undef dstPixel
#undef ObtainAlpha
#undef PixMult
#undef SrcToARGB
#undef DstToARGB
#undef ARGBToDst
#undef DoScanLineBackwards
#undef DoFastCopyScanLineBackwards
#undef DoCopyScanLineBackwards
#undef DoBlendScanLine
#undef DoScaledBlendScanLine
#undef TmpVarDeclarations
#undef StepSource

#if (SourceBits == 8)
	#define SourceAlignment 1
	#define SourceType uint8
#elif (SourceBits == 32)
	#define SourceAlignment 4
	#define SourceType uint32
#elif (SourceBits == 1)
	#define SourceAlignment 0
	#define SourceType uint8
#else
	#define SourceAlignment 2
	#define SourceType uint16
#endif

#if (DestBits == 8)
	#define DestAlignment 1
#elif (DestBits == 32)
	#define DestAlignment 4
#elif (DestBits == 1)
	#define DestAlignment 0
#else
	#define DestAlignment 2
#endif

/* Macros for deciding whether a source pixel is transparent */
#if (SourceBits == 8)
  #define SrcPixelIsTransparent(pix) (pix == TRANSPARENT_MAGIC_8BIT)
#elif (SourceBits == 16)
  #define SrcPixelIsTransparent(pix) 0
#endif

#if (SourceEndianess == HostEndianess)
	#if (SourceBits == 15)
		#define SrcPixelIsTransparent(pix) (pix == TRANSPARENT_MAGIC_15BIT)
	#elif (SourceBits == 32)
		#define SrcPixelIsTransparent(pix) (pix == TRANSPARENT_MAGIC_32BIT)
	#endif
#else /* (SourceEndianess != HostEndianess) */
	#if (SourceBits == 15)
		#define SrcPixelIsTransparent(pix) (pix == TRANSPARENT_MAGIC_15BIT_SWAPPED)
	#elif (SourceBits == 32)
		#define SrcPixelIsTransparent(pix) (pix == TRANSPARENT_MAGIC_32BIT_SWAPPED)
	#endif
#endif

#undef SourceFormatBits
#undef SourceFormatEndianess
#undef DestFormatBits
#undef DestFormatEndianess

#define SourceFormatBits		SourceBits
#define SourceFormatEndianess	SourceEndianess
#define DestFormatBits			DestBits
#define DestFormatEndianess		DestEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define PixelConversion(a,b) ConvertPixelFormat(a,b)
#else
	#define PixelConversion(a,b) b=a
#endif

/*

	These macros expect:

		Provided:
			uint8 *srcPtr;
			uint8 *dstPtr;
			int32 count;
		
		Implicit:
			uint32 srcPixel,dstPixel,srcTransaction,dstTransaction;

*/

#if (DestAlignment == 1)
	#define DestType uint8
#elif (DestAlignment == 2)
	#define DestType uint16
#else
	#define DestType uint32
#endif

#if (SourceAlpha == SOURCE_ALPHA_CONSTANT)
	#define ObtainAlpha(a) a = (SrcPixelIsTransparent(srcPixel)?0:cache->foreSrcAlpha);
#else
	#define ObtainAlpha(a) { (a) = SrcAlpha(srcPixel);  (a) += (a)>>7; }
#endif

#if BlitScaling

	#define DoScaledOverScanLine														\
		{																				\
			while (count>0) {															\
				srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
				if (!SrcPixelIsTransparent(srcPixel)) {									\
					PixelConversion(srcPixel,dstPixel);									\
					*((DestType*)dstPtr) = dstPixel;									\
				};																		\
				fixedX += incX;															\
				dstPtr += DestAlignment; count--;										\
			};																			\
		}
	
	#if (SourceBits == 32) || (SourceAlpha == SOURCE_ALPHA_CONSTANT)
		#define DoScaledBlendScanLine														\
			{																				\
				while (count>0) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					ObtainAlpha(srcAlpha);													\
					if (srcAlpha == 256) {													\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					} else if (srcAlpha) {													\
						dstPixel = *((DestType*)dstPtr);									\
						AlphaBlend(srcPixel,dstPixel,srcAlpha);								\
						*((DestType*)dstPtr) = dstPixel;									\
					}																		\
					dstPtr+=DestAlignment; fixedX += incX; count--;							\
				};																			\
			}
	#elif (SourceBits == 15)
		#define DoScaledBlendScanLine														\
			{																				\
				while (count>0) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					if (SrcAlphaBit(srcPixel)) {											\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					fixedX += incX;															\
					dstPtr += DestAlignment; count--;										\
				};																			\
			}
	#endif
#endif

#if (SourceBits == 15) && (SourceAlpha == SOURCE_ALPHA_PIXEL)
	#define DoBlendScanLine																\
		{																				\
			while (count>0) {															\
				srcPixel = *((SourceType*)srcPtr);										\
				if (SrcAlphaBit(srcPixel)) {											\
					PixelConversion(srcPixel,dstPixel);									\
					*((DestType*)dstPtr) = dstPixel;									\
				};																		\
				dstPtr+=sizeof(DestType); srcPtr+=StepSource; count--;					\
			};																			\
		}
#else
	#define DoBlendScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while (count>0) {															\
				srcPixel = *((SourceType*)srcPtr);										\
				ObtainAlpha(srcAlpha);													\
				if (srcAlpha == 256) {													\
					PixelConversion(srcPixel,dstPixel);									\
					*((DestType*)dstPtr) = dstPixel;									\
				} else if (srcAlpha) {													\
					dstPixel = *((DestType*)dstPtr);									\
					AlphaBlend(srcPixel,dstPixel,srcAlpha);								\
					*((DestType*)dstPtr) = dstPixel;									\
				}																		\
				dstPtr+=sizeof(DestType); srcPtr+=StepSource; count--;					\
			};																			\
		}
#endif

#ifdef RotatedBlit
	#if (SourceAlignment == 1)
		#define PixMult(a) (-(a)*srcByteWidth)
	#elif (SourceAlignment == 2)
		#define PixMult(a) (-(a)*srcByteWidth)
	#else
		#define PixMult(a) (-(a)*srcByteWidth)
	#endif
	#define StepSource	(-srcBytesPerRow)
#else
	#if (SourceAlignment == 1)
		#define PixMult(a) (a)
	#elif (SourceAlignment == 2)
		#define PixMult(a) ((a)<<1)
	#else
		#define PixMult(a) ((a)<<2)
	#endif
	#define StepSource	sizeof(SourceType)
#endif

#if BlitScaling
	#if (DestAlignment == 1)
		#define DoScaledCopyScanLine														\
			{																				\
				while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					*(dstPtr++) = dstPixel;													\
					fixedX += incX;															\
					count--;																\
				};																			\
				while (count>=4) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction = ShiftToByte0(dstPixel);								\
					fixedX += incX;															\
																							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction |= ShiftToByte1(dstPixel);								\
					fixedX += incX;															\
																							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction |= ShiftToByte2(dstPixel);								\
					fixedX += incX;															\
																							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction |= ShiftToByte3(dstPixel);								\
					fixedX += incX;															\
																							\
					*((uint32*)dstPtr) = dstTransaction;									\
					dstPtr += 4;															\
					count-=4;																\
				};																			\
				while (count>0) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					*(dstPtr++) = dstPixel;													\
					fixedX += incX;															\
					count--;																\
				};																			\
			}
	#elif (DestAlignment == 2)
		#define DoScaledCopyScanLine														\
			{																				\
				while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					fixedX += incX;															\
					dstPtr += 2;															\
					count--;																\
				};																			\
				while (count>=2) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction = ShiftToWord0(dstPixel);								\
					fixedX += incX;															\
																							\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					dstTransaction |= ShiftToWord1(dstPixel);								\
					fixedX += incX;															\
																							\
					*((uint32*)dstPtr) = dstTransaction;									\
					dstPtr += 4;															\
					count -= 2;																\
				};																			\
				while (count>0) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					fixedX += incX;															\
					dstPtr += 2;															\
					count--;																\
				};																			\
			}
	#elif (DestAlignment == 4)
		#define DoScaledCopyScanLine														\
			{																				\
				while (count>0) {															\
					srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));					\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint32*)dstPtr) = dstPixel;											\
					fixedX += incX;															\
					dstPtr += 4;															\
					count--;																\
				};																			\
			};
	#endif
#endif

#if (SourceAlignment == 1) && (DestAlignment == 1)
	#define DoOverScanLine(srcPtr,dstPtr,count)											\
		{																				\
			uint32 magicConstant = 0x81010101;											\
			while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
				srcPixel = *(srcPtr++);													\
				if (srcPixel != 0xFF) {													\
					*dstPtr = srcPixel;													\
				};																		\
				dstPtr++; count--;														\
			};																			\
			while (count>=8) {															\
				srcPixel = *((uint32*)srcPtr);											\
				srcTransaction = *((uint32*)(srcPtr+4));								\
				if (!((																	\
					~(	((srcTransaction + magicConstant) ^ srcTransaction) &			\
						((srcPixel       + magicConstant) ^ srcPixel      ) )			\
						) & magicConstant)) {											\
					*((uint32*)dstPtr) = srcPixel;										\
					*((uint32*)(dstPtr+4)) = srcTransaction;							\
					dstPtr += 8; srcPtr += 8; count -= 8;								\
					continue;															\
				} else if (!(~(srcPixel & srcTransaction))) {							\
					dstPtr += 8; srcPtr += 8; count -= 8;								\
					continue;															\
				} else {																\
					dstTransaction = ShiftMaskByte0(srcPixel);							\
					if (dstTransaction != 0xFF) *(dstPtr) = dstTransaction;				\
					dstTransaction = ShiftMaskByte1(srcPixel);							\
					if (dstTransaction != 0xFF) *(dstPtr+1) = dstTransaction;			\
					dstTransaction = ShiftMaskByte2(srcPixel);							\
					if (dstTransaction != 0xFF) *(dstPtr+2) = dstTransaction;			\
					dstTransaction = ShiftMaskByte3(srcPixel);							\
					if (dstTransaction != 0xFF) *(dstPtr+3) = dstTransaction;			\
					dstTransaction = ShiftMaskByte0(srcTransaction);					\
					if (dstTransaction != 0xFF) *(dstPtr+4) = dstTransaction;			\
					dstTransaction = ShiftMaskByte1(srcTransaction);					\
					if (dstTransaction != 0xFF) *(dstPtr+5) = dstTransaction;			\
					dstTransaction = ShiftMaskByte2(srcTransaction);					\
					if (dstTransaction != 0xFF) *(dstPtr+6) = dstTransaction;			\
					dstTransaction = ShiftMaskByte3(srcTransaction);					\
					if (dstTransaction != 0xFF) *(dstPtr+7) = dstTransaction;			\
					dstPtr += 8; srcPtr += 8; count -= 8;								\
					continue;															\
				};																		\
			};																			\
			while (count>0) {															\
				srcPixel = *(srcPtr++);													\
				if (srcPixel != 0xFF) {													\
					*dstPtr = srcPixel;													\
				};																		\
				dstPtr++; count--;														\
			};																			\
		}
#elif (SourceAlignment == 1)
	#ifdef RotatedBlit
		#define DoOverScanLine(srcPtr,dstPtr,count)											\
			{																				\
				while (count>0) {															\
					srcPixel = *srcPtr;														\
					srcPtr += StepSource;													\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					dstPtr += sizeof(DestType); count--;									\
				};																			\
			}
	#else
		#define DoOverScanLine(srcPtr,dstPtr,count)											\
			{																				\
				uint32 magicConstant = 0x81010101;											\
				while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
					srcPixel = *(srcPtr++);													\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					dstPtr += sizeof(DestType); count--;									\
				};																			\
				while (count>=8) {															\
					srcPixel = *((uint32*)srcPtr);											\
					srcTransaction = *((uint32*)(srcPtr+4));								\
					if (!((																	\
						~(	((srcTransaction + magicConstant) ^ srcTransaction) &			\
							((srcPixel       + magicConstant) ^ srcPixel      ) )			\
							) & magicConstant)) {											\
						PixelConversion(ShiftMaskByte0(srcPixel),dstPixel);					\
						*((DestType*)dstPtr) = dstPixel;									\
						PixelConversion(ShiftMaskByte1(srcPixel),dstPixel);					\
						*((DestType*)(dstPtr+DestAlignment)) = dstPixel;					\
						PixelConversion(ShiftMaskByte2(srcPixel),dstPixel);					\
						*((DestType*)(dstPtr+(2*DestAlignment))) = dstPixel;				\
						PixelConversion(ShiftMaskByte3(srcPixel),dstPixel);					\
						*((DestType*)(dstPtr+(3*DestAlignment))) = dstPixel;				\
						PixelConversion(ShiftMaskByte0(srcTransaction),dstPixel);			\
						*((DestType*)(dstPtr+(4*DestAlignment))) = dstPixel;				\
						PixelConversion(ShiftMaskByte1(srcTransaction),dstPixel);			\
						*((DestType*)(dstPtr+(5*DestAlignment))) = dstPixel;				\
						PixelConversion(ShiftMaskByte2(srcTransaction),dstPixel);			\
						*((DestType*)(dstPtr+(6*DestAlignment))) = dstPixel;				\
						PixelConversion(ShiftMaskByte3(srcTransaction),dstPixel);			\
						*((DestType*)(dstPtr+(7*DestAlignment))) = dstPixel;				\
						dstPtr += 8*DestAlignment; srcPtr += 8; count -= 8;					\
						continue;															\
					} else if (!(~(srcPixel & srcTransaction))) {							\
						dstPtr += 8*DestAlignment; srcPtr += 8; count -= 8;					\
						continue;															\
					} else {																\
						dstTransaction = ShiftMaskByte0(srcPixel);							\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)dstPtr) = dstPixel;								\
						};																	\
						dstTransaction = ShiftMaskByte1(srcPixel);							\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+DestAlignment)) = dstPixel;				\
						};																	\
						dstTransaction = ShiftMaskByte2(srcPixel);							\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(2*DestAlignment))) = dstPixel;			\
						};																	\
						dstTransaction = ShiftMaskByte3(srcPixel);							\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(3*DestAlignment))) = dstPixel;			\
						};																	\
						dstTransaction = ShiftMaskByte0(srcTransaction);					\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(4*DestAlignment))) = dstPixel;			\
						};																	\
						dstTransaction = ShiftMaskByte1(srcTransaction);					\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(5*DestAlignment))) = dstPixel;			\
						};																	\
						dstTransaction = ShiftMaskByte2(srcTransaction);					\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(6*DestAlignment))) = dstPixel;			\
						};																	\
						dstTransaction = ShiftMaskByte3(srcTransaction);					\
						if (dstTransaction != 0xFF) {										\
							PixelConversion(dstTransaction,dstPixel);						\
							*((DestType*)(dstPtr+(7*DestAlignment))) = dstPixel;			\
						};																	\
						srcPtr+=8; dstPtr+=(8*DestAlignment); count-=8;						\
						continue;															\
					};																		\
				};																			\
				while (count>0) {															\
					srcPixel = *(srcPtr++);													\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					dstPtr += sizeof(DestType); count--;									\
				};																			\
			}
	#endif
#elif (SourceAlignment == 2)
	#ifdef RotatedBlit
		#define DoOverScanLine(srcPtr,dstPtr,count)											\
			{																				\
				while (count>0) {															\
					srcPixel = *((uint16*)srcPtr);											\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					srcPtr+=StepSource; dstPtr += sizeof(DestType); count--;				\
				};																			\
			}
	#else
		#define DoOverScanLine(srcPtr,dstPtr,count)											\
			{																				\
				while ((count>0) && (((uint32)srcPtr) & 0x03)) {							\
					srcPixel = *((uint16*)srcPtr);											\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					srcPtr+=2; dstPtr += sizeof(DestType); count--;							\
				};																			\
				while (count>=2) {															\
					srcTransaction = *((uint32*)srcPtr);									\
					srcPixel = ShiftMaskWord0(srcTransaction);								\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					srcPixel = ShiftMaskWord1(srcTransaction);								\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)(dstPtr+DestAlignment)) = dstPixel;					\
					};																		\
					srcPtr+=4; dstPtr+=(2*DestAlignment); count-=2;							\
				};																			\
				while (count>0) {															\
					srcPixel = *((uint16*)srcPtr);											\
					if (!SrcPixelIsTransparent(srcPixel)) {									\
						PixelConversion(srcPixel,dstPixel);									\
						*((DestType*)dstPtr) = dstPixel;									\
					};																		\
					srcPtr+=2; dstPtr += sizeof(DestType); count--;							\
				};																			\
			}
	#endif
#elif (SourceAlignment == 4)

	#define DoOverScanLine(srcPtr,dstPtr,count)											\
		{																				\
			while (count>0) {															\
				srcPixel = *((uint32*)srcPtr);											\
				if (!SrcPixelIsTransparent(srcPixel)) {									\
					PixelConversion(srcPixel,dstPixel);									\
					*((DestType*)dstPtr) = dstPixel;									\
				};																		\
				srcPtr+=StepSource; dstPtr += sizeof(DestType); count--;							\
			};																			\
		};

#endif

#if (HostEndianess == BIG_ENDIAN)
	#if (SourceBits == 1)
		#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
			{																				\
				DestType dstColor[2];														\
				dstColor[1] = (DestType)cache->foreColorCanvasFormat;						\
				dstColor[0] = (DestType)cache->backColorCanvasFormat;						\
				if (srcOffs) {																\
					srcPixel = *(srcPtr++);													\
					for (int32 j=7-srcOffs;(j>=0) && (count>0);j--) {						\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
						count--; dstPtr += DestAlignment;									\
					};																		\
				};																			\
				while ((count>=8) && (((uint32)srcPtr)&0x03)) {								\
					srcPixel = *(srcPtr++);													\
					for (int32 j=7;j>=0;j--) {												\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 8;																\
				};																			\
				while (count>=32) {															\
					srcPixel = *((uint32*)srcPtr);											\
					srcPtr += 4;															\
					for (int32 j=31;j>=0;j--) {												\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 32;															\
				};																			\
				while (count>=8) {															\
					srcPixel = *(srcPtr++);													\
					for (int32 j=7;j>=0;j--) {												\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 8;																\
				};																			\
				if (count>0) {																\
					srcPixel = *srcPtr;														\
					count = 7-count;														\
					for (int32 j=7;j>count;j--) {											\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
						dstPtr += DestAlignment;											\
					};																		\
				};																			\
			}
		#define DoOverScanLine(srcPtr,dstPtr,count)											\
			{																				\
				DestType dstColor = (context->drawOp==OP_OVER) ?							\
					(DestType)cache->foreColorCanvasFormat   :								\
					(DestType)cache->backColorCanvasFormat   ;								\
				uint32 inverter = (context->drawOp==OP_OVER)?0:0xFFFFFFFF;					\
				if (srcOffs) {																\
					srcPixel = *(srcPtr++) ^ inverter;										\
					for (int32 j=7-srcOffs;(j>=0) && (count>0);j--) {						\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
						count--; dstPtr += DestAlignment;									\
					};																		\
				};																			\
				while ((count>=8) && (((uint32)srcPtr)&0x03)) {								\
					srcPixel = *(srcPtr++) ^ inverter;										\
					for (int32 j=7;j>=0;j--) {												\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 8;																\
				};																			\
				while (count>=32) {															\
					srcPixel = *((uint32*)srcPtr) ^ inverter;								\
					srcPtr += 4;															\
					for (int32 j=31;j>=0;j--) {												\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 32;															\
				};																			\
				while (count>=8) {															\
					srcPixel = *(srcPtr++) ^ inverter;										\
					for (int32 j=7;j>=0;j--) {												\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
						dstPtr += DestAlignment;											\
					};																		\
					count -= 8;																\
				};																			\
				if (count>0) {																\
					srcPixel = *srcPtr ^ inverter;											\
					count = 7-count;														\
					for (int32 j=7;j>count;j--) {											\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
						dstPtr += DestAlignment;											\
					};																		\
				};																			\
			}
	#endif
#else /* (HostEndianess == LITTLE_ENDIAN) */
	#if (SourceBits == 1)
		#ifdef RotatedBlit
			#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
				{																				\
					DestType dstColor[2];														\
					dstColor[1] = (DestType)cache->foreColorCanvasFormat;						\
					dstColor[0] = (DestType)cache->backColorCanvasFormat;						\
					int32 j = 7-srcOffs;																\
					while (count > 0) {															\
						srcPixel = *(srcPtr++);													\
						srcPtr += StepSource;													\
						*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];					\
						count--; dstPtr += DestAlignment;										\
					};																			\
				}
			#define DoOverScanLine(srcPtr,dstPtr,count)											\
				{																				\
					DestType dstColor = (context->drawOp==OP_OVER) ?							\
						(DestType)cache->foreColorCanvasFormat   :							\
						(DestType)cache->backColorCanvasFormat   ;							\
					uint32 inverter = (context->drawOp==OP_OVER)?0:0xFFFFFFFF;					\
					int32 j = 7-srcOffs;																\
					while (count > 0) {															\
						srcPixel = *(srcPtr++) ^ inverter;										\
						srcPtr += StepSource;													\
						if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;				\
						count--; dstPtr += DestAlignment;										\
					};																			\
				}
		#else
			#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
				{																				\
					DestType dstColor[2];														\
					dstColor[1] = (DestType)cache->foreColorCanvasFormat;						\
					dstColor[0] = (DestType)cache->backColorCanvasFormat;						\
					if (srcOffs) {																\
						srcPixel = *(srcPtr++);													\
						for (int32 j=7-srcOffs;(j>=0) && (count>0);j--) {						\
							*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
							count--; dstPtr += DestAlignment;									\
						};																		\
					};																			\
					while (count>=8) {															\
						srcPixel = *(srcPtr++);													\
						for (int32 j=7;j>=0;j--) {												\
							*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
							dstPtr += DestAlignment;											\
						};																		\
						count -= 8;																\
					};																			\
					if (count>0) {																\
						srcPixel = *srcPtr;														\
						count = 7-count;														\
						for (int32 j=7;j>count;j--) {											\
							*((DestType*)dstPtr) = dstColor[((srcPixel>>j)&0x01)];				\
							dstPtr += DestAlignment;											\
						};																		\
					};																			\
				}
			#define DoOverScanLine(srcPtr,dstPtr,count)											\
				{																				\
					DestType dstColor = (context->drawOp==OP_OVER) ?							\
						(DestType)cache->foreColorCanvasFormat   :								\
						(DestType)cache->backColorCanvasFormat   ;								\
					uint32 inverter = (context->drawOp==OP_OVER)?0:0xFFFFFFFF;					\
					if (srcOffs) {																\
						srcPixel = *(srcPtr++) ^ inverter;										\
						for (int32 j=7-srcOffs;(j>=0) && (count>0);j--) {						\
							if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
							count--; dstPtr += DestAlignment;									\
						};																		\
					};																			\
					while (count>=8) {															\
						srcPixel = *(srcPtr++) ^ inverter;										\
						for (int32 j=7;j>=0;j--) {												\
							if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
							dstPtr += DestAlignment;											\
						};																		\
						count -= 8;																\
					};																			\
					if (count>0) {																\
						srcPixel = *srcPtr ^ inverter;											\
						count = 7-count;														\
						for (int32 j=7;j>count;j--) {											\
							if ((srcPixel>>j)&0x01) *((DestType*)dstPtr) = dstColor;			\
							dstPtr += DestAlignment;											\
						};																		\
					};																			\
				}
		#endif
	#endif
#endif /* HostEndianess */

#if (SourceAlignment == 1) && (DestAlignment == 2)
	/* 1 byte --> 2 byte alignment */
	#ifdef RotatedBlit
		#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
			{																				\
				while (count>0) {															\
					srcPixel = *srcPtr;														\
					srcPtr += StepSource;													\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					dstPtr+=2; count--;														\
				};																			\
			}
	#else
		#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
			{																				\
				while ((count>0) && (((uint32)srcPtr) & 0x03)) {							\
					srcPixel = *(srcPtr++);													\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					dstPtr+=2; count--;														\
				};																			\
				if (count >= 4) {															\
					if (((uint32)dstPtr) & 0x03) {											\
						srcPixel = *((uint32*)srcPtr); srcPtr += 4;							\
						PixelConversion(ShiftMaskByte0(srcPixel),dstTransaction);			\
						count--;															\
						*((uint16*)dstPtr) = dstTransaction;  dstPtr+=2;					\
						while (count>=4) {													\
							PixelConversion(ShiftMaskByte1(srcPixel),dstTransaction);		\
							PixelConversion(ShiftMaskByte2(srcPixel),dstPixel);				\
							*((uint32*)dstPtr) = 	ShiftToWord0(dstTransaction) |			\
													ShiftToWord1(dstPixel);					\
							PixelConversion(ShiftMaskByte3(srcPixel),dstTransaction);		\
							srcPixel = *((uint32*)srcPtr); srcPtr += 4;						\
							PixelConversion(ShiftMaskByte0(srcPixel),dstPixel);				\
							*((uint32*)(dstPtr+4)) = 	ShiftToWord0(dstTransaction) |		\
														ShiftToWord1(dstPixel);				\
							count -= 4; dstPtr += 8;										\
						};																	\
						if (count>0) {														\
							PixelConversion(ShiftMaskByte1(srcPixel),dstPixel);				\
							*((uint16*)dstPtr) = dstPixel;									\
							dstPtr+=2; count--;												\
						};																	\
						if (count>0) {														\
							PixelConversion(ShiftMaskByte2(srcPixel),dstPixel);				\
							*((uint16*)dstPtr) = dstPixel;									\
							dstPtr+=2; count--;												\
						};																	\
						if (count>0) {														\
							PixelConversion(ShiftMaskByte3(srcPixel),dstPixel);				\
							*((uint16*)dstPtr) = dstPixel;									\
							dstPtr+=2; count--;												\
						};																	\
					} else {																\
						while (count>=4) {													\
							srcPixel = *((uint32*)srcPtr); srcPtr += 4;						\
							PixelConversion(ShiftMaskByte0(srcPixel),dstTransaction);		\
							PixelConversion(ShiftMaskByte1(srcPixel),dstPixel);				\
							*((uint32*)dstPtr) = 	ShiftToWord0(dstTransaction) |			\
													ShiftToWord1(dstPixel);					\
							PixelConversion(ShiftMaskByte2(srcPixel),dstTransaction);		\
							PixelConversion(ShiftMaskByte3(srcPixel),dstPixel);				\
							*((uint32*)(dstPtr+4)) = 	ShiftToWord0(dstTransaction) |		\
														ShiftToWord1(dstPixel);				\
							count-=4; dstPtr+=8;											\
						};																	\
						goto oneByOne;														\
					};																		\
				} else {																	\
					oneByOne:																\
					while (count>0) {														\
						srcPixel = *(srcPtr++);												\
						PixelConversion(srcPixel,dstPixel);									\
						*((uint16*)dstPtr) = dstPixel;										\
						dstPtr+=2; count--;													\
					};																		\
				};																			\
			};
	#endif
#elif (SourceAlignment == 1) && (DestAlignment == 4)
	/* 1 byte --> 4 byte alignment */
	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while ((count>0) && (((uint32)srcPtr) & 0x03)) {							\
				srcPixel = *(srcPtr++);													\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				dstPtr+=4; count--;														\
			};																			\
			while (count>=4) {															\
				srcPixel = *((uint32*)srcPtr); srcPtr += 4;								\
				PixelConversion(ShiftMaskByte0(srcPixel),dstPixel);						\
				*((uint32*)dstPtr) = dstPixel;											\
				PixelConversion(ShiftMaskByte1(srcPixel),dstPixel);						\
				*((uint32*)(dstPtr+4)) = dstPixel;										\
				PixelConversion(ShiftMaskByte2(srcPixel),dstPixel);						\
				*((uint32*)(dstPtr+8)) = dstPixel;										\
				PixelConversion(ShiftMaskByte3(srcPixel),dstPixel);						\
				*((uint32*)(dstPtr+12)) = dstPixel;										\
				count-=4; dstPtr+=16;													\
			};																			\
			while (count>0) {															\
				srcPixel = *(srcPtr++);													\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				dstPtr+=4; count--;														\
			};																			\
		};
#elif (SourceAlignment == 2) && (DestAlignment == 1)
	/* 2 byte --> 1 byte alignment */
	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
				srcPixel = *((uint16*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*(dstPtr++) = dstPixel;													\
				srcPtr+=2; count--;														\
			};																			\
			while (count>=4) {															\
				srcPixel = *((uint16*)srcPtr);											\
				PixelConversion(srcPixel,dstTransaction);								\
				dstTransaction = ShiftToByte0(dstTransaction);							\
				srcPixel = *((uint16*)(srcPtr+2));										\
				PixelConversion(srcPixel,dstPixel);										\
				dstTransaction |= ShiftToByte1(dstPixel);								\
				srcPixel = *((uint16*)(srcPtr+4));										\
				PixelConversion(srcPixel,dstPixel);										\
				dstTransaction |= ShiftToByte2(dstPixel);								\
				srcPixel = *((uint16*)(srcPtr+6));										\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = (dstTransaction|(ShiftToByte3(dstPixel)));			\
				count -= 4; dstPtr += 4; srcPtr += 8;									\
			};																			\
			while (count>0) {															\
				srcPixel = *((uint16*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*(dstPtr++) = dstPixel;													\
				srcPtr+=2; count--;														\
			};																			\
		}
#elif (SourceAlignment == 2) && (DestAlignment == 4)
	/* 2 byte --> 4 byte alignment */
	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while ((count>0) && (((uint32)srcPtr) & 0x03)) {							\
				srcPixel = *((uint16*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				srcPtr+=2; count--; dstPtr+=4;											\
			};																			\
			while (count>=2) {															\
				srcTransaction = *((uint32*)srcPtr);									\
				srcPixel = ShiftMaskWord0(srcTransaction);								\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				srcPixel = ShiftMaskWord1(srcTransaction);								\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)(dstPtr+4)) = dstPixel;										\
				count -= 2; dstPtr += 8; srcPtr += 4;									\
			};																			\
			while (count>0) {															\
				srcPixel = *((uint16*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				srcPtr+=2; count--; dstPtr+=4;											\
			};																			\
		}
#elif (SourceAlignment == 4) && (DestAlignment == 1)
	/* 4 byte --> 1 byte alignment */

	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*(dstPtr++) = dstPixel;													\
				srcPtr+=4; count--;														\
			};																			\
			while (count>=4) {															\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstTransaction);								\
				dstTransaction = ShiftToByte0(dstTransaction);							\
				srcPixel = *((uint32*)(srcPtr+4));										\
				PixelConversion(srcPixel,dstPixel);										\
				dstTransaction |= ShiftToByte1(dstPixel);								\
				srcPixel = *((uint32*)(srcPtr+8));										\
				PixelConversion(srcPixel,dstPixel);										\
				dstTransaction |= ShiftToByte2(dstPixel);								\
				srcPixel = *((uint32*)(srcPtr+12));										\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = (dstTransaction|ShiftToByte3(dstPixel));			\
				count -= 4; dstPtr += 4; srcPtr += 16;									\
			};																			\
			while (count>0) {															\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*(dstPtr++) = dstPixel;													\
				srcPtr+=4; count--;														\
			};																			\
		}

#elif (SourceAlignment == 4) && (DestAlignment == 2)
	/* 4 byte --> 2 byte alignment */

	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while ((count>0) && (((uint32)dstPtr) & 0x03)) {							\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint16*)dstPtr) = dstPixel;											\
				srcPtr+=StepSource; dstPtr+=2; count--;											\
			};																			\
			while (count>=2) {															\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstTransaction);								\
				dstTransaction = ShiftToWord0(dstTransaction);							\
				srcPixel = *((uint32*)(srcPtr+StepSource));										\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = (dstTransaction|ShiftToWord1(dstPixel));			\
				count -= 2; dstPtr += 4; srcPtr += StepSource*2;									\
			};																			\
			while (count>0) {															\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint16*)dstPtr) = dstPixel;											\
				srcPtr+=StepSource; dstPtr+=2; count--;											\
			};																			\
		}

#elif (SourceAlignment == 2) && (DestAlignment == 2)
	#ifdef RotatedBlit
		#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
			{																				\
				while (count>0) {															\
					srcPixel = *((uint16*)srcPtr);											\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					dstPtr+=2; srcPtr+=StepSource; count--;									\
				};																			\
			}
	#else
		#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
			{																				\
				while ((count>0) && (((uint32)srcPtr) & 0x03)) {							\
					srcPixel = *((uint16*)srcPtr);											\
					PixelConversion(srcPixel,dstPixel);										\
					*((uint16*)dstPtr) = dstPixel;											\
					dstPtr+=2; srcPtr+=2; count--;											\
				};																			\
				if (((uint32)dstPtr) & 0x03) {												\
					if (count>=2) {															\
						srcTransaction = *((uint32*)srcPtr);								\
						srcPixel = ShiftMaskWord0(srcTransaction);							\
						PixelConversion(srcPixel,dstPixel);									\
						*((uint16*)dstPtr) = dstPixel;										\
						count--; dstPtr+=2; srcPtr+=4;										\
						while (count>=3) {													\
							srcPixel = ShiftMaskWord1(srcTransaction);						\
							PixelConversion(srcPixel,dstTransaction);						\
							dstTransaction = ShiftToWord0(dstTransaction);					\
							srcTransaction = *((uint32*)srcPtr);							\
							srcPixel = ShiftMaskWord0(srcTransaction);						\
							PixelConversion(srcPixel,dstPixel);								\
							*((uint32*)dstPtr) = (dstTransaction|ShiftToWord1(dstPixel));	\
							count -= 2; dstPtr += 4; srcPtr += 4;							\
						};																	\
						if (count>0) {														\
							srcPixel = ShiftMaskWord1(srcTransaction);						\
							PixelConversion(srcPixel,dstPixel);								\
							*((uint16*)dstPtr) = dstPixel;									\
							dstPtr += 2; count--;											\
						};																	\
					};																		\
					goto generalCase;														\
				} else {																	\
					while (count>=2) {														\
						srcTransaction = *((uint32*)srcPtr);								\
						srcPixel = ShiftMaskWord0(srcTransaction);							\
						PixelConversion(srcPixel,dstTransaction);							\
						dstTransaction = ShiftToWord0(dstTransaction);						\
						srcPixel = ShiftMaskWord1(srcTransaction);							\
						PixelConversion(srcPixel,dstPixel);									\
						*((uint32*)dstPtr) = (dstTransaction|ShiftToWord1(dstPixel));		\
						count -= 2; dstPtr += 4; srcPtr += 4;								\
					};																		\
					generalCase:															\
					while (count>0) {														\
						srcPixel = *((uint16*)srcPtr);										\
						PixelConversion(srcPixel,dstPixel);									\
						*((uint16*)dstPtr) = dstPixel;										\
						dstPtr+=2; srcPtr+=2; count--;										\
					};																		\
				};																			\
			};
	#endif
#elif (SourceAlignment == 4) && (DestAlignment == 4)

	#define DoSlowCopyScanLine(srcPtr,dstPtr,count)										\
		{																				\
			while (count>0) {															\
				srcPixel = *((uint32*)srcPtr);											\
				PixelConversion(srcPixel,dstPixel);										\
				*((uint32*)dstPtr) = dstPixel;											\
				dstPtr+=4; srcPtr+=4; count--;											\
			};																			\
		};

#endif

#if (SourceBits == DestBits) && (SourceEndianess == DestEndianess) && (RenderMode == DEFINE_OP_COPY)
	/*	This is the fastest case. */
	#if (WorryAboutWriteGathering)
		#if (HostEndianess == BIG_ENDIAN)
			#define WriteGatherDeclarations \
				union { double d; struct { uint32 w0,w1; } w; } writeGather;
		#else
			#define WriteGatherDeclarations \
				union { double d; struct { uint32 w1,w0; } w; } writeGather;
		#endif
		/*	We want to do "manual" write gathering.  The idea here is that
			we're going over a slow bus from a dumb or RISCy processor (i.e. PPC)
			and writing in 64-bit chunks will speed things up significantly.  As
			this single routine is used for _all_ write-gathered fast blitting,
			there is no duplication of this code, so the profusion of fall-throughs
			is probably justified. */
		#define DoFastCopyScanLine(srcPtr,dstPtr,count)									\
			count *= bytesPerPixel;														\
			if ((count>0) && (((uint32)dstPtr) & 0x01)) {							\
				*(dstPtr++) = *(srcPtr++); count--;										\
			};																			\
			if ((count>=2) && (((uint32)dstPtr) & 0x02)) {							\
				*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
				dstPtr+=2; srcPtr+=2; count-=2;											\
			};																			\
			if ((count>=4) && (((uint32)dstPtr) & 0x04)) {							\
				*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
				dstPtr+=4; srcPtr+=4; count-=4;											\
			};																			\
			while (count >= 32) {														\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
				writeGather.w.w0 = *((uint32*)(srcPtr+8));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+12));								\
				*((double*)(dstPtr+8)) = writeGather.d;									\
				writeGather.w.w0 = *((uint32*)(srcPtr+16));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+20));								\
				*((double*)(dstPtr+16)) = writeGather.d;								\
				writeGather.w.w0 = *((uint32*)(srcPtr+24));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+28));								\
				*((double*)(dstPtr+24)) = writeGather.d;								\
				dstPtr += 32;  srcPtr += 32;  count -= 32;								\
			};																			\
			if (count >= 16) {															\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
				writeGather.w.w0 = *((uint32*)(srcPtr+8));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+12));								\
				*((double*)(dstPtr+8)) = writeGather.d;									\
				dstPtr += 16;  srcPtr += 16;  count -= 16;								\
			};																			\
			if (count >= 8) {															\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
				dstPtr += 8;  srcPtr += 8;  count -= 8;									\
			};																			\
			if (count >= 4) {															\
				*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
				dstPtr += 4; srcPtr += 4; count -= 4;									\
			};																			\
			if (count >= 2) {															\
				*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
				dstPtr += 2; srcPtr += 2; count -= 2;									\
			};																			\
			if (count > 0) *dstPtr = *srcPtr;											
		#define DoFastCopyScanLineBackwards(srcPtr,dstPtr,count)						\
			count *= bytesPerPixel;														\
			while ((count>0) && (((uint32)dstPtr) & 0x01)) {							\
				*(--dstPtr) = *(--srcPtr); count--;										\
			};																			\
			while ((count>=2) && (((uint32)dstPtr) & 0x07)) {							\
				dstPtr-=2; srcPtr-=2; count-=2;											\
				*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
			};																			\
			while (count >= 32) {														\
				dstPtr -= 32;  srcPtr -= 32;  count -= 32;								\
				writeGather.w.w0 = *((uint32*)(srcPtr+24));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+28));								\
				*((double*)(dstPtr+24)) = writeGather.d;								\
				writeGather.w.w0 = *((uint32*)(srcPtr+16));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+20));								\
				*((double*)(dstPtr+16)) = writeGather.d;								\
				writeGather.w.w0 = *((uint32*)(srcPtr+8));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+12));								\
				*((double*)(dstPtr+8)) = writeGather.d;									\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
			};																			\
			if (count >= 16) {															\
				dstPtr -= 16;  srcPtr -= 16;  count -= 16;								\
				writeGather.w.w0 = *((uint32*)(srcPtr+8));								\
				writeGather.w.w1 = *((uint32*)(srcPtr+12));								\
				*((double*)(dstPtr+8)) = writeGather.d;									\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
			};																			\
			if (count >= 8) {															\
				dstPtr -= 8;  srcPtr -= 8;  count -= 8;									\
				writeGather.w.w0 = *((uint32*)srcPtr);									\
				writeGather.w.w1 = *((uint32*)(srcPtr+4));								\
				*((double*)dstPtr) = writeGather.d;										\
			};																			\
			if (count >= 4) {															\
				dstPtr -= 4; srcPtr -= 4; count -= 4;									\
				*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
			};																			\
			if (count >= 2) {															\
				dstPtr -= 2; srcPtr -= 2; count -= 2;									\
				*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
			};																			\
			if (count > 0) *(--dstPtr) = *(--srcPtr);
	#else /* (!WorryAboutWriteGathering) */
#ifdef RotatedBlit
		#define WriteGatherDeclarations \
			uint32			tmp;		\
			uint32			*dest32;
		/*	We don't give a damn about write gathering.  This might be faster on Intel. */
			#define DoFastCopyScanLine(srcPtr,dstPtr,count)									\
				if ((count>0) && (((uint32)dstPtr) & 0x02)) {								\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
					dstPtr += 2; srcPtr += StepSource; count--;								\
				};																			\
				dest32 = (uint32*)dstPtr;													\
				while (count>=4) {															\
					tmp = *((uint16*)srcPtr); srcPtr += StepSource;							\
					tmp |= *((uint16*)srcPtr)<<16; srcPtr += StepSource;					\
					*dest32++ = tmp;														\
					tmp = *((uint16*)srcPtr); srcPtr += StepSource;							\
					tmp |= *((uint16*)srcPtr)<<16; srcPtr += StepSource;					\
					*dest32++ = tmp;														\
					count-=4;																\
				};																			\
				if (count>=2) {																\
					tmp = *((uint16*)srcPtr); srcPtr += StepSource;							\
					tmp |= *((uint16*)srcPtr)<<16; srcPtr += StepSource;					\
					*dest32++ = tmp;														\
				};																			\
				if (count>0) {																\
					*((uint16*)dest32) = *((uint16*)srcPtr);								\
				};
			#define DoFastCopyScanLineBackwards(srcPtr,dstPtr,count)						\
				while ((count>0) && (((uint32)dstPtr) & 0x02)) {							\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
					dstPtr -= 2; srcPtr -= StepSource; count--;								\
				};																			\
				while (count>=2) {															\
					*((uint32*)dstPtr) = *((uint16*)srcPtr)+(((uint16*)(srcPtr+StepSource))[0]<<16);	\
					dstPtr -= 4; srcPtr -= 2*StepSource; count-=2;							\
				};																			\
				if (count>0) {																\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
				};
#else
		#define WriteGatherDeclarations
		/*	We don't give a damn about write gathering.  This might be faster on Intel. */
			#define DoFastCopyScanLine(srcPtr,dstPtr,count)									\
				count *= bytesPerPixel;														\
				while ((count>0) && (((uint32)dstPtr) & 0x01)) {							\
					*dstPtr++ = *srcPtr++; count--;											\
				};																			\
				while ((count>=2) && (((uint32)dstPtr) & 0x03)) {							\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
					dstPtr+=2; srcPtr+=2; count-=2;											\
				};																			\
				while ((count>=4) && (((uint32)dstPtr) & 0x07)) {							\
					*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
					dstPtr+=4; srcPtr+=4; count-=4;											\
				};																			\
				while (count>=16) {															\
					*((uint32*)(dstPtr)) = *((uint32*)(srcPtr));							\
					*((uint32*)(dstPtr+4)) = *((uint32*)(srcPtr+4));						\
					*((uint32*)(dstPtr+8)) = *((uint32*)(srcPtr+8));						\
					*((uint32*)(dstPtr+12)) = *((uint32*)(srcPtr+12));						\
					dstPtr+=16; srcPtr+=16; count-=16;										\
				};																			\
				while (count>=4) {															\
					*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
					dstPtr+=4; srcPtr+=4; count-=4;											\
				};																			\
				if (count>=2) {																\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
					dstPtr+=2; srcPtr+=2; count-=2;											\
				};																			\
				if (count>0) {																\
					*((uint8*)dstPtr) = *((uint8*)srcPtr);									\
				};
			#define DoFastCopyScanLineBackwards(srcPtr,dstPtr,count)						\
				count *= bytesPerPixel;														\
				while ((count>0) && (((uint32)dstPtr) & 0x01)) {							\
					*(--dstPtr) = *(--srcPtr); count--;										\
				};																			\
				while ((count>=2) && (((uint32)dstPtr) & 0x03)) {							\
					dstPtr-=2; srcPtr-=2; count-=2;											\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
				};																			\
				while ((count>=4) && (((uint32)dstPtr) & 0x07)) {							\
					dstPtr-=4; srcPtr-=4; count-=4;											\
					*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
				};																			\
				while (count>=16) {															\
					dstPtr-=16; srcPtr-=16; count-=16;										\
					*((uint32*)(dstPtr+12)) = *((uint32*)(srcPtr+12));						\
					*((uint32*)(dstPtr+8)) = *((uint32*)(srcPtr+8));						\
					*((uint32*)(dstPtr+4)) = *((uint32*)(srcPtr+4));						\
					*((uint32*)(dstPtr)) = *((uint32*)(srcPtr));							\
				};																			\
				while (count>=4) {															\
					dstPtr-=4; srcPtr-=4; count-=4;											\
					*((uint32*)dstPtr) = *((uint32*)srcPtr);								\
				};																			\
				if (count>=2) {																\
					dstPtr-=2; srcPtr-=2; count-=2;											\
					*((uint16*)dstPtr) = *((uint16*)srcPtr);								\
				};																			\
				if (count>0) {																\
					*((uint8*)(--dstPtr)) = *((uint8*)(--srcPtr));							\
				};
#endif
	#endif
	#define DoCopyScanLine DoFastCopyScanLine
	#define DoCopyScanLineBackwards DoFastCopyScanLineBackwards
#else
	#define WriteGatherDeclarations
	#define DoCopyScanLine DoSlowCopyScanLine
	#define DoCopyScanLineBackwards
#endif


#if (RenderMode == DEFINE_OP_FUNCTION) || (RenderMode == DEFINE_OP_BLEND)
	#define CMapDeclarations												\
		uint32 *index2ARGB = (uint32*)srcPixMap->colorMap->index2ARGB;		\
		uint8 *RGB2Index = (uint8*)srcPixMap->colorMap->inverted;
#elif (SourceBits == 8) && (DestBits != 8)
	#define CMapDeclarations uint32 *index2ARGB = (uint32*)srcPixMap->colorMap->index2ARGB;
#elif (DestBits == 8) && (SourceBits != 8)
	#define CMapDeclarations uint8 *RGB2Index = (uint8*)srcPixMap->colorMap->inverted;
#else
	#define CMapDeclarations
#endif

#define DestPixelDeclarations uint32 dstPixel,dstTransaction;

#if (RenderMode == DEFINE_OP_BLEND)
	#if (AlphaFunction == ALPHA_FUNCTION_COMPOSITE)
		#define TmpVarDeclarations uint32 tmpVar1,srcAlpha,dstAlpha,srcFactor,dstFactor;
	#else
		#define TmpVarDeclarations uint32 tmpVar1,srcAlpha;
	#endif
#else
	#define TmpVarDeclarations
#endif

#define ExtraDeclarations 		\
	DestPixelDeclarations		\
	CMapDeclarations			\
	WriteGatherDeclarations		\
	TmpVarDeclarations

#if (RenderMode == DEFINE_OP_COPY)
	#define DoScanLine DoCopyScanLine
	#define DoScanLineBackwards DoCopyScanLineBackwards
	#define DoScaledScanLine DoScaledCopyScanLine
#elif (RenderMode == DEFINE_OP_OVER)
	#define DoScanLine DoOverScanLine
	#define DoScanLineBackwards
	#define DoScaledScanLine DoScaledOverScanLine
#elif (RenderMode == DEFINE_OP_BLEND)
	#define DoScanLine DoBlendScanLine
	#define DoScanLineBackwards
	#define DoScaledScanLine DoScaledBlendScanLine
#else
	#define DoScanLine DoFunctionScanLine
	#define DoScanLineBackwards
	#define DoScaledScanLine DoScaledFunctionScanLine
#endif

#ifndef DoNotGenerateFunction
#if (RenderMode == DEFINE_OP_FUNCTION)
	static void BlitFuncName (
		RenderContext *context,
		RenderSubPort *port,
		Pixels *srcPixMap, Pixels *dstPixMap,
		uint8 *src, int32 srcByteWidth,
		uint8 *dst, int32 dstByteWidth,
		int32 countX, int32 fixedXOrig, int32 incX,
		int32 countY, int32 errorY, int32 incY, int32 defaultPtrIncY,
		int32 bytesPerPixel, int32 srcOffs, int32 directionFlags
	) {
		grAssertLocked(context, port);
		RenderCache *cache = port->cache;
		uint8 *dstPtr;
		int32 count,error,fixedX;
		uint32 srcPixel, srcTransaction;
		ColorOpFuncPixel pixelOpFunc = (ColorOpFuncPixel)colorOpTable[context->drawOp];
		ExtraDeclarations;
		while (countY--) {
			dstPtr = dst;
			count = countX;
			fixedX = fixedXOrig;
			while (count>0) {															
				srcTransaction = *((SourceType*)(src+PixMult(fixedX>>16)));				

#define SourceFormatBits		SourceBits
#define SourceFormatEndianess	SourceEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define SrcToARGB(a,b) ConvertPixelFormat(a,b);
#else
	#define SrcToARGB(a,b) b=a;
#endif

				SrcToARGB(srcTransaction,srcPixel);										
				dstTransaction = *((DestType*)dstPtr);									

#define SourceFormatBits		DestBits
#define SourceFormatEndianess	DestEndianess
#define DestFormatBits			32
#define DestFormatEndianess		HostEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define DstToARGB(a,b) ConvertPixelFormat(a,b);
#else
	#define DstToARGB(a,b) b=a;
#endif

				DstToARGB(dstTransaction,srcTransaction);										
				dstTransaction = pixelOpFunc(srcPixel,srcTransaction,context);				
				if (dstTransaction != ARGBMagicTransparent) {							

#define SourceFormatBits		32
#define SourceFormatEndianess	HostEndianess
#define DestFormatBits			DestBits
#define DestFormatEndianess		DestEndianess
#include "pixelConvert.inc"
#ifdef ConvertPixelFormat
	#define ARGBToDst(a,b) ConvertPixelFormat(a,b);
#else
	#define ARGBToDst(a,b) b=a;
#endif

					ARGBToDst(dstTransaction,dstPixel);									
					*((DestType*)dstPtr) = dstPixel;									
				};																		
				fixedX += incX;															
				dstPtr += DestAlignment; count--;										
			};																			
			dst += dstByteWidth;
			errorY -= incY;
			srcPixel = errorY >> 31;
			errorY += (srcPixel & 65536);
	#ifdef RotatedBlit
			src += (srcPixel & sizeof(SourceType)) + defaultPtrIncY;
	#else
			src += (srcPixel & srcByteWidth) + defaultPtrIncY;
	#endif
		};
	};
#elif (BlitScaling)
	static void BlitFuncName (
		RenderContext *context,
		RenderSubPort *port,
		Pixels *srcPixMap, Pixels *dstPixMap,
		uint8 *src, int32 srcByteWidth,
		uint8 *dst, int32 dstByteWidth,
		int32 countX, int32 fixedXOrig, int32 incX,
		int32 countY, int32 errorY, int32 incY, int32 defaultPtrIncY,
		int32 bytesPerPixel, int32 srcOffs, int32 directionFlags
	) {
		RenderCache *cache = port->cache;
		uint8 *dstPtr;
		int32 count,error,fixedX;
		uint32 srcPixel, srcTransaction;
		ExtraDeclarations;
		while (countY--) {
			dstPtr = dst;
			count = countX;
			fixedX = fixedXOrig;
			DoScaledScanLine;
			dst += dstByteWidth;
			errorY -= incY;
			srcPixel = errorY >> 31;
			errorY += (srcPixel & 65536);
	#ifdef RotatedBlit
			src += (srcPixel & sizeof(SourceType)) + defaultPtrIncY;
	#else
			src += (srcPixel & srcByteWidth) + defaultPtrIncY;
	#endif
		};
	};
#else
	static void BlitFuncName (
		RenderContext *context,
		RenderSubPort *port,
		Pixels *srcPixMap, Pixels *dstPixMap,
		uint8 *src, int32 srcBytesPerRow,
		uint8 *dst, int32 dstBytesPerRow,
		int32 width, int32 fixedX, int32 incX,
		int32 height, int32 errorY, int32 incY, int32 defaultPtrIncY,
		int32 bytesPerPixel, int32 srcOffs, int32 directionFlags
	) {
		RenderCache *cache = port->cache;
		uint8 *srcPtr,*dstPtr;
		uint32 srcPixel;
		uint32 srcTransaction;
		ExtraDeclarations;
		int32 count;
	#ifdef RotatedBlit
		int32 RowStepSource = sizeof(SourceType);
	#endif
		
		if ((directionFlags&TOP_TO_BOTTOM) == 0) {
	#ifdef RotatedBlit
			src += (height-1)*RowStepSource;
			RowStepSource = -RowStepSource;
	#else
			src += ((height-1)*srcBytesPerRow);
			srcBytesPerRow = -srcBytesPerRow;
	#endif
			dst += ((height-1)*dstBytesPerRow);
			dstBytesPerRow = -dstBytesPerRow;
		};
		
		if (directionFlags&LEFT_TO_RIGHT) {
			while (height--) {
				count = width;
				srcPtr = src;
				dstPtr = dst;
				DoScanLine(srcPtr,dstPtr,count);
	#ifdef RotatedBlit
				src += RowStepSource;
	#else
				src += srcBytesPerRow;
	#endif
				dst += dstBytesPerRow;
			};
		} else {
 			src += width*bytesPerPixel;
 			dst += width*bytesPerPixel;
 			while (height--) {
				count = width;
				srcPtr = src;
				dstPtr = dst;
				DoScanLineBackwards(srcPtr,dstPtr,count);
	#ifdef RotatedBlit
				src += RowStepSource;
	#else
				src += srcBytesPerRow;
	#endif
				dst += dstBytesPerRow;
			};
		};
	};
#endif
#endif

#undef BlitFuncName
