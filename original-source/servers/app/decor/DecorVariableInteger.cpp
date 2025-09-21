
#include <stdio.h>

#include "as_support.h"
#include "DecorVariableInteger.h"
#include "DecorState.h"
#include "DecorDef.h"

struct DecorVariableIntegerVars {
	int32 value;
};

DecorVariableIntegerSingle::~DecorVariableIntegerSingle()
{
}

int32 
DecorVariableIntegerSingle::Integer(const VarSpace *space)
{
	DecorVariableIntegerVars *vars = (DecorVariableIntegerVars*)space->Lookup(m_localVars);
	return vars->value;
}

void 
DecorVariableIntegerSingle::InitLocal(DecorState *instance)
{
	DecorVariableIntegerVars *vars = (DecorVariableIntegerVars*)instance->Space()->Lookup(m_localVars);
	vars->value = m_initialValue;
}

void 
DecorVariableIntegerSingle::SetValue(VarSpace *space, DecorState *changes, DecorValue val)
{
	DecorVariableIntegerVars *vars = (DecorVariableIntegerVars*)space->Lookup(m_localVars);
	if (vars->value != val.i) {
		vars->value = val.i;
		AnalyzeChanges(space,changes);
	};
}

status_t
DecorVariableIntegerSingle::Unarchive(VarSpace *space, DecorState *changes, const BMessage& source)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	DecorValue val;
	status_t result = source.FindInt32(Name(), &val.i);
	if (result >= B_OK)
		SetValue(space, changes, val);
	return result;
}

void DecorVariableInteger::GetValue(const VarSpace *space, DecorValue *val)
{
	val->i = Integer(space);
}

status_t DecorVariableInteger::Archive(const VarSpace *space, BMessage* target)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	return target->AddInt32(Name(), Integer(space));
}

void DecorVariableIntegerSingle::Unflatten(DecorStream *f)
{
	DecorVariableInteger::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_localVars);
	f->ReadOrSet(&m_initialValue, 0);
	f->FinishReadBytes();
	//printf("Starting single int variable %s with %ld\n", Name(), m_initialValue);
}

DecorVariableIntegerAdd::DecorVariableIntegerAdd()
	:	m_components(NULL)
{
}

void DecorVariableIntegerAdd::Unflatten(DecorStream *f)
{
	DecorVariableInteger::Unflatten(f);
	grFree(m_components);
	
	f->StartReadBytes();
	f->Read(&m_componentCount);
	f->FinishReadBytes();
		
	m_components = (DecorAtom**)grMalloc(m_componentCount*sizeof(DecorAtom*), "decor",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_componentCount;i++) {
		f->StartReadBytes();
		if (f->Read(&m_components[i]) < B_OK)
			m_components[i] = NULL;
		f->FinishReadBytes();
	}
}

DecorVariableIntegerAdd::~DecorVariableIntegerAdd()
{
	grFree(m_components);
}

int32 DecorVariableIntegerAdd::Integer(const VarSpace *space)
{
	int32 total = 0;
	for (int32 i=0;i<m_componentCount;i++)
		total += ((DecorVariableInteger*)m_components[i]->Choose(space))->Integer(space);
	return total;
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	DecorVariableInteger::DecorVariableInteger(DecorTable *t) : DecorVariable(t)
	{
	}

	bool DecorVariableInteger::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableInteger)) || DecorVariable::IsTypeOf(t);
	}

	DecorVariableIntegerSingle::DecorVariableIntegerSingle(DecorTable *t) : DecorVariableInteger(t)
	{
		m_initialValue = (int32)t->GetNumber("Value");
		//printf("Initializing single int variable %s with %ld\n", Name(), m_initialValue);
	}
	
	bool DecorVariableIntegerSingle::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableIntegerSingle)) || DecorVariableInteger::IsTypeOf(t);
	}
	
	void DecorVariableIntegerSingle::AllocateLocal(DecorDef *decor)
	{
		DecorVariableInteger::AllocateLocal(decor);
		m_localVars = decor->AllocateVarSpace(sizeof(DecorVariableIntegerVars));
	}

	void DecorVariableIntegerSingle::Flatten(DecorStream *f)
	{
		DecorVariableInteger::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_localVars);
		f->Write(&m_initialValue);
		f->FinishWriteBytes();
	}

	DecorVariableIntegerAdd::DecorVariableIntegerAdd(DecorTable *t) : DecorVariableInteger(t)
	{
		t = t->GetDecorTable("Components",ERROR);
		m_componentCount = t->Count();
		m_components = (DecorAtom**)grMalloc(m_componentCount*sizeof(DecorAtom*), "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_componentCount;i++)
			m_components[i] = t->GetAtom(i+1,ERROR);
		delete t;
	}
	
	void DecorVariableIntegerAdd::RegisterAtomTree()
	{
		DecorVariableInteger::RegisterAtomTree();
		for (int32 i=0;i<m_componentCount;i++) {
			m_components[i]->RegisterAtomTree();
			m_components[i]->RegisterDependency(this,dfEverything);
		};
	}

	bool DecorVariableIntegerAdd::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableIntegerAdd)) || DecorVariableInteger::IsTypeOf(t);
	}
	
	void DecorVariableIntegerAdd::Flatten(DecorStream *f)
	{
		DecorVariableInteger::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_componentCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_componentCount;i++) {
			f->StartWriteBytes();
			f->Write(m_components[i]);
			f->FinishWriteBytes();
		}
	}
	
#endif
