#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "device_icons.c"

static status_t vd_open(const char *name, uint32 flags, void **cookie);
static status_t vd_free (void *cookie);
static status_t vd_close(void *cookie);
static status_t vd_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t vd_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t vd_write(void *cookie, off_t pos, const void *buf, size_t *count);

#define B_LARGE_ICON 32
#define B_MINI_ICON 16
#define MAXDEVS 4

#define ddprintf

typedef struct
{
	uint32 devnum;
	int blocksize;
	char *path;
} devinfo;


typedef struct __loopdev {
	int lock;
	int refcount;
	int configured;
	int fd;
	size_t size;
	size_t blocks;
	size_t blocksize;
} loopdev;

loopdev *loopdevs;

void create_loopdev_area(void)
{
	area_id area = find_area("loopdev_workspace");

	if(area == B_NAME_NOT_FOUND) {
		void* addr;
		area = create_area ("loopdev_workspace", &addr,
				B_ANY_KERNEL_ADDRESS, /* kernel team will own this area */
				4096, B_LAZY_LOCK, B_READ_AREA | B_WRITE_AREA);

        if((area==B_ERROR) || (area==B_NO_MEMORY) || (area==B_BAD_VALUE)) 
			addr = NULL;
			
		loopdevs = (loopdev *) addr;
		if(addr){
			char buf[128];
			int i;
			ddprintf("loopdev_workspace @ 0x%08x\n",addr);
			for(i=0;i<MAXDEVS;i++){
				sprintf(buf,"loopdev_sem_%d",i);
				loopdevs[i].lock = create_sem(1,buf);
				loopdevs[i].fd = -1;
				loopdevs[i].refcount = 0;
				loopdevs[i].configured = 0;
			}
		}
	} else {
		area_info info;
		get_area_info(area, &info);
		loopdevs = (loopdev *) info.address;
	}                                      
}
/*
uchar	icon_disk[B_LARGE_ICON * B_LARGE_ICON];
uchar	icon_disk_mini[B_MINI_ICON * B_MINI_ICON];
*/
static const char *vd_name[] = {
	"config/loopback",
	"disk/virtual/loopback/0",
	"disk/virtual/loopback/1",
	"disk/virtual/loopback/2",
	"disk/virtual/loopback/3",
	NULL
};


static device_hooks vd_devices = {
	vd_open, 			
	vd_close, 			
	vd_free,			
	vd_control, 		
	vd_read,			
	vd_write			 
};



_EXPORT
status_t init_hardware(void)
{
	ddprintf("init_hardware()\n");
        return B_OK;
}
                                    

_EXPORT
status_t
init_driver(void)
{
	ddprintf("loopback: %s %s, init_driver()\n", __DATE__, __TIME__);
	create_loopdev_area();
	return B_NO_ERROR;
}

_EXPORT
void
uninit_driver(void)
{
	ddprintf("loopback: uninit_driver()\n"); 
}

_EXPORT
const char**
publish_devices()
{
	ddprintf("loopback: publish_devices()\n"); 
	return vd_name;
}

_EXPORT
device_hooks*
find_device(const char* name)
{
	ddprintf("loopback: find_device()\n"); 
	return &vd_devices;
}


static status_t
vd_open(const char *dname, uint32 flags, void **cookie)
{
	loopdev *ld;
	int n;
	
	int fd;
	struct stat si;

	if(!strcmp(dname,"config/loopback")){
		*cookie = NULL;
		return B_NO_ERROR;
	}
	
	n = atoi(dname + strlen("disk/virtual/loopback/"));
	
	if(n < 0 || n > 4) return B_ERROR;

	ld = &(loopdevs[n]);
	acquire_sem(ld->lock);
	ld->refcount++;
	dprintf("loopback: open(%s) - refs=%d fd=%d\n", dname,ld->refcount,ld->fd); 	
	*cookie = (void *) ld;
	release_sem(ld->lock);
	return B_NO_ERROR;
}

 
static status_t
configure(loopdev *ld, devinfo *di)
{
	int fd;
	struct stat si;

    if(di->blocksize == 0) return B_OK;

	if((fd = open(di->path,O_RDWR)) < 0) {
		dprintf("loopback: can't open '%s'\n",di->path);
		return B_ERROR;
	}

	if(ioctl(fd, 10000, NULL, 0)) {
		dprintf("loopback: cannot disable cache...\n");
		return B_ERROR;
	}
	if(fstat(fd, &si)) {
		close(fd);
		return B_ERROR;
	}
	ld->fd = fd;
	ld->size = si.st_size;
	ld->blocksize = di->blocksize;
	ld->blocks = ld->size / ld->blocksize;
	ld->configured = 1;
	
	dprintf("loopback: open okay (%d blocks of %d bytes)\n",ld->blocks,
		ld->blocksize);
    return B_NO_ERROR;
}
 
 
static status_t
vd_free (void *cookie)
{
	ddprintf("loopback: free()\n"); 	
	return B_NO_ERROR;
}


