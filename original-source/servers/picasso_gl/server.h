//******************************************************************************
//
// File: server.h
//
// Copyright 2001, Be Incorporated
//
//******************************************************************************

#ifndef SERVER_H
#define SERVER_H

#include <math.h>

#include <support2/SupportDefs.h>
#include <support2/String.h>
#include <support2/Binder.h>
#include <support2/ValueStream.h>
#include <interface2/SurfaceManager.h>
#include <interface2/Surface.h>

using B::Support2::atom_ptr;
using B::Support2::LValueOutput;
using B::Support2::BValue;
using B::Interface2::LSurfaceManager;
using B::Interface2::ISurface;

// ---------------------------------------------------------

class BSurface;

// ---------------------------------------------------------

class BPicassoServer : public LSurfaceManager, public LValueOutput
{
	public:
								BPicassoServer();
		virtual					~BPicassoServer();
	
				void			DispatchEvent(const B::Support2::BMessage &msg);
	
		virtual	ISurface::ptr	RootSurface();
		virtual	ISurface::ptr	CreateSurface(int32 width, int32 height, B::Interface2::color_space format);
	
		virtual	BValue			Inspect(const BValue &which, uint32 flags = 0);
		virtual	status_t		Write(const BValue &out);
		virtual	status_t		End();
		
	private:

		atom_ptr<BSurface>		m_rootSurface;
};

// ---------------------------------------------------------

#endif

