//////////////////////////////////////////////////////////////////////////////
// General Vertical Blanking Interrupt Code
//
//    This file implements kernel driver functions related to the vertical
// blanking interrupt, as well as the hardware-independent components of the
// vertical blanking interrupt handler.
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
// Generic Driver Vertical Blanking Interupt Work
//    This function performs generic driver functions that must be attended
// to during the vertical blanking interrupt. For the time being, this means
// releasing threads that are blocked on the VBI semaphore, and setting the
// palette table if so indicated.
//    This returns B_HANDLED_INTERRUPT, B_INVOKE_SCHEDULER, or other return
// values appropriate to an interrupt service routine.

__inline int32 generic_vblank_work(SHARED_INFO *si)
{
  int32 *flags = &(si->vbi.flags);
  int32 result = B_HANDLED_INTERRUPT;


  // Program the CLUT, if requested.
  if(atomic_and(flags, ~GDS_PROGRAM_CLUT) & GDS_PROGRAM_CLUT)
    {
      set_palette(si->display.first_color, si->display.color_count,
        si->display.color_data + (si->display.first_color * 3), si);
    }

  // Release the vertical blanking semaphore, if necessary.
  if(si->vbi.vblank >= 0)
    {
      int32 blocked;

      // Do an error check here, as the owning team may have died.
      if((get_sem_count(si->vbi.vblank, &blocked) == B_OK) && (blocked < 0))
        {
          release_sem_etc(si->vbi.vblank, -blocked, B_DO_NOT_RESCHEDULE);
          result = B_INVOKE_SCHEDULER;
        }
    }

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Vertical Blanking Interupt Work - Wrapper
//    This function is a wrapper for the generic and hardware-specific
// vertical blanking interrupt work functions. Providing this makes it easier
// to ensure that the real and fake interrupt handlers always call the same
// functions. It's set as an inline, and so should impose no extra overhead.
//    This returns B_HANDLED_INTERRUPT, B_INVOKE_SCHEDULER, or other return
// values appropriate to an interrupt service routine.

__inline int32 vblank_interrupt_work(SHARED_INFO *si)
{
  int32 result;

  // Perform hardware-specific functions that must be attended to during the
  // vertical blanking interrupt.
  hardware_specific_vblank_work(si);

  // Perform generic driver functions that must be attended to during the
  // vertical blanking interrupt.
  result = generic_vblank_work(si);

  return result;
}


//////////////////////////////////////////////////////////////////////////////
// Interrupt handler
//    This function is called every time a hardware interrupt for the VBI
// occurs. It's also called for other interrupts on the same line, which we
// don't handle.

int32 card_interrupt_handler(void *data)
{
  int32 handled = B_UNHANDLED_INTERRUPT; // Default return value.
  DEVICE_INFO *di = (DEVICE_INFO *)data;
  SHARED_INFO *si = di->si;

  // Sanity check, in case we haven't finished servicing an old interrupt.
  if (atomic_or(&(di->kernel_vbi.handler_mutex), 0x01))
    {
      // Bail if an old instance of the handler is still active.
      return B_UNHANDLED_INTERRUPT;
    }


  // did this card cause an interrupt?

  if(CARD_CAUSED_VB_INTERRUPT)
    {
      // do our stuff

      handled = vblank_interrupt_work(si);

      // clear the interrupt status

      CLEAR_VB_INTERRUPT;
    }

  // Note that we're no longer in the interrupt handler.
  atomic_and(&(di->kernel_vbi.handler_mutex), ~(0x01l));

  return handled;                               
}


//////////////////////////////////////////////////////////////////////////////
// Fake Interupt Thread Function
//    This function emulates interrupt calls by running a loop synchronized
// to the vertical blanking interval and calling the interrupt handler on
// each pass if interrupts are enabled.

int32 fake_interrupt_thread_func(void *_di)
{
  DEVICE_INFO *di = (DEVICE_INFO *)_di;
  SHARED_INFO *si = di->si;
  bigtime_t last_sync;
  bigtime_t this_sync;
  bigtime_t diff_sync;
  uint32 counter = 1;

  // Diagnostics.
  ddprintf(("GDS fake_interrupt_thread_func begins\ndi: 0x%08lx\n handler_installed: %d\n", (uint32) di, di->kernel_vbi.fake_handler_installed));


  // Initialize the time of the last VBI to a plausible value.
  last_sync = system_time() - 8333;


  // Loop as long as the fake interrupt handler is told to be active.

  while(di->kernel_vbi.fake_handler_installed)
    {
      // Wait for the vertical blanking interval.
      POLL_FOR_VBI;


      // If interrupts are enabled, call the interrupt servicing function.
      if(di->kernel_vbi.can_interrupt)
        vblank_interrupt_work(si);


      // Get the system_time.
      this_sync = system_time();

      // Find out how long it's been since the last blanking interval.
      diff_sync = this_sync - last_sync;

      // Back off a little so we're sure to catch the retrace.
      diff_sync -= diff_sync / 10;


      // Impose some limits for sanity's sake.
      // Supported refresh rates are 48 Hz - 120 Hz, so these limits should
      // be slightly wider.

      if(diff_sync < 8000)
        {
          diff_sync = 8000; // Not less than 1/125th of a second.
        }

      if(diff_sync > 16666)
        {
          diff_sync = 20000; // Not more than 1/40th of a second.
        }

      // To recover from refresh rate changes, periodically quarter the
      // delay so that we can resync.
      // NOTE: In pathalogical cases, refresh syncing will get stuck on a
      // multiple of the real vertical refresh period until this resync is
      // performed. This happens only once every 256 refresh cycles, so
      // effects will persist for a few seconds until resync.
      if((counter++ & 0xff) == 0)
        {
          diff_sync >>= 2;
        }

      // Update for next go-around.
      last_sync = this_sync;

      // Snooze until our next retrace.
      snooze_until(this_sync + diff_sync, B_SYSTEM_TIMEBASE);
    }


  // Diagnostics.
  ddprintf(("GDS fake_interrupt_thread_func ends with handler_installed = %d\n", di->kernel_vbi.fake_handler_installed));

  // Return success.
  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
