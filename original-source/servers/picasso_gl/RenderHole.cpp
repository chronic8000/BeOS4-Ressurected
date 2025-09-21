//******************************************************************************
//
//	File:			RenderHole.cpp
//
//	Description:	BRenderHole implementation
//	
//	Written by:		George Hoffman, Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>

#include "Surface.h"
#include "RenderHole.h"
#include "RenderStream.h"
#include "OGLDevice.h"
#include <support2/StdIO.h>
#include <render2/2dTransform.h>
#include <render2_p/RenderProtocol.h>
#include <font2/FontEngine.h>

using namespace B::Font2;

#define DEBUG_DRAWING		0
#define DUMB_PROFILE		0
#define DEBUG_DEPENDENCY	1

#if DEBUG_DRAWING
#	define PRINT_COMMAND(_x)	_x
#else
#	define PRINT_COMMAND(_x)
#endif


// *************************************************************
// #pragma mark -

BRenderHole::BRenderHole(BSurface *surface)
	:	m_surface(surface),
		m_current_stream(NULL)
{
}

BRenderHole::~BRenderHole()
{
	m_surface->EndUpdate();
}

void BRenderHole::AssertState(const BRenderStream *current)
{
	// Set the last state on the stack
	const BRenderState *s = current->State();
	while (s->Next())
		s = s->Next();
	m_current_state = const_cast<BRenderState *>(s);
	m_hole_state = &(const_cast<BRenderStream *>(current)->HoleState());

	// Set the current stream
	m_current_stream = const_cast<BRenderStream *>(current);
}

lock_status_t BRenderHole::Lock()
{
	lock_status_t status = m_lock.Lock();
	if (m_lock.NestingLevel() == 1)
		status = OGLDevice.Context_Lock();
	return status;
}

void BRenderHole::Unlock()
{
	if (m_lock.NestingLevel() == 1) {
		m_current_stream = NULL;
		OGLDevice.Context_Unlock();
	}
	m_lock.Unlock();
}

// *************************************************************
// #pragma mark -

void BRenderHole::MoveTo(const BPoint& pt)
{
	PRINT_COMMAND(	bout << "MoveTo(" << pt << ")" << endl; )
	// ---- dumb ass implementation ----
	State().path[State().cntpt++] = State().Transform().Transform(pt);
}

void BRenderHole::LineTo(const BPoint& endpoint)
{
	PRINT_COMMAND(	bout << "LineTo(" << endpoint << ")" << endl; )
	// ---- dumb ass implementation ----
	State().path[State().cntpt++] = State().Transform().Transform(endpoint);
}

void BRenderHole::BezierTo(const BPoint& p1, const BPoint& p2, const BPoint& endpoint)
{
	PRINT_COMMAND(	bout << "BezierTo(" << p1 << "," << p2 << "," << endpoint << ")" << endl; )
}

void BRenderHole::ArcTo(const BPoint& p1, const BPoint& p2, coord radius)
{
	PRINT_COMMAND(	bout << "ArcTo(" << p1 << "," << p2 << "," << radius << ")" << endl; )
}

void BRenderHole::Arc(const BPoint& center, coord radius, float startAngle, float arcLen, bool connected)
{
	PRINT_COMMAND(	bout << "Arc(" << center << "," << radius << "," << startAngle << "," << arcLen << "," << connected << ")" << endl; )
}

void BRenderHole::Text(const char *text, int32 len, const escapements& escapements)
{
//	PRINT_COMMAND(	bout << "Text(" << text << "," << len << ")" << endl; )
	// more stupidity for testing purposes - append the string - ignore the escapements
	State().string.Append(text, len);
}

void BRenderHole::Close()
{
	PRINT_COMMAND(	bout << "Close()" << endl; )
	// ---- dumb ass implementation ----
	State().closed = true;
}

void BRenderHole::Clear()
{
	PRINT_COMMAND(	bout << "Clear()" << endl; )
	// ---- dumb ass implementation ----
	State().cntpt = 0;
	State().closed = false;
	State().string.SetTo(NULL);
}

void BRenderHole::Stroke()
{
	PRINT_COMMAND(	bout << "Stroke()" << endl; )
}

void BRenderHole::TextOnPath(const char *text, int32 len, const escapements& escapements, uint32 flags)
{
//	PRINT_COMMAND(	bout << "TextOnPath(" << text << "," << (void*)flags << ")" << endl; )
	for (int i=0 ; i<len ; i++)
		bout << text[i];
	bout << endl;
}

void BRenderHole::SetFont(const BFont &font)
{
	PRINT_COMMAND(	bout << "SetFont(...)" << endl; )
}

void BRenderHole::SetWindingRule(winding_rule rule)
{
	PRINT_COMMAND(	bout << "SetWindingRule(" << (int)rule << ")" << endl; )
}

