//******************************************************************************
//
//	File:			RenderStream.cpp
//
//	Description:	BRenderStream & BRenderStreamProxy implementation
//	
//	Written by:		Mathias Agopian, George Hoffman
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>

#include <support2_p/BinderKeys.h>

#include <support2/Atom.h>
#include <support2/ByteOrder.h>
#include <support2/StdIO.h>
#include <support2/TextStream.h>
#include <support2_p/BinderKeys.h>
#include <render2/Render.h>
#include <render2_p/RenderProtocol.h>
#include "RenderStream.h"
#include "RenderHole.h"
#include "Surface.h"
#include "OGLDevice.h"

using namespace B::Support2;
using namespace B::Render2;
using namespace B::Private;

#define DEBUG_DEPENDENCY	1

/******************************************************************************/

BRenderStreamProxy::BRenderStreamProxy(atom_ptr<BRenderStream> stream)
	: m_stream(stream)
{
}
BRenderStreamProxy::~BRenderStreamProxy()
{
}
status_t BRenderStreamProxy::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags) {
	return m_stream->Transact(code, data, reply, flags);
}
status_t BRenderStreamProxy::Called(BValue &in, const BValue &outBindings, BValue &out) {
	return m_stream->Called(in, outBindings, out);
}
BValue BRenderStreamProxy::Inspect(const BValue &in, uint32 flags) {
	return m_stream->Inspect(in,flags);
}
void BRenderStreamProxy::DrawRemote(const IVisual::ptr& visual) {
	visual->AsBinder()->Put(BValue(g_keyDraw, BValue::Binder(this)));
}


/******************************************************************************/

BRenderStream::BRenderStream(const BRenderHole::ptr& hole)
	:	m_opcode(GRP_NOP), m_count(0),
		m_textState(NULL),
		m_flags(0),
		m_hole(hole),
		m_hole_ref(*m_hole),
		m_isActive(true)
{
}

BRenderStream::BRenderStream(const BRenderStream::ptr& parent, uint32 flags)
	:	m_opcode(GRP_NOP), m_count(0),
		m_textState(NULL), 
		m_flags(flags),
		m_hole(parent->m_hole),
		m_hole_ref(*m_hole),
		m_parent(parent),
		m_isActive(false),
		m_hole_state(parent->m_hole_state)
{
}


BRenderStream::~BRenderStream()
{
	#if DEBUG_DEPENDENCY
		if (m_active_list.m_head != NULL) {
			debugger("BRenderStream deleted, but m_active_list is not empty!");
		}
		if (m_pending_list.m_head != NULL) {
			debugger("BRenderStream deleted, but m_pending_list is not empty!");
		}
		if (m_async_list.m_head != NULL) {
			debugger("BRenderStream deleted, but m_async_list is not empty!");
		}
		if ((m_textState) || (m_bitmapState)) {
			debugger("BRenderStream deleted, but a text/bits state is still active!");
		}
		if (m_branches.CountItems() != 0) {
			debugger("BRenderStream deleted, but m_branches.CountItems() != 0");
		}
	#endif
}

// ---------------------------------------------------------------
// #pragma mark -

bool BRenderStream::DrawVisual(const IVisual::ptr& visual)
{
	BRenderStream::ptr child = new BRenderStream(m_hole.ptr());
	child->m_visual = visual;
	child->m_hole_state = m_hole_state;
	(new BRenderStreamProxy(child))->DrawRemote(visual);
	m_hole->AssertState(this);
	return false;
}

status_t BRenderStream::Draw()
{
	status_t err;
	if (m_isActive == false)
		return B_NOT_ACTIVE;

	while (true)
	{
		// If we have dependencies
		BRenderStream::ptr dep = m_active_list.head().promote();
		if (dep != NULL) {
			// And if they're active, then Draw() them first
			if ((err = dep->Draw()) != B_OK) {
				// the dependency needs more data. stop here.
				return err;
			}
			// This dependency just finished, check if we have another one
			continue;
		}

		err = ProcessBuffer();
		if (err == B_BRANCHED) {
			// The stream we just branched might be async,
			// in this case, we draw it now (there is no
			// ordering imposed by an async stream, but the
			// sooner the better). We hafta to do that because
			// the clientside of the stream might be finished
			// and so it won't receive any buffer anymore.
			const dependency_t *walk = m_async_list.m_head;
			while (walk) {
				BRenderStream::ptr dep = walk->dep.promote();
				walk = walk->next;
				if (dep != NULL) {
					dep->Draw();
				}
			}
			continue;
		} else if (err == B_OK) {
			// This Stream has just finished its work.
			// If it has a parent, jump to it.
			if (m_parent != NULL) {
				m_parent->Draw();
			}
			return B_OK;			
		}
		return err;
	}
}

