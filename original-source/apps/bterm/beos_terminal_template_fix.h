#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
typedef int ssize_t;
class _CEventPort_;  // Forward declaration
#define B_MAX_CPU_COUNT 8

// Fix the BMessage template bug by providing a corrected version
template <class TYPE> inline status_t FindAtomRef_Fixed(const char *name, int32 index, void* obj);
#define FindAtomRef FindAtomRef_Fixed
