
//******************************************************************************
//
//	File:			atom.h
//
//	Description:	Base class for resources that are shared between threads
//					(i.e. that need careful deletion practices)
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include "atom.h"
#include "system.h"
#include <TLS.h>
#include <Debug.h>

#if 0
SAtom * grab_atom_at(SAtom **atom)
{
	SAtom *r;
	uint32 *theAtom = (uint32*)atom;
	uint32 semID = (uint32)tls_get(gThreadSem), theValue;

	tryAgain:
	theValue = atomic_set((int32*)theAtom,semID);
	if (theValue && (theValue < 0x80000000)) {
		acquire_sem(theValue);
		goto tryAgain;
	};

	r = ((SAtom*)theValue);
	if (r) r->ServerLock();

	theValue = atomic_set((int32*)theAtom,theValue);
	if (theValue != semID)
		release_sem(semID);

	return r;
};

void set_atom_at(SAtom **atom, SAtom *setTo)
{
	SAtom *r;
	uint32 *theAtom = (uint32*)atom;
	uint32 semID = (uint32)tls_get(gThreadSem), theValue;

	if (setTo) setTo->ServerLock();

	tryAgain:
	theValue = atomic_set((int32*)theAtom,semID);
	if (theValue && (theValue < 0x80000000)) {
		acquire_sem(theValue);
		goto tryAgain;
	};

	r = ((SAtom*)theValue);
	if (r) r->ServerUnlock();

	theValue = atomic_set((int32*)theAtom,(uint32)setTo);
	if (theValue != semID)
		release_sem(semID);
};
#endif

SAtom::~SAtom()
{
};

void SAtom::ClientUnref()
{
	while (ClientUnlock() > 1) {};
};

#if AS_DEBUG
void SAtom::DumpDebugInfo(DebugInfo *)
{
	DebugPrintf(("SAtom(0x%08x): refCount = %d\n",this,refCount));
};
#endif

