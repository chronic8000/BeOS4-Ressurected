/*========================================================================
   File			:	serial.c
 
   Purpose		:	Implementation file for the serial port interace.  This
   					file contains all the functions the BeOS operating system
   					needs to interface with our modem driver. 
   					
   					The functions in this file are grouped in the following
   					way : Utilities used by the hook functions, the hook
   					functions, the driver functions and the GMC interface 
   					functions.
   					
   					The Utilities are functions that are used by the other 
   					functions to make the code more modular and 
   					comprehensible.
   					
   					The hook functions are the basic open, close, free, read,
   					write and control functions which are implemented in 
   					every driver in the BeOS architecture.  It's via these
   					functions that external applications will be able to
   					interface with our modem driver.
   					
   					The driver functions are the function the BeOS operating
   					system will use to load and unload our driver from 
   					memory.
   					
   					The GMC interface functions are the function the GMC 
   					will call to have access to the serial ports data qeueus
   					and the	control registers.
         
   NOTE			:   In the header file serial.h, there are several defines
   					that enable or disable certain options.  The most 
   					important of theses are the USESEM, USEALLOC and the
   					USEINITHW.  I'll explain theses here.
   					
   					USESEM will add/enable all the semaphores in the code 
   					that protect the hook functions from being called more
   					then once at a time.
   					
   					USEALLOC will put the serial FIFO buffers in a 
   					dynamically allocated array.  This appears to malfunction
   					under certain platforms however.
   					
   					USEINITHW enables a hardware scan inside the 
   					init_hardware driver function.  This way the driver
   					will never be loaded into memory (and the port will not
   					be available) if no supported hardware is found.  
   					However, this hardware scan will be performed again
   					by the GMC when the port will be opened.
   					
   					USEFORCEDEBUG will turn on the debug trace output on
   					the BeBox's com port and USEDEBUG will enable all the
   					debug prints inside serial.c and ddalapi.cpp.  I 
   					seperated this functionality into two seperate defines
   					so that we can scan for error debug prints caused by 
   					other applications while our software is running without
   					having to navigate through our own debug prints.  Note
   					that the com port will be put back to its initial state
   					when the driver will be unloaded from memory.
   
   Author		:	Bryan Pong, 2000-05

   Modifications:	Extensive remodeling to be compatible with DUN, BONE and
   						other BeOS software.
   					Cleaned up the code and removed unused functions.
   					Added extended and exagerated comments. ^_^
      				Lee Patekar, 2000-11

             		Copyright (C) 1999 by Trisignal communications Inc.
                            	     All Rights Reserved
  =========================================================================*/

#include <signal.h>
#include <Drivers.h>
#include <PCI.h>
#include <KernelExport.h>
#include <driver_settings.h>
#include <tty/ttylayer.h>
#include <PCI.h>
#include <malloc.h>

#include "HostCommon.h"
#include "serial.h"
#include "ddalapi.hpp"
#include "lnk_time.h"

#ifdef USELOCK
#include "lock.h"
#endif



/*========================================================================
			Constant definitions
   ========================================================================*/
// Default kernel buffer size
#define BUFFERSIZE 2048
#define ILOWATER    (BUFFERSIZE * 2/8)
#define IHIWATER    (BUFFERSIZE * 7/8)
#define OHIWATER    (BUFFERSIZE * 7/8)
#define OLOWATER    (BUFFERSIZE * 1/8)

// Termios initial value
static const struct termios TDEFAULT = {
	0,                 /* c_iflag */
	OPOST,             /* c_oflag */
	B19200|CS8|HUPCL,  /* c_cflag */
	0,                 /* c_lflag */
	0,                 /* c_line */
	0,                 /* c_ixxxxx */
	0,                 /* c_oxxxxx */
	{
		ctrl('C'),         /* c_cc[VINTR] */
		ctrl('\\'),        /* c_cc[VQUIT] */
		ctrl('H'),         /* c_cc[VERASE] */
		ctrl('U'),         /* c_cc[VKILL] */
		ctrl('D'),         /* c_cc[VMIN] */
		10,                /* c_cc[VTIME] */
		0,                 /* c_cc[VEOL2] */
		0,                 /* c_cc[VSWTCH] */
		ctrl('S'),         /* c_cc[VSTART] */
		ctrl('Q'),         /* c_cc[VSTOP] */
		0                  /* c_cc[VSUSP] */
	}
};

// Default winsize value, unused in our modem
static const struct winsize WDEFAULT = {24, 80, 240, 800};

// PCI device and vendor IDs suported by our driver
static struct {
	uint16	vendor_id;
	uint16	device_id;
} supported_devices[] =
{
	{ 0x14F1, 0x1035 },
	{ 0x14F1, 0x1085 },
	{ 0x14F1, 0x1435 },
	{ 0x127a, 0x1035 },
	{ 0x127a, 0x1085 },
	{ 0x127a, 0x1435 },
	{ 0x127A, 0x1033 },
	{ 0x14F1, 0x1033 },
	{ 0x13E0, 0x0271 }
};

/*========================================================================
			Macro definitions
   ========================================================================*/

#ifdef USEDEBUG
//# define debugprintf(x...) dprintf("TRIMODEM: " x, __VA_ARGS__)
# define debugprintf(x...) dprintf("TRIMODEM: " ##x)
#else
# define debugprintf(x...)
#endif

/*========================================================================
			Structure definitions
   ========================================================================*/

// The buffer structure is used to implement a simple FIFO circular buffer.
// Instead of using a count element to know if the buffer is full or not,
// we use the head and tail pointers.  The buffers will be used by two
// seperate entities : the GMC and the application.  Because theses distinct
// entities only increment one variable that the other entity can only read,
// this makes the circular buffers safer then they would be if a count 
// variable was used since it would be modified constantly by both sides.

struct buffer
{
	BYTE   *Base;			// Base of the buffer
	volatile uint32  Head;	// Head Index
	volatile uint32  Tail;	// Tail Index
	sem_id user_sem;		// Access Semaphore
};

// The GlobalDriverInfoStruct is the structure where all information relevant
// to the serial port aspect of the modem are stored.  It contains the FIFO
// buffers, the termios setup and all the semaphores as well.
typedef struct 
{
	uint32          open_count;
#ifdef USELOCK
	lock			dev_lock;
#endif

	BOOLEAN 		bDriverOpen;	// Indicated if the driver is open
	uint			flags;			// Status flags
	struct termios	ttyConfig;		// Termios configuration
	struct buffer	RxBuffer;		// FIFO Rx buffer
	struct buffer   TxBuffer;		// FIFO Tx buffer
	bigtime_t		vtime;			// Current Timeout time for read/write
	struct winsize	wsize;			// unused
} GlobalDriverInfoStruct; 

/*========================================================================
				Public variables definitions
   ========================================================================*/
// No public variables

/*========================================================================
				Private variables definitions
   ========================================================================*/

// The globlal driver info structure which will contain all (or most) of the
// modems settings, semaphores and flags.
static GlobalDriverInfoStruct 	gDriverInfo;

// supported ports string used by BeOS to find and identify our modem port
static const char      *devnames[] = {"ports/tri_modem",NULL};	

// The pci module is used to detect and configure PCI devices.  This pointer
// will point to the PCI module that will be used by the GMC (and the
// init_hardware function if USEINITHW was defined).
static pci_module_info *pci;

// To communicate with the GMC.  It contains status and control registers.
static unsigned char   registers[8];