ssize_t BRenderStream::AddDependency(const BRenderStream::ptr& child)
{
	dependency_t *item = new(std::nothrow) dependency_t(child);
	if (item) {
		m_pending_list.add_locked(item);
		BAutolock locker(m_lockBranchList.Lock());
		const int32 id = (int32)(child.ptr());
		m_branches.AddItem(id, child);
		return id;
	}
	return B_NO_MEMORY;
}

void BRenderStream::RemoveDependency(const BRenderStream::ptr& child)
{
	delete (m_pending_list.remove_locked(child.ptr()));
	BAutolock locker(m_lockBranchList.Lock());
	m_branches.RemoveItemFor((int32)child.ptr());
}

BRenderStream::ptr BRenderStream::FindDependency(int32 id)
{
	BAutolock locker(m_lockBranchList.Lock());
	bool found;
	BRenderStream::ptr branch = m_branches.ValueFor(id, &found);
	if (found == false) {
		branch = NULL;
	}
	return branch;
}

void BRenderStream::Finished()
{
	// When this is called, the Hole() lock is held
	MakeActive(false);
	InputPipe().PreloadIfAvaillable();
	if (m_parent != NULL) {
		m_parent->RemoveDependency(this);
	}	
}

void BRenderStream::MakeActive(bool state)
{
	// When this is called, the Hole() lock is held
	if (state != m_isActive) {
		if (state == true) {
			m_state = BRenderState(m_parent->m_state);
			if (m_parent != NULL) {
				// Move this stream at the end of its parent's active list
				dependency_t *item = m_parent->m_pending_list.remove_locked(this);
				if (item) {
					if (IsAsync())	m_parent->m_async_list.add(item);
					else			m_parent->m_active_list.add(item);
				}
			}
		} else {
			if (m_parent != NULL) {
				// Move this stream in its parent's pending list
				dependency_t *item;
				if (IsAsync())	item = m_parent->m_async_list.remove(this);
				else			item = m_parent->m_active_list.remove(this);
				if (item) {
					m_parent->m_pending_list.add_locked(item);
				}
			}
		}
		m_isActive = state;
	}
}

// *************************************************************
// #pragma mark -

