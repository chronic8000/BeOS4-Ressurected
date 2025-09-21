c_row, rect src,							\
								uint32 *colors, uint32 *alpha)									\
{																								\
	int			i, j, cmd, hmin, hmax, imin, imax;												\
	uint16 *	dst_ptr;																		\
	uint32		theColor,colorIn,alpha0=alpha[0],color0=colors[0],color8=colors[8];				\
	uint8 *		dst_rowBase;																	\
																								\
	dst_ptr = (uint16*)(dst_rowBase = (dst_base+dst_row*dst.top)+(dst.left-src.left)*2);		\
	for (j=0; j<src.top; j++)																	\
		do src_base++; while (src_base[-1] != 0xff);											\
	for (; j<=src.bottom; j++) {																\
		hmin = 0;																				\
		while (TRUE) {																			\
			do {																				\
				cmd = *src_base++;																\
				if (cmd == 0xff)																\
					goto next_line;																\
				hmin += cmd;																	\
			} while (cmd == 0xfe);																\
			hmax = hmin;																		\
			do {																				\
				cmd = *src_base++;																\
				hmax += cmd;																	\
			} while (cmd == 0xfe);																\
			if (hmin > src.right) {																\
				do src_base++; while (src_base[-1] != 0xff);									\
				goto next_line;																	\
			}																					\
			if (hmin >= src.left)																\
				imin = hmin;																	\
			else																				\
				imin = src.left;																\
			if (hmax <= src.right)																\
				imax = hmax;																	\
			else																				\
				imax = src.right+1;																\
			for (i=imin; i<imax; i++) {															\
				theColor = dst_ptr[i];															\
				CanvasToARGB(theColor,colorIn);													\
				colorIn =																		\
					(((((colorIn & 0x00FF00FF) * alpha0) +										\
						color0) >> 8) & 0x00FF00FF) |											\
					(((((colorIn & 0x0000FF00) * alpha0) +										\
						color8) >> 8) & 0x0000FF00) ;											\
				dst_p