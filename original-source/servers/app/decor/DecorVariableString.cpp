
#include <stdio.h>

#include "as_support.h"
#include "DecorVariableString.h"
#include "DecorState.h"
#include "DecorDef.h"

struct DecorVariableStringVars {
	char value[128];
};

DecorVariableString::DecorVariableString()
{
}

DecorVariableString::~DecorVariableString()
{
}

const char * DecorVariableString::String(const VarSpace *space)
{
	DecorVariableStringVars *vars = (DecorVariableStringVars*)space->Lookup(m_localVars);
	return vars->value;
}

void DecorVariableString::InitLocal(DecorState *instance)
{
	DecorVariableStringVars *vars = (DecorVariableStringVars*)instance->Space()->Lookup(m_localVars);
	strncpy(vars->value,m_initialValue.String(),127);
	vars->value[127] = 0;
}

void DecorVariableString::SetValue(VarSpace *space, DecorState *changes, DecorValue val)
{
	DecorVariableStringVars *vars = (DecorVariableStringVars*)space->Lookup(m_localVars);
	if (strcmp(vars->value,val.s)) {
		strncpy(vars->value,val.s,127);
		vars->value[127] = 0;
		AnalyzeChanges(space,changes);
	};
}

void DecorVariableString::GetValue(const VarSpace *space, DecorValue *val)
{
	DecorVariableStringVars *vars = (DecorVariableStringVars*)space->Lookup(m_localVars);
	strcpy(val->s,vars->value);
}

status_t DecorVariableString::Archive(const VarSpace *space, BMessage* target)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	return target->AddString(Name(), String(space));
}

status_t DecorVariableString::Unarchive(VarSpace *space, DecorState *changes, const BMessage& source)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	DecorValue val;
	status_t result = source.FindString(Name(), (const char**)&val.s);
	if (result >= B_OK)
		SetValue(space, changes, val);
	return result;
}

void DecorVariableString::Unflatten(DecorStream *f)
{
	DecorVariable::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_localVars);
	f->ReadOrSet(&m_initialValue, "");
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	DecorVariableString::DecorVariableString(DecorTable *t) : DecorVariable(t)
	{
		m_initialValue = t->GetString("Value");
	}
	
	bool DecorVariableString::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableString)) || DecorVariable::IsTypeOf(t);
	}

	void DecorVariableString::AllocateLocal(DecorDef *decor)
	{
		DecorVariable::AllocateLocal(decor);
		m_localVars = decor->AllocateVarSpace(sizeof(DecorVariableStringVars));
	}

	void DecorVariableString::Flatten(DecorStream *f)
	{
		DecorVariable::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_localVars);
		f->Write(&m_initialValue);
		f->FinishWriteBytes();
	}

#endif