// Used by HcfSpkPhHal_BeOS.cpp to map the datapump's (hardware) registers
// in accesible memory space.  Not used in this file, however, we need the
// area id to unallocate (free) the memory during the free hook.
static area_id         areaID;

// To preserve the previous debug output enable state to restore the
// com port to its previous state after our driver unloads.
#ifdef USEFORCEDEBUG
static bool LastDebugOut;
#endif

// If not using allocated memory use static buffers
static BYTE RxBase[BUFFERSIZE];
static BYTE TxBase[BUFFERSIZE];

/*========================================================================
			External routines declarations
   ========================================================================*/

// If the hardware detection phase is enabled in the init_hardware function
// we'll need this function to find the PCI device.
#ifdef USEINITHW
extern bool lookup_pci_device(short vendID, short devID, pci_info *return_info);
#endif

// These two externals are declared in OSALcallback.cpp and they are used to
// create and remove the lock that protects the callback table.
extern void InitCallbacks(void);
extern void UninitCallbacks(void);

/*========================================================================
			Private routines declarations
   ========================================================================*/


/*=========================================================================

  Function	:	evalcarrier

  Purpose	:   Sets the carrier flag in the globlal driver info structure 
  				flag member or clears the transmit buffers if the carrier 
  				was lost.
  				
  Input		:   nothing

  Output	:	Modifies the flags variable in the global driver info
  				structure by setting or unsetting the TTYCARRIER bit.
  				This function can also clear the local Transmit buffer as
  				well as the GMC transmit buffer.

  Returns	:   nothing

  NOTE		:	The TTYCARRIER bit flag is used in all the read and write 
  				operations so it cannot be ignored.  

  				The flag depends on the DCD signal, so if the carrier is lost
  				(unset by the GMC) the transmit buffers are flushed and all 
  				read and write operations are suspended.

  				The flag also depends on the CLOCAL bit in the termios 
  				structure supplied by the calling application when the port
  				is opened or via the IO control hook.  The CLOCAL bit flag
  				must be true or else the carrier will never be detected.
            
  =========================================================================*/
static void evalcarrier(void)
{
	if((gDriverInfo.ttyConfig.c_cflag & CLOCAL) || (gDriverInfo.flags & TTYHWDCD))
	{
		gDriverInfo.flags |= TTYCARRIER;
	}
	else
	{
		gDriverInfo.TxBuffer.Head = gDriverInfo.TxBuffer.Tail;

		gDriverInfo.flags &= ~TTYCARRIER;
		SetGmcEvent(PURGE_TX_BUFFER);
	}
}


/*=========================================================================

  Function	:	ttyhwsignal

  Purpose	:   This function is called whenever the GMC alters one of its
  				serial control signals (DCD, CTS, DSR and RI).  Basically, 
  				this function make a copy of the signal's state into the 
  				globlal driver info structure flags variable.

  Input		:   sig : 
  				
  				An integer containing the signal that has been modified.  The
  				signals are TTYHWDCD, TTYHWCTS, TTYHWDSR and TTYHWRI.
  				
  				asserted : 
  				
  				A boolean value indicating if the signal is true or false.

  Output	:   Modifies the flags variable in the global driver info
  				structure by setting or unsetting the bit that mirrors the
  				specific signal represented by sig.

  Returns	:   nothing

	NOTE	:	This function will evaluate the carrier if DCD has changed.
	
  =========================================================================*/
static void ttyhwsignal(int sig, bool asserted)
{
	switch(sig)
	{
		case TTYHWDCD:
			if(asserted)
			{
				gDriverInfo.flags |= TTYHWDCD;
			}
			else
			{
				gDriverInfo.flags &= ~ TTYHWDCD;
			}

			// If DCD changes, recheck the carrier!
			evalcarrier();
			break;

		case TTYHWCTS:
			if(asserted)
			{
				gDriverInfo.flags |= TTYHWCTS;
			}
			else
			{
				gDriverInfo.flags &= ~ TTYHWCTS;
			}
			break;

		case TTYHWDSR:
			if(asserted)
			{
				gDriverInfo.flags |= TTYHWDSR;
			}
			else
			{
				gDriverInfo.flags &= ~TTYHWDSR;
			}
			break;

		case TTYHWRI:
			if (asserted)
			{
				gDriverInfo.flags |= TTYHWRI;
			}
			else
			{
				gDriverInfo.flags &= ~ TTYHWRI;
			}
			break;

		default:
			break;
	}
}


/*=========================================================================

  Function	:	serial_service

  Purpose	:   This function contains all of the services the serial port
  				must implement.  It is the interface we use to access the
				registers in a clear and readable way.  The services we 
				implement here are the following :
				
				TTYENABLE : Used to open the port.  It activates DTR and
				the RTS, it also verifies the state of the DCD and CTS.
				
				TTYDISABLE : Used to close the port.  It disactivates DTR
				and RTS.
				
				TTYISTOP : Used for flow control, it disactivates RTS wich
				stops data input.
				
				TTYIRESUME : Used for flow control, it re-activates RTS to
				allow (or resume) data input.
				
				TTYSETDTR : Sets the DTR.
				
				TTYCLRDTR : Clears the DTR.

				TTYGETSIGNALS : Fetches the DSR and RI signal's state and 
				updates the globlal driver info structure flags variable
				with the signals state via the before mentioned ttyhwsignal
				function.

  Input		:   com :

				An unsigned integer (uint) containing the number of the 
				service we wish to invoke.  The values com can take are :
				TTYENABLE, TTYDISABLE, TTYISTOP, TTYIRESUME, TTYSETDTR,
				TTYCLRDTR and TTYGETSIGNALS.			

  Output	:   This function can modify the control register 
  				(registers[MCR]) as well as the globlal driver info 
  				structure flags variable.  This function also communicates
  				to the GMC via the SetGmcEvent function.

  Returns	:   Always returns TRUE

  NOTE		:	
            
  =========================================================================*/
static bool serial_service(uint com)
{
    switch(com)
    {
        case TTYENABLE:
            // Get DCD, CTS
            ttyhwsignal(TTYHWDCD, registers[MSR]&MSR_DCD);
            ttyhwsignal(TTYHWCTS, registers[MSR]&MSR_CTS);
            // Set DTR, RTS on 
            registers[MCR] = MCR_DTR|MCR_RTS;
            SetGmcEvent(MCR_CHANGE);
            break;

        case TTYDISABLE:
        	// Unset DTR and RTS
            registers[MCR] = 0;
            SetGmcEvent(MCR_CHANGE);
            break;

        case TTYISTOP:
        	// Unset RTS
            registers[MCR] &= ~MCR_RTS;
            SetGmcEvent(MCR_CHANGE);
            break;

        case TTYIRESUME:
			// Set RTS
            registers[MCR] |= MCR_RTS;
            SetGmcEvent(MCR_CHANGE);
            break;

        case TTYSETDTR:
			// Set DTR
            registers[MCR] |= MCR_DTR;
            SetGmcEvent(MCR_CHANGE);
            break;

        case TTYCLRDTR:
			// Unset DTR
            registers[MCR] &= ~MCR_DTR;
            SetGmcEvent(MCR_CHANGE);
            break;

			// Get DSR and RI value
        case TTYGETSIGNALS:
            ttyhwsignal(TTYHWDSR, registers[MSR] & MSR_DSR);
            ttyhwsignal(TTYHWRI,  registers[MSR] & MSR_RI);
            break;
    }
    return TRUE;
}


