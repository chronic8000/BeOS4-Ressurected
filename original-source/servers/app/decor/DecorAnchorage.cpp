
#include <stdio.h>

#include "enums.h"
#include "rect.h"
#include "DecorAnchorage.h"
#include "DecorState.h"
#include "DecorVariableInteger.h"

#define ANCHOR_LEFT		0x01
#define ANCHOR_TOP		0x02
#define ANCHOR_RIGHT	0x04
#define ANCHOR_BOTTOM	0x08

struct DecorAnchorageLayout {
	rect	bounds;
};

DecorAnchorage::DecorAnchorage()
	:	m_anchoring(NULL),
		m_leftAnchor(NULL), m_topAnchor(NULL),
		m_rightAnchor(NULL), m_bottomAnchor(NULL),
		m_leftOffset(0), m_topOffset(0),
		m_rightOffset(0), m_bottomOffset(0),
		m_anchors(0),
		m_layoutVars(0), m_bitmask(0), m_mouseContainment(0)
{
}

void 
DecorAnchorage::Unflatten(DecorStream *f)
{
	DecorAtom::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_anchoring);
	f->Read(&m_anchors);
	f->Read(&m_layoutVars);
	f->Read(&m_bitmask);
	f->Read(&m_leftAnchor);
	f->Read(&m_topAnchor);
	f->Read(&m_rightAnchor);
	f->Read(&m_bottomAnchor);
	f->Read(&m_leftOffset);
	f->Read(&m_topOffset);
	f->Read(&m_rightOffset);
	f->Read(&m_bottomOffset);
	f->Read(&m_mouseContainment);
	f->FinishReadBytes();
}

DecorAnchorage::~DecorAnchorage()
{
}

void 
DecorAnchorage::InitLocal(DecorState *instance)
{
	DecorAnchorageLayout *vars = (DecorAnchorageLayout*)instance->LayoutCache()->Lookup(m_layoutVars);
	vars->bounds.left = vars->bounds.top = 0;
	vars->bounds.right = vars->bounds.bottom = -1;
}

void 
DecorAnchorage::CollectRegion(CollectRegionStruct *collect)
{
	DecorAnchorageLayout *vars = (DecorAnchorageLayout*)collect->instance->LayoutCache()->Lookup(m_layoutVars);
	((DecorPart*)m_anchoring->Choose(collect->instance->Space()))->CollectRegion(collect,vars->bounds);
}

void 
DecorAnchorage::AdjustLimits(DecorState *instance, DecorVariableInteger *variable, int32 *min, int32 *max)
{
	LimitsStruct lim;
	int32 pos,high,low;
	DecorPart *part = (DecorPart*)m_anchoring->Choose(instance->Space());
	part->FetchLimits(instance,&lim);

	if ((m_anchors & (ANCHOR_LEFT|ANCHOR_RIGHT)) == (ANCHOR_LEFT|ANCHOR_RIGHT)) {
		if (m_leftAnchor && ((DecorAtom*)variable == m_leftAnchor->Choose(instance->Space()))) {
			pos = m_rightOffset;
			if (m_rightAnchor) pos += ((DecorVariableInteger*)m_rightAnchor->Choose(instance->Space()))->Integer(instance->Space());
			pos = m_leftOffset - pos;
		} else if (m_rightAnchor && (variable == m_rightAnchor->Choose(instance->Space()))) {
			pos = m_leftOffset;
			if (m_leftAnchor) pos += ((DecorVariableInteger*)m_leftAnchor->Choose(instance->Space()))->Integer(instance->Space());
			pos = pos - m_rightOffset;
		} else
			goto tryVertical;
		low = pos + lim.widths[MIN_LIMIT];
		if (*min < low) *min = low;
		if (lim.widths[MAX_LIMIT] == INFINITE_LIMIT) high = INFINITE_LIMIT;
		else high = pos + lim.widths[MAX_LIMIT];
		if (((*max > high) || (*max == INFINITE_LIMIT)) && (high != INFINITE_LIMIT)) *max = high;
	};

	tryVertical:

	if ((m_anchors & (ANCHOR_TOP|ANCHOR_BOTTOM)) == (ANCHOR_TOP|ANCHOR_BOTTOM)) {
		if (m_topAnchor && (variable == m_topAnchor->Choose(instance->Space()))) {
			pos = m_bottomOffset;
			if (m_bottomAnchor) pos += ((DecorVariableInteger*)m_bottomAnchor->Choose(instance->Space()))->Integer(instance->Space());
			pos = m_topOffset - pos;
		} else if (m_bottomAnchor && (variable == m_bottomAnchor->Choose(instance->Space()))) {
			pos = m_topOffset;
			if (m_topAnchor) pos += ((DecorVariableInteger*)m_topAnchor->Choose(instance->Space()))->Integer(instance->Space());
			pos = pos - m_bottomOffset;
		} else
			goto ohWell;
		low = pos + lim.heights[MIN_LIMIT];
		if (*min < low) *min = low;
		if (lim.heights[MAX_LIMIT] == INFINITE_LIMIT) high = INFINITE_LIMIT;
		else high = pos + lim.heights[MAX_LIMIT];
		if (((*max > high) || (*max == INFINITE_LIMIT)) && (high != INFINITE_LIMIT)) *max = high;
	};

	ohWell:;
}

