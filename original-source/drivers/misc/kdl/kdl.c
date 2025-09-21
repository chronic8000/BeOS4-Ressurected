// /dev/misc/kdl -- force a drop into KDL

#include <Drivers.h>
#include <KernelExport.h>

#define DEVICE_NAME "misc/kdl"
#define TOUCH(x) ((void)x)

// local hook implementations
static status_t
kdebug_open(const char* name, uint32 mode, void** cookie)
{
	TOUCH(name); TOUCH(mode);
	*cookie = NULL;
	return B_OK;
}

static status_t
kdebug_close(void *cookie)
{
	TOUCH(cookie);
	return B_OK;
}

static status_t
kdebug_free(void *cookie)
{
	return B_OK;
}

static status_t
kdebug_ioctl(void *cookie, uint32 op, void *data, size_t len)
{
	TOUCH(cookie); TOUCH(op); TOUCH(data); TOUCH(len);
	return B_OK;
}

static status_t
kdebug_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	TOUCH(cookie); TOUCH(pos); TOUCH(buffer); TOUCH(len);
	return B_IO_ERROR;
}

static status_t
kdebug_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	TOUCH(cookie); TOUCH(pos); TOUCH(len);
	kernel_debugger(buffer);
	return B_OK;
}

// global entry points

status_t init_hardware()
{
	return B_OK;
}

const char **publish_devices()
{
	static const char *devices[] = {
		DEVICE_NAME, NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	static device_hooks hooks = {
		&kdebug_open,
		&kdebug_close,
		&kdebug_free,
		&kdebug_ioctl,
		&kdebug_read,
		&kdebug_write,
		/* Leave select/deselect/readv/writev undefined. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}

status_t init_driver()
{
	return B_OK;
}

void uninit_driver()
{
}
