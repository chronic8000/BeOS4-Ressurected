#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef __BEOS__
#include <Drivers.h>
#endif

#include "bfs.h"


#define BFS_BIG_ENDIAN       0x42494745    /* BIGE */
#define BFS_LITTLE_ENDIAN    0x45474942    /* EGIB */

int
my_byte_order(void)
{
	int  foo = BFS_BIG_ENDIAN;
	char *cp = (char *)&foo;

	if (*cp == 'B')
		return BFS_BIG_ENDIAN;
	else
		return BFS_LITTLE_ENDIAN;
}

int
check_byte_order(int disk_val)
{
	char *cp = (char *)&disk_val;

	if (*cp == 'B')
		return BFS_BIG_ENDIAN;
	else
		return BFS_LITTLE_ENDIAN;
}

int
device_is_read_only(const char *device)
{
#ifdef unix
	return 0;   /* XXXdbg should do an ioctl or something */
#else
	int             fd;
	device_geometry dg;

	fd = open(device, O_RDONLY);
	if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) {
		close(fd);
		return 0;
	}

	close(fd);

	return dg.read_only;
#endif
}

int
get_device_block_size(int fd)
{
#ifdef unix
	return 512;   /* XXXdbg should do an ioctl or something */
#else
	struct stat     st;
	device_geometry dg;

	if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) {
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
			return 0;

		return 512;   /* just assume it's a plain old file or something */
	}

	return dg.bytes_per_sector;
#endif
}

dr9_off_t
get_num_device_blocks(int fd)
{
#ifdef unix
	struct stat st;
	
	fstat(fd, &st);    /* XXXdbg should be an ioctl or something */

	return st.st_size / get_device_block_size(fd);
#else
	struct stat st;
	device_geometry dg;

	if (ioctl(fd, B_GET_GEOMETRY, &dg) >= 0) {
		return (dr9_off_t)dg.cylinder_count *
			   (dr9_off_t)dg.sectors_per_track *
			   (dr9_off_t)dg.head_count;
	}

	/* if the ioctl fails, try just stat'ing in case it's a regular file */
	if (fstat(fd, &st) < 0)
		return 0;

	return st.st_size / get_device_block_size(fd);
#endif
}

int
device_is_removeable(int fd)
{
#ifdef unix
	return 0;   /* XXXdbg should do an ioctl or something */
#else
	struct stat     st;
	device_geometry dg;

	if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) {
		return 0;
	}

	return dg.removable;
#endif
}

#if defined(__BEOS__) && !defined(USER)
#include "scsi.h"
#endif

int
lock_removeable_device(int fd, bool on_or_off)
{
#if defined(unix) || defined(USER)
	return 0;   /* XXXdbg should do an ioctl or something */
#else
	return ioctl(fd, B_SCSI_PREVENT_ALLOW, &on_or_off);
#endif
}




#ifdef USER 

int 
has_signals_pending(void *junk)
{
   return 0;
}


#ifndef __BEOS__
ssize_t
read_pos(int fd, dr9_off_t _pos, void *data,  size_t nbytes)
{
	off_t  pos = (off_t)_pos;
	size_t ret;
	
    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return EINVAL;
    }
    
    ret = read(fd, data, nbytes);

    if (ret != nbytes) {
        printf("read_pos: wanted %d, got %d\n", nbytes, ret);
        return -1;
    }

	return ret;
}

ssize_t
write_pos(int fd, dr9_off_t _pos, const void *data,  size_t nbytes)
{
	off_t  pos = (off_t)_pos;
	size_t ret;
	
    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("read lseek");
        return EINVAL;
    }
    
    ret = write(fd, data, nbytes);

    if (ret != nbytes) {
        printf("write_pos: wanted %d, got %d\n", nbytes, ret);
        return -1;
    }

	return ret;
}
#endif /* __BEOS__ */


#include <stdarg.h>


void
panic(const char *format, ...)
{
    va_list     ap;
    
    va_start(ap, format);
    vfprintf(stderr, format, ap);
	va_end(ap);

    while (TRUE)
        ;
}


void
add_debugger_command()
{
}

void
kernel_debugger(char *str)
{
  printf("\n");
  fflush(stdout);
  fflush(stderr);

  while(1) {
	  printf("we're dead: ");
	  fflush(stdout);
	  
	  getc(stdin);
  }

}

ulong
parse_expression(char *str)
{
	return strtoul(str, NULL, 0);
}


#include "lock.h"

int
new_lock(lock *l, const char *name)
{
	l->c = 1;
	l->s = create_sem(0, (char *)name);
	if (l->s <= 0)
		return l->s;
	return 0;
}

int
free_lock(lock *l)
{
	delete_sem(l->s);

	return 0;
}

int
new_mlock(mlock *l, long c, const char *name)
{
	l->s = create_sem(c, (char *)name);
	if (l->s <= 0)
		return l->s;
	return 0;
}

int
free_mlock(mlock *l)
{
	delete_sem(l->s);

	return 0;
}
#endif /* USER */


#ifdef unix
bigtime_t
system_time(void)
{
  return (bigtime_t)time(NULL);
}
#endif
