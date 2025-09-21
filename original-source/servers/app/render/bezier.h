
//******************************************************************************
//
//	File:			bezier.h
//	Description:	Tesselation of cubic bezier curves (interface)
//	Written by:		George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef BEZIER_H
#define BEZIER_H

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "basic_types.h"
#include "BArray.h"

void TesselateClippedCubicBezier(
	fpoint *controlPoints,
	BArray<fpoint> &curve,
	frect clip, float maxError);

#endif
