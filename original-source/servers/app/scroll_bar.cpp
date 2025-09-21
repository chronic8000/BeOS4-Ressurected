//******************************************************************************
//
//	File: scroll_bar.h
//
//	Description: The damn ScrollBar
//	
//	Written by:	Mathias Agopian
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#include <OS.h>
#include <Debug.h>

#include "scroll_bar.h"

#include "server.h"
#include "rect.h"
#include "eventport.h"

#include "DecorState.h"
#include "DecorEvent.h"
#include "DecorThumb.h"
#include "nuDecor.h"

extern bool scroll_double_arrows;

// Decor variables
static const char * const VAR_RANGE = "@range";
static const char * const VAR_VALUE = "@value";
static const char * const VAR_PROPORTION = "@proportion";
static const char * const VAR_DOUBLE = "double";

TScrollBar::TScrollBar(rect* bnd, int32 orientation, uint32 flags)
   :	TView(bnd, flags | FULL_DRAW | FAST, vfWillDraw | vfNeedsHilite),
   		m_decor(NULL),
		m_newDecorState(NULL),
		m_mouseDownDecorInstance(NULL),
		m_workingEvent(NULL),
		m_inputChangeRec(NULL),
		m_mouseDownPart(NULL),
		m_transactionOpen(false),
		m_transaction_level(0),
		m_workingThumb(NULL),
		m_mouseState(msNothing),
		m_lastMove(0),
		m_lastDelta(-100000),
		f_proportion(-1.0f),// -1 -> not set
		f_value(0),			// keep in sync with libbe (ScrollBar.cpp)
		s_value(-1000000),
		f_min(0),			// keep in sync with libbe (ScrollBar.cpp)
		f_max(1000),		// keep in sync with libbe (ScrollBar.cpp)
		f_page_value(10),	// keep in sync with libbe (ScrollBar.cpp)
		f_inc_value(1),		// keep in sync with libbe (ScrollBar.cpp)
		f_vertical(orientation == B_VERTICAL),
		m_queueIterValue(0xFFFFFFFF),
		m_valueMsgToken(NONZERO_NULL)
{
	// init the decor
	m_oldBounds.left = m_oldBounds.top = m_oldBounds.right = m_oldBounds.bottom = -1;
	m_oldActive = !IsActive();

	m_lastMouseContainment = &m_mouseContainer1;
	
	ResetDecor();
}

//------------------------------------------------------------------------------

TScrollBar::~TScrollBar()
{
	m_decor->ServerUnlock();
	if (m_newDecorState)			m_newDecorState->ServerUnlock();
	if (m_inputChangeRec)			m_inputChangeRec->ServerUnlock();
	if (m_mouseDownDecorInstance)	m_mouseDownDecorInstance->ServerUnlock();
}

void TScrollBar::OpenDecorTransaction()
{
	if (m_transaction_level == 0) {
		if (!m_newDecorState) {
			m_newDecorState = new DecorState(m_decor);
			m_newDecorState->ServerLock();
			m_newDecorState->OpenTransaction();
		}
	}
	m_transaction_level++;
}

void TScrollBar::CloseDecorTransaction()
{
	if (m_transaction_level == 1) {
		if (m_newDecorState) {
			m_newDecorState->BitMask()->setAll();
			m_newDecorState->PartialCommit();
			if (m_newDecorState->NeedLayout())
				m_newDecorState->Layout(NULL);
			m_newDecorState->CommitChanges();
			m_newDecorState->CloseTransaction();
			if (m_newDecorState->NeedDraw())
				m_newDecorState->Draw(renderContext);
			m_newDecorState->ServerUnlock();
			m_newDecorState = NULL;
		}
	}
	m_transaction_level--;
}

void TScrollBar::OpenTransaction()
{
	if (!m_transactionOpen) {
		if (m_newDecorState) {
			debugger("m_newDecorState != NULL !");
		}
		m_newDecorState = m_inputChangeRec;
		m_newDecorState->ServerLock();
		m_transactionOpen = true;
		OpenDecorTransaction();
	}
}

//------------------------------------------------------------------------------

