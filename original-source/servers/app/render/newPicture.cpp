
//******************************************************************************
//
//	File:			newPicture.cpp
//
//	Description:	The new server-side picture object
//	
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "renderInternal.h"
#include "newPicture.h"
#include "renderContext.h"
#include "renderPort.h"
#include "renderCanvas.h"
#include "renderLock.h"
#include "accelPackage.h"
#include "font_cache.h"
#include "edges.h"
#include "shape.h"
#include "lines.h"
#include "fastmath.h"
#include "renderArc.h"
#include "newPicture.h"
#include "render.h"
#include "rect.h"
#include "bitmap.h"
#include "../token.h"

#include <Debug.h>
#include <IRegion.h>
#include <unistd.h>

//#define P(a) DebugPrintf(a)
#define P(a)

extern	TokenSpace		*tokens;
extern bool IsValidGraphicsType(uint32 type);
extern void GraphicsTypeToInternal(uint32 type, int32 *pixelFormat, uint8 *endianess);

long	db_file = 0;

/*
int32 SPicture::ServerLock()
{
	int32 r = SAtom::ServerLock();
	DebugPrintf(("%08x: SPicture::ServerLock(%ld) ",this,r));
	CallStack stack;
	stack.Update();
	stack.Print();
	DebugPrintf(("\n"));
	return r;
}

int32 SPicture::ServerUnlock()
{
	int32 r = SAtom::ServerUnlock();
	DebugPrintf(("%08x: SPicture::ServerUnlock(%ld) ",this,r));
	CallStack stack;
	stack.Update();
	stack.Print();
	DebugPrintf(("\n"));
	return r;
}
*/
void uprintf(char *format, ...)
{
	va_list args;
	char buf[255];

	if (db_file == 0) {
		db_file = open("/boot/db_file", O_RDWR);
	}

	va_start(args,format);
	vsprintf (buf, format, args);
	va_end(args);
	write(db_file, buf, strlen(buf));
}

int
SPicture::DiskMe(char *filename, long offset)
{
	P(("disk me %s %ld\n", filename, offset));
	file_ref = open(filename, O_RDWR);
	readable_length = 0;
	
	if (file_ref > 0) {
		readable_length = 0;
		lseek(file_ref, offset, SEEK_SET);				//skip picture length... will fill after
		read(file_ref, &readable_length, 4);
		P(("reabdle length=%ld\n", readable_length));
		lseek(file_ref, offset + 4, SEEK_SET);			//skip picture length... will fill after
		disk_picture = true;
	}
	written_length = 0;
	disk_writes = false;

	picture_start_offset = offset + 4;
	disk_buffer = 0;
	return 0;
};


SPicture::SPicture(SPicture *pic) : opBegin(8)
{
	P(("SPicture::SPicture\n"));
	data = NULL;
	dataLen = dataPtr = 0;
	disk_picture = false;
	needInitialState = true;
	pictureStack = pic;
	updatePenLocation = false;
	m_rasterized.rects = newregion();
	m_rasterized.inverseRects = NULL;
	m_rasterized.bounds.left = 
	m_rasterized.bounds.top = 0x0FFFFFFF;
	m_rasterized.bounds.right = 
	m_rasterized.bounds.bottom = -0x0FFFFFFF;
	m_rasterized.scale = 0;
	disk_buffer = 0;
	server_token = NO_TOKEN;
	P(("SPicture::SPicture done\n"));
};


void SPicture::Restart(SPicture *oldPicture)
{
	P(("SPicture::Restart(%08x)\n",oldPicture));
	if (data)
		grFree(data);

	/*
	 *	Just steal the old picture's data buffer
	 */
	data = oldPicture->data;
	dataLen = oldPicture->dataLen;
	dataPtr = oldPicture->dataPtr;
	oldPicture->data = 0;
	oldPicture->dataLen = 0;
	
	needInitialState = oldPicture->needInitialState;
	updatePenLocation = oldPicture->updatePenLocation;
	renderState = oldPicture->renderState;
	m_rasterized.inverseRects = NULL;
	m_rasterized.bounds.left = 
	m_rasterized.bounds.top = 0x0FFFFFFF;
	m_rasterized.bounds.right = 
	m_rasterized.bounds.bottom = -0x0FFFFFFF;
	m_rasterized.scale = 0;
	server_token = NO_TOKEN;
	for (int32 i = 0; i < pictureLibrary.CountItems(); i++)
		pictureLibrary[i]->ServerUnlock();
	
	pictureLibrary.SetItems(0);
	pictureLibrary.AddArray(&oldPicture->pictureLibrary);
	for (int32 i = 0;i < pictureLibrary.CountItems(); i++)
		pictureLibrary[i]->ServerLock();
}

void SPicture::Copy(SPicture *copy)
{
	P(("SPicture::Copy(%08x)\n",copy));
	if (data) grFree(data);
	data = (uint8*)grMalloc(copy->dataLen,"picture copy",MALLOC_CANNOT_FAIL);
	dataLen = dataPtr = copy->dataPtr;
	memcpy(data,copy->data,dataLen);
	needInitialState = copy->needInitialState;
	updatePenLocation = copy->updatePenLocation;
	renderState = copy->renderState;
	m_rasterized.inverseRects = NULL;
	m_rasterized.bounds.left = 
	m_rasterized.bounds.top = 0x0FFFFFFF;
	m_rasterized.bounds.right = 
	m_rasterized.bounds.bottom = -0x0FFFFFFF;
	m_rasterized.scale = 0;
	server_token = NO_TOKEN;

	for (int32 i=0;i<pictureLibrary.CountItems();i++)
		pictureLibrary[i]->ServerUnlock();
	
	pictureLibrary.SetItems(0);
	pictureLibrary.AddArray(&copy->pictureLibrary);

	for (int32 i=0;i<pictureLibrary.CountItems();i++)
		pictureLibrary[i]->ServerLock();

	P(("SPicture::Copy done\n"));
};

void SPicture::Usurp(SPicture *pic)
{
	pictureStack = pic;
	needInitialState = false;
	updatePenLocation = true;
};

SPicture::~SPicture()
{
	P(("SPicture::~SPicture %08x\n",this));

	if (server_token != NO_TOKEN)
		tokens->remove_token(server_token);
	
	if (disk_picture) {
		if (disk_writes) {
			lseek(file_ref, picture_start_offset - 4, SEEK_SET);			//skip picture length... will fill after
			write(file_ref, &written_length, sizeof(written_length));
		}
		close(file_ref);
	}

	if (data != NULL)
		grFree(data);

	for (int32 i=0;i<pictureLibrary.CountItems();i++)
		pictureLibrary[i]->ServerUnlock();

	if (m_rasterized.rects) kill_region(m_rasterized.rects);
	if (m_rasterized.inverseRects) kill_region(m_rasterized.inverseRects);

	P(("SPicture::~SPicture done\n"));
};

void * SPicture::Data()
{
	return data;
};

int32 SPicture::DataLength()
{
	return dataPtr;
};

void SPicture::SetDataLength(int32 len)
{
	AssertSpace(len);
	dataPtr = len;
};

void SPicture::BeginOp(int32 opCode)
{
	if (disk_picture) {
		opBegin.AddItem(written_length);
		AddInt16(opCode);
		AddInt32(0);
	}
	else {
		opBegin.AddItem(dataPtr);
		AddInt16(opCode);
		AddInt32(0);
	}
};

//---------------------------------------------------------------

void SPicture::EndOp()
{
	if (disk_picture) {
		long	tmp;

		int32 begin = opBegin[opBegin.CountItems()-1];
		opBegin.SetItems(opBegin.CountItems()-1);
		int32 *p = (int32*)(data+begin+2);
		tmp = written_length - begin - 6;
		//tmp = 0xaaaaaaaa;
		lseek(file_ref, picture_start_offset + begin + 2, SEEK_SET);
		write(file_ref, &tmp, sizeof(tmp));
		lseek(file_ref, picture_start_offset + written_length, SEEK_SET);
		return;
	}
	
	int32 begin = opBegin[opBegin.CountItems()-1];
	opBegin.SetItems(opBegin.CountItems()-1);
	int32 *p = (int32*)(data+begin+2);
	*p = dataPtr - begin - 6;
};

//---------------------------------------------------------------

