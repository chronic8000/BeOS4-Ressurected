//////////////////////////////////////////////////////////////////////////////
// Driver File Hooks
//
//    This file implements the hook functions that are called when a device
// in /dev is manipulated as a file.
//
//    NOTE: control_hook, which handles IOCTL calls, is implemented in its
// own file.
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
// Open a graphics device.
//    This is called when a graphics device in /dev is opened. The cookie
// value returned is a pointer to the device_info structure for this device.
//    Returns B_OK if successful or another value if not.

// HACK - This still contains legacy cruft. We should allocate a new
// semaphore for this team if we don't already have one, and return a cookie
// that contains this semaphore and points to the device_info record.

status_t open_hook(const char* name, uint32 flags, void** cookie)
{
  DEVICE_INFO *di;
  SHARED_INFO *si;
  thread_id     thid;
  thread_info   thinfo;
  int index;


  // Diagnostics.
  ddprintf(("[GDS] open_hook called (%s, 0x%08lx, 0x%08lx).\n", name,
    (uint32) flags, (uint32) cookie));


  // Find the device name in the list of devices.
  for (index = 0;
    (driver_data.device_names[index] != NULL)
      && (strcmp(name, driver_data.device_names[index]) != 0);
    index++);

  // Record DEVICE_INFO and SHARED_INFO pointers.
  di = &(driver_data.di[index]);
  si = di->si;


  // Make sure the name was valid. In theory, it always is, but this test
  // doesn't cost us much.
  if (driver_data.device_names[index] == NULL)
  {
    // Diagnostics.
    ddprintf(("[GDS] Unable to find specified device (open_hook).\n"));
    // Return failure.
    return B_ERROR;
  }


  // Make sure that this device is not in use.

  // HACK: This check is what keeps us from opening our drivers multiple
  // times. It's needed for the moment, but we'll have to build support for
  // multiple opening in in the near future.

  if(di->is_open)
  {
    // Return failure.
    return B_BUSY;
  }


  // Give the vertical blanking semaphore to the opener's team.

  // HACK - This should be a new/duplicate semaphore, not the main semaphore.
  // I have no idea why the semaphore ownership needs to be changed in the
  // first place.

  thid = find_thread(NULL);
  get_thread_info(thid, &thinfo);
  set_sem_owner(si->vbi.vblank, thinfo.team);



  // Note that we're using the device.
  di->is_open = 1;

  // Set the cookie value.
  *cookie = di;


  // Done. Return success.        
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Close a graphics device.
//    This is called when a graphics device in /dev is closed. "dev" is a
// pointer to the device_info structure for this device.

status_t close_hook(void* dev)
{
  ddprintf(("[GDS] close_hook(%08lx)\n", (uint32) dev));

  // Don't do anything, as dup'd file descriptors might still be in use.

  return B_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// Free a graphics device's cookie.
//    This is called when no open file handles remain to a given graphics
// device from a given team. "dev" is a pointer to the device_info structure
// for this device.
//    NOTE: I *think* that these are the conditions under which this function
// is called. This will be important when we implement proper cookie
// management.

// HACK - This still contains legacy cruft. We should perform cookie cleanup
// here when we implement proper cookies. We'll need proper locking for this,
// too.

status_t free_hook(void* dev)
{
  DEVICE_INFO *di = (DEVICE_INFO *)dev;

  // Diagnostics.
  ddprintf(("[GDS] free_hook() called.\n"));


  // Note that we're no longer using the device.
  di->is_open = 0;


  // Diagnostics.
  ddprintf(("[GDS] free_hook() done.\n"));

  // Done. Return success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Read from a graphics device.
//    This is called when a graphics device in /dev is read from. "dev" is a
// pointer to the device_info structure for this device.

status_t read_hook(void* dev, off_t pos, void* buf, size_t* len)
{
  // Do nothing, gracefully.
  *len = 0;
  return B_NOT_ALLOWED;
}


//////////////////////////////////////////////////////////////////////////////
// Write to a graphics device.
//    This is called when a graphics device in /dev is written to. "dev" is a
// pointer to the device_info structure for this device.

status_t write_hook(void* dev, off_t pos, const void* buf, size_t* len)
{
  // Do nothing, gracefully.
  *len = 0;
  return B_NOT_ALLOWED;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
