//////////////////////////////////////////////////////////////////////////////
// Driver Hooks
//
//    This file declares the hook functions that are called when the kernel
// driver is used.
//    NOTE: "void *dev" should be replaced with "void *cookie", and we should
// implement proper cookie management to handle things like team-specific
// VBI semaphores and the like. Implement this in the next major revision
// of the skeleton.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Hooks used for general driver control.
//////////////////////////////////////////////////////////////////////////////
//    NOTE: These are included here for convenience only. The actual
// prototypes for these functions are in Driver.h in /boot/develop/headers.

// Don't actually use these as prototypes.
#if 0


//////////////////////////////////////////////////////////////////////////////
// Initialize Driver
//   This sets up the driver, or dies.  If it succeeds, it returns B_OK
// and is in charge of the devices it controls.  If it returns anything
// else, it is unceremoniously tossed from memory.
status_t init_driver(void);

//////////////////////////////////////////////////////////////////////////////
// Uninitialize the Driver
//    This shuts down the driver.
void uninit_driver(void);

//////////////////////////////////////////////////////////////////////////////
// Initialize Hardware
//    This sets up the hardware, or dies.  If it succeeds, it returns B_OK
// and the driver is in charge of the devices it controls.  If it returns
// anything else, the driver is unceremoniously tossed from memory.
status_t init_hardware(void);

//////////////////////////////////////////////////////////////////////////////
// Publish Devices
//    This returns a list of supported devices.
//    HACK - This is the only kernel hook that has () instead of (void) in
// Drivers.h.
const char **publish_devices();

//////////////////////////////////////////////////////////////////////////////
// Find a Device
//    This looks for the device that matches the input string, and returns a
// table to its file-hook functions if the device is found. In practice, we
// just have one table of file hooks that apply to all devices.
device_hooks *find_device(const char *name);


#endif
// Everything from here onwards is still active.


//////////////////////////////////////////////////////////////////////////////
// Hooks used when devices in /dev are manipulated as files.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Open a graphics device.
//    This is called when a graphics device in /dev is opened. The cookie
// value returned is a pointer to the device_info structure for this device.
status_t open_hook(const char* name, uint32 flags, void** cookie);

//////////////////////////////////////////////////////////////////////////////
// Close a graphics device.
//    This is called when a graphics device in /dev is closed. "dev" is a
// pointer to the device_info structure for this device.
status_t close_hook(void* dev);

//////////////////////////////////////////////////////////////////////////////
// Free a graphics device's cookie.
//    This is called when no open file handles remain to a given graphics
// device from a given team. "dev" is a pointer to the device_info structure
// for this device.
//    NOTE: I *think* that these are the conditions under which this function
// is called. This will be important when we implement proper cookie
// management.
status_t free_hook(void* dev);

//////////////////////////////////////////////////////////////////////////////
// Read from a graphics device.
//    This is called when a graphics device in /dev is read from. "dev" is a
// pointer to the device_info structure for this device.
status_t read_hook(void* dev, off_t pos, void* buf, size_t* len);

//////////////////////////////////////////////////////////////////////////////
// Write to a graphics device.
//    This is called when a graphics device in /dev is written to. "dev" is a
// pointer to the device_info structure for this device.
status_t write_hook(void* dev, off_t pos, const void* buf, size_t* len);

//////////////////////////////////////////////////////////////////////////////
// IOCTL Hook
//    This handles IOCTL calls made to a graphics device. This is the primary
// means of communication between user-space accelerants and the kernel
// driver. "dev" is a pointer to the device_info structure for this this
// device.
status_t control_hook(void* dev, uint32 msg, void *buf, size_t len);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