void SPicture::EndAndPossiblyForgetOp()
{
	if (disk_picture) {
		int32 begin = opBegin[opBegin.CountItems()-1];
		opBegin.SetItems(opBegin.CountItems()-1);
		if ((written_length - begin - 6) == 0) {
		} else {

			int32	tmp;

			tmp = written_length - begin - 6;
			//tmp = 0xbbbbbbbb;
			lseek(file_ref, picture_start_offset + begin + 2, SEEK_SET);
			write(file_ref, &tmp, sizeof(tmp));
			lseek(file_ref, picture_start_offset + written_length, SEEK_SET);
		};
		return;
	}

	int32 begin = opBegin[opBegin.CountItems()-1];
	opBegin.SetItems(opBegin.CountItems()-1);
	if ((dataPtr - begin - 6) == 0) {
		dataPtr = begin;
	} else {
		int32 *p = (int32*)(data+begin+2);
		*p = dataPtr - begin - 6;
	};
};

//---------------------------------------------------------------

int32 SPicture::Token()
{
	return server_token;
};

void SPicture::SetToken(int32 token)
{
	server_token = token;
};

//---------------------------------------------------------------

void SPicture::AddDiskData(void *_data, int32 len)
{
	disk_writes = true;
	write(file_ref, _data, len);
	written_length += len;
}

//---------------------------------------------------------------

void SPicture::AddData(void *_data, int32 len)
{
	if (disk_picture) {
		AddDiskData(_data, len);
		return;
	}

	AssertSpace(len);
	memcpy(data+dataPtr,_data,len);	
	dataPtr += len;
};

//---------------------------------------------------------------

void SPicture::AddPicture(SPicture *pic)
{
	int32 token = -1;
	for (int32 i=0;i<pictureLibrary.CountItems();i++) {
		if (pictureLibrary[i] == pic) {
			token = i;
			break;
		};
	};
	
	if (token == -1) {
		token = pictureLibrary.CountItems();
		pic->ServerLock();
		pictureLibrary.AddItem(pic);
	};
	
	P(("AddPicture(0x%08x) - adding token = %d\n",pic,token));
	AddInt32(token);
};

//---------------------------------------------------------------

void SPicture::AddRect(frect value)
{
	AddData(&value,sizeof(frect));
};

void SPicture::AddIRect(rect value)
{
	AddData(&value,sizeof(rect));
};

//---------------------------------------------------------------

void SPicture::AddColor(rgb_color value)
{
	AddData(&value,sizeof(rgb_color));
};

//---------------------------------------------------------------

void SPicture::AddCoord(fpoint value)
{
	AddData(&value,sizeof(fpoint));
};

void SPicture::AddFloat(float value)
{
	AddData(&value,sizeof(float));
};

void SPicture::AddInt64(int64 value)
{
	AddData(&value,sizeof(int64));
};

void SPicture::AddInt32(int32 value)
{
	AddData(&value,sizeof(int32));
};

void SPicture::AddInt16(int16 value)
{
	AddData(&value,sizeof(int16));
};

void SPicture::AddInt8(int8 value)
{
	AddData(&value,sizeof(int8));
};

void SPicture::AddString(char *str)
{
	int32 len = strlen(str);
	AddInt32(len);
	AddData(str,len);
};

void SPicture::AddCoords(fpoint *values, int32 count)
{
	AddData(values,count*sizeof(fpoint));
};

void SPicture::AddFloats(float *values, int32 count)
{
	AddData(values,count*sizeof(float));
};

void SPicture::AddInt64s(int64 *values, int32 count)
{
	AddData(values,count*sizeof(int64));
};

void SPicture::AddInt32s(int32 *values, int32 count)
{
	AddData(values,count*sizeof(int32));
};

void SPicture::AddInt16s(int16 *values, int32 count)
{
	AddData(values,count*sizeof(int16));
};

void SPicture::AddInt8s(int8 *values, int32 count)
{
	AddData(values,count*sizeof(int8));
};

void SPicture::AssertSpace(int32 size)
{
	if (disk_picture)
		return;

	if ((dataLen-dataPtr) < size) {
		while ((dataLen-dataPtr) < size) {
			if (dataLen == 0)
				dataLen = 4;
			else
				dataLen *= 2;
		};
		data = (uint8*)grRealloc(data,dataLen,"picture data",MALLOC_CANNOT_FAIL);
	};
};

void SPicture::PenLocationChanged()
{
	updatePenLocation = true;
};