/*=========================================================================

  Function	:	iresume

  Purpose	:   Used for flow control, it allows (or resumes) data input if
  				we are not currently reading and the receive buffers are low.
  				This function uses and depends on the serial ports control
  				signals (DTR) so unless theses signals are used for flow 
  				control this function is not required.

  Input		:   none

  Output	:   

  Returns	:   nothing

  NOTE		:	TTYREADING is always true and the serial service TTYSTOP is
  				only used in the control hook so this function appears to be
  				incomplete.  This functionality is not used in DUN or any 
  				other application at our disposal.

  TODO		:   We need to disable the TTYREADING flag when the FIFO gets
  				filled up to a certain point.
            
  =========================================================================*/
static void iresume(void)
{
	uint32 count;

	uint32 TmpHead;
	uint32 TmpTail;

    // To make sure we use values that have not been modified by the interrupt
    // Worse case here is that we return a smaller count than actual count,
    // which should not be problematic.

	TmpHead = gDriverInfo.RxBuffer.Head;
	TmpTail = gDriverInfo.RxBuffer.Tail;
	
	count  = (BUFFERSIZE + TmpHead - TmpTail) % BUFFERSIZE;

    if(!(gDriverInfo.flags & TTYREADING) && (count <= ILOWATER))
    {
        gDriverInfo.flags |= TTYREADING;
        serial_service(TTYIRESUME);
    }
}


/*=========================================================================

  Function	:	waitevent

  Purpose	:   Waits for data to arrive in the Receive buffer.  If the 
  				already is data in the receive buffer, waitevent will exit
  				immediately.  If the buffer never receives data, this 
  				function will timeout after the timeout periode specified
  				in vtime.

  Input		:   nbyte :
  
  				Pointer to an unsigned integer (uint) buffer space.  Must be
  				a valid address where waitevent can write at least one 
  				integer.

  Output	:   This function will write the circular FIFO buffer's current
  				count as an unsigned integer at the address pointed to by 
  				the nbyte pointer.

  Returns	:   Returns the the count of the amount of data inside the FIFO 
  				buffer (RxBuffer).

	NOTE	:	vtime is the serial port's default timeout period in 10 ms 
				units.  It is defined when the port is opened or via the 
				TCVTIME IO control in the control hook (serial_control).

  =========================================================================*/
static uint waitevent(uint *nbyte)
{
	uint32		count= 0;
    bigtime_t 	tfin = system_time() + gDriverInfo.vtime * 100000;

	uint32 TmpHead;
	uint32 TmpTail;

		// while we haven't timed out and the port is still open
	while ((system_time() < tfin) && gDriverInfo.bDriverOpen)
	{
        // We sample the FIFO buffer's head and tail pointers to make sure 
        // we use values that have not been modified by the interrupt.  
        // Worse case here is that we return a smaller count than the actual
        // count, which should not be problematic.

		TmpHead = gDriverInfo.RxBuffer.Head;
		TmpTail = gDriverInfo.RxBuffer.Tail;

		// check to see if we have something in the buffers
		count  = (BUFFERSIZE + TmpHead - TmpTail) % BUFFERSIZE;

		// if we have something, stop waiting around!!!
		if(count)
		{
			break;
		}

		snooze(10000);
	}

	// If the received pointer is valid, say how much data is available
    if (nbyte)
    {
        *(uint *)nbyte = count;
    }

    return count;
}


/*=========================================================================

  Function	:	cleartty

  Purpose	:   Will "reformat" the tty and indicate that the port is closed.
				This function also resets the circular buffers.  It is
				usually called in the close hook.

  Input		:   none

  Output	:   Modifies bDriverOpen and the flags inside the global modem
  				info structure.

  Returns	:   nothing
            
  =========================================================================*/
static void cleartty(void)
{
	// Indicate that the port is now closed.  All read, write and control
	// calls will now be blocked.  Waitevent will also automatically timeout.
    gDriverInfo.bDriverOpen = FALSE;

	// Clear all the flags
    gDriverInfo.flags = 0;
}


/*=========================================================================

  Function	:	open_tty

  Purpose	:   Opens and initializes the global modem info structure and all
  				of its contents which including the termios settings, the
  				modem's timeout value (vtime) and the circular buffers.  This
  				function will also modify the flags. 

  Input		:   none.

  Output	:   Initializes the global modem info structure.

  Returns	:   A status variable indicating if the global modem info 
  				structure and all its contents were setup correctly.

  NOTE		:	The flags must be initialized with the flags value received
  				by the user application *before* calling this function.
            
  =========================================================================*/
static status_t open_tty(void)
{
	// only initialize the global modem info structure if it has not been
	// previously initialized.  Normally, this should never happen.
    if(gDriverInfo.bDriverOpen != TRUE)
    {
    	gDriverInfo.bDriverOpen = TRUE;

		// Setup Circular buffers
		gDriverInfo.RxBuffer.Base= RxBase;
		gDriverInfo.RxBuffer.Head= 0;
		gDriverInfo.RxBuffer.Tail= 0;

		gDriverInfo.TxBuffer.Base= TxBase;
		gDriverInfo.TxBuffer.Head= 0;
		gDriverInfo.TxBuffer.Tail= 0;

		// Set the default termios values
        gDriverInfo.ttyConfig = TDEFAULT;

        // Used for flow control and the iresume fuction.  Is deprecated?
        gDriverInfo.flags  |= TTYREADING;

        // Initialize the global timeout to the termios default
        gDriverInfo.vtime = gDriverInfo.ttyConfig.c_cc[VTIME];

        // Initialize the registers.  (Will set DTR and CTS and refresh the
        // flags value to represent the control signals provided by the GMC.
        if(!serial_service(TTYENABLE))
        {
            cleartty();
            return ENODEV;
        }
        
		snooze(500000);
 		debugprintf("TTY setup completed\n");
    }
    return B_OK;
}

/*=====================================
   Hook functions called by Kernel
 =====================================*/

/*=========================================================================

  Function	:	serial_open

  Purpose	:   This is the open hook called by beOS.  This is where we 
  				setup and initialize the serial port and the GMC.

  Input		:   name :
  
  				The device name BeOS wants to open.
  				
  				flags :
  				
  				The initial value of the status flags our port will use.
  				
  				cookie :
  				
  				Not used because we only allow the port to be opened once and
  				because this driver only services one modem card at a time.

  Output	:   Creates the global modem info structure and initialized
  				all of its contents.

  Returns	:   Returns the port open status.  the possibles returns are : 
  				B_OK 	:	If the port is opened successuffuly.
  				EINVAL 	:	If the device being opened is not supported by 
  							this driver.
  				B_DEV_NOT_READY	:	If the port was already opened.
  				ENXIO	:	If a hardware error occured.
  				ENOMEM	:	If a software error occured or if there isn't
  							enough memory to allocate all the buffers and
  							classes we need.
  				ENODEV	:	If the GMC failed to startup corrrectly

  NOTE		:	See the writing drivers section of the Be book for additional
  				information about the hook functions or check out 
  				/boot/beos/documentation/BeBook/Drivers/writing_drivers.html 
            
  =========================================================================*/
