
#ifndef DECORWIDGET_H
#define DECORWIDGET_H

#include "DecorImagePart.h"

/**************************************************************/

class DecorWidget : public DecorImagePart
{
				DecorWidget&	operator=(DecorWidget&);
								DecorWidget(DecorWidget&);
	
	protected:

		DecorAtom *				m_image;
		rect					m_imageRect;
		region *				m_subRegion;

	public:
								DecorWidget();
		virtual					~DecorWidget();

#if defined(DECOR_CLIENT)
								DecorWidget(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			RegisterAtomTree();
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits);
		virtual	void			GetImage(DecorState *instance, DecorImage **image);
		virtual	void			CollectRegion(CollectRegionStruct *collect, rect bounds);

		#define THISCLASS		DecorWidget
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