void SPicture::SyncState(RenderContext *context)
{
	const char*	tmp;
	int32		length;
#if USE_FONTS
	SFont		font = context->fontContext;
#endif

	BeginOp(SPIC_SET_STATE);

	if (needInitialState) {
		BeginOp(SPIC_LOCATION);
		AddCoord(context->penLocation);
		EndOp();

		BeginOp(SPIC_PEN_SIZE);
		AddFloat(context->penSize);
		EndOp();

		BeginOp(SPIC_LINE_MODE);
		AddInt16(context->capRule);
		AddInt16(context->joinRule);
		AddFloat(context->miterLimit);
		EndOp();

		BeginOp(SPIC_STIPPLE);
		AddInt64(*((int64*)&context->stipplePattern));
		EndOp();

		BeginOp(SPIC_DRAW_OP);
		AddInt16(context->drawOp);
		EndOp();
		
		BeginOp(SPIC_BLENDING_MODE);
		AddInt16(context->srcAlphaSrc);
		AddInt16(context->alphaFunction);
		EndOp();

		BeginOp(SPIC_SCALE);
		AddFloat(context->localScale);
		EndOp();

		BeginOp(SPIC_FORE_COLOR);
		AddColor(context->foreColor);
		EndOp();

		BeginOp(SPIC_BACK_COLOR);
		AddColor(context->backColor);
		EndOp();

		BeginOp(SPIC_ORIGIN);
		AddCoord(context->localOrigin);
		EndOp();

		if (context->localRegionClip) {
			BeginOp(SPIC_CLIP_TO_RECTS);
			region *r = context->localRegionClip;
			AddIRect(r->Bounds());
			const int32 clipCount = r->CountRects();
			const rect* clipRects = r->Rects();
			for (int32 i=0;i<clipCount;i++)
				AddIRect(clipRects[i]);
			EndOp();
		} else if (context->localPictureClip) {
			BeginOp(SPIC_CLIP_TO_PICTURE);
			AddPicture(context->localPictureClip);
			AddCoord(context->localPictureClipOffset);
			AddInt32(context->localPictureClipInverted);
			EndOp();
		} else {
			BeginOp(SPIC_CLEAR_CLIP);
			EndOp();
		};

		
#if USE_FONTS
		BeginOp(SPIC_FONT);
			
			// TO DO: Store as flattened font.
			
			tmp = font.FamilyName();
			if (tmp) {
				BeginOp(SPIC_FAMILY);
				length = strlen(tmp) + 1;
				AddInt32(length);
				AddData((void*)tmp, length);
				EndOp();
			}
	
			tmp = font.StyleName();
			if (tmp) {
				BeginOp(SPIC_STYLE);
				length = strlen(tmp) + 1;
				AddInt32(length);
				AddData((void*)tmp, length);
				EndOp();
			}
	
			BeginOp(SPIC_SIZE);
			AddFloat(font.Size());
			EndOp();

			BeginOp(SPIC_ENCODING);
			AddInt32(font.Encoding());
			EndOp();
	
			BeginOp(SPIC_SHEAR);
			AddFloat(font.ShearRaw());
			EndOp();
	
			BeginOp(SPIC_ROTATE);
			AddFloat(font.RotationRaw());
			EndOp();
	
			BeginOp(SPIC_SPACING);
			AddInt32(font.Spacing());
			EndOp();
	
			// Don't do this.  The font system will take care of
			// setting the bits per pixel correctly, based on the
			// current environment and font flags.
			#if 0
			BeginOp(SPIC_BPP);
			AddInt32(font.BitsPerPixel());
			EndOp();
			#endif
			
			BeginOp(SPIC_FLAGS);
			AddInt32(font.Flags());
			EndOp();
	
			BeginOp(SPIC_FACES);
			AddInt32(font.Face());
			EndOp();

		EndOp();
#endif
		grSaveState(context,&renderState);
		needInitialState = false;
	} else {
		if (updatePenLocation) {
			updatePenLocation = false;
			renderState.penLocation = context->penLocation;
			BeginOp(SPIC_LOCATION);
			AddCoord(context->penLocation);
			EndOp();
		};
	
		if (renderState.penSize != context->penSize) {
			renderState.penSize = context->penSize;
			BeginOp(SPIC_PEN_SIZE);
			AddFloat(context->penSize);
			EndOp();
		};
		if (	(renderState.capRule != context->capRule) ||
			(renderState.joinRule != context->joinRule) ||
			(renderState.miterLimit != context->miterLimit)) {
			renderState.capRule = context->capRule;
			renderState.joinRule = context->joinRule;
			renderState.miterLimit = context->miterLimit;
			BeginOp(SPIC_LINE_MODE);
			AddInt16(context->capRule);
			AddInt16(context->joinRule);
			AddFloat(context->miterLimit);
			EndOp();
		};
		if (*((int64*)&renderState.stipplePattern) != *((int64*)&context->stipplePattern)) {
			renderState.stipplePattern = context->stipplePattern;
			BeginOp(SPIC_STIPPLE);
			AddInt64(*((int64*)&context->stipplePattern));
			EndOp();
		};
		if (renderState.drawOp != context->drawOp) {
			renderState.drawOp = context->drawOp;
			BeginOp(SPIC_DRAW_OP);
			AddInt16(context->drawOp);
			EndOp();
		};
		if ((renderState.srcAlphaSrc != context->srcAlphaSrc) ||
			(renderState.alphaFunction != context->alphaFunction)) {
			BeginOp(SPIC_BLENDING_MODE);
			AddInt16(context->srcAlphaSrc);
			AddInt16(context->alphaFunction);
			EndOp();
		};
		if (renderState.localScale != context->localScale) {
			renderState.localScale = context->localScale;
			BeginOp(SPIC_SCALE);
			AddFloat(context->localScale);
			EndOp();
		};
		if (*((int32*)&renderState.foreColor) != *((int32*)&context->foreColor)) {
			renderState.foreColor = context->foreColor;
			BeginOp(SPIC_FORE_COLOR);
			AddColor(context->foreColor);
			EndOp();
		};
		if (*((int32*)&renderState.backColor) != *((int32*)&context->backColor)) {
			renderState.backColor = context->backColor;
			BeginOp(SPIC_BACK_COLOR);
			AddColor(context->backColor);
			EndOp();
		};
		if (	(renderState.localOrigin.h != context->localOrigin.h) ||
				(renderState.localOrigin.v != context->localOrigin.v) ) {
			renderState.localOrigin = context->localOrigin;
			BeginOp(SPIC_ORIGIN);
			AddCoord(context->localOrigin);
			EndOp();
		};
		
		if (!context->localRegionClip && !context->localPictureClip &&
			(renderState.localRegionClip || renderState.localPictureClip)) {
			BeginOp(SPIC_CLEAR_CLIP);
			EndOp();
			
			if (renderState.localRegionClip) kill_region(renderState.localRegionClip);
			renderState.localRegionClip = NULL;
			if (renderState.localPictureClip) renderState.localPictureClip->ServerUnlock();
			renderState.localPictureClip = NULL;
		} else {
			if (context->localRegionClip) {
				bool clipChanged = false;
				if (!renderState.localRegionClip) {
					clipChanged = true;
				} else {
					if ((*renderState.localRegionClip) != (*context->localRegionClip)) {
						clipChanged = true;
					};
				};
				
				if (clipChanged) {
					BeginOp(SPIC_CLIP_TO_RECTS);
					if (context->localRegionClip) {
						region *r = context->localRegionClip;
						AddIRect(r->Bounds());
						const int32 clipCount = r->CountRects();
						const rect* clipRects = r->Rects();
						for (int32 i=0;i<clipCount;i++)
							AddIRect(clipRects[i]);
					};
					EndOp();
					grCopyRegion(&context->localRegionClip,&renderState.localRegionClip);
				};

				if (renderState.localPictureClip) renderState.localPictureClip->ServerUnlock();
				renderState.localPictureClip = NULL;
			} else if (context->localPictureClip) {
				if ((context->localPictureClip != renderState.localPictureClip) ||
					(context->localPictureClipInverted != renderState.localPictureClipInverted) ||
					(context->localPictureClipOffset.h != renderState.localPictureClipOffset.h) ||
					(context->localPictureClipOffset.v != renderState.localPictureClipOffset.v)) {
					BeginOp(SPIC_CLIP_TO_PICTURE);
					AddPicture(context->localPictureClip);
					AddCoord(context->localPictureClipOffset);
					AddInt32(context->localPictureClipInverted);
					EndOp();
					
					SPicture *oldPic = renderState.localPictureClip;
					renderState.localPictureClip = context->localPictureClip;
					renderState.localPictureClipInverted = context->localPictureClipInverted;
					renderState.localPictureClipOffset = context->localPictureClipOffset;
					if (renderState.localPictureClip) renderState.localPictureClip->ServerLock();
					if (oldPic) oldPic->ServerUnlock();
				};
				if (renderState.localRegionClip) kill_region(renderState.localRegionClip);
				renderState.localRegionClip = NULL;
			};
		};

#if USE_FONTS
		BeginOp(SPIC_FONT);
		
		if (renderState.fontContext.Size() != font.Size()) {
			BeginOp(SPIC_SIZE);
			AddFloat(font.Size());
			EndOp();
			renderState.fontContext.SetSize(font.Size());
		}	
		
		if ((renderState.fontContext.StyleID() != font.StyleID()) ||
			(renderState.fontContext.FamilyID() != font.FamilyID())) {
			const char 	*tmp;
			long	length;
			
			tmp = font.FamilyName();
			if (tmp) {
				BeginOp(SPIC_FAMILY);
				AddString((char*)tmp);
				EndOp();
			}

			tmp = font.StyleName();
			if (tmp) {
				BeginOp(SPIC_STYLE);
				AddString((char*)tmp);
				EndOp();
			}

			renderState.fontContext.SetFamilyID(font.FamilyID());
			renderState.fontContext.SetStyleID(font.StyleID());
		}
	
		if (renderState.fontContext.Encoding() != font.Encoding()) {
			renderState.fontContext.SetEncoding(font.Encoding());
			BeginOp(SPIC_ENCODING);
			AddInt32(font.Encoding());
			EndOp();
		}
	
		if (renderState.fontContext.ShearRaw() != font.ShearRaw()) {
			renderState.fontContext.SetShearRaw(font.ShearRaw());
			BeginOp(SPIC_SHEAR);
			AddFloat(font.Shear());
			EndOp();
		}
	
		if (renderState.fontContext.RotationRaw() != font.RotationRaw()) {
			renderState.fontContext.SetRotationRaw(font.RotationRaw());
			BeginOp(SPIC_ROTATE);
			AddFloat(font.Rotation());
			EndOp();
		}
	
		if (renderState.fontContext.Spacing() != font.Spacing()) {
			renderState.fontContext.SetSpacing(font.Spacing());
			BeginOp(SPIC_SPACING);
			AddInt32(font.Spacing());
			EndOp();
		}
	
		// Don't do this.  The font system will take care of
		// setting the bits per pixel correctly, based on the
		// current environment and font flags.
		#if 0
		if (renderState.fontContext.BitsPerPixel() != font.BitsPerPixel()) {
			renderState.fontContext.SetBitsPerPixel(font.BitsPerPixel());
			BeginOp(SPIC_BPP);
			AddInt32(font.BitsPerPixel());
			EndOp();
		}
		#endif
	
		if (renderState.fontContext.Flags() != font.Flags()) {
			renderState.fontContext.SetFlags(font.Flags());
			BeginOp(SPIC_FLAGS);
			AddInt32(font.Flags());
			EndOp();
		}
	
		if (renderState.fontContext.Face() != font.Face()) {
			renderState.fontContext.SetFace(font.Face());
			BeginOp(SPIC_FACES);
			AddInt32(font.Face());
			EndOp();
		}
	
		EndAndPossiblyForgetOp();
#endif
	};
	
	EndAndPossiblyForgetOp();
};

bool SPicture::Verify()
{
	return true;
};

void SPicture::RecordStatePush(RenderContext *context)
{
	BeginOp(SPIC_PUSH_STATE);
	EndOp();
	if (renderState.localPictureClip) {
		renderState.localPictureClip->ServerUnlock();
		renderState.localPictureClip = NULL;
	};
	grCopyRegion(&context->localRegionClip,&renderState.localRegionClip);
	grCopyRegion(&context->globalEffective_Clip,&renderState.globalEffective_Clip);
};

void SPicture::RecordStatePop(RenderContext *context)
{
	BeginOp(SPIC_POP_STATE);
	EndOp();
	grSaveState(context,&renderState);
};

SPicture * SPicture::OlderPicture()
{
	SPicture *p = pictureStack;
	pictureStack = NULL;
	return p;
};

