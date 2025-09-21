
#include <stdio.h>

#include "as_support.h"
#include "enums.h"
#include "rect.h"
#include "DecorDef.h"
#include "DecorGroup.h"
#include "DecorBehavior.h"
#include "DecorActionDef.h"
#include "DecorState.h"

DecorGroup::DecorGroup()
	: m_partList(NULL)
{
}

DecorGroup::~DecorGroup()
{
	grFree(m_partList); // mathias - 12/9/2000: Don't leak our parts!
}

visual_style DecorGroup::VisualStyle(DecorState *instance)
{
	visual_style style = B_OPAQUE_VISUAL;

	for (int32 i=0;i<m_partCount;i++) {
		DecorPart* c = ChildAt(instance, i);
		if (c) {
			visual_style s = c->VisualStyle(instance);
			if (s > style) style = s;
		}
	}
	
	return style;
}

uint32 DecorGroup::CacheLimits(DecorState *changes)
{
//	printf("CacheLimits for %s at %ld\n",m_name,m_bitmaskIndex);

	bool havePref = false;
	LimitsStruct lim;
	LimitsStruct tot;
	uint32 single,r = 0;
	int32 avg = 0;
	DecorPart *child;

	int16 *limW, *limH, *totW, *totH;
	if (m_orientation == GROUP_H) {
		limW = lim.widths; limH = lim.heights;
		totW = tot.widths; totH = tot.heights;
	} else {
		limW = lim.heights; limH = lim.widths;
		totW = tot.heights; totH = tot.widths;
	}
	
	totW[MIN_LIMIT] = totW[MAX_LIMIT] = 0;
	totH[MIN_LIMIT] = -1;
	totH[MAX_LIMIT] = 0x7FFF;
	totW[PREF_LIMIT] = totH[PREF_LIMIT] = 0;
	
	for (int32 i=0;i<m_partCount;i++) {
		child = ChildAt(changes,i);
		if (changes->BitMask()->test(m_bitmaskIndex+i)) {
			single = child->CacheLimits(changes);
//				if (!(single & LIMITS_NEED_LAYOUT))
//					changes->LayoutMask()->clear(m_bitmaskIndex+i+1);
			r |= single;
		};
		child->FetchLimits(changes,&lim);
		if (limW[PREF_LIMIT] < 0) limW[PREF_LIMIT] = limW[MIN_LIMIT];
		else havePref = true;
		totW[PREF_LIMIT] += limW[PREF_LIMIT];
		if (limH[PREF_LIMIT] >= 0) { totH[PREF_LIMIT] += limH[PREF_LIMIT]; avg++; };
		if (limW[MIN_LIMIT] > 0) totW[MIN_LIMIT] += limW[MIN_LIMIT];
		if (limW[MAX_LIMIT] == INFINITE_LIMIT) totW[MAX_LIMIT] = INFINITE_LIMIT;
		else if (totW[MAX_LIMIT] != INFINITE_LIMIT && limW[MAX_LIMIT] > 0)
			totW[MAX_LIMIT] += limW[MAX_LIMIT];
		if (limH[MIN_LIMIT] > totH[MIN_LIMIT]) totH[MIN_LIMIT] = limH[MIN_LIMIT];
		if ((limH[MAX_LIMIT] >= 0) && (limH[MAX_LIMIT] < totH[MAX_LIMIT]))
			totH[MAX_LIMIT] = limH[MAX_LIMIT];
	};
	if (totH[MAX_LIMIT] == 0x7FFF) totH[MAX_LIMIT] = INFINITE_LIMIT;
	if (avg) totH[PREF_LIMIT] /= avg;
	else totH[PREF_LIMIT] = NO_LIMIT;
	if (!havePref) totW[PREF_LIMIT] = NO_LIMIT;
	
	tot.update(Limits());
	
	DecorGroupLimits *vars = (DecorGroupLimits*)changes->LimitCache()->Lookup(m_limitSpace);

	if (vars->lim != tot) {
		r |= LIMITS_NEED_LAYOUT;
		vars = (DecorGroupLimits*)changes->WritableLimitCache()->Lookup(m_limitSpace);
		vars->lim = tot;
	};

	return r;
}

void DecorGroup::FetchLimits(
	DecorState *changes,
	LimitsStruct *lim)
{
	DecorGroupLimits *vars = (DecorGroupLimits*)changes->LimitCache()->Lookup(m_limitSpace);
	*lim = vars->lim;
};

int32 DecorGroup::HandleEvent(HandleEventStruct *handle)
{
	const DecorGroupLayout *vars;
	ReadLayout(handle->state, m_layoutSpace, &vars);
	
	int32 ret = 0;
	int32 transit = handle->event->Transit();
	rect bounds = handle->bounds;
	point pt = handle->localPoint;
	bool dispatch;

	if (handle->event->what == B_MOUSE_MOVED) {
//		printf("%s dispatching ...\n",Name());
		for (int32 i=0;i<m_partCount;i++) {
			dispatch = false;
			const bool hit = point_in_rect(&vars[i].frame, pt.h, pt.v);
			if (handle->mouseContainers->test(m_mouseContainers+i)) {
				if ((transit == B_EXITED) || (!hit))
					handle->event->SetTransit(B_EXITED);
				else {
					handle->event->SetTransit(B_INSIDE);
					handle->newMouseContainers->set(m_mouseContainers+i);
				}
				dispatch = true;
			} else if ((transit != B_EXITED) && hit) {
				handle->newMouseContainers->set(m_mouseContainers+i);
				handle->event->SetTransit(B_ENTERED);
				dispatch = true;
			}
/*			
			if (dispatch) {
				printf("%s dispatching to %ld:%s as %s\n",Name(),i,
					ChildAt(handle->changes,i)->Name(),
					(handle->event->Transit() == B_EXITED)?"B_EXITED":
					(handle->event->Transit() == B_ENTERED)?"B_ENTERED":
					(handle->event->Transit() == B_INSIDE)?"B_INSIDE":
					(handle->event->Transit() == B_OUTSIDE)?"B_OUTSIDE":
					"unknown!"
					);
			};
*/			
			if (dispatch) {
				handle->localPoint.h -= vars[i].frame.left;
				handle->localPoint.v -= vars[i].frame.top;
				ret |= ChildAt(handle->state,i)->HandleEvent(handle);
				handle->event->SetTransit(transit);
				handle->localPoint = pt;
			}
		};
	} else {
		for (int32 i=0;i<m_partCount;i++) {
			if (point_in_rect(&vars[i].frame, pt.h, pt.v)) {
/*
				printf("%s dispatching to %ld:%s as %s\n",Name(),i,
					ChildAt(handle->state,i)->Name(),
					(handle->event->Transit() == B_EXITED)?"B_EXITED":
					(handle->event->Transit() == B_ENTERED)?"B_ENTERED":
					(handle->event->Transit() == B_INSIDE)?"B_INSIDE":
					(handle->event->Transit() == B_OUTSIDE)?"B_OUTSIDE":
					"unknown!"
					);
*/
				handle->localPoint.h -= vars[i].frame.left;
				handle->localPoint.v -= vars[i].frame.top;
				ret = ChildAt(handle->state,i)->HandleEvent(handle);
				
//+				printf("ret = %08x\n",ret);
				break;
			};
		};
	};

	if (!(ret & EVENT_TERMINAL) && m_behavior) {
		DecorActionDef *action = ((DecorBehavior*)m_behavior->Choose(handle->state->Space()))->React(handle->state,handle->event);
		if (!(ret & EVENT_HANDLED)) handle->leaf = this;
		if (action) action->Perform(handle->state,handle->executor);
		ret |= EVENT_HANDLED;
	};

	handle->bounds = bounds;
	handle->localPoint = pt;
	return ret;
}