void TScrollBar::Draw(bool inUpdate)
{
	if (m_newDecorState != NULL) {
		DebugPrintf(("TScrollBar::Draw() m_newDecorState == %p\n", m_newDecorState));
	}

	if (inUpdate)
		BeginUpdate();

	const bool active = IsActive();
	rect t;
	get_bound(&t);

	const bool need_transaction =
				((m_oldBounds.left != t.left) ||
				(m_oldBounds.top != t.top) ||
				(m_oldBounds.right != t.right) ||
				(m_oldBounds.bottom != t.bottom) ||
				(active != m_oldActive));

	if (need_transaction)
	{
		OpenDecorTransaction();
		m_newDecorState->SetBounds(t);
		m_oldBounds = t;
		m_newDecorState->SetFocus(active);
		m_oldActive = active;
		CloseDecorTransaction();
	} else {
		m_decor->BitMask()->setAll();
		m_decor->Draw(renderContext);
	}

	if (inUpdate)
		EndUpdate();
}

// ----------------------------------------------------------------
// #pragma mark -

void TScrollBar::HandleEvent(BParcel *event)
{
	// We need a write lock (because we'll draw)
	the_server->UnlockWindowsForRead();
	the_server->LockWindowsForWrite();
	if (!is_still_valid()) {
		the_server->DowngradeWindowsWriteToRead();
		return;
	}

	m_workingEvent = event;
	m_inputChangeRec->Reparent(m_decor);
	m_inputChangeRec->OpenTransaction();

	// Set the point, so that it is more easy to access it
	fpoint pt;
	point ipt;
	m_workingEvent->FindPoint("be:view_where", &pt);
	ipt.h = (int32)pt.h; ipt.v = (int32)pt.v;
	m_workingEvent->SetScreenLocation(ipt);
	m_workingEvent->SetLocation(pt);

	// build the HandleEventStruct to pass to the decor
	HandleEventStruct handle;
	handle.state = m_inputChangeRec;
	handle.event = event;
	handle.executor = this;
	handle.localPoint.h = ipt.h;
	handle.localPoint.v = ipt.v;
	handle.leaf = NULL;
	handle.thumb = NULL;
	// For some reason I don't get for now, the "Transit" is not set in the message
	handle.event->SetTransit(handle.event->FindInt32("be:transit"));

	// We'll need to acces 'handle.thumb' from Action()
	m_workingEventStruct = &handle;

	// The simple state machine...
	if (event->what == B_MOUSE_MOVED)
	{
		if (m_mouseState == msDragThumb)
		{
			const point pt = the_server->GetMouse(); // Filtered mouse coords
			const int32 delta = (vertical() ? (pt.v - m_dragPt.v) : (pt.h - m_dragPt.h));
			const bigtime_t now = system_time();
			// Don't do anything if the mouse didn't move
			if (delta == m_lastDelta)
				goto exit;
			if ((now - m_lastMove) < the_server->ScreenRefreshPeriod())
				goto exit;
			m_lastDelta = delta;
			m_lastMove = now;
			const int32 value = f_min + m_workingThumb->GetValueFromSetPixel(delta);
			if (value != f_value) {
				OpenTransaction();
				set_value(value, true);
			}
		}
		else
		{
			// Handle the rollovers
			const int32 bits = m_decor->Def()->MouseContainerCount();
			if (m_mouseContainer1.size() < bits)	m_mouseContainer1.setSize(bits);
			if (m_mouseContainer2.size() < bits)	m_mouseContainer2.setSize(bits);
			handle.mouseContainers = m_lastMouseContainment;
			handle.newMouseContainers = &m_mouseContainer1;
			if (handle.newMouseContainers == handle.mouseContainers)
				handle.newMouseContainers = &m_mouseContainer2;
			handle.newMouseContainers->clearAll();
			handle.state->Def()->HandleEvent(&handle);
			m_lastMouseContainment = handle.newMouseContainers;
		}
	}
	else if (event->what == B_MOUSE_UP)
	{
		if (m_mouseState == msDragThumb)
		{
			const point pt = the_server->GetMouse(); // Filtered mouse coords
			const int32 delta = (vertical() ? (pt.v - m_dragPt.v) : (pt.h - m_dragPt.h));
			if (delta != m_lastDelta) {
				const int32 value = f_min + m_workingThumb->GetValueFromSetPixel(delta);
				if (value != f_value) {
					OpenTransaction();
					set_value(value, true);
				}
			}
			// Release the decor
			if (m_mouseDownDecorInstance) {
				m_mouseDownDecorInstance->ServerUnlock();
				m_mouseDownDecorInstance = NULL;
			}
			m_mouseState = msNothing;
			m_workingThumb->StopDragging();
			m_workingThumb = NULL;
		}
		// If we clicked on an decor part, handle that event
		if (m_mouseDownPart)
			m_mouseDownPart->HandleEvent(&handle);
		m_mouseDownPart = NULL;
		UnaugmentEventMask(LOCK_STATE_WRITE);
	}
	else if (event->what == B_MOUSE_DOWN)
	{
		AugmentEventMask(etPointerEvent, aoLockWindowFocus | aoNoPointerHistory);
		if (m_mouseState == msDragThumb)
		{ // nothing to do here
		} else {
			handle.state->Def()->HandleEvent(&handle);
			m_mouseDownPart = handle.leaf; // set by the preceding call
		}
	}
	
	// ---------------------------------------------
	// Update the decor
	if (!m_transactionOpen) {
		handle.state->PartialCommit();
		if (handle.state->NeedLayout())
			handle.state->Layout(NULL);
		handle.state->CommitChanges();
		if (renderContext && handle.state->NeedDraw())
			handle.state->Draw(renderContext);
	} else {
		CloseDecorTransaction();
		m_transactionOpen = false;
	}

exit:
	the_server->DowngradeWindowsWriteToRead();
	handle.state->CloseTransaction();
	m_workingEventStruct = NULL;
}

