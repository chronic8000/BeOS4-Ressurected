
#include "OGLDevice.h"
#include "OGLSurface.h"
#include "mini.h"

extern AppServerWindow * asWindow;

BOGLSurface::BOGLSurface(uint32 width, uint32 height, color_space /*cs*/)
{
	BOGLPlane *plane;
	OGLDevice.Plane_Create( width, height, 0,
						BGL_ANY|BGL_SINGLE, BGL_NONE, BGL_NONE, BGL_NONE,
						(BRasterPlane**)&plane);
	OGLDevice.Plane_Display(plane);
	SetRasterPlane(plane);
	plane->SetEventSurface(this);	// MA: what is this supposed to do?
	asWindow->SetSurface(this);
}
