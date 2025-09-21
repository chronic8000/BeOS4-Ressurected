
#include <stdio.h>
#include <string.h>

#include "rect.h"
#include "DecorTextWidget.h"
#include "DecorColor.h"
#include "DecorFont.h"
#include "DecorVariableString.h"
#include "DecorState.h"

#include <Font.h>
#include <View.h>

struct DecorTextWidgetVars {
	int32	textWidth;
	int32	textHeight;
};

DecorTextWidget::DecorTextWidget()
	:	m_font(NULL), m_text(NULL), m_color(NULL)
{
}

DecorTextWidget::~DecorTextWidget()
{
}

uint32 DecorTextWidget::Layout(
	CollectRegionStruct *collect,
	rect newBounds, rect oldBounds)
{
	return LAYOUT_NEED_REDRAW;
}

uint32 DecorTextWidget::CacheLimits(DecorState *instance)
{
	font_height h;
	int32 width,height;
	FontContext f;
	((DecorFont*)m_font->Choose(instance->Space()))->GetFont(instance->Space(), &f);
	
	f.GetHeight(&h);
	width = (int32)(f.StringWidth(m_text->String(instance->Space())) + 0.5)+1;

	height = (int32)(h.ascent + h.descent + 0.5);
	width++;

	DecorTextWidgetVars *vars = (DecorTextWidgetVars*)instance->LimitCache()->Lookup(m_localVars);
	if ((vars->textWidth != width) ||
		(vars->textHeight != height)) {
		vars = (DecorTextWidgetVars*)instance->WritableLimitCache()->Lookup(m_localVars);
		vars->textWidth = width;
		vars->textHeight = height;
		return LIMITS_NEED_LAYOUT;
	}
	
	return 0;
}

void DecorTextWidget::FetchLimits(
	DecorState *instance,
	LimitsStruct *lim)
{
	DecorTextWidgetVars *vars = (DecorTextWidgetVars*)instance->LimitCache()->Lookup(m_localVars);
	lim->widths[MAX_LIMIT] = lim->widths[PREF_LIMIT] = vars->textWidth;
	lim->heights[MIN_LIMIT] = lim->heights[PREF_LIMIT] = vars->textHeight;
	lim->widths[MIN_LIMIT] = 0;
	lim->heights[MAX_LIMIT] = INFINITE_LIMIT;
};

void DecorTextWidget::Draw(
	DecorState *instance,
	rect r, DrawingContext context,
	region *update)
{
	font_height h;
	char toDraw[256];
	FontContext f;
	((DecorFont*)m_font->Choose(instance->Space()))->GetFont(instance->Space(), &f);
	rgb_color c = ((DecorColor*)m_color->Choose(instance->Space()))->Color(instance->Space());
	const char *src = m_text->String(instance->Space());

	if (update && !overlap_rect(update->Bounds(),r)) return;
	
	f.GetHeight(&h);
	int32 height = r.bottom-r.top+1;
	float diff = height-h.ascent;
	diff /= 2;
	if (h.descent > diff) diff = h.descent;
	
	#if defined(DECOR_CLIENT)
		char *ptr = toDraw;
		context->SetFont(&f);
		context->SetHighColor(c);
		if (c.alpha == 255)
			context->SetDrawingMode(B_OP_OVER);
		else
			context->SetDrawingMode(B_OP_ALPHA);
		f.GetTruncatedStrings(&src,1,B_TRUNCATE_END,r.right-r.left,&ptr);
		context->DrawString(toDraw,BPoint(r.left+m_offset.h+1,r.bottom+m_offset.v-diff+0.5));
	#else
		fpoint pen;
		pen.h = r.left+m_offset.h+1;
		pen.v = r.bottom+m_offset.v-diff+0.5;
		grSetFont(context,f);
		grSetForeColor(context,c);
		if (c.alpha == 255)
			grSetDrawOp(context,B_OP_OVER);
		else
			grSetDrawOp(context,B_OP_ALPHA);
		grSetPenLocation(context,pen);
		grTruncateString(context, r.right-r.left, B_TRUNCATE_END, (uchar*)src, (uchar*)toDraw);
		grDrawString(context,(uchar*)toDraw);
	#endif
}

void DecorTextWidget::Unflatten(DecorStream *f)
{
	DecorPart::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_font);
	f->Read((DecorAtom**)&m_text);
	f->Read(&m_color);
	f->Read(&m_offset);
	f->Read(&m_localVars);
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	DecorTextWidget::DecorTextWidget(DecorTable *t) : DecorPart(t)
	{
		m_font = t->GetFont("FontBinding", 1);
		DecorVariable *v = (DecorVariable*)t->GetVariable("ValueBinding");
		if (!v) {
			panic(("Value binding is NIL!\n"));
		} else {
			m_text = dynamic_cast<DecorVariableString*>(v);
			if (!m_text) {
				panic(("Value binding is of the wrong type!\n"));
				delete v;
			};
		};
		m_color = t->GetColor("ColorBinding", 1);
		m_offset.h = m_offset.v = 0;
		t = t->GetDecorTable("Offset");
		if (t) {
			m_offset.h = (int32)t->GetNumber(1);
			m_offset.v = (int32)t->GetNumber(2);
			delete t;
		};
	}

	void DecorTextWidget::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		m_font->RegisterAtomTree();
		m_text->RegisterAtomTree();
		m_color->RegisterAtomTree();
		m_font->RegisterDependency(this,dfLayout|dfRedraw);
		m_text->RegisterDependency(this,dfLayout|dfRedraw);
		m_color->RegisterDependency(this,dfRedraw);
	}
	
	void DecorTextWidget::AllocateLocal(DecorDef *decor)
	{
		DecorPart::AllocateLocal(decor);
		m_localVars = decor->AllocateLimitCache(sizeof(DecorTextWidgetVars));
	}

	bool DecorTextWidget::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorTextWidget)) || DecorPart::IsTypeOf(t);
	}

	void DecorTextWidget::Flatten(DecorStream *f)
	{
		DecorPart::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_font);
		f->Write(m_text);
		f->Write(m_color);
		f->Write(&m_offset);
		f->Write(&m_localVars);
		f->FinishWriteBytes();
	}

#endif
