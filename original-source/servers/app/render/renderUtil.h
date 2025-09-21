				\
						} else goto next_pixel;											\
				};																		\
				colorIn = BackToCanvas(colorIn);										\
				alreadyInNativeFormat:													\
				dst_ptr[i] = colorIn;													\
				next_pixel:;															\
			};																			\
			hmin = hmax;																\
		}																				\
		next_line:																		\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);										\
	}																					\
}

#define AlphaPreprocessFuncs(AlphaPreprocessName,AlphaDrawString,AlphaDrawChar,AlphaDrawCharAA)	\
static void AlphaDrawCharAA (	uchar *dst_base, int dst_row, rect dst,							\
								uchar *src_base, int src_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int32 i,j,gray;																				\
	uint32 colorIn,theColor;																	\
	uint16 *dst_p