static status_t serial_open(const char *name, uint32 flags, void **  /* cookie */)
{
    status_t ret= B_OK;
    status_t err;

	debugprintf("Serial Open. Resistance is futile\n");

    /* Check devname is the same as our modem */
    if(strcmp(devnames[0], name))
    {
		debugprintf("Invalid device name \n");
        /* Invalid device name */
        return EINVAL;
    }


	LOCK(gDriverInfo.dev_lock);
	do
	{
		// Initialize the serial port's flags with the ones given to us by the
		// user application.
		gDriverInfo.flags = flags;

		if(gDriverInfo.open_count== 0)
		{
			debugprintf("InitCallbacks()\n");

			// The InitCallbacks function is declared in OSALCallback.c and is used
			// to setup the callback structure's lock
			InitCallbacks();

			debugprintf("InitGmcModules()\n");

			// initialize the GMC module
    		if((err = InitGmcModules())!= 0)
    		{
        		debugprintf("DDAL::osless_open InitGmcModules failed with %ld\n", ret);

            	ret= ENOMEM;

		        // If there was a hardware error ...
        		if (err == -2)
        		{
					debugprintf("HW error\n");
            		ret= ENXIO;
        		}
		        // If there was a software error ...
        		else
        		{
					debugprintf("SW error\n");
            		ret= ENXIO;
				}

				break;
        	}

			// If the GMC was Intialized without any problems we should activate it
			// at this time because we need to GMC to continue opening the port.
    		if (StartGmc() != 0)
    		{
        		debugprintf("DDAL::osless_open failed on StartGmc\n");

				CleanupGmcModules();
	
        		ret= ENODEV;
				break;
       		}

    		debugprintf("Creating tty!\n");

			// Initialize the global modem info structure and the circular buffers.
			// The GMC must be initialized and enabled for this function call.
    		if(open_tty() != B_OK)
    		{
    			debugprintf("Could not open tty\n");
	    	
		    	StopGmc(0);
	    	
		    	CleanupGmcModules();
	    	    	
        		ret= ENOMEM;
				break;
    		}
		}

		//
		// we succed opening the device
		//
		gDriverInfo.open_count++;
		ret= B_OK;
	} while(0);

	// if we are exiting with an error code, we should uninit the callbacks
	if(ret != B_OK)
	{
		UninitCallbacks();
	}

	UNLOCK(gDriverInfo.dev_lock);

	debugprintf("Open function finished\n");

    return ret;
}


/*=========================================================================

  Function	:	serial_close

  Purpose	:   This is the close hook called by BeOS.  This function 
  				closes the port which blocks all read, write and control
  				hooks on the device.

  Input		:   cookie :
  
  				Not used in this implementation.

  Output	:   

  Returns	:   Returns the port close status.  The value is always B_OK.

  NOTE		:	If we try to close a port that was not opened, the function
  				call will still return B_OK.
            
  =========================================================================*/
static status_t serial_close(void *  /* cookie */)
{
    bigtime_t      tfin;

	debugprintf("Serial Close. You will be assimilated\n");

	LOCK(gDriverInfo.dev_lock);
	do
	{
		if(gDriverInfo.open_count== 1)
		{
    		if(gDriverInfo.bDriverOpen == TRUE)
    		{
				// Calculate how long we are willing to wait untill the receive FIFO
				// buffer has emptied itself.
				tfin= system_time( ) + 3000000;

				// Wait for the receive FIFO to empty itself.
        		while(gDriverInfo.TxBuffer.Head != gDriverInfo.TxBuffer.Tail)
        		{
 					snooze(250000);

            		if(system_time() > tfin)
            		{
						debugprintf("Timout while waiting for transmit buffer to empty itself\n");
                		break;
            		}
				}
    		}
		}
		gDriverInfo.open_count--;
	} while(0);
	UNLOCK(gDriverInfo.dev_lock);

	debugprintf("Serial Close finished\n");

    return B_OK;
}


/*=========================================================================

  Function	:	serial_free

  Purpose	:   This is the free hook called by BeOS.  This function free's
  				all the resources used by our modem driver.  The port must
  				be closed and all read, write and controls operations must 
  				be completed before we call this hook.  We should ignore all
  				read, write and control requests after the port has been 
  				freed as well.

  Input		:   cookie :
  			
  				Not used in this implementation.

  Output	:   

  Returns	:   Always returns B_OK (or B_NO_ERROR) if the serial port's 
  				resources were freed (or if the port was never opened to 
  				begin with).
  				
  NOTE		:	Calling this hook on an unopened port is potentially lethal.
  				Since its the BeOS Operating System that manipulates the 
  				hook functions this should never happen.
            
  =========================================================================*/
static status_t serial_free(void *  /* cookie */)
{
    status_t       error= B_OK;

	debugprintf("Serial Free\n");


	LOCK(gDriverInfo.dev_lock);
	do {
		if(gDriverInfo.open_count== 0)
		{
			/* Clear tty module */
			/* Set MCR to 0 */
			serial_service(TTYDISABLE);
	        cleartty();

			debugprintf("Stopping the GMC\n");

			StopGmc(0);

    		debugprintf("Cleaning the GMC\n");

    		CleanupGmcModules();

    		if((error = delete_area(areaID)) != B_OK)
    		{
        		debugprintf("Delete area failed\n");
    		}

    		debugprintf("Free tty\n");

			UninitCallbacks();
		}
	} while(0);
	UNLOCK(gDriverInfo.dev_lock);


	debugprintf("Serial free finished\n");

    return error;
}


/*=========================================================================

  Function	:	serial_read

  Purpose	:   This is the reed hook called by BeOS.  This function fetches
  				data from our serial port's FIFO buffers and gives it to the
  				BeOS operating system (which then sends it to the user's
  				application or dialup networking).

  Input		:   count :

				Contains the amount of data BeOS want to receive or it 
				contains the maximum amount of data BeOS can receive.
				
				buf :
				
				Pointer to BeOS's data buffer.
  
  				pos and cookie :
 				
 				Not used in this implementation.

  Output	:   The count argument will contain the amount of data bytes
  				copied to the BeOS buffer.
  				This hook modifies the Receive FIFO buffer Tail pointer.

  Returns	:   Returns the result of the read attempt.  Possible values are :
				B_OK	: If the data was succesfully transfered.
				ENODEV	: If the port isn't even open.
				EIO		: If the carrier was lost.	
				B_WOULD_BLOCK : If non-blocking mode is selected and the
						  read hook is allready being used (or disabled).

		
  NOTE		:	If blocking mode is selected, the read hook will wait up
  				to vtime * 10 ms for data if there isn't enough data in the
  				FIFO buffer to meet the demand.
  				If non-blocking mode is selected, the read hook will not wait
  				for additional data and exit right away.
            
  =========================================================================*/