static status_t
vd_close(void *cookie)
{
	loopdev *ld = (loopdev *) cookie;
	if(cookie){
		acquire_sem(ld->lock);
		ld->refcount --;
		dprintf("loopback: close() refs now = %d\n",ld->refcount); 
		release_sem(ld->lock);
		return B_NO_ERROR;
	}
}



static status_t
vd_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	loopdev *ld = (loopdev *) cookie;
	size_t len;
	status_t ret = B_NO_ERROR;

	if(!cookie) return B_ERROR;
	
	acquire_sem(ld->lock);	
	if(!ld->configured) {
		release_sem(ld->lock);
		return B_DEV_NO_MEDIA;
	}

	ddprintf("vd_read(%d,%d)\n",((int) pos),*count);	
	
	if(pos >= ld->size) {
		len = 0;
		ret = B_ERROR;
	} else {
		len = (pos+(*count) > ld->size) ? (ld->size - pos) : (*count);
   		ret = read_pos(ld->fd, pos, buf, len);

		if(ret >= 0) {
			*count = ret;
			ret = B_NO_ERROR;
		} else {
			*count = 0;
			ret = B_ERROR;
		}
	}
	release_sem(ld->lock);
	return ret;
}


static status_t
vd_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	loopdev *ld = (loopdev *) cookie;
	size_t len;
	status_t ret = B_NO_ERROR;

	if(!cookie) return B_ERROR;

	acquire_sem(ld->lock);	
	if(!ld->configured) {
		release_sem(ld->lock);
		return B_DEV_NO_MEDIA;
	}

	ddprintf("vd_write(%d,%d)\n",((int)pos),*count);	
	if(pos >= ld->size) {
		ret = B_ERROR;
		len = 0;
	} else {
		len = (pos+(*count) > ld->size) ? (ld->size - pos) : (*count);
   		ret = write_pos(ld->fd, pos, buf, len);

		if(ret >= 0) {
			*count = ret;
			ret = B_NO_ERROR;
		} else {
			*count = 0;
			ret = B_ERROR;
		}
	}
	release_sem(ld->lock);
	return ret;
}

static status_t
vd_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	loopdev *ld = (loopdev *) cookie;
	device_geometry	*dinfo;

	ddprintf("loopback: control(%d)\n", msg); 

	/* this is a config device if it has no cookie */
	if(!cookie) {
		if(msg == B_SET_DEVICE_SIZE) {
			status_t ret;
			devinfo *di = (devinfo *) arg1;
			
			if(di->devnum >= MAXDEVS) return B_ERROR;
			
			ld = &(loopdevs[di->devnum]);			
			acquire_sem(ld->lock);
			if(ld->refcount > 1) {
				ddprintf("loopback: setsize w/ refcount > 1\n");
			}
			if(ld->fd) {
				close(ld->fd);
				ld->fd = -1;
				ld->blocks = 0;
				ld->blocksize = 0;
				ld->configured = 0;
			}
			ddprintf("loopback: setsize name='%s' blocksize=%d\n",di->path,di->blocksize);
			ret = configure(ld,di);
			release_sem(ld->lock);
			return ret;
		} else {
			return B_DEV_INVALID_IOCTL;
		}
		
	}	
		
	acquire_sem(ld->lock);

	switch (msg) 
	{
	case B_GET_GEOMETRY:
		dinfo = (device_geometry *) arg1;
		dinfo->sectors_per_track = 1;		
		dinfo->cylinder_count    = ld->blocks;
		dinfo->head_count        = 1;
		
		dinfo->bytes_per_sector = ld->blocksize;
		dinfo->removable = TRUE;
		dinfo->read_only = FALSE;
		dinfo->device_type = B_DISK;
		dinfo->write_once = FALSE;	
	
		release_sem(ld->lock);	
		return B_NO_ERROR;
		
	case B_FORMAT_DEVICE:
		release_sem(ld->lock);	
		return B_NO_ERROR;
		
	case B_GET_DEVICE_SIZE:
		*(size_t*)arg1 = (ld->blocksize * ld->blocks);
		release_sem(ld->lock);	
		return B_NO_ERROR;

	case B_GET_MEDIA_STATUS:		
		*((status_t*)arg1) = ld->configured ? B_NO_ERROR : B_DEV_NO_MEDIA;
		release_sem(ld->lock);
		return B_NO_ERROR;
		  
	case B_GET_ICON:
		switch (((device_icon *)arg1)->icon_size)
		{
		case B_LARGE_ICON:
			memcpy(((device_icon *)arg1)->icon_data, icon_disk,
				B_LARGE_ICON * B_LARGE_ICON);
			break;
			
		case B_MINI_ICON:
			memcpy(((device_icon *)arg1)->icon_data, icon_disk_mini,
				B_MINI_ICON * B_MINI_ICON);
			break;
			
		default:
			release_sem(ld->lock);	
			return B_BAD_TYPE;
		}
		release_sem(ld->lock);	
		return B_NO_ERROR;

	default:
		release_sem(ld->lock);	
		return  B_DEV_INVALID_IOCTL;
	}
}



