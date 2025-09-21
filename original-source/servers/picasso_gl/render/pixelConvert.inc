
#undef ConvertPixelFormat
#undef SrcRed
#undef SrcGreen
#undef SrcBlue
#undef SrcAlpha
#undef SrcAlphaBit
#undef DstRed
#undef DstGreen
#undef DstBlue
#undef DstAlpha
#undef SwapForComponents
#undef AlphaBlend
#undef AlphaComposite
#undef HalfAlphaBlend
#undef PreprocessSource

#if (SourceFormatBits == 8)
	#if (DestFormatEndianess == HostEndianess)
		#if (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					\
				dstPixel = 	((dstPixel & 0x80000000) >> 16) |					\
							((dstPixel & 0x00F80000) >>  9) |					\
							((dstPixel & 0x0000F800) >>  6) |					\
							((dstPixel & 0x000000F8) >>  3) ;
		#elif (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					\
				dstPixel = 	((dstPixel & 0x00F80000) >>  8) |					\
							((dstPixel & 0x0000FC00) >>  5) |					\
							((dstPixel & 0x000000F8) >>  3) ;					
		#elif (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					
		#endif
	#else /* (DestFormatEndianess != HostEndianess) */
		#if (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					\
				dstPixel = 	((dstPixel & 0x80000000) >> 24) |					\
							((dstPixel & 0x00F80000) >> 17) |					\
							((dstPixel & 0x0000F800) >> 14) |					\
							((dstPixel & 0x00003800) <<  2) |					\
							((dstPixel & 0x000000F8) <<  5) ;
		#elif (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					\
				dstPixel = 	((dstPixel & 0x00F80000) >> 16) |					\
							((dstPixel & 0x0000FC00) >> 13) |					\
							((dstPixel & 0x00001C00) <<  3) |					\
							((dstPixel & 0x000000F8) <<  5) ;
		#elif (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = *((uint32*)&index2ARGB[(srcPixel)]);					\
				dstPixel = 	((dstPixel & 0xFF000000) >> 24) |					\
							((dstPixel & 0x00FF0000) >>  8) |					\
							((dstPixel & 0x0000FF00) <<  8) |					\
							((dstPixel & 0x000000FF) << 24) ;
		#endif
	#endif
#endif

#if (SourceFormatBits == 8)
	#define SrcAlpha(a)			((a) >> 24)
	#define SrcRed(a)			(((a) >> 16) & 0xFF)
	#define SrcGreen(a)			(((a) >> 8) & 0xFF)
	#define SrcBlue(a)			((a) & 0xFF)
	#define PreprocessSource	srcPixel = *((uint32*)&index2ARGB[srcPixel]);
#elif (SourceFormatBits == 16)
	#define SrcAlpha(a)			255
	#define SrcRed(a)			((((a) >> 8) & 0xF8) | (((a) >> 13) & 0x7))
	#define SrcGreen(a)			((((a) >> 3) & 0xFC) | (((a) >> 9) & 0x3))
	#define SrcBlue(a)			((((a) << 3) & 0xF8) | (((a) >> 2) & 0x7))
	#if (SourceFormatEndianess == HostEndianess)
	#define PreprocessSource
	#else
	#define PreprocessSource	(srcPixel = (srcPixel >> 8) | (srcPixel << 8))
	#endif
#elif (SourceFormatBits == 15)
	#define SrcAlpha(a)		((0-((a) >> 15)) & 0xFF)
	#define SrcRed(a)		((((a) >> 7) & 0xF8) | (((a) >> 12) & 0x7))
	#define SrcGreen(a)		((((a) >> 2) & 0xF8) | (((a) >> 7) & 0x7))
	#define SrcBlue(a)		((((a) << 3) & 0xF8) | (((a) >> 2) & 0x7))
	#if (SourceFormatEndianess == HostEndianess)
	#define PreprocessSource
	#else
	#define PreprocessSource	(srcPixel = (srcPixel >> 8) | (srcPixel << 8))
	#endif