ssize_t BRenderStream::ProcessBuffer()
{
	uint8 opcode = m_opcode;
	uint32 count = m_count;
	BRenderInputPipe& input = InputPipe();

	#define READ(_x)							\
			if (input.read(_x) != B_OK) {		\
				goto end_of_stream;				\
			}

	m_hole->AssertState(this);

	if (count>0) {
		// We were in the middle of a command
		goto decode;
	}

	while (input.readOp(&opcode, &count) == B_OK) {
decode:	
		switch (opcode) {
			case GRP_NOP:
				count = 0;
				break;
			case GRP_FINISHED:
				Finished();				
				count = 0;
				goto finished;
			case GRP_BRANCH:
				do {
					int32 id;
					READ(&id);
					BRenderStream::ptr branch = FindDependency(id);
					if (branch != NULL) {
						branch->MakeActive();
						count--;
						goto branched;
					}
				} while (--count);
				break;

			// Primitive path creation and manipulation
			case GRP_MOVE_TO:
				{
					BPoint p;
					do {
						READ(&p);
					} while (--count);
					Hole().MoveTo(p);
				}
				break;

			case GRP_LINE_TO:
				do {
					BPoint p;
					READ(&p);
					Hole().LineTo(p);
				} while (--count);
				break;

			case GRP_BEZIER_TO:
				do {
					BPoint pts[3];
					READ(pts+0);
					READ(pts+1);
					READ(pts+2);
					Hole().BezierTo(pts[0],pts[1],pts[2]);
				} while (--count);
				break;

			case GRP_ARC_TO:
				do {
					BPoint pts[2];
					coord r;
					READ(pts+0);
					READ(pts+1);
					READ(&r);
					Hole().ArcTo(pts[0],pts[1],r);
				} while (--count);
				break;

			case GRP_ARC:
			case GRP_ARC_CONECTED:
				do {
					BPoint p;
					coord r;
					float s, e;
					READ(&p);
					READ(&r);
					READ(&s);
					READ(&e);
					Hole().Arc(p,r,s,e, (opcode == GRP_ARC_CONECTED));
				} while (--count);
				break;

			case GRP_TEXT:
			case GRP_TEXT_ESCAPEMENT:
				do {
					BRenderStream::text_state_t& state = TextState();
					if (state.al == 0) {
						state.e = B_NO_ESCAPEMENT;
						if (opcode == GRP_TEXT_ESCAPEMENT) {
							READ(&state.e);
						}
						int32 size;
						READ(&size);
						if (state.init(size) != B_OK) {
							debugger("not enough memory!");
							#warning Handle B_NO_MEMORY better
						}
					}
					const uint8 *p;
					size_t size;
					while ((size = input.readptr(&p, state.al))) {
						if (state.append((const char *)p, size) == 0) {
							Hole().Text((const char *)state.b, state.l, state.e);
							ResetTextState();
							break;
						}
					}
					if (size == 0)
						goto end_of_stream;
				} while (--count);
				break;

			case GRP_CLOSE:
				Hole().Close();
				count = 0;
				break;

			case GRP_CLEAR:
				Hole().Clear();
				count = 0;
				break;

			// Composite path operations
			case GRP_STROKE:
				do {
					Hole().Stroke();
				} while (--count);
				break;

			case GRP_TEXT_ON_PATH:
				do {
					BRenderStream::text_state_t& state = TextState();
					if (state.al == 0) {
						int32 size;
						READ(&state.e);
						READ(&state.f);
						READ(&size);
						if (state.init(size) != B_OK) {
							debugger("not enough memory!");
							#warning Handle B_NO_MEMORY better
						}
					}
					const uint8 *p;
					size_t size;
					while ((size = input.readptr(&p, state.al))) {
						if (state.append((const char *)p, size) == 0) {
							Hole().TextOnPath((const char *)state.b, state.l, state.e, state.f);
							ResetTextState();
							break;
						}
					}
					if (size == 0)
						goto end_of_stream;
				} while (--count);
				break;

			// Path filling
			case GRP_FILL:
				Hole().Fill();
				count = 0;
				break;

			case GRP_COLOR:
				{	BColor c;
					do {
						READ(&c);
					} while (--count);
					Hole().Color(c);
				} break;

			case GRP_SHADE:
				do {
					BGradient gradient;
					READ(&gradient);
					Hole().Shade(gradient);
				} while (--count);
				break;

			// Path generation state
			case GRP_SET_FONT:
				#warning implement GRP_SET_FONT
				break;

			case GRP_SET_WINDING_RULE:
				{	int32 wr;
					do {
						READ(&wr);
					} while (--count);
					Hole().SetWindingRule((winding_rule)wr);
				} break;

			case GRP_SET_STROKE:
				#warning implement GRP_SET_STROKE
				break;

			// Composition stacks
			case GRP_TRANSFORM_2D:
				do {
					B2dTransform t;
					READ(&t);
					Hole().Transform(t);
				} while (--count);
				break;

			case GRP_TRANSFORM_COLOR:
				do {
					BColorTransform t;
					READ(&t);
					Hole().Transform(t);
				} while (--count);
				break;

			case GRP_BEGIN_COMPOSE:
				do {
					Hole().BeginComposition();
				} while (--count);
				break;

			case GRP_END_COMPOSE:
				do {
					Hole().EndComposition();
				} while (--count);
				break;

			// Save/restore state
			case GRP_PUSH_STATE:
				do {
					Hole().PushState();
				} while (--count);
				break;

			case GRP_POP_STATE:
				do {
					Hole().PopState();
				} while (--count);
				break;

			// Pixmap/sample support
			case GRP_BEGIN_PIXELS:
				if (count != 1) {
					berr << "GRP_BEGIN_PIXELS protocol error" << endl;
					goto protocol_error;
				} else {
					uint32 w,h,f;
					pixel_format cs;
					READ(&w);
					READ(&h);
					READ(&f);
					READ(&cs);
					BRenderStream::bitmap_state_t& state = BitsState();
					state.texture = BPixelDescription(w,h,cs,f);
					state.state = BRenderStream::bitmap_state_t::BEGIN;
					count = 0;
				}
				break;

			case GRP_PLACE_PIXELS:
				do {
					BRenderStream::bitmap_state_t& state = BitsState();
					if ((state.state != BRenderStream::bitmap_state_t::BEGIN) &&
						(state.state != BRenderStream::bitmap_state_t::PLACE))
					{
						berr << "GRP_PLACE_PIXELS protocol error" << endl;
						goto protocol_error;
					}
					if (state.al == 0) {
						uint32 w,h,bpr,l;
						pixel_format cs;
						READ(&state.at);
						READ(&w);
						READ(&h);
						READ(&bpr);
						READ(&cs);
						READ(&l);
						if (state.init(l) != B_OK) {
							debugger("not enough memory!");
							#warning Handle B_NO_MEMORY better
						}
						state.state = BRenderStream::bitmap_state_t::PLACE;
						state.sub_texture = BPixelData(w,h,bpr,cs,state.b);
					}
					const uint8 *p;
					size_t size;
					while ((size = input.readptr(&p, state.al))) {
						if (state.append((const char *)p, size) == 0) {
							if (!state.begin_called) {
								state.begin_called = true;
								if (Visual() == NULL) { // we must create a intermediate texture
									Hole().BeginPixelBlock(state.texture);
								}
							}
							state.state = BRenderStream::bitmap_state_t::END;
							Hole().PlacePixels(state.at, state.sub_texture);
							state.reset();
							break;
						}
					}
					if (size == 0)
						goto end_of_stream;
				} while (--count);
				break;

			case GRP_END_PIXELS:
				if (count != 1) {
					berr << "GRP_END_PIXELS protocol error 1" << endl;
					goto protocol_error;
				} else {
					BRenderStream::bitmap_state_t& state = BitsState();
					if (state.state != BRenderStream::bitmap_state_t::END) {
						berr << "GRP_PLACE_PIXELS protocol error 2" << endl;
						goto protocol_error;
					}
					if (Visual() == NULL) { // we must create a intermediate texture
						Hole().EndPixelBlock();
					}
					ResetBitsState();
					count = 0;
				}
				break;

			// Drawing a visual
			case GRP_DISPLAY_CACHED:
				do {
					IBinder::ptr binder;
					uint32 flags;
					int32 cache_id;
					READ(&binder)
					READ(&flags)
					READ(&cache_id)
					if (Hole().DisplayCached(binder, flags, cache_id) == true) {
						count--;
						goto branched;
					}
				} while (--count);
				break;

			default:
				goto protocol_error;
		}
		if (count) {
		// if the count is not null, repeat the command
			count--;
			goto decode;
		}
	}
	m_opcode = GRP_NOP;
	m_count = 0;
	return B_STILL_WAITING;

finished:
	m_opcode = GRP_NOP;
	m_count = 0;
	return B_OK;

end_of_stream:
	m_opcode = opcode;
	m_count = count;
	return B_STILL_WAITING;

branched:
	m_opcode = opcode;
	m_count = count;
	return B_BRANCHED;

protocol_error:
	debugger("protocol error");
	return B_ERROR;
}

