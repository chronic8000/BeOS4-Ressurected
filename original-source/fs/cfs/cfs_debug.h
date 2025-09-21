#ifndef CFS_DEBUG_H
#define CFS_DEBUG_H

#include <KernelExport.h>

#define CFS_DEBUG_LEVEL 4

#if CFS_DEBUG_LEVEL == 5

#define PRINT_FLOW(x) dprintf x
#define PRINT_ERROR(x) dprintf x
#define PRINT_WARNING(x) dprintf x
#define PRINT_INFO(x) dprintf x
#define PRINT_INTERNAL_ERROR(x) dprintf x

#elif CFS_DEBUG_LEVEL == 4

#define PRINT_FLOW(x)
#define PRINT_ERROR(x) dprintf x
#define PRINT_WARNING(x) dprintf x
#define PRINT_INFO(x) dprintf x
#define PRINT_INTERNAL_ERROR(x) dprintf x

#elif CFS_DEBUG_LEVEL == 3

#define PRINT_FLOW(x)
#define PRINT_ERROR(x) dprintf x
#define PRINT_WARNING(x) dprintf x
#define PRINT_INFO(x)
#define PRINT_INTERNAL_ERROR(x) dprintf x

#elif CFS_DEBUG_LEVEL == 2

#define PRINT_FLOW(x)
#define PRINT_ERROR(x) dprintf x
#define PRINT_WARNING(x)
#define PRINT_INFO(x)
#define PRINT_INTERNAL_ERROR(x)

#else

#define PRINT_FLOW(x)
#define PRINT_ERROR(x)
#define PRINT_WARNING(x)
#define PRINT_INFO(x)
#define PRINT_INTERNAL_ERROR(x)

#endif

#endif