void BRenderHole::SetStroke(coord penWidth, cap_mode capping, join_mode joining, float miter, coord *stippling)
{
	PRINT_COMMAND(	bout << "SetStroke(" << penWidth << "," << (int)capping << "," << (int)joining << "," << miter << ", ..." << ")" << endl; )
}

void BRenderHole::Fill()
{
	PRINT_COMMAND(	bout << "Fill()" << endl; )
	BColor c(1,1,1,1);
	Color(c);
}

void BRenderHole::Color(const BColor& c)
{
	PRINT_COMMAND(	bout << "Color(" << c << ")" << endl; )
	// ---- dumb ass implementation ----
	if ((State().cntpt == 4) && (State().closed))
	{
		BPoint *p = State().path;
		glColor4f( c.red, c.green, c.blue, c.alpha );
		glBegin( GL_QUADS );
			glVertex2f( p[0].x, p[0].y );
			glVertex2f( p[1].x, p[1].y );
			glVertex2f( p[2].x, p[2].y );
			glVertex2f( p[3].x, p[3].y );
		glEnd();
	}
	// render text the slow and evil way!
	if (State().string.Length() != 0) {
		IFontEngine::ptr engine = new BFontEngine;
		
		// Create a texture big enough to hold the largest glyph
		// texture size must be a power of 2.
		uint32 texture_id;				
		OGLDevice.Texture_Create(&texture_id);
		OGLDevice.Texture_Select(texture_id);
		OGLDevice.Texture_Load(32, 32, B_GRAY8, NULL);
		
		// Activate alpha blending, set the color
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glColor4f( c.red, c.green, c.blue, c.alpha );

		const char *string = State().string.String();
		const int32 len = State().string.Length();
		IFontEngine::glyph_metrics gm;
		uchar *bits = NULL;
		
		BPoint curPt(0,0);
		if (State().cntpt > 0)
			curPt = State().path[State().cntpt - 1];
		
		for (int ix = 0; ix < len; ix++) {
			status_t status = engine->RenderGlyph(string[ix], NULL, &gm, (void **)&bits);
			if (status == B_OK) {
				//place the pixels - again very stupidly
				if (bits != NULL) {
					const int32 width = (int32)gm.bounds.Width();
					const int32 height = (int32)gm.bounds.Height();
					OGLDevice.Texture_SubLoad(width, height, 0,0, B_GRAY8, bits);
					BRect src(0,0,width,height);
					BPoint dst(curPt.x + gm.bounds.left, curPt.y + gm.bounds.top);
					blit_text_texture(texture_id, src, dst);
					free(bits); bits = NULL;
				}		
				// update the pt
				curPt.x += gm.x_escape;
				curPt.y += gm.y_escape;

				// reset the bits and gm
				gm.reset();
			} 
		}
		OGLDevice.Texture_Release(texture_id);
		glDisable(GL_BLEND);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}

	// Clear the path
	Clear();
}

void BRenderHole::Shade(const BGradient&)
{
	PRINT_COMMAND(	bout << "Shade(...)" << endl; )
}

void BRenderHole::Transform(const B2dTransform& tr)
{
	PRINT_COMMAND(	bout << "Transform(" << tr << ")" << endl; )
	m_current_state->ApplyTransform(tr);
}

void BRenderHole::Transform(const BColorTransform&)
{
	PRINT_COMMAND(	bout << "Transform(...)" << endl; )
}

void BRenderHole::BeginComposition()
{
	PRINT_COMMAND(	bout << "BeginComposition()" << endl; )
}

void BRenderHole::EndComposition()
{
	PRINT_COMMAND(	bout << "EndComposition()" << endl; )
}

void BRenderHole::PushState()
{
	PRINT_COMMAND(	bout << "PushState()" << endl; )
	BRenderState::Push(&m_current_state, new BRenderState(*m_current_state));
}

void BRenderHole::PopState()
{
	PRINT_COMMAND(	bout << "PopState()" << endl; )
	BRenderState::Pop(&m_current_state);
}

void BRenderHole::BeginPixelBlock(const BPixelDescription& pixels)
{
	PRINT_COMMAND(	bout << "BeginPixelBlock(...)" << endl; )
	OGLDevice.Texture_Create(&(m_hole_state->texture_id));
	OGLDevice.Texture_Select(m_hole_state->texture_id);
	OGLDevice.Texture_Load(pixels.Width(), pixels.Height(), B_RGB32, NULL);
}

void BRenderHole::PlacePixels(const BRasterPoint& at, const BPixelData& pixels)
{
	PRINT_COMMAND(	bout << "PlacePixels(" << at << ", ...)" << endl; )
	OGLDevice.Texture_Select(m_hole_state->texture_id);
	OGLDevice.Texture_SubLoad(pixels.Width(), pixels.Height(), at.x, at.y, B_RGB32, pixels.Data());
}