void TScrollBar::HandleDecorEvent(Event *event)
{
	if (!is_still_valid())
		goto exit;

	m_inputChangeRec->Reparent(m_decor);
	m_inputChangeRec->OpenTransaction();

	if ((event->event == NULL) && (event->part)) { // bullet proofing
		if (event->state != m_decor) { // because we are paranoid. Should not happen.
			DebugPrintf(("TScrollBar::HandleDecorEvent() WARNING : event->state (%p) != m_decor (%p)", event->state, m_decor));
		}
		BParcel msg(B_PULSE);
		m_workingEvent = &msg;
		HandleEventStruct handle;
		handle.state = m_inputChangeRec;
		handle.event = m_workingEvent;
		handle.executor = this;
		handle.localPoint.h = 0;
		handle.localPoint.v = 0;
		handle.leaf = NULL;
		handle.thumb = NULL;
		m_workingEventStruct = &handle;
		// Directly apply the event on the requesting DecorPart
		event->part->HandleEvent(&handle);
	} else if (event->event) { // bullet proofing
		event->event->Trigger(m_inputChangeRec, this);
	}

	if (!m_transactionOpen) {
		m_inputChangeRec->PartialCommit();
		if (m_inputChangeRec->NeedLayout())
			m_inputChangeRec->Layout(NULL);
		m_inputChangeRec->CommitChanges();
		if (renderContext && m_inputChangeRec->NeedDraw())
			m_inputChangeRec->Draw(renderContext);
	} else {
		CloseDecorTransaction();
		m_transactionOpen = false;
	}
	m_inputChangeRec->CloseTransaction();

exit:
	if (event->state)	event->state->ServerUnlock();
	if (event->view)	event->view->ServerUnlock();
	if (event->window)	event->window->ServerUnlock();
	delete event;
}

bool TScrollBar::ResetDecor()
{
	if (m_inputChangeRec) {
		m_inputChangeRec->ServerUnlock();
		m_inputChangeRec = NULL;
	}
	if (m_mouseDownDecorInstance) {
		m_mouseDownDecorInstance->ServerUnlock();
		m_mouseDownDecorInstance = NULL;
	}
	if (m_newDecorState) {
		m_newDecorState->ServerUnlock();
		m_newDecorState = NULL;
	}
	m_mouseDownPart = NULL;
	
	if (m_decor) m_decor->ServerUnlock();
	m_decor = GenerateDecor(vertical() ? 101 : 100);
	m_decor->ServerLock();

	m_decor->OpenTransaction();
	rect t;
	get_bound(&t);
	m_decor->SetBounds(t);
	m_decor->SetFocus(IsActive());
	m_decor->SetInteger(VAR_RANGE, (f_max-f_min));
	m_decor->SetInteger(VAR_VALUE, f_value - f_min);
	m_decor->SetInteger(VAR_PROPORTION, Proportion());
	m_decor->SetInteger(VAR_DOUBLE, (int32)scroll_double_arrows);	
	m_decor->BitMask()->setAll();
	m_decor->PartialCommit();
	m_decor->Layout(NULL);
	m_decor->CommitChanges();
	m_decor->CloseTransaction();

	m_inputChangeRec = new DecorState(m_decor);
	m_inputChangeRec->ServerLock();
	
	return true;
}

