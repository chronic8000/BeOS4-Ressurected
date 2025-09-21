//////////////////////////////////////////////////////////////////////////////
// General Vertical Blanking Interrupt Code
//
//    This file declares kernel driver functions related to the vertical
// blanking interrupt, as well as the hardware-independent components of the
// vertical blanking interrupt handler.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Prototypes ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Generic Driver Vertical Blanking Interupt Work
//    This function performs generic driver functions that must be attended
// to during the vertical blanking interrupt. For the time being, this means
// releasing threads that are blocked on the VBI semaphore, and setting the
// palette table if so indicated.
//    This returns B_HANDLED_INTERRUPT, B_INVOKE_SCHEDULER, or other return
// values appropriate to an interrupt service routine.

__inline int32 generic_vblank_work(SHARED_INFO *si);


//////////////////////////////////////////////////////////////////////////////
// Vertical Blanking Interupt Work - Wrapper
//    This function is a wrapper for the generic and hardware-specific
// vertical blanking interrupt work functions. Providing this makes it easier
// to ensure that the real and fake interrupt handlers always call the same
// functions. It's set as an inline, and so should impose no extra overhead.
//    This returns B_HANDLED_INTERRUPT, B_INVOKE_SCHEDULER, or other return
// values appropriate to an interrupt service routine.

__inline int32 vblank_interrupt_work(SHARED_INFO *si);


//////////////////////////////////////////////////////////////////////////////
// Interrupt handler
//    This function is called every time a hardware interrupt for the VBI
// occurs. It's also called for other interrupts on the same line, which we
// don't handle.

int32 card_interrupt_handler(void *data);


//////////////////////////////////////////////////////////////////////////////
// Fake Interupt Thread Function
//    This function emulates interrupt calls by running a loop synchronized
// to the vertical blanking interval and calling the interrupt handler on
// each pass if interrupts are enabled.

int32 fake_interrupt_thread_func(void *_di);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
