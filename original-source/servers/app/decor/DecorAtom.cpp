
#include "DecorAtom.h"

#include <Message.h>

#include <stdio.h>

status_t add_decor_atom(BMessage& dest, const char* name, DecorAtom* atom)
{
	return dest.AddData(name, DECOR_ATOM_TYPE, &atom, sizeof(atom));
}

status_t find_decor_atom(const BMessage& src, const char* name, DecorAtom** atom)
{
	return find_decor_atom(src, name, 0, atom);
}

status_t find_decor_atom(const BMessage& src, const char* name, int32 index, DecorAtom** atom)
{
	const void* data;
	ssize_t size;
	const status_t result = src.FindData(name, DECOR_ATOM_TYPE, index, &data, &size);
	if (result >= B_OK && data != NULL && size == sizeof(DecorAtom*)) {
		*atom = *(DecorAtom**)data;
		return result;
	}
	
	*atom = NULL;
	return B_BAD_VALUE;
}

static uint32 gDecorPrintMode = 0;
uint32 DecorPrintMode() { return gDecorPrintMode; }
void SetDecorPrintMode(uint32 flags) { gDecorPrintMode = flags; }

DecorAtom::DecorAtom()
{
#if defined(DECOR_CLIENT)
	m_atomRegistered = false;
#endif
}

DecorAtom::~DecorAtom()
{
}

DecorAtom * DecorAtom::ChooseLocal(const VarSpace *space)
{
	return this;
}

void DecorAtom::Unflatten(DecorStream *f)
{
	f->StartReadBytes();
	f->FinishReadBytes();
}

#if defined(DECOR_CLIENT)

	#include "DecorDef.h"
	extern DecorDef *gCurrentDef;

	bool DecorAtom::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorAtom));
	}

	void DecorAtom::Flatten(DecorStream *f)
	{
		f->StartWriteBytes();
		f->FinishWriteBytes();
	}

	void DecorAtom::RegisterAtomTree()
	{
		if (!m_atomRegistered) {
			gCurrentDef->RegisterAtom(this);
			m_atomRegistered = true;
		};
	}

	DecorAtom * DecorAtom::Iterate(int32 *proceed)
	{
		*proceed = 0;
		return this;
	}
	
	DecorAtom * DecorAtom::CopySubTree(const std::type_info &leafType, AtomGenerator applyToLeaves, void *userData)
	{
		if (IsTypeOf(leafType)) return applyToLeaves(userData,this);
		return this;
	}
	
	bool DecorAtom::PrettyMuchEquivalentTo(DecorAtom *atom)
	{
		return false;
	}
	
	DecorAtom * DecorAtom::Reduce(int32 *proceed)
	{
		*proceed = 0;
		return this;
	}
	

#endif
