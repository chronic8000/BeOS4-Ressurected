
#include <stdio.h>

#include "as_support.h"
#include "DecorVariable.h"
#include "DecorDef.h"

DecorVariable::DecorVariable()
	:	m_visibility(PRIVATE_VISIBILITY)
{
}

DecorVariable::~DecorVariable()
{
}

const char * DecorVariable::Name() const
{
	if (m_name.Length() > 0) return m_name.String();
	return "<anonymous>";
}

int32 DecorVariable::Visibility() const
{
	return m_visibility;
}

void DecorVariable::Unflatten(DecorStream *f)
{
	DecorSharedAtom::Unflatten(f);
	
	f->StartReadBytes();
	f->ReadOrSet(&m_name, "");
	f->ReadOrSet(&m_visibility, PRIVATE_VISIBILITY);
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)
	
	extern DecorDef *gCurrentDef;

	#include "LuaDecor.h"
	DecorVariable::DecorVariable(DecorTable *t)
	{
		m_name = t->GetString("Name");
		const char* v = t->GetString("Visibility");
		if (v && strcmp(v, "global") == 0)
			m_visibility = GLOBAL_VISIBILITY;
		else if (v && strcmp(v, "public") == 0)
			m_visibility = PUBLIC_VISIBILITY;
		else if (v && strcmp(v, "protected") == 0)
			m_visibility = PROTECTED_VISIBILITY;
		else
			m_visibility = PRIVATE_VISIBILITY;
	}
	
	bool DecorVariable::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariable)) || DecorSharedAtom::IsTypeOf(t);
	}

	DecorAtom * DecorVariable::Iterate(int32 *proceed)
	{
		if (m_name.Length() > 0) {
			DecorVariable *v = gCurrentDef->GetVariable(m_name.String());
			if (v && (v != this) && (typeid(*v) == typeid(*this))) {
				*proceed = 1;
				delete this;
				return v;
			};
		};		
		*proceed = 0;
		return this;
	}

	void DecorVariable::Flatten(DecorStream *f)
	{
		DecorSharedAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_name);
		f->Write(&m_visibility);
		f->FinishWriteBytes();
	}
	
#endif
