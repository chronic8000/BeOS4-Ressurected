//////////////////////////////////////////////////////////////////////////////
// Kernel Driver Hardware-Specific Function Prototypes
//
//    This file contains extern declarations and function prototypes for all
// device-dependent driver variables and functions that must be implemented
// in order to produce a kernel driver.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Supported devices table entry definition.

typedef struct
{
  // PCI vendor ID and device ID for this class of device.
  uint16 vendor_id;
  uint16 device_id;

  // Prefix for device names exported to /dev/graphics.
  char *device_name;
} DEVICE_TABLE_ROW;


//////////////////////////////////////////////////////////////////////////////
// Extern Constants //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Supported devices table.
// This is terminated by an entry with a vendor ID of 0x0000.

extern DEVICE_TABLE_ROW SupportedDevices[];


//////////////////////////////////////////////////////////////////////////////
// Prototypes ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialization related functions.
//

// This function extracts frame buffer and register aperture pointers from
// PCI configuration space. This usually follows standard conventions, but
// not always, so it has to be considered hardware-specific.
// The pointers are physical addresses. The generic skeleton will map them
// into virtual address space.
// A NULL register pointer will be handled gracefully. This might occur if
// a particular card doesn't use memory mapped registers.
void GetCardPointers(pci_info *pcii, void **fb_pointer, uint32 *fb_size,
  void **reg_pointer, uint32 *reg_size);


//////////////////////////////////////////////////////////////////////////////
// IOCTL related functions.
//

// This function handles IOCTL calls with non-standard opcodes, used for
// implementing device-specific features. If the main IOCTL handler doesn't
// recognize the opcode passed to it, it calls this function, giving a
// pointer to the device_info structure of the device that the calling
// accelerant is managing.

status_t Handle_Hardware_Specific_Ioctl(DEVICE_INFO *di, uint32 opcode,
  void *buf, size_t len);


//////////////////////////////////////////////////////////////////////////////
// Interrupt related functions.
//

//    This function sets the colour lookup table used for palette generation
// at 8 bpp. "first" indicates the index of the first colour to be written,
// "count" specifies the number of palette entries to set, and "data" points
// to an array of bytes defining the red, green, and blue values of the
// palette entries that are to be written (in that order). "si" points to the
// SHARED_INFO structure associated with the card to be programmed.

void set_palette(int first, int count, uint8 *data, SHARED_INFO *si);


//    This function performs hardware-specific activities that must be
// attended to to service the vertical blanking interrupt.

void hardware_specific_vblank_work(SHARED_INFO *si);

//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
