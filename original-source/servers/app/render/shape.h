)+dst.left*2);					\
	for (j=src.top; j<=src.bottom; j++) {														\
		for (i=src.left; i<=src.right; i++) {													\
			gray = (src_ptr[i>>1]>>(4-((i<<2)&4)))&7;											\
			if (gray != 0) {																	\
				theColor = *dst_ptr;															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha[gray]) +									\
						colors[gray  ]) >> 8) & 0x00FF00FF) |									\
					(((((colorIn & 0x0000FF00) * alpha[gray]) +									\
						colors[gray+8]) >> 8) & 0x0000FF00) ;									\
				*dst_ptr = ComponentsToPixel(colorIn>>8,colorIn,colorIn<<8);					\
			};																					\
			dst_ptr++;																			\
		};																						\
		dst_ptr = (uint16*)(dst_rowBase+=dst_row);												\
		src_ptr += src_row;																		\
	};																							\
};																								\
									