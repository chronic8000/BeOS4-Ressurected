#include "common/driver_basics.h"
#include "common/pci_mem.c"
#include <stdlib.h>

/*
 * Utility functions
 */
static int32 get_pci_list(pci_info *info[], int32 maxEntries)
{
	status_t status;
	int32 i, entries;
	pci_info	*item;
	
	item = (pci_info *)malloc(sizeof(pci_info));
	
	for (i=0, entries=0; entries<maxEntries; i++)
	{
		if ((status = gPCIModInfo->get_nth_pci_info(i, item)) != B_OK)
			break;
		
		/* check if the device really has an IRQ */
		if (item->u.h0.interrupt_line==0 || item->u.h0.interrupt_line==0xFF)
		{
			// DEBUG_MSG(INFO, "found without valid IRQ: check BIOS\n");
			continue;
		}
		
		if (check_device(item) == B_OK)
		{
			DEBUG_MSG(INFO, "found at IRQ %x\n", item->u.h0.interrupt_line);
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
		}
	}
	info[entries] = NULL;
	free(item);
	return entries;
}

static status_t free_pci_list(pci_info *info[])
{
	int32 		i;
	pci_info	*item;
	
	for (i=0; (item=info[i]); i++)
		free(item);
	return B_OK;
}



/*
 * Driver Entry Points
 */
status_t init_hardware(void )
{
	DEBUG_MSG(FUNCTION, "init_hardware()\n");
	return B_NO_ERROR;
}


status_t init_driver()
{
	int32			entries;
	char			devName[PATH_MAX];
	int32			i;
	status_t		status;
	
	DEBUG_MSG(FUNCTION, "entering init_driver()\n");
	
	
	status = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo);
	if (status != B_OK)
	{
		DEBUG_MSG(ERR, "leaving init_driver(): get_module failed! %s\n", strerror(status));
		return status;
	}
	
	// Find Lan cards
	entries = get_pci_list(gDevList, MAX_DEVICES);
	if (entries == 0)
	{
		DEBUG_MSG(ERR, "leaving init_driver(): " kDevName " not found\n");
		free_pci_list(gDevList);
		put_module(B_PCI_MODULE_NAME );
		return B_ERROR;
	}
	else
	{
		DEBUG_MSG(INFO, "init_driver(): Found %ld devices.\n", entries);
	}
	
	// Create device name list
	for (i=0; i<entries; i++ )
	{
		sprintf(devName, kDevDir "/" kDevName "/%ld", i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
	}
	gDevNameList[i] = NULL;
	
	#ifdef DEBUG
	add_debugger_command (kDevName, debug_hook, kDevName " driver Info");
	#endif
	
	DEBUG_MSG(FUNCTION, "leaving init_driver()\n");
	return B_OK;
}


const char** publish_devices(void )
{
	DEBUG_MSG(FUNCTION, "publish_devices()\n");
	return (const char **)gDevNameList;
}


device_hooks *find_device(const char *name)
{
	int32 	i;
	char 	*item;
	
	DEBUG_MSG(FUNCTION, "entering find_device()\n");
	/* Find device name */
	for (i=0; (item=gDevNameList[i]); i++)
	{
		if (strcmp(name, item) == 0)
		{
			DEBUG_MSG(INFO, "find_device(): device found\n");
			DEBUG_MSG(FUNCTION, "leaving find_device()\n");
			return &gDeviceHooks;
		}
	}
	return NULL; /*Device not found */
}


void uninit_driver(void)
{
	int32 	i;
	void 	*item;

	DEBUG_MSG(FUNCTION, "entering uninit_driver()\n");

	// Free device name list
	for (i=0; (item=gDevNameList[i]); i++)
		free(item);
	
	// Free device list
	free_pci_list(gDevList);
	put_module(B_PCI_MODULE_NAME);
	
	#ifdef DEBUG
	remove_debugger_command(kDevName, debug_hook);
	#endif
	
	DEBUG_MSG(FUNCTION, "leaving uninit_driver()\n");
}


/*
 * Device hooks
 */