void 
DecorAnchorage::AdjustLimits(DecorState *instance, int32 *minWidth, int32 *minHeight, int32 *maxWidth, int32 *maxHeight)
{
	AdjustLimits(instance,instance->WidthVariable(),minWidth,maxWidth);
	AdjustLimits(instance,instance->HeightVariable(),minHeight,maxHeight);
}

bool DecorAnchorage::NeedAdjustLimits(DecorState *instance)
{
	 return instance->BitMask()->test(m_bitmask+3);
};

bool DecorAnchorage::NeedLayout(DecorState *instance)
{
/*	printf("I DO %sNEED TO LAYOUT!\n",
		(instance->LayoutMask()->test(m_bitmask) ||
		instance->LayoutMask()->test(m_bitmask+1))
		?"":"NOT ");
*/
	return	instance->BitMask()->test(m_bitmask) ||
			instance->BitMask()->test(m_bitmask+1);
}

bool DecorAnchorage::NeedDraw(DecorState *instance)
{
//	printf("I DO %sNEED TO DRAW!\n",instance->LayoutMask()->test(m_bitmask+2)?"":"NOT ");
	return instance->BitMask()->test(m_bitmask+2);
}

uint32 
DecorAnchorage::Layout(CollectRegionStruct *collect)
{
	DecorState *instance = collect->instance;
	DecorAnchorageLayout *vars = (DecorAnchorageLayout*)instance->LayoutCache()->Lookup(m_layoutVars);
	DecorPart *part = (DecorPart*)m_anchoring->Choose(instance->Space());
	rect newBounds,oldBounds;
	LimitsStruct lim;
	uint32 r = 0;
	bool boundsChanged = false;
	bool limitsChanged;

	limitsChanged = (part->CacheLimits(instance) & LIMITS_NEED_LAYOUT);
	part->FetchLimits(instance,&lim);
	if (limitsChanged) instance->BitMask()->set(m_bitmask+3);
	if (lim.widths[PREF_LIMIT] < lim.widths[MIN_LIMIT])
		lim.widths[PREF_LIMIT] = lim.widths[MIN_LIMIT];
	if (lim.heights[PREF_LIMIT] < lim.heights[MIN_LIMIT])
		lim.heights[PREF_LIMIT] = lim.heights[MIN_LIMIT];

	oldBounds = vars->bounds;
	
	if (limitsChanged || instance->BitMask()->test(m_bitmask)) {
		newBounds.left = newBounds.top = newBounds.right = newBounds.bottom = 0x7FFFFFFF;

		if (m_anchors & ANCHOR_LEFT) {
			newBounds.left = m_leftOffset;
			if (m_leftAnchor) newBounds.left +=
				((DecorVariableInteger*)m_leftAnchor->Choose(instance->Space()))->Integer(instance->Space());
		};	
	
		if (m_anchors & ANCHOR_RIGHT) {
			newBounds.right = m_rightOffset;
			if (m_rightAnchor) 	newBounds.right +=
				((DecorVariableInteger*)m_rightAnchor->Choose(instance->Space()))->Integer(instance->Space());
			newBounds.right--;
		};	
	
		if (m_anchors & ANCHOR_TOP) {
			newBounds.top = m_topOffset;
			if (m_topAnchor) newBounds.top +=
				((DecorVariableInteger*)m_topAnchor->Choose(instance->Space()))->Integer(instance->Space());
		};	
	
		if (m_anchors & ANCHOR_BOTTOM) {
			newBounds.bottom = m_bottomOffset;
			if (m_bottomAnchor) newBounds.bottom +=
				((DecorVariableInteger*)m_bottomAnchor->Choose(instance->Space()))->Integer(instance->Space());
			newBounds.bottom--;
		};

//		printf("Anchored to %d,%d,%d,%d\n",newBounds.left,newBounds.top,newBounds.right,newBounds.bottom);
		
		if (newBounds.left != 0x7FFFFFFF) {
			if (newBounds.right == 0x7FFFFFFF)
				newBounds.right = newBounds.left + lim.widths[PREF_LIMIT] - 1;
		} else if (newBounds.right != 0x7FFFFFFF) {
			if (newBounds.left == 0x7FFFFFFF)
				newBounds.left = newBounds.right - lim.widths[PREF_LIMIT] + 1;
		} else {
			printf("No horizontal anchors for anchorage?\n");
			return LAYOUT_ERROR;
		};
	
		if (newBounds.top != 0x7FFFFFFF) {
			if (newBounds.bottom == 0x7FFFFFFF)
				newBounds.bottom = newBounds.top + lim.heights[PREF_LIMIT] - 1;
		} else if (newBounds.bottom != 0x7FFFFFFF) {
			if (newBounds.top == 0x7FFFFFFF)
				newBounds.top = newBounds.bottom - lim.heights[PREF_LIMIT] + 1;
		} else {
			printf("No vertical anchors for anchorage?\n");
			return LAYOUT_ERROR;
		};
		
		boundsChanged = !equal_rect(&newBounds,&vars->bounds);
		if (boundsChanged) {
			vars = (DecorAnchorageLayout*)instance->WritableLayoutCache()->Lookup(m_layoutVars);
			vars->bounds = newBounds;
		};
	} else {
		newBounds = oldBounds;
	};
	
	if (limitsChanged || boundsChanged || instance->BitMask()->test(m_bitmask+1)) {
		vars->bounds = newBounds;
		r = part->Layout(collect,newBounds,oldBounds);
		if (r & LAYOUT_NEED_REDRAW) instance->BitMask()->set(m_bitmask+2);
	};
	
	return r;
}

