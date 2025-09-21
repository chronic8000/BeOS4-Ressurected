
#include <stdio.h>

#include "as_support.h"
#include "DecorChoice.h"
#include "DecorVariableString.h"
#include "DecorVariableInteger.h"

struct Decision {
	union {
		int32 i;
		float f;
		char *s;
	} value;
	DecorAtom *branch;
	int8 op;
};

DecorChoiceBase::DecorChoiceBase()
	:	m_variable(NULL), m_default(NULL), m_decisions(NULL),
		m_decisionCount(0)
{
}

DecorChoiceBase::DecorChoiceBase(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def)
{
	m_variable = var;
	m_decisions = decisionList;
	m_decisionCount = count;
	m_default = def;
}

DecorChoiceBase::~DecorChoiceBase()
{
	grFree(m_decisions);
}

DecorStringChoice::DecorStringChoice()
{
}

DecorStringChoice::DecorStringChoice(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def)
	: DecorChoiceBase(var,decisionList,count,def)
{
};

DecorIntChoice::DecorIntChoice()
{
}

DecorIntChoice::DecorIntChoice(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def)
	: DecorChoiceBase(var,decisionList,count,def)
{
};

DecorAtom * DecorStringChoice::ChooseLocal(const VarSpace *space)
{
	int32 which = 0;
	DecorAtom *choice = m_default;
	if (space) {
		const char *value = ((DecorVariableString*)m_variable->Choose(space))->String(space);
		while ((which < m_decisionCount) &&
				strcmp(m_decisions[which].value.s,value)) which++;
		if (which < m_decisionCount) choice = m_decisions[which].branch;
	};
	return choice;
};

DecorAtom * DecorIntChoice::ChooseLocal(const VarSpace *space)
{
	int32 which = 0;
	DecorAtom *choice = m_default;
	if (space) {
		int32 value = ((DecorVariableInteger*)m_variable->Choose(space))->Integer(space);
		while (which < m_decisionCount) {
			bool found = false;
			switch (m_decisions[which].op) {
				case B_EQ:	found = (value == m_decisions[which].value.i);	break;
				case B_NE:	found = (value != m_decisions[which].value.i);	break;
				case B_GT:	found = (value >  m_decisions[which].value.i);	break;
				case B_LT:	found = (value <  m_decisions[which].value.i);	break;
				case B_GE:	found = (value >= m_decisions[which].value.i);	break;
				case B_LE:	found = (value <= m_decisions[which].value.i);	break;
			}
			if (found)
				break;
			which++;
		}
		if (which < m_decisionCount) choice = m_decisions[which].branch;
	}
	return choice;
}

void DecorChoiceBase::PropogateChanges(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags,
	DecorAtom *newChoice)
{
	if (instigator == m_variable) {
		DecorSharedAtom::PropogateChanges(space,this,changes,dfChoice,Choose(space));
	} else if (instigator == ChooseLocal(space)) {
		DecorSharedAtom::PropogateChanges(space,this,changes,depFlags,NULL);
	};
};

