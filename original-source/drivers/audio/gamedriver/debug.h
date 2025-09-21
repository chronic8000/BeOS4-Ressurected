// Copyright 2001, Be Incorporated. All Rights Reserved 
// This file may be used under the terms of the Be Sample Code License 
#if !defined(debug_h)
#define debug_h

#include <KernelExport.h>
#include <Drivers.h>
#include <stdio.h>


#if !defined(DEBUG)
 #define DEBUG 0
#endif
#if !defined(DEBUG_LOCKS)
 #define DEBUG_LOCKS 0
#endif

#define eprintf(x) dprintf x

#if defined(ASSERT)
#undef ASSERT
#endif
#if DEBUG
 extern int g_debug;
 #define ddprintf(x) if (g_debug < 1) ; else dprintf x
 #define d2printf(x) if (g_debug < 2) ; else dprintf x
inline static void _assert_fail(const char * cond, const char * file, int line) {
	char str[200];
	sprintf(str, "ASSERT failed: %s file %s line %d\n", cond, file, line);
	set_dprintf_enabled(true);
	kernel_debugger(str);
}
 #define ASSERT(x) if (!(x)) _assert_fail(#x, __FILE__, __LINE__); else (void)0
#else
 #define ddprintf(x) (void)0
 #define d2printf(x) (void)0
 #define ASSERT(x) (void)0
#endif

#if DEBUG
 #define LOCKVAL find_thread(NULL)
 #define ASSERT_LOCKED(ap) if ((ap)->locked != find_thread(NULL)) _assert_fail("ASSERT_LOCKED", __FILE__, __LINE__); else (void)0
 #define CHECK_NO_LOCK(x) if ((x) == find_thread(NULL)) kernel_debugger("\033[34mCHECK_NO_LOCK failed\033[0m\n"); else 
#else
 #define LOCKVAL true
 #define ASSERT_LOCKED(ap) (void)0
 #define CHECK_NO_LOCK(x)
#endif

#if DEBUG_LOCKS && DEBUG
 #define DEBUG_ARGS __FILE__, __LINE__
 #define DEBUG_ARGS2 , __FILE__, __LINE__
 #define DEBUG_ARGS_DECL const char * file, int line
 #define DEBUG_ARGS_DECL2 , const char * file, int line
 #define lddprintf(x) dprintf x
#else
 #define DEBUG_ARGS
 #define DEBUG_ARGS2
 #define DEBUG_ARGS_DECL
 #define DEBUG_ARGS_DECL2
 #define lddprintf(x) (void)0
#endif

//	My apologies for the error codes.

#define B_DONT_DO_THAT B_BAD_VALUE
#define B_SHOULDNT_HAVE_DONE_THAT B_ERROR
#define B_OH_OH B_NO_MEMORY
#define B_SHUCKS EBADF
#define B_WHATEVER B_OK
#define B_IM_SORRY_DAVE B_DEV_INVALID_IOCTL
#define B_BLAME_THE_HARDWARE ENOSYS
#define B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT EPERM
#define B_IM_WAY_AHEAD_OF_YOU EBUSY
#define B_WE_CANT_GO_ON_LIKE_THIS B_MISMATCHED_VALUES

#endif	//	debug_h