// ----------------------------------------------------------------
// #pragma mark -

void TScrollBar::Action(DecorState *changes, BMessage *action)
{
	OpenTransaction();
	switch (action->what) {
		case SCROLL_DOWN_ACTION:
			set_value(f_value-f_inc_value, true);
			break;
		
		case SCROLL_UP_ACTION:
			set_value(f_value+f_inc_value, true);
			break;

		case SCROLL_PAGE_DOWN_ACTION:
			set_value(f_value-f_page_value, true);
			break;
		
		case SCROLL_PAGE_UP_ACTION:
			set_value(f_value+f_page_value, true);
			break;

		case SCROLL_THUMB_ACTION:
			Action_DragThumb();
			break;

		case SCROLL_END_ACTION:
			set_value(f_max, true);
			break;
		
		case SCROLL_HOME_ACTION:
			set_value(f_min, true);
			break;

		default:
			DecorExecutor::Action(changes, action);
	}
}

void TScrollBar::Action_DragThumb()
{
	if (Owner()->is_focus() == false) {
		Owner()->bring_to_front();
		if ((Owner()->flags & DISLIKE_FOCUS) == false) {
			return;
		}
	}

	m_workingThumb = dynamic_cast<DecorThumb *>(m_workingEventStruct->thumb);	
	if (m_workingThumb) {
		// Keep a reference on the curent decor until we reach MOUSE_UP
		if (m_mouseDownDecorInstance) m_mouseDownDecorInstance->ServerUnlock();
		m_mouseDownDecorInstance = m_decor;
		m_mouseDownDecorInstance->ServerLock();

		m_mouseState = msDragThumb;
		m_dragPt = the_server->GetMouse();	// Filtered mouse coords
		m_workingThumb->StartDragging(m_workingEventStruct);
	}
}

void TScrollBar::Action_SetEvent(DecorEvent *event, uint32 time)
{
	Event *newEvent = new Event;
	newEvent->event = event;
	newEvent->window = Owner();
	newEvent->view = this;
	newEvent->time = system_time() + time;
	newEvent->view->ServerLock();
	newEvent->window->ServerLock();
	Event **p = the_server->WindowManager()->EventList();
	while (*p && ((*p)->time < newEvent->time)) p = &(*p)->next;
	newEvent->next = *p;
	*p = newEvent;
}

void TScrollBar::Action_SetAlarm(bigtime_t time)
{
	// Be defensive here...
	if (m_workingEventStruct->leaf == NULL) { // weird, should not happen
		DebugPrintf(("TScrollBar::Action_SetAlarm(%Lu). Alarm not set because leaf = NULL\n", time));
		return;
	}
	
	if (time) {
		// Make sure this DecorPart/DecorState has no alarm set
		Event **p = the_server->WindowManager()->EventList();
		while (*p) {
			if (((*p)->part == m_workingEventStruct->leaf) &&
				((*p)->state == m_decor))
			{
				DebugPrintf(("TScrollBar::Action_SetAlarm(%Lu). Alarm not set because there is one already\n", time));
				return;
			}
			p = &(*p)->next;
		}

		Event *newEvent = new Event;
		newEvent->event = NULL;
		newEvent->part = m_workingEventStruct->leaf;
		newEvent->state = m_decor;
		newEvent->window = Owner();
		newEvent->view = this;
		newEvent->time = system_time() + time;
		newEvent->state->ServerLock();
		newEvent->view->ServerLock();
		newEvent->window->ServerLock();
		p = the_server->WindowManager()->EventList();
		while (*p && ((*p)->time < newEvent->time)) p = &(*p)->next;
		newEvent->next = *p;
		*p = newEvent;
	} else {
		// Geh's style...
		Event **p = the_server->WindowManager()->EventList();
		while (*p) { // remove all alarms set by this DecorPart
			if (((*p)->part == m_workingEventStruct->leaf) &&
				((*p)->state == m_decor))
			{
				(*p)->state->ServerUnlock();
				(*p)->view->ServerUnlock();
				(*p)->window->ServerUnlock();
				Event *next = (*p)->next;
				delete (*p);
				*p = next;
				continue;
			}
			p = &(*p)->next;
		}
	}
}