void KiltSort(int32 *smin, int32 *smax)
{
	int32   sref[3];
	int32   *smin2, *smax2;
	int32   index;
	int32    offset;
	static uint32 crc_alea = 0x4589ef71;
	
	// init sorting
	smin2 = smin;
	smax2 = smax;
	// randomize the quicksort
	index = (smax-smin)/3;
	if (index > 2) {	
		if ((crc_alea <<= 1) < 0)
			crc_alea ^= 0x71a3;
		index = (index>>1)|(index>>2);
		index |= index>>2;
		index &= (crc_alea>>11);
		offset = (-1-index)*3;
		sref[0] = smax2[offset];
		sref[1] = smax2[offset+1];
		sref[2] = smax2[offset+2];
		smax2[offset] = smax2[0];
		smax2[offset+1] = smax2[1];
		smax2[offset+2] = smax2[2];
	}
	else {
		sref[0] = smax2[0];
		sref[1] = smax2[1];
		sref[2] = smax2[2];
	}

	// do iterative sorting
	while (smin2 != smax2) {
		if ((smin2[0] > sref[0]) ||
			((smin2[0] == sref[0]) && (smin2[1] > sref[1]))) {
			smax2[0] = smin2[0];
			smax2[1] = smin2[1];
			smax2[2] = smin2[2];
			smax2-=3;
		loop_min:
			if (smin2 == smax2) break;
			if ((smax2[0] > sref[0]) ||
				((smax2[0] == sref[0]) && (smax2[1] >= sref[1]))) {
				smax2-=3;
				goto loop_min;
			}
			smin2[0] = smax2[0];
			smin2[1] = smax2[1];
			smin2[2] = smax2[2];
		}
		smin2+=3;
	}

	// put the reference back
	smin2[0] = sref[0];
	smin2[1] = sref[1];
	smin2[2] = sref[2];

	// do recursive sorting
	if (smin2-3 > smin)
		KiltSort(smin, smin2-3);
	if (smax2+3 < smax)
		KiltSort(smax2+3, smax);
}

struct SpanCanvas : public RenderCanvas {
	BArray<int32> spans;
	region *tmpRegion;
};

static void CollectLine(
 	RenderContext *context,
	RenderSubPort *port,
 	uint8 *base, int32 bpr,
	int32 numPixels,
	int32 x, int32 y,
	int32 xInc, int32 yInc)
{
	int32 i,*p;
	BArray<int32> &spans = ((SpanCanvas*)port->canvas)->spans;

	i = spans.CountItems();
	spans.SetItems(i+numPixels*3);
	p = spans.Items()+i;
	while (numPixels--) {
		*p++ = y>>16;
		*p++ = x>>16;
		*p++ = x>>16;
		x += xInc; y += yInc;
	};
};

static void CollectRects(
	RenderContext *context,
	RenderSubPort *port,
	rect *fillRects, int32 rectCount)
{
	int32 moreSpans;
	int32 x,y,j;
	BArray<int32> &spans = ((SpanCanvas*)port->canvas)->spans;
	region reg,*tmpRegion = ((SpanCanvas*)port->canvas)->tmpRegion;

	rect* buf = reg.CreateRects(rectCount, rectCount);
	if (!buf) return;
	reg.SetRectCount(rectCount);
	memcpy(buf, fillRects, rectCount*sizeof(rect));

	reg.Bounds() = fillRects[0];
	reg.Bounds().bottom = fillRects[rectCount-1].bottom;
	
	// XXXX Debug
	x = fillRects[0].left;
	y = fillRects[0].top;
/*	DebugPrintf(("  (%d,%d,%d,%d)\n",
		fillRects[0].left,
		fillRects[0].top,
		fillRects[0].right,
		fillRects[0].bottom));
*/	reg.Bounds() = fillRects[0];
	for (int32 i=1;i<rectCount;i++) {
/*		DebugPrintf(("  (%d,%d,%d,%d)\n",
			fillRects[i].left,
			fillRects[i].top,
			fillRects[i].right,
			fillRects[i].bottom));
*/		if (fillRects[i].left < reg.Bounds().left)
			reg.Bounds().left = fillRects[i].left;
		if (fillRects[i].right > reg.Bounds().right)
			reg.Bounds().right = fillRects[i].right;
		if (fillRects[i].bottom > reg.Bounds().bottom)
			reg.Bounds().bottom = fillRects[i].bottom;
/*
		if (fillRects[i].top == y) {
			if (fillRects[i].left < x) {
				debugger("Left to right ordering compromised!\n");
				goto after;
			};
		} else if (fillRects[i].top < y) {
			debugger("Top to bottom ordering compromised!\n");
			goto after;
		};
		x = fillRects[i].left;
		y = fillRects[i].top;
*/
	};
	after:
	
	and_region(&reg,port->drawingClip,tmpRegion);

	const rect* destRects = tmpRegion->Rects();
	rectCount = tmpRegion->CountRects();
	
	for (int32 i=0;i<rectCount;i++) {
		moreSpans = destRects[i].bottom - (y=destRects[i].top) + 1;
		j = spans.CountItems();
		spans.SetItems(j+(moreSpans*3));
		while (moreSpans--) {
			spans[j] = y;
			spans[j+1] = destRects[i].left;
			spans[j+2] = destRects[i].right;
			j+=3; y++;
		};
	};
};

static void CollectChar(
	rect dst,
	uchar *src_base,
	rect src,
	BArray<int32> &spans)
{
	int        x, y, i, j, cmd, hmin, hmax, imin, imax, imed;
	uchar      *dst_ptr;

	y = dst.top;
	x = dst.left-src.left;
	/* pass through the top lines that are clipped */
	for (j=0; j<src.top; j++)
		do src_base++; while (src_base[-1] != 0xff);
	/* for each visible lines, draw the visible runs */
	for (; j<=src.bottom; j++) {
		hmin = 0;
		while (TRUE) {
			/* decode next run */
			do {
				cmd = *src_base++;
				/* end of run list for this line */
				if (cmd == 0xff)
					goto next_line;
				hmin += cmd;
			} while (cmd == 0xfe);
			hmax = hmin;
			do {
				cmd = *src_base++;
				hmax += cmd;
			} while (cmd == 0xfe);
			/* horizontal clipping */
			if (hmin > src.right) {
				do src_base++; while (src_base[-1] != 0xff);
				goto next_line;
			}
			if (hmin >= src.left)
				imin = hmin;
			else
				imin = src.left;
			if (hmax <= src.right)
				imax = hmax;
			else
				imax = src.right+1;
			spans.AddItem(y);
			spans.AddItem(x+imin);
			spans.AddItem(x+imax-1);
			hmin = hmax;
		}
	next_line:
		y++;
	}
}

static void CollectString(
	RenderContext *	context,
	RenderSubPort * port,
	fc_string *		str)
{
	int			i, r, g, b, index;
	int			cur_row, energy0;
	int			mode_direct;
	region *	clip = port->drawingClip;
	const rect	*clipRects = clip->Rects();
	const int32	rc=clip->CountRects();
	int32		ri;
	rect		cur, src,clipbox;
	uint		colors;
	uchar *		src_base, *cur_base;
	fc_char *	my_char;
	fc_point *	my_offset;
	BArray<int32> &spans = ((SpanCanvas*)port->canvas)->spans;

	for (ri=0;ri<rc;ri++) {
		clipbox = clipRects[ri];
		for (i=0; i<str->char_count; i++) {
			my_char = str->chars[i];
			my_offset = str->offsets+i;
			if ((my_offset->x < -100000) ||
				(my_offset->x > +100000) ||
				(my_offset->y < -100000) ||
				(my_offset->y > +100000))
				continue;
			/* calculate character horizontal limits */
			cur.left = my_char->bbox.left + my_offset->x;
			cur.right = my_char->bbox.right + my_offset->x;
			src.left = 0;
			src.right = cur.right-cur.left;
			if (src.right < 0)
				continue;
			/* cliping with horizontal external box */
			if (clipbox.left > cur.left) {
				if (clipbox.left > cur.right)
					continue;
				src.left += clipbox.left-cur.left;
				cur.left = clipbox.left;
			}
			if (clipbox.right < cur.right) {
				if (clipbox.right < cur.left)
					continue;
				src.right += clipbox.right-cur.right;
				cur.right = clipbox.right;
			}
			/* calculate character vertical limits */
			cur.top = my_char->bbox.top + my_offset->y;
			cur.bottom = my_char->bbox.bottom + my_offset->y;
			src.top = 0;
			src.bottom = cur.bottom-cur.top;
			/* cliping with vertical external box */
			if (clipbox.top > cur.top) {
				if (clipbox.top > cur.bottom)
					continue;
				src.top += clipbox.top-cur.top;
				cur.top = clipbox.top;
			}
			if (clipbox.bottom < cur.bottom) {
				if (clipbox.bottom < cur.top)
					continue;
				src.bottom += clipbox.bottom-cur.bottom;
				cur.bottom = clipbox.bottom;
			}
			/* complete description before drawing */
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			CollectChar(cur, src_base, src, spans);
		}
	}
};

