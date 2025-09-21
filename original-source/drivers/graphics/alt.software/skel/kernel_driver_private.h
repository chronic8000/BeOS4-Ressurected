//////////////////////////////////////////////////////////////////////////////
// Kernel Driver Macros and Typedefs
//
//    This header contains macros and typedefs used by the generic kernel
// driver that the accelerant doesn't need to know about.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Macros ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PCI Get and Put Functions
//    These assume the existence of a device_info pointer "di". This is as
// clean as I can make these.

#define get_pci(o, s) (*driver_data.pci_bus->read_pci_config)(di->pcii.bus, \
  di->pcii.device, di->pcii.function, (o), (s))

#define set_pci(o, s, v) (*driver_data.pci_bus->write_pci_config)(          \
  di->pcii.bus, di->pcii.device, di->pcii.function, (o), (s), (v))


//////////////////////////////////////////////////////////////////////////////
// PCI-related Constants

// PCI device control register. We treat PCI_command and PCI_status as one
// register.
#define PCI_DEVCTRL 0x04


//////////////////////////////////////////////////////////////////////////////
// Misc Useful Constants

#define GDS_MAX_DEVICES 8


//////////////////////////////////////////////////////////////////////////////
// Glue for constructing /dev file paths.

#if !defined(GRAPHICS_DEVICE_PATH)
#define GRAPHICS_DEVICE_PATH ""
#endif


//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Device info

typedef struct
{
  // General device-related information.
  uint32          is_open; // Indicates whether or not this device is open.
  char            name[B_OS_NAME_LENGTH]; // Name of this device.
  pci_info        pcii; // PCI info structure for this device.

  // Structure containing information shared with the accelerant.
  area_id         shared_area;
  SHARED_INFO     *si;

  // Variables related to the vertical blanking interrupt.
  struct
  {
    volatile int32  can_interrupt;
    long            handler_mutex; // Flag indicating whether the VBI handler
                                   // is active.
    thread_id       tid; // Fake interrupt thread ID.
    volatile int    fake_handler_installed; // Flag indicating that the fake
                                            // VBI thread is installed.
  } kernel_vbi;
} DEVICE_INFO;


//////////////////////////////////////////////////////////////////////////////
// Device data for all devices

typedef struct
{
  // Driver-wide benaphore for global locking.
  gds_benaphore   driver_benaphore;

  // PCI bus module, used by our PCI read/write macros.
  pci_module_info *pci_bus;

  // Table of device information records.
  uint32          device_count; // number of devices actually found
  DEVICE_INFO     di[GDS_MAX_DEVICES]; // device specific stuff
  char            *device_names[GDS_MAX_DEVICES+1]; // device name pointers
} DRIVER_DATA;


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
