
#ifndef DECORFONT_H
#define DECORFONT_H

#include <SupportDefs.h>
#include <String.h>
#include <Font.h>

#include "DecorTypes.h"
#include "DecorVariable.h"
#include "DecorDef.h"

/**************************************************************/

class DecorTable;
class DecorFont : public DecorVariable
{
				DecorFont&		operator=(DecorFont&);
								DecorFont(DecorFont&);
	
	protected:
				void			UnflattenFontPart(DecorStream* f, FontContext* context, BString* css);
#if defined(DECOR_CLIENT)
				void			FlattenFontPart(DecorStream* f, const FontContext* context, const BString* css);
				void			DecodeFont(DecorTable *t, FontContext* context, BString* css);
#endif
	
	public:
#if defined(DECOR_CLIENT)
								DecorFont(DecorTable *table) : DecorVariable(table) {};
		virtual	bool			IsTypeOf(const std::type_info &t);
#endif

								DecorFont() {};
		virtual					~DecorFont() {};
						
		virtual	void			GetFont(const VarSpace *space, FontContext *target) = 0;
};

class DecorStaticFont : public DecorFont
{
				FontContext		m_font;
				BString			m_CSS;
				
				DecorStaticFont&	operator=(DecorStaticFont&);
								DecorStaticFont(DecorStaticFont&);
	
	public:
								DecorStaticFont();
		virtual					~DecorStaticFont();
						
#if defined(DECOR_CLIENT)
								DecorStaticFont(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	bool			IsVariable() const { return false; }
		
		virtual	void			GetFont(const VarSpace *space, FontContext *target);

		#define THISCLASS		DecorStaticFont
		#include				"DecorAtomInclude.h"
};

class DecorVariableFont : public DecorFont
{
			int32				m_localVars;
			FontContext			m_font;
			BString				m_CSS;
			
				DecorVariableFont&	operator=(DecorVariableFont&);
								DecorVariableFont(DecorVariableFont&);
	
	public:
								DecorVariableFont();
		virtual					~DecorVariableFont();

#if defined(DECOR_CLIENT)
								DecorVariableFont(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

				int32			Type() { return DECOR_VAR_FONT; };
		
		virtual	void			GetFont(const VarSpace *space, FontContext *target);

		virtual	void			InitLocal(DecorState *instance);

		virtual	void 			SetValue(VarSpace *space, DecorState *changes, DecorValue val);
		virtual	void 			GetValue(const VarSpace *space, DecorValue *val);

		virtual	status_t		Archive(const VarSpace *space, BMessage* target);
		virtual	status_t		Unarchive(VarSpace *space, DecorState *changes, const BMessage& source);
		
		#define THISCLASS		DecorVariableFont
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
