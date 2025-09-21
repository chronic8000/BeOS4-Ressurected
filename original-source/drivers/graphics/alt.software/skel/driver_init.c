//////////////////////////////////////////////////////////////////////////////
// Driver Initialization
//
//    This file implements helper functions used for kernel driver
// initialization.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <driver_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialize a Device Record
//    This function initializes a DEVICE_INFO record to blank values and
// creates an associated SHARED_INFO record, also initialized to blank
// values. "id_number" is an identification signature used to label areas
// dynamically allocated for this DEVICE_INFO record.
//    Returns B_OK if successful or another value if not.

status_t init_device_record(DEVICE_INFO *di, uint32 id_number)
{
  char shared_name[B_OS_NAME_LENGTH];
  size_t shared_info_size;
  SHARED_INFO *si;


  // Init DEVICE_INFO fields.


  // General device-related information.

  // Device has not been opened.
  di->is_open = 0;

  // Blank device name.
  di->name[0] = 0;

  // NOTE: Not initializing the PCI_INFO structure.


  // SHARED_INFO structure, first pass.

  // No SHARED_INFO structure yet.
  di->shared_area = -1;
  di->si = NULL;


  // Variables related to the vertical blanking interrupt.

  // Can't generate interrupts.
  di->kernel_vbi.can_interrupt = 0;

  // Interrupt handler is not active.
  di->kernel_vbi.handler_mutex = 0;

  // No fake interrupt thread.
  di->kernel_vbi.tid = -1;
  di->kernel_vbi.fake_handler_installed = 0;



  // Allocate the SHARED_INFO structure.


  // Pick a name for this area.
  sprintf(shared_name,
          "GDS shared info id %08lx",
          id_number);

  // Calculate the amount of space required for a SHARED_INFO structure,
  // rounding up to an integral number of pages.
  shared_info_size = sizeof(SHARED_INFO) + B_PAGE_SIZE - 1;
  shared_info_size -= shared_info_size & (B_PAGE_SIZE - 1);

  // Diagnostics.
  ddprintf(("[GDS] Allocating %lu bytes for SHARED_INFO structure (requires \
    %lu bytes).\n", (uint32) shared_info_size, (uint32)                     \
    sizeof(SHARED_INFO) ));

  // Allocate the area.
  di->shared_area = create_area(shared_name,
                                (void **)&(di->si),
                                B_ANY_KERNEL_ADDRESS,
                                shared_info_size,
                                B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);

  // Bail if we couldn't allocate the area.
  if((di->shared_area < 0) || (di->si == NULL))
  {
    // Return the error value we were given.
    return di->shared_area;
  }



  // Initialize SHARED_INFO fields.
  //    NOTE: Most of this isn't really necessary, as we should be
  // re-initializing it elsewhere, but this will make errors more
  // deterministic if we forget to initialize anything down the road.


  // For convenience.
  si = di->si;


  // Card-related parameters.

  // Blank device information.
  si->card.vendor_id = 0;
  si->card.device_id = 0;
  si->card.revision = 0;

  // No interrupt line.
  si->card.interrupt_line = 0xFF;

  // No register area.
  si->card.regs_area = -1;
  si->card.regs = NULL;
  si->card.reg_aperture_size = 0;

  // No frame buffer area.
  si->card.fb_area = -1;
  si->card.fb_base = NULL;
  si->card.fb_base_pci = 0;
  si->card.fb_aperture_size = 0;

  // No memory.
  si->card.mem_size = 0;

  // Blank pixel clock limits.
  si->card.pix_clk_max8 = 0;
  si->card.pix_clk_max16 = 0;
  si->card.pix_clk_max32 = 0;


  // Variables related to 2D drawing engine management.

  // Blank FIFO information.
  si->engine.last_idle_fifo = 0;
  si->engine.fifo_count = 0;
  si->engine.fifo_limit = 0;

  // No engine benaphore.
  si->engine.engine_ben.sem = -1;
  si->engine.engine_ben.lock = 0;


  // Variables related to vertical blanking interrupt handling.
  si->vbi.vblank = -1;
  si->vbi.flags = 0;


  // Variables related to hardware cursor management.

  // NOTE: Not initializing cursor images.

  si->cursor.cursor_x = 0;
  si->cursor.cursor_y = 0;
  si->cursor.hot_x = 0;
  si->cursor.hot_y = 0;


  // Variables related to CRT display mode and frame buffer configuration.

  // NOTE: Not initializing the current display mode configuration.
  // NOTE: Not initializing the current frame buffer pitch and offset.
  // NOTE: Not initializing palette setting data.


  // NOTE: Not initializing HARDWARE_SPECIFIC_SHARED_INFO struct.


  // Return success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Delete a Device Record
//    This function de-allocates all dynamically allocated entities
// associated with the specified DEVICE_INFO record.

void delete_device_record(DEVICE_INFO *di)
{
  // Assume that most de-initialization has already been performed, and we
  // just have to handle de-allocation of dynamically allocated memory.

  // Deal with si, if it's defined.
  if (di->shared_area >= 0)
  {
    // Assume that semaphores and benaphores have already been
    // de-initialized.

    // Delete the shared area.
    delete_area(di->shared_area);
  }
}


//////////////////////////////////////////////////////////////////////////////
// Turn on PCI access to the device.
//    This function enables PCI access to the device (through memory mapped
// apertures, etc.).

void enable_pci_access(DEVICE_INFO *di)
{
  uint32 LTemp;

  // Enable desired PCI access features. For now, this is just memory
  // mapping.
  
  LTemp = get_pci(PCI_DEVCTRL, 4);
  LTemp &= 0xfffffff8; // Turn off bus mastering, memory mapping, and i/o port mapping.

  // HACK: Bus mastering was apparently causing problems for an earlier
  // version of the (Matrox?) driver. Keep it turned off, but bear in mind
  // that this will cause problems for drivers that actually need it. Modify
  // this at some point to depend on a flag.

  LTemp |= 0x00000002; // Turn on memory mapping.
  set_pci(PCI_DEVCTRL, 4, LTemp);
}


//////////////////////////////////////////////////////////////////////////////
// Disable PCI access to the device.
//    This function disables PCI access to the device (reads/writes to memory
// mapped apertures are no longer decoded, etc.).

void disable_pci_access(DEVICE_INFO *di)
{
  uint32 LTemp;

  // Clear appropriate bits in the PCI device control register.

  LTemp = get_pci(PCI_DEVCTRL, 4);
  LTemp &= 0xfffffff8; // Turn off bus mastering, memory mapping, and i/o port mapping.
  set_pci(PCI_DEVCTRL, 4, LTemp);
}


//////////////////////////////////////////////////////////////////////////////
// Map the device's apertures.
//    This function maps the specified device's PCI memory apertures into
// virtual memory space.
//    Returns B_OK if successful or another value if not.

status_t map_device(DEVICE_INFO *di)
{
  void *fb_pointer, *reg_pointer;
  uint32 fb_size, reg_size;
  SHARED_INFO *si = di->si;
  char buffer[B_OS_NAME_LENGTH];


  // Extract frame buffer and register aperture pointers from PCI configuration
  // space. This usually follows standard conventions, but not always, so it
  // has to be considered hardware-specific.
  GetCardPointers(&(di->pcii), &fb_pointer, &fb_size, &reg_pointer, &reg_size);

  // Adjust the relevant sizes to be multiples of B_PAGE_SIZE, which is a power
  // of two.
  fb_size = (fb_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
  reg_size = (reg_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);



  // Initialize the aperture fields in shared_info.

  si->card.regs_area = -1;
  si->card.regs = NULL;
  si->card.reg_aperture_size = 0;

  si->card.fb_area = -1;
  si->card.fb_base = NULL;
  si->card.fb_base_pci = NULL;
  si->card.fb_aperture_size = 0;



  // Map the areas.

  // NOTE: Fix the naming convention for these at some point to use the
  // exported device names (hopefully it won't mind / characters).

  // Map the register area only if we receive a non-NULL pointer and a positive
  // size.
  if ((reg_pointer != NULL) && (reg_size > 0))
  {
    sprintf(buffer,
            "%04X_%04X_%02X%02X%02X regs",
            di->pcii.vendor_id,
            di->pcii.device_id,
            di->pcii.bus,
            di->pcii.device,
            di->pcii.function);

    si->card.regs_area
      = map_physical_memory(buffer,
                            reg_pointer,
                            reg_size,
                            B_ANY_KERNEL_ADDRESS,
                            B_READ_AREA + B_WRITE_AREA,
                            (void **)&(si->card.regs));

    // If there was an error, return the error code.
    if (si->card.regs_area < 0)
    {
      ddprintf(("[GDS] Error - unable to map %s.\n", buffer));
      return si->card.regs_area;
    }

    // If we're ok, record the nominal aperture size as well.
    si->card.reg_aperture_size = reg_size;
  }


  // Map the frame buffer no matter what. It had better be valid.

  sprintf(buffer,
          "%04X_%04X_%02X%02X%02X frame buffer",
          di->pcii.vendor_id,
          di->pcii.device_id,
          di->pcii.bus, di->pcii.device,
          di->pcii.function);

  // NOTE: B_ANY_KERNEL_BLOCK_ADDRESS is only valid for late betas of R4.1
  // and later releases. We used B_ANY_KERNEL_ADDRESS before.

  si->card.fb_area 
    = map_physical_memory(buffer,
                          fb_pointer,
                          fb_size,
#if defined(__INTEL__)
                          // Try to map with write combining on the Intel
                          // platform.
                          B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
                          B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
                          B_READ_AREA + B_WRITE_AREA,
                          &(si->card.fb_base));

#if defined(__INTEL__)
  // If mapping with write combining failed, try mapping without it.
  if(si->card.fb_area < 0)
  {
    si->card.fb_area 
      = map_physical_memory(buffer,
                            fb_pointer,
                            fb_size,
                            B_ANY_KERNEL_BLOCK_ADDRESS,
                            B_READ_AREA + B_WRITE_AREA,
                            &(si->card.fb_base));
  }
#endif

  // If there was an error, delete the register area and return the error code.
  if(si->card.fb_area < 0) 
  {
    delete_area(si->card.regs_area);
    si->card.regs_area = -1;

    ddprintf(("[GDS] Error - unable to map %s.\n", buffer));
    return si->card.fb_area;
  }


  // If we're ok, record the aperture size as well.
  si->card.fb_aperture_size = fb_size;

  // Remember the DMA (physical) address of the frame buffer for
  // BDirectWindow purposes.
  si->card.fb_base_pci = fb_pointer;


  // Return success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Unmap the device's apertures.
//    This function deletes the specified device's memory-mapped apertures
// from virtual memory space.

void unmap_device(DEVICE_INFO *di)
{
  SHARED_INFO *si = di->si;

  // Delete the register aperture, if it was allocated.
  if(si->card.regs_area >= 0) delete_area(si->card.regs_area);

  // Delete the frame buffer aperture, if it was allocated.
  if(si->card.fb_area >= 0) delete_area(si->card.fb_area);


  // Reset register and frame buffer pointers and area ID values.

  si->card.regs_area = -1;
  si->card.regs = NULL;
  si->card.reg_aperture_size = 0;

  si->card.fb_area = -1;
  si->card.fb_base = NULL;
  si->card.fb_base_pci = NULL;
  si->card.fb_aperture_size = 0;
}


//////////////////////////////////////////////////////////////////////////////
// Set up interrupts for the device.
//    This function initializes interrupt handling for the specified device.
//    Returns B_OK if successful or another value if not.

status_t init_device_interrupts(DEVICE_INFO *di)
{
  SHARED_INFO *si = di->si;
  status_t ErrCode;

  // Create a semaphore for vertical blanking interval management.
  // HACK: For now, this semaphore will be given to the team performing the
  // first opening of this device. In the future, this semaphore should be
  // used internally to synchronize the release of caller semaphores.

  si->vbi.vblank = create_sem(0, di->name);

  // Proceed only if successful.
  if (si->vbi.vblank < 0)
  {
    // Failure.

    // Diagnostics.
    ddprintf(("[GDS] Error - Unable to create vertical blanking " \
      "semaphore (init_device_interrupts).\n"));

    // Return the error code.
    ErrCode = si->vbi.vblank;
  }
  else
  {
    // Disable and clear any pending interrupts. Shouldn't be required, but
    // do it anyways.
    DISABLE_CARD_INTERRUPTS;

    // The interrupt line has already been recorded in si->card.interrupt_line.

    // Initialize the interrupt handler mutex.
    di->kernel_vbi.handler_mutex = 0;

    // Set up the interrupt handler.

    // See if we have a valid interrupt line.
    if(si->card.interrupt_line == 0xff)
    {
      // We don't have an interrupt line. Fake interrupts using a polling
      // thread.

      // Fake interrupt thread defaults to inactive.
      di->kernel_vbi.can_interrupt = 0;

      // We're using a fake interrupt thread.
      di->kernel_vbi.fake_handler_installed = 1;

      // Create the thread.
      di->kernel_vbi.tid = spawn_kernel_thread(
        fake_interrupt_thread_func,
        "GDS fake interrupt",
        B_REAL_TIME_DISPLAY_PRIORITY,
        di);

      // Proceed if we were successful.
      if (di->kernel_vbi.tid < 0)
      {
        // Failure.

        // Diagnostics.
        ddprintf(("[GDS] Error - Unable to create fake vertical blanking" \
          "thread (init_device_interrupts).\n"));

        // Return the error code.
        ErrCode = di->kernel_vbi.tid;
      }
      else
      {
        // Thread successfully created.

        // Start the thread.
        resume_thread(di->kernel_vbi.tid);

        // Return success and exit.
        return B_OK;
      }
    }
    else
    {
      // We do have an interrupt line. Install a real handler.

      // No fake interrupt thread.
      di->kernel_vbi.fake_handler_installed = 0;

      // Install the handler.
      install_io_interrupt_handler(si->card.interrupt_line,
        card_interrupt_handler, (void *)di, 0);

      // Return success and exit.
      return B_OK;
    }

    // Initialization wasn't successful; delete the blanking semaphore.
    delete_sem(si->vbi.vblank);
    si->vbi.vblank = -1;
  }


  // Initialization wasn't successful.
  return ErrCode;
}


//////////////////////////////////////////////////////////////////////////////
// Unhook and remove interrupts for the device.
//    This function removes interrupt handling for the specified device.

void uninit_device_interrupts(DEVICE_INFO *di)
{
  SHARED_INFO *si = di->si;

  // Disable interrupts for this card and clear any pending interrupts.
  DISABLE_CARD_INTERRUPTS;

  // See if we have a real IRQ line.
  if(si->card.interrupt_line == 0xff)
  {
    // We don't have an interrupt line.
    // Stop our interrupt faking thread

    // Disable "interrupts".
    di->kernel_vbi.can_interrupt = 0;

    // Disable the handler. It'll return within a few tens of milliseconds at
    // worst.
    di->kernel_vbi.fake_handler_installed = 0;
  }
  else // remove interrupt handler
  {
    // We do have an interrupt line.
    // Remove the interrupt handler.
    remove_io_interrupt_handler(si->card.interrupt_line,
      card_interrupt_handler, di);
  }


  // Delete the vertical blanking semaphore.
  // Ignore any errors, as the owning team may have died on us.
  // HACK - In the future, we should always own this semaphore.
  delete_sem(si->vbi.vblank);
  si->vbi.vblank = -1;
}


//////////////////////////////////////////////////////////////////////////////
// Update SHARED_INFO from DEVICE_INFO
//    This function updates a DEVICE_INFO record's SHARED_INFO structure
// based on the information contained within the DEVICE_INFO structure.

void update_si_from_di(DEVICE_INFO *di)
{
  // Sanity check on si.
  if (di->si == NULL)
  {
    ddprintf(("[GDS] Error - Invalid si pointer (update_si_from_di).\n"));
  }

  // si seems to be a valid pointer. Proceeed.

  // Update PCI-related information.
  di->si->card.vendor_id = di->pcii.vendor_id;
  di->si->card.device_id = di->pcii.device_id;
  di->si->card.revision = di->pcii.revision;
  di->si->card.interrupt_line = di->pcii.u.h0.interrupt_line;

  // That's all we can do automatically.
}


//////////////////////////////////////////////////////////////////////////////
// Probe Devices
//    This detects all supported devices in PCI space and sets up DEVICE_INFO
// entries for them in driver_data.di. SHARED_INFO records are also
// allocated.

void probe_devices(void)
{
  pci_info pcii;
  DEVICE_INFO *di = driver_data.di;
  uint32 count = 0;
  uint32 id_number;
  long pci_index;
  int index;

  // Diagnostics.
  ddprintf(("[GDS] probe_devices called.\n"));


  // Test all devices found in PCI space.
  for(pci_index = 0;
    (count < GDS_MAX_DEVICES) &&
    ((*driver_data.pci_bus->get_nth_pci_info)(pci_index, &pcii)
      == B_NO_ERROR);
    pci_index++)
  {
    // See if this device is in our list of supported devices.

    for(index = 0 ; SupportedDevices[index].vendor_id ; index++)
    {
      if((SupportedDevices[index].vendor_id == pcii.vendor_id)
        && (SupportedDevices[index].device_id == pcii.device_id))
      {
        // Initialize the ID number used to identify areas dynamically
        // allocated for this DEVICE_INFO record.
        id_number = ((uint32) pcii.bus) << 24 | ((uint32) pcii.device) << 16
          | ((uint32) pcii.function << 8);


        // Try to initialize a DEVICE_INFO record for this device.
        // Proceed if successful.
        if (init_device_record(di, id_number) != B_OK)
        {
          ddprintf(("[GDS] Error - Unable to initialize device record.\n"));
          // This shouldn't happen.
          // Skip this device and don't increment our device counter.
        }
        else
        {
          // Update di.

          // Store a copy of the PCI info structure in di.
          di->pcii = pcii;


          // Generate the device name.
          //    This specifies the device name within the /dev/graphics
          // directory. The path, if any, from GRAPHICS_DEVICE_PATH
          // is appended to /dev/graphics, and then our custom name.
          //    We presently build a name by taking the name prefix from
          // SupportedDevices and tacking on a few digits representing
          // the PCI address of the device to guarantee uniqueness. The
          // standard BeOS convention, OTOH, is to append "/1", "/2",
          // etc. to the device names. This code will be switched over
          // eventually.

          // NOTE: The device_name field in SupportedDevices had better
          // not be NULL here!.
          // NOTE: We'd better hope that we don't run into name length
          // problems with sprintf.

          // Store the device name in di->name.
          sprintf(di->name,
            "graphics%s/%s_%02X%02X%02X",
            GRAPHICS_DEVICE_PATH,
            SupportedDevices[index].device_name,
            di->pcii.bus,
            di->pcii.device,
            di->pcii.function);

          // Store a pointer to the device name in device_names, also.
          driver_data.device_names[count] = di->name;

          // Diagnostics.
          ddprintf(("[GDS] Adding /dev/%s\n", di->name));


          // Update di->si.
          update_si_from_di(di);


          // Increment our device counter and move on to the next record.
          count++;
          di++;
        }
      }
    }
  }

  // Record the device count.
  driver_data.device_count = count;

  // Terminate list of device names with a null pointer
  driver_data.device_names[driver_data.device_count] = NULL;

  // Diagnostics.
  ddprintf(("[GDS] probe_devices: %lu supported devices\n",
    (uint32) driver_data.device_count));
}


//////////////////////////////////////////////////////////////////////////////
// Initialize a Device
//    This function initializes the specified device, allocates dynamic
// variables needed by the device, and updates di and si appropriately.
// Returns B_OK if successful or another value if not.

status_t init_device(DEVICE_INFO *di)
{
  status_t ErrCode;

  // Diagnostics.
  ddprintf(("[GDS] init_device called.\n"));


  // The device record has already been initialized to blank values.
  // The device PCI information has already been loaded into di.si.

  // Turn on PCI access to the device.
  enable_pci_access(di);

  // Map the device's apertures.
  ErrCode = map_device(di);

  // Proceed if successful.
  if (ErrCode == B_OK)
  {
    // HACK - This is where hardware initialization belongs. For now, it's
    // still in the accelerant.


    // Set up interrupts for the device.
    ErrCode = init_device_interrupts(di);

    // Proceed if successful.
    if (ErrCode == B_OK)
    {
      // Return success and exit.
      return B_OK;
    }

    // Initialization failed. De-initialize.
    unmap_device(di);
  }

  // Initialization failed. De-initialize.
  disable_pci_access(di);

  // Initialization failed. Return the error code.
  return ErrCode;
}


/////////////////////////////////////////////////////////////////////////////
// Uninitialize a Device
//    This function de-initializes the specified device, releases dynamic
// variables allocated for the device, and updates di appropriately (si is
// de-allocated).

void uninit_device(DEVICE_INFO *di)
{
  // Diagnostics.
  ddprintf(("[GDS] uninit_device called.\n"));


  // Unhook and remove interrupts for the device.
  uninit_device_interrupts(di);


  // HACK - This is where hardware de-initialization belongs.


  // Unmap the device's apertures.
  unmap_device(di);

  // Disable PCI access to the device.
  disable_pci_access(di);

  // Get rid of this device's DEVICE_INFO record.
  delete_device_record(di);
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
