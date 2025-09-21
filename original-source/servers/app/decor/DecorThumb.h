
#ifndef DECORTHUMB_H
#define DECORTHUMB_H

#include "DecorGroup.h"

/**************************************************************/

class DecorThumb : public DecorGroup
{
				DecorThumb&		operator=(DecorThumb&);
								DecorThumb(DecorThumb&);

		// used just for dragging
		int32	m_dragPosition;
		int32	m_thumbRange;
		int32	m_size;
		int32	m_slideArea;
	
	protected:
		DecorAtom *				m_slideAreaVar;
		DecorAtom *				m_range;
		DecorAtom *				m_value;
		DecorAtom *				m_prop;

	public:
								DecorThumb();
		virtual					~DecorThumb();

#if defined(DECOR_CLIENT)
								DecorThumb(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			RegisterAtomTree();
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	int32			HandleEvent(HandleEventStruct *handle);

				int32	 		GetValueFromSetPixel(int32);
				void	 		StartDragging(HandleEventStruct *handle);
				void	 		StopDragging();

		#define THISCLASS		DecorThumb
		#include				"DecorAtomInclude.h"
};

#endif