uint32 DecorGroup::Layout(
	CollectRegionStruct *collect,
	rect group, rect oldBounds)
{
	int32 total=0,need,inc;
	int32 *thisOne,*nextOne;
	int32 min[32][2],max[32][2],pref[32][2],index;
	int32 parts[32];
	DecorGroupLayout layout[32];
	point corner, oldCorner;
	bool boundsChanged;

	DecorState *changes = collect->instance;
	const DecorGroupLayout *vars;
	ReadLayout(changes, m_layoutSpace, &vars);

	if (m_partCount > 32) {
		printf(	"Not enough slots for layout engine to work on %s!\n"
				"(Need %ld, but maximum is 32.)\n",
				Name(), m_partCount);
		return 0;
	}
	
//	printf("Laying out %s within %d,%d,%d,%d\n",m_name,group.left,group.top,group.right,group.bottom);

	corner.h = group.left;
	corner.v = group.top;
	oldCorner.h = oldBounds.left;
	oldCorner.v = oldBounds.top;
	
	if (m_orientation == GROUP_H) {
		thisOne = &group.left;
		nextOne = &group.right;
		need = group.right - group.left + 1;
		index = 0;
	} else {
		thisOne = &group.top;
		nextOne = &group.bottom;
		need = group.bottom - group.top + 1;
		index = 1;
	};
	
	for (int32 i=0;i<m_partCount;i++) {
//		printf("limits %08x ... ",tmpLimits);
		LimitsStruct lim;
		ChildAt(changes,i)->FetchLimits(changes, &lim);
		min[i][0] = lim.widths[MIN_LIMIT];
		min[i][1] = lim.heights[MIN_LIMIT];
		max[i][0] = lim.widths[MAX_LIMIT];
		max[i][1] = lim.heights[MAX_LIMIT];
		pref[i][0] = lim.widths[PREF_LIMIT];
		pref[i][1] = lim.heights[PREF_LIMIT];
		if (pref[i][index] >= 0)
			parts[i] = pref[i][index];
		else
			parts[i] = min[i][index];
		total += parts[i];
//		printf("pos[%d]=%d\n",i,(*positions)[i]);
	};

//	printf("%s: total=%d, need=%d\n",m_name,total,need);

	if (total < need) {
		for (int32 i=0;i<m_partCount;i++) {
			inc = max[i][index];
			if (inc == INFINITE_LIMIT) {
				inc = need-total;
			} else {
				if (inc < 0) inc = 0;
				inc = inc - parts[i];
				if (inc < 0) inc = 0;
				if ((total+inc) > need) inc = need-total;
			};
			parts[i] += inc;
			total += inc;
			if (total == need) break;
		};
	} else if (total > need) {
		for (int32 i=0;i<m_partCount;i++) {
			inc = min[i][index];
			inc = parts[i] - inc;
			if (inc < 0) inc = 0;
			if ((total-inc) < need) inc = total-need;
			parts[i] -= inc;
			total -= inc;
			if (total == need) break;
		};
	};
/*
	if (total != need) {
		printf("Warning! Unable to satisfy dependencies!\n");
	};
*/
	region *savedRegion = collect->theRegion;
	collect->theRegion = newregion();

	Bitmap *bitmask = changes->BitMask();
	uint32 single,r = 0;
	bool changed = false;
	for (int32 i=0;i<m_partCount;i++) {
		*nextOne = *thisOne + parts[i] - 1;
		rect oldFrame = vars[i].frame;
		offset_rect(&oldFrame, oldCorner.h, oldCorner.v);
		boundsChanged = !equal_rect(&group,&oldFrame);
		if (boundsChanged || bitmask->test(m_bitmaskIndex+i)) {
			single = ChildAt(changes,i)->Layout(collect,group,oldFrame);
			if (single & LAYOUT_NEED_REDRAW) bitmask->set(m_bitmaskIndex+m_partCount+i);
			r |= single;
		}
		layout[i].frame = group;
		offset_rect(&layout[i].frame, -corner.h, -corner.v);
		changed = changed || (!equal_rect(&layout[i].frame,&vars[i].frame));
		*thisOne += parts[i];
	};
	
	if (r & LAYOUT_NEED_REDRAW) bitmask->set(m_bitmaskIndex+m_partCount*2);

	if (changed) {
		DecorGroupLayout *dest;
		WriteLayout(collect->instance, m_layoutSpace, &dest);
		for (int32 i=0;i<m_partCount;i++)
			dest[i] = layout[i];
	};

	or_region(collect->theRegion,savedRegion,collect->tmpRegion[0]);
	kill_region(savedRegion);
	savedRegion = collect->tmpRegion[0];
	collect->tmpRegion[0] = collect->theRegion;
	collect->theRegion = savedRegion;
	return r;
}

void DecorGroup::InitLocal(DecorState *instance)
{
	DecorPart::InitLocal(instance);
//	DecorGroupVars *vars = (DecorGroupVars*)instance->LimitCacheAt(m_localVars);
}

extern void ShowRegionReal(region *r, char *s);

void DecorGroup::CollectRegion(CollectRegionStruct *collect, rect bounds)
{
	if (m_partCount > 32) return;
	
	region *savedRegion = collect->theRegion;
	collect->theRegion = newregion();

	const DecorGroupLayout *vars;
	ReadLayout(collect->instance, m_layoutSpace, &vars);

	for (int32 i=0;i<m_partCount;i++) {
		rect frame = vars[i].frame;
		offset_rect(&frame, bounds.left, bounds.top);
		ChildAt(collect->instance,i)->CollectRegion(collect,frame);
	};

	or_region(collect->theRegion,savedRegion,collect->tmpRegion[0]);
	kill_region(savedRegion);
	savedRegion = collect->tmpRegion[0];
	collect->tmpRegion[0] = collect->theRegion;
	collect->theRegion = savedRegion;
}

