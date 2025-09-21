#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
typedef int ssize_t;
class _CEventPort_;
#define B_MAX_CPU_COUNT 8

// Pre-declare to avoid template compilation
template <class TYPE> struct atomref;
template <class TYPE> inline status_t FindAtomRef(const char*, atomref<TYPE>&) { return 0; }
#define SKIP_BROKEN_TEMPLATE 1
