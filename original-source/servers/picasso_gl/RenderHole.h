
#ifndef RENDER_HOLE_H
#define RENDER_HOLE_H

#include <support2/Atom.h>
#include <support2/Autolock.h>
#include <support2/KeyedVector.h>
#include <support2/Locker.h>
#include <render2/Render.h>
#include <render2/RenderDefs.h>
#include <render2/RenderState.h>

using namespace B::Support2;
using namespace B::Raster2;
using namespace B::Interface2;


enum {
// Return values of ProcessBuffer()
	B_STILL_WAITING = 1,
	B_NOT_ACTIVE	= 2,
	B_BRANCHED		= 3
};


class BSurface;
class BRenderStream;

struct hole_state_t {
	uint32	texture_id;
	BRect	texture_frame;
};


class BRenderHole : public BAtom
{
public:

	B_STANDARD_ATOM_TYPEDEFS(BRenderHole)

							BRenderHole(BSurface *surface);
							~BRenderHole();

			lock_status_t	Lock();
			void			Unlock();

			void			AssertState(const BRenderStream *current);

			BSurface *		Surface() { return m_surface; }
			BRenderStream&	Stream() const { return *m_current_stream; }

	void	MoveTo(const BPoint& pt);
	void	LineTo(const BPoint& endpoint);
	void	BezierTo(const BPoint& pt1, const BPoint& pt2, const BPoint& endpoint);
	void	ArcTo(const BPoint& p1, const BPoint& p2, coord radius);
	void	Arc(const BPoint& center, coord radius, float startAngle, float arcLen, bool connected);
	void	Text(const char *text, int32 len, const escapements& escapements);
	void	Close();
	void	Clear();
	void	Stroke();
	void	TextOnPath(const char *text, int32 len, const escapements& escapements, uint32 flags);
	void	SetFont(const BFont &font);
	void	SetWindingRule(winding_rule rule);
	void	SetStroke(coord penWidth, cap_mode capping, join_mode joining, float miter, coord *stippling);
	void	Fill();
	void	Color(const BColor&);
	void	Shade(const BGradient&);
	void	Transform(const B2dTransform&);
	void	Transform(const BColorTransform&);
	void	BeginComposition();
	void	EndComposition();
	void	PushState();
	void	PopState();
	void	BeginPixelBlock(const BPixelDescription& pixels);
	void	PlacePixels(const BRasterPoint& at, const BPixelData& pixels);
	void	EndPixelBlock();
	bool	DisplayCached(const atom_ptr<IBinder>& visual, uint32 flags, int32 cache_id);

private:
	BRenderState&	State() const { return *m_current_state; }
	void blit_text_texture(uint32 texture_id, const BRect& src, const BPoint& dst);
	void blit_texture(uint32 texture_id, const BRect& src, const BRect& dst);

private:
	BSurface 										*m_surface;
	BRenderStream									*m_current_stream;
	BRenderState									*m_current_state;
	BNestedLocker									m_lock;
	hole_state_t									*m_hole_state;
};

#endif