static void CollectScaledCopyBitmap (
	RenderContext *context,
	RenderSubPort * port,
	Pixels *srcPixMap, Pixels *dstPixMap,
	uint8 *src, int32 srcByteWidth,
	uint8 *dst, int32 dstByteWidth,
	int32 countX, int32 fixedXOrig, int32 incX,
	int32 countY, int32 errorY, int32 incY, int32 defaultPtrIncY,
	int32 bytesPerPixel, int32 srcOffs, int32 directionFlags)
{
	RenderCanvas *canvas = port->canvas;
	BArray<int32> &spans = ((SpanCanvas*)canvas)->spans;
	int32 y = (((int32)dst) / canvas->pixels.bytesPerRow);
	int32 startX = (((int32)dst) - (y * canvas->pixels.bytesPerRow))>>1;
	int32 endX = startX + countX-1;
	int32 oldCount = spans.CountItems();
	spans.SetItems(oldCount+countY*3);
	while (countY--) {
		spans[oldCount] = y++;
		spans[oldCount+1] = startX;
		spans[oldCount+2] = endX;
		oldCount+=3;
	};
};

#define BlitSpan																					\
	static void BlitFuncName (																		\
		RenderContext *context,																		\
		RenderSubPort * port,																		\
		Pixels *srcPixMap, Pixels *dstPixMap,														\
		uint8 *src, int32 srcByteWidth,																\
		uint8 *dst, int32 dstByteWidth,																\
		int32 countX, int32 fixedXOrig, int32 incX,													\
		int32 countY, int32 errorY, int32 incY, int32 defaultPtrIncY,								\
		int32 bytesPerPixel, int32 srcOffs, int32 directionFlags)									\
	{																								\
		if (!countX) return;																		\
																									\
		uint8 *dstPtr;																				\
		int32 count,error,fixedX;																	\
		uint32 srcPixel, srcTransaction;															\
		RenderCanvas *canvas = port->canvas;														\
		BArray<int32> &spans = ((SpanCanvas*)canvas)->spans;										\
		int32 thisStartX,x,y = ((int32)dst) / canvas->pixels.bytesPerRow;							\
		int32 startX = (((int32)dst) - (y * canvas->pixels.bytesPerRow))>>1;						\
		ExtraDeclarations;																			\
		while (countY--) {																			\
			dstPtr = dst;																			\
			count = countX;																			\
			fixedX = fixedXOrig;																	\
																									\
			x = thisStartX = startX;																\
			srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));									\
			if (!SrcPixelIsTransparent(srcPixel)) 	goto endOfInSpan;								\
			else									goto endOfOutSpan;								\
																									\
			while (1) {																				\
				srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));								\
				if (SrcPixelIsTransparent(srcPixel)) {												\
					spans.AddItem(y);																\
					spans.AddItem(thisStartX);														\
					spans.AddItem(x-1);																\
					goto endOfOutSpan;																\
				};																					\
				endOfInSpan:																		\
				fixedX += incX; x++;																\
				if (!(--count)) goto lastSpan;														\
			};																						\
																									\
			while (1) {																				\
				srcPixel = *((SourceType*)(src+PixMult(fixedX>>16)));								\
				if (!SrcPixelIsTransparent(srcPixel)) {												\
					thisStartX = x;																	\
					goto endOfInSpan;																\
				};																					\
				endOfOutSpan:																		\
				fixedX += incX;	x++;																\
				if (!(--count)) goto doneLine;														\
			};																						\
																									\
			lastSpan:																				\
			spans.AddItem(y);																		\
			spans.AddItem(thisStartX);																\
			spans.AddItem(x-1);																		\
			doneLine:																				\
																									\
			errorY -= incY;																			\
			srcPixel = errorY >> 31;																\
			errorY += (srcPixel & 65536);															\
			src += (srcPixel & srcByteWidth) + defaultPtrIncY;										\
			y++;																					\
		};																							\
	};

#define DoNotGenerateFunction
#define BlitScaling 											1
#define RenderMode												DEFINE_OP_OVER
	#define DestBits 											16
	#define DestEndianess										LITTLE_ENDIAN
		#if 0
		#define SourceBits										1
			#define SourceEndianess								LITTLE_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOver1ToSpans
				BlitSpan
			#undef SourceEndianess
		#undef SourceBits
		#endif
		#define SourceBits										8
			#define SourceEndianess								LITTLE_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOver8ToSpans
				BlitSpan
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										15
			#define SourceEndianess								LITTLE_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOverLittle15ToSpans
				BlitSpan
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOverBig15ToSpans
				BlitSpan
			#undef SourceEndianess
		#undef SourceBits
		#define SourceBits										32
			#define SourceEndianess								LITTLE_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOverLittle32ToSpans
				BlitSpan
			#undef SourceEndianess
			#define SourceEndianess								BIG_ENDIAN
				#include "blitting.inc"
				#define BlitFuncName							BlitScaleOverBig32ToSpans
				BlitSpan
			#undef SourceEndianess
		#undef SourceBits
	#undef DestEndianess
	#undef DestBits
#undef RenderMode

BlitFunction CollectScaledCopyBitmapList[] =
{
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap
};

BlitFunction CollectScaledOverBitmapList[] =
{
	NULL,//BlitScaleOver1ToSpans,
	NULL,//	BlitScaleOver1ToSpans,
	BlitScaleOver8ToSpans,
	BlitScaleOver8ToSpans,
	BlitScaleOverLittle15ToSpans,
	BlitScaleOverBig15ToSpans,
	CollectScaledCopyBitmap,
	CollectScaledCopyBitmap,
	BlitScaleOverLittle32ToSpans,
	BlitScaleOverBig32ToSpans
};

BlitFunction *CollectBlitTable[NUM_OPS] =
{
	CollectScaledCopyBitmapList,
	CollectScaledOverBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList,
	CollectScaledCopyBitmapList
};

static void PickRenderersSpans(RenderContext *context, RenderSubPort *port)
{
	RenderCache *cache = port->cache;
	int32 op = context->drawOp;
	cache->fFillRects = CollectRects;
	cache->fDrawOneLine = CollectLine;
	cache->fDrawString = CollectString;
	cache->fFillSpans = grFillSpansSoftware;
	cache->fBlitTable =
	cache->fScaledBlitTable = CollectBlitTable[context->drawOp];
#if ROTATE_DISPLAY
	cache->fRotatedBlitTable = NULL;
	cache->fScaledRotatedBlitTable = NULL;
#endif
	cache->fColorOpTransFunc = (ColorOpFunc)colorOpTransTable[(5*10)+PIX_16BIT*10+op];
};

extern void grForeColorChanged_Fallback(RenderContext *context, RenderSubPort *port);
extern void grBackColorChanged_Fallback(RenderContext *context, RenderSubPort *port);

RenderingPackage spanCollectionPackage =
{
	PickRenderersSpans,
	grForeColorChanged_Fallback,
	grBackColorChanged_Fallback
};

