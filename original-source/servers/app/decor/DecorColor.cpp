
#include "DecorColor.h"

#include <stdio.h>
#include "DecorState.h"
#include "DecorDef.h"

#if !defined(DECOR_CLIENT)
#include "server.h"
#endif

static const rgb_color white = { 255, 255, 255, 255 };

struct DecorVariableColorVars {
	rgb_color color;
};

DecorStaticColor::DecorStaticColor()
{
}

DecorStaticColor::~DecorStaticColor()
{
}

rgb_color DecorStaticColor::Color(const VarSpace *space)
{
	return m_color;
}

DecorVariableColor::DecorVariableColor()
	:	m_localVars(0)
{
}

DecorVariableColor::~DecorVariableColor()
{
}

rgb_color DecorVariableColor::Color(const VarSpace *space)
{
	DecorVariableColorVars *vars = (DecorVariableColorVars*)space->Lookup(m_localVars);
	return vars->color;
}

void DecorVariableColor::InitLocal(DecorState *instance)
{
	DecorVariableColorVars *vars = (DecorVariableColorVars*)instance->Space()->Lookup(m_localVars);
	vars->color = m_color;
}

void DecorVariableColor::SetValue(VarSpace *space, DecorState *changes, DecorValue val)
{
	DecorVariableColorVars *vars = (DecorVariableColorVars*)space->Lookup(m_localVars);
	if (vars->color != val.color) {
		vars->color = val.color;
		AnalyzeChanges(space,changes);
	}
}

void DecorVariableColor::GetValue(const VarSpace *space, DecorValue *val)
{
	val->color = Color(space);
}

status_t DecorVariableColor::Archive(const VarSpace *space, BMessage* target)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	return target->AddRGBColor(Name(), Color(space));
}

status_t DecorVariableColor::Unarchive(VarSpace *space, DecorState *changes, const BMessage& source)
{
	if (!Name() || !*Name()) return B_UNSUPPORTED;
	DecorValue val;
	status_t result = source.FindRGBColor(Name(), &val.color);
	if (result >= B_OK)
		SetValue(space, changes, val);
	return result;
}

rgb_color DecorColor::LookupColor(const char* name)
{
	rgb_color color = white;
	
#if defined(DECOR_CLIENT)
	BMessage data;
	if (get_ui_settings(&data) == B_OK) {
		if (data.FindRGBColor(name, &color) != B_OK) color = white;
	}
#else
	bool found = false;
	BMessage* data = LockUISettings();
	if (data->FindRGBColor(name, &color) == B_OK) found = true;
	UnlockUISettings();
	if (!found) {
		BMessage def;
		get_default_settings(&def);
		if (def.FindRGBColor(name, &color) != B_OK) color = white;
	}
#endif
	
	return color;
}

void DecorColor::UnflattenColorPart(DecorStream *f, rgb_color* color, BString* name)
{
	f->StartReadBytes();
	
	int32 type;
	f->Read(&type);
	
	*color = white;
	if (type == 0) {
		f->Read(color);
		*name = "";
	} else if (type == 1) {
		f->Read(name);
		*color = LookupColor(name->String());
	}
	
	f->FinishReadBytes();
}

void DecorVariableColor::Unflatten(DecorStream *f)
{
	DecorVariable::Unflatten(f);
	
	// Think of this as inheritance.
	UnflattenColorPart(f, &m_color, &m_name);
		
	f->StartReadBytes();
	f->Read(&m_localVars);
	f->FinishReadBytes();
}

void DecorStaticColor::Unflatten(DecorStream *f)
{
	DecorVariable::Unflatten(f);

	// Think of this as inheritance.
	UnflattenColorPart(f, &m_color, &m_name);
	
	f->StartReadBytes();
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	void DecorColor::DecodeColor(DecorTable *t, rgb_color* color, BString* name)
	{
		if (!t) {
			*color = white;
			*name = "";
			return;
		}
		
		DecorTable* c = t->GetDecorTable("Value", 0);
		if (c) {
			if (c->Has(1) && c->Has(2) && c->Has(3)) {
				color->red = (int32)(c->GetNumber(1)*255);
				color->green = (int32)(c->GetNumber(2)*255);
				color->blue = (int32)(c->GetNumber(3)*255);
				if (c->Has(4)) color->alpha = (int32)(c->GetNumber(4)*255);
				else color->alpha = 255;
			} else {
				*color = white;
			}
			delete c;
		} else {
			*name = t->GetString("Value");
			*color = white;
			if (name->Length() <= 0) {
				fprintf(stderr, "Color not defined for variable %s!\n", Name());
			}
		}
	}

	DecorVariableColor::DecorVariableColor(DecorTable *t) : DecorColor(t)
	{
		DecodeColor(t, &m_color, &m_name);
		gCurrentDef->AddVariable(this);
	}
	
	bool DecorVariableColor::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorVariableColor)) || DecorColor::IsTypeOf(t);
	}
	
	bool DecorColor::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorColor)) || DecorVariable::IsTypeOf(t);
	}
	
	DecorStaticColor::DecorStaticColor(DecorTable *t) : DecorColor(t)
	{
		DecodeColor(t, &m_color, &m_name);
	}
	
	bool DecorStaticColor::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorStaticColor)) || DecorColor::IsTypeOf(t);
	}
	
	void DecorVariableColor::AllocateLocal(DecorDef *decor)
	{
		DecorColor::AllocateLocal(decor);
		m_localVars = decor->AllocateVarSpace(sizeof(DecorVariableColorVars));
	}

	void DecorColor::FlattenColorPart(DecorStream *f, const rgb_color* color, const BString* name)
	{
		f->StartWriteBytes();
		if (name->Length() == 0) {
			int32 type = 0;
			f->Write(&type);
			f->Write(color);
		} else {
			int32 type = 1;
			f->Write(&type);
			f->Write(name);
		}
		f->FinishWriteBytes();
	}

	void DecorVariableColor::Flatten(DecorStream *f)
	{
		DecorVariable::Flatten(f);
		
		// Think of this as inheritance.
		FlattenColorPart(f, &m_color, &m_name);
		
		f->StartWriteBytes();
		f->Write(&m_localVars);
		f->FinishWriteBytes();
	}

	void DecorStaticColor::Flatten(DecorStream *f)
	{
		DecorVariable::Flatten(f);
		
		// Think of this as inheritance.
		FlattenColorPart(f, &m_color, &m_name);
		
		f->StartWriteBytes();
		f->FinishWriteBytes();
	}
	
#endif
