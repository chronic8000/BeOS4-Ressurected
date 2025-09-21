
#include <stdio.h>

#include "rect.h"
#include "DecorDef.h"
#include "DecorPart.h"
#include "DecorBehavior.h"
#include "DecorActionDef.h"
#include "DecorState.h"

static struct { uint16 width, height; }
limit_flags[_NUM_LIMIT] = {
	{ 0x0001, 0x0010 },
	{ 0x0002, 0x0020 },
	{ 0x0004, 0x0040 }
};
	
const LimitsStruct DecorPart::no_limits = {
	{ NO_LIMIT, NO_LIMIT, NO_LIMIT },
	{ NO_LIMIT, NO_LIMIT, NO_LIMIT }
};

static void position_gravity(int32* lower, int32* upper, int32 maxSize, int32 flags)
{
	const int32 extra = (*upper-*lower+1)-maxSize;
	if (extra > 0) {
		switch (flags&B_NORTH_SOUTH) {
			case B_FILL_CENTER: {
				const int32 toph = extra/2;
				*lower += toph + (extra&1);
				*upper -= toph;
				break;
			}
			case B_NORTH: {
				*upper -= extra;
				break;
			}
			case B_SOUTH: {
				*lower += extra;
				break;
			}
		}
	}
}

void apply_gravity(rect* used, const LimitsStruct& limits, gravity g)
{
	if (limits.heights[MAX_LIMIT] >= 0)
		position_gravity(&used->top, &used->bottom, max_c(limits.heights[MIN_LIMIT], limits.heights[MAX_LIMIT]), g);
	if (limits.widths[MAX_LIMIT] >= 0)
		position_gravity(&used->left, &used->right, max_c(limits.widths[MIN_LIMIT], limits.widths[MAX_LIMIT]), g>>2);
}

void LimitsStruct::update(const LimitsStruct& o)
{
	for (int32 i=0; i<_NUM_LIMIT; i++) {
		if (o.widths[i] != NO_LIMIT) widths[i] = o.widths[i];
		if (o.heights[i] != NO_LIMIT) heights[i] = o.heights[i];
	}
}

void LimitsStruct::fillin(const LimitsStruct& o)
{
	for (int32 i=0; i<_NUM_LIMIT; i++) {
		if (widths[i] == NO_LIMIT) widths[i] = o.widths[i];
		if (heights[i] == NO_LIMIT) heights[i] = o.heights[i];
	}
}

DecorPart::DecorPart()
{
	m_limits = no_limits;
}

DecorPart::~DecorPart()
{
}

visual_style DecorPart::VisualStyle(DecorState *)
{
	return B_OPAQUE_VISUAL;
}

uint32 DecorPart::Layout(
	CollectRegionStruct *collect,
	rect newBounds, rect oldBounds)
{
	if (!empty_rect(oldBounds)) CollectRegion(collect,oldBounds);
	return LAYOUT_REGION_CHANGED|LAYOUT_NEED_REDRAW;
}

int32 DecorPart::HandleEvent(HandleEventStruct *handle)
{
	if (!m_behavior) return 0;
	handle->leaf = this;
	DecorActionDef *action = ((DecorBehavior*)m_behavior->Choose(handle->state->Space()))->React(handle->state,handle->event);
	if (action) action->Perform(handle->state,handle->executor);
	return EVENT_HANDLED|EVENT_TERMINAL;
}

void DecorPart::PropogateChanges(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags,
	DecorAtom *newChoice)
{
	if (m_parent && (m_parentDeps & depFlags))
		m_parent->PropogateChanges(space,this,changes,depFlags & m_parentDeps,NULL);
};

void DecorPart::Unflatten(DecorStream *f)
{
	DecorAtom::Unflatten(f);
	
	f->StartReadBytes();
	f->Read(&m_parent);
	f->Read(&m_behavior);
	f->Read(m_name);
	f->Read(&m_parentDeps);
	if (f->Read(&m_weight) < B_OK) m_weight = 1;
	if (f->Read(&m_gravity) < B_OK) m_gravity = B_FILL_ALL;
	
	m_limits = no_limits;
	uint16 which;
	if (f->Read(&which) >= B_OK) {
		for (int32 i=0; i<_NUM_LIMIT; i++) {
			if ((which&limit_flags[i].width)) {
				f->Read(&m_limits.widths[i]);
			}
			if ((which&limit_flags[i].height)) {
				f->Read(&m_limits.heights[i]);
			}
		}
	}
	f->FinishReadBytes();
}

