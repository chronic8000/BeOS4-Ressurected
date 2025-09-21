extern "C" void Rotate4x4x16( void *src, int32 srcStride, void *dst, int32 dstStride );
extern "C" void RotateNxNx16( void *src, int32 srcStride, void *dst, int32 dstStride, int32 hcount, int32 vcount );
extern "C" void Combo4x4x16x16( void *src, int32 srcStride, void *dst, int32 dstStride,
							 void *src2, void *dst2, void *prefetch);
extern "C" void Combo4x4x16( void *src, int32 srcStride, void *dst, void *src2, void *dst2);

static void RotatedBlitCopy16Slow (
	uint8 *src, int32 srcBytesPerRow,
	uint8 *dst, int32 dstBytesPerRow,
	int32 width, int32 height
) {
	int32		count;
	uint8		*srcPtr,*dstPtr;
	uint32		tmp;
	uint32		*dest32;

	while (height--) {
		count = width;
		srcPtr = src;
		dstPtr = dst;
		if ((count>0) && (((uint32)dstPtr) & 0x02)) {
			*((uint16*)dstPtr) = *((uint16*)srcPtr);
			dstPtr += sizeof(uint16); srcPtr -= srcBytesPerRow; count--;
		};
		dest32 = (uint32*)dstPtr;
		while (count>=4) {
			tmp = *((uint16*)srcPtr); srcPtr -= srcBytesPerRow;
			tmp |= *((uint16*)srcPtr)<<16; srcPtr -= srcBytesPerRow;
			*dest32++ = tmp;
			tmp = *((uint16*)srcPtr); srcPtr -= srcBytesPerRow;
			tmp |= *((uint16*)srcPtr)<<16; srcPtr -= srcBytesPerRow;
			*dest32++ = tmp;
			count -= 4;
		};
		if (count>=2) {
			tmp = *((uint16*)srcPtr); srcPtr -= srcBytesPerRow;
			tmp |= *((uint16*)srcPtr)<<16; srcPtr -= srcBytesPerRow;
			*dest32++ = tmp;
			count -= 2;
		};
		if (count>0) {
			*((uint16*)dest32) = *((uint16*)srcPtr);
		};
		src += sizeof(uint16);
		dst += dstBytesPerRow;
	};
}

static uint8 RotatedBlitCopy16Fast (
	uint8 *src, int32 srcBytesPerRow,
	uint8 *dst, int32 dstBytesPerRow,
	int32 width, int32 height
) {
	int32		dst_col[3], dst_line[3];
	int32		count, i, cell_count, cell_index;
	uint8		or_mask;
	uint8		buffer[16*16*2*2];
	uint8		*prefetch_src, *rotate_src, *rotate_dst, *copy_src, *copy_dst;
	
	/* No FPU instructions allowed in this function (MMX being used without emms) */
	or_mask = 0;
	// how many cells in the matrix total
	cell_count = (width*height)>>8;	
	// first two partial pass (in one)
		// init the cell pointer for the prefetch on the second cell
		if (width == 16) {
			dst_col[0] = 0;
			dst_line[0] = 16;
		}
		else {
			dst_col[0] = 16;
			dst_line[0] = 0;
		}
		// init the cell pointer for rotation on the first cell
		dst_col[1] = 0;
		dst_line[1] = 0;
		// run prefetch and rotation pass
		prefetch_src = (uint8*)(src - srcBytesPerRow*dst_col[0]) + dst_line[0]*2;
		rotate_src = (uint8*)(src - srcBytesPerRow*(dst_col[1]+3)) + dst_line[1]*2;
		rotate_dst = buffer;
		for (count=0; count<16; count++) {
			or_mask |= prefetch_src[0];
			prefetch_src -= srcBytesPerRow;
			Rotate4x4x16( rotate_src, srcBytesPerRow,
			              rotate_dst, 16*2);
			rotate_src += 4*2;
			rotate_dst += 16*2*4;
			if ((count & 3) == 3) {
				rotate_src -= 4*srcBytesPerRow + 16*2;
				rotate_dst -= 16*2*4*4 - 4*2;
			}
		}
	// run all the fully interlaced pass
	for (cell_index=2; ; cell_index++) {
		// increment cell index
		dst_col[2] = dst_col[1];
		dst_line[2] = dst_line[1];
		dst_col[1] = dst_col[0];
		dst_line[1] = dst_line[0];
		dst_col[0] += 16;
		if (dst_col[0] >= width) {
			dst_col[0] = 0;
			dst_line[0] += 16;
		}
		// update pointers
		prefetch_src = (uint8*)(src - srcBytesPerRow*dst_col[0]) + dst_line[0]*2;
		rotate_src = (uint8*)(src - srcBytesPerRow*(dst_col[1]+12+3)) + dst_line[1]*2;
		rotate_dst = buffer+16*16*2*((1+cell_index)&1)+12*2;
		copy_src = buffer+16*16*2*(cell_index&1);
		copy_dst = (uint8*)(dst+dstBytesPerRow*dst_line[2]) + dst_col[2]*2;
		// exit condition
		if (cell_index == cell_count)
			break;
		// run prefetch, rotation and copy pass
		Combo4x4x16x16( rotate_src, srcBytesPerRow, rotate_dst, dstBytesPerRow-24,
					 copy_src, copy_dst, prefetch_src);

	}
	// does the rotate and copy pass
	for (count=0; count<16; count++) {
		Combo4x4x16(rotate_src, srcBytesPerRow, rotate_dst, copy_src, copy_dst);
		copy_src += 16*2;
		copy_dst += dstBytesPerRow;
		rotate_src += 4*srcBytesPerRow;
		rotate_dst -= 4*2;
		if ((count & 3) == 3) {
			rotate_src -= 16*srcBytesPerRow - 4*2;
			rotate_dst += 16*2*4 + 16*2;
		}
	}
	// does the last copy pass
	copy_src = buffer+16*16*2*((1+cell_count)&1);
	copy_dst = (uint8*)(dst+dstBytesPerRow*dst_line[1]) + dst_col[1]*2;
	for (count=0; count<16; count++) {
		for (i=0; i<8; i++)
			((uint32*)copy_dst)[i] = ((uint32*)copy_src)[i];
		copy_src += 16*2;
		copy_dst += dstBytesPerRow;
	}
	return or_mask;	
}

