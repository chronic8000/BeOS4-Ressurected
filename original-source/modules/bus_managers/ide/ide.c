/* IDE bus manager
*/

#include <IDE.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include "protocol.h"
#include <KernelExport.h>
#include <checkmalloc.h>
#include <string.h>

typedef struct {
	ide_bus_info *moduleinfo;
	uint32 cookie;
	bool first_bus_in_module;
	bool is_handled[2];
} ide_bus;

uint32 ide_bus_count;
uint32 ide_abs_bus_count;
static ide_bus *ide_busses;

static
status_t get_nth_ide_bus(uint32 bus, ide_bus_info **ide_bus, uint32 *cookie)
{
	if((bus < 0) || (bus >= ide_bus_count))
		return B_ERROR;
	
	*ide_bus = ide_busses[bus].moduleinfo;
	*cookie = ide_busses[bus].cookie;
	if(*ide_bus == NULL) {
		dprintf("IDE -- get_nth_ide_bus: bad module info\n");
		return B_ERROR;
	}
	return B_NO_ERROR;
}

static
uint32 get_ide_bus_count()
{
	return ide_bus_count;
}

static
status_t handle_device(uint32 bus, bool device)
{
	if((bus < 0) || (bus >= ide_bus_count))
		return B_ERROR;
	if(ide_busses[bus].is_handled[device])
		return B_ERROR;
	ide_busses[bus].is_handled[device] = true;
	return B_NO_ERROR;
}

static
status_t release_device(uint32 bus, bool device)
{
	if((bus < 0) || (bus >= ide_bus_count))
		return B_ERROR;
	ide_busses[bus].is_handled[device] = false;
	return B_NO_ERROR;
}

/*
** Init
*/

typedef struct tmpmod_s {
	struct tmpmod_s *next;
	ide_bus_info *info;
	int count;
} tmpmod;

int32 	max_abs_bus_num;
tmpmod *tmpmodlist = NULL;
tmpmod *tmpmodlistlast = NULL;

static status_t
try_module(char *path)
{
	status_t err;
	int j;
	tmpmod *tmp;
	
	if(strcmp(path+strlen(path)-5, "/v0.6") != 0) {
		dprintf("IDE -- try_module: %s is wrong version\n", path);
		return B_ERROR;
	}
	
	tmp = malloc(sizeof(tmpmod));
	if(tmp == NULL)
		return B_NO_MEMORY;
	err = get_module(path, (module_info **)&tmp->info);
	if (err != B_NO_ERROR) {
		//dprintf("IDE -- try_module: could not get module %s\n", path);
		free(tmp);
		return err;
	}
	//dprintf("IDE -- try_module: got module %s\n", path);
	tmp->count = tmp->info->get_bus_count();
	if(tmp->count < 1) {
		dprintf("IDE -- try_module: %s has no busses\n", path);
		put_module(path);
		free(tmp);
		return B_NO_ERROR;
	}
	for(j = 0; j < tmp->count; j++) {
		int32 abs_bus_num =
			tmp->info->get_abs_bus_num(tmp->info->get_nth_cookie(j));
		if(abs_bus_num >= 0)
			ide_abs_bus_count++;
		if(abs_bus_num > max_abs_bus_num)
			max_abs_bus_num = abs_bus_num;
	}
	//dprintf("IDE -- try_module: found module %s, handling %d bus%s\n",
	//        path, tmp->count, tmp->count != 1 ? "ses" : "");
			
	// todo: only add modules not already seen
	
	ide_bus_count += tmp->count;
	tmp->next = NULL;
	if(tmpmodlistlast)
		tmpmodlistlast->next = tmp;
	else
		tmpmodlist = tmp;
	tmpmodlistlast = tmp;

	return B_NO_ERROR;
}

