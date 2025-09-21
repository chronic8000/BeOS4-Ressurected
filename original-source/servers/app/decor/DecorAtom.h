	
#ifndef DECORATOM_H
#define DECORATOM_H

#include <SupportDefs.h>
#include "DecorTypes.h"

class BMessage;
class DecorAtom;

enum {
	DECOR_ATOM_TYPE			= 'dcra'
};

status_t add_decor_atom(BMessage& dest, const char* name, DecorAtom* atom);
status_t find_decor_atom(const BMessage& src, const char* name, DecorAtom** atom);
status_t find_decor_atom(const BMessage& src, const char* name, int32 index, DecorAtom** atom);

enum {
	DECOR_PRINT_TREE	= 0x0001,
	DECOR_PRINT_STATS	= 0x0002
};
uint32 DecorPrintMode();
void SetDecorPrintMode(uint32 flags);

/**************************************************************/

#define dfLayout		0x00000001
#define dfRedraw		0x00000002
#define dfChoice		0x00000004
#define dfVariable		0x00000008
#define dfEverything	0xFFFFFFFF

typedef DecorAtom * (*AtomGenerator)(void *userData, DecorAtom *leaf);

class DecorAtom {

#if defined(DECOR_CLIENT)
				bool			m_atomRegistered;
#endif

	public:

								DecorAtom();
		virtual					~DecorAtom();

#if defined(DECOR_CLIENT)
		virtual	bool			IsTypeOf(const std::type_info &t);
		virtual	void			AllocateLocal(DecorDef *decor) {};
		virtual void			Flatten(DecorStream *f);
		virtual void			RegisterDependency(DecorAtom *dependant, uint32 depFlags) {};
		virtual void			RegisterAtomTree();
		virtual	bool			PrettyMuchEquivalentTo(DecorAtom *atom);
		virtual	DecorAtom *		Reduce(int32 *proceed);
		virtual DecorAtom *		CopySubTree(const std::type_info &leafType, AtomGenerator applyToLeaves, void *userData);
		virtual	DecorAtom *		Iterate(int32 *proceed);
#endif
		virtual	void			Unflatten(DecorStream *f);

		virtual	void			InitLocal(DecorState *instance) {};

		virtual	bool			IsVariable() const { return false; }
		
		virtual void			DownPropagate(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags) {};

		virtual void			PropogateChanges(
									const VarSpace *space,
									DecorAtom *instigator,
									DecorState *changes,
									uint32 depFlags,
									DecorAtom *newChoice=NULL) {};

		virtual	DecorAtom *		ChooseLocal(const VarSpace *space);
				DecorAtom *		Choose(const VarSpace *space) {
									DecorAtom *lastA=this,*a = ChooseLocal(space);
									while (a != lastA) {
										lastA = a;
										a = a->ChooseLocal(space);
									};
									return a;
								};
};

/**************************************************************/

#endif