static status_t serial_read(void *  /*cookie*/, off_t /*pos*/, void *buf, size_t *count)
{
    uint32     i = 0, n;
	BYTE 	   *buffer = (BYTE *)buf;
    status_t   error = B_OK;
	bigtime_t  timeout;

	uint32 TmpHead;
	uint32 TmpTail;
	bool   canTimeout;

	// If the port is not opened we should not attempt to read from it.
	if(gDriverInfo.bDriverOpen != TRUE)
	{
		*count = 0;
		debugprintf("Error, can't read on a closed port\n");
		return ENODEV;
	}

	// If the count variable contains invalid information, we should just
	// respond that all is ok BUT we must indicate that no data was 
	// transfered though!
	if(*count <= 0)
	{
		*count = 0;
		return B_OK;
	}

	timeout = system_time() + gDriverInfo.vtime * 100000;
	canTimeout= ~((gDriverInfo.vtime== 0) && (gDriverInfo.ttyConfig.c_cc[VMIN]> 0));
	error = B_OK;

    while(true)
    {
        // To make sure we use values that have not been modified by the interrupt
        // Worse case here is that we return a smaller count than actual count,
        // which should not be problematic.

		TmpHead = gDriverInfo.RxBuffer.Head;
		TmpTail = gDriverInfo.RxBuffer.Tail;

		n= (BUFFERSIZE + TmpHead - TmpTail) % BUFFERSIZE;

        if(n >= *count)				// if count is what we need
        {
            while (i < *count)
            {
                buffer[i]= gDriverInfo.RxBuffer.Base[gDriverInfo.RxBuffer.Tail];

				i++;
				gDriverInfo.RxBuffer.Tail++;
				gDriverInfo.RxBuffer.Tail%= BUFFERSIZE;
            }

			iresume();
            *count = i;
            break;
        }

			// if non-blocking selected or if we timout
		if((gDriverInfo.flags & O_NONBLOCK) || (canTimeout && (system_time() >= timeout)))
		{
			if(n)
			{
				//
				// if we have data to give
				//
	            while (i < n)
	            {
                	buffer[i]= gDriverInfo.RxBuffer.Base[gDriverInfo.RxBuffer.Tail];

					i++;
					gDriverInfo.RxBuffer.Tail++;
					gDriverInfo.RxBuffer.Tail%= BUFFERSIZE;
           		}

				iresume();
            	*count = i;
			} else {
				if(gDriverInfo.flags & O_NONBLOCK) {
					error= EAGAIN;
				} else {
					error= B_OK;
				}
	            *count = 0;
            }
            break;
        }

		// If the carrier is lost between our modem and the remote modem 
		// while we're waiting for data, we should exit the read hook 
		// immediately and indicated that the IO has ended.
        if(!(gDriverInfo.flags & TTYCARRIER))		// if connection lost
        {
            *count = 0;
            error = EIO;			// Signal end of IO
			debugprintf("Carrier lost during read\n");
            break;
        }

        snooze(10000);				// wait 10 ms
	}

    return error;
}


/*=========================================================================

  Function	:	serial_write

  Purpose	:   This is the write hook called by BeOS.  This function moves
  				data from the BeOS operating (which was received by a user 
  				application or dialup networking) was system to the serial 
  				port's transmit FIFO buffer.

  Input		:    count :

				Contains the amount of data BeOS wants to give us.
				
				buf :
				
				Pointer to BeOS's data buffer containing the data.
  
  				pos and cookie :
 				
 				Not used in this implementation.

  Output	:	The count argument will contain the amount of data bytes
  				taken from the BeOS buffer.
  				This hook modifies the transmit FIFO buffer Head pointer.

  Returns	:	Returns the result of the write attempt.  Possible values are
				B_OK	: If the data was succesfully transfered.
				ENODEV	: If the port isn't even open.
				EIO		: If the carrier was lost.
				B_WOULD_BLOCK : If non-blocking mode is selected and the
						  write hook is allready being used (or disabled).

  NOTE		:	If blocking mode is selected, the write hook will wait up
  				to vtime * 10 ms if there isn't enough room in the FIFO 
  				buffer to meet the demand.
  				If non-blocking mode is selected, the write hook will not 
  				wait for additional space.  It will take as much data as it
  				can and exit.
            
  =========================================================================*/
static status_t serial_write(void *  /*cookie*/, off_t /*pos*/, const void *buf, size_t *count)
{
	BYTE *buffer = (BYTE *)buf;
    uint32         n, i = 0;
    status_t       error = B_OK;
	bigtime_t	timeout;

	uint32 TmpHead;
	uint32 TmpTail;

	// If the port is not opened we should not attempt to write to it.
	if(gDriverInfo.bDriverOpen != TRUE)
	{
		*count = 0;
		debugprintf("Error, can't write on a closed port\n");
		return ENODEV;
	}

	// If the count variable contains invalid information, we should just
	// respond that all is ok BUT we must indicate that no data was 
	// transfered though!
	if(*count <= 0)
	{
		*count = 0;
		return B_OK;
	}


	timeout = system_time() + gDriverInfo.vtime * 100000;
	error = B_OK;

	while(true)
	{
        // Here we take a snapshot of the FIFO buffer's pointers to make sure
        // that we use values that have not been modified by the interrupt.
        // Worse case here is that we return a smaller count than the actual
        // count, which should not be problematic.
		TmpHead = gDriverInfo.TxBuffer.Head;
		TmpTail = gDriverInfo.TxBuffer.Tail;

		// Here we check to see how much space we have in the FIFO buffer.
		n= (BUFFERSIZE - 1) - (BUFFERSIZE + TmpHead - TmpTail) % BUFFERSIZE;

		// If the carrier is lost between our modem and the remote modem 
		// while we're trying to write data, we should exit the write hook 
		// immediately and indicated that the IO has ended.
		if(!(gDriverInfo.flags & TTYCARRIER))
		{
			// if carrier was lost
			debugprintf("Carrier lost during write\n");
			*count = 0;
			i = 0;
			error = EIO;
			break;
		}

		// If we have some room inside the FIFO buffer to receive data, we
		// transfer as many bytes of available data to the FIFO until it is
		// either filled or until we run out of data.
		if(n > 0)
		{
			uint j = 0;		// count current writes

            while ((j < n) && ((i + j) < *count))
            {
                gDriverInfo.TxBuffer.Base[gDriverInfo.TxBuffer.Head] = buffer[i+j];			// take a bytes

                j++;
				gDriverInfo.TxBuffer.Head++;
				gDriverInfo.TxBuffer.Head%= BUFFERSIZE;
            }
            i+= j;		// count total writes
		}

		// If we are in non-blocking mode or if there is no more data left
		// to transfer or if we timeout, we should exit the write hook.
 		if((gDriverInfo.flags & O_NONBLOCK) || (i == *count) || (system_time() >= timeout))
 		{
			break;		// exit the function
		}

		snooze(10000);			// wait 10ms for buffer to empty itself
	}

	*count = i;

	return error;
}

/*=========================================================================

  Function	:	serial_control

  Purpose	:   This is the control hook called by BeOS.  It implements all
  				of the IO controls specific to modems and serial ports.  
  				These IO controls are the following :

  				TCGETA	:	Get the Termios configuration settings.
  				TCSETA	:	Sets the Termios configuration settings.
				TCSETAW :	Like above.
				TCSETAF :	Like above but also flushes the receive FIFO
				TCWAITEVENT : Waits until there is some data in the 
							  recive FIFO
				TCFLSH :	Reset the receive FIFO or reset the transmit FIFO
							or reset both FIFOs.
				TCSETDTR :	Set or clear DTR.
				TCSETRTS :  Set or clear RTS.
				TCGETBITS : fetches the control signals status (CTS, DSR, 
							DCD and RI).
				TIOCGWINSZ : Unimplemented, returns B_OK
				TIOCSWINSZ : Unimplemented, returns B_OK
				B_SET_BLOCKING_IO : Sets the modem in blocking mode.
				B_SET_NONBLOCKING_IO : Sets the modem in non-blocking mode.
				TCVTIME : 	Sets the timeout period in 10ms units.

				'ichr'	:	Non-standard IO control for BONE.  Returns the
							amount of data in the receive FIFO without 
							blocking.

  Input		:   com :
  
  				Contains the ID of the serial IO control BeOS or the user's
  				application wants to perform.
  				
  				buf :
  				
  				Points to a variable or data used for the IO control 
  				specified by com.
  				
  				len and cookie :
  				
  				Unused in this implementation.

  Output	:   Depends on the IO control being executed.  This hook can 
  				modify the registers, the status flags and the termios 
  				configuration settings contained in the global modem info
  				structure.  The FIFO buffers are NOT modified.

  Returns	:	The return value depends on the IO control selected by com.
  				Possible values are :    
   				B_OK 	: If the IO control selected by com is implemented 
   						  (fully or in part)
  				ENODEV	: If the port is not open.
  				B_WOULD_BLOCK : If non-blocking mode is selected and the
						  		write hook is allready being used (or 
						  		disabled).
  				B_DEV_INVALID_IOCTL : If the IO control is unimplemented or 
  									  is unknown.

  =========================================================================*/
