
#include <stdio.h>

#include <AppDefs.h>
#include <View.h>
#include "DecorDef.h"
#include "DecorBehavior.h"
#include "DecorButton.h"
#include "DecorState.h"
#include "DecorActionDef.h"

#define HOT_RECT		0x01

#define INSIDE_BUTTON	0x01
#define BUTTON_DOWN		0x02

struct DecorButtonVars {
	uint8	buttonState;
};

DecorButton::DecorButton()
	: m_actionDef(NULL), m_flags(0), m_localVars(0)
{
	m_buttonImages[0] = m_buttonImages[1] = NULL;
}

DecorButton::~DecorButton()
{
}

DecorAtom * DecorButton::WhichImage(uint8 flags)
{
	DecorAtom *image = m_image;
	switch (flags) {
		case BUTTON_DOWN:
		case INSIDE_BUTTON:
			image = m_buttonImages[0]; break;
		case BUTTON_DOWN|INSIDE_BUTTON:
			image = m_buttonImages[1]; break;
	};
	return image;
};

void DecorButton::GetImage(DecorState *instance, DecorImage **image)
{
	DecorButtonVars *vars = (DecorButtonVars*)instance->Space()->Lookup(m_localVars);
	*image = (DecorImage*)WhichImage(vars->buttonState)->Choose(instance->Space());
}

void DecorButton::InitLocal(DecorState *instance)
{
	DecorWidget::InitLocal(instance);
	DecorButtonVars *vars = (DecorButtonVars*)instance->Space()->Lookup(m_localVars);
	vars->buttonState = 0;
}

uint32 DecorButton::MouseMoved(HandleEventStruct *handle)
{
	DecorButtonVars *vars = (DecorButtonVars*)handle->state->Space()->Lookup(m_localVars);

	bool inside = false;
	const uint8 oldButtonState = vars->buttonState;
	const uint32 transit = handle->event->Transit();

	if (m_flags & HOT_RECT) {
		inside = 	(handle->localPoint.h >= m_hotRect.left) && (handle->localPoint.h <= m_hotRect.right) &&
					(handle->localPoint.v >= m_hotRect.top) && (handle->localPoint.v <= m_hotRect.bottom) &&
					((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW));
		const bool oldInside = (vars->buttonState & INSIDE_BUTTON);
		if (inside != oldInside) {
			if (inside) vars->buttonState |= INSIDE_BUTTON;
			else		vars->buttonState &= ~INSIDE_BUTTON;
		}
	} else {
		if ((transit == B_ENTERED_VIEW) || (transit == B_EXITED_VIEW)) {
			if (transit == B_ENTERED_VIEW)	vars->buttonState |= INSIDE_BUTTON;
			else							vars->buttonState &= ~INSIDE_BUTTON;
			inside = vars->buttonState & INSIDE_BUTTON;
		}
	}

	DecorAtom *newImage = WhichImage(vars->buttonState);
	if (WhichImage(oldButtonState) != newImage)
		PropogateChanges(handle->state->Space(),NULL,handle->state,dfChoice,newImage);

	if (inside) handle->leaf = this;
	return inside?(EVENT_HANDLED|EVENT_TERMINAL):0;
}

uint32 DecorButton::MouseDown(HandleEventStruct *handle)
{
	DecorButtonVars *vars = (DecorButtonVars*)handle->state->Space()->Lookup(m_localVars);
	if (!(vars->buttonState & INSIDE_BUTTON)) return 0;
	uint8 oldButtonState = vars->buttonState;
	vars->buttonState |= BUTTON_DOWN;

	DecorAtom *newImage = WhichImage(vars->buttonState);
	if (WhichImage(oldButtonState) != newImage)
		PropogateChanges(handle->state->Space(),NULL,handle->state,dfChoice,newImage);

	handle->leaf = this;
	return EVENT_HANDLED|EVENT_TERMINAL;
}

