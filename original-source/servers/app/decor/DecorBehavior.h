
#ifndef DECORBEHAVIOR_H
#define DECORBEHAVIOR_H

#include "DecorSharedAtom.h"

/**************************************************************/

struct Instinct;

class DecorTable;
class DecorActionDef;
class DecorBehavior : public DecorSharedAtom {

				DecorBehavior&		operator=(DecorBehavior&);
									DecorBehavior(DecorBehavior&);
	protected:

				Instinct *			m_instincts;
				int32				m_instinctCount;

	public:

									DecorBehavior();
		virtual						~DecorBehavior();

#if defined(DECOR_CLIENT)
									DecorBehavior(DecorTable *t);
		virtual	bool				IsTypeOf(const std::type_info &t);
		virtual void				Flatten(DecorStream *f);
		virtual	void				RegisterAtomTree();
#endif
		virtual	void				Unflatten(DecorStream *f);

		virtual	DecorActionDef *	React(DecorState *changes, DecorMessage *event);

		#define THISCLASS			DecorBehavior
		#include					"DecorAtomInclude.h"
};

/**************************************************************/

#endif
