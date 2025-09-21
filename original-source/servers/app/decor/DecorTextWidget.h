
#ifndef DECORTEXTWIDGET_H
#define DECORTEXTWIDGET_H

/**************************************************************/

#include <GraphicsDefs.h>
#include "DecorPart.h"

class DecorTextWidget : public DecorPart
{
		DecorAtom *				m_font;
		DecorVariableString	*	m_text;
		DecorAtom *				m_color;
		point					m_offset;
		int32					m_localVars;

				DecorTextWidget&	operator=(DecorTextWidget&);
								DecorTextWidget(DecorTextWidget&);
	
	public:
								DecorTextWidget();
		virtual					~DecorTextWidget();

#if defined(DECOR_CLIENT)
								DecorTextWidget(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor);
		virtual	void			RegisterAtomTree();
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	void			Draw(DecorState *instance, rect r,
										DrawingContext context, region *update);
		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	uint32			CacheLimits(DecorState *instance);
		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits);

		#define THISCLASS		DecorTextWidget
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
