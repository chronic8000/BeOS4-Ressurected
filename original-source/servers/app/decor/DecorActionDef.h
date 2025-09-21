
#ifndef DECORACTIONDEF_H
#define DECORACTIONDEF_H

#include "DecorAtom.h"

/**************************************************************/

class BMessage;

class DecorTable;
class DecorActionDef : public DecorAtom {

				DecorActionDef&	operator=(DecorActionDef&);
								DecorActionDef(DecorActionDef&);
				
	protected:

				BMessage *		m_actions;
				int32			m_actionCount;

	public:

								DecorActionDef();
		virtual					~DecorActionDef();
		
#if defined(DECOR_CLIENT)
								DecorActionDef(DecorTable *t);
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			RegisterAtomTree();
		virtual void			Flatten(DecorStream *f);
#endif
		virtual	void			Unflatten(DecorStream *f);

				void			Perform(DecorState *changes, DecorExecutor *executor);
						
		#define THISCLASS		DecorActionDef
		#include				"DecorAtomInclude.h"
};

/**************************************************************/

#endif
