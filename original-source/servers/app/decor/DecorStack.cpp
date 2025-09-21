
#include <stdio.h>

#include "as_support.h"
#include "enums.h"
#include "rect.h"
#include "DecorDef.h"
#include "DecorStack.h"
#include "DecorBehavior.h"
#include "DecorActionDef.h"

// This is the per-instance constraint data that we store for a
// particular DecorStack object.
struct DecorStackVars {
	LimitsStruct lim;
};

// This is the per-instance layout data that we store for every
// child in a stack.  (I.e., there are m_numParts of these in a
// particular stack.)  This frame is stored relative to the Stack.
// That is, if a child's top left corner is at the top left corner
// of the stack, then frame.left == frame.right == 0.
struct DecorStackLayout {
	rect	frame;
};

DecorStack::DecorStack()
	: m_partList(NULL)
{
}

DecorStack::~DecorStack()
{
	grFree(m_partList);
}

visual_style DecorStack::VisualStyle(DecorState *instance)
{
	visual_style style = B_OPAQUE_VISUAL;
	bool gotFirst = false;
	
	// The visual style of this stack is opaque if any children are
	// opaque, otherwise translucent if any children are translucent,
	// and finally transparent.
	for (int32 i=0;i<m_partCount;i++) {
		DecorPart* c = ChildAt(instance, i);
		if (c) {
			visual_style s = c->VisualStyle(instance);
			if (!gotFirst) {
				gotFirst = true;
				style = s;
			} else if (s == B_OPAQUE_VISUAL || s == B_TRANSLUCENT_VISUAL) {
				style = s;
			}
			if (style == B_OPAQUE_VISUAL)
				break;
		}
	}
	
	return style;
}

uint32 DecorStack::CacheLimits(DecorState *instance)
{
	DecorPart *child;
	LimitsStruct lim;
	LimitsStruct tot;
	uint32 single,r=0;

	tot.widths[MIN_LIMIT] = tot.heights[MIN_LIMIT] = 0;
	tot.widths[MAX_LIMIT] = tot.heights[MAX_LIMIT] = 0x7FFF;
	tot.widths[PREF_LIMIT] = tot.heights[PREF_LIMIT] = NO_LIMIT;
	for (int32 i=0;i<m_partCount;i++) {
		child = ChildAt(instance,i);
		if (instance->BitMask()->test(m_bitmaskIndex+i)) {
			single = child->CacheLimits(instance);
			r |= single;
		};
		child->FetchLimits(instance,&lim);
		if (lim.widths[MIN_LIMIT] > tot.widths[MIN_LIMIT])
			tot.widths[MIN_LIMIT] = lim.widths[MIN_LIMIT];
		if (lim.heights[MIN_LIMIT] > tot.heights[MIN_LIMIT])
			tot.heights[MIN_LIMIT] = lim.heights[MIN_LIMIT];
		if ((lim.widths[MAX_LIMIT]) > 0 && (lim.widths[MAX_LIMIT] < tot.widths[MAX_LIMIT]))
			tot.widths[MAX_LIMIT] = lim.widths[MAX_LIMIT];
		if ((lim.heights[MAX_LIMIT]) > 0 && (lim.heights[MAX_LIMIT] < tot.heights[MAX_LIMIT]))
			tot.heights[MAX_LIMIT] = lim.heights[MAX_LIMIT];
		if (lim.widths[PREF_LIMIT] > tot.widths[PREF_LIMIT])
			tot.widths[PREF_LIMIT] = lim.widths[PREF_LIMIT];
		if (lim.heights[PREF_LIMIT] > tot.heights[PREF_LIMIT])
			tot.heights[PREF_LIMIT] = lim.heights[PREF_LIMIT];
	}
	if (tot.widths[MAX_LIMIT] < tot.widths[MIN_LIMIT]) tot.widths[MAX_LIMIT] = tot.widths[MIN_LIMIT];
	if (tot.heights[MAX_LIMIT] < tot.heights[MIN_LIMIT]) tot.heights[MAX_LIMIT] = tot.heights[MIN_LIMIT];
	if (tot.widths[PREF_LIMIT] > tot.widths[MAX_LIMIT]) tot.widths[PREF_LIMIT] = tot.widths[MAX_LIMIT];
	if (tot.heights[PREF_LIMIT] > tot.heights[MAX_LIMIT]) tot.heights[PREF_LIMIT] = tot.heights[MAX_LIMIT];
	if (tot.widths[MAX_LIMIT] == 0x7FFF) tot.widths[MAX_LIMIT] = INFINITE_LIMIT;
	if (tot.heights[MAX_LIMIT] == 0x7FFF) tot.heights[MAX_LIMIT] = INFINITE_LIMIT;
	
	tot.update(Limits());
	
	DecorStackVars *vars = (DecorStackVars*)instance->LimitCache()->Lookup(m_limitSpace);

	if (vars->lim != tot) {
		r |= LIMITS_NEED_LAYOUT;
		vars = (DecorStackVars*)instance->WritableLimitCache()->Lookup(m_limitSpace);
		vars->lim = tot;
	};

	return r;
}

