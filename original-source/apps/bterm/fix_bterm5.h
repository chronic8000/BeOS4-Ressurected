#include <sys/types.h>
#include <unistd.h>
inline ssize_t beos_write(int fd, const void *buf, size_t count) {
    return ::write(fd, buf, count);
}
#define write beos_write