void 
DecorAnchorage::Draw(DecorState *instance, DrawingContext context, region *update)
{
	DecorAnchorageLayout *vars = (DecorAnchorageLayout*)instance->LayoutCache()->Lookup(m_layoutVars);

	if ((update && overlap_rect(update->Bounds(),vars->bounds) ||
		(!update && instance->BitMask()->test(m_bitmask+2))))
		((DecorPart*)m_anchoring->Choose(instance->Space()))->Draw(instance,vars->bounds,context,update);
}

int32 
DecorAnchorage::HandleEvent(HandleEventStruct *handle)
{
	DecorAnchorageLayout *vars = (DecorAnchorageLayout*)handle->state->LayoutCache()->Lookup(m_layoutVars);
	int32 transit = (handle->event->what == B_MOUSE_MOVED) ? handle->event->Transit() : B_INSIDE;

	if (((handle->event->what != B_MOUSE_MOVED) || !handle->mouseContainers->test(m_mouseContainment)) && (
		(handle->localPoint.h < vars->bounds.left) ||
		(handle->localPoint.h > vars->bounds.right) ||
		(handle->localPoint.v < vars->bounds.top) ||
		(handle->localPoint.v > vars->bounds.bottom)))
		return 0;

//	printf("getting passed a mouse move\n");

	point pt = handle->localPoint;
	handle->localPoint.h -= vars->bounds.left;
	handle->localPoint.v -= vars->bounds.top;
	handle->bounds = vars->bounds;

	if ((handle->event->what == B_MOUSE_MOVED) && (transit != B_EXITED))
		handle->newMouseContainers->set(m_mouseContainment);

	handle->event->SetTransit(transit);
	return ((DecorPart*)m_anchoring->Choose(handle->state->Space()))->HandleEvent(handle);

	handle->localPoint = pt;
}

