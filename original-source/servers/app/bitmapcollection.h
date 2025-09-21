
//******************************************************************************
//
//	File:			bitmapcollection.cpp
//
//	Description:	An SAtom based class to handle LBX
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2000, Be Incorporated
//
//******************************************************************************/

#ifndef SBITMAP_COLLECTION_H
#define SBITMAP_COLLECTION_H

#include <OS.h>
#include <Debug.h>

#include "token.h"

class LBX_DrawEngine;
class LBX_Container;

struct lbx_pixel_data_t
{
	lbx_pixel_data_t() : draw_engine(NULL), index(-1), lbx_token(0xFFFFFFFF) { }
	LBX_DrawEngine *draw_engine;
	int32 index;
	int32 lbx_token;
};


class SBitmapCollection : public SAtom
{
public:
				SBitmapCollection(int32 lbx_size);
	virtual 	~SBitmapCollection();
	
	bool IsValid() const;
	area_id AreaID() const;

	LBX_Container *Container();

private:
	area_id fArea;
	LBX_Container *fLbxContainer;
	void *fAddress;
};

#endif
