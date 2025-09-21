
#include "as_support.h"
#include "DecorActionDef.h"
#include "DecorEvent.h"
#include "DecorVariable.h"
#include "DecorState.h"

#include <stdio.h>
#include <new>

DecorActionDef::DecorActionDef()
	:	m_actions(NULL), m_actionCount(0)
{
}

DecorActionDef::~DecorActionDef()
{
	delete[] m_actions;
}

void DecorActionDef::Perform(DecorState *changes, DecorExecutor *executor)
{
	for (int32 i=0;i<m_actionCount;i++)
		executor->Action(changes, m_actions+i);
}

void DecorActionDef::Unflatten(DecorStream *f)
{
	DecorAtom::Unflatten(f);
	
	delete[] m_actions;
	
	f->StartReadBytes();
	f->Read(&m_actionCount);
	f->FinishReadBytes();
	
	m_actions = new(std::nothrow) BMessage[m_actionCount];
	for (int32 i=0;i<m_actionCount;i++) {
		f->StartReadBytes();
		f->Read(&m_actions[i]);
		f->FinishReadBytes();
	}
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	DecorActionDef::DecorActionDef(DecorTable *t)
	{
		DecorTable *action;
		t = t->GetDecorTable("Actions",ERROR);
		m_actionCount = t->Count();
		m_actions = new(std::nothrow) BMessage[m_actionCount];
		if (!m_actionCount) panic(("ActionDef has no actions!\n"));
		for (int32 i=0;i<m_actionCount;i++) {
			action = t->GetDecorTable(i+1,ERROR);
			action->ConvertToMessage(&m_actions[i]);
			m_actions[i].RemoveName("Verb");
			m_actions[i].what = (int32)action->GetNumber("Verb");
			if (DecorPrintMode()&DECOR_PRINT_TREE) {
				printf("\nAction table: "); m_actions[i].PrintToStream();
			}
			delete action;
		};
		delete t;
	}

	bool DecorActionDef::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorActionDef)) || DecorAtom::IsTypeOf(t);
	}

	void DecorActionDef::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		for (int32 i=0;i<m_actionCount;i++) {
			const BMessage& a(m_actions[i]);
			const char* name;
			type_code type;
			int32 count;
			for (int32 j=0;
					a.GetInfo(DECOR_ATOM_TYPE, j, &name, &type, &count) == B_OK;
					j++) {
				for (int32 k=0; k<count; k++) {
					DecorAtom* atom;
					if (find_decor_atom(a, name, k, &atom) >= B_OK && atom != NULL) {
						atom->RegisterAtomTree();
						atom->RegisterDependency(this,dfEverything);
					}
				}
			}
		}
	}

	void DecorActionDef::Flatten(DecorStream *f)
	{
		DecorAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_actionCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_actionCount;i++) {
			f->StartWriteBytes();
			f->Write(&m_actions[i]);
			f->FinishWriteBytes();
		}
	}
	
#endif
