//////////////////////////////////////////////////////////////////////////////
// General IOCTL Handler
//
//    This file contains the kernel driver ioctl handler for non-hardware-
// -specific IOCTL calls.
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
// Control Hook
//    This handles IOCTL calls made to a graphics device. This is the primary
// means of communication between user-space accelerants and the kernel
// driver. "dev" is a pointer to the device_info structure for this this
// device.

status_t control_hook(void* dev, uint32 msg, void *buf, size_t len)
{
  DEVICE_INFO *di = (DEVICE_INFO *)dev;
  SHARED_INFO   *si = di->si;
  status_t result = B_DEV_INVALID_IOCTL;

  // ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

  switch(msg)
  {
    // the only PUBLIC ioctl

    case B_GET_ACCELERANT_SIGNATURE:
      {
        char *sig = (char *)buf;

        // Return the name of the accelerant to be used with this driver.

        // HACK - No length checking. There had better be enough space in the
        // specified buffer.
        strcpy(sig, ACCELERANT_NAME);

        result = B_OK;
      }
      break;

                
    // PRIVATE ioctl from here on

    case GDS_GET_PRIVATE_DATA:
      {
        gds_get_private_data *gpd = (gds_get_private_data *)buf;

        // Return a pointer to the SHARED_INFO structure.

        if(gpd->magic == GDS_IOCTL_MAGIC)
        {
          gpd->si = si;
          result = B_OK;
        }
      }
      break;

    case GDS_GET_PCI:
      {
        gds_get_set_pci *gsp = (gds_get_set_pci *)buf;

        // Read a value from PCI configuration space.
      
        if(gsp->magic == GDS_IOCTL_MAGIC)
        {
          gsp->value = get_pci(gsp->offset, gsp->size);
          result = B_OK;
        }
      }
      break;

    case GDS_SET_PCI:
      {
        gds_get_set_pci *gsp = (gds_get_set_pci *)buf;
        
        // Write a value to PCI configuration space.

        if(gsp->magic == GDS_IOCTL_MAGIC)
        {
          set_pci(gsp->offset, gsp->size, gsp->value);
          result = B_OK;
        }
      }
      break;

    case GDS_RUN_INTERRUPTS:
      {
        gds_set_bool_state *ri = (gds_set_bool_state *)buf;

        // Set the "interrupt enable" state.

        if(ri->magic == GDS_IOCTL_MAGIC)
        {
          // See if we have a real interrupt line or not.
          if(di->si->card.interrupt_line == 0xff)
          {
            // We don't have a real interrupt line. Signal the fake
            // interrupt thread to generate/not generate "interrupts".
            di->kernel_vbi.can_interrupt = ri->do_it;
          }
          else
          {
            // We have a real interrupt thread. Call the appropriate
            // hardware-specific macro.
            if(ri->do_it)
            {
              // Resume interrupts.
              RESUME_CARD_INTERRUPTS;
            }
            else
            {
              // Disable interrupts.
              DISABLE_CARD_INTERRUPTS;
            }
          }

          result = B_OK;
        }
      }
      break;

    default:
      // We don't recognize this opcode, so assume that it's a driver-specific
      // opcode and pass it on to the driver-specific handler.
      result = Handle_Hardware_Specific_Ioctl(di, msg, buf, len);
  }

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
