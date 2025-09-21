//******************************************************************************
//
// File: server.cpp
//
// Copyright 2001, Be Incorporated
//
//******************************************************************************

#include <support2/StdIO.h>
#include <content2/Content.h>

using namespace B::Interface2;
using namespace B::Support2;
using namespace B::Content2;

#include "OGLDevice.h"
#include "OGLSurface.h"
#include "server.h"
#include "input.h"
#include "Surface.h"

// --------------------------------------------------

extern void create_bapp();

// --------------------------------------------------

// There's only one input server link allowed, so make it global.
InputServerLink *gInputServerLink = NULL;

// --------------------------------------------------

BPicassoServer::BPicassoServer()
{
//	init_system();
	create_bapp();
	m_rootSurface = new BOGLSurface(640,480);
//	SFont::Start();					// the font engine
	gInputServerLink = new InputServerLink("picasso",this);
}

BPicassoServer::~BPicassoServer()
{
	delete gInputServerLink;
	gInputServerLink = NULL;
}

void 
BPicassoServer::DispatchEvent(const B::Support2::BMessage &msg)
{
	m_rootSurface->DispatchEvent(msg,BPoint(msg.Data()["where"]));
}

ISurface::ptr BPicassoServer::RootSurface()
{
	return m_rootSurface;
}

ISurface::ptr BPicassoServer::CreateSurface(int32 /*width*/, int32 /*height*/, color_space /*format*/)
{
	return NULL;
}

BValue BPicassoServer::Inspect(const BValue &which, uint32 flags)
{
	return LSurfaceManager::Inspect(which, flags)
		.Overlay(LValueOutput::Inspect(which, flags));
}

status_t BPicassoServer::Write(const BValue &out)
{
	BValue val;
	if ((val = out.ValueFor("root"))) {
		IBinder::ptr rootObj = val.AsBinder();
		IView::ptr v = IView::AsInterface(rootObj);
		if (v == NULL) {
			bout << "not a view!" << endl;
			IContent::ptr content = IContent::AsInterface(rootObj);
			if (content == NULL) {
				bout << "not a content! I give up..." << endl;
			} else {
				bout << "is content... getting view..." << endl;
				v = content->CreateView(BMessage());
				if (v == NULL) {
					bout << "no view! giving up" << endl;
				}
			}
		}
		if (v != NULL) m_rootSurface->SetHost(v);
	}
	return B_OK;
}

status_t BPicassoServer::End()
{
	return B_OK;
}