uint32 DecorButton::MouseUp(HandleEventStruct *handle)
{
	DecorButtonVars *vars = (DecorButtonVars*)handle->state->Space()->Lookup(m_localVars);
	uint8 oldButtonState = vars->buttonState;

	if (!(vars->buttonState & BUTTON_DOWN)) return 0;

	if ((vars->buttonState & INSIDE_BUTTON) && m_actionDef) {
		DecorActionDef *action = (DecorActionDef*)m_actionDef->Choose(handle->state->Space());
		action->Perform(handle->state,handle->executor);
	};

	vars->buttonState &= ~BUTTON_DOWN;

	DecorAtom *newImage = WhichImage(vars->buttonState);
	if (WhichImage(oldButtonState) != newImage)
		PropogateChanges(handle->state->Space(),NULL,handle->state,dfChoice,newImage);

	handle->leaf = this;
	return EVENT_HANDLED|EVENT_TERMINAL;
}

int32 DecorButton::HandleEvent(HandleEventStruct *handle)
{
	int32 flags = DecorWidget::HandleEvent(handle);
	
	switch (handle->event->what) {
		case B_MOUSE_MOVED:	return flags | MouseMoved(handle);
		case B_MOUSE_DOWN:	return flags | MouseDown(handle);
		case B_MOUSE_UP:	return flags | MouseUp(handle);
	};
	
	return flags;
};

void DecorButton::Unflatten(DecorStream *f)
{
	DecorWidget::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_buttonImages[0]);
	f->Read(&m_buttonImages[1]);
	f->Read(&m_actionDef);
	f->Read(&m_flags);
	f->Read(&m_localVars);
	f->Read(&m_hotRect);
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)
	#include "LuaDecor.h"
	
	DecorButton::DecorButton(DecorTable *t) : DecorWidget(t)
	{
		m_flags = 0;
		m_actionDef = t->GetAtom("ActionDef",0);
		m_buttonImages[0] = t->GetImage("RolloverLook");
		m_buttonImages[1] = t->GetImage("ActivatedLook");
		if (m_buttonImages[0]) m_buttonImages[0]->RegisterDependency(this,dfChoice);
		else m_buttonImages[0] = m_image;
		if (m_buttonImages[1]) m_buttonImages[1]->RegisterDependency(this,dfChoice);
		else m_buttonImages[1] = m_image;
		t = t->GetDecorTable("HotRect");
		if (t) {
			m_flags |= HOT_RECT;
			m_hotRect.left = (int32)t->GetNumber(1);
			m_hotRect.top = (int32)t->GetNumber(2);
			m_hotRect.right = (int32)t->GetNumber(3);
			m_hotRect.bottom = (int32)t->GetNumber(4);
			delete t;
		} else {
			// XXX
			m_flags |= HOT_RECT;
			m_hotRect.left = m_hotRect.top = 0;
			// mathias: what's that??? This code is fucked up. m_hotRect.right / m_hotRect.bottom are not initialized here!
			// I dont know what it is supposed to do anyway.
			m_hotRect.right = m_imageRect.right-m_imageRect.left;
			m_hotRect.bottom = m_imageRect.bottom-m_imageRect.top;
		};
	}

	void DecorButton::RegisterAtomTree()
	{
		DecorWidget::RegisterAtomTree();
		m_buttonImages[0]->RegisterAtomTree();
		m_buttonImages[1]->RegisterAtomTree();
		if (m_actionDef) m_actionDef->RegisterAtomTree();
	}
	
	DecorAtom * DecorButton::Reduce(int32 *proceed)
	{
		int32 subProceed;
		subProceed = 1; while (subProceed) m_image = m_image->Reduce(&subProceed);
		subProceed = 1; while (subProceed) m_buttonImages[0] = m_buttonImages[0]->Reduce(&subProceed);
		subProceed = 1; while (subProceed) m_buttonImages[1] = m_buttonImages[1]->Reduce(&subProceed);
		*proceed = 0;
		return this;
	}

	bool DecorButton::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(*this)) || DecorWidget::IsTypeOf(t);
	}
	
	void DecorButton::AllocateLocal(DecorDef *decor)
	{
		DecorWidget::AllocateLocal(decor);
		m_localVars = decor->AllocateVarSpace(sizeof(DecorButtonVars));
	}

	void DecorButton::Flatten(DecorStream *f)
	{
		DecorWidget::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_buttonImages[0]);
		f->Write(m_buttonImages[1]);
		f->Write(m_actionDef);
		f->Write(&m_flags);
		f->Write(&m_localVars);
		f->Write(&m_hotRect);
		f->FinishWriteBytes();
	}
	
#endif
