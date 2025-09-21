#include <sys/types.h>
#include <unistd.h>
extern "C" {
ssize_t write(int fd, const void *buf, size_t count);
}
