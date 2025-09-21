
#include <stdio.h>

#include "as_support.h"
#include "enums.h"
#include "rect.h"
#include "DecorDef.h"
#include "DecorThumb.h"
#include "DecorBehavior.h"
#include "DecorActionDef.h"
#include "DecorState.h"
#include "DecorVariableInteger.h"

DecorThumb::DecorThumb()
	:	m_dragPosition(0)
{
}

DecorThumb::~DecorThumb()
{
}

int32 DecorThumb::HandleEvent(HandleEventStruct *handle)
{
	handle->thumb = this;
	int32 ret = DecorGroup::HandleEvent(handle);
	if (ret & EVENT_HANDLED) {
		ret |= EVENT_TERMINAL;
	}
	return ret;
}

uint32 DecorThumb::Layout(
	CollectRegionStruct *collect,
	rect group, rect oldBounds)
{
	DecorState *changes = collect->instance;

	float thumbProp = 0;
	const int32 thumbRange = ((DecorVariableInteger*)m_range->Choose(changes->Space()))->Integer(changes->Space());
	const int32 thumbValue = ((DecorVariableInteger*)m_value->Choose(changes->Space()))->Integer(changes->Space());
	const int32 prop = ((DecorVariableInteger*)m_prop->Choose(changes->Space()))->Integer(changes->Space());
	const float newProp = *((float *)(&prop));
	if ((newProp >= 0.0) && (newProp <= 1.0))
		thumbProp = newProp;

	int32 slideArea;
	rect thumbRect = group;

	if (thumbRange > 0) {
		int32 pixelValue = 0;
		int32 pixelSize = 0;
		DecorGroupLimits *vars = (DecorGroupLimits*)changes->LimitCache()->Lookup(m_limitSpace);
		if (m_orientation == GROUP_H) {
			// First the thumb's size
			slideArea = thumbRect.right - thumbRect.left + 1;
			pixelSize = (int32)((float)slideArea * thumbProp);
			if (pixelSize < vars->lim.widths[MIN_LIMIT])
				pixelSize = vars->lim.widths[MIN_LIMIT];
			else if ((vars->lim.widths[MAX_LIMIT] >= 0) && (pixelSize > vars->lim.widths[MAX_LIMIT]))
				pixelSize = vars->lim.widths[MAX_LIMIT];
	
			// Then the position
			pixelValue = (int32)(((float)(slideArea - pixelSize) * (float)thumbValue) / (float)thumbRange);
			if (pixelValue < 0) pixelValue = 0;
	
			// Then, make sure it fits in the bar
			if (thumbRect.left + pixelValue + pixelSize > thumbRect.right)
				pixelValue = thumbRect.right - thumbRect.left - pixelSize;

			thumbRect.left += pixelValue;
			thumbRect.right = thumbRect.left + pixelSize;	
		} else {
			// First the thumb's size
			slideArea = thumbRect.bottom - thumbRect.top + 1;
			pixelSize = (int32)((float)slideArea * thumbProp);
			if (pixelSize < vars->lim.heights[MIN_LIMIT])
				pixelSize = vars->lim.heights[MIN_LIMIT];
			else if ((vars->lim.heights[MAX_LIMIT] >= 0) && (pixelSize > vars->lim.heights[MAX_LIMIT]))
				pixelSize = vars->lim.heights[MAX_LIMIT];
	
			// Then the position
			pixelValue = (int32)(((float)(slideArea - pixelSize) * (float)thumbValue) / (float)thumbRange);
			if (pixelValue < 0) pixelValue = 0;
	
			// Then, make sure it fits in the bar
			if (thumbRect.top + pixelValue + pixelSize > thumbRect.bottom)
				pixelValue = thumbRect.bottom - thumbRect.top - pixelSize;

			thumbRect.top += pixelValue;
			thumbRect.bottom = thumbRect.top + pixelSize;
		}
		//DebugPrintf(("v=%ld, s=%ld\n", pixelValue, pixelSize));
	} else { // just compute the slideArea in this case
		if (m_orientation == GROUP_H) {
			slideArea = thumbRect.right - thumbRect.left + 1;
		} else {
			slideArea = thumbRect.bottom - thumbRect.top + 1;
		}
		//DebugPrintf(("thumbRange=%ld\n", thumbRange));
	}
	// We need to keep the size of the scrollvar in pixels
	DecorValue v;
	v.i = slideArea;
	((DecorVariableInteger *)m_slideAreaVar)->SetValue(changes->WritableSpace(), changes, v);


	// do the actual layout
	DecorGroupLayout layout[32];
	
	if (m_partCount != 3) {
		printf(	"%s : m_partCount should be 3, but it is %d\n", Name(), (int)m_partCount);
		return 0;
	}
	
	rect& newBounds = group;
	const bool moved = (newBounds.left != oldBounds.left)
						|| (newBounds.top != oldBounds.top);
	Bitmap *bitmask = changes->BitMask();

	// Retrieve previous child layout.
	const DecorGroupLayout *vars;
	ReadLayout(changes, m_layoutSpace, &vars);
	
	// Stash away the current region and start a new one.
	region *savedRegion = collect->theRegion;
	collect->theRegion = newregion();

	uint32 single,r = 0;
	bool changed = false;
	for (int32 i=0;i<m_partCount;i++) {
		// Retrieve child.
		DecorPart* c = ChildAt(changes,i);
		if (!c) continue;
		
		// Get child layout limits.
		LimitsStruct l;
		c->FetchLimits(changes, &l);
		
		layout[i].frame = newBounds;
		switch (i)
		{
			case 0:
				if (m_orientation == GROUP_H) {
					layout[i].frame.right = thumbRect.left-1;
				} else {
					layout[i].frame.bottom = thumbRect.top-1;
				}
				break;
			case 1:
				layout[i].frame = thumbRect;
				break;
			case 2:
				if (m_orientation == GROUP_H) {
					layout[i].frame.left = thumbRect.right+1;
				} else {
					layout[i].frame.top = thumbRect.bottom+1;
				}
				break;
		}

		rect frame = layout[i].frame;
		offset_rect(&layout[i].frame, -newBounds.left, -newBounds.top);
		
		// Has this child changed location in the stack since the last layout?
		const bool boundsChanged = !equal_rect(&layout[i].frame,&vars[i].frame);
		changed = changed || boundsChanged;
		
		// If location in stack, or location of stack itself, changed; or just
		// need to layout, then do the layout.
		if (boundsChanged || moved || bitmask->test(m_bitmaskIndex+i)) {
			// Convert previous frame into absolute coordinates.
			rect oldFrame = vars[i].frame;
			offset_rect(&oldFrame, oldBounds.left, oldBounds.top);
			single = c->Layout(collect,frame,oldFrame);
			r |= single;
		};
	};

	// Any of the child positions changed, need to write them back into
	// the decor state.
	if (changed) {
		DecorGroupLayout *dest;
		WriteLayout(changes, m_layoutSpace, &dest);
		for (int32 i=0;i<m_partCount;i++)
			dest[i] = layout[i];
	};
	
	// Apply regions from children.
	or_region(collect->theRegion,savedRegion,collect->tmpRegion[0]);
	kill_region(savedRegion);
	savedRegion = collect->tmpRegion[0];
	collect->tmpRegion[0] = collect->theRegion;
	collect->theRegion = savedRegion;

	// If any children reported that they need to be drawn, then we need
	// to redraw everything in the stack.
	if (r & LAYOUT_NEED_REDRAW) bitmask->set(m_bitmaskIndex+m_partCount);
	
	return r;
}