void 
DecorAnchorage::PropogateChanges(const VarSpace *space, DecorAtom *instigator, DecorState *instance, uint32 depFlags, DecorAtom *newChoice)
{
	if (instigator == m_anchoring) {
		if (depFlags & dfChoice) {
			instance->BitMask()->set(m_bitmask+1);
			instance->BitMask()->set(m_bitmask+2);
			if (newChoice) newChoice->Choose(space)->DownPropagate(space,this,instance,dfLayout|dfRedraw);
		} else {
			if (depFlags & dfLayout)
				instance->BitMask()->set(m_bitmask+1);
			if (depFlags & dfRedraw)
				instance->BitMask()->set(m_bitmask+2);
		};
	} else {
		instance->BitMask()->set(m_bitmask);
	};
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"

	DecorAnchorage::DecorAnchorage(DecorTable *t)
	{
		DecorTable *t2;

		m_leftAnchor = NULL;
		m_rightAnchor = NULL;
		m_topAnchor = NULL;
		m_bottomAnchor = NULL;

		m_leftOffset = 0;
		m_rightOffset = 0;
		m_topOffset = 0;
		m_bottomOffset = 0;
		
		m_anchors = 0;
	
		t2 = t->GetDecorTable("LeftAnchor");
		if (t2) {
			m_leftAnchor = t2->GetVariable("Variable");
			m_leftOffset = (int32)t2->GetNumber("Offset");
			if (m_leftAnchor) {
				if (!m_leftAnchor->IsTypeOf(typeid(DecorVariableInteger)))
					panic(("Anchor must be an integer variable!\n"));
				m_leftAnchor->RegisterDependency(this,dfVariable|dfChoice);
			};
			m_anchors |= ANCHOR_LEFT;
			delete t2;
		};

		t2 = t->GetDecorTable("RightAnchor");
		if (t2) {
			m_rightAnchor = t2->GetVariable("Variable");
			m_rightOffset = (int32)t2->GetNumber("Offset");
			if (m_rightAnchor) {
				if (!m_rightAnchor->IsTypeOf(typeid(DecorVariableInteger)))
					panic(("Anchor must be an integer variable!\n"));
				m_rightAnchor->RegisterDependency(this,dfVariable|dfChoice);
			};
			m_anchors |= ANCHOR_RIGHT;
			delete t2;
		};
	
		t2 = t->GetDecorTable("TopAnchor");
		if (t2) {
			m_topAnchor = t2->GetVariable("Variable");
			m_topOffset = (int32)t2->GetNumber("Offset");
			if (m_topAnchor) {
				if (!m_topAnchor->IsTypeOf(typeid(DecorVariableInteger)))
					panic(("Anchor must be an integer variable!\n"));
				m_topAnchor->RegisterDependency(this,dfVariable|dfChoice);
			};
			m_anchors |= ANCHOR_TOP;
			delete t2;
		};

		t2 = t->GetDecorTable("BottomAnchor");
		if (t2) {
			m_bottomAnchor = t2->GetVariable("Variable");
			m_bottomOffset = (int32)t2->GetNumber("Offset");
			if (m_bottomAnchor) {
				if (!m_bottomAnchor->IsTypeOf(typeid(DecorVariableInteger)))
					panic(("Anchor must be an integer variable!\n"));
				m_bottomAnchor->RegisterDependency(this,dfVariable|dfChoice);
			};
			m_anchors |= ANCHOR_BOTTOM;
			delete t2;
		};

		if (!(m_anchors & (ANCHOR_LEFT|ANCHOR_RIGHT)))
			panic(("Must have at least one horizontal anchor!\n"));

		if (!(m_anchors & (ANCHOR_TOP|ANCHOR_BOTTOM)))
			panic(("Must have at least one horizontal anchor!\n"));
		
		m_anchoring = t->GetPart("Part",ERROR);
		m_anchoring->RegisterDependency(this,dfLayout|dfRedraw|dfChoice);
	}

	void DecorAnchorage::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		m_anchoring->RegisterAtomTree();
		if (m_leftAnchor) m_leftAnchor->RegisterAtomTree();
		if (m_rightAnchor) m_rightAnchor->RegisterAtomTree();
		if (m_topAnchor) m_topAnchor->RegisterAtomTree();
		if (m_bottomAnchor) m_bottomAnchor->RegisterAtomTree();
	}
	
	DecorAtom * DecorAnchorage::Reduce(int32 *proceed)
	{
		int32 subProceed;
		subProceed = 1; while (subProceed) m_anchoring = m_anchoring->Reduce(&subProceed);
		*proceed = 0;
		return this;
	}
	
	bool 
	DecorAnchorage::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorAnchorage)) || DecorAtom::IsTypeOf(t);
	}
	
	void 
	DecorAnchorage::AllocateLocal(DecorDef *decor)
	{
		DecorAtom::AllocateLocal(decor);
		m_layoutVars = decor->AllocateLayout(sizeof(DecorAnchorageLayout));
		m_bitmask = decor->AllocateBitmask(4);
		m_mouseContainment = decor->AllocateMouseContainer(1);
	}
	
	void 
	DecorAnchorage::Flatten(DecorStream *f)
	{
		DecorAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_anchoring);
		f->Write(&m_anchors);
		f->Write(&m_layoutVars);
		f->Write(&m_bitmask);
		f->Write(m_leftAnchor);
		f->Write(m_topAnchor);
		f->Write(m_rightAnchor);
		f->Write(m_bottomAnchor);
		f->Write(&m_leftOffset);
		f->Write(&m_topOffset);
		f->Write(&m_rightOffset);
		f->Write(&m_bottomOffset);
		f->Write(&m_mouseContainment);
		f->FinishWriteBytes();
	}

#endif
