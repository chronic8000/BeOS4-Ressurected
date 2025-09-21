
//******************************************************************************
//
//	File:			newPicture.h
//	Description:	The new server-side picture object
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef NEWPICTURE_H
#define NEWPICTURE_H

#include <stdlib.h>
#include "shared_picture.h"
#include "renderContext.h"
#include "BArray.h"
#include "../atom.h"

struct SRasterizedPicture {
	rect		bounds;
	float		scale;
	fpoint		origin;
	region *	rects;
	region *	inverseRects;
	bool		subpix;
};

class SPicturePlayer;

class SPicture : public SAtom {

public:

							SPicture(SPicture *oldPicture = NULL);
							~SPicture();

		void				Restart(SPicture *oldPicture);
		void				Copy(SPicture *copy);
		
		void				BeginOp(int32 opCode);
		void				EndOp();
		void				EndAndPossiblyForgetOp();
		int					DiskMe(char *filename, long offset);

		void				AddDiskData(void *_data, int32 len);
		void				AddData(void *data, int32 len);
		void				AddRect(frect value);
		void				AddIRect(rect value);
		void				AddColor(rgb_color value);
		void				AddCoord(fpoint value);
		void				AddFloat(float value);
		void				AddInt64(int64 value);
		void				AddInt32(int32 value);
		void				AddInt16(int16 value);
		void				AddInt8(int8 value);
		void				AddString(char *str);
		void				AddPicture(SPicture *picture);

		void				AddCoords(fpoint *values, int32 count);
		void				AddFloats(float *values, int32 count);
		void				AddInt64s(int64 *values, int32 count);
		void				AddInt32s(int32 *values, int32 count);
		void				AddInt16s(int16 *values, int32 count);
		void				AddInt8s(int8 *values, int32 count);
		
		void				Play(RenderContext *context);
		void				SyncState(RenderContext *context);
		void				Record(RenderContext *context);
		void				EndRecording();
		
		bool				Verify();
		
		int32				Token();
		void				SetToken(int32 token);
		
		BArray<SPicture*> &	PicLib() { return pictureLibrary; };
		
		void *				Data();
		int32				DataLength();

		void				SetDataLength(int32 size);
		
		void				RecordStatePush(RenderContext *context);
		void				RecordStatePop(RenderContext *context);

		void				Usurp(SPicture *pic);
		SPicture *			OlderPicture();
		void				PenLocationChanged();

		void				Rasterize(
								RenderContext *context,
								bool inverted, bool subpixelPrecision,
								rect *bounds, fpoint origin, float scale,
								region *theRegion);
/*
		virtual	int32		ServerLock();
		virtual	int32		ServerUnlock();
*/
private:



		SRasterizedPicture	m_rasterized;
		SPicture *			pictureStack;
		BArray<SPicture*>	pictureLibrary;

		RenderState			renderState;
		bool				needInitialState;
		bool				updatePenLocation;

		uint8 *				data;
		int32				dataPtr;
		int32				dataLen;
		BArray<int32>		opBegin;

		bool				disk_picture;
		char				*disk_buffer;
		bool				disk_writes;
		int32				written_length;
		int32				picture_start_offset;
		int32				file_ref;
		int32				readable_length;
		int32				server_token;

		void				AssertSpace(int32 size);
};

class SPicturePlayer {

	public:

		void				Play(RenderContext *context, SPicture *picture);

	private:
	
		void				FontSubplay(RenderContext *context, uint32 *starts, int32 *size);

		int32				m_ip;
		SPicture *			m_picture;
		uint8 *				m_data;
		uint32				m_dataLen;

		void				Rewind();

		int16				GetOp();
		frect				GetRect();
		rect				GetIRect();
		rgb_color			GetColor();
		fpoint				GetCoord();
		float				GetFloat();
		int64				GetInt64();
		int32				GetInt32();
		int16				GetInt16();
		int8				GetInt8();
		SPicture *			GetPicture();
		void				GetString(char *str);
		void *				GetData(int32 size);
		void				GetData(void *buf, int32 size);
};

#endif
