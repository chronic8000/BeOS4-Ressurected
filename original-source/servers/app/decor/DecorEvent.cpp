
#include "DecorState.h"
#include "DecorActionDef.h"
#include "DecorEvent.h"

void DecorEvent::Unflatten(DecorStream *f)
{
	DecorAtom::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_actionDef);
	f->Read(&m_flags);
	f->FinishReadBytes();
}

uint32 DecorEvent::Trigger(DecorState *state, DecorExecutor *executor)
{
	((DecorActionDef*)m_actionDef->Choose(state->Space()))->Perform(state,executor);
	return m_flags;
}

DecorEvent::DecorEvent()
	:	m_actionDef(NULL), m_flags(0)
{
}

DecorEvent::~DecorEvent()
{
}

#if defined(DECOR_CLIENT)

	DecorEvent::DecorEvent(DecorTable *t)
	{
		m_actionDef = t->GetAtom("ActionDef",ERROR);
		if (!m_actionDef->IsTypeOf(typeid(DecorActionDef)))
			panic(("ActionDef needs to be of type ACTIONDEF (duh)\n"));
	}
	
	bool DecorEvent::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorEvent)) || DecorAtom::IsTypeOf(t);
	}
	
	void DecorEvent::Flatten(DecorStream *f)
	{
		DecorAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_actionDef);
		f->Write(&m_flags);
		f->FinishWriteBytes();
	}
	
	void DecorEvent::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		m_actionDef->RegisterAtomTree();
		m_actionDef->RegisterDependency(this,dfEverything);
	}

#endif
