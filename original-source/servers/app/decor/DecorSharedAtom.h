	
#ifndef DECORSHAREDATOM_H
#define DECORSHAREDATOM_H

#include <SupportDefs.h>
#include "DecorAtom.h"

/**************************************************************/

struct ParentRec {
	DecorAtom *parent;
	uint16 deps;
};

class DecorSharedAtom : public DecorAtom {

				DecorSharedAtom&	operator=(DecorSharedAtom&);
								DecorSharedAtom(DecorSharedAtom&);
	
	private:
	
				ParentRec *		m_parents;
				int32			m_parentCount;

	public:

								DecorSharedAtom();
		virtual					~DecorSharedAtom();

#if defined(DECOR_CLIENT)
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual void			Flatten(DecorStream *f);
		virtual void			RegisterDependency(DecorAtom *dependant, uint32 depFlags);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual void			PropogateChanges(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags,
									DecorAtom *newChoice=NULL);
};

/**************************************************************/

#endif

