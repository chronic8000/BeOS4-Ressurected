//////////////////////////////////////////////////////////////////////////////
// Driver Kernel Hooks
//
//    This file declares the hook functions that are called when the kernel
// manipulates the driver.
//////////////////////////////////////////////////////////////////////////////
// NOTE: As far as I can tell, the order of calls is:
//    - init_hardware
//    - init_driver
//    - publish_driver
//    - uninit_driver
//    - init_driver
//    - find_device
//    - [...]
//    - uninit_driver
// It would seem that both init_hardware and init_driver have to probe for
// supported devices, with init_hardware being a stand-alone function.

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <driver_includes.h>

const char **publish_devices(void);


//////////////////////////////////////////////////////////////////////////////
// Hook Functions ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialize Hardware
//    This is a stand-alone function that sets up the hardware or dies. If
// it succeeds, it returns B_OK and the driver will be in charge of the
// devices it controls. If it returns anything else, the driver is
// unceremoniously tossed from memory.

//    NOTE: This would properly be called after init_driver, and would do
// full device initialization. For now, though, it just probes the devices
// and returns B_OK if any supported devices are detected. Initialization
// is left to init_driver.

status_t init_hardware(void)
{
  int           index;

  // Diagnostics.
  ddprintf(("[GDS] init_hardware called.\n"));


  // Get a handle for the pci bus.
  // Proceed if successful.
  if (get_module(B_PCI_MODULE_NAME, (module_info **)&(driver_data.pci_bus))
    != B_OK)
  {
    // We couldn't load the PCI module.
    ddprintf(("[GDS] No supported devices found. init_hardware returning B_ERROR.\n"));
  }
  else
  {
    // PCI module loaded.

    // Initialize the device list to size zero.
    // Unnecessary, but do it anyways.
    driver_data.device_count = 0;

    // Find all of our supported devices.
    // This also initializes DEVICE_INFO and SHARED_INFO records for
    // detected devices.
    probe_devices();

    // Bail if no devices were found.
    if (driver_data.device_count <= 0)
    {
      ddprintf(("[GDS] No supported devices found. init_hardware returning B_ERROR.\n"));
    }
    else
    {
      // We've detected supported devices; delete the device records we
      // allocated and return success.
      for (index = 0; index < driver_data.device_count; index++)
      {
        // Get rid of this device's DEVICE_INFO record.
        delete_device_record(&(driver_data.di[index]));
      }

      // Return success.
      return B_OK;
    }

    // If we reached here, initialization failed.
    // Put the pci module away.
    put_module(B_PCI_MODULE_NAME);
  }


  // If we reached here, initialization failed.
  return B_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// Initialize Driver
//    This sets up the driver, or dies. If it succeeds, it returns B_OK. If
// it returns anything else, it is unceremoniously tossed from memory.
//    NOTE: This has to be stand-alone; init_hardware isn't guaranteed to be
// called before this.
//    NOTE: Card initialization should be done here, instead of in the
// accelerant.

status_t init_driver(void) 
{
  int           index;

  // Diagnostics.
  ddprintf(("[GDS] init_driver called.\n"));

  // Initialize device_data.

  // Initialize the device list to size zero.
  //    This must always be done regardless of success or failure, so that
  // on failure uninit_driver doesn't try to de-initialize nonexistant
  // devices.
  driver_data.device_count = 0;

  // Get a handle for the pci bus.
  // Proceed if successful.
  if (get_module(B_PCI_MODULE_NAME, (module_info **)&(driver_data.pci_bus))
    != B_OK)
  {
    // We couldn't load the PCI module.
    ddprintf(("[GDS] No supported devices found. init_hardware returning B_ERROR.\n"));
    driver_data.pci_bus = NULL;
  }
  else
  {
    // PCI module loaded.

    // Initialize the driver locking benaphore.
    INIT_GDS_BENAPHORE(&(driver_data.driver_benaphore),
      DRIVER_NAME "kernel driver lock");

    // Proceed if successful.
    if (driver_data.driver_benaphore.sem < 0)
    {
      // Benaphore was not initialized.
      ddprintf(("[GDS] Error - Couldn't initialize semaphore in driver locking benaphore.\n"));
    }
    else
    {
      // Benaphore initialized.

      // Find all of our supported devices.
      // This also initializes DEVICE_INFO and SHARED_INFO records for
      // detected devices.
      probe_devices();

      // Bail if no devices were found.
      if (driver_data.device_count <= 0)
      {
        ddprintf(("[GDS] No supported devices found.\n"));
      }
      else
      {
        // Initialize devices.
        for (index = 0; index < driver_data.device_count; index++)
        {
          // HACK - Ignore errors, for now.
          init_device(&(driver_data.di[index]));
        }

        // Return success.
        return B_OK;
      }

      // If we reached here, initialization failed.
      // De-initialize the driver locking benaphore.
      INIT_GDS_BENAPHORE(&(driver_data.driver_benaphore), DRIVER_NAME "kernel driver lock");
    }

    // If we reached here, initialization failed.
    // Put the pci module away.
    put_module(B_PCI_MODULE_NAME);
  }


  // If we reached here, initialization failed.
  return B_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// Uninitialize the Driver
//    This shuts down the driver.

void uninit_driver(void)
{
  uint32 index;

  // Diagnostics.
  ddprintf(("[GDS] uninit_driver called.\n"));


  // De-initialize device_data.

  // De-initialize devices, if any.
  for (index = 0; index < driver_data.device_count; index++)
  {
    uninit_device(&(driver_data.di[index]));
  }

  // Dispose of the driver locking benaphore, if initialized.
  if (driver_data.driver_benaphore.sem >= 0)
  {
    DISPOSE_GDS_BENAPHORE(&(driver_data.driver_benaphore));
  }

  // Put the pci module away if we loaded it.
  if (driver_data.pci_bus != NULL)
  {
    put_module(B_PCI_MODULE_NAME);
  }
}


//////////////////////////////////////////////////////////////////////////////
// Publish Devices
//    This returns a list of supported devices.
//    HACK - This is the only kernel hook that has () instead of (void) in
// Drivers.h.

const char **publish_devices()
{
  // Diagnostics.
  ddprintf(("[GDS] publish_devices called.\n"));

  return (const char **)driver_data.device_names;
}


//////////////////////////////////////////////////////////////////////////////
// Find a Device
//    This looks for the device that matches the input string, and returns a
// table to its file-hook functions if the device is found. In practice, we
// just have one table of file hooks that apply to all devices.

device_hooks *find_device(const char *name)
{
  int index = 0;

  // Diagnostics.
  ddprintf(("[GDS] find_device called.\n"));

  // Find the requested device, if it exists.
  while(driver_data.device_names[index])
  {
    if(strcmp(name, driver_data.device_names[index]) == 0)
    {
      return &graphics_device_hooks;
    }
    index++;
  }

  return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
