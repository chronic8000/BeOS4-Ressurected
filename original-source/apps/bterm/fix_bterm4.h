#include <sys/types.h>
#include <unistd.h>
#define write(fd, buf, count) ::write(fd, buf, count)
