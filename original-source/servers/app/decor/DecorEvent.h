
#ifndef DECOREVENT_H
#define DECOREVENT_H

#include "DecorAtom.h"

/**************************************************************/

class DecorTable;
class DecorActionDef;
class DecorEvent : public DecorAtom {

				DecorEvent&			operator=(DecorEvent&);
									DecorEvent(DecorEvent&);
	
	protected:

				DecorAtom *			m_actionDef;
				uint32				m_flags;

	public:

									DecorEvent();
		virtual						~DecorEvent();

#if defined(DECOR_CLIENT)
									DecorEvent(DecorTable *t);
		virtual	bool				IsTypeOf(const std::type_info &t);
		virtual void				Flatten(DecorStream *f);
		virtual	void				RegisterAtomTree();
#endif
		virtual	void				Unflatten(DecorStream *f);

				uint32				Trigger(DecorState *state, DecorExecutor *executor);

		#define THISCLASS			DecorEvent
		#include					"DecorAtomInclude.h"
};

/**************************************************************/

#endif