void BRenderHole::EndPixelBlock()
{
	PRINT_COMMAND(	bout << "EndPixelBlock()" << endl; )
	blit_texture(m_hole_state->texture_id, m_hole_state->texture_frame, m_hole_state->texture_frame);
	OGLDevice.Texture_Release(m_hole_state->texture_id);
}

bool BRenderHole::DisplayCached(const atom_ptr<IBinder>& binder, uint32 flags, int32 cache_id)
{
	PRINT_COMMAND(	bout << "DisplayCached(" << binder << "," << (void*)flags << "," << cache_id << ")" << endl; )
	const TextureCache::texture_t *texture = OGLDevice.GetTexture(binder);
	if (texture && (texture->cache_id == (uint32)cache_id)) {
		PRINT_COMMAND (bout << "IVisual " << binder << " has a texture cached (texture id = " << texture->texture_id << ")" << endl;)
		blit_texture(texture->texture_id, texture->frame, texture->frame);
		return false;
	} else {
		if (!texture) {
			PRINT_COMMAND (bout << "IVisual " << binder << " has no texture cached" << endl;)
			IVisual::ptr visual = IVisual::AsInterface(binder);
			const BRect frame = visual->Bounds();
			const uint32 width = (uint32)frame.Width();
			const uint32 height = (uint32)frame.Height();
			TextureCache::texture_t t;
			t.cache_id = cache_id;
			t.frame = frame;
			
			OGLDevice.Texture_Create(&t.texture_id);
			OGLDevice.Texture_Select(t.texture_id);
			OGLDevice.Texture_Load(width, height, B_RGB32, NULL);
			OGLDevice.CacheTexture(binder, t);

			// Save the current texture state
			m_hole_state->texture_id = t.texture_id;
			m_hole_state->texture_frame = t.frame;
			bool branched = Stream().DrawVisual(visual);
			if (branched == false) {
				blit_texture(t.texture_id, t.frame, t.frame);
			}
			return branched;
		} else {
			PRINT_COMMAND (bout << "IVisual " << visual << " texture cached, but it must be updated" << endl;)
			#warning Update the texture
		}
	}
	return false;
}

void BRenderHole::blit_texture(uint32 texture_id, const BRect& src, const BRect& dst)
{
	BPoint pts[4];
	const B2dTransform& tr = State().Transform();
	pts[0] = tr.Transform(dst.LeftTop());
	pts[1] = tr.Transform(dst.RightTop());
	pts[2] = tr.Transform(dst.RightBottom());
	pts[3] = tr.Transform(dst.LeftBottom());

#if DUMB_PROFILE
	bigtime_t now = -system_time();
#endif

	OGLDevice.Texture_Select(texture_id);
	OGLDevice.Texture_Enable(true);
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	glBegin( GL_QUADS );
		glTexCoord2f(src.right, src.top);		glVertex2f( pts[1].x, pts[1].y );
		glTexCoord2f(src.right, src.bottom);	glVertex2f( pts[2].x, pts[2].y );
		glTexCoord2f(src.left, src.bottom);		glVertex2f( pts[3].x, pts[3].y );
		glTexCoord2f(src.left, src.top);		glVertex2f( pts[0].x, pts[0].y );
	glEnd();
	OGLDevice.Texture_Enable(false);

#if DUMB_PROFILE
	now += system_time();
	bout << "profile: (" << find_thread(0) << ") " << src.Width() << "x" << src.Height() << " bitmap in " << now/1000.0f << "ms" << endl;
#endif
}

void BRenderHole::blit_text_texture(uint32 texture_id, const BRect& src, const BPoint& dst)
{
	BPoint pts[4];
	const B2dTransform& tr = State().Transform();
	pts[0] = tr.Transform(dst + src.LeftTop());
	pts[1] = tr.Transform(dst + src.RightTop());
	pts[2] = tr.Transform(dst + src.RightBottom());
	pts[3] = tr.Transform(dst + src.LeftBottom());
	OGLDevice.Texture_Select(texture_id);
	OGLDevice.Texture_Enable(true);
	glBegin( GL_QUADS );
		glTexCoord2f(src.right, src.top);		glVertex2f( pts[1].x, pts[1].y );
		glTexCoord2f(src.right, src.bottom);	glVertex2f( pts[2].x, pts[2].y );
		glTexCoord2f(src.left, src.bottom);		glVertex2f( pts[3].x, pts[3].y );
		glTexCoord2f(src.left, src.top);		glVertex2f( pts[0].x, pts[0].y );
	glEnd();
	OGLDevice.Texture_Enable(false);
}