void SPicture::Rasterize(
	RenderContext *context,
	bool inverted, bool subpixelPrecision,
	rect *boundsP, fpoint origin, float scale,
	region *theRegion)
{
	rect r,bounds=*boundsP;
	float x,y;
	int32 ox,oy;
	x = origin.h;
	y = origin.v;
	ox = (int32)floor(x);
	oy = (int32)floor(y);
	x -= ox;
	y -= oy;
	
	bounds.left -= ox;
	bounds.right -= ox;
	bounds.top -= oy;
	bounds.bottom -= oy;

	if ((bounds.left > bounds.right) ||
		(bounds.top > bounds.bottom)) {
		clear_region(theRegion);
		return;
	};

	if ((scale == m_rasterized.scale) &&
		(x == m_rasterized.origin.h) &&
		(y == m_rasterized.origin.v) &&
		(subpixelPrecision == m_rasterized.subpix)) {
		if ((m_rasterized.bounds.left > m_rasterized.bounds.right) ||
			(m_rasterized.bounds.top > m_rasterized.bounds.bottom))
			r = bounds;
		else
			union_rect(&bounds,&m_rasterized.bounds,&r);
		if (equal_rect(&r,&m_rasterized.bounds)) {
			if (inverted) {
				if (!m_rasterized.inverseRects) {
					set_region(theRegion,&r);
					m_rasterized.inverseRects = newregion();
//					sub_region(theRegion,m_rasterized.rects,m_rasterized.inverseRects);
					region *tmpRegion = newregion();
					sub_region(theRegion,m_rasterized.rects,m_rasterized.inverseRects);
					kill_region(tmpRegion);
				};
				copy_region(m_rasterized.inverseRects,theRegion);
			} else
				copy_region(m_rasterized.rects,theRegion);

			offset_region(theRegion,
				m_rasterized.bounds.left+ox,
				m_rasterized.bounds.top+oy);
			*boundsP = m_rasterized.bounds;
			offset_rect(boundsP,ox,oy);
/*			DebugPrintf(("Adapting old: %f, (%d,%d), (%f,%f), (%d,%d,%d,%d), (%d,%d,%d,%d)\n",
				scale,ox,oy,x,y,bounds.left,bounds.top,bounds.right,bounds.bottom,
				m_rasterized.bounds.left,m_rasterized.bounds.top,m_rasterized.bounds.right,m_rasterized.bounds.bottom));

			ShowRegion((theRegion,"old rasterize"));
*/			return;
		};
	} else {
		if ((m_rasterized.bounds.left > m_rasterized.bounds.right) ||
			(m_rasterized.bounds.top > m_rasterized.bounds.bottom))
			r = bounds;
		else
			union_rect(&bounds,&m_rasterized.bounds,&r);
	};

	int32 sizeThis = (bounds.right - bounds.left + 1) * (bounds.bottom - bounds.top + 1);
	int32 sizeU = (r.right - r.left + 1) * (r.bottom - r.top + 1);

	if (sizeU > sizeThis*10) r = bounds;

	m_rasterized.subpix = subpixelPrecision;
	m_rasterized.scale = scale;
	m_rasterized.origin.h = x;
	m_rasterized.origin.v = y;

	m_rasterized.bounds = r;
	*boundsP = m_rasterized.bounds;
	offset_rect(boundsP,ox,oy);
/*
	DebugPrintf(("Entering1: (%d,%d,%d,%d), (%d,%d,%d,%d), (%d,%d,%d,%d)\n",
		theRegion->bound.left,theRegion->bound.top,theRegion->bound.right,theRegion->bound.bottom,
		bounds.left,bounds.top,bounds.right,bounds.bottom,
		r.left,r.top,r.right,r.bottom));
*/
	r.right -= r.left;
	r.bottom -= r.top;
	r.left = r.top = 0;
/*
	DebugPrintf(("Entering2: (%d,%d,%d,%d)\n",
		r.left,r.top,r.right,r.bottom));
*/
	clear_region(m_rasterized.rects);
	if (m_rasterized.inverseRects) {
		kill_region(m_rasterized.inverseRects);
		m_rasterized.inverseRects = NULL;
	};

	RenderContext fakeContext;
	RenderPort port;
	SpanCanvas canvas;

	region *tmpRegion = canvas.tmpRegion = newregion();
	
	grInitRenderContext(&fakeContext);
		grInitRenderPort(&port);

			port.infoFlags = 0;
			port.origin.h =
			port.origin.v = 0;
			port.next = NULL;
			
			set_region(port.portClip,&r);
			set_region(port.drawingClip,&r);
			
			// 12/17/2000 - mathias: Initialize the clipping bounds too.
			// It seems that nobody will do it for us. Usually it's done in
			// LockPort(), however our 'fake' version of LockPort() does nothing.
			// (This fixes the crashes with new BBox (Michelangelo's))
			port.portBounds = port.portClip->Bounds();
			port.drawingBounds = port.drawingClip->Bounds();
	
			canvas.fLockAccelPackage = NULL;
			canvas.fUnlockAccelPackage = NULL;
			canvas.accelPackage = &noAccel;
			canvas.pixels.w = r.right+1;
			canvas.pixels.h = r.bottom+1;
			canvas.pixels.pixelData = NULL;
			canvas.pixels.pixelFormat = PIX_16BIT;
			canvas.pixels.endianess = 0;
			canvas.pixels.isCompressed = false;
			canvas.pixels.isLbxCompressed = false;
#if ROTATE_DISPLAY
			canvas.pixels.pixelIsRotated = false;
#endif
			canvas.pixels.bytesPerRow = r.right+1;
			canvas.renderingPackage = &spanCollectionPackage;

			grSetContextPort_NoLock(&fakeContext,&port);
			grSetPortCanvas_NoLock(&port,&canvas);
			
			fakeContext.spanBatch = 1024;

			fpoint fp;
			fp.h = (x - m_rasterized.bounds.left)/scale;
			fp.v = (y - m_rasterized.bounds.top)/scale;
		
			grSetLockingPersistence(&fakeContext,1000000);

			grSetRounding(&fakeContext,m_rasterized.subpix);
			grForceFontFlags(&fakeContext,B_DISABLE_ANTIALIASING|B_ENABLE_HINTING);
			grSetScale(&fakeContext,scale);
			grSetOrigin(&fakeContext,fp);
			grPushState(&fakeContext);
				Play(&fakeContext);
			grPopState(&fakeContext);

		grDestroyRenderPort(&port);
	grDestroyRenderContext(&fakeContext);
	
	int32 count = canvas.spans.CountItems();
	int32 *spans = canvas.spans.Items();
	if (count) {
		KiltSort(spans,spans+count-3);
	
		region *clip = m_rasterized.rects;
		rect* writeIndex = clip->CreateRects(count/3, count/3);
		const int32 *readIndex=spans,*endIndex=spans+count;
		rect bnd;
	
		bnd.top = readIndex[0];
		bnd.right = -MAX_OUT;
	
		writeIndex->top =
		writeIndex->bottom = readIndex[0];
		bnd.left = writeIndex->left = readIndex[1];
		writeIndex->right = readIndex[2];
		readIndex += 3;
	
		while (readIndex < endIndex) {
			if ((readIndex[0] != writeIndex->top) ||
				(readIndex[1] > writeIndex->right+1)) {
				noMerge:
				if (writeIndex->right > bnd.right)
					bnd.right = writeIndex->right;
				writeIndex++;
				writeIndex->top =
				writeIndex->bottom = readIndex[0];
				writeIndex->left = readIndex[1];
				writeIndex->right = readIndex[2];
				readIndex += 3;
				if (writeIndex->left < bnd.left)
					bnd.left = writeIndex->left;
				continue;
			};
			if (readIndex[2] > writeIndex->right) writeIndex->right = readIndex[2];
			readIndex += 3;
		};
		
		bnd.bottom = writeIndex->bottom;
		if (writeIndex->right > bnd.right)
			bnd.right = writeIndex->right;
		clip->SetRectCount((writeIndex-clip->Rects())+1);
		clip->Bounds() = bnd;
		
		compress_spans(m_rasterized.rects);

		if (inverted) {
			if (!m_rasterized.inverseRects) {
				set_region(theRegion,&r);
				m_rasterized.inverseRects = newregion();
//				sub_region(theRegion,m_rasterized.rects,m_rasterized.inverseRects);
				sub_region(theRegion,m_rasterized.rects,m_rasterized.inverseRects);
//				ShowRegion((m_rasterized.inverseRects,"spanisized"));
			};
			copy_region(m_rasterized.inverseRects,theRegion);
		} else
			copy_region(m_rasterized.rects,theRegion);

		offset_region(theRegion,
			m_rasterized.bounds.left+ox,
			m_rasterized.bounds.top+oy);
/*		DebugPrintf(("Made new: %f, (%d,%d), (%f,%f), (%d,%d,%d,%d), (%d,%d,%d,%d), (%d,%d,%d,%d)\n",
			scale,ox,oy,x,y,bounds.left,bounds.top,bounds.right,bounds.bottom,
			m_rasterized.bounds.left,m_rasterized.bounds.top,m_rasterized.bounds.right,m_rasterized.bounds.bottom,
			bnd.left,bnd.top,bnd.right,bnd.bottom));
*/	} else {
		if (inverted) {
			set_region(m_rasterized.rects,&r);
			set_region(theRegion,&r);
			offset_region(theRegion,
				m_rasterized.bounds.left+ox,
				m_rasterized.bounds.top+oy);
		} else {
			clear_region(m_rasterized.rects);
			clear_region(theRegion);
		};
/*		DebugPrintf(("Cleared: %f, (%d,%d), (%f,%f), (%d,%d,%d,%d), (%d,%d,%d,%d)\n",
			scale,ox,oy,x,y,bounds.left,bounds.top,bounds.right,bounds.bottom,
			m_rasterized.bounds.left,m_rasterized.bounds.top,m_rasterized.bounds.right,m_rasterized.bounds.bottom));
*/	};
//	ShowRegion((theRegion,"new rasterize"));

	kill_region(tmpRegion);
};

