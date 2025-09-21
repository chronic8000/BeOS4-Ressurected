
#ifndef DECORFILLER_H
#define DECORFILLER_H

#include "DecorPart.h"

/**************************************************************/

class DecorFiller : public DecorPart
{
				DecorFiller&	operator=(DecorFiller&);
								DecorFiller(DecorFiller&);
	
	public:
								DecorFiller();
		virtual					~DecorFiller();

#if defined(DECOR_CLIENT)
								DecorFiller(DecorTable *table);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	visual_style	VisualStyle(DecorState *instance) { return B_TRANSPARENT_VISUAL; }
		
		virtual	uint32			Layout(	CollectRegionStruct *collect,
										rect newBounds, rect oldBounds);
		virtual	void			FetchLimits(DecorState *changes,
											LimitsStruct *limits);

		#define THISCLASS		DecorFiller
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
