/*
 * driver_basics.h
 * Copyright (c) 2000 Be, Inc.	All Rights Reserved
 * written by Peter Folk <pfolk@be.com>
 *
 *
 * Driver fundamentals (common to most/all PCI drivers)
 *
 * To use this, put the following in your driver's main
 * file, foo.c:
 *
 *   #define kDevName "foo"   // the device name
 *   #define kDevDir  "bar"   // the device dir
 *   // devices will appear as /dev/bar/foo/...
 *
 *   #define DEBUG             // if you want debug output
 *   #define MAX_DEVICES 4     // instances of this driver, max
 *   #define SINGLE_OPEN       // if only one open session per
 *                             // device is allowed at a time
 *   #define SINGLE_RWC        // if only one read/write/control
 *                             // at a time per device is allowed
 *   #define SINGLE_READ       // if only one read at a time...
 *   #define SINGLE_WRITE      // if only one write at a time...
 *   #define SINGLE_CONTROL    // if only one control at a time...
 *
 *   #define CARDBUS_SUPPORT   // if you want cardbus device support
 *
 *   typedef struct {
 *   	int32     devID;       // device identifier: 0-MAX_DEVICES
 *   	pci_info  *pciInfo;    // info about this device
 *   	uint32    debug;       // debugging level mask
 *      ...
 *   } device_info_t;  // your per-device globals
 *   #include <driver_basics.h>
 *   #include <driver_basics.c>
 *
 *   // Then your versions of...
 *   static status_t check_device(const pci_info* dev);
 *   static status_t open_device(device_info_t *dev, uint32 flags);
 *   static status_t close_device(device_info_t *dev);
 *   static status_t free_device(device_info_t *dev);
 *   static status_t control_device(device_info_t *dev, uint32 msg,void *buf, size_t len);
 *   static status_t read_device( device_info_t *dev, off_t pos, void *buf, size_t *len);
 *   static status_t write_device(device_info_t *dev, off_t pos, const void *buf, size_t *len);
 *
 * Easy as could be, eh?  In your code, print debug messages with
 *   DEBUG_DEVICE(type, "msg", device, args...);
 * where type is one of ERR, INFO, WARN, SEQ, FUNCTION, INTERRUPT,
 * PCI_IO or something else you |= with debug_mask in device_debugcmd().
 *
 */
#ifndef _DRIVER_BASICS_H
#define _DRIVER_BASICS_H

#define DEVNAME_LENGTH  64			

/* for serial debug command */
#ifdef DEBUG
/* debug flags */
enum debug_flags {
    ERR       = 0x0001,
    INFO      = 0x0002,
    RX        = 0x0004,    /* dump received frames */
    TX        = 0x0008,    /* dump transmitted frames */
    INTERRUPT = 0x0010,    /* interrupt calls */
    FUNCTION  = 0x0020,    /* function calls */
    PCI_IO    = 0x0040,    /* pci reads and writes */
    SEQ		  = 0x0080,    /* trasnmit & receive TCP/IP sequence sequence numbers */
    WARN	  = 0x0100,    /* Warnings - off on final release */
	TIMER	  = 0x0200,	   /* Timer - show all timer information */
	MEM		  = 0x0400,	   /* Memory - show any memory-mapping messages */
};

/* diagnostic debug flags - compile in here or set while running with debugger "bcm4210" command */
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO )
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN )
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | SEQ)
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | RX | TX | INTERRUPT)
#define DEFAULT_DEBUG_FLAGS (ERR | FUNCTION | INFO | INTERRUPT)
//#define DEFAULT_DEBUG_FLAGS (ERR | INTERRUPT)
//#define DEFAULT_DEBUG_FLAGS (ERR)

//static int debugger_cmd(int argc, char **argv);
uint32 debug_mask = DEFAULT_DEBUG_FLAGS;

#define DEBUG_MSG(enabled, args...) \
	if (debug_mask & enabled) dprintf(kDevName " - " args);