void SPicture::Play(RenderContext *context)
{
	SPicturePlayer player;
	player.Play(context,this);
};

/*----------------------------------------------------------------*/

void SPicturePlayer::Rewind()
{
	m_ip = 0;
};

inline void * SPicturePlayer::GetData(int32 size)
{
	void *p = (m_data+m_ip);
	m_ip += size;
	return p;
};

inline void SPicturePlayer::GetData(void *v, int32 size)
{
	void *p = (m_data+m_ip);
	memcpy(v,p,size);
	m_ip += size;
};

inline frect SPicturePlayer::GetRect()
{
	frect f;
	GetData(&f,sizeof(frect));
	return f;
};

inline rect SPicturePlayer::GetIRect()
{
	rect f;
	GetData(&f,sizeof(rect));
	return f;
};

inline rgb_color SPicturePlayer::GetColor()
{
	rgb_color f;
	GetData(&f,sizeof(rgb_color));
	return f;
};

inline fpoint SPicturePlayer::GetCoord()
{
	fpoint p;
	GetData(&p,sizeof(fpoint));
	return p;
};

inline float SPicturePlayer::GetFloat()
{
	float f;
	GetData(&f,sizeof(float));
	return f;
};

inline int64 SPicturePlayer::GetInt64()
{
	int64 f;
	GetData(&f,sizeof(int64));
	return f;
};

inline int32 SPicturePlayer::GetInt32()
{
	int32 f;
	GetData(&f,sizeof(int32));
	return f;
};

inline int16 SPicturePlayer::GetInt16()
{
	int16 f;
	GetData(&f,sizeof(int16));

	return f;
};

inline int8 SPicturePlayer::GetInt8()
{
	int8 f;
	GetData(&f,sizeof(int8));
	return f;
};

inline void SPicturePlayer::GetString(char *str)
{
	int32 len = GetInt32();
	GetData(str,len);
	str[len] = 0;
	P(("GetString: \"%s\"\n",str));
};

inline SPicture * SPicturePlayer::GetPicture()
{
	int32 token = GetInt32();
	P(("GetPicture token=%d, lib=%d\n",token,m_picture->PicLib().CountItems()));
	if ((token < 0) || (token >= m_picture->PicLib().CountItems())) return NULL;
	return m_picture->PicLib()[token];
};

long	dcnt=0;

int16 SPicturePlayer::GetOp()
{
	int32	result;
	char	buf[32];

	if (m_ip > (m_dataLen-6)) return 0;

	result = GetInt16();
	
	dcnt++;
	return result;
};

void SPicturePlayer::FontSubplay(RenderContext *context, uint32 *starts, int32 *size)
{
	P(("SPIC_FONT\n"));
	int32 mask = 0;
	int32 op;
	char str[256];
	bool setFam = false;
	fc_context_packet packet;
	packet.overlay_size = 0;
	while (m_ip < (starts[1]+size[1])) {
		op = GetOp();
		size[2] = GetInt32();
		starts[2] = m_ip;
		switch (op) {
			case SPIC_FAMILY:
				P(("SPIC_FAMILY\n"));
				GetString(str);
				packet.f_id = fc_get_family_id(str);
				setFam = true;
				mask |= B_FONT_FAMILY_AND_STYLE;
				break;
			case SPIC_SPACING:
				P(("SPIC_SPACING\n"));
				packet.spacing_mode = GetInt32();
				mask |= B_FONT_SPACING;
				break;
			case SPIC_SIZE:
				P(("SPIC_SIZE\n"));
				packet.size = GetFloat();
				mask |= B_FONT_SIZE;
				break;
			case SPIC_ROTATE:
				P(("SPIC_ROTATE\n"));
				packet.rotation = GetFloat();
				mask |= B_FONT_ROTATION;
				break;
			case SPIC_ENCODING:
				P(("SPIC_ENCODING\n"));
				packet.encoding = GetInt32();
				mask |= B_FONT_ENCODING;
				break;
			case SPIC_FLAGS:
				P(("SPIC_FLAGS\n"));
				packet.flags = GetInt32();
				mask |= B_FONT_FLAGS;
				break;
			case SPIC_STYLE:
				P(("SPIC_STYLE\n"));
				GetString(str);
				if (!setFam && context->fontContext.IsValid())
					packet.f_id = context->fontContext.FamilyID();
				packet.s_id = fc_get_style_id(packet.f_id,str);
				mask |= B_FONT_FAMILY_AND_STYLE;
				break;
			case SPIC_SHEAR:
				P(("SPIC_SHEAR\n"));
				packet.shear = GetFloat();
				mask |= B_FONT_SHEAR;
				break;
			case SPIC_BPP:
				P(("SPIC_BPP\n"));
				GetInt32();
				break;
			case SPIC_FACES:
				P(("SPIC_FACES\n"));
				packet.faces = GetInt32();
				mask |= B_FONT_FACE;
				break;
			default:
				P(("UNKNOWN FONT OP=%d\n",op));
				break;
		};
	};
	grSetFontFromPacket(context,&packet,sizeof(packet),mask);
};

