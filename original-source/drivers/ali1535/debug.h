
#if !defined(debug_h)
#define debug_h

#include <KernelExport.h>
#include <Drivers.h>
#include <stdio.h>


extern void iad_dprintf(const char * fmt, ...);
//#define dprintf iad_dprintf

#if !defined(DEBUG)
 #define DEBUG 0
#endif
#if !defined(DEBUG_LOCKS)
 #define DEBUG_LOCKS 0
#endif

#define eprintf(x) do { dprintf("\033[31m"); dprintf x; dprintf("\033[0m"); } while (0)

#if defined(ASSERT)
#undef ASSERT
#endif
#if DEBUG
 #define ddprintf(x) dprintf x
inline static void _assert_fail(const char * cond, const char * file, int line) {
	char str[200];
	sprintf(str, "ASSERT failed: %s file %s line %d\n", cond, file, line);
	set_dprintf_enabled(true);
	kernel_debugger(str);
}
 #define ASSERT(x) if (!(x)) _assert_fail(#x, __FILE__, __LINE__); else (void)0
#else
 #define ddprintf(x) (void)0
 #define ASSERT(x) (void)0
#endif


#endif	//	debug_h
