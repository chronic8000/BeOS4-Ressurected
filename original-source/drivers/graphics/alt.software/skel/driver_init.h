//////////////////////////////////////////////////////////////////////////////
// Driver Initialization
//
//    This file declares helper functions used for kernel driver
// initialization.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialize a Device Record
//    This function initializes a DEVICE_INFO record to blank values and
// creates an associated SHARED_INFO record, also initialized to blank
// values. "id_number" is an identification signature used to label areas
// dynamically allocated for this DEVICE_INFO record.
//    Returns B_OK if successful or another value if not.
status_t init_device_record(DEVICE_INFO *di, uint32 id_number);

//////////////////////////////////////////////////////////////////////////////
// Delete a Device Record
//    This function de-allocates all dynamically allocated entities
// associated with the specified DEVICE_INFO record.
void delete_device_record(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Turn on PCI access to the device.
//    This function enables PCI access to the device (through memory mapped
// apertures, etc.).
void enable_pci_access(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Disable PCI access to the device.
//    This function disables PCI access to the device (reads/writes to memory
// mapped apertures are no longer decoded, etc.).
void disable_pci_access(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Map the device's apertures.
//    This function maps the specified device's PCI memory apertures into
// virtual memory space.
//    Returns B_OK if successful or another value if not.
status_t map_device(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Unmap the device's apertures.
//    This function deletes the specified device's memory-mapped apertures
// from virtual memory space.
void unmap_device(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Set up interrupts for the device.
//    This function initializes interrupt handling for the specified device.
//    Returns B_OK if successful or another value if not.
status_t init_device_interrupts(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Unhook and remove interrupts for the device.
//    This function removes interrupt handling for the specified device.
void uninit_device_interrupts(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Update SHARED_INFO from DEVICE_INFO
//    This function updates a DEVICE_INFO record's SHARED_INFO structure
// based on the information contained within the DEVICE_INFO structure.
void update_si_from_di(DEVICE_INFO *di);

//////////////////////////////////////////////////////////////////////////////
// Probe Devices
//    This detects all supported devices in PCI space and sets up DEVICE_INFO
// entries for them in driver_data.di. SHARED_INFO records are also
// allocated.
void probe_devices(void);

//////////////////////////////////////////////////////////////////////////////
// Initialize a Device
//    This function initializes the specified device, allocates dynamic
// variables needed by the device, and updates di and si appropriately.
// Returns B_OK if successful or another value if not.
status_t init_device(DEVICE_INFO *di);

/////////////////////////////////////////////////////////////////////////////
// Uninitialize a Device
//    This function de-initializes the specified device, releases dynamic
// variables allocated for the device, and updates di appropriately (si is
// de-allocated).
void uninit_device(DEVICE_INFO *di);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
