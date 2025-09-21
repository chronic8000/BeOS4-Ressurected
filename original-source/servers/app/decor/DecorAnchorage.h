
#ifndef DECORANCHORAGE_H
#define DECORANCHORAGE_H

#include <SupportDefs.h>
#include "DecorPart.h"

/**************************************************************/

class DecorAnchorage : public DecorAtom
{
				DecorAnchorage&	operator=(DecorAnchorage&);
								DecorAnchorage(DecorAnchorage&);
				
	protected:
				DecorAtom *		m_anchoring;

				DecorAtom *		m_leftAnchor;
				DecorAtom *		m_topAnchor;
				DecorAtom *		m_rightAnchor;
				DecorAtom *		m_bottomAnchor;
				
				int32			m_leftOffset;
				int32			m_topOffset;
				int32			m_rightOffset;
				int32			m_bottomOffset;
				uint8			m_anchors;
				
				int32			m_layoutVars;
				int32			m_bitmask;
				int32			m_mouseContainment;

	public:
								DecorAnchorage();
		virtual					~DecorAnchorage();

#if defined(DECOR_CLIENT)
								DecorAnchorage(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual	void			RegisterAtomTree();
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	void			InitLocal(DecorState *instance);

				bool			NeedAdjustLimits(DecorState *instance);
				bool			NeedLayout(DecorState *instance);
				bool			NeedDraw(DecorState *instance);

		virtual void			PropogateChanges(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags,
									DecorAtom *newChoice=NULL);

				void			AdjustLimits(DecorState *instance,
									DecorVariableInteger *variable,
									int32 *min, int32 *max);
				void			AdjustLimits(DecorState *instance,
									int32 *minWidth, int32 *minHeight,
									int32 *maxWidth, int32 *maxHeight);
				void			CollectRegion(CollectRegionStruct *collect);
				uint32			Layout(CollectRegionStruct *collect);
				void			Draw(DecorState *instance, DrawingContext context, region *updateRegion);
				int32			HandleEvent(HandleEventStruct *handle);

		#define THISCLASS		DecorAnchorage
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif

