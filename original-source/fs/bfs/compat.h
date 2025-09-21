//depot/main/src/fs/bfs/compat.h#17 - edit change 18335 (text)
#ifndef _FAKE_STUFF_H
#define _FAKE_STUFF_H

#ifndef REAL_LONG_LONG
#define REAL_LONG_LONG 1
#endif

#ifdef REAL_LONG_LONG
typedef long long dr9_off_t;
typedef long long dr9_ino_t;
#else
typedef long dr9_off_t;
typedef long dr9_ino_t;
#endif

#include <dirent.h>

#ifndef __BEOS__

#ifndef S_IUMSK
#define S_IUMSK 07777
#endif

#ifndef B_OS_NAME_LENGTH
#define B_OS_NAME_LENGTH  32           /* XXXdbg -- revisit these two */
#endif  /* B_OS_NAME_LENGTH */


#ifndef B_FILE_NAME_LENGTH
#define B_FILE_NAME_LENGTH  64        /* XXXdbg -- revisit these two */
#endif  /* B_FILE_NAME_LENGTH */

#ifndef B_ERROR
#define B_ERROR EINVAL
#endif

/* Data types for indices and such */
#define B_INT32_TYPE        1
#define B_UINT32_TYPE       2
#define B_INT64_TYPE        3
#define B_UINT64_TYPE       4
#define B_FLOAT_TYPE        5
#define B_DOUBLE_TYPE       6
#define B_STRING_TYPE       7
#define B_ASCII_TYPE        8
#define B_MIME_STRING_TYPE  9

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define 	S_ATTR_DIR    		01000000000
#define 	S_ATTR        		02000000000
#define 	S_INDEX_DIR   		04000000000
#define 	S_STR_INDEX   		00100000000
#define 	S_INT_INDEX   		00200000000
#define 	S_UINT_INDEX   		00400000000
#define 	S_LONG_LONG_INDEX   00010000000
#define 	S_ULONG_LONG_INDEX  00020000000
#define 	S_FLOAT_INDEX 		00040000000
#define 	S_DOUBLE_INDEX 		00001000000
#define 	S_ALLOW_DUPS  		00002000000

#define     S_ISINDEX(m)  (((m) & S_INDEX_DIR) == S_INDEX_DIR)
#define     st_crtime st_ctime

typedef long sem_id;
typedef unsigned char uchar;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
typedef unsigned int port_id;
typedef int bool;
typedef int image_id;
typedef long long bigtime_t;

extern sem_id   create_sem(long count, const char *name);
extern long     delete_sem(sem_id sem);
extern long     acquire_sem(sem_id sem);
extern long		acquire_sem_etc(sem_id sem, int count, int flags,
								bigtime_t microsecond_timeout);
extern long		release_sem(sem_id sem);
extern long		release_sem_etc(sem_id sem, long count, long flags);

extern long atomic_add(long *value, long addvalue);
extern bigtime_t	system_time(void);

#include "fs_info.h"
#include "fs_index.h"
#include "fs_attr.h"
#include "TypeConstants.h"

#else

#include <AppDefs.h>
#include <OS.h>
#include <TypeConstants.h>
#include <sys/stat.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_attr.h>

#endif /* __BEOS__ */


#ifdef USER
ssize_t read_pos(int fd, dr9_off_t pos, void *data,  size_t nbytes);
ssize_t write_pos(int fd, dr9_off_t pos, const void *data,  size_t nbytes);
#endif /* USER */



#endif /* _FAKE_STUFF_H */