void DecorChoiceBase::Unflatten(DecorStream *f)
{
	DecorSharedAtom::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_variable);
	f->Read(&m_default);
	f->Read(&m_decisionCount);
	f->FinishReadBytes();
	
	grFree(m_decisions);	// mathias - 12/9/2000: don't leak decisions!
	m_decisions = (Decision*)grMalloc(m_decisionCount*sizeof(Decision), "DecorChoiceBase::m_decisions",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_decisionCount;i++) {
		f->StartReadBytes();
		f->Read(&m_decisions[i].value.i);
		f->Read(&m_decisions[i].branch);
		f->Read(&m_decisions[i].op);
		f->FinishReadBytes();
	};
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	DecorChoice::DecorChoice()
		: m_varType(0)
	{
	}
	
	DecorChoice::DecorChoice(DecorTable *t) 
	{
		DecorTable *branch;
		m_decisionCount = 0;
		m_variable = t->GetVariable("Variable");
		m_varType = ((DecorVariable*)m_variable->Choose(NULL))->Type();
		if ((m_varType != DECOR_VAR_STRING) &&
			(m_varType != DECOR_VAR_INTEGER)) {
			panic(("Cannot make choices based on this variable type\n"));
		};
	
		m_default = t->GetAtom("Default");
		if (!m_default) panic(("Required field \"Default\" is missing!\n"));
		
		t = t->GetDecorTable("Branches");
		if (!t) panic(("Required field \"Branches\" is missing!\n"));
		while (t->Has(m_decisionCount+1)) m_decisionCount++;
		if (!m_decisionCount) panic(("No fields!\n"));

		m_decisions = (Decision*)grMalloc(m_decisionCount * sizeof(Decision), "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_decisionCount;i++) {
			branch = t->GetDecorTable(i+1);
			if (!branch) panic(("Alien element is part of decision list\n"));
			m_decisions[i].branch = branch->GetAtom("Decision");
			m_decisions[i].op = (int8)(branch->GetNumber("Operation", B_EQ));
	
			if (!branch->Has("Value"))
				panic(("Required field \"Value\" is missing!\n"));
	
			if (m_varType == DECOR_VAR_STRING) {
				m_decisions[i].value.s = strdup(branch->GetString("Value"));
				if (!m_decisions[i].value.s) panic(("bad value!\n"));
			} else if (m_varType == DECOR_VAR_INTEGER) {
				m_decisions[i].value.i = (int32)branch->GetNumber("Value");
			};
		};
		delete t;
	};
	
	DecorChoice::~DecorChoice()
	{
		m_decisionCount = 0;
		m_decisions = NULL;
		m_variable = NULL;
		m_default = NULL;
	};
	
	DecorAtom * DecorChoice::Iterate(int32 *proceed)
	{
		DecorAtom *newMe = NULL;
		if (m_varType == DECOR_VAR_STRING)
			newMe = new DecorStringChoice(m_variable,m_decisions,m_decisionCount,m_default);
		else if (m_varType == DECOR_VAR_INTEGER)
			newMe = new DecorIntChoice(m_variable,m_decisions,m_decisionCount,m_default);
		
		if (!newMe) panic(("Cannot reduce choice to usable form!\n"));

		delete this;
		*proceed = 1;
		return newMe;
	};

	DecorAtom * DecorChoiceBase::Reduce(int32 *proceed)
	{
		bool canReduce = true;
		int32 subProceed;
		subProceed = 1;
		while (subProceed) m_default = m_default->Reduce(&subProceed);
		for (int32 i=0;i<m_decisionCount;i++) {
			subProceed = 1;
			while (subProceed) m_decisions[i].branch = m_decisions[i].branch->Reduce(&subProceed);
			canReduce = canReduce && m_decisions[i].branch->PrettyMuchEquivalentTo(m_default);
		};

		if (canReduce) {
			*proceed = 1;
			return m_default;
		};
		*proceed = 0;
		return this;
	}

	DecorAtom * DecorChoiceBase::CopySubTree(const std::type_info &leafType, AtomGenerator applyToLeaves, void *userData)
	{
		Decision * decisions;
		decisions = (Decision*)grMalloc(m_decisionCount * sizeof(Decision), "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_decisionCount;i++) {
			decisions[i] = m_decisions[i];
			decisions[i].branch = decisions[i].branch->CopySubTree(leafType,applyToLeaves,userData);
		};
		
		return Generate(m_variable,decisions,m_decisionCount,m_default->CopySubTree(leafType,applyToLeaves,userData));
	}
	
	void DecorChoiceBase::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		for (int32 i=0;i<m_decisionCount;i++)
			m_decisions[i].branch->RegisterAtomTree();
		m_default->RegisterAtomTree();
		m_variable->RegisterAtomTree();
	
		for (int32 i=0;i<m_decisionCount;i++)
			m_decisions[i].branch->RegisterDependency(this,dfEverything);
		m_variable->RegisterDependency(this,dfEverything);
		m_default->RegisterDependency(this,dfEverything);
	}
	
	bool DecorChoiceBase::IsTypeOf(const std::type_info &t)
	{
		bool ofType = true;
		for (int32 i=0;i<m_decisionCount;i++)
			ofType = ofType && m_decisions[i].branch->IsTypeOf(t);
		return ofType;
	}

	void DecorChoiceBase::Flatten(DecorStream *f)
	{
		DecorSharedAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_variable);
		f->Write(m_default);
		f->Write(&m_decisionCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_decisionCount;i++) {
			f->StartWriteBytes();
			f->Write(&m_decisions[i].value.i);
			f->Write(m_decisions[i].branch);
			f->Write(&m_decisions[i].op);
			f->FinishWriteBytes();
		}
	}

	DecorChoiceBase * DecorStringChoice::Generate(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def)
	{
		return new DecorStringChoice(var,decisionList,count,def);
	}

	DecorChoiceBase * DecorIntChoice::Generate(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def)
	{
		return new DecorIntChoice(var,decisionList,count,def);
	}

#endif