static status_t serial_control(void *  /*cookie*/, uint32 com, void *buf, size_t /*len*/)
{
    struct termios ttyTemp;
    int            i;
	bigtime_t 	   vt;
	status_t error;
	uint theEvent;

	// If the port is not opened we should not attempt to access it.
	if(gDriverInfo.bDriverOpen != TRUE)
	{
		debugprintf("Error, can't issue control commands on a closed port\n");
		return ENODEV;
	}

	error = B_OK;

    switch(com)
    {
        case TCGETA:  /* get termios struct (struct termios) */
            *(struct termio *)buf = gDriverInfo.ttyConfig;
            break;

        case TCSETA:  /* set termios struct (struct termios) */
        case TCSETAW: /* drain output, set (struct termios) */
        case TCSETAF: /* drain output, flush input, set (struct termios) */
            ttyTemp = *(struct termios *)buf;

            /* Set Baudrate, MCR, LCR, etc */
            if(ttyTemp.c_cflag == gDriverInfo.ttyConfig.c_cflag)
            {
                gDriverInfo.ttyConfig = ttyTemp;
                gDriverInfo.vtime = gDriverInfo.ttyConfig.c_cc[VTIME];
            }
            else
            {
                gDriverInfo.ttyConfig = ttyTemp;
                gDriverInfo.vtime = gDriverInfo.ttyConfig.c_cc[VTIME];
                SetGmcEvent(MCR_CHANGE);
            }

            /* Check carrier */
            evalcarrier();
            /* Flush input */
            if(com == TCSETAF)
            {
				debugprintf("Clear RxBuff\n");
				gDriverInfo.RxBuffer.Tail = gDriverInfo.RxBuffer.Head;
                SetGmcEvent(PURGE_RX_BUFFER);
                iresume();
            }
            break;

        case TCWAITEVENT:  /* Wait for input */
        	theEvent = waitevent((uint *)buf);
			return theEvent;

        case TCFLSH:  /* Flush input or output */
            switch((int)buf)
            {
                case 2:  /* Flush input and output */
					debugprintf("Clear TxBuff\n");
					gDriverInfo.TxBuffer.Head = gDriverInfo.TxBuffer.Tail;
                    SetGmcEvent(PURGE_TX_BUFFER);
                case 0:  /* Flush input */
					debugprintf("Clear RxBuff\n");
					gDriverInfo.RxBuffer.Tail = gDriverInfo.RxBuffer.Head;
                    SetGmcEvent(PURGE_RX_BUFFER);
                    iresume();
                    break;
                case 1:  /* Flush output */
					debugprintf("Clear TxBuff\n");
					gDriverInfo.TxBuffer.Head = gDriverInfo.TxBuffer.Tail;
                    SetGmcEvent(PURGE_TX_BUFFER);
            }
            break;

        case TCSETDTR:  /* Set or clear DTR */
            i = *(bool *)buf? TTYSETDTR: TTYCLRDTR;
            serial_service(i);
            break;

        case TCSETRTS:  /* Set or clear RTS */
            if(!(gDriverInfo.flags & TTYFLOWFORCED))
            {
                i = *(bool *)buf? TTYIRESUME: TTYISTOP;
                serial_service(i);
            }
            break;

        case TCGETBITS:  /* Get MSR data */
            if(buf)
            {
                uint signals = 0;

                serial_service(TTYGETSIGNALS);
                if(gDriverInfo.flags & TTYHWCTS)
                {
                    signals |= TCGB_CTS;
                }
                if(gDriverInfo.flags & TTYHWDSR)
                {
                    signals |= TCGB_DSR;
                }
                if(gDriverInfo.flags & TTYHWDCD)
                {
                    signals |= TCGB_DCD;
                }
                if(gDriverInfo.flags & TTYHWRI)
                {
                    signals |= TCGB_RI;
                }
                *(uint *)buf = signals;
            }
            break;

        case TIOCGWINSZ:  /* Get winsize, unused */
            *(struct winsize *)buf = gDriverInfo.wsize;
            break;

        case TIOCSWINSZ:  /* Set winsize, unused */
            gDriverInfo.wsize = *(struct winsize *)buf;
            break;

        case B_SET_BLOCKING_IO:  /* Set IO blocking */
            gDriverInfo.flags &= ~O_NONBLOCK;
            debugprintf("Blocking mode Selected\n");
            break;

        case B_SET_NONBLOCKING_IO:  /* Set IO non-blocking */
            gDriverInfo.flags |= O_NONBLOCK;
			debugprintf("Non-Blocking mode Selected\n");
            break;

		case TCVTIME:
			vt =  gDriverInfo.vtime;
			gDriverInfo.vtime = *(bigtime_t *)buf;
			*(bigtime_t *)buf = vt;
			break;

// non-standard BONE IOCTL START
		case 'ichr':

			uint32 TmpHead;
			uint32 TmpTail;

			int count;

		    // To make sure we use values that have not been modified by the interrupt
		    // Worse case here is that we return a smaller count than actual count,
		    // which should not be problematic.

			TmpHead = gDriverInfo.RxBuffer.Head;
			TmpTail = gDriverInfo.RxBuffer.Tail;

			count= (BUFFERSIZE + TmpHead - TmpTail) % BUFFERSIZE;

			if(!(gDriverInfo.flags & TTYCARRIER)) // if connection lost
			{
				count = 0;
				error = EIO;			// Signal end of IO
			}

			*(int *)buf = count;

			break;
// non-standard BONE IOCTL END

        case TCQUERYCONNECTED:
        case TCSBRK:  /* Set break control for 250ms */
        case TCXONC:
        default:
			debugprintf("inv ioctl\n");

            error= B_DEV_INVALID_IOCTL;//EINVAL;
    }

    return error;
}



// This is where we register the hook function.  Using this structure BeOS 
// will access our hook functions to communicate with our modem driver.
static device_hooks devhooks =
{
    &serial_open,
    &serial_close,
    &serial_free,
    &serial_control,
    &serial_read,
    &serial_write,
	NULL,
	NULL,
	NULL,
	NULL
};


/*======================================
   External functions called by Kernel
 ======================================*/


/*=========================================================================

  Function	:	publish_devices

  Purpose	:   Returns a null-terminated array of devices supported by 
  				this driver.

  Input		:   none

  Output	:   nothing special

  Returns	:   The address of our product string pointer.

  NOTE		:	See the BeBook for further details.
            
  =========================================================================*/
const char **publish_devices()
{
    return devnames;
}


