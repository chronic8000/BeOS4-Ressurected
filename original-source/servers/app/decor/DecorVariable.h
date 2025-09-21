
#ifndef DECORVARIABLE_H
#define DECORVARIABLE_H

#include <String.h>

#include "DecorTypes.h"
#include "DecorSharedAtom.h"

/**************************************************************/

class DecorTable;

class DecorVariable : public DecorSharedAtom
{
				BString			m_name;
				int32			m_visibility;
				
				DecorVariable&	operator=(DecorVariable&);
								DecorVariable(DecorVariable&);
	
	public:
								DecorVariable();
		virtual					~DecorVariable();

#if defined(DECOR_CLIENT)
								DecorVariable(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
		virtual	DecorAtom *		Iterate(int32 *proceed);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	bool			IsVariable() const { return true; }
		
				const char *	Name() const;
		virtual	int32			Type() { return DECOR_VAR_NIL; };
				int32			Visibility() const;
				
		inline	void			AnalyzeChanges(const VarSpace *space, DecorState *changes);
		virtual	void 			SetValue(VarSpace *space, DecorState *changes, DecorValue val) {};
		virtual	void 			GetValue(const VarSpace *space, DecorValue *val) {};

		virtual	status_t		Archive(const VarSpace *space, BMessage* target) { return B_UNSUPPORTED; }
		virtual	status_t		Unarchive(VarSpace *space, DecorState *changes, const BMessage& source) { return B_UNSUPPORTED; }
		
		#define THISCLASS		DecorVariable
		#include				"DecorAtomInclude.h"
};

void DecorVariable::AnalyzeChanges(const VarSpace *space, DecorState *changes)
{
	PropogateChanges(space,NULL,changes,dfEverything);
};

/**************************************************************/

#endif