static status_t open_hook(const char *name, uint32 flags, void **cookie)
{
	int32				devID;
	int32				mask;
	status_t			status;
	char 				*devName;
	dev_info_t 			*device;
	
	DEBUG_MSG(FUNCTION, "entering open(%s)\n", name);
	
	for (devID=0; (devName=gDevNameList[devID]); devID++)
	{	/* Find device name */
		if (strcmp(name, devName) == 0)
		{
			DEBUG_MSG(INFO, "open(%s): found device at index %ld\n", devName, devID);
			break;
		}
	}
	if (!devName)
	{
		DEBUG_MSG(ERR, "open(%s) failed: no such device!\n", name);
		DEBUG_MSG(FUNCTION, "leaving open(%s)\n", name);
		return EINVAL;
	}
	
	/* Check if the device is busy and set in-use flag if not */
	mask = 1 << devID;
	if (atomic_or(&gOpenMask, mask) &mask)
	{
	#ifdef SINGLE_OPEN
		DEBUG_MSG(ERR, "open(%s) failed: device busy!\n", name);
		DEBUG_MSG(FUNCTION, "leaving open(%s)\n", name);
		return B_BUSY;
	#endif
	}
	
	/* Allocate storage for per driver instance global data */
	// This memory is automatically locked (standard kernel malloc behavior)
	if (!(*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t))))
	{
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));
	
	device->devID = devID;
	device->pciInfo = gDevList[devID];
	device->debug = DEFAULT_DEBUG_FLAGS;
	
	DEBUG_MSG(INFO, "open(%s): cookie=%p\n", name, device);
	
	
	status = open_device(device, flags);
	if (status != B_OK) goto err1;
	gDevInfo[devID] = device;  /* for the debug command */
	
	
	DEBUG_MSG(FUNCTION, "leaving open(%s)\n", name);
	return B_OK;
	
	
  err1:
	free_device(device);
	free(device);	
	
  err0:
	#ifdef SINGLE_OPEN
	atomic_and(&gOpenMask, ~mask);
	#endif
	DEBUG_MSG(ERR, "open(%s) failed: %s!\n", name, strerror(status));
	DEBUG_MSG(FUNCTION, "leaving open(%s)\n", name);
	return status;
}

static status_t close_hook(void *_device)
{
	status_t status;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering close()\n", device);
	
	status = close_device(device);
	if (status != B_OK)
	{
		DEBUG_DEVICE(ERR, "close() failed: %s!\n", device, strerror(status));
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving close()\n", device);
	return status;
}

static status_t free_hook(void *_device)
{
	status_t status;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering free()\n", device);
	
	status = free_device(device);
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "free() failed: %s!\n", device, strerror(status));
	}
	free(device);
	
	// Device is now available again
	atomic_and(&gOpenMask, ~(1 << device->devID));
	
	DEBUG_DEVICE(FUNCTION, "leaving free()\n", device);
	return status;
}

static status_t control_hook(void *_device, uint32 msg, void *buf, size_t len)
{
	status_t status;
	int32 mask;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering control()\n", device);
	
	mask = 1 << device->devID;
	#ifdef SINGLE_RWC
	/* protect against re-entrant rwc */
	if (atomic_or(&gRWCMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "control() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving control()\n", device);
		return B_BUSY;
	}
	#endif
	
	#ifdef SINGLE_CONTROL
	/* protect against re-entrant control */
	if (atomic_or(&gControlMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "control() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving control()\n", device);
		
		#ifdef SINGLE_RWC
		// Device is now available again
		atomic_and(&gRWCMask, ~mask);
		#endif
	
		return B_BUSY;
	}
	#endif
	
	
	status = control_device(device,msg,buf,len);
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "control() failed: %s!\n", device, strerror(status));
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving control()\n", device);
	
	
	#ifdef SINGLE_CONTROL
	// Device is now available again
	atomic_and(&gControlMask, ~mask);
	#endif
	
	#ifdef SINGLE_RWC
	// Device is now available again
	atomic_and(&gRWCMask, ~mask);
	#endif
	
	return status;
}

static status_t read_hook(void *_device, off_t pos, void *buf, size_t *len)
{
	status_t status;
	int32 mask;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering read()\n", device);
	
	mask = 1 << device->devID;
	#ifdef SINGLE_RWC
	/* protect against re-entrant rwc */
	if (atomic_or(&gRWCMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "read() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving read()\n", device);
		return B_BUSY;
	}
	#endif
	
	#ifdef SINGLE_READ
	/* protect against re-entrant read */
	if (atomic_or(&gReadMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "read() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving read()\n", device);
		
		#ifdef SINGLE_RWC
		// Device is now available again
		atomic_and(&gRWCMask, ~mask);
		#endif
	
		return B_BUSY;
	}
	#endif
	
	
	#ifdef RW_ALWAYS_V
	{
		struct iovec io = { buf,*len };
		status = read_device(device,pos,&io,1,len);
	}
	#else
	status = read_device(device,pos,buf,len);
	#endif
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "read() failed: %s!\n", device, strerror(status));
	}
	else
	{
		DEBUG_DEVICE(INFO, "read %ld bytes.\n", device, *len);
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving read()\n", device);
	
	
	#ifdef SINGLE_READ
	// Device is now available again
	atomic_and(&gReadMask, ~mask);
	#endif
	
	#ifdef SINGLE_RWC
	// Device is now available again
	atomic_and(&gRWCMask, ~mask);
	#endif
	
	return status;
}

static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len)
{
	status_t status;
	int32 mask;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering write()\n", device);
	
	mask = 1 << device->devID;
	#ifdef SINGLE_RWC
	/* protect against re-entrant rwc */
	if (atomic_or(&gRWCMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "write() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving write()\n", device);
		return B_BUSY;
	}
	#endif
	
	#ifdef SINGLE_WRITE
	/* protect against re-entrant write */
	if (atomic_or(&gWriteMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "write() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving write()\n", device);
		
		#ifdef SINGLE_RWC
		// Device is now available again
		atomic_and(&gRWCMask, ~mask);
		#endif
		
		return B_BUSY;
	}
	#endif
	
	
	#ifdef RW_ALWAYS_V
	{
		struct iovec io = { buf,*len };
		status = write_device(device,pos,&io,1,len);
	}
	#else
	status = write_device(device,pos,buf,len);
	#endif
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "write() failed: %s!\n", device, strerror(status));
	}
	else
	{
		DEBUG_DEVICE(INFO, "wrote %ld bytes.\n", device, *len);
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving write()\n", device);
	
	
	#ifdef SINGLE_WRITE
	// Device is now available again
	atomic_and(&gWriteMask, ~mask);
	#endif
	
	#ifdef SINGLE_RWC
	// Device is now available again
	atomic_and(&gRWCMask, ~mask);
	#endif
	
	return status;
}

