
#undef DrawPixel
#undef DrawPixelSpan
#undef RenderOp
#undef RenderOpMasked
#undef InitialSetupPattern
#undef SetupY

#if		(RenderMode==DEFINE_OP_COPY)

#define RenderOp(a,b) 				b
#define RenderOpMasked(a,b,mask)	((a&(~mask)) | (b&mask))

#elif	(RenderMode==DEFINE_OP_OVER)

#define RenderOp(a,b)				(a | b)
#define RenderOpMasked(a,b,mask)	(a | (b&mask))

#elif	(RenderMode==DEFINE_OP_ERASE)

#define RenderOp(a,b)				(a & ~b)
#define RenderOpMasked(a,b,mask)	(a & ~(b&mask))

#elif	(RenderMode==DEFINE_OP_INVERT)

#define RenderOp(a,b)				(a ^ b)
#define RenderOpMasked(a,b,mask)	(a ^ (b&mask))

#endif

#if (RenderPattern==1)

#define DrawPixel(p,x,y)														\
{																				\
		pixelPattern = pat.data[(phaseY+y)&0x7];								\
		pixelPattern = (pixelPattern<<phaseX) | (pixelPattern>>(8-phaseX));		\
		uint8 *atP = (p) + (x>>3);												\
		*(atP) = RenderOpMasked((*atP),pixelPattern,(0x80 >> (x&0x7)));	\
}

#define InitialSetupPattern								\
		pattern pat = context->stipplePattern;			\
		int32 phaseY = port->phase_y;				\
		int32 phaseX = port->phase_x;				

#define SetupY(y)										\
		pixelPattern = pat.data[(phaseY+y)&0x7];		\
		pixelPattern |= pixelPattern<<8;				\
		pixelPattern |= pixelPattern<<16;				\
		pixelPattern = (pixelPattern << phaseX) |		\
					   (pixelPattern >> 32-phaseX);

#else

#define DrawPixel(p,x,y)														\
{																				\
		uint8 *atP = (p) + (x>>3);												\
		*(atP) = RenderOpMasked((*atP),pixelPattern,(0x80 >> (x&0x7)));	\
}

#define InitialSetupPattern					\
		pixelPattern = (context->solidColor==0)?0xFFFFFFFF:0x00000000;

#define SetupY(y)

#endif

#define DrawPixelSpan(rowBase,x,count)								\
{ 																	\
	uint8 *pix,bitOffs;												\
	uint32 pixVal,tmpPix,mask;										\
																	\
	pix = rowBase+(x>>3);											\
	bitOffs = x & 0x7;												\
	if (count < (8-bitOffs)) goto last;								\
	mask = 0xFF >> bitOffs;											\
	*pix++ = RenderOpMasked((*pix),pixelPattern,mask);				\
	count -= (8-bitOffs);											\
	while ((count >=8) && ((uint32)pix) & 0x3) {					\
		*pix = RenderOp((*pix),pixelPattern);						\
		pix++; count-=8;											\
	};																\
	while (count >= 32) {											\
		*((uint32*)pix) = RenderOp(*((uint32*)pix),pixelPattern);	\
		pix+=4; count-=32;											\
	};																\
	while (count >=8) {												\
		*pix++ = RenderOp((*pix),pixelPattern);						\
		count-=8;													\
	};																\
	if (count > 0) {												\
		bitOffs = 0;												\
		last:														\
		mask = ((0xFF << (8-count)) & 0xFF ) >> bitOffs;			\
		*pix++ = RenderOpMasked((*pix),pixelPattern,mask);			\
	};																\
}

#define PointInRect(x,y) 																\
	((x >= rleft) && 																	\
	 (x <= rright) &&																	\
	 (y >= rtop) &&																	\
	 (y <= rbottom))

#ifdef FillRectsFunctionName

static void FillRectsFunctionName
(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount
)
{
	region *clipRegion = port->RealDrawingRegion();
	int32	rowBytes=port->canvas->pixels.bytesPerRow,rc;
	uint8	*p,*base=((uint8*)port->canvas->pixels.pixelData);
	int32	i,width,height,pixelInc,x1,x2,y1,y2;
	uint32	pixelPattern;
	int32	clips;
	const rect	*curSrcRect=fillRects,*curClipRect;
	point	offset = port->origin;

	grAssertLocked(context, port);
	
	InitialSetupPattern;

	while (rectCount) {
		curClipRect = clipRegion->Rects();
		clips = clipRegion->CountRects();
		while (clips) {
			x1 = curSrcRect->left + offset.h;
			x2 = curSrcRect->right + offset.h;
			y1 = curSrcRect->top + offset.v;
			y2 = curSrcRect->bottom + offset.v;
			if (curClipRect->top > y1) y1 = curClipRect->top;
			if (curClipRect->bottom < y2) y2 = curClipRect->bottom;
			if (curClipRect->left > x1) x1 = curClipRect->left;
			if (curClipRect->right < x2) x2 = curClipRect->right;
			if ((y1 <= y2) && (x1 <= x2)) {
				height = (y2-y1+1);
				width = (x2-x1+1);
				p = base+(y1*rowBytes);
				while (y1<=y2) {
					i = width;
					SetupY(y1);
					DrawPixelSpan(p,x1,i);
					y1++; p += rowBytes;
				};
			};
			clips--;
			curClipRect++;
		};
		rectCount--;
		curSrcRect++;
	};
};

