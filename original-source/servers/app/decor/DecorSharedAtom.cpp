
#include <stdio.h>

#include "as_support.h"
#include "DecorSharedAtom.h"

DecorSharedAtom::DecorSharedAtom()
{
	m_parentCount = 0;
	m_parents = NULL;
}

DecorSharedAtom::~DecorSharedAtom()
{
	if (m_parents) grFree(m_parents);
}

void DecorSharedAtom::Unflatten(DecorStream *f)
{
	f->StartReadBytes();
	f->Read(&m_parentCount);
	f->FinishReadBytes();
	
	m_parents = (ParentRec*)grMalloc(m_parentCount*sizeof(ParentRec), "decor",MALLOC_CANNOT_FAIL);
	for (int32 i=0;i<m_parentCount;i++) {
		f->StartReadBytes();
		f->Read(&m_parents[i].parent);
		f->Read(&m_parents[i].deps);
		f->FinishReadBytes();
	}
}

void DecorSharedAtom::PropogateChanges(
	const VarSpace *space,
	DecorAtom *instigator,
	DecorState *changes,
	uint32 depFlags,
	DecorAtom *newChoice)
{
	for (int32 i=0;i<m_parentCount;i++) {
		if (depFlags & m_parents[i].deps)
			m_parents[i].parent->PropogateChanges(space,this,changes,depFlags & m_parents[i].deps,newChoice);
	};
}

#if defined(DECOR_CLIENT)

	void DecorSharedAtom::Flatten(DecorStream *f)
	{
		f->StartWriteBytes();
		f->Write(&m_parentCount);
		f->FinishWriteBytes();
		
		for (int32 i=0;i<m_parentCount;i++) {
			f->StartWriteBytes();
			f->Write(m_parents[i].parent);
			f->Write(&m_parents[i].deps);
			f->FinishWriteBytes();
		}
	}
	
	bool DecorSharedAtom::IsTypeOf(const std::type_info &t)
	{
		return (t == typeid(DecorAtom)) || DecorAtom::IsTypeOf(t);
	}
	
	void DecorSharedAtom::RegisterDependency(DecorAtom *dependant, uint32 depFlags)
	{
		DecorAtom::RegisterDependency(dependant,depFlags);
		m_parents = (ParentRec*)grRealloc(m_parents,(m_parentCount+1)*sizeof(ParentRec), "decor",MALLOC_CANNOT_FAIL);
		m_parents[m_parentCount].parent = dependant;
		m_parents[m_parentCount].deps = depFlags;
		m_parentCount++;
	}

#endif
