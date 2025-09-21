
//******************************************************************************
//
//	File:			system.h
//	Description:	Wrappers for system calls
//	Written by:		George Hoffman
//
//	Copyright 1998, Be Incorporated
//
//******************************************************************************/

#ifndef SYSTEM_H
#define SYSTEM_H

#ifndef _OS_H
#include <OS.h>
#endif

#include <TLS.h>
#include <image.h>
#include <IRegion.h>

enum {
	B_PROTOCOL = ECONNABORTED
};

/* Note that xspawn_thread does a resume_thread */

struct	RenderContext;
struct	RenderPort;

		thread_id			xspawn_thread(	thread_entry func, char *name, int32 priority, void *data, bool start=true);
		image_id			xload_add_on(char *path);
		status_t			xunload_add_on(image_id image);
		area_id 			xcreate_area(char *name, void **basePtr, uint32 flags,
								 uint32 size, uint32 locking, uint32 perms);
		status_t 			xresize_area( area_id areaToResize, uint32 newSize);
		status_t 			xdelete_area(area_id areaToDelete);
		void				send_into_debugger(team_id team);
		void				init_system();

		void				birth();
		void				death();
		void				suicide(int32 returnValue);
		
		void				ClipThreadPort(region *clip);

extern	int32 				gThreadSem;
extern	int32 				gThreadRenderContext;
extern	int32 				gThreadRenderPort;
extern	int32 				gThreadRenderInfo;

inline	RenderContext *		ThreadRenderContext() { return (RenderContext*)tls_get(gThreadRenderContext); };
inline	RenderPort *		ThreadRenderPort() { return (RenderPort*)tls_get(gThreadRenderPort); };


#ifdef __INTEL__

inline	int32		xgetpid() { return find_thread(NULL); };
		int32		atomic_set(int32 *addr, int32 value);

#ifndef NOINLINE
inline	int32		atomic_set(int32 *addr, int32 value) {
	int32 ret = value;
	__asm__ __volatile__ ( 
		"lock xchgl %%eax,(%%edx)\n\t"
		: "=a"(ret) : "0"(ret), "d"(addr) );
	return ret;
}
#endif

#else

int32		xgetpid_ppc();
#define 	xgetpid xgetpid_ppc

#endif

#endif