#if 0
static status_t readv_hook(void *_device, off_t pos, const struct iovec *ios, size_t count, size_t *len)
{
	int i;
	int32 mask;
	status_t status = B_OK;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering readv()\n", device);
	
	mask = 1 << device->devID;
	#ifdef SINGLE_RWC
	/* protect against re-entrant rwc */
	if (atomic_or(&gRWCMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "readv() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving readv()\n", device);
		return B_BUSY;
	}
	#endif
	
	#ifdef SINGLE_READ
	/* protect against re-entrant read */
	if (atomic_or(&gReadMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "readv() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving readv()\n", device);
		
		#ifdef SINGLE_RWC
		// Device is now available again
		atomic_and(&gRWCMask, ~mask);
		#endif
		
		return B_BUSY;
	}
	#endif
	
	
	#ifdef RW_ALWAYS_V
	status = read_device(device,pos,ios,count,len);
	#else
	*len = 0;
	for (i=0; i<count; ++i)
	{
		size_t iov_len = ios[i].iov_len;
		status = read_device(device,pos,ios[i].iov_base,&iov_len);
		*len += iov_len;
		
		if (status < B_OK) break;
	}
	#endif
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "readv() failed: %s!\n", device, strerror(status));
	}
	else
	{
		DEBUG_DEVICE(INFO, "read %ld bytes.\n", device, *len);
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving readv()\n", device);
	
	
	#ifdef SINGLE_READ
	// Device is now available again
	atomic_and(&gReadMask, ~mask);
	#endif
	
	#ifdef SINGLE_RWC
	// Device is now available again
	atomic_and(&gRWCMask, ~mask);
	#endif
	
	return status;
}

static status_t writev_hook(void *_device, off_t pos, const struct iovec *ios, size_t count, size_t *len)
{
	int i;
	int32 mask;
	status_t status = B_OK;
	dev_info_t *device = (dev_info_t *) _device;
	DEBUG_DEVICE(FUNCTION, "entering writev()\n", device);
	
	mask = 1 << device->devID;
	#ifdef SINGLE_RWC
	/* protect against re-entrant rwc */
	if (atomic_or(&gRWCMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "writev() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving writev()\n", device);
		return B_BUSY;
	}
	#endif
	
	#ifdef SINGLE_WRITE
	/* protect against re-entrant write */
	if (atomic_or(&gWriteMask, mask) &mask)
	{
		DEBUG_DEVICE(ERR, "readv() failed: device busy!\n", device);
		DEBUG_DEVICE(FUNCTION, "leaving readv()\n", device);
		
		#ifdef SINGLE_RWC
		// Device is now available again
		atomic_and(&gRWCMask, ~mask);
		#endif
		
		return B_BUSY;
	}
	#endif
	
	
	#ifdef RW_ALWAYS_V
	status = write_device(device,pos,ios,count,len);
	#else
	*len=0;
	for (i=0; i<count; ++i)
	{
		size_t iov_len = ios[i].iov_len;
		status = write_device(device,pos,ios[i].iov_base,&iov_len);
		*len += iov_len;
		
		if (status < B_OK) break;
	}
	#endif
	if (status < B_OK)
	{
		DEBUG_DEVICE(ERR, "writev() failed: %s!\n", device, strerror(status));
	}
	else
	{
		DEBUG_DEVICE(INFO, "wrote %ld bytes.\n", device, *len);
	}
	
	DEBUG_DEVICE(FUNCTION, "leaving writev()\n", device);
	
	
	#ifdef SINGLE_WRITE
	// Device is now available again
	atomic_and(&gWriteMask, ~mask);
	#endif
	
	#ifdef SINGLE_RWC
	// Device is now available again
	atomic_and(&gRWCMask, ~mask);
	#endif
	
	return status;
}
#endif


#ifdef DEBUG
int debug_hook(int argc, char **argv)
{
	if (argc>1 && argv[1][0]>='0' && argv[1][0]<='9')
	{
		int devID = strtoul(argv[1],NULL,0);
		if (devID>8*sizeof(gOpenMask) || ((1<<devID)&gOpenMask) == 0)
		{
			kprintf("usage: " kDevName " [device_id] ...\n");
			return -1;
		}
		else return debug_device(gDevInfo[devID],argc-1,argv+1);
	}
	else return debug_device(NULL,argc,argv);
}
#endif


#include "fc_lock.c"
#include "benaphore.c"
int32	api_version = B_CUR_DRIVER_API_VERSION;
