
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

#ifndef ATOM_H
#define ATOM_H

#include <Atom.h>

#ifndef GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif 

#include "as_debug.h"

class SAtom : public BAtom
#if AS_DEBUG
 , public DebugNode
#endif
{

private:

		int32	refCount;
		int32	clientRefCount;

public:

						SAtom() { }
	virtual				~SAtom();
		
		#if AS_DEBUG
		virtual void	DumpDebugInfo(DebugInfo *);
		virtual	int32	ClientLock()		{ return BAtom::Acquire(); }
		virtual	int32	ClientUnlock()		{ return BAtom::Release(); }
		virtual	int32	ServerLock()		{ return BAtom::IncRefs(); }
		virtual	int32	ServerUnlock()		{ return BAtom::DecRefs(); }
		#else
		inline	int32	ClientLock()		{ return BAtom::Acquire(); }
		inline	int32	ClientUnlock()		{ return BAtom::Release(); }
		inline	int32	ServerLock()		{ return BAtom::IncRefs(); }
		inline	int32	ServerUnlock()		{ return BAtom::DecRefs(); }
		#endif
				void	ClientUnref();

		inline	int32	ClientRefs()		{ return BAtom::AcquireCount(); };
		inline	int32	ServerRefs()		{ return BAtom::IncRefsCount(); };

};

extern SAtom *	grab_atom_at(SAtom **atom);
extern void 	set_atom_at(SAtom **atom, SAtom *setTo);

#endif