#endif

#ifdef FillSpanFunctionName

static void FillSpanFunctionName
(
	RenderContext *context,									
	RenderSubPort *port,
	int32 x1, int32 x2, int32 y1,							
	region *clipRegion)									
{
	int32			rowBytes=port->canvas->pixels.bytesPerRow;
	uint8			*base=((uint8*)port->canvas->pixels.pixelData)+(y1*rowBytes);
	int32			rc,i,x1Backup,x2Backup;
	rect			*clipRect;
	uint32			pixelPattern;

	grAssertLocked(context, port);
	
	InitialSetupPattern;
	SetupY(y1);

	if (x2 < x1) {
		i = x2;
		x2 = x1;
		x1 = i;
	}

	if (!clipRegion) goto intoIt;

	x1Backup = x1;
	x2Backup = x2;
	rc = clipRegion->count;
	if (!rc) return;
	clipRect = clipRegion->data;

back:

	if (x1 < clipRect->left) x1 = clipRect->left;
	if (x2 > clipRect->right) x2 = clipRect->right;
	if ((y1 < clipRect->top)||(y1 > clipRect->bottom)||(x1>x2)) {
		rc--; clipRect++;
		if (!rc) return;
		x1 = x1Backup;
		x2 = x2Backup;
		goto back;
	};
	
intoIt:

	x2 = x2-x1+1;
	if (x2 > 0) {
		DrawPixelSpan(base,x1,x2);
	};
	
	if (!clipRegion) return;
	
	rc--; clipRect++;
	if (!rc) return;
	x1 = x1Backup;
	x2 = x2Backup;
	goto back;
}

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
	grAssertLocked(context, port);
	
	uint8 *p;
	uint32 pixelPattern;
	InitialSetupPattern;

	while (numPixels--) {
		p = base + (bpr*(y>>16));
		DrawPixel(p,(x>>16),(y>>16));
		x += xInc; y += yInc;
	};
};

#if 0
static void DrawOneLineFunctionName
(
 	RenderContext *context,
	RenderSubPort *port,
 	uint8 *base,
 	int32 signX, int32 signY,
 	int32 error0, int32 errorInc,
 	int32 sx, int32 sy,
 	int32 *ex, int32 *ey,
 	int32 count0, int32 bpr,
 	int32 ix, int32 iy,
	uint32 flags
)
{
	uint8 *p;
	int32 	j,yInc = signY*bpr,
			rleft,rright,rtop,rbottom;
	bool sir,eir;
	int32 count,error;
	uint32 pixelPattern;
	region *clip = port->RealDrawingRegion();
	InitialSetupPattern;
	
	if (flags & LINEFLAG_DRAWPOINT) {
		p = base + (bpr*iy);
		for (j=0;j<clip->count;j++) {
			rleft = clip->data[j].left;
			rright = clip->data[j].right;
			rtop = clip->data[j].top;
			rbottom = clip->data[j].bottom;
			if (PointInRect(ix,iy)) {
				DrawPixel(p,ix,iy);
				break;
			};
		};
	};
			
	if (!(flags & LINEFLAG_DRAWLINE)) return;

	base = base + (bpr*sy);
	for (j=0;j<clip->count;j++) {
		ix = sx; iy = sy;
		p = base;
		count = count0;
		error = error0;
		rleft = clip->data[j].left;
		rright = clip->data[j].right;
		rtop = clip->data[j].top;
		rbottom = clip->data[j].bottom;
		sir = PointInRect(sx,sy);
		eir = PointInRect(*ex,*ey);
		if (sir && eir) {
			if (flags & LINEFLAG_XMAJOR) {
				if (count > 0) goto intoIt1;
				while (count > 0) {
					ix+=signX;
					error += errorInc;
					if (error >= 0x10000) {
						error -= 0x10000;
						p+=yInc; iy+=signY;
					};
					intoIt1:
					count--;
					DrawPixel(p,ix,iy);
				};
			} else {
				if (count > 0) goto intoIt2;
				while (count > 0) {
					p+=yInc; iy+=signY;
					error += errorInc;
					if (error >= 0x10000) {
						error -= 0x10000;
						ix+=signX;
					};
					intoIt2:
					count--;
					DrawPixel(p,ix,iy);
				};
			};
		} else {
			if (flags & LINEFLAG_XMAJOR) {
				if (count > 0) goto intoIt12;
				while (count > 0) {
					ix+=signX;
					error += errorInc;
					if (error >= 0x10000) {
						error -= 0x10000;
						p+=yInc; iy+=signY;
					};
					intoIt12:
					count--;
					if (PointInRect(ix,iy)) { DrawPixel(p,ix,iy); }
				};
			} else {
				if (count > 0) goto intoIt22;
				while (count > 0) {
					p+=yInc; iy+=signY;
					error += errorInc;
					if (error >= 0x10000) {
						error -= 0x10000;
						ix += signX;
					};
					intoIt22:
					count--;
					if (PointInRect(ix,iy)) { DrawPixel(p,ix,iy); }
				};
			};
		};
		*ex = ix;
		*ey = iy;
	};
};
#endif

#endif

#undef DrawOneLineFunctionName
#undef FillSpanFunctionName
#undef FillRectsFunctionName
