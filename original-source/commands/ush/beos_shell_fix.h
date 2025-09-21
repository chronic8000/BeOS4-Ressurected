#ifndef BEOS_SHELL_FIX_H
#define BEOS_SHELL_FIX_H

// Essential includes for BeOS shell
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>

// BeOS types for shell
typedef int32_t status_t;
typedef int32_t thread_id;
typedef int32_t team_id;
typedef int32_t area_id;
typedef int32_t port_id;
typedef int32_t sem_id;
typedef int64_t bigtime_t;
typedef int32_t image_id;
typedef int32_t int32;
typedef uint32_t uint32;

// BeOS constants
#define B_OK 0
#define B_ERROR -1
#define B_NO_ERROR 0
#define B_BAD_THREAD_ID -1
#define B_BAD_VALUE -2
#define B_STRING_TYPE 'CSTR'
#define MAXPATHLEN PATH_MAX

// Global variables
extern char **environ;

// Stub BeOS functions for shell (removed load_image_path to avoid conflict)
static inline status_t find_thread(const char* name) { return getpid(); }
static inline status_t kill_thread(thread_id thread) { return kill(thread, SIGTERM); }
static inline status_t resume_thread(thread_id thread) { return 0; }
static inline status_t suspend_thread(thread_id thread) { return 0; }
static inline bigtime_t system_time(void) { return 0; }
static inline void snooze(bigtime_t amount) { usleep(amount); }
static inline image_id load_image(int argc, const char* const* argv, const char* const* env) { return 1; }
static inline status_t wait_for_image(image_id image, status_t* returnCode) {
    if (returnCode) *returnCode = 0;
    return B_OK;
}
static inline status_t wait_for_thread(thread_id thread, status_t* returnCode) {
    if (returnCode) *returnCode = 0;
    return B_OK;
}
static inline status_t send_signal(thread_id thread, int signal) {
    return kill(thread, signal);
}

// Stub missing kernel functions
static inline image_id _kload_image_etc_(int argc, char **argv, char **envp, char *errbuf, int errbufsiz) {
    return 1; // Return fake image ID
}

// BeOS filesystem attribute stubs
typedef struct {
    uint32_t type;
    off_t size;
    char name[256];
} attr_info;

static inline ssize_t fs_read_attr(int fd, const char* attribute, uint32_t type, off_t pos, void* buffer, size_t readBytes) { return -1; }
static inline ssize_t fs_write_attr(int fd, const char* attribute, uint32_t type, off_t pos, const void* buffer, size_t writeBytes) { return -1; }
static inline int fs_remove_attr(int fd, const char* attribute) { return -1; }
static inline DIR* fs_fopen_attr_dir(int fd) { return NULL; }
static inline struct dirent* fs_read_attr_dir(DIR* dir) { return NULL; }
static inline int fs_close_attr_dir(DIR* dir) { return -1; }
static inline int fs_stat_attr(int fd, const char* attribute, attr_info* ai) { return -1; }

// System call stubs
static inline int _kshutdown_(int mode) { return system("shutdown -h now"); }

// Debug function stubs
#define PRINT(x) printf x
#define ASSERT(x) assert(x)

#endif // BEOS_SHELL_FIX_H
