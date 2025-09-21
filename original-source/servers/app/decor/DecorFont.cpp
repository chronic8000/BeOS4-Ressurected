
#include <stdio.h>
#include "DecorFont.h"
#include "DecorState.h"
#include "DecorDef.h"

#include "shared_fonts.h"
#include "interface_misc.h"

struct DecorVariableFontVars {
	fc_context_packet font;
};

DecorStaticFont::DecorStaticFont()
{
}

DecorStaticFont::~DecorStaticFont()
{
}

void DecorStaticFont::GetFont(const VarSpace *space, FontContext* font)
{
	*font = m_font;
}

DecorVariableFont::DecorVariableFont()
	:	m_localVars(0)
{
}

DecorVariableFont::~DecorVariableFont()
{
}

void DecorVariableFont::GetFont(const VarSpace *space, FontContext *font)
{
	DecorVariableFontVars *vars = (DecorVariableFontVars*)space->Lookup(m_localVars);
#if defined(DECOR_CLIENT)
	IKAccess::ReadFontPacket(font, &vars->font);
#else
	font->SetTo(&vars->font, sizeof(vars->font));
#endif
}

void DecorVariableFont::InitLocal(DecorState *instance)
{
	DecorVariableFontVars *vars = (DecorVariableFontVars*)instance->Space()->Lookup(m_localVars);
#if defined(DECOR_CLIENT)
	IKAccess::WriteFontBasicPacket(&m_font, &vars->font);
#else
	m_font.GetBasicPacket(&vars->font);
#endif
}

void DecorVariableFont::SetValue(VarSpace *space, DecorState *changes, DecorValue val)
{
	fc_context_packet newPacket;
#if defined(DECOR_CLIENT)
	IKAccess::WriteFontBasicPacket(val.font, &newPacket);
#else
	m_font.GetBasicPacket(&newPacket);
#endif
	DecorVariableFontVars *vars = (DecorVariableFontVars*)space->Lookup(m_localVars);
	if (memcmp(&newPacket, &vars->font, sizeof(newPacket)) != 0) {
		vars->font = newPacket;
		AnalyzeChanges(space,changes);
	};
}

void DecorVariableFont::GetValue(const VarSpace *space, DecorValue *val)
{
	GetFont(space, val->font);
}

status_t DecorVariableFont::Archive(const VarSpace *space, BMessage* target)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	FontContext font;
	GetFont(space, &font);
	return target->AddFlat(Name(), &font);
}

status_t DecorVariableFont::Unarchive(VarSpace *space, DecorState *changes, const BMessage& source)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	FontContext font;
	status_t result = source.FindFlat(Name(), &font);
	if (result >= B_OK) {
		DecorValue val;
		val.font = &font;
		SetValue(space, changes, val);
	}
	return result;
}

void DecorFont::UnflattenFontPart(DecorStream *f, FontContext* context, BString* css)
{
	f->StartReadBytes();
	
	int32 type;
	f->Read(&type);
	
	context->SetTo(B_PLAIN_FONT);
	if (type == 0) {
		f->Read(context);
		*css = "";
	} else if (type == 1) {
		f->Read(css);
		context->ApplyCSS(css->String());
	}
	
	f->FinishReadBytes();
}

void DecorVariableFont::Unflatten(DecorStream *f)
{
	DecorVariable::Unflatten(f);
	
	// Think of this as inheritance.
	UnflattenFontPart(f, &m_font, &m_CSS);
	
	f->StartReadBytes();
	f->Read(&m_localVars);
	f->FinishReadBytes();
}

void DecorStaticFont::Unflatten(DecorStream *f)
{
	DecorVariable::Unflatten(f);

	// Think of this as inheritance.
	UnflattenFontPart(f, &m_font, &m_CSS);
	
	f->StartReadBytes();
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	void DecorFont::DecodeFont(DecorTable *t, FontContext* context, BString* css)
	{
		if (!t) {
			context->SetTo(B_PLAIN_FONT);
			*css = "";
			return;
		}
		
		*css = t->GetString("Value");
		if (css->Length() > 0) {
			context->SetTo(B_PLAIN_FONT);
			context->ApplyCSS(css->String());
		} else {
			fprintf(stderr, "Font not defined for variable %s!\n", Name());
			
			// The CSS stuff really makes this form useless.
			const char *family = t->GetString("Family");
			if (!family) family = "Swis721 BT";
		
			const char *style = t->GetString("Style");
			if (!style) style = "Regular";
		
			float size = t->GetNumber("Size");
			if (size == 0) size = 12;
		
			context->SetFamilyAndStyle(family,style);
			context->SetSize(size);
		}
	}

	DecorVariableFont::DecorVariableFont(DecorTable *t) : DecorFont(t)
	{
		DecodeFont(t, &m_font, &m_CSS);
		gCurrentDef->AddVariable(this);
	}
	
	bool DecorVariableFont::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableFont)) || DecorFont::IsTypeOf(t);
	}
	
	bool DecorFont::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorFont)) || DecorVariable::IsTypeOf(t);
	}
	
	DecorStaticFont::DecorStaticFont(DecorTable *t) : DecorFont(t)
	{
		DecodeFont(t, &m_font, &m_CSS);
	}
	
	bool DecorStaticFont::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorStaticFont)) || DecorFont::IsTypeOf(t);
	}
	
	void DecorVariableFont::AllocateLocal(DecorDef *decor)
	{
		DecorFont::AllocateLocal(decor);
		m_localVars = decor->AllocateVarSpace(sizeof(DecorVariableFontVars));
	}

	void DecorFont::FlattenFontPart(DecorStream *f, const FontContext* context, const BString* css)
	{
		f->StartWriteBytes();
		if (css->Length() == 0) {
			int32 type = 0;
			f->Write(&type);
			f->Write(context);
		} else {
			int32 type = 1;
			f->Write(&type);
			f->Write(css);
		}
		f->FinishWriteBytes();
	}

	void DecorVariableFont::Flatten(DecorStream *f)
	{
		DecorVariable::Flatten(f);
		
		// Think of this as inheritance.
		FlattenFontPart(f, &m_font, &m_CSS);
		
		f->StartWriteBytes();
		f->Write(&m_localVars);
		f->FinishWriteBytes();
	}

	void DecorStaticFont::Flatten(DecorStream *f)
	{
		DecorVariable::Flatten(f);
		
		// Think of this as inheritance.
		FlattenFontPart(f, &m_font, &m_CSS);
		
		f->StartWriteBytes();
		f->FinishWriteBytes();
	}
	
#endif
