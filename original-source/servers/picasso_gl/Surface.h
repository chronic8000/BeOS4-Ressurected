
#ifndef PICASSO_BSURFACE_H
#define PICASSO_BSURFACE_H

#include <support2/Message.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/View.h>
#include <interface2/ViewParent.h>
#include <interface2/Surface.h>
#include "RasterDevice.h"

using namespace B::Support2;
using namespace B::Render2;
using namespace B::Interface2;

class BRenderStream;

enum {
	bmsgUpdate = 'updt',
};


class BSurface : public LSurface, public LViewParent, public BHandler
{
	public:

									BSurface(BRasterPlane *plane=NULL);

				void				SetRasterPlane(BRasterPlane *plane);
				BRasterPlane *		RasterPlane() { return m_plane; };

		virtual	status_t			Released(const void* id);

		virtual	status_t			HandleMessage(const BMessage &msg);

		virtual	status_t			ConstrainChild(const IView::ptr& child, const BLayoutConstraints& constraints);
		virtual	status_t			InvalidateChild(const IView::ptr& child, const BUpdate& update);
		virtual	void				MarkTraversalPath(int32 type);

		virtual	status_t			SetHost(const IView::ptr& hostView);
		virtual	IViewParent::ptr	ViewParent();

		virtual	void				DispatchEvent(	const BMessage &msg,
													const BPoint& where);
		
				void				EndUpdate();
				
				void				Lock();
				void				Unlock();
				
	private:
				void				update_screen(const BUpdate& update);

				BRasterPlane *		m_plane;
				BLocker				m_lock;
				IView::ptr			m_host;
				BUpdate				m_update;
				sem_id				m_updateSem;
				bool				m_updatePending:1;
				bool				m_inUpdate:1;
};

#endif
