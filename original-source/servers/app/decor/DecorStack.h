
#ifndef DECORSTACK_H
#define DECORSTACK_H

#include "DecorPart.h"
#include "DecorState.h"

/**************************************************************/

class DecorStack : public DecorPart
{
		int32					m_partCount;
		DecorAtom **			m_partList;
		int32					m_limitSpace;
		int32					m_layoutSpace;
		int32					m_mouseContainers;
		int32					m_bitmaskIndex;

				DecorStack&		operator=(DecorStack&);
								DecorStack(DecorStack&);
	
	public:
								DecorStack();
		virtual					~DecorStack();

#if defined(DECOR_CLIENT)
								DecorStack(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual void			Flatten(DecorStream *f);
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual	void			RegisterAtomTree();
#endif
		virtual	void			Unflatten(DecorStream *f);

		inline	DecorPart *		ChildAt(DecorState *instance, int32 index);

		virtual	void			InitLocal(DecorState *instance);

		virtual	visual_style	VisualStyle(DecorState *instance);

		virtual	void			CollectRegion(CollectRegionStruct *collect, rect bounds);
		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	uint32			CacheLimits(DecorState *changes);
		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits);
		virtual	void			Draw(	DecorState *instance,
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

		#define THISCLASS		DecorStack
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

DecorPart * DecorStack::ChildAt(DecorState *instance, int32 index)
{
	return (DecorPart*)m_partList[index]->Choose(instance->Space());
};

#endif