/**************************************************************/

#if defined(DECOR_CLIENT)

	#include "LuaDecor.h"
	DecorPart::DecorPart(DecorTable *t)
	{
		const char *str = t->GetString("Name");
		if (str) strncpy(m_name,str,63);
		else m_name[0] = 0;
		
		m_parent = NULL;
		m_behavior = t->GetAtom("Behavior");
		if (m_behavior && (!m_behavior->IsTypeOf(typeid(DecorBehavior))))
			panic(("Field \"Behavior\" must be of type BEHAVIOR (duh!)\n"));
		if (t->Has("Weight")) m_weight = t->GetNumber("Weight");
		else m_weight = 1;
		if (t->Has("Gravity")) m_gravity = (uint8)t->GetNumber("Gravity");
		else m_gravity = B_FILL_ALL;
		
		m_limits = no_limits;
		if (t->Has("MinWidth")) {
			m_limits.widths[MIN_LIMIT] = (int32)t->GetNumber("MinWidth");
			//printf("MinWidth = %ld\n", m_limits.widths[MIN_LIMIT]);
		}
		if (t->Has("MaxWidth")) {
			m_limits.widths[MAX_LIMIT] = (int32)t->GetNumber("MaxWidth");
			//printf("MaxWidth = %ld\n", m_limits.widths[MAX_LIMIT]);
		}
		if (t->Has("MinHeight")) {
			m_limits.heights[MIN_LIMIT] = (int32)t->GetNumber("MinHeight");
			//printf("MinHeight = %ld\n", m_limits.heights[MIN_LIMIT]);
		}
		if (t->Has("MaxHeight")) {
			m_limits.heights[MAX_LIMIT] = (int32)t->GetNumber("MaxHeight");
			//printf("MaxHeight = %ld\n", m_limits.heights[MAX_LIMIT]);
		}
		if (t->Has("PrefWidth")) {
			m_limits.widths[PREF_LIMIT] = (int32)t->GetNumber("PrefWidth");
			//printf("PrefWidth = %ld\n", m_limits.widths[PREF_LIMIT]);
		}
		if (t->Has("PrefHeight")) {
			m_limits.heights[PREF_LIMIT] = (int32)t->GetNumber("PrefHeight");
			//printf("PrefHeight = %ld\n", m_limits.heights[PREF_LIMIT]);
		}
	}

	void DecorPart::RegisterAtomTree()
	{
		DecorAtom::RegisterAtomTree();
		if (m_behavior) {
			m_behavior->RegisterAtomTree();
			m_behavior->RegisterDependency(this,dfChoice);
		};
	}
	
	bool DecorPart::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorPart)) || DecorAtom::IsTypeOf(t);
	}

	void DecorPart::AllocateLocal(DecorDef *decor)
	{
		DecorAtom::AllocateLocal(decor);
	}

	void DecorPart::Flatten(DecorStream *f)
	{
		DecorAtom::Flatten(f);
		
		f->StartWriteBytes();
		f->Write(m_parent);
		f->Write(m_behavior);
		f->Write(m_name);
		f->Write(&m_parentDeps);
		f->Write(&m_weight);
		f->Write(&m_gravity);
		uint16 which = 0;
		for (int32 i=0; i<_NUM_LIMIT; i++) {
			if (m_limits.widths[i] != NO_LIMIT) which |= limit_flags[i].width;
			if (m_limits.heights[i] != NO_LIMIT) which |= limit_flags[i].height;
		}
		f->Write(&which);
		for (int32 i=0; i<_NUM_LIMIT; i++) {
			if (m_limits.widths[i] != NO_LIMIT) {
				f->Write(&m_limits.widths[i]);
			}
			if (m_limits.heights[i] != NO_LIMIT) {
				f->Write(&m_limits.heights[i]);
			}
		}
		f->FinishWriteBytes();
	}

	void 
	DecorPart::RegisterDependency(DecorAtom *dependant, uint32 depFlags)
	{
		if (m_parent) panic(("Decor parts cannot be shared!"));
		if (!dependant) panic(("Trying to set parent to NULL!"));
		DecorAtom::RegisterDependency(dependant,depFlags);
		m_parent = dependant;
		m_parentDeps = (uint16)depFlags;
	}
	
#endif