void DecorStack::FetchLimits(
	DecorState *instance,
	LimitsStruct *lim)
{
	DecorStackVars *vars = (DecorStackVars*)instance->LimitCache()->Lookup(m_limitSpace);
	*lim = vars->lim;
};

uint32 DecorStack::Layout(
	CollectRegionStruct *collect,
	rect newBounds, rect oldBounds)
{
	DecorStackLayout layout[32];
	
	if (m_partCount > 32) {
		printf(	"Not enough slots for layout engine to work on %s!\n"
				"(Need %ld, but maximum is 32.)\n",
				Name(), m_partCount);
		return 0;
	}
	
	DecorState *changes = collect->instance;
	
	const bool moved = (newBounds.left != oldBounds.left)
						|| (newBounds.top != oldBounds.top);
	Bitmap *bitmask = changes->BitMask();

	// Retrieve previous child layout.
	const DecorStackLayout *vars;
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
		
		// Every child gets placed at the same position as the stack,
		// with gravity applied if the child width or height is larger
		// than its maximum dimensions.
		rect frame = newBounds;
		apply_gravity(&frame, l, c->Gravity());
		
		// Generate frame in stack's coordinate system.
		layout[i].frame = frame;
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
		DecorStackLayout *dest;
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

void DecorStack::InitLocal(DecorState *instance)
{
	DecorPart::InitLocal(instance);
}

void DecorStack::CollectRegion(CollectRegionStruct *collect, rect bounds)
{
	if (m_partCount > 32) return;
	
	// Stash away the current region and start a new one.
	region *savedRegion = collect->theRegion;
	collect->theRegion = newregion();

	// Retrieve current children's layout.
	const DecorStackLayout *vars;
	ReadLayout(collect->instance, m_layoutSpace, &vars);
	
	// For each child, position frame in the stack and add in its region.
	for (int32 i=0;i<m_partCount;i++) {
		rect frame = vars[i].frame;
		offset_rect(&frame, bounds.left, bounds.top);
		ChildAt(collect->instance,i)->CollectRegion(collect,frame);
	}

	// Apply collected regions from children to the current region.
	or_region(collect->theRegion,savedRegion,collect->tmpRegion[0]);
	kill_region(savedRegion);
	savedRegion = collect->tmpRegion[0];
	collect->tmpRegion[0] = collect->theRegion;
	collect->theRegion = savedRegion;
}

void DecorStack::Draw(
	DecorState *instance,
	rect r, DrawingContext context,
	region *update)
{
	if (m_partCount > 32) return;
	
	// If the stack's position does not intersect the update region,
	// then there's nothing to draw.  Also don't draw if we are not
	// in an update and this part has not been marked as needing a redraw.
	if ((update && !overlap_rect(update->Bounds(),r)) ||
		(!update && !instance->BitMask()->test(m_bitmaskIndex+m_partCount))) return;

	// Retrieve current children's layout.
	const DecorStackLayout *vars;
	ReadLayout(instance, m_layoutSpace, &vars);
	
	// For each child, redraw if there is an update region or it is
	// marked as needing a redraw.
	for (int32 i=0;i<m_partCount;i++) {
		rect frame = vars[i].frame;
		offset_rect(&frame, r.left, r.top);
		ChildAt(instance,i)->Draw(instance,frame,context,update);
	}
}

void DecorStack::DownPropagate(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags)
{
	for (int32 i=0;i<m_partCount;i++) {
		if (depFlags & dfRedraw)
			changes->BitMask()->set(m_bitmaskIndex+m_partCount);
		if (depFlags & dfLayout)
			changes->BitMask()->set(m_bitmaskIndex+i);
		DecorPart::DownPropagate(space,this,changes,depFlags);
	};
};

void DecorStack::PropogateChanges(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags,
	DecorAtom *newChoice)
{
	for (int32 i=0;i<m_partCount;i++) {
		if (m_partList[i] == instigator) {
			if (depFlags & dfChoice)
				depFlags = (dfRedraw|dfLayout);
			if (depFlags & dfRedraw)
				changes->BitMask()->set(m_bitmaskIndex+m_partCount);
			if (depFlags & dfLayout)
				changes->BitMask()->set(m_bitmaskIndex+i);
			DecorPart::PropogateChanges(space,instigator,changes,depFlags,NULL);
			if (newChoice) newChoice->Choose(space)->DownPropagate(space,this,changes,depFlags);
			else if (depFlags & dfRedraw) {
				for (int32 j=0;j<m_partCount;j++) {
					if (j!=i) m_partList[j]->Choose(space)->DownPropagate(space,this,changes,depFlags);
				};
			};
			return;
		};
	};
}

int32 DecorStack::HandleEvent(HandleEventStruct *handle)
{
	const DecorStackLayout *vars;
	ReadLayout(handle->state, m_layoutSpace, &vars);
	
	int32 ret = 0;
	int32 transit = handle->event->Transit();
	rect bounds = handle->bounds;
	point pt = handle->localPoint;
	bool dispatch;

	if (handle->event->what == B_MOUSE_MOVED) {
		for (int32 i=m_partCount-1;i>=0;i--) {
			dispatch = false;
			const bool hit = point_in_rect(&vars[i].frame, pt.h, pt.v);
			if (handle->mouseContainers->test(m_mouseContainers+i)) {
				if ((transit == B_EXITED) || (!hit) || (ret&EVENT_TERMINAL))
					handle->event->SetTransit(B_EXITED);
				else {
					handle->event->SetTransit(B_INSIDE);
					handle->newMouseContainers->set(m_mouseContainers+i);
				}
				dispatch = true;
			} else if ((transit != B_EXITED) && hit && !(ret&EVENT_TERMINAL)) {
				handle->newMouseContainers->set(m_mouseContainers+i);
				handle->event->SetTransit(B_ENTERED);
				dispatch = true;
			}
			
			if (dispatch) {
				handle->localPoint.h -= vars[i].frame.left;
				handle->localPoint.v -= vars[i].frame.top;
				// Hand off to child.  Note that we don't want to look at the
				// return result for B_EXITED transit, since it shouldn't cover
				// any underlying children.
				if (handle->event->Transit() != B_EXITED)
					ret |= ChildAt(handle->state,i)->HandleEvent(handle);
				else
					ChildAt(handle->state,i)->HandleEvent(handle);
				handle->event->SetTransit(transit);
				handle->localPoint = pt;
			}
		};
	} else {
		for (int32 i=m_partCount-1;i>=0 && !(ret&EVENT_TERMINAL);i--) {
			if (point_in_rect(&vars[i].frame, pt.h, pt.v)) {
				handle->localPoint.h -= vars[i].frame.left;
				handle->localPoint.v -= vars[i].frame.top;
				ret = ChildAt(handle->state,i)->HandleEvent(handle);
			};
		};
	};

	if (!(ret & EVENT_TERMINAL) && m_behavior) {
		DecorActionDef *action = ((DecorBehavior*)m_behavior->Choose(handle->state->Space()))->React(handle->state,handle->event);
		if (action) {
			action->Perform(handle->state,handle->executor);
			if (!(ret & EVENT_HANDLED)) handle->leaf = this;
			ret |= EVENT_HANDLED;
		}
	};

	handle->bounds = bounds;
	handle->localPoint = pt;
	return ret;
};

void DecorStack::Unflatten(DecorStream *f)
{
	DecorPart::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_bitmaskIndex);
	f->Read(&m_limitSpace);
	f->Read(&m_layoutSpace);
	f->Read(&m_mouseContainers);
	f->Read(&m_partCount);
	f->FinishReadBytes();
		
	grFree(m_partList); // mathias - 12/9/2000: don't leak our parts
	m_partList = (DecorAtom**)grMalloc(m_partCount*sizeof(DecorAtom*), "DecorStack::m_partList",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_partCount;i++) {
		f->StartReadBytes();
		f->Read(&m_partList[i]);
		f->FinishReadBytes();
	}
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	DecorStack::DecorStack(DecorTable *t) : DecorPart(t)
	{
		m_partCount = 0;
		m_partList = NULL;
	
		t = t->GetDecorTable("Parts");
		if (!t) {
			printf("Stack %s doesn't have any parts!\n", Name());
			return;
		};
		while (t->Has(m_partCount+1)) m_partCount++;
		m_partList = (DecorAtom**)grMalloc(m_partCount*sizeof(DecorAtom*), "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_partCount;i++) {
			m_partList[i] = t->GetAtom(i+1);
		};
		delete t;
	};
	
	void DecorStack::RegisterAtomTree()
	{
		DecorPart::RegisterAtomTree();
		for (int32 i=0;i<m_partCount;i++) {
			m_partList[i]->RegisterAtomTree();
			m_partList[i]->RegisterDependency(this,dfLayout|dfRedraw|dfChoice);
		};
	}
	
	DecorAtom * DecorStack::Reduce(int32 *proceed)
	{
		int32 subProceed;
		for (int32 i=0;i<m_partCount;i++) {
			subProceed = 1; while (subProceed) m_partList[i] = m_partList[i]->Reduce(&subProceed);
		};
		*proceed = 0;
		return this;
	}

	bool DecorStack::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorStack)) || DecorPart::IsTypeOf(t);
	}
	
	void DecorStack::AllocateLocal(DecorDef *decor)
	{
		m_bitmaskIndex = decor->AllocateBitmask(m_partCount + 1);
		m_limitSpace = decor->AllocateLimitCache(sizeof(DecorStackVars));
		m_layoutSpace = decor->AllocateLayout(m_partCount*sizeof(DecorStackLayout));
		m_mouseContainers = decor->AllocateMouseContainer(m_partCount);
	}

	void DecorStack::Flatten(DecorStream *f)
	{
		DecorPart::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_bitmaskIndex);
		f->Write(&m_limitSpace);
		f->Write(&m_layoutSpace);
		f->Write(&m_mouseContainers);
		f->Write(&m_partCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_partCount;i++) {
			f->StartWriteBytes();
			f->Write(m_partList[i]);
			f->FinishWriteBytes();
		}
	}
	
#endif