#define DEBUG_DEVICE(enabled, format, device, args...) \
	if (device->debug & enabled) \
	dprintf(kDevName "/%ld - " format, device->devID , ##args)
	// Doh!  the ##__VA_ARGS__ trick only works in Gnu CPP---does
	// MetroWerks CPP even handle varargs macros?
#define DEBUG_ASSERT( condition, format, device, args...) \
	if ( !condition ) \
	dprintf(kDevName "/%ld: ASSERTION FAILED! - " format, (long int)device->devID, ##args)
#else
#define DEBUG_MSG(...)
#define DEBUG_DEVICE(enabled, device, args...)
#define DEBUG_ASSERT( condition, format, device, args...)
#endif

enum _device_flags {
	DevHardwarePresent	= 0x00000001,
	DevDriverOpened		= 0x00000002,
	DevBusPCI			= 0x00000010,
	DevBusCardBus		= 0x00000020,
	DevBusISA			= 0x00000040,
	DevBusPCMCIA		= 0x00000080,
};

static struct _global_dev_info {
	pci_info*		dev_pci_info;		/* globally-accessible pci_info* */
	device_info_t*	dev_info;			/* globally-accessible device_info_t* */
	uint32			dev_flags;			/* Device status flags */
	uint32			dev_other;			/* available for driver use */
} gDev[MAX_DEVICES + 1];

/* Global DeviceID for the debugger command */
static int32 gDevID;

static pci_module_info        *gPCIModInfo;
static fc_lock_module_info	  *gFC;
static char                   *gDevNameList[MAX_DEVICES+1];
#ifdef SINGLE_OPEN
static int32                  gOpenMask = 0;
#endif
#ifdef SINGLE_RWC
static int32                  gRWCMask = 0;
#endif
#ifdef SINGLE_READ
static int32                  gReadMask = 0;
#endif
#ifdef SINGLE_WRITE
static int32                  gWriteMask = 0;
#endif
#ifdef SINGLE_CONTROL
static int32                  gControlMask = 0;
#endif

/* Driver Entry Points - implemented in driver_basics.c */
status_t init_hardware(void );
status_t init_driver(void );
void uninit_driver(void );
const char** publish_devices(void );
device_hooks *find_device(const char *name );

static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *len);
//static status_t select_hook(void *cookie, uint8 event, uint32 ref, selectsync *sync);
//static status_t deselect_hook(void *cookie, uint8 event, uint32 ref, selectsync *sync);
//static status_t readv_hook(void *cookie, off_t position, const struct iovec *vec, size_t count, size_t *len);
//static status_t writev_hook(void *cookie, off_t position, const struct iovec *vec, size_t count, size_t *len);

static device_hooks gDeviceHooks =  {
	open_hook,          /* -> open entry point */
	close_hook,         /* -> close entry point */
	free_hook,          /* -> free entry point */
	control_hook,       /* -> control entry point */
	read_hook,          /* -> read entry point */
	write_hook,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,               /* -> deselect entry point */
	NULL,               /* -> readv */
	NULL       	        /* -> writev */
};


/* device hooks - you implement */
static status_t check_device(const pci_info* dev, uint32* devIDptr);  /* return B_OK if it's ours */
static status_t open_device(device_info_t *dev, uint32 flags);
#ifdef CARDBUS_SUPPORT
static status_t stop_device(device_info_t *dev); /* hardware removed, when driver still opened */
#endif
static status_t close_device(device_info_t *dev);
static status_t free_device(device_info_t *dev);
static status_t control_device(device_info_t *dev, uint32 msg,void *buf, size_t len);
/* For future use...
	static status_t select_device(device_info_t *dev, uint8 event, uint32 ref, selectsync *sync);
	static status_t deselect_device(device_info_t *dev, uint8 event, uint32 ref, selectsync *sync);
	#ifdef RW_ALWAYS_V
	static status_t read_device( device_info_t *dev, off_t pos, const struct iovec *vec, size_t count, size_t *len);
	static status_t write_device(device_info_t *dev, off_t pos, const struct iovec *vec, size_t count, size_t *len);
	#else
*/
static status_t read_device( device_info_t *dev, off_t pos, void *buf, size_t *len);
static status_t write_device(device_info_t *dev, off_t pos, const void *buf, size_t *len);
static int device_debugcmd( int argc, char **argv);
//	#endif

#endif /* _DRIVER_BASICS_H */