void DecorGroup::Draw(
	DecorState *instance,
	rect r, DrawingContext context,
	region *update)
{
	if (m_partCount > 32) return;
	
	if (update && !overlap_rect(update->Bounds(),r)) return;

	const DecorGroupLayout *vars;
	ReadLayout(instance, m_layoutSpace, &vars);
	Bitmap *bitmask = instance->BitMask();

	for (int32 i=0;i<m_partCount;i++) {
		if (update || bitmask->test(m_bitmaskIndex+m_partCount+i)) {
			rect frame = vars[i].frame;
			offset_rect(&frame, r.left, r.top);
			ChildAt(instance,i)->Draw(instance,frame,context,update);
		}
	};
}

void DecorGroup::DownPropagate(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags)
{
	for (int32 i=0;i<m_partCount;i++) {
		if (depFlags & dfRedraw) {
			changes->BitMask()->set(m_bitmaskIndex+m_partCount*2);
			changes->BitMask()->set(m_bitmaskIndex+m_partCount+i);
		}
		if (depFlags & dfLayout)
			changes->BitMask()->set(m_bitmaskIndex+i);
		m_partList[i]->DownPropagate(space,this,changes,depFlags);
	};
};

void DecorGroup::PropogateChanges(
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
			if (depFlags & dfRedraw) {
				changes->BitMask()->set(m_bitmaskIndex+m_partCount*2);
				changes->BitMask()->set(m_bitmaskIndex+m_partCount+i);
			}
			if (depFlags & dfLayout)
				changes->BitMask()->set(m_bitmaskIndex+i);
			DecorPart::PropogateChanges(space,instigator,changes,depFlags,NULL);
			if (newChoice) newChoice->Choose(space)->DownPropagate(space,this,changes,depFlags);
			break;
		};
	};
}

void DecorGroup::Unflatten(DecorStream *f)
{
	DecorPart::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_bitmaskIndex);
	f->Read(&m_orientation);
	f->Read(&m_limitSpace);
	f->Read(&m_layoutSpace);
	f->Read(&m_mouseContainers);
	f->Read(&m_partCount);
	f->FinishReadBytes();
	
	grFree(m_partList); // mathias - 12/9/2000: Don't leak our parts!
	m_partList = (DecorAtom**)grMalloc(m_partCount*sizeof(DecorAtom*), "DecorGroup::m_partList",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_partCount;i++) {
		f->StartReadBytes();
		f->Read(&m_partList[i]);
		f->FinishReadBytes();
	}
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	
	DecorGroup::DecorGroup(DecorTable *t) : DecorPart(t)
	{
		m_partCount = 0;
	
		const char * orient = t->GetString("Orientation");
		if (!orient) {
			printf("Group %s doesn't have an orientation specified!\n", Name());
		};
		if (!strcmp(orient,"vertical")) m_orientation = GROUP_V;
		else m_orientation = GROUP_H;
	
		t = t->GetDecorTable("Parts");
		if (!t) panic(("Group doesn't have any parts!\n"));
	
		while (t->Has(m_partCount+1)) m_partCount++;
		m_partList = (DecorAtom**)grMalloc(m_partCount*sizeof(DecorAtom*), "decor",MALLOC_CANNOT_FAIL);
		for (int32 i=0;i<m_partCount;i++) {
			m_partList[i] = t->GetPart(i+1);
		};
		delete t;
	}
	
	DecorAtom * DecorGroup::Reduce(int32 *proceed)
	{
		int32 subProceed;
		for (int32 i=0;i<m_partCount;i++) {
			subProceed = 1; while (subProceed) m_partList[i] = m_partList[i]->Reduce(&subProceed);
		};
		*proceed = 0;
		return this;
	}

	void DecorGroup::RegisterAtomTree()
	{
		DecorPart::RegisterAtomTree();
		for (int32 i=0;i<m_partCount;i++) {
			m_partList[i]->RegisterAtomTree();
			m_partList[i]->RegisterDependency(this,dfLayout|dfChoice|dfRedraw);
		};
	}
	
	bool DecorGroup::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorGroup)) || DecorPart::IsTypeOf(t);
	}
	
	void DecorGroup::AllocateLocal(DecorDef *decor)
	{
		m_bitmaskIndex = decor->AllocateBitmask(m_partCount*2 + 1);
		m_limitSpace = decor->AllocateLimitCache(sizeof(DecorGroupLimits));
		m_layoutSpace = decor->AllocateLayout(m_partCount*sizeof(DecorGroupLayout));
		m_mouseContainers = decor->AllocateMouseContainer(m_partCount);
	}

	void DecorGroup::Flatten(DecorStream *f)
	{
		DecorPart::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(&m_bitmaskIndex);
		f->Write(&m_orientation);
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
