
#include <string.h>
#include <stdlib.h>
#include "binder_defs.h"
#include "binder_driver.h"
#include "binder_thread.h"
#include "binder_team.h"

static int binder_debug(int argc, char **argv);

inline void* thread_to_cookie(binder_thread* t)
{
	return reinterpret_cast<void*>(reinterpret_cast<uint32>(t) - 0x80000000);
}

inline binder_thread* cookie_to_thread(void* c)
{
	return reinterpret_cast<binder_thread*>(reinterpret_cast<uint32>(c) + 0x80000000);
}

extern "C" {
	status_t init_driver(void);
	void uninit_driver(void);
	device_hooks *find_device(const char *name);
	const char **publish_devices();
}

status_t
init_driver(void)
{
	status_t err;
	ddprintf(("binder -- init_driver\n"));
	if ((err = init_binder())) goto err1;
	if ((err = add_debugger_command("binder", binder_debug, "dump binder info"))) goto err2;

	return B_OK;

//err3:
	remove_debugger_command("binder", binder_debug);
err2:
	uninit_binder();
err1:
	ddprintf(("binder -- init_driver: failed, %s\n", strerror(err)));
	return err;
}

void
uninit_driver(void)
{
	ddprintf(("binder -- uninit_driver\n"));
	remove_debugger_command("binder", binder_debug);
	uninit_binder();
	ddprintf(("binder -- uninit_driver: done\n"));
}	

status_t
binder_open_etc(int32 teamID, void **cookie)
{
	binder_team *team = get_team(teamID);
	if (team) {
		*cookie = thread_to_cookie(team->OpenThread(find_thread(NULL)));
		team->Release(SECONDARY);
		return B_OK;
	}
	return B_ERROR;
}

status_t
binder_open(const char *, uint32 , void **cookie)
{
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	return binder_open_etc(ti.team,cookie);
}

status_t
binder_close(void *cookie)
{
	binder_thread *t = cookie_to_thread(cookie);
	printf("*** CLOSING DEVICE FOR THREAD %ld!  Refs remaining: (%ld,%ld)\n",
			t->Thid(), t->PrimaryRefCount(), t->SecondaryRefCount());
	t->Release(PRIMARY);
	return B_OK;
}

status_t
binder_free(void *)
{
	return B_OK;
}

status_t
binder_control(void *cookie, uint32 op, void *data, size_t len)
{
	binder_thread *t = cookie_to_thread(cookie);
	return t->Control(op,data,len);
}

status_t
binder_read(void *cookie, off_t , void *data, size_t *numBytes)
{
	binder_thread *t = cookie_to_thread(cookie);
	int32 amt = t->Read(data,*numBytes);
	if (amt < 0) {
		*numBytes = 0;
		return amt;
	}
	
	*numBytes = amt;
	return B_OK;
}

status_t
binder_write(void *cookie, off_t , const void *data, size_t *numBytes)
{
	binder_thread *t = cookie_to_thread(cookie);
	int32 amt = t->Write(const_cast<void*>(data),*numBytes);
	if (amt < 0) {
		*numBytes = 0;
		return amt;
	}
	
	*numBytes = amt;
	return B_OK;
}

const char **
publish_devices()
{
	static const char *device_list[] = {
		"misc/binder",
		NULL
	};
	ddprintf(("binder -- publish_devices\n"));
	return device_list;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;

device_hooks *
find_device(const char *name)
{
	static device_hooks binder_device_hooks = {
		binder_open,
		binder_close,
		binder_free,
		binder_control,
		binder_read,
		binder_write,
		NULL,		/* select */
		NULL,		/* deselect */
		NULL,		/* readv */
		NULL		/* writev */
	};
	ddprintf(("binder -- find_device: name == %s\n", name));
	return &binder_device_hooks;
}

// #pragma mark -

static int
binder_debug(int , char **)
{
	return 0;
}

#if BINDER_DEBUG_LIB

// put a static object here that will call init_binder
class call_init_binder {
	public: call_init_binder() { init_binder(); };
};
call_init_binder _call_init_binder;

int open_binder(int teamID)
{
	int desc,err;
	err = binder_open_etc(teamID,(void**)&desc);
	if (err < 0) return err;
	return desc;
}

status_t close_binder(int desc)
{
	return binder_close((void*)desc);
}

status_t ioctl_binder(int desc, int cmd, void *data, int32 len)
{
	return binder_control((void*)desc,cmd,data,len);
}

ssize_t read_binder(int desc, void *data, size_t numBytes)
{
	int err = binder_read((void*)desc,0,data,&numBytes);
	if (err < 0) return err;
	return numBytes;
}

ssize_t write_binder(int desc, void *data, size_t numBytes)
{
	int err = binder_write((void*)desc,0,data,&numBytes);
	if (err < 0) return err;
	return numBytes;
}

#endif
