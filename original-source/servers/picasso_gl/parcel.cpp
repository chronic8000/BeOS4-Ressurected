//*****************************************************************************
//
//	File:		parcel.cpp
//	
//	Written by:	Peter Potrebic & Benoit Schillings
//  Destroyed by: George Hoffman
//	Swept up by: Dianne Hackborn
//
//	Copyright 1994-2001, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#include <stdlib.h>
#include <stdio.h>

#include <support2/Debug.h>
#include "parcel.h"
#include "old/Message.h"

BParcel::BParcel()
	: B::Support2::BMessage()
{
	init_data();
}

BParcel::BParcel(uint32 what)
	: B::Support2::BMessage(what)
{
	init_data();
}

BParcel::BParcel(const BParcel &msg)
	: B::Support2::BMessage(msg)
{
}

BParcel::BParcel(const B::Support2::BValue &val)
	: B::Support2::BMessage(val)
{
}

void BParcel::init_data()
{
}


BParcel &BParcel::operator = (const BParcel &msg)
{
	if (this != &msg)
		B::Support2::BMessage::operator = (msg);
	return *this;
}

BParcel::~BParcel()
{
}