int32 DecorThumb::GetValueFromSetPixel(int32 p)
{
	int32 range = m_slideArea - m_size;
	if (range <= 0)
		range = 1;
	int32 value = (int32)(((m_dragPosition + p) * (float)m_thumbRange) / (float)range);
	if (value < 0)					value = 0;
	else if (value > m_thumbRange)	value = m_thumbRange;
	return value;
}

void DecorThumb::StartDragging(HandleEventStruct *handle)
{
	const DecorGroupLayout *layout;
	ReadLayout(handle->state, m_layoutSpace, &layout);
	if (m_orientation == GROUP_H) {
		m_dragPosition = layout[1].frame.left;
		m_size = layout[1].frame.right - layout[1].frame.left + 1;
	} else {
		m_dragPosition = layout[1].frame.top;
		m_size = layout[1].frame.bottom - layout[1].frame.top + 1;
	}		
	m_thumbRange = ((DecorVariableInteger*)m_range->Choose(handle->state->Space()))->Integer(handle->state->Space());
	m_slideArea = ((DecorVariableInteger*)m_slideAreaVar->Choose(handle->state->Space()))->Integer(handle->state->Space());
}

void DecorThumb::StopDragging()
{
	m_dragPosition = 0;
}

void DecorThumb::Unflatten(DecorStream *f)
{
	DecorGroup::Unflatten(f);	
	f->StartReadBytes();
	f->Read(&m_range);
	f->Read(&m_value);
	f->Read(&m_prop);
	f->Read(&m_slideAreaVar);
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	
	DecorThumb::DecorThumb(DecorTable *t) : DecorGroup(t)
	{
		if (m_partCount != 3)	panic(("Thumb require 3 parts (not more, not less)!\n"));
		m_range = t->GetVariable("Range");
		if (!m_range)											panic(("Thumb has no Range variable!\n"));
		if (!m_range->IsTypeOf(typeid(DecorVariableInteger)))	panic(("Range must be an integer variable!\n"));
		m_value = t->GetVariable("Value");
		if (!m_value)											panic(("Thumb has no Value variable!\n"));
		if (!m_value->IsTypeOf(typeid(DecorVariableInteger)))	panic(("Value must be an integer variable!\n"));
		m_prop = t->GetVariable("Proportion");
		if (!m_prop)											panic(("Thumb has no Prop variable!\n"));
		if (!m_prop->IsTypeOf(typeid(DecorVariableInteger)))	panic(("Prop must be an integer variable!\n"));
		m_slideAreaVar = new DecorVariableIntegerSingle();
	}

	void DecorThumb::RegisterAtomTree()
	{
		DecorGroup::RegisterAtomTree();
		m_range->RegisterAtomTree();
		m_range->RegisterDependency(this, dfVariable | dfLayout | dfRedraw);
		m_value->RegisterAtomTree();
		m_value->RegisterDependency(this, dfVariable | dfLayout | dfRedraw);
		m_prop->RegisterAtomTree();
		m_prop->RegisterDependency(this, dfVariable | dfLayout | dfRedraw);
		m_slideAreaVar->RegisterAtomTree();
	}
	
	bool DecorThumb::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorThumb)) || DecorPart::IsTypeOf(t);
	}

	void DecorThumb::Flatten(DecorStream *f)
	{
		DecorGroup::Flatten(f);
		f->StartWriteBytes();
		f->Write(m_range);
		f->Write(m_value);
		f->Write(m_prop);
		f->Write(m_slideAreaVar);
		f->FinishWriteBytes();
	}
	
#endif