/*=========================================================================

  Function	:	find_device

  Purpose	:   Returns a pointer to the device hooks structure for a given
  				device name

  Input		:   dev :

				A pointer to the string containing the device name.

  Output	:   none.

  Returns	:   A pointer to the requested device

  NOTE		:	We only support one device ...
            
  =========================================================================*/
device_hooks *find_device(const char *dev)
{
    if(!strcmp(devnames[0], dev))
    {
        return &devhooks;
    }

    return NULL;
}


/*=========================================================================

  Function	:	init_hardware

  Purpose	:   Called by the BeOS kernel to initialize the hardware.  It
  				detects the presence of our hardware (if enabled).

  Input		:   Nothing

  Output	:   Even less ...

  Returns	:   B_OK if the hardware was found, B_ERROR if the hardware was
  				not found or if we're not on a supported platform.

  NOTE		:	We do not perform hardware checks, the checks are done when
  				the GMC is created on port open.
  				The detect hardware feature prevents the driver from ever
  				being loaded if the modem card is not in the system.
            
  =========================================================================*/
status_t init_hardware(void)
{
	status_t error;
	unsigned i;
    pci_info info;
	char timebuffer[100];

	// If we want to use debug prints, enable the debug print output on the
	// com port now.
	#ifdef USEFORCEDEBUG
	LastDebugOut = set_dprintf_enabled(true);
	#endif

    debugprintf("init_hardware. Insert (2) coins to continue\n");

	error = B_OK;

    CompiledTime((BYTE *)timebuffer);
    debugprintf("compiled at %s\n",timebuffer);

    // Get PCI module items
    error = get_module(B_PCI_MODULE_NAME, (module_info **)&pci);
    if(error != B_OK)
    {
    	debugprintf("Could not load PCI module\n");
		return B_ERROR;
	}

	debugprintf("Detecting PCI card\n");

	for(i= 0; i< sizeof(supported_devices)/sizeof(supported_devices[0]); i++)
	{
	    if(lookup_pci_device(supported_devices[i].device_id, supported_devices[i].vendor_id, &info))
	    {
			debugprintf("Found modem at 0x%04x 0x%04x\n", supported_devices[i].device_id, supported_devices[i].vendor_id);

			error = B_OK;
			break;
		}
	}

 	put_module(B_PCI_MODULE_NAME);

	debugprintf("Init Hardware complete\n");

	// And if we enabled them, we disable the debug prints.  We must leave 
	// the system as it was before our software was executed.
#ifdef USEFORCEDEBUG
	set_dprintf_enabled(LastDebugOut);
#endif

	return error;
}


/*=========================================================================

  Function	:	init_driver

  Purpose	:   This function is called whenever BeOS wants to load our modem
  				driver.  It initializes the semaphores and the PCI module 
  				that will be used by the GMC.  This function also activates
  				the debug prints, if that option is enabled in the header 
  				file.

  Input		:   Nothing.

  Output	:   This function initializes all the semaphores inside the 
  				global modem info structure if they are enabled.  It also
  				loads the PCI module for the GMC.

  Returns	:   B_OK if the driver was initialized correctly, it returns 
  				B_ERROR otherwise.

            
  =========================================================================*/
status_t init_driver()
{
	// Activate the debug prints if the option is enabled, but we'll keep
	// the debug port's previous state so we can undo our change when we
	// unload the driver.
	#ifdef USEFORCEDEBUG
	LastDebugOut = set_dprintf_enabled(true);
	#endif


    debugprintf("Initialize modem driver. Do or Do NOT... there is no try\n");

   	// Now we'll get PCI module
	if(get_module(B_PCI_MODULE_NAME, (module_info **)&pci) != B_OK)
	{
		debugprintf("Can't open PCI module\n");
		return B_ERROR;
	}


	new_lock(&gDriverInfo.dev_lock, "trimodem_dev_lock");
	gDriverInfo.open_count= 0;


   	debugprintf("init driver finished\n");

    return B_OK;
}


/*=========================================================================

  Function	:	uninit_driver

  Purpose	:   This function is called whenever BeOS wants to unload our
  				modem driver.  In this function we release the PCI module,
  				remove all our semaphores from the system and we also return
  				the debug port to it's previous state.

  Input		:   Nothing.

  Output	:   Absolutely nothing!

  Returns	:   Even less ...

  =========================================================================*/
void uninit_driver()
{
	debugprintf("Uninit driver. Live long and prosper\n");


	// Now we'll release the PCI module we used to detect and interface with
	// the modem hardware.
    put_module(B_PCI_MODULE_NAME);

	free_lock(&gDriverInfo.dev_lock);

	debugprintf("Uninit driver finished\n");
	
	// And, last but not least, we reset the debug port to it's previous 
	// state.
	#ifdef USEFORCEDEBUG
	set_dprintf_enabled(LastDebugOut);
	#endif
}


// This variable is used by BeOS.  It identifies which API version our
// modem driver was compiled for.  For futher information reagarding this
// symbol export, see consult the Be book in the writing drivers section
// at /boot/beos/documentation/BeBook/Drivers/writing_drivers.html
int32 api_version = B_CUR_DRIVER_API_VERSION;


/*=====================================================
  Functions called by ddalapi.cpp (thread/timer events)
 ======================================================*/

/*=========================================================================

  Function	:	transmit_chars

  Purpose	:   Used by the GMC to read data from the transmit FIFO buffer.
  				In other words, the data flows from the serial ports transmit
  				FIFO buffer to the GMC.  The transmit_chars function will 
  				take as much data from the FIFO buffer as possible.  If the
  				FIFO is empty, or emptied, the function will return with 
  				whatever data, if any, it could take.  This function does 
  				not block.  *ever*

  Input		:   gmcBuffer :

				A pointer to the GMC's data buffer.
				
				maxByte :
				
				The size of the GMC's data buffer (which is the maximum 
				amount of data we can take).

  Output	:   The transmit_chars function modifies the transmit FIFO
  				buffer's Tail pointer. It also modifies the contents of the
  				GMC's buffer.

  Returns	:   This function returns the amount of data actually transfered.

  NOTE		:	This function is called by the GMC via the callback 
  				functions.  If the modems threads were not created or are 
  				unable to function, this function will never be called.

  =========================================================================*/
uint32 transmit_chars(unsigned char* gmcBuffer, uint32 maxByte)
{
	uint32 i = 0;

	// We remain inside the function as long as the GMC's buffer isn't filled
	// or as long as the FIFO buffer isn't emptied.
	while(i < maxByte)
	{
		// We constantly check to make sure there's data in the FIFO buffer.
		// if the FIFO buffer is suddenly empty, we exit the function.
		if(gDriverInfo.TxBuffer.Tail == gDriverInfo.TxBuffer.Head)
		{
			break;
		}
	
		// If the FIFO is not empty, we transfer a byte of data to the GMC's
		// buffer.
		gmcBuffer[i]= gDriverInfo.TxBuffer.Base[gDriverInfo.TxBuffer.Tail];

		// The serial port's FIFO buffers are circular buffers.  So we need
		// to wrap the pointers around if ever they reach the end of the 
		// buffer.
		i++;
		gDriverInfo.TxBuffer.Tail++;
		gDriverInfo.TxBuffer.Tail%= BUFFERSIZE;
	}
	
	return i;
}