#elif (SourceFormatBits == 32)
	#if (SourceFormatEndianess == HostEndianess)
		#define SrcAlpha(a)		((a) >> 24)
		#define SrcRed(a)		(((a) >> 16) & 0xFF)
		#define SrcGreen(a)		(((a) >> 8) & 0xFF)
		#define SrcBlue(a)		((a) & 0xFF)
	#else
		#define SrcBlue(a)		((a) >> 24)
		#define SrcGreen(a)		(((a) >> 16) & 0xFF)
		#define SrcRed(a)		(((a) >> 8) & 0xFF)
		#define SrcAlpha(a)		((a) & 0xFF)
	#endif
	#define PreprocessSource
#endif

#if (SourceFormatEndianess == HostEndianess)
	#define SrcAlphaBit(a)		((a) & 0x8000)
#else
	#define SrcAlphaBit(a)		((a) & 0x0080)
#endif

#if (DestFormatEndianess == HostEndianess)
	#define SwapForComponents(a)
	#if (DestFormatBits == 32)
		#define DstAlpha(a)		((a) >> 24)
		#define DstRed(a)		(((a) >> 16) & 0xFF)
		#define DstGreen(a)		(((a) >> 8) & 0xFF)
		#define DstBlue(a)		((a) & 0xFF)
	#endif
#else
	#define SwapForComponents(a) ((a) = (((a) >> 8) | ((a) << 8)) & 0xFFFF)
	#if (DestFormatBits == 32)
		#define DstBlue(a)		((a) >> 24)
		#define DstGreen(a)		(((a) >> 16) & 0xFF)
		#define DstRed(a)		(((a) >> 8) & 0xFF)
		#define DstAlpha(a)		((a) & 0xFF)
	#endif
#endif

#if (DestFormatBits == 8)
	#define DstAlpha(a)		((a) >> 24)
	#define DstRed(a)		(((a) >> 16) & 0xFF)
	#define DstGreen(a)		(((a) >> 8) & 0xFF)
	#define DstBlue(a)		((a) & 0xFF)
#elif (DestFormatBits == 16)
	#define DstAlpha(a)		255
	#define DstRed(a)		((((a) >> 8) & 0xF8) | (((a) >> 13) & 0x7))
	#define DstGreen(a)		((((a) >> 3) & 0xFC) | (((a) >> 9) & 0x3))
	#define DstBlue(a)		((((a) << 3) & 0xF8) | (((a) >> 2) & 0x7))
#elif (DestFormatBits == 15)
	#define DstAlpha(a)		((0-((a) >> 15)) & 0xFF)
	#define DstRed(a)		((((a) >> 7) & 0xF8) | (((a) >> 12) & 0x7))
	#define DstGreen(a)		((((a) >> 2) & 0xF8) | (((a) >> 7) & 0x7))
	#define DstBlue(a)		((((a) << 3) & 0xF8) | (((a) >> 2) & 0x7))
#endif

