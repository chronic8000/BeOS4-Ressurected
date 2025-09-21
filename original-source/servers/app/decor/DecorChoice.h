
#ifndef DECORCHOICE_H
#define DECORCHOICE_H

#include "DecorSharedAtom.h"

/**************************************************************/

struct Decision;
class DecorTable;

class DecorChoiceBase : public DecorSharedAtom {

	protected:

				DecorAtom *			m_variable;
				DecorAtom *			m_default;
				Decision *			m_decisions;
				int32				m_decisionCount;

				DecorChoiceBase&	operator=(DecorChoiceBase&);
									DecorChoiceBase(DecorChoiceBase&);
	
	public:

									DecorChoiceBase();
									DecorChoiceBase(DecorAtom *var, Decision *decisionList,
										int32 count, DecorAtom *def);
		virtual						~DecorChoiceBase();

#if defined(DECOR_CLIENT)
		virtual	bool				IsTypeOf(const std::type_info &t);
		virtual void				RegisterAtomTree();
		virtual void				Flatten(DecorStream *f);
		virtual DecorAtom *			Reduce(int32 *proceed);
		virtual	DecorChoiceBase *	Generate(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def) { return NULL; };
		virtual DecorAtom *			CopySubTree(const std::type_info &leafType, AtomGenerator applyToLeaves, void *userData);
#endif
		virtual	void				Unflatten(DecorStream *f);

		virtual void				PropogateChanges(
										const VarSpace *space,
										DecorAtom *instigator,
										DecorState *changes,
										uint32 depFlags,
										DecorAtom *newChoice=NULL);
};

#if defined(DECOR_CLIENT)

class DecorChoice : public DecorChoiceBase {

				int32			m_varType;
				
				DecorChoice&	operator=(DecorChoice&);
								DecorChoice(DecorChoice&);

	public:

								DecorChoice();
								DecorChoice(DecorTable *t);
		virtual	DecorAtom *		Iterate(int32 *proceed);
		virtual					~DecorChoice();

	#define THISCLASS	DecorChoice
	#define 			INTERMEDIARY_CLASS
	#include			"DecorAtomInclude.h"
};

#endif

class DecorStringChoice : public DecorChoiceBase {

		DecorStringChoice&		operator=(DecorStringChoice&);
								DecorStringChoice(DecorStringChoice&);
	
	public:
								DecorStringChoice();
								DecorStringChoice(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def);

#if defined(DECOR_CLIENT)
	virtual	DecorChoiceBase *	Generate(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def);
#endif

	virtual	DecorAtom * 		ChooseLocal(const VarSpace *space);

	#define THISCLASS			DecorStringChoice
	#define 					INTERNAL_CLASS
	#include					"DecorAtomInclude.h"
};

class DecorIntChoice : public DecorChoiceBase {

		DecorIntChoice&			operator=(DecorIntChoice&);
								DecorIntChoice(DecorIntChoice&);

	public:
								DecorIntChoice();
								DecorIntChoice(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def);

#if defined(DECOR_CLIENT)
	virtual	DecorChoiceBase *	Generate(DecorAtom *var, Decision *decisionList, int32 count, DecorAtom *def);
#endif

	virtual	DecorAtom * 		ChooseLocal(const VarSpace *space);

	#define THISCLASS			DecorIntChoice
	#define 					INTERNAL_CLASS
	#include					"DecorAtomInclude.h"
};

/**************************************************************/

#endif
