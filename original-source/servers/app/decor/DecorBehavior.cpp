
#include <stdio.h>

#include <AppDefs.h>

#include "as_support.h"
#include "enums.h"
#include "DecorBehavior.h"
#include "DecorActionDef.h"
#include "DecorState.h"

struct Instinct {
	int32 event;
	DecorAtom *action;
	union {
		struct {
			int16 button;
			uint32 require_mods;
			uint32 reject_mods;
			union {
				int16 click;
				int16 transit;
			} which;
			int16 mods;
		} mouse;
		int32 delay;
	} params;
};

DecorBehavior::DecorBehavior()
	:	m_instincts(NULL), m_instinctCount(0)
{
}

DecorBehavior::~DecorBehavior()
{
	grFree(m_instincts);
}

DecorActionDef * DecorBehavior::React(DecorState *changes, DecorMessage *event)
{
//	DebugPrintf(("Behavior %08x reacting %ld\n",this,m_instinctCount));
	int32 selected = -1;
	for (int32 i=0;i<m_instinctCount;i++) {
		bool takeAction=false;
		bool mightMatch=false;
//		DebugPrintf(("'%.4s' ?= '%.4s' --> ",&event->what,&m_instincts[i].event));
		if (event->what == m_instincts[i].event) {
//			DebugPrintf(("yes\n"));
			if (	event->what == B_MOUSE_MOVED ||
					event->what == B_MOUSE_DOWN ||
					event->what == B_MOUSE_UP)
			{
				if (event->what == B_MOUSE_MOVED) {
					const int16 transit = (int16)event->Transit();
					if (transit != m_instincts[i].params.mouse.which.transit) {
						// The 'transit' is not matching
						mightMatch = false;
						takeAction = false;
						continue;
					}
				}

				const int32 buttons = event->FindInt32("buttons");
				const uint32 modifiers = (uint32)event->FindInt32("modifiers");
				
				// Check for matching buttons.
				mightMatch = true;
				if (m_instincts[i].params.mouse.button) {
					if ((buttons&m_instincts[i].params.mouse.button) == 0) {
						// Buttons requested, but not matching.  Skip.
//						DebugPrintf(("Buttons don't match, skip\n"));
						continue;
					}
					takeAction = true;
				}

				// Check for matching mouse move properties.
				if (mightMatch) {
					const uint32 req = m_instincts[i].params.mouse.require_mods;
					const uint32 rej = m_instincts[i].params.mouse.reject_mods;
					if ((req) || (rej)) {
						if ((modifiers&req) != req || (modifiers&rej) != 0) {
							mightMatch = false;
							takeAction = false;
						}
					}
				}
								
				// Checking for matching mouse down properties.
				if (mightMatch && event->what == B_MOUSE_DOWN) {
					const int32 clicks = event->FindInt32("clicks");
//					DebugPrintf(("Requested clicks=%d, clicks=%ld\n",
//							m_instincts[i].params.mouse.which.click, clicks));
					if (m_instincts[i].params.mouse.which.click < 0) {
						takeAction = false;
					} else if (clicks != m_instincts[i].params.mouse.which.click) {
						mightMatch = false;
						takeAction = false;
					}
				}				
			} else
				takeAction = true;
		} else {
//			DebugPrintf(("no\n"));
		};
		
		if (takeAction) {
//			DebugPrintf(("take Action!\n"));
			selected = i;
			break;
		} else if (mightMatch && selected < 0) {
			selected = i;
//			DebugPrintf(("Might match: %ld\n", selected));
		}
	};
	
	if (selected >= 0)
		return (DecorActionDef*)m_instincts[selected].action->Choose(changes->Space());
	
	return NULL;
}

void DecorBehavior::Unflatten(DecorStream *f)
{
	DecorSharedAtom::Unflatten(f);
	
	grFree(m_instincts);
	
	f->StartReadBytes();
	f->Read(&m_instinctCount);
	f->FinishReadBytes();
	
	m_instincts = (Instinct*)grMalloc(m_instinctCount*sizeof(Instinct), "decor",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_instinctCount;i++) {
		f->StartReadBytes();
		f->Read(&m_instincts[i].event);
		f->Read(&m_instincts[i].action);
		f->Read(&m_instincts[i].params.mouse.button);
		switch (m_instincts[i].event) {
			case B_MOUSE_DOWN:
				f->Read(&m_instincts[i].params.mouse.which.click);
				break;
			case B_MOUSE_MOVED:
				f->Read(&m_instincts[i].params.mouse.which.transit);
				break;
		}
		f->ReadOrSet(&m_instincts[i].params.mouse.require_mods, 0);
		f->ReadOrSet(&m_instincts[i].params.mouse.reject_mods, 0);
		f->FinishReadBytes();
	}
}