static status_t
init_v06()
{
	void *list_handle;
#if _SUPPORTS_FEATURE_SHORT_PATH_MAX
	char buf[128];
#else
	char buf[MAXPATHLEN];
#endif
	size_t bufsize;
	status_t err;
	int i,j;
	
//	dprintf("IDE -- init\n");
	tmpmodlist = NULL;
	tmpmodlistlast = NULL;
	ide_bus_count = 0;
	ide_abs_bus_count = 0;
	max_abs_bus_num = -1;
	
	list_handle = open_module_list("busses/ide");
	while(bufsize = sizeof(buf),
	      read_next_module_name(list_handle,buf,&bufsize) == B_NO_ERROR) {
		err = try_module(buf);
		if(err == B_NO_MEMORY)
			goto err;
	}
	close_module_list(list_handle);
		
	if(ide_bus_count < 1) {
		dprintf("IDE -- init: no ide busses found\n");
		return B_ERROR;
	}
	
	if(ide_abs_bus_count != max_abs_bus_num+1) {
		dprintf("IDE -- init: max fixed ide bus number is %ld, "
		        "but there %s %ld fixed bus%s\n",
		        max_abs_bus_num, ide_abs_bus_count == 1 ? "is" : "are",
		        ide_abs_bus_count, ide_abs_bus_count == 1 ? "" : "ses");

		// make hole or ignore duplicate busnum
		ide_bus_count += max_abs_bus_num - ide_abs_bus_count + 1;
	}
	ide_busses = malloc(sizeof(ide_bus) * ide_bus_count);
	i=0;
	while(i <= max_abs_bus_num) {
		ide_busses[i].moduleinfo = NULL;
		ide_busses[i].first_bus_in_module = false;
		i++;
	}
	while(tmpmodlist) {
		uint32 buscount;
		tmpmod *tmp = tmpmodlist;
		bool first_bus_in_module = true;
		
		tmpmodlist = tmp->next;
		buscount = tmp->count;
		for(j = 0; j < tmp->count ; j++) {
			uint32 buscookie = tmp->info->get_nth_cookie(j);
			int32 busnum = tmp->info->get_abs_bus_num(buscookie);
			if(busnum < 0) {
				// add to end
				busnum = i;
				i++;
			}
			else if(ide_busses[busnum].moduleinfo != NULL) {
				dprintf("IDE -- init: ignoring duplicate module %s for bus %ld\n",
				        tmp->info->binfo.minfo.name, busnum);
				buscount--;
				continue;
			}
			ide_busses[busnum].moduleinfo = tmp->info;
			ide_busses[busnum].cookie = buscookie;
			ide_busses[busnum].first_bus_in_module = first_bus_in_module;
			first_bus_in_module = false;
			ide_busses[busnum].is_handled[0] = false;
			ide_busses[busnum].is_handled[1] = false;
			//dprintf("IDE -- init: bus %d is handled by module %s\n",
			//        busnum, tmp->info->binfo.minfo.name);
		}
		if(buscount < 1) {
			put_module(tmp->info->binfo.minfo.name);
		}
		free(tmp);
	}
	if(i != ide_bus_count) {
		dprintf("IDE -- init: fatal error, i = %d != %ld\n", i, ide_bus_count);
		return B_ERROR;
	}

	return B_NO_ERROR;

err:
	while(tmpmodlist) {
		tmpmod *tmp = tmpmodlist;
		tmpmodlist = tmp->next;
		put_module(tmp->info->binfo.minfo.name);
		free(tmp);
	}
	return err;
}

static status_t
uninit()
{
	int i;
//	dprintf("IDE -- uninit\n");
	for(i=0; i < ide_bus_count; i++) {
		if(ide_busses[i].first_bus_in_module) {
			put_module(ide_busses[i].moduleinfo->binfo.minfo.name);
		}
	}
	free(ide_busses);
#ifdef CHECKMALLOC
	my_memory_used();
#endif
	return B_NO_ERROR;
}

static status_t
std_ops_v06(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init_v06();
		
	case B_MODULE_UNINIT:
		return uninit();
	default:
		return -1;
	}
}

static status_t
rescan()
{
	return 0;
}

ide_module_info ide_module = {
	{
		{
			"bus_managers/ide/v0.6",
			0,
			&std_ops_v06
		},
		&rescan
	},
	&get_nth_ide_bus,
	&get_ide_bus_count,
	&handle_device,
	&release_device,

	&wait_status,
	&send_ata,
	&ide_reset
};

#ifdef __ZRECOVER
# include <recovery/module_registry.h>
  REGISTER_STATIC_MODULE(ide_module);
#else
# if !_BUILDING_kernel && !BOOT
  _EXPORT 
  ide_module_info *modules[] = {
	&ide_module,
	NULL
  };
# endif
#endif