void SPicturePlayer::Play(RenderContext *context, SPicture *playthis)
{
	fpoint p[4];
	frect r;
	float startTheta,endTheta;
	int32 op,i;
	int32 size[8];
	uint32 starts[8];
	uint16 word16[4];
	uint32 word32[4];
	void *ptr[4];
	uint64 i64;
	float f[4];
	char str[1024];
	SPath path(context);
	SPicture *pic;

	m_picture = playthis;
	m_data = (uint8*)m_picture->Data();
	m_dataLen = m_picture->DataLength();

	if (!m_data) return;

	Rewind();
	P(("Starting playback, dataPtr = %d\n",m_dataLen));
	while ((op=GetOp()) != 0) {
		size[0] = GetInt32();
		starts[0] = m_ip;
		switch (op) {
			case SPIC_MOVE_PEN:
				P(("SPIC_MOVE_PEN\n"));
				p[0] = GetCoord();
				grMovePenLocation(context,p[0]);
				break;
			case SPIC_STROKE_LINE:
				P(("SPIC_STROKE_LINE\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				grStrokeLine(context,p[0],p[1]);
				break;
			case SPIC_STROKE_RECT:
				P(("SPIC_STROKE_RECT\n"));
				r = GetRect();
				grStrokeRect(context,r);
				break;
			case SPIC_STROKE_ROUNDRECT:
				P(("SPIC_STROKE_ROUNDRECT\n"));
				r = GetRect();
				p[0] = GetCoord();
				grStrokeRoundRect(context,r,p[0]);
				break;
			case SPIC_STROKE_BEZIER:
				P(("SPIC_STROKE_BEZIER\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				p[2] = GetCoord();
				p[3] = GetCoord();
				grStrokeBezier(context,p);
				break;
			case SPIC_INSCRIBE_STROKE_ARC:
				P(("SPIC_INSCRIBE_STROKE_ARC\n"));
				r = GetRect();
				startTheta = GetFloat();
				endTheta = GetFloat();
				grInscribeStrokeArc(context,r,startTheta,endTheta);
				break;
			case SPIC_STROKE_ARC:
				P(("SPIC_STROKE_ARC\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				startTheta = GetFloat();
				endTheta = GetFloat();
				grStrokeArc(context,p[0],p[1],startTheta,endTheta);
				break;
			case SPIC_INSCRIBE_STROKE_ELLIPSE:
				P(("SPIC_INSCRIBE_STROKE_ELLIPSE\n"));
				grInscribeStrokeEllipse(context,GetRect());
				break;
			case SPIC_STROKE_ELLIPSE:
				P(("SPIC_STROKE_ELLIPSE\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				grStrokeEllipse(context,p[0],p[1]);
				break;
			case SPIC_STROKE_POLYGON:
			{
				P(("SPIC_STROKE_POLYGON\n"));
				word32[0] = GetInt32();
				word32[1] = word32[0]*sizeof(fpoint);
				fpoint *fp = (fpoint*)grMalloc(word32[1],"tmpPictPolyBuf",MALLOC_CANNOT_FAIL);
				GetData(fp,word32[1]);
				grStrokePolygon(context,fp,word32[0],GetInt8());
				grFree(fp);
				break;
			}
			case SPIC_STROKE_PATH:
				P(("SPIC_STROKE_PATH\n"));
				word32[0] = GetInt32();
				path.Ops()->SetItems(word32[0]);
				GetData(path.Ops()->Items(),word32[0]*4);
				word32[1] = GetInt32();
				path.Points()->SetItems(word32[1]);
				GetData(path.Points()->Items(),word32[1]*sizeof(fpoint));
				grStrokePath(context,&path);
				break;
			case SPIC_FILL_RECT:
				P(("SPIC_FILL_RECT\n"));
				r = GetRect();
				grFillRect(context,r);
				break;
			case SPIC_FILL_ROUNDRECT:
				P(("SPIC_FILL_ROUNDRECT\n"));
				r = GetRect();
				p[0] = GetCoord();
				grFillRoundRect(context,r,p[0]);
				break;
			case SPIC_FILL_BEZIER:
				P(("SPIC_FILL_BEZIER\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				p[2] = GetCoord();
				p[3] = GetCoord();
				grFillBezier(context,p);
				break;
			case SPIC_INSCRIBE_FILL_ARC:
				P(("SPIC_INSCRIBE_FILL_ARC\n"));
				r = GetRect();
				startTheta = GetFloat();
				endTheta = GetFloat();
				grInscribeFillArc(context,r,startTheta,endTheta);
				break;
			case SPIC_FILL_ARC:
				P(("SPIC_FILL_ARC\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				startTheta = GetFloat();
				endTheta = GetFloat();
				grFillArc(context,p[0],p[1],startTheta,endTheta);
				break;
			case SPIC_INSCRIBE_FILL_ELLIPSE:
				P(("SPIC_INSCRIBE_FILL_ELLIPSE\n"));
				grInscribeFillEllipse(context,GetRect());
				break;
			case SPIC_FILL_ELLIPSE:
				P(("SPIC_FILL_ELLIPSE\n"));
				p[0] = GetCoord();
				p[1] = GetCoord();
				grFillEllipse(context,p[0],p[1]);
				break;
			case SPIC_FILL_POLYGON:
			{
				P(("SPIC_FILL_POLYGON\n"));
				word32[0] = GetInt32();
				word32[1] = word32[0]*sizeof(fpoint);
				fpoint *fp = (fpoint*)grMalloc(word32[1],"tmpPictFillPolyBuf",MALLOC_CANNOT_FAIL);
				GetData(fp,word32[1]);
				grFillPolygon(context,fp,word32[0]);
				grFree(fp);
				break;
			}
			case SPIC_FILL_PATH:
				P(("SPIC_FILL_PATH\n"));
				word32[0] = GetInt32();
				path.Ops()->SetItems(word32[0]);
				GetData(path.Ops()->Items(),word32[0]*4);
				word32[1] = GetInt32();
				path.Points()->SetItems(word32[1]);
				GetData(path.Points()->Items(),word32[1]*sizeof(fpoint));
				grFillPath(context,&path);
				break;
			case SPIC_DRAW_STRING:
				P(("SPIC_DRAW_STRING\n"));
				GetString(str);
				f[0] = GetFloat();
				f[1] = GetFloat();
				grDrawString(context,(uchar*)str,f);
				break;
			case SPIC_PLAY_PICTURE:
				P(("SPIC_PLAY_PICTURE\n"));
				p[0] = GetCoord();
				pic = GetPicture();
				if (pic) grPlayPicture(context,pic,p[0]);
				break;
			case SPIC_SET_STATE:
			{
				P(("SPIC_SET_STATE\n"));
//				grHoldPicks(context);
				while (m_ip < (starts[0]+size[0])) {
					op = GetOp();
					size[1] = GetInt32();
					starts[1] = m_ip;
					switch (op) {
						case SPIC_ORIGIN:
							P(("SPIC_ORIGIN\n"));
							grSetOrigin(context,GetCoord());
							break;
						case SPIC_LOCATION:
							P(("SPIC_LOCATION\n"));
							grSetPenLocation(context,GetCoord());
							break;
						case SPIC_DRAW_OP:
							P(("SPIC_DRAW_OP\n"));
							grSetDrawOp(context,GetInt16());
							break;
						case SPIC_BLENDING_MODE:
							P(("SPIC_BLENDING_MODE\n"));
							word16[0] = GetInt16();
							grSetBlendingMode(context,word16[0],GetInt16());
							break;
						case SPIC_LINE_MODE:
							P(("SPIC_LINE_MODE\n"));
							word16[0] = GetInt16();
							word16[1] = GetInt16();
							grSetLineMode(context,word16[0],word16[1],GetFloat());
							break;
						case SPIC_PEN_SIZE:
							P(("SPIC_PEN_SIZE\n"));
							grSetPenSize(context,GetFloat());
							break;
						case SPIC_FORE_COLOR:
							P(("SPIC_FORE_COLOR\n"));
							grSetForeColor(context,GetColor());
							break;
						case SPIC_BACK_COLOR:
							P(("SPIC_BACK_COLOR\n"));
							grSetBackColor(context,GetColor());
							break;
						case SPIC_STIPPLE:
							P(("SPIC_STIPPLE\n"));
							i64 = GetInt64();
							grSetStipplePattern(context,(pattern*)&i64);
							break;
						case SPIC_SCALE:
							P(("SPIC_SCALE\n"));
							grSetScale(context,GetFloat());
							break;
						case SPIC_CLEAR_CLIP:
							P(("SPIC_CLEAR_CLIP\n"));
							grClearClip(context);
							break;
						case SPIC_CLIP_TO_RECTS:
							P(("SPIC_CLIP_TO_RECTS\n"));
							if (size[1]) {
								region r;
								word32[0] = size[1]/sizeof(rect);
								rect* data = r.CreateRects(word32[0]-1, word32[0]-1);
								GetData(&r.Bounds(),sizeof(rect));
								GetData(data,(word32[0]-1)*sizeof(rect));
								r.SetRectCount(word32[0]-1);
								grClipToRegion(context,&r);
								m_ip = starts[1]+size[1];
							};
							break;
						case SPIC_CLIP_TO_PICTURE:
							pic = GetPicture();
							p[0] = GetCoord();
							word32[0] = GetInt32();
							P(("SPIC_CLIP_TO_PICTURE 0x%08x, (%f,%f), %08x\n",pic,p[0].h,p[0].v,word32[0]));
							if (pic) grClipToPicture(context,pic,p[0],word32[0]);
							break;
						case SPIC_FONT:
							FontSubplay(context,starts,size);
							break;
						default:
							P(("UNKNOWN STATE OP=%d\n",op));
							break;
					};
				};
//				grAllowPicks(context);
				break;
			}
			case SPIC_PUSH_STATE:
				P(("SPIC_PUSH_STATE\n"));
				grPushState(context);
				break;
			case SPIC_POP_STATE:
				P(("SPIC_POP_STATE\n"));
				grPopState(context);
				break;
			case SPIC_PIXELS:
			{
				P(("SPIC_PIXELS\n"));
				int32 np,bpp,s,f;
				frect src,dst;
				Pixels p;

				src = GetRect();				// Source rect
				dst = GetRect();				// Destination rect
				p.w = GetInt32(); 				// Width
				p.h = GetInt32(); 				// Height
				p.bytesPerRow = GetInt32(); 	// Bytes per row
				uint8 endianess;
				GraphicsTypeToInternal(GetInt32(),&p.pixelFormat,&endianess);
				p.endianess = endianess;

				f = GetInt32();					// Flags
				p.isCompressed = false;
				p.isLbxCompressed = false;
				if (f & BITMAP_IS_LBX_COMPRESSED) {
					// XXX: right now this should not happen
					// see renderPixels.cpp, grDrawPixelsRecordPart()
					p.isCompressed = true;
					p.isLbxCompressed = true;
				}
#if ROTATE_DISPLAY
				p.pixelIsRotated = false;
				if (f & B_BITMAP_IS_ROTATED) {
					// XXX: right now this should not happen
					// see renderPixels.cpp, grDrawPixelsRecordPart()
					p.pixelIsRotated = true;
				}
#endif
				s = GetInt32();					// Image size
				p.pixelData = m_data+m_ip;		// Image data ptr
				p.colorMap = xSystemColorMap;
				m_ip += s;
				grDrawPixels(context,src,dst,&p);
				break;
			}
			case SPIC_BLIT:
				P(("SPIC_BLIT\n"));
				break;
			default:
				P(("UNKNOWN OP=%d\n",op));
				return;
		}
	}
}