#if (DestFormatBits == 8)

	#define AlphaBlend(srcPixel,dstPixel,srcAlpha)											\
		if ((AlphaFunction != ALPHA_FUNCTION_COMPOSITE) || (dstPixel != 0xFF)) {			\
			PreprocessSource;																\
			dstPixel = *((uint32*)&index2ARGB[(dstPixel)]);									\
			tmpVar1 =	((																	\
						(((SrcGreen(srcPixel) << 16) | SrcRed(srcPixel)) * srcAlpha) +		\
						(((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
					) & 0xF800F800);														\
			dstPixel = 	((tmpVar1 >> 1) & 0xFFFF) | (tmpVar1 >> 22) |						\
						(((DstBlue(dstPixel) * (256-srcAlpha)) +							\
						 (SrcBlue(srcPixel) * srcAlpha)) >> 11);							\
			dstPixel = RGB2Index[dstPixel];													\
		} else dstPixel = srcPixel;

	#define HalfAlphaBlend(srcPixel,dstPixel,srcAlpha)									\
		dstPixel = *((uint32*)&index2ARGB[(dstPixel)]);									\
		tmpVar1 =	((((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
				& 0xF800F800);															\
		dstPixel = 	((tmpVar1 >> 1) & 0xFFFF) | (tmpVar1 >> 22) |						\
					((DstBlue(dstPixel) * (256-srcAlpha)) >> 11);						\
		dstPixel = RGB2Index[dstPixel+srcPixel];

#elif (DestFormatBits == 15)

	#define AlphaBlend(srcPixel,dstPixel,srcAlpha)											\
		PreprocessSource;																	\
		SwapForComponents(dstPixel);														\
		if ((AlphaFunction != ALPHA_FUNCTION_COMPOSITE) || (dstPixel & 0x8000)) {			\
			tmpVar1 =	((																	\
						(((SrcGreen(srcPixel) << 16) | SrcRed(srcPixel)) * srcAlpha) +		\
						(((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
					) & 0xF800F800);														\
			dstPixel = 	((tmpVar1 >> 1) & 0xFFFF) | (tmpVar1 >> 22) |						\
						(((DstBlue(dstPixel) * (256-srcAlpha)) +							\
						 (SrcBlue(srcPixel) * srcAlpha)) >> 11);							\
			SwapForComponents(dstPixel);													\
		} else dstPixel = srcPixel;

	#define HalfAlphaBlend(srcPixel,dstPixel,srcAlpha)									\
		SwapForComponents(dstPixel);													\
		tmpVar1 =	(((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
				& 0xF800F800;															\
		dstPixel = 	((tmpVar1 >> 1) & 0xFFFF) | (tmpVar1 >> 22) |						\
					((DstBlue(dstPixel) * (256-srcAlpha)) >> 11);						\
		SwapForComponents(dstPixel);													\
		dstPixel += srcPixel;

#elif (DestFormatBits == 16)

	#define AlphaBlend(srcPixel,dstPixel,srcAlpha)										\
		PreprocessSource;																\
		SwapForComponents(dstPixel);													\
		tmpVar1 = ((																	\
					(((SrcGreen(srcPixel) << 16) | SrcRed(srcPixel)) * srcAlpha) +		\
					(((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
				) & 0xFC00F800);														\
		dstPixel = 	(tmpVar1 & 0xFFFF) | (tmpVar1 >> 21) |								\
					(((DstBlue(dstPixel) * (256-srcAlpha)) +							\
					 (SrcBlue(srcPixel) * srcAlpha)) >> 11);							\
		SwapForComponents(dstPixel);												

	#define HalfAlphaBlend(srcPixel,dstPixel,srcAlpha)									\
		SwapForComponents(dstPixel);													\
		tmpVar1 = (((DstGreen(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
				& 0xFC00F800;															\
		dstPixel = 	(tmpVar1 & 0xFFFF) | (tmpVar1 >> 21) |								\
					((DstBlue(dstPixel) * (256-srcAlpha)) >> 11);						\
		SwapForComponents(dstPixel);													\
		dstPixel += srcPixel;

#else // (DestFormatBits == 32)

	#if (DestFormatEndianess == HostEndianess)

		#if (AlphaFunction == ALPHA_FUNCTION_COMPOSITE)
			#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
				PreprocessSource;															\
				dstAlpha = DstAlpha(dstPixel);												\
				dstAlpha += (dstAlpha >> 7);												\
				srcFactor = 256 - dstAlpha;													\
				tmpVar1 = ((srcFactor*srcAlpha)>>8) + dstAlpha;								\
				if (tmpVar1 > 255) tmpVar1 = 255;											\
				tmpVar1 <<= 24;																\
				srcFactor = ((srcFactor*srcFactor) + (srcAlpha*dstAlpha)) >> 8;				\
				dstFactor = ((512-dstAlpha-srcAlpha) * dstAlpha) >> 8;						\
				dstPixel = tmpVar1 |														\
					   ((																	\
						(SrcGreen(srcPixel) * srcFactor) +									\
						(DstGreen(dstPixel) * dstFactor)									\
					   ) & 0xFF00FF00) |													\
					   (((																	\
						(((SrcRed(srcPixel) << 16) | SrcBlue(srcPixel)) * srcFactor) +		\
						(((DstRed(dstPixel) << 16) | DstBlue(dstPixel)) * dstFactor)		\
					   ) & 0xFF00FF00) >> 8);
		#else
			#if (SourceFormatEndianess == HostEndianess) && (SourceFormatBits == 32)
				#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
					PreprocessSource;															\
					dstPixel =																	\
						((																		\
							(((dstPixel>>8)&0x00FF00FF) * (256-srcAlpha)) +						\
							(((srcPixel>>8)&0x00FF00FF) * srcAlpha) 							\
						) & 0xFF00FF00) |														\
						(((																		\
							((srcPixel&0x00FF00FF) * srcAlpha) +								\
							((dstPixel&0x00FF00FF) * (256-srcAlpha))							\
						) >> 8) & 0x00FF00FF) ;													
			#else
				#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
					PreprocessSource;															\
					dstPixel =																	\
						   ((																	\
							(((SrcAlpha(srcPixel) << 16) | SrcGreen(srcPixel)) * srcAlpha) +	\
							(((DstAlpha(dstPixel) << 16) | DstGreen(dstPixel)) * (256-srcAlpha))\
						   ) & 0xFF00FF00) |													\
						   (((																	\
							(((SrcRed(srcPixel) << 16) | SrcBlue(srcPixel)) * srcAlpha) +		\
							(((DstRed(dstPixel) << 16) | DstBlue(dstPixel)) * (256-srcAlpha))	\
						   ) & 0xFF00FF00) >> 8);
			#endif
		#endif

		#define HalfAlphaBlend(srcPixel,dstPixel,srcAlpha)								\
			dstPixel =																	\
				((((dstPixel>>8)&0x00FF00FF) * (256-srcAlpha)) & 0xFF00FF00) |			\
				((((dstPixel&0x00FF00FF) * (256-srcAlpha))>>8) & 0x00FF00FF) ;			\
			dstPixel += srcPixel;

	#else

		#if (AlphaFunction == ALPHA_FUNCTION_COMPOSITE)
			#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
				PreprocessSource;															\
				dstAlpha = DstAlpha(dstPixel);												\
				dstAlpha += (dstAlpha >> 7);												\
				srcFactor = 256 - dstAlpha;													\
				tmpVar1 = ((srcFactor*srcAlpha)>>8) + dstAlpha;								\
				if (tmpVar1 > 255) tmpVar1 = 255;											\
				srcFactor = ((srcFactor*srcFactor) + (srcAlpha*dstAlpha)) >> 8;				\
				dstFactor = ((512-dstAlpha-srcAlpha) * dstAlpha) >> 8;						\
				dstPixel = tmpVar1 |														\
					   (((																	\
						(SrcGreen(srcPixel) * srcFactor) +									\
						(DstGreen(dstPixel) * dstFactor)									\
					   ) & 0x0000FF00) << 8) |												\
					   ((																	\
						(((SrcBlue(srcPixel) << 16) | SrcRed(srcPixel)) * srcFactor) +		\
						(((DstBlue(dstPixel) << 16) | DstRed(dstPixel)) * dstFactor)		\
					   ) & 0xFF00FF00);												
		#else
			#if (SourceFormatEndianess != HostEndianess) && (SourceFormatBits == 32)
				#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
					PreprocessSource;															\
					dstPixel =																	\
						((																		\
							(((dstPixel>>8)&0x00FF00FF) * (256-srcAlpha)) +						\
							(((srcPixel>>8)&0x00FF00FF) * srcAlpha) 							\
						) & 0xFF00FF00) |														\
						(((																		\
							((srcPixel&0x00FF00FF) * srcAlpha) +								\
							((dstPixel&0x00FF00FF) * (256-srcAlpha))							\
						) >> 8) & 0x00FF00FF) ;													
			#else
				#define AlphaBlend(srcPixel,dstPixel,srcAlpha)									\
					PreprocessSource;															\
					dstPixel =																	\
						   ((																	\
							(((SrcBlue(srcPixel) << 16) | SrcRed(srcPixel)) * srcAlpha) +		\
							(((DstBlue(dstPixel) << 16) | DstRed(dstPixel)) * (256-srcAlpha))	\
						   ) & 0xFF00FF00) |													\
						   (((																	\
							(((SrcGreen(srcPixel) << 16) | SrcAlpha(srcPixel)) * srcAlpha) +	\
							(((DstGreen(dstPixel) << 16) | DstAlpha(dstPixel)) * (256-srcAlpha))\
						   ) & 0xFF00FF00) >> 8);												
			#endif
		#endif

		#define HalfAlphaBlend(srcPixel,dstPixel,srcAlpha)								\
			dstPixel =																	\
				((((dstPixel>>8)&0x00FF00FF) * (256-srcAlpha)) & 0xFF00FF00) |			\
				((((dstPixel&0x00FF00FF) * (256-srcAlpha))>>8) & 0x00FF00FF) ;			\
			dstPixel += srcPixel;

	#endif
#endif

#if (SourceFormatEndianess == HostEndianess)
	#if (SourceFormatBits == 32) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)									\
			dstPixel = 	RGB2Index[														\
							((srcPixel & 0x00F80000) >> 9) |							\
							((srcPixel & 0x0000F800) >> 6) |							\
							((srcPixel & 0x000000F8) >> 3) ];
	#elif (SourceFormatBits == 16) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)						\
			dstPixel = 	RGB2Index[											\
							((srcPixel & 0x0000FFC0) >> 1) |				\
							((srcPixel & 0x0000001F))      ];
	#elif (SourceFormatBits == 15) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)						\
			dstPixel = 	RGB2Index[srcPixel & 0x00007FFF];
	#endif
	#if (DestFormatEndianess == HostEndianess)
		#if (SourceFormatBits == 32) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x80000000) >> 16) |					\
							((srcPixel & 0x00F80000) >>  9) |					\
							((srcPixel & 0x0000F800) >>  6) |					\
							((srcPixel & 0x000000F8) >>  3) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x00F80000) >>  8) |					\
							((srcPixel & 0x0000FC00) >>  5) |					\
							((srcPixel & 0x000000F8) >>  3) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x00007FE0) <<  1) |					\
							((srcPixel & 0x00000200) >>  4) |					\
							((srcPixel & 0x0000001F))       ;
		#elif (SourceFormatBits == 16) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000FFC0) >>  1) |					\
							((srcPixel & 0x0000001F))       |					\
							0x00008000                      ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x00007C00) << 9) |					/* 5-bit red to 8-bit red high bits */				\
							((srcPixel & 0x000003E0) << 6) |					/* 5-bit green to 8-bit green high bits */			\
							((srcPixel & 0x0000001F) << 3) ;					/* 5-bit blue to 8-bit blue high bits */			\
				dstPixel |=	((dstPixel & 0x00E0E0E0) >> 5) | ((((int32)(srcPixel<<16)) >> 7) & 0xFF000000);
		#elif (SourceFormatBits == 16) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x0000F800) << 8)  |					/* 5-bit red to 8-bit red high bits */				\
							((srcPixel & 0x000007E0) << 5)  |					/* 6-bit green to 8-bit green high bits */			\
							((srcPixel & 0x0000001F) << 3)  ;					/* 5-bit blue to 8-bit blue high bits */			\
				dstPixel |=	((dstPixel & 0x00E000E0) >> 5)  |																		\
							((dstPixel & 0x0000C000) >> 6)  |																		\
							0xFF000000;
		#endif
	#else /* (DestFormatEndianess != HostEndianess) */
		#if (SourceFormatBits == 32) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x80000000) >> 24) |					\
							((srcPixel & 0x00F80000) >> 17) |					\
							((srcPixel & 0x0000F800) >> 14) |					\
							((srcPixel & 0x00003800) <<  2) |					\
							((srcPixel & 0x000000F8) <<  5) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x00F80000) >> 16) |					\
							((srcPixel & 0x0000FC00) >> 13) |					\
							((srcPixel & 0x00001C00) <<  3) |					\
							((srcPixel & 0x000000F8) <<  5) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0xFF000000) >> 24) |					\
							((srcPixel & 0x00FF0000) >>  8) |					\
							((srcPixel & 0x0000FF00) <<  8) |					\
							((srcPixel & 0x000000FF) << 24) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x00007FE0) >>  7) |					\
							((srcPixel & 0x00000060) <<  9) |					\
							((srcPixel & 0x00000200) <<  4) |					\
							((srcPixel & 0x0000001F) <<  8) ;
		#elif (SourceFormatBits == 16) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000FFC0) >>  9) |					\
							((srcPixel & 0x000001C0) <<  7) |					\
							((srcPixel & 0x0000001F) <<  8) |					\
							0x00000080                      ;
		#elif (SourceFormatBits == DestFormatBits)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000FF00) >>  8) |					\
							((srcPixel & 0x000000FF) <<  8) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x00007C00) <<  1) |																			\
							((srcPixel & 0x000003E0) << 14) |																			\
							((srcPixel & 0x0000001F) << 27) ;																			\
				dstPixel |=	((dstPixel & 0xE0E0E000) >> 5) | ((((int32)(srcPixel<<16))>> 31) & 0x000000FF);
		#elif (SourceFormatBits == 16) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x0000F800))       |					/* 5-bit red to 8-bit red high bits */					\
							((srcPixel & 0x000007E0) << 13) |					/* 6-bit green to 8-bit green high bits */				\
							((srcPixel & 0x0000001F) << 27) ;					/* 5-bit blue to 8-bit blue high bits */				\
				dstPixel |=	((dstPixel & 0xE000E000) >> 5)  |																			\
							((dstPixel & 0x00C00000) >> 6)  |																			\
							0x000000FF;
		#endif
	#endif
#else /* (SourceFormatEndianess != HostEndianess) */
	#if (SourceFormatBits == 32) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)							\
			dstPixel = 	RGB2Index[												\
							((srcPixel & 0x0000F800) >>  1) |					\
							((srcPixel & 0x00F80000) >> 14) |					\
							((srcPixel & 0xF8000000) >> 27) ];
	#elif (SourceFormatBits == 16) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)							\
			dstPixel = 	RGB2Index[												\
							((srcPixel & 0x000000FF) << 7) |					\
							((srcPixel & 0x0000C000) >> 9) |					\
							((srcPixel & 0x00001F00) >> 8) ];
	#elif (SourceFormatBits == 15) && (DestFormatBits == 8)
		#define ConvertPixelFormat(srcPixel,dstPixel)							\
			dstPixel = 	RGB2Index[												\
							((srcPixel & 0x0000FF00) >> 8) |					\
							((srcPixel & 0x0000007F) << 8) ];
	#endif
	#if (DestFormatEndianess == HostEndianess)
		#if (SourceFormatBits == 32) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x00000080) <<  8) |					\
							((srcPixel & 0x0000F800) >>  1) |					\
							((srcPixel & 0x00F80000) >> 14) |					\
							((srcPixel & 0xF8000000) >> 27) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000F800))       |					\
							((srcPixel & 0x00FC0000) >> 13) |					\
							((srcPixel & 0xF8000000) >> 27) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0xFF000000) >> 24) |					\
							((srcPixel & 0x00FF0000) >>  8) |					\
							((srcPixel & 0x0000FF00) <<  8) |					\
							((srcPixel & 0x000000FF) << 24) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000007F) <<  9) |					\
							((srcPixel & 0x00000002) <<  4) |					\
							((srcPixel & 0x0000E000) >>  7) |					\
							((srcPixel & 0x00001F00) >>  8) ;					
		#elif (SourceFormatBits == 16) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x000000FF) <<  7) |					\
							((srcPixel & 0x0000C000) >>  9) |					\
							((srcPixel & 0x00001F00) >>  8) |					\
							0x00008000                      ;
		#elif (SourceFormatBits == DestFormatBits)
			#define ConvertPixelFormat(srcPixel,dstPixel)						\
				dstPixel = 	((srcPixel & 0x0000FF00) >>  8) |					\
							((srcPixel & 0x000000FF) <<  8) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x0000007C) << 17) |					/* 5-bit red to 8-bit red */							\
							((srcPixel & 0x00000003) << 14) |					/* 5-bit green to 8-bit green */						\
							((srcPixel & 0x0000E000) >>  2) |																			\
							((srcPixel & 0x00001F00) >>  5) ;					/* 5-bit blue to 8-bit blue high bits */				\
				dstPixel |= ((dstPixel & 0x00E0E0E0) >> 5)  |																			\
							((((int32)(srcPixel<<24))>> 31) & 0xFF000000);		/* 1-bit alpha to 8-bit alpha */
		#elif (SourceFormatBits == 16) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x000000F8) << 16) |					/* 5-bit red to 8-bit red */							\
							((srcPixel & 0x00000007) << 13) |					/* 5-bit green to 8-bit green */						\
							((srcPixel & 0x0000E000) >>  3) |																			\
							((srcPixel & 0x00001F00) >>  5) ;					/* 5-bit blue to 8-bit blue high bits */				\
				dstPixel |= ((dstPixel & 0x00E000E0) >> 5)  |																			\
							((dstPixel & 0x0000C000) >> 6)  |																			\
							0xFF000000;
		#endif
	#else /* (DestFormatEndianess != HostEndianess) */
		#if (SourceFormatBits == 32) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)							\
				dstPixel = 	((srcPixel & 0x00000080))       |					\
							((srcPixel & 0x0000F800) >>  9) |					\
							((srcPixel & 0x00F80000) >> 22) |					\
							((srcPixel & 0x00380000) >>  6) |					\
							((srcPixel & 0xF8000000) >> 19) ;
		#elif (SourceFormatBits == 32) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)							\
				dstPixel = 	((srcPixel & 0x0000F800) >>  8) |					\
							((srcPixel & 0x00FC0000) >> 21) |					\
							((srcPixel & 0x001C0000) >>  5) |					\
							((srcPixel & 0xF8000000) >> 19) ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 16)
			#define ConvertPixelFormat(srcPixel,dstPixel)							\
				dstPixel = 	((srcPixel & 0x0000007F) <<  1) |					\
							((srcPixel & 0x00008000) >> 15) |					\
							((srcPixel & 0x00006000) <<  1) |					\
							((srcPixel & 0x00000002) << 12) |					\
							((srcPixel & 0x00001F00))       ;					
		#elif (SourceFormatBits == 16) && (DestFormatBits == 15)
			#define ConvertPixelFormat(srcPixel,dstPixel)							\
				dstPixel = 	((srcPixel & 0x000000FE) >>  1) |					\
							((srcPixel & 0x00000001) << 15) |					\
							((srcPixel & 0x0000C000) >>  1) |					\
							((srcPixel & 0x00001F00))       |					\
							0x00000080                      ;
		#elif (SourceFormatBits == 15) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel =	((srcPixel & 0x0000007C) <<  9) |					/* 5-bit red to 8-bit red */							\
							((srcPixel & 0x00000003) << 22) |					/* 5-bit green to 8-bit green */						\
							((srcPixel & 0x0000E000) <<  6) |																			\
							((srcPixel & 0x00001F00) << 19) ;					/* 5-bit blue to 8-bit blue high bits */				\
				dstPixel |= ((dstPixel & 0xE0E0E000) >> 5)  |																			\
							((((int32)(srcPixel<<24))>> 31) & 0x000000FF);		/* 1-bit alpha to 8-bit alpha */
		#elif (SourceFormatBits == 16) && (DestFormatBits == 32)
			#define ConvertPixelFormat(srcPixel,dstPixel)																				\
				dstPixel = 	((srcPixel & 0x000000F8) <<  8) |					/* 5-bit red to 8-bit red */							\
							((srcPixel & 0x00000007) << 21) |					/* 5-bit green to 8-bit green */						\
							((srcPixel & 0x0000E000) <<  5) |																			\
							((srcPixel & 0x00001F00) << 19) ;					/* 5-bit blue to 8-bit blue high bits */				\
				dstPixel |= ((dstPixel & 0xE000E000) >> 5)  |																			\
							((dstPixel & 0x00C00000) >> 6)  |																			\
							0x000000FF;
		#endif
	#endif
#endif

#undef SourceFormatBits
#undef SourceFormatEndianess
#undef DestFormatBits
#undef DestFormatEndianess
