//////////////////////////////////////////////////////////////////////////////
// Driver
//   My goodness, but there's some work to do here.  I think we can probably
// bust this up a bit and fix it.  At the moment, I'm just going to clean
// it up a bit and see if I can modularize it.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Debug Settings
//   This is where we decide how verbose the driver is going to be.

#if DEBUG > 0

#define DPRINTF_ON      old_state = set_dprintf_enabled(TRUE)
#define DPRINTF_RESET   set_dprintf_enabled(old_state)
#define ddprintf(a)     dprintf a

#else

#define DPRINTF_ON
#define DPRINTF_RESET
#define ddprintf(a)

#endif


//////////////////////////////////////////////////////////////////////////////
// PCI Get and Put Functions

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))


//////////////////////////////////////////////////////////////////////////////
// Useful Constants

#define MAX_MGA_DEVICES 8
#define MGA_VENDOR      0x102b
#define STORM_VLINE_INT (1 << 5)


//////////////////////////////////////////////////////////////////////////////
// Uh...

#if !defined(GRAPHICS_DEVICE_PATH)
#define GRAPHICS_DEVICE_PATH ""
#endif


//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Device Info

typedef struct
{
  uint32      is_open;
  area_id     shared_area;
  SHARED_INFO *si;
  int32       can_interrupt;
  thread_id   tid;           /* only used if we're faking interrupts */
  pci_info    pcii;
  char        name[B_OS_NAME_LENGTH];
} DEVICE_INFO;


//////////////////////////////////////////////////////////////////////////////
// Device Data

typedef struct
{
  uint32 count;  // number of devices actually found
  int32  ben;    // integer part of driver-wide benaphore
  sem_id sem;    // semaphore part of driver-wide benaphore
  char   *device_names[MAX_MGA_DEVICES+1]; // device name pointer storage
  DEVICE_INFO di[MAX_MGA_DEVICES];         // device specific stuff
} DEVICE_DATA;


//////////////////////////////////////////////////////////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static status_t open_hook(const char* name,
                          uint32 flags,
                          void** cookie);

static status_t close_hook(void* dev);

static status_t free_hook(void* dev);

static status_t control_hook(void* dev,
                             uint32 msg,
                             void *buf,
                             size_t len);

static status_t read_hook(void* dev,
                          off_t pos,
                          void* buf,
                          size_t* len);

static status_t write_hook(void* dev,
                           off_t pos,
                           const void* buf,
                           size_t* len);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