/*=========================================================================

  Function	:	get_num_chars_in_txbuf

  Purpose	:   This function is called by the GMC.  It checks to see if 
  				there's data in the transmit FIFO buffer.  If data is 
  				available we tell the GMC how many bytes of data we have.  If
  				no data is available we return a false value (zero eh eh).
  				This function does not block. *ever*

  Input		:   nothing.

  Output	:   Does not modify anything

  Returns	:   The transmit FIFO buffer's data count

  NOTE		:	This function is called by the GMC via the callback 
  				functions.  If the modems threads were not created or are 
  				unable to function, this function will never be called.

  =========================================================================*/
uint32 get_num_chars_in_txbuf(void)
{
	uint32 count;

	uint32 TmpHead;
	uint32 TmpTail;

    // We take a snapshot of the Head and Tail pointers to make sure we use 
    // values that have not been modified by the interrupt timer.  Worse case
    // here is that we return a smaller count than actual count, which should
    // not be problematic.
	TmpHead = gDriverInfo.TxBuffer.Head;
	TmpTail = gDriverInfo.TxBuffer.Tail;

	// Here we calculate how many bytes of data are inside the FIFO buffer
	count = (BUFFERSIZE + TmpHead-TmpTail) % BUFFERSIZE;
	
	return count;
}   


/*=========================================================================

  Function	:	receive_chars

  Purpose	:   Used by the GMC to transmit data to the receive FIFO buffer.
				In other words, the data flows from the GMC to the serial 
				port's receive FIFO buffer.  The receive_chars function will
				fill the FIFO until it runs out of data or until the FIFO is
				filled.  This function does not block.  *ever*

  Input		:   gmcBuffer :
  
  				A pointer pointing to the GMC's data buffer.  This buffer
  				contains the data that will be transfered to our FIFO buffer.
  				
  				gmcBufSize :
  				
  				Contains the count, in bytes, of valid data inside the GMC's
  				data buffer.

  Output	:   Will modify the receive FIFO buffer's Head pointer.  It also
  				modifies the contents of the FIFO buffer ( it better eh! ).

  Returns	:   the amount of data bytes actually transfered.

  NOTE		:	This function is called by the GMC via the callback 
  				functions.  If the modems threads were not created or are 
  				unable to function, this function will never be called.
            
  =========================================================================*/
uint32 receive_chars(unsigned char* gmcBuffer, uint32 gmcBufSize) 
{
	uint32 i = 0;
	uint32 TmpHead;
	uint32 TmpTail;

	// We stay inside the function as long as there's valid data left inside
	// the GMC's buffer and as long as the FIFO buffer isn't filled (or full).
	while(i < gmcBufSize)
	{

	    // We take a snapshot of the Head and Tail pointers to make sure we use 
	    // values that have not been modified by the interrupt timer.  Worse case
	    // here is that we return a smaller count than actual count, which should
	    // not be problematic.
		TmpHead = gDriverInfo.RxBuffer.Head;
		TmpTail = gDriverInfo.RxBuffer.Tail;	

		// Here we check to see if our circular FIFO buffer is full.  If it
		// is full we exit the function.
		if( (BUFFERSIZE + TmpHead-TmpTail) % BUFFERSIZE == (BUFFERSIZE - 1) )
		{
			break;
		}

		// If the FIFO is not full, we transfer a byte of data from the GMC's
		// buffer into the FIFO buffer.
		gDriverInfo.RxBuffer.Base[gDriverInfo.RxBuffer.Head] = gmcBuffer[i];
			
		// The serial port's FIFO buffers are circular buffers.  So we need
		// to wrap the pointers around if ever they reach the end of the 
		// buffer.
		i++;
		gDriverInfo.RxBuffer.Head++;
		gDriverInfo.RxBuffer.Head%= BUFFERSIZE;
	}

	return i;
}


/*=========================================================================

  Function	:	get_register_value

  Purpose	:   Used by the GMC to fetch a byte of data from the registers.

  Input		:   offset :
  
  				Contains the number identifying the register.

  Output	:   This function does not modify anything.

  Returns	:   Returns the byte of information contained inside the 
  				register.

  NOTE		:	The registers are a way for the serial interface to 
  				communicate with the GMC.  The registers contain status and
  				control information.
            
  =========================================================================*/
unsigned char get_register_value(uint16 offset)
{
    return registers[offset];
}


/*=========================================================================

  Function	:	set_register_value

  Purpose	:   Used by the GMC to write a byte of information into the 
  				registers.  If the GMC is updating the serial ports control
  				signals (DCD or CTS) we automatically updaten the control
  				signal's value in the global modem info structure and take
  				the appropriate action.

  Input		:   offset :
  
  				Contains the number identifying the register.

				val :
				
				Contains the new value the register should take.

  Output	:   Writes the data into the selected register and updtates the
  				global modem info structure's flags.

  Returns	:   nothing

  NOTE		:	The registers are a way for the serial interface to 
  				communicate with the GMC.  The registers contain status and
  				control information.
            
  =========================================================================*/
void set_register_value(uint16 offset, unsigned char val)
{
    if(registers[offset] != val)
    {
		// If the register's contents are abot to change and the register
		// is the status register that contails all the control signals
		// we should update the changes in the rest of the serial interface.
        if(offset == MSR)
        {
            if((val & MSR_DCD) != (registers[MSR] & MSR_DCD))
            {
                ttyhwsignal(TTYHWDCD, val & MSR_DCD);
            }
            if((val & MSR_CTS) != (registers[MSR] & MSR_CTS))
            {
                ttyhwsignal(TTYHWCTS, val & MSR_CTS);
            }
        }

		// We can now write the new value into the register.
        registers[offset] = val;
    }
}


/*=========================================================================

  Function	:	setBaudRate

  Purpose	:   Used by the GMC to set the serial port interface's baud rate.

  Input		:   brate :
  
  				Cointains the desired baud rate.

  Output	:   Modifies the global modem info structure to reflect the 
  				changed baud rate.

  Returns	:   nothing.

  =========================================================================*/
void setBaudRate(uint16 brate)
{
	// Clear the current baud rate
    gDriverInfo.ttyConfig.c_cflag &= ~CBAUD;
	// Set the new baud rate
    gDriverInfo.ttyConfig.c_cflag |= (brate & CBAUD);
}


/*=========================================================================

  Function	:	getControlMode

  Purpose	:   Used by the GMC to get the serial port's current control 
  				mode.  The control mode is set when the port is opened or in
  				the control hook with the SETA IO control.

  Input		:   nothing

  Output	:   Does not change any values.

  Returns	:   The termios's control mode flag

  =========================================================================*/
tcflag_t getControlMode(void)
{
    return gDriverInfo.ttyConfig.c_cflag;
}


/*===========================================
  Functions called by HCFSpkPhoneHal_Be.cpp
 ===========================================*/


/*=========================================================================

  Function	:	getPCIModule

  Purpose	:   Called by the GMC to fetch the PCI module we allocated in the
  				inti_driver function.

  Input		:   nothing

  Output	:   none

  Returns	:   Returns a pointer to the PCI module.

  =========================================================================*/
pci_module_info *getPCIModule(void)
{
    return pci;
}


/*=========================================================================

  Function	:	setAreaID

  Purpose	:   Called by the GMC to give us the ID of a memory area.  The 
  				serial interface does not use this area, however, we must
  				delete this area from memory when we free the serial port.

  Input		:   The area's ID number.

  Output	:   Nothing.

  Returns	:   Even less.

  NOTE		:	The memory area is used to interface with the data pump.
            
  =========================================================================*/
void setAreaID(area_id aid)
{
    areaID = aid;
}