// ---------------------------------------------------------------
// #pragma mark -

status_t BRenderStream::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	if ((code == B_RENDER_TRANSACTION_CMDS) ||
		(code == B_RENDER_TRANSACTION_DATA))
	{
		if (reply) {
			ssize_t result = data.Length();
			reply->Copy(&result, sizeof(result));
		}
		m_input.Receive(code, data, reply, flags);
		if (m_hole->Lock() == B_OK) {
			Draw();
			m_hole->Unlock();
		} 
		return B_OK;
	}
	return BBinder::Transact(code, data, reply, flags);
}

status_t BRenderStream::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	// bout << "Called" << endl << indent << in << endl <<  outBindings << dedent << endl;
	BValue branch = in[g_keyBranch];
	if (branch) {
		// Create an inactive RenderStream
		uint32 flags = branch["flags"].AsInteger();
		BRenderStream::ptr child = new BRenderStream(this, flags);
		BRenderStreamProxy::ptr proxy = new BRenderStreamProxy(child);

		// Add the branch to the pending list
		// Get an ID for it and send it back to the client
		const int32 id = AddDependency(child);
		out = outBindings *
			BValue(g_keyBranch,	BValue().Overlay("stream", BValue::Binder(proxy))
										.Overlay("id", BValue::Int32(id))); 
	}

	return B_OK;
}


BValue BRenderStream::Inspect(const BValue &which, uint32)
{
	return which * BValue(IRender::descriptor, BValue::Binder(this));
}