static void RotatedBlitCopySameFormat16 (
	RenderContext *context,
	RenderSubPort *port,
	Pixels *srcPixMap, Pixels *dstPixMap,
	uint8 *src, int32 srcBytesPerRow,
	uint8 *dst, int32 dstBytesPerRow,
	int32 width, int32 fixedX, int32 incX,
	int32 height, int32 errorY, int32 incY, int32 defaultPtrIncY,
	int32 bytesPerPixel, int32 srcOffs, int32 directionFlags
) {
	int32		count;

	if ((width >= 19) && (height >= 31) && (width*height > 3000)) {
		/* align on the right side */
		if ((int32)src & 31) {
			count = 32-((int32)src & 31);
			RotatedBlitCopy16Slow(src, srcBytesPerRow, dst, dstBytesPerRow, width, count>>1);
			height -= count>>1;
			src += count;
			dst += (count>>1)*dstBytesPerRow;
		} 
		/* align on the left side */
		if (height & 15) {
			count = height & 15;
			RotatedBlitCopy16Slow(src+(height-count)*2, srcBytesPerRow,
								  dst+dstBytesPerRow*(height-count), dstBytesPerRow, width, count);
			height -= count;
		} 
		/* align on the bottom side */
		if ((int32)dst & 7) {
			count = (8-((int32)dst & 7)) >> 1;
			RotatedBlitCopy16Slow(src, srcBytesPerRow, dst, dstBytesPerRow, count, height);
			width -= count;
			dst += 2*count;
			src -= srcBytesPerRow*count;
		}
		/* align on the top side */
		if (width & 15) {
			count = width & 15;
			RotatedBlitCopy16Slow(src-srcBytesPerRow*(width-count), srcBytesPerRow,
								  dst+2*(width-count), dstBytesPerRow,
								  count, height);
			width -= count;
		}
		/* handle center, using MMX code */
		RotatedBlitCopy16Fast(src, srcBytesPerRow, dst, dstBytesPerRow, width, height);
		/* No FPU instructions allowed in the previous function call, as it's
		   using MMX and don't call emms */
		__asm__ __volatile__( "emms \n\t" );
	}
	else
		RotatedBlitCopy16Slow(src, srcBytesPerRow, dst, dstBytesPerRow, width, height);
};
