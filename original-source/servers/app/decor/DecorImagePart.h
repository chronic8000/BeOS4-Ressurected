
#ifndef DECORIMAGEPART_H
#define DECORIMAGEPART_H

#include "DecorPart.h"

/**************************************************************/

#define RULE_X_SCALE	0x00000001
#define RULE_X_TILE		0x00000002
#define RULE_X			0x0000000F

#define RULE_Y_SCALE	0x00000010
#define RULE_Y_TILE		0x00000020
#define RULE_Y			0x000000F0

class DecorImagePart : public DecorPart
{
				DecorImagePart&	operator=(DecorImagePart&);
								DecorImagePart(DecorImagePart&);
	
	protected:
	
		uint32			m_rules;
		visual_style	m_visualStyle;

	public:
						DecorImagePart() {}
		virtual			~DecorImagePart();

#if defined(DECOR_CLIENT)
						DecorImagePart(DecorTable *table);
		virtual	bool	IsTypeOf(const std::type_info &t);
		virtual void	Flatten(DecorStream *f);
#endif
		virtual	void	Unflatten(DecorStream *f);

		virtual	visual_style	VisualStyle(DecorState *instance);
		
		virtual	void	GetImage(DecorState *instance, DecorImage **image) = 0;
		virtual	void	Draw(DecorState *instance, rect r,
							DrawingContext context, region *update);
		virtual	void	FetchLimits(DecorState *changes,
									LimitsStruct *limits);

		virtual void	PropogateChanges(
							const VarSpace *space,
							DecorAtom *instigator,
							DecorState *changes,
							uint32 depFlags,
							DecorAtom *newChoice=NULL);
};

/**************************************************************/

#endif
