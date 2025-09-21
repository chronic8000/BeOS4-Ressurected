
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

#include <OS.h>

#include "proto.h"
#include "server.h"
#include "bitmapcollection.h"
#include "DrawEngine.h"

extern TokenSpace *tokens;


SBitmapCollection::SBitmapCollection(int32 lbx_size)
	: SAtom(),
	fLbxContainer(NULL)
{
	// Create the shared area	
	fArea = Area::CreateArea("LBX", &fAddress, B_ANY_ADDRESS, (lbx_size + 4095) & 0xFFFFF000, 0, B_READ_AREA|B_WRITE_AREA);
	if (fArea <= 0)
		fArea = 0xFFFFFFFF;
}

SBitmapCollection::~SBitmapCollection()
{
	if (fArea != 0xFFFFFFFF)
		Area::DeleteArea(fArea);
	delete fLbxContainer;
}

bool SBitmapCollection::IsValid() const
{
	return (fArea != 0xFFFFFFFF);
}

area_id SBitmapCollection::AreaID() const
{
	return fArea;
}

LBX_Container *SBitmapCollection::Container()
{
	if (!fLbxContainer)
		fLbxContainer = LBX_BuildContainer((uint8 *)fAddress);

	return fLbxContainer;
}

