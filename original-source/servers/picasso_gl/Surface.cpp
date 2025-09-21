//******************************************************************************
//
//	File:			Surface.cpp
//
//	Description:	BSurface implementation
//	
//	Written by:		Mathias Agopian, George Hoffman
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <support2_p/BinderKeys.h>

#include <support2/Looper.h>
#include <support2/Team.h>
#include <support2/StdIO.h>
#include <render2/Region.h>
#include <render2/2dTransform.h>
#include <render2/Update.h>
#include "OGLDevice.h"
#include "RenderStream.h"
#include "Surface.h"

using namespace B::Private;

void lock_context(BSurface *surface)
{
	surface->Lock();
}

void unlock_context(BSurface *surface)
{
	surface->Unlock();
}

void inval(BSurface *surface, B::Raster2::BRasterRegion &region) {
	// get a BRegion from BRasterRegion
	BRegion reg(region,B2dTransform::MakeIdentity());
	// create a corresponding Update description
	BUpdate update(BUpdate::B_OPAQUE, B2dTransform::MakeIdentity(), reg, reg);
	surface->InvalidateChild(NULL, update);
}

// ---------------------------------------------------------------
// #pragma mark -

BSurface::BSurface(BRasterPlane *plane)
	: 	m_plane(plane),
		m_update(BUpdate::empty),
		m_updatePending(false),
		m_inUpdate(false)
{
}

void 
BSurface::Lock()
{
	m_lock.Lock();
}

void 
BSurface::Unlock()
{
	m_lock.Unlock();
}

void 
BSurface::SetRasterPlane(BRasterPlane *plane)
{
	m_plane = plane;
}

status_t
BSurface::Released(const void* id)
{
	return BHandler::Released(id);
}

status_t 
BSurface::SetHost(const IView::ptr& hostView)
{
	bout << "BSurface::SetHost " << m_plane->Width() << " " << m_plane->Height() << endl;

	BRect r(0, 0, m_plane->Width(), m_plane->Height());
	BUpdate dummy;
	m_lock.Lock();
		hostView->SetParent(this);
		hostView->SetTransform(B2dTransform::MakeIdentity());
		hostView->SetLayoutBounds(r);
		m_host = hostView->PostTraversal(dummy);
	m_lock.Unlock();

	BUpdate update(BUpdate::B_OPAQUE, m_host->Transform(), m_host->Shape(), m_host->Shape());
	InvalidateChild(m_host, update);	
	return B_OK;
}

IViewParent::ptr 
BSurface::ViewParent()
{
	return this;
}

status_t 
BSurface::ConstrainChild(const IView::ptr&, const BLayoutConstraints &)
{
	return B_OK;
}

status_t 
BSurface::InvalidateChild(const IView::ptr&, const BUpdate &update)
{
	bool sendMsg = false;
	if (update.IsEmpty() == false) {
		m_lock.Lock();

			// Compose the update we just received with ourself, this will
			// (1) convert it to our coordintate space
			// (2) clip it by our bounds
			BUpdate me(	BUpdate::B_OPAQUE,
						B2dTransform::MakeIdentity(),
						BRect(0,0,m_plane->Width(),m_plane->Height()));
			me.ComposeWithChildren(update);

			// Then compose this update with what we already have
			m_update.ComposeBefore(me);
	
			sendMsg = (!m_updatePending && !m_inUpdate);
			m_updatePending = true;
		m_lock.Unlock();
		if (sendMsg)
			PostMessage(BMessage(bmsgUpdate));
	}
	return B_OK;
}

void 
BSurface::MarkTraversalPath(int32 /* type */)
{
}

void
BSurface::DispatchEvent(const BMessage &msg, const BPoint& where)
{
	if (m_host != NULL)
		m_host->DispatchEvent(msg, m_host->Transform().Invert().Transform(where));
}

void 
BSurface::EndUpdate()
{
	bool sendMsg = false;
	m_lock.Lock();
		m_inUpdate = false;
		sendMsg = m_updatePending;
	m_lock.Unlock();
	if (sendMsg)
		PostMessage(BMessage(bmsgUpdate));
}

status_t 
BSurface::HandleMessage(const BMessage &msg)
{
	// Set the priority
	BLooper::SetThreadPriority(B_DISPLAY_PRIORITY);

	switch (msg.What()) {
		case bmsgUpdate: {
			BUpdate update;
			m_lock.Lock();
			if (!m_updatePending || m_inUpdate || (m_host==NULL)) {
				m_lock.Unlock();
				return B_OK;
			}
			update = m_update;
			m_update.MakeEmpty();
			m_updatePending = false;
			m_inUpdate = true;
			IView::ptr host = m_host;
			m_lock.Unlock();

			// This will take some time to process...			
			ResumeScheduling();

			// Execute all needed screen to screen blits
			update_screen(update);

			// Now, send the updates
			BRegion dirty = update.DirtyRegion();
			if (!dirty.IsEmpty()) {
				BRasterRegion reg;
				dirty.Rasterize(&reg);
				BRenderHole::ptr hole = new BRenderHole(this);
				BRenderStream::ptr stream = new BRenderStream(hole);
				(new BRenderStreamProxy(stream))->DrawRemote(host);
			} else { // nothing to update
				m_lock.Lock();
				m_inUpdate = false;
				m_lock.Unlock();
			}
		} break;
	}

	return B_OK;
}

void BSurface::update_screen(const BUpdate& update)
{
	const uint32 count = update.MovedRegions().CountItems();
	if (count == 0)
		return;

	BVector<BRasterRegion>	regions;
	BVector<BRasterPoint>	offsets;
	regions.SetCapacity(count);
	offsets.SetCapacity(count);

	// Rasterize all BRegions
	for (uint32 i=0 ; i<count ; i++) {		
		const BPoint& offset = update.MovedRegions().ItemAt(i).offset;
		const BRegion& region = update.MovedRegions().ItemAt(i).region;
		BRasterRegion reg;
		region.Rasterize(&reg);
		offsets.AddItem(BRasterPoint(offset.x, offset.y));
		regions.AddItem(reg);
	}

	if (OGLDevice.Context_Lock() == B_OK) {
		for (uint32 i=0 ; i<count ; i++) {
			const BRasterPoint& o = offsets[i];
			const BRasterRegion& r = regions[i];
			if (((o.x == 0) && (o.y ==0)) || r.IsEmpty())
				continue;
			const uint32 c = r.CountRects();
			for (uint32 j=0 ; j<c ; j++) {
				const int32 index = (o.y > 0) ? (c-j-1) : (j);
				const BRasterRect& rect = r.Rects()[index];
				glRasterPos2i(rect.left+o.x, rect.top+o.y);
//				glCopyPixels(rect.left, rect.top, rect.Width(), rect.Height(), GL_COLOR);
			}
		}
		OGLDevice.Context_Unlock();
	}
}
