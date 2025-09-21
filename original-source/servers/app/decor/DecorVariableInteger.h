
#ifndef DECORVARIABLEINT_H
#define DECORVARIABLEINT_H

#include "DecorTypes.h"
#include "DecorVariable.h"

/**************************************************************/

class DecorTable;

class DecorVariableInteger : public DecorVariable
{
				DecorVariableInteger&	operator=(DecorVariableInteger&);
								DecorVariableInteger(DecorVariableInteger&);
	
	public:
								DecorVariableInteger() {};
		virtual					~DecorVariableInteger() {};
		
#if defined(DECOR_CLIENT)
								DecorVariableInteger(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
#endif

				int32			Type() { return DECOR_VAR_INTEGER; };
		virtual	int32			Integer(const VarSpace *space) = 0;
		virtual	void 			GetValue(const VarSpace *space, DecorValue *val);
		virtual	status_t		Archive(const VarSpace *space, BMessage* target);
};

class DecorVariableIntegerSingle : public DecorVariableInteger
{
				int32			m_localVars;
				int32			m_initialValue;

				DecorVariableIntegerSingle&	operator=(DecorVariableIntegerSingle&);
								DecorVariableIntegerSingle(DecorVariableIntegerSingle&);
	
	public:
								DecorVariableIntegerSingle() {}
		virtual					~DecorVariableIntegerSingle();

#if defined(DECOR_CLIENT)
								DecorVariableIntegerSingle(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	int32			Integer(const VarSpace *space);

		virtual	void			InitLocal(DecorState *instance);
		
		virtual	void 			SetValue(VarSpace *space, DecorState *changes, DecorValue val);
		virtual	status_t		Unarchive(VarSpace *space, DecorState *changes, const BMessage& source);

		#define THISCLASS		DecorVariableIntegerSingle
		#include				"DecorAtomInclude.h"
};

class DecorVariableIntegerAdd : public DecorVariableInteger
{
				int32			m_componentCount;
				DecorAtom **	m_components;

				DecorVariableIntegerAdd&	operator=(DecorVariableIntegerAdd&);
								DecorVariableIntegerAdd(DecorVariableIntegerAdd&);
	
	public:
								DecorVariableIntegerAdd();
		virtual					~DecorVariableIntegerAdd();

#if defined(DECOR_CLIENT)
								DecorVariableIntegerAdd(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
		virtual	void			RegisterAtomTree();
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	bool			IsVariable() const { return false; }
		
		virtual	int32			Integer(const VarSpace *space);

		#define THISCLASS		DecorVariableIntegerAdd
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