//------------------------------------------------------------

int32 TScrollBar::Proportion() const
{ // convert the proportion into an int32
	float p;
	if (f_proportion < 0) { // proportion not set, use the page value
		p = f_page_value / (float)(f_page_value + f_max - f_min);
	}else{
		p = f_proportion;
	}
	return *(int32*)(&p);
}

//------------------------------------------------------------

void TScrollBar::set_steps(int32 small_step, int32 big_step)
{
	f_inc_value = small_step;
	f_page_value = big_step;
	the_server->LockWindowsForWrite();
	if (is_still_valid()) {
		OpenDecorTransaction();
		if (f_proportion < 0)
			m_newDecorState->SetInteger(VAR_PROPORTION, Proportion());
		const bool active = IsActive();
		m_newDecorState->SetFocus(active);
		CloseDecorTransaction();
	}
	the_server->UnlockWindowsForWrite();
}

//------------------------------------------------------------

void TScrollBar::set_range(int32 min, int32 max)
{
	const bool min_changed = (f_min != min);
	const bool max_changed = (f_max != max);
	if (min_changed || max_changed) {
		f_min = min;
		f_max = max;
		//DebugPrintf(("set_range(%ld,%ld)\n", f_min, f_max));
		if (f_value < f_min) 		set_value(f_min, false);
		else if (f_value > f_max)	set_value(f_max, false);
		the_server->LockWindowsForWrite();
		if (is_still_valid()) {
			OpenDecorTransaction();
			m_newDecorState->SetInteger(VAR_RANGE, f_max-f_min);
			if (min_changed)		m_newDecorState->SetInteger(VAR_VALUE, f_value-f_min);
			if (f_proportion < 0)	m_newDecorState->SetInteger(VAR_PROPORTION, Proportion());
			m_newDecorState->SetFocus(IsActive());
			CloseDecorTransaction();
		}
		the_server->UnlockWindowsForWrite();
	}
}

//------------------------------------------------------------

void TScrollBar::set_value(int32 val, bool msg)
{
	if ((m_mouseState == msDragThumb) && !msg)
		return;
	if (val < f_min)		val = f_min;
	else if (val > f_max)	val = f_max;
	if (val == f_value) {
		if ((s_value != val) && (msg))
			send_value();
		return;
	}
	f_value = val;
	//DebugPrintf(("set_value(%ld->%ld)\n", val, f_value));

	the_server->LockWindowsForWrite();
	if (is_still_valid()) {
		OpenDecorTransaction();
		m_newDecorState->SetInteger(VAR_VALUE, f_value - f_min);
		CloseDecorTransaction();
	}
	the_server->UnlockWindowsForWrite();

	if (msg)
		send_value();
}


//------------------------------------------------------------

void TScrollBar::set_proportion(float p)
{
	f_proportion = p;
	the_server->LockWindowsForWrite();
	if (is_still_valid()) {
		OpenDecorTransaction();
		m_newDecorState->SetInteger(VAR_PROPORTION, Proportion());
		CloseDecorTransaction();
	}
	the_server->UnlockWindowsForWrite();
}

//------------------------------------------------------------


void TScrollBar::set_min_max(int32 min, int32 max)
{ // This is called only once after the ctor
	if (max < min) {
		DebugPrintf(("TScrollBar::set_min_max illegal values\n"));
	}	
	if ((f_min != min) || (f_max != max)) {
		f_min = min;
		f_max = max;
		m_decor->SetInteger(VAR_RANGE, f_max-f_min);
		m_decor->SetInteger(VAR_VALUE, f_value-f_min);
		m_decor->SetInteger(VAR_PROPORTION, Proportion());
		m_decor->SetFocus(IsActive());
	}
}

//------------------------------------------------------------

void TScrollBar::send_value()
{
	if (s_value == f_value) return;
	if (Owner()) {
		BParcel	a_parcel(B_VALUE_CHANGED);
		a_parcel.SetTarget(client_token, false);
		a_parcel.AddInt64("when", system_time());
		a_parcel.AddInt32("value", f_value);
		Owner()->EventPort()->ReplaceEvent(&a_parcel, &m_queueIterValue, &m_valueMsgToken);
		s_value = f_value;
	}
}


