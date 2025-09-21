
#ifndef RENDER_STREAM_H
#define RENDER_STREAM_H

#include <stdlib.h>

#include <interface2/View.h>
#include <render2/RenderState.h>
#include <support2/Handler.h>
#include <support2/Parcel.h>
#include <support2/ByteStream.h>
#include <support2/MemoryStore.h>

#include <render2_p/RenderInputPipe.h>

#include "RenderHole.h"



class BRenderStream;



/////////////////////////////////////////////////////////////////////////////////
// BRenderStreamProxy
/////////////////////////////////////////////////////////////////////////////////

class BRenderStreamProxy : public BBinder
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BRenderStreamProxy)

	BRenderStreamProxy(atom_ptr<BRenderStream> stream);
	~BRenderStreamProxy();

	virtual	status_t		Transact(	uint32 code,
										BParcel& data,
										BParcel* reply = NULL,
										uint32 flags = 0);
	virtual status_t		Called(BValue &in, const BValue &outBindings, BValue &out);
	virtual	BValue			Inspect(const BValue &which, uint32 flags = 0);
	void 					DrawRemote(const IVisual::ptr& visual);

private:
	atom_ptr<BRenderStream>		m_stream;
};

/////////////////////////////////////////////////////////////////////////////////
// BRenderStream
/////////////////////////////////////////////////////////////////////////////////

class BRenderStream : public BBinder
{
public:
		B_STANDARD_ATOM_TYPEDEFS(BRenderStream)

		BRenderStream(const BRenderHole::ptr& hole);
		BRenderStream(const BRenderStream::ptr& parent, uint32 flags = 0);
		~BRenderStream();

	virtual	status_t		Transact(	uint32 code,
										BParcel& data,
										BParcel* reply = NULL,
										uint32 flags = 0);
	virtual status_t		Called(BValue &in, const BValue &outBindings, BValue &out);
	virtual	BValue			Inspect(const BValue &which, uint32 flags = 0);
	
			bool						DrawVisual(const IVisual::ptr& visual);
			void						MakeActive(bool state = true);
			void						Finished();

			const IVisual::const_ptr&	Visual() 			{ return m_visual; }
			const BRenderState *		State() const		{ return &m_state; }
			BRenderInputPipe& 			InputPipe() 		{ return m_input; }
			BRenderHole&				Hole()				{ return m_hole_ref; }
			hole_state_t&				HoleState()			{ return m_hole_state; }

			ssize_t 					ProcessBuffer();

// Temporary state for the stream decoding
			
			struct vsize_t {
				vsize_t() : b(NULL), al(0) { }
				~vsize_t() { free((void*)b); }
				const void *b;
				int32 l;
				size_t al;
				status_t init(size_t s) {
					l = s; cur = 0;
					al = l + ((4 - (l & 0x3)) & 0x3);
					b = malloc(al);
					return ((b) ? (B_OK) : (B_NO_MEMORY));
				}
				size_t append(const char *buffer, size_t size) {
					memcpy((uint8*)b+cur, buffer, size);
					cur += size; al -= size;
					return al;
				}
				void reset() {
					free((void*)b);	b = NULL;
					al = 0;
				}
			private:
				size_t cur;
			};
			
			struct text_state_t : public vsize_t {
				text_state_t() : vsize_t() { }
				escapements e;
				uint32 f;
			};
						
			struct bitmap_state_t : public vsize_t {
				enum { INIT, BEGIN, PLACE, END };
				bitmap_state_t() : vsize_t() { reset(); reinit(); }
				BPixelDescription	texture;
				BPixelData			sub_texture;
				BRasterPoint		at;
				int32				state;
				bool				begin_called;
				void reinit() {
					state = INIT;
					begin_called = false;
				}
			};

			union { // We only have one state at a time
				text_state_t		*m_textState;
				bitmap_state_t		*m_bitmapState;
			};
			text_state_t& TextState() {
				if (m_textState == NULL)
					m_textState = new text_state_t;
				return *m_textState;
			}
			bitmap_state_t& BitsState() {
				if (m_bitmapState == NULL)
					m_bitmapState = new bitmap_state_t;
				return *m_bitmapState;
			}
			void ResetTextState() {
				delete m_textState;
				m_textState = NULL;
			}
			void ResetBitsState() {
				delete m_textState;
				m_textState = NULL;
			}


private:
			status_t			Draw();
			ssize_t				AddDependency(const BRenderStream::ptr &child);
			void				RemoveDependency(const BRenderStream::ptr& child);
			bool				IsAsync()			{ return (m_flags & IRender::B_ASYNCHRONOUS); }
			BRenderStream::ptr	FindDependency(int32 id);

// our internal state
	uint32						m_opcode;
	uint32						m_count;
	int32						m_flags;
	BLocker						m_lock;
	BRenderHole::ptr			m_hole;
	BRenderHole&				m_hole_ref;
	BRenderStream::ptr			m_parent;
	BRenderState				m_state;
	BRenderInputPipe			m_input;
	IVisual::const_ptr			m_visual;
	bool						m_isActive : 1;

	// The renderhole needs a state per stream
	hole_state_t				m_hole_state;

	// the pending branches list
	BLocker											m_lockBranchList;
	BKeyedVector<int32, atom_ptr<BRenderStream> >	m_branches;

// the dependency list management
	struct dependency_t {
		dependency_t(const BRenderStream::ptr& child) : dep(child), dep_ptr(child.ptr()), next(NULL) { };
		BRenderStream::ref		dep;
		const BRenderStream		*dep_ptr;
		dependency_t			*next;
	};
	struct dependency_list {
		dependency_list() : m_head(NULL), m_tail(&m_head) {}
		dependency_t			*m_head;
		dependency_t			**m_tail;
		BLocker					m_lock;
		BRenderStream::ref head() {
			return m_head ? (m_head->dep) : NULL;
		}
		void add(dependency_t *item) {
			*m_tail = item;
			m_tail = &item->next;
		}
		dependency_t *remove(const BRenderStream *stream) {
			dependency_t **item = &m_head;
			while (*item) {
				if ((*item)->dep_ptr == stream) {
					dependency_t *elem = *item;
					if (m_tail == &(elem->next))
						m_tail = item;
					*item = elem->next;
					return elem;
				}
				item = &((*item)->next);
			}
			return NULL;
		}
		void add_locked(dependency_t *item) {
			BAutolock locker(m_lock.Lock());
			add(item);
		}
		dependency_t *remove_locked(const BRenderStream *stream) {
			BAutolock locker(m_lock.Lock());
			return remove(stream);
		}
	};
	dependency_list				m_pending_list;
	dependency_list				m_active_list;
	dependency_list				m_async_list;
};


#endif