/**************************************************************/

#if defined(DECOR_CLIENT)
	
	#include "LuaDecor.h"

	DecorBehavior::DecorBehavior(DecorTable *t)
	{
		DecorTable *instinct;
		t = t->GetDecorTable("Instincts",ERROR);
		m_instinctCount = t->Count();
		m_instincts = (Instinct*)grMalloc(sizeof(Instinct)*m_instinctCount, "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_instinctCount;i++) {
			instinct = t->GetDecorTable(i+1,ERROR);
			m_instincts[i].event = (int32)instinct->GetNumber("Event");
			m_instincts[i].action = instinct->GetAtom("ActionDef",ERROR);
			if (!m_instincts[i].action->IsTypeOf(typeid(DecorActionDef)))
				panic(("ActionDef needs to be of type ACTIONDEF (duh)\n"));

			m_instincts[i].params.mouse.button = (int32)instinct->GetNumber("Button", 0);

			uint32 mods = (uint32)instinct->GetNumber("Modifiers", 0);
			if (mods != 0) {
				m_instincts[i].params.mouse.require_mods = mods;
				// This are the only ones we want to reject -- otherwise,
				// this parameter would be fairly useless.
				mods |= ~(B_SHIFT_KEY|B_COMMAND_KEY|B_CONTROL_KEY|B_OPTION_KEY|B_MENU_KEY);
				m_instincts[i].params.mouse.reject_mods = ~mods;
			} else {
				m_instincts[i].params.mouse.require_mods = (uint32)instinct->GetNumber("RequireModifiers", 0);
				m_instincts[i].params.mouse.reject_mods = (uint32)instinct->GetNumber("RejectModifiers", 0);
			}

			switch (m_instincts[i].event) {
				case MOUSE_UP: {
					m_instincts[i].event = B_MOUSE_UP;
				} break;
				case MOUSE_DOWN: {
					m_instincts[i].event = B_MOUSE_DOWN;
					m_instincts[i].params.mouse.which.click = (int32)instinct->GetNumber("Click", -1);
				} break;
				case MOUSE_MOVED:
				case MOUSE_ENTER:
				case MOUSE_EXIT: {
					int16 transit = B_INSIDE;
					switch (m_instincts[i].event) {
						case MOUSE_MOVED:	transit = B_INSIDE;		break;
						case MOUSE_ENTER:	transit = B_ENTERED;	break;
						case MOUSE_EXIT:	transit = B_EXITED;		break;
					}
					m_instincts[i].event = B_MOUSE_MOVED;
					m_instincts[i].params.mouse.which.transit = transit;
				} break;
				case ALARM: {
					m_instincts[i].event = B_PULSE;
					m_instincts[i].params.delay = (int32)instinct->GetNumber("Time", 0);
				} break;
				default: {
					panic(("Unknown event type %ld\n",m_instincts[i].event));
				} break;
			};
			delete instinct;
		};
		delete t;
	}
	
	void DecorBehavior::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		for (int32 i=0;i<m_instinctCount;i++) {
			m_instincts[i].action->RegisterAtomTree();
			m_instincts[i].action->RegisterDependency(this,dfEverything);
		};
	}
	
	bool DecorBehavior::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorBehavior)) || DecorAtom::IsTypeOf(t);
	}

	void DecorBehavior::Flatten(DecorStream *f)
	{
		DecorSharedAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_instinctCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_instinctCount;i++) {
			f->StartWriteBytes();
			f->Write(&m_instincts[i].event);
			f->Write(m_instincts[i].action);
			f->Write(&m_instincts[i].params.mouse.button);
			switch (m_instincts[i].event) {
				//case B_MOUSE_UP:
				case B_MOUSE_DOWN:
					f->Write(&m_instincts[i].params.mouse.which.click);
					break;
				case B_MOUSE_MOVED:
					f->Write(&m_instincts[i].params.mouse.which.transit);
					break;
			}
			if (m_instincts[i].params.mouse.require_mods)
				f->Write(&m_instincts[i].params.mouse.require_mods);
			if (m_instincts[i].params.mouse.reject_mods)
				f->Write(&m_instincts[i].params.mouse.reject_mods);
			f->FinishWriteBytes();
		}
	}

#endif
