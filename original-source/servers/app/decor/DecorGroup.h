
#ifndef DECORGROUP_H
#define DECORGROUP_H

#include "DecorPart.h"
#include "DecorState.h"

/**************************************************************/

#define GROUP_H		0x01
#define GROUP_V		0x02

class DecorGroup : public DecorPart
{
				DecorGroup&		operator=(DecorGroup&);
								DecorGroup(DecorGroup&);
	
	protected:

		struct DecorGroupLimits {
			LimitsStruct lim;
		};
		
		struct DecorGroupLayout {
			rect	frame;
		};

		uint8					m_orientation;
		int32					m_partCount;
		DecorAtom **			m_partList;
		int32					m_limitSpace;
		int32					m_layoutSpace;
		int32					m_bitmaskIndex;
		int32					m_mouseContainers;

	public:
								DecorGroup();
		virtual					~DecorGroup();

#if defined(DECOR_CLIENT)
								DecorGroup(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual	void			RegisterAtomTree();
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		inline	DecorPart *		ChildAt(DecorState *rec, int32 index);

		virtual	void			InitLocal(DecorState *instance);
		
		virtual	visual_style	VisualStyle(DecorState *instance);

		virtual	void			CollectRegion(CollectRegionStruct *collect, rect bounds);
		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	uint32			CacheLimits(DecorState *changes);
		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits);
		virtual	void			Draw(		DecorState *instance,
											rect bounds, DrawingContext context,
											region *updateRegion);
		virtual	int32			HandleEvent(HandleEventStruct *handle);

		virtual void			DownPropagate(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags);

		virtual void			PropogateChanges(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags,
									DecorAtom *newChoice=NULL);

		#define THISCLASS		DecorGroup
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

DecorPart * DecorGroup::ChildAt(DecorState *rec, int32 index)
{
	return (DecorPart*)m_partList[index]->Choose(rec->Space());
};

#endif
