
#ifndef DECORCOLOR_H
#define DECORCOLOR_H

#include <SupportDefs.h>
#include <String.h>
#include <Font.h>

#include "DecorTypes.h"
#include "DecorVariable.h"
#include "DecorDef.h"

/**************************************************************/

class DecorTable;
class DecorColor : public DecorVariable
{
				DecorColor&		operator=(DecorColor&);
								DecorColor(DecorColor&);
	
	protected:
				void			UnflattenColorPart(DecorStream* f, rgb_color* color, BString* name);
				rgb_color		LookupColor(const char* name);
#if defined(DECOR_CLIENT)
				void			FlattenColorPart(DecorStream* f, const rgb_color* color, const BString* name);
				void			DecodeColor(DecorTable *t, rgb_color* color, BString* name);
#endif
	
	public:
#if defined(DECOR_CLIENT)
								DecorColor(DecorTable *table) : DecorVariable(table) {};
		virtual	bool			IsTypeOf(const std::type_info &t);
#endif

								DecorColor() {};
		virtual					~DecorColor() {};
						
		virtual rgb_color		Color(const VarSpace *space) = 0;
};

class DecorStaticColor : public DecorColor
{
				rgb_color		m_color;
				BString			m_name;
				
				DecorStaticColor&	operator=(DecorStaticColor&);
								DecorStaticColor(DecorStaticColor&);
	
	public:
								DecorStaticColor();
		virtual					~DecorStaticColor();
						
#if defined(DECOR_CLIENT)
								DecorStaticColor(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	bool			IsVariable() const { return false; }
		
		virtual rgb_color		Color(const VarSpace *space);

		#define THISCLASS		DecorStaticColor
		#include				"DecorAtomInclude.h"
};

class DecorVariableColor : public DecorColor
{
			int32				m_localVars;
			rgb_color			m_color;
			BString				m_name;
			
				DecorVariableColor&	operator=(DecorVariableColor&);
								DecorVariableColor(DecorVariableColor&);
	
	public:
								DecorVariableColor();
		virtual					~DecorVariableColor();

#if defined(DECOR_CLIENT)
								DecorVariableColor(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

				int32			Type() { return DECOR_VAR_COLOR; };
						
		virtual rgb_color		Color(const VarSpace *space);

		virtual	void			InitLocal(DecorState *instance);

		virtual	void 			SetValue(VarSpace *space, DecorState *changes, DecorValue val);
		virtual	void 			GetValue(const VarSpace *space, DecorValue *val);

		virtual	status_t		Archive(const VarSpace *space, BMessage* target);
		virtual	status_t		Unarchive(VarSpace *space, DecorState *changes, const BMessage& source);
		
		#define THISCLASS		DecorVariableColor
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
