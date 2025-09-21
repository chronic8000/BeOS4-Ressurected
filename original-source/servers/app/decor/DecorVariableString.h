
#ifndef DECORVARIABLESTRING_H
#define DECORVARIABLESTRING_H

#include <String.h>

#include "DecorTypes.h"
#include "DecorVariable.h"

/**************************************************************/

class DecorTable;

class DecorVariableString : public DecorVariable
{
				int32			m_localVars;
				BString			m_initialValue;

				DecorVariableString&	operator=(DecorVariableString&);
								DecorVariableString(DecorVariableString&);
	
	public:
								DecorVariableString();
		virtual					~DecorVariableString();

#if defined(DECOR_CLIENT)
								DecorVariableString(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

				int32			Type() { return DECOR_VAR_STRING; };

				const char *	String(const VarSpace *space);

		virtual	void			InitLocal(DecorState *instance);
		
		virtual	void 			SetValue(VarSpace *space, DecorState *changes, DecorValue val);
		virtual	void 			GetValue(const VarSpace *space, DecorValue *val);

		virtual	status_t		Archive(const VarSpace *space, BMessage* target);
		virtual	status_t		Unarchive(VarSpace *space, DecorState *changes, const BMessage& source);
		
		#define THISCLASS		DecorVariableString
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
