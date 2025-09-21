/* -----------------------------------------------------------------------
//	File:	kb_mouse.c
//	Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.
//
//	Conditional Hotplugging Support:	(Dec/00)
//	 	If BIOS has a ps2 mouse option and it is set to ENABLED, hotplugging the
//		mouse should work. If the option is not present or if it is but is set to
//		AUTO or something else, it probably will not work.
/------------------------------------------------------------------------- */

#include "cbuf.h"
#include "kb_mouse_driver.h"
#include <8042.h>
#include <KernelExport.h>
#include <ISA.h>
#include <SupportDefs.h>
#include <Drivers.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>
#include <driver_settings.h>
#include <priv_kernelexport.h>

#if 0
#define ddprintf dprintf
#define dtrace(str) dprintf (" %s ", str)
#define dtrace1(str,x) dprintf (" %s%.2x ", str,(uchar)(x))
#define dtrace2(str,x,y) dprintf (" %s%.2x,%.2x ", str,(uchar)(x),(uchar)(y))
#else
#define ddprintf dprintf
#define dtrace(str)
#define dtrace1(str,x)
#define dtrace2(str,x,y)
#endif

/* =================================================================================== */
/*	configuration constants                                                            */
/* =================================================================================== */

#define KB_CMD_TIMEOUT	1000000

#define CTRLR_LATENCY 100000		/*Delay before ctrlr interrupt fires */
#define KBMS_LATENCY 1000			/*Delay before kb-timer interrupt fires */ 
#define KBMS_TIMER_CNT 5			/*iterations of kb & ms timers */
#define MAX_EMPTY_MS_INTS 25		/*if exeeded indicates streaming interrupts */

#define KB_DRIVER_COOKIE	'kbd '
#define PS2_MOUSE_DRIVER_COOKIE	'mous'

#define KB		0x60
#define KBIRQ	1


/* macros for dealing with key_states - do not attempt to read */
#define IS_KEY_SET(ks, k)			((ks)[(k)>>3]&(1<<((7-(k))&7)))
#define SET_KEY(ks, k)				((ks)[(k)>>3]|=(1<<((7-(k))&7)))
#define UNSET_KEY(ks, k)			((ks)[(k)>>3]&=~(1<<((7-(k))&7)))
#define TOGGLE_KEY(ks, k)			((ks)[(k)>>3]^=(1<<((7-(k))&7)))

char *kATKeyboardDeviceName = "input/keyboard/at/0";
char *kPS2MouseDeviceName = "input/mouse/ps2/0";

/* --------------------------------------------------------------------
/	forward declarations
/ */

static int32 kb_inth(void *);
static int32 ms_inth(void *);
static void set_mouse(int, int, int);
static int  set_key_repeat_info(int rate, bigtime_t delay);
static status_t reset_timer_handler(timer *t);
extern void	do_control_alt_del();
static status_t kbtimer_inth(timer *t);
static status_t mstimer_inth(timer *t);
static status_t ctrlr_inth(timer *t);
static int32 kb_rd_obuf(void);
static int32 ms_rd_obuf(void);
static int32 parity_hook(void);
static status_t enable_keyboard(void);
#if 0
static void unhandled_interrupt_hook_8042();
#endif

/* --------------------------------------------------------------------
/	private globals
/ */

static isa_module_info	*isa;
static char isa_name[] = B_ISA_MODULE_NAME;
static uint8			cmd_byte;				/* current byte */
static struct cbuf_rec	*kb_buffer = NULL;		/* circular buf for mouse */
static struct cbuf_rec	*ms_buffer = NULL;		/* circular buf for mouse */
static vint32			waiting = 0;			/* waiting for a result count */
static vint32			msopen_cnt = 0;			/* limit number of mouse opens to one */
static vint32			kbopen_cnt = 0;			/* limit number of kb opens to one */
static sem_id			kb_wait_sem = -1;
static vuchar			keyboardResult[16];			/* command result */
static int				last_held_mouse_buttons = 0;
static int				mouse_buttons = 0;
static int				mouse_xdelta = 0;
static int				mouse_ydelta = 0;
static int				wheel_delta = 0;
static long				click_count = 0;
static bigtime_t		last_click = 0;
static long				mouse_mods = 0;
static short			mouse_sync = 0;
static short			mouse_bytes = 0;
static bigtime_t		mouse_time = 0;
static short			mouse_plugged = 0;		
static mouse_type		ms_type = MOUSE_3_BUTTON;
static int				mouse_mapping[3] = {1, 2, 4};
static mouse_accel		accel = {TRUE, 7, 1};
static bigtime_t		click_speed = 500000;	// default .5 second
static sem_id			kbsem = -1;
static sem_id           mssem = -1;
static short			ps2_mouse_found = 0;	/* notfound=0, fnd=1, err=2 */

static sem_id			opensem = -1;
static sem_id			resetms_sem = -1;
static volatile short	resetms = 0;

static volatile short	msstate=0;
static volatile short 	unread=0;

static int32            typematic_rate = 250;
static bigtime_t        repeat_delay   = 250;
static uint8			key_states[32];
static uint8			non_repeating[32];

static short			found_keyboard = 0;

typedef struct {
	struct timer		t;
	int					cnt;
	bool				flag;
}	timer_str;

static timer_str		kbtimer;
static timer_str		mstimer;
static timer_str		ctrlrtimer;
static vint32			ctlr_init = 0;
static vint32			msinth_installed = 0;  		/* 1 = ms inth installed */

static timer			reset_timer;
static bigtime_t		control_alt_del_timeout = KB_DEFAULT_CONTROL_ALT_DEL_TIMEOUT;
static bool				reset_pending = FALSE;
static bool				debugger_keys = TRUE;

static int32 			ms_spinlock = 0;
static int32 			kb_spinlock = 0;

static volatile int32	tstcnt=0;




/* ====================================================================
/	wait_status waits until the status register bit identified in the
/	passed mask reaches the passed value, with a failsafe timeout.
/ 	WARNING: This routine should not be called when interrupts are disabled
/		because it calls snooze. */

static int wait_status(uchar mask, uchar value)
{
	int		i;
	int		desired_state;
	desired_state = (value != 0);

	if ( (((*isa->read_io_8) (KB+KB_STATUS) & mask) != 0) == desired_state)
			return B_NO_ERROR;
			
	if (!(interrupts_enabled())) {
		ddprintf("kb_mouse: ERROR: wait_status called w/interrupts disabled. \n");
		return B_ERROR;		
	}
		
	for (i = 100; i; i--) {	/* retry for 50 millisecs */
		if ( (((*isa->read_io_8) (KB+KB_STATUS) & mask) != 0) == desired_state)
			return B_NO_ERROR;
		snooze(500);  	/* int enabled: go to sleep while waiting for bit to be posted */
	}
	return B_ERROR;
}

/* --------------------------------------------------------------------
/	get_data waits for data to appear (output data buffer full), and
/	returns it.
/ */

static int get_data(uchar *data, uchar mask)
{
	/* wait till output buffer full flag set */
	if (wait_status(mask, 1) != B_NO_ERROR) 
		return B_ERROR;

	*data = (*isa->read_io_8) (KB + KB_OBUF);
	dtrace1 ("gd",*data);
	return B_NO_ERROR;
}


/* --------------------------------------------------------------------
/	remove_data flushes any pending data from the controller output buffer
/ */

static void remove_data(void)
{
	uchar	val;	
	dtrace ("rd");
	while (wait_status(KBS_OBF, 1) == B_NO_ERROR) {
		val = (*isa->read_io_8) (KB + KB_OBUF);
		tstcnt=0;			/*we may be stealing byte before inth has a chance */
		ddprintf ("flushed %.2x\n", val);
	}
}

/* --------------------------------------------------------------------
/	put_data waits until the input data buffer is empty, then writes
/	a value there.
/ */

static int put_data(uchar data)
{
	dtrace1 ("pd",data);
	/* wait till controller input buffer empty */
	if (wait_status(KBS_IBF, 0) != B_NO_ERROR) {
		ddprintf("put_data error: input buffer not empty!\n");
		return B_ERROR;
	}

	(*isa->write_io_8) (KB + KB_IBUF, data);
	return B_NO_ERROR;
}

/* --------------------------------------------------------------------
/	controller_command writes a controller command to the
/	command register.  It assumes the command take no arguments, and
/	waits until the command finishes executing.  
/ */

static int controller_command(uchar cmd)
{
	dtrace1 ("cc1",cmd);
	/* wait till controller input buffer empty */
	if (wait_status(KBS_IBF, 0) != B_NO_ERROR) {
		ddprintf("controller_command: input buffer not empty!\n");
		return B_ERROR;
	}

	(*isa->write_io_8) (KB+KB_COMMAND, cmd);

	return B_NO_ERROR;
}

/* --------------------------------------------------------------------
/	controller_command_with_data writes a command to the controller
/	command register and an argument to the data register, and then
/	waits until the command finishes executing.  
/ */

static int controller_command_with_data(uchar cmd, uchar data)
{
	dtrace2 ("ccwd",cmd,data);
	if (controller_command(cmd) != B_NO_ERROR)
		return B_ERROR;
	return put_data(data);
}

/* --------------------------------------------------------------------
/	keyboard_command writes a single byte to the keyboard, and checks that
/   the keyboard sends an ACK.
/ */

static int
keyboard_command (uchar cmd)
{
	uchar	ack;
	short	retry = 5;
	dtrace1 ("kc",cmd);
	do {
		if (put_data(cmd) != B_NO_ERROR)
			break;
		if (get_data(&ack, KBS_OBF) != B_NO_ERROR)
			break;
		if (ack == KBC_ACK)
			return B_NO_ERROR;
		retry--;
	} while ((ack == KBC_RESEND) && (retry));
	
	ddprintf("!!!! keyboard_command error! %x %x\n", cmd, ack);

	return B_ERROR;
}

/* --------------------------------------------------------------------
/	change_command_bits changes selected bits in the keyboard controller's
/	command register.
/ */

static int change_command_bits(uchar mask, uchar value)
{
	int	retry;
	int	err;
	dtrace2 ("ccb",mask,value);
	cmd_byte &= ~mask;		/* clear selected bits */
	value &= mask;			/* select new bits */
	cmd_byte |= value;		/* merge new bits */

	for (retry = 0; retry < 3; retry++) {
		err = controller_command_with_data(KBC_WRCOMMAND, cmd_byte);
		if (err == B_NO_ERROR) {
			ddprintf("Changed bits finished w/out errors \n");
			return err;
		}	
	}
	return err;
}

/* ----------
	init_cmd_byte - read the initial command byte from the controller
----- */
static void
init_cmd_byte(void)
{
	int	i;

	for (i = 0; i < 3; i++) {
		dtrace ("icb1");
		remove_data();
		
		if (controller_command(KBC_RDCOMMAND) != B_NO_ERROR)
			continue;
		dtrace ("icb2");
		
		if (get_data(&cmd_byte, KBS_OBF) == B_NO_ERROR) 
			break;
	}
	dtrace1 ("icb3",cmd_byte);
}

/* --------------------------------------------------------------------
/	active_keyboard_command_with_data - sends a command w/data to keyboard 
/   while the keyboard is enabled and sending us scan codes.
/ */
static int
active_keyboard_command_with_data (uchar cmd, uchar data)
{
	int	retry;
	int	err = B_ERROR;
	dtrace2 ("akcwd1",cmd,data);
	for (retry = 0; retry < 3; retry++) {

		waiting = 1;
		if (put_data(cmd) != B_NO_ERROR)
			continue;
		dtrace ("akcwd2");
		err = acquire_sem_etc(kb_wait_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, KB_CMD_TIMEOUT);
		if (err == B_INTERRUPTED)
			break;
    	if (err != B_NO_ERROR)
			continue;

		dtrace ("akcwd3");
		if (keyboardResult[0] != KBC_ACK) {
			dtrace ("akcwdERR1");
			continue;
		}

		waiting = 1;
		if (put_data(data) != B_NO_ERROR)
			continue;
		err = acquire_sem_etc(kb_wait_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, KB_CMD_TIMEOUT);
		if (err == B_INTERRUPTED)
			break;
    	if (err != B_NO_ERROR)
			continue;

		if (keyboardResult[0] == KBC_ACK)
			return B_NO_ERROR;
	}
	dtrace ("akcwd4");
	waiting = 0;
	return err;
}


// --------------------------------------------------------------------
// active_keyboard_command_get_results - sends a command w/data to keyboard 
//   and gets some returned data, while the keyboard is enabled and sending us
//   scan codes.
//
static int
active_keyboard_command_get_results (uchar cmd, uchar *result, int result_count)
{
	int	retry;
	int err = B_ERROR;

	dtrace1 ("akcgr1",cmd);
	for (retry = 0; retry < 3; retry++) {

		waiting = 1 + result_count;
		if (put_data(cmd) != B_NO_ERROR)
			continue;
		dtrace ("akcgr2");
		err = acquire_sem_etc(kb_wait_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, KB_CMD_TIMEOUT);
		if (err == B_INTERRUPTED)
			break;
    	if (err != B_NO_ERROR)
			continue;

		if (keyboardResult[result_count] != KBC_ACK)
			continue;

		memcpy (result, (void *)keyboardResult, result_count);
		return B_NO_ERROR;
	}
	dtrace ("akcgr3");
	waiting = 0;
	return err;
}

/* --------------------------------------------------------------------
/	initialize the keyboard
/ */

static void
init_keyboard()
{
	uchar	tmp;
	short	retry;
	int		err;
	
	remove_data();
	found_keyboard = 0;

	if (controller_command(KBC_SLFTST) == B_NO_ERROR) {
		for (retry = 0; retry < 5; retry++) {
			if (get_data(&tmp, KBS_OBF) == B_NO_ERROR) {
				if (tmp == 0x55) {
					ddprintf("got 0x55 after %d retries\n", retry);
					break;
				} else {
					ddprintf("kb controller test failed: %.2x\n", tmp);
				}
			}
		}
		if (retry == 5)
			ddprintf("Keyboard self-test failed after %d retries.\n", retry);
	} else {
		ddprintf("kb controller test failed\n");
	}
	
	if (controller_command(KBC_KBTST) == B_OK) {
		for (retry = 0; retry < 5; retry++) {
			if (get_data(&tmp, KBS_OBF) == B_OK) {
				if (tmp == 0) {
					ddprintf("got a 0 after %d retries\n", retry);
					break;
				} else {
					ddprintf("kb interface test failed: 0x%x\n", tmp);
				}
			}
		}
		if (retry == 5)
			ddprintf("Keyboard interface test failed after %d retries.\n", retry);
	} else {
		ddprintf("kb interface test failed\n");
	}
	
	if ((controller_command(KBC_ENABLEKB))==B_OK) { 			/* enabled keyboard */
		found_keyboard = 1;
		ddprintf("Found a keyboard!\n");
	} else
		ddprintf("No keyboard!\n");

	if (keyboard_command(KB_DISABLE) != B_OK) 
		ddprintf("Error disabling kb\n");
	
	if (found_keyboard) {
		err = enable_keyboard();
		if (err==B_ERROR)
			found_keyboard = 0;  /*enable kb failed. Do not publish */	
	}
}


/* --------------------------------------------------------------------
/	enable the keyboard
/ */

static status_t
enable_keyboard(void)
{
	short	retry;
	retry = 8;
	remove_data();
	while (retry--) {	/* enable the keyboard */
		remove_data();
		if (keyboard_command(KB_ENABLE) == B_NO_ERROR)
			return B_NO_ERROR;
		ddprintf("Response to keyboard enable was not ACK!\n");
	}
	return B_ERROR;
}


/* --------------------------------------------------------------------
/	disable the ps2 mouse
/ */

static status_t
disable_ps2_mouse(void)
{
	uchar	tmp;
	short	retry;

	remove_data();
	retry = 8;
	while (retry--) {	/* disable the mouse */
		controller_command_with_data(KBC_WRMOUSE, MS_DISABLE);
		if (get_data(&tmp, KBS_OBF) == B_NO_ERROR) {
			if (tmp == KBC_ACK)
				return (B_NO_ERROR);
		}
		remove_data();
	}
	ddprintf("Disable ps2 mouse failed: response was not ACK! (was %.2x)\n", tmp);
	return (B_ERROR);
}

/* --------------------------------------------------------------------
/	initialize the ps2 mouse
/ */

static int
init_ps2_mouse(void)
{
	uchar	tmp,status;
	int		result = B_NO_ERROR;
	
	if (controller_command(KBC_ENABLEAUX) == B_OK)
		ddprintf("Enabling aux interface worked!\n");
	else
		ddprintf("Enabling aux interface failed\n");
	
	/* this seems to get the mouse started */
	controller_command(KBC_WRMOUSEOB);
	put_data(0xbe);
	if (wait_status(KBS_OBF|KBS_ODS, 1) == B_OK)
		tmp = (*isa->read_io_8)(KB + KB_OBUF);
		
	result = disable_ps2_mouse();
	ps2_mouse_found = 0;		/*0 = ps2_mouse not found (but ok to hotplug) */
	if (result == B_NO_ERROR) {
		ps2_mouse_found = 1;		/*1 = ps2_mouse found */
		ddprintf("Found a PS2 mouse!\n");
	} else {
		ddprintf("PS2 mouse Not Found!\n");
		status = (*isa->read_io_8) (KB + KB_STATUS);
		if (status & KBS_GTO) {	  /*repeating gtos */
			ddprintf("***GEN'L TIME OUT ERROR Initializing PS2 Mouse  %.2x *****\n",status);
			ps2_mouse_found = 2;   /*2 = Mouse not found and gto errors occuring */
		}			
	}	
	result=B_NO_ERROR;

/* disable mouse test for intel part since it always fails */
#ifdef MOUSE_TEST
	retry = 3;
	while (retry--) {	/* test the mouse interface */
		if (controller_command(KBC_TSTAUX) == B_NO_ERROR) {
			if (get_data(&tmp, KBS_OBF) == B_NO_ERROR) {
				if (tmp != 0x00)
					ddprintf("mouse test failed: %.2x\n", tmp);
				else
					break;
			}
		}
		else
			ddprintf("mouse test failed\n");
	}
#endif

#ifdef MOUSE_ID
	retry = 3;
	while (retry--) {	/* check mouse ID */
		controller_command_with_data(KBC_WRMOUSE, MS_RDID);
		if (get_data(&tmp, KBS_OBF) == B_NO_ERROR) {
			if (tmp == KBC_ACK) {
				if (get_data(&tmp, KBS_OBF) == B_NO_ERROR) {
					if (tmp == 0x00)
						goto done2;
					else
						ddprintf("mouse ID is not 0: %x\n", tmp);
				}
			}
		}
		if (!retry)
			result = B_ERROR;
	}
done2:;
#endif

	return result;
}



/* --------------------------------------------------------------------
/	enable the ps2 mouse
/ */

static int set_ps2_sample_rate(int rate)
{
	uchar	tmp;
	short 	retry = 5;

	while (retry--) {
		controller_command_with_data(KBC_WRMOUSE, MS_SETSAMPLE);
		if(get_data(&tmp, KBS_OBF) != B_NO_ERROR || tmp != KBC_ACK)
			continue;	/*if errors, try again */
	
		controller_command_with_data(KBC_WRMOUSE, rate);
		if(get_data(&tmp, KBS_OBF) != B_NO_ERROR || tmp != KBC_ACK) 
			continue;
		
		return B_NO_ERROR;	/*else no errors, rtn okay */	
	}
	return	B_ERROR		/*unable to set rate after 5 tries */;
}

static void enable_ps2_intellimouse()
{
	short	retry = 5;
	
	while (retry--) {
		if (set_ps2_sample_rate(200)== B_ERROR)
			continue;
		if (set_ps2_sample_rate(100)== B_ERROR)
			continue;
		if (set_ps2_sample_rate(80) == B_ERROR)
			continue;
		if (set_ps2_sample_rate(100)== B_ERROR)
			continue;
	dprintf("Set rates succeeded: Enabled ps2 Intellimouse \n");		
		return;
	}
	dprintf("unable to set sample rate properly after 5 tries \n");	
}


static int enable_ps2_mouse(void)
{
	uchar	tmp;
	short	retry;

	retry = 10;
	mouse_bytes = 3;

		
	while (retry--) {
		
		controller_command_with_data(KBC_WRMOUSE, MS_ENABLE);
		
		if (get_data(&tmp, KBS_OBF) != B_NO_ERROR)
			continue;

		if (tmp == KBC_RESEND)
			continue;
			
		if (tmp == KBC_ACK) {
			ddprintf("Successfully enabled PS2 mouse\n");
			enable_ps2_intellimouse();
			return (B_NO_ERROR);
		}
	}
	ddprintf("Enable ps2 mouse failed \n");
	return (B_ERROR);
}


/* ====================================================================
	read the number of outstanding events for a ps2 mouse
*/
long
ps2_mouse_num_events(void)
{
	long count;

	get_sem_count(mssem, &count);

	return count;
}
/* ====================================================================
	read the state of the PS/2 mouse (for MS_READ)
*/

static status_t
do_ps2_mouse_read(void)
{
	int 		x = 0;
	int			y = 0;
	uint		b1;
	uchar		data;
	static  	uchar buff[4];      /* it's static so it's in locked memory */
	status_t	err = B_NO_ERROR;

	/* interlock w/int handler & wait for data */
	err = acquire_sem_etc(mssem, 1, B_CAN_INTERRUPT, 0);
	if (err != B_NO_ERROR)
		return (err);
	
	if (msinth_installed ==1 ) { 	 /*ms-inth has been disabled -disable ms in kb ctrlr */
		change_command_bits (KBM_EMI, 0);  /*disable mouse ints whil initializing*/
	 	ddprintf("Mouse disabled in kb_controller\n");
	 	return (B_ERROR);	
   	}	

	if (resetms) {					/*inth detected mouse needs to be re-initialized */
			err = acquire_sem_etc(resetms_sem,1, B_CAN_INTERRUPT,0);
			if (err != B_NO_ERROR) {
					ddprintf("ms-read Error: could not aquire resetms sem\n");
				return (B_ERROR);
			}

			if (resetms) {
				ddprintf("ms-read: calling enable_ps2_mouse\n");
				resetms=0;
			    change_command_bits (KBM_EMI, 0);  /*disable mouse ints while initializing*/
			   	msstate=0;			/* don't post anymore bytes to buffer */ 
			   	remove_data();				   	
				enable_ps2_mouse();
				ps2_mouse_found = 1;	/* set flag that we have a ps2 mouse */
				mouse_bytes=3;
				mouse_sync=0;
				cbuf_flush(ms_buffer);	
				change_command_bits (KBM_EMI, KBM_EMI);  /*re-enable interupts from mouse */
				data = (*isa->read_io_8) (KB + KB_OBUF); /*clear out extraneous bytes from obuf */
				msstate=1;			/*working state - start posting again*/				
				ddprintf("Mouse re-connected\n");
			}	
			release_sem_etc(resetms_sem, 1, 0);
			return (B_ERROR);		
	}

	if (cbuf_getn(ms_buffer, (char *)buff, mouse_bytes) != mouse_bytes) {   
		/* ddprintf("Yikes! didn't read %d bytes from ms_buffer\n", mouse_bytes); */
		return (B_ERROR);
	}

#if 0
	if(mouse_bytes == 4)
		dprintf("got mouse data 0x%02x 0x%02x 0x%02x 0x%02x\n",
		        buff[0], buff[1], buff[2], buff[3]);
	else
		dprintf("got mouse data 0x%02x 0x%02x 0x%02x\n",
		        buff[0], buff[1], buff[2]);
#endif
	
	b1 = buff[0];
	if (b1 & 0x10)				/* x is neg */
		x = 0xFFFFFF00;
	if (b1 & 0x20)				/* y is neg */
		y = 0xFFFFFF00;

	x |= buff[1]; /* get distances */
	y |= buff[2]; /* get distances */
	
	if(mouse_bytes == 4) {
		wheel_delta = (int8)buff[3];
	}
	else {
		wheel_delta = 0;
	}

	set_mouse(x, y, b1 & 0x7);

	return (B_NO_ERROR);
}

/* ====================================================================
/	kb_mouse_close shuts down the driver.
/ */

static long
kb_mouse_close(void *cookie)
{
	return B_NO_ERROR;
}

static long
kb_mouse_free(void *cookie)
{
	ddprintf("kbmouse free\n");

	if (cookie == (void *)PS2_MOUSE_DRIVER_COOKIE) {
		cancel_timer((timer *)&mstimer);
		change_command_bits (KBM_EMI, 0);
		disable_ps2_mouse();
		if (atomic_or((long *)&msinth_installed,1)==0) { /*was prev inth removed due to errors? */
			remove_io_interrupt_handler (12, ms_inth, NULL);
			ddprintf("PS2 Mouse Free: inth removed \n");
		}	
		if (atomic_add((long *)&ctlr_init,-1)== 1)
			cancel_timer((timer *)&ctrlrtimer);
		msopen_cnt = 0;
		return B_OK;

	} else if (cookie == (void *)KB_DRIVER_COOKIE) {
		cancel_timer((timer *)&kbtimer);
		change_command_bits (KBM_EKI, 0);
		remove_io_interrupt_handler (KBIRQ, kb_inth, NULL);
		if (atomic_add((long *)&ctlr_init,-1)== 1)
			cancel_timer((timer *)&ctrlrtimer);
		kbopen_cnt = 0;
		return B_OK;
	} else 
		return B_ERROR;				/*cookie not found */
}

/* ====================================================================
/	kb_mouse_init sets up the controller hardware, installs the interrupt
/	handler and sets up circular buffers for keyboard and mouse data.
/
/	No effort is made to keep track of multiple clients.  To do so,
/	we need to not shut down the driver till the last client closes.
/ */


/* ----------
	init_hardware - one-time hardware initialization
----- */
status_t
init_hardware (void)
{
#if __POWERPC__
	if (platform() != B_BEBOX_PLATFORM)
		return ENODEV;
#endif

	return B_OK;
}

/* ----------
	init_driver - load-time driver initialization
----- */
long
init_driver (void)
{
	status_t	err;
	void *		settings;
	
	
	settings = load_driver_settings("kernel");
	if (settings != NULL)
	{
		debugger_keys = get_driver_boolean_parameter(settings, "debugger_keys", true, true);
		unload_driver_settings(settings);
	}

	ddprintf ("++++++++ init_driver kb_mouse \n");

	mstimer.flag=false;
	mstimer.cnt=0;
	kbtimer.flag=false;
	kbtimer.cnt=0;

	if ((err = get_module(isa_name, (module_info **)&isa)))
		return err;
	
	mstimer.flag=false;
	mstimer.cnt=0;
	kbtimer.flag=false;
	kbtimer.cnt=0;
	
	err = ENOMEM;
	if (!(kb_buffer = cbuf_init(32 * sizeof (at_kbd_io)+1)))
		goto err1;
	if (!(ms_buffer = cbuf_init(3*4*20+1)))
		goto err2;
	if ((err = kbsem = create_sem(0, "kbsem")) < 0)
		goto err3;
	if ((err = mssem = create_sem(0, "mssem")) < 0)
		goto err4;
	if ((err = kb_wait_sem = create_sem(0, "kb_wait_sem")) < 0)
		goto err5;
	if ((err = opensem = create_sem(1, "opensem")) < 0)
		goto err6;
	if ((err = resetms_sem = create_sem(1, "resetms_sem")) < 0)
		goto err7;
		
	memset (key_states, 0, sizeof (key_states));
	memset (non_repeating, 0, sizeof (non_repeating));

	init_keyboard();
	init_ps2_mouse();
	init_cmd_byte();
	
	control_alt_del_timeout = KB_DEFAULT_CONTROL_ALT_DEL_TIMEOUT;
	reset_pending = FALSE;
		
//	if (found_keyboard)
//		err = enable_keyboard();
	

//	if (err < 0)
//		goto err8;
	
	return B_NO_ERROR;
	
err8:
	delete_sem (resetms_sem);
err7:
	delete_sem (opensem);
err6:
	delete_sem (kb_wait_sem);	
err5:
	delete_sem (mssem);
err4:
	delete_sem (kbsem);
err3:
	cbuf_delete (ms_buffer);
err2:
	cbuf_delete (kb_buffer);
err1:
	put_module (isa_name);
	
	ddprintf ("kb_mouse initialization error\n");
	return err;
}


void
uninit_driver()
{
	ddprintf ("kb_mouse: uninit driver\n");

	if (reset_pending)
		cancel_timer(&reset_timer);
	cancel_timer((timer *)&ctrlrtimer);
	delete_sem (resetms_sem);
	delete_sem (opensem);
	delete_sem (kbsem);
	delete_sem (mssem);
	delete_sem (kb_wait_sem);
	cbuf_delete (kb_buffer);
	cbuf_delete (ms_buffer);
	put_module (isa_name);
}

static status_t
kb_mouse_open(const char *name, uint32 flags, void **cookie)
{
	status_t	err;
	uchar 		data;

	ddprintf("Kbmouse open '%s'\n", name ? name : "NULL");

	if (strcmp(name, kPS2MouseDeviceName) == 0) {
		err = acquire_sem_etc(opensem,1, B_CAN_INTERRUPT,0); /*synchronize kb & mouse opening */
		if (err != B_NO_ERROR) 
			return ENODEV;
		
		if (atomic_or((long *)&msopen_cnt, 1)!= 0) {		/*only allow a single mouse open */
			ddprintf("Kb_mouse open: Mouse is already opened. Only one at a time please. \n");
	 	 	release_sem_etc(opensem, 1, 0);
	 	 	return B_ERROR;  		
		}
		mstimer.cnt=0;
		mstimer.flag=false;
		msstate=0;
		ddprintf ("Opening PS2 Mouse\n");
		err=enable_ps2_mouse();		/* mouse can post bytes to kb ctrlr */ 
		msstate=1;					/*set state to working */
		mstimer.flag=false; 		/*init ms timer flag to inactive  */
		tstcnt=0;	
		if (atomic_add((long *)&ctlr_init, 1)== 0)
			add_timer((timer *)&ctrlrtimer,ctrlr_inth,CTRLR_LATENCY,B_PERIODIC_TIMER);
		ddprintf("Installing PS2 MS-INTH \n");
		atomic_and((long *)&msinth_installed,0);  
		install_io_interrupt_handler (12, ms_inth, NULL, 0);	/* mouse */
		change_command_bits(KBM_EMI, KBM_EMI);
		data = (*isa->read_io_8) (KB + KB_OBUF);  /*clear extraneous bytes from obuf*/
		*cookie = (void *)PS2_MOUSE_DRIVER_COOKIE;
		ddprintf ("PS2 Mouse Open Completed \n");				
		release_sem_etc(opensem, 1, 0);
		return B_OK;
	}
	
	if (strcmp(name, kATKeyboardDeviceName) == 0) {
		if (found_keyboard==0)				/* no kb connected, bail out */
			return ENODEV;
			
		err = acquire_sem_etc(opensem,1, B_CAN_INTERRUPT,0);  
		if (err != B_NO_ERROR) 
			return ENODEV;
			
		if (atomic_or((long *)&kbopen_cnt, 1) != 0) {	/*only allow a single KB open */
 	 		release_sem_etc(opensem, 1, 0);
 	 		return B_ERROR;  		
		}
		cancel_timer((timer *)&ctrlrtimer);
		atomic_and((long *)&ctlr_init, 0);
		*cookie = (void *)KB_DRIVER_COOKIE;
		ddprintf ("Opening AT keyboard\n");		
		install_io_interrupt_handler (KBIRQ, kb_inth, NULL, 0);
		change_command_bits(KBM_EKI | KBM_KCC | KBM_SYS, KBM_EKI | KBM_KCC | KBM_SYS);
		/* If user types on keyboard between intstall int handler and change_command,
		  extraneous bytes may remain unread in obuf.  The read below clears out 
		  any potential garbage left in the buffer*/
		data = (*isa->read_io_8) (KB + KB_OBUF);  /*clear extraneous bytes from obuf */
		ddprintf ("AT Keyboard Open Completed\n");
		set_key_repeat_info(typematic_rate, repeat_delay);
		if (atomic_add((long *)&ctlr_init, 1)== 0)
			add_timer((timer *)&ctrlrtimer,ctrlr_inth,CTRLR_LATENCY,B_PERIODIC_TIMER);
		release_sem_etc(opensem, 1, 0);		
		return B_OK;
	}

	return ENODEV;
}

static
int valid_rates[] = { 300, 267, 240, 218, 200, 185, 171, 160, 150,
                                   133, 120, 109, 100, 92, 86, 80, 75, 67,
                                   60, 55, 50, 46, 43, 40, 37, 33, 30, 27,
                                   25, 23, 21, 20 };
#define RATE_COUNT (sizeof(valid_rates) / sizeof(int))

static int valid_delays[] = { 250000, 500000, 750000, 1000000 };
#define DELAY_COUNT (sizeof(valid_delays) / sizeof(valid_delays[0]))

static int
set_key_repeat_info(int rate, bigtime_t delay)
{
	int i;
	int	 value = 0x7f;

	if (rate < 0)
		rate = typematic_rate;

	if (delay < 0)
		delay = repeat_delay;

	for (i = 0; i < RATE_COUNT; i++) {
		if (rate >= valid_rates[i]) {
			typematic_rate = valid_rates[i];
			value &= 0x60;
            value  = i;
            break;
		}
	}
	
	for (i = 0; i < DELAY_COUNT; i++) {
		if (delay <= valid_delays[i]) {
			repeat_delay = valid_delays[i];
            value &= 0x1f;
            value |= i << 5;
            break;
		}
	}

	active_keyboard_command_with_data (KB_SETRATE, value);
	return B_OK;
}



/* ====================================================================
/	kb_mouse_control
/ */

static long
kb_mouse_control(void *cookie, uint32 msg, void *buf, size_t len)
{
	int				i;
	status_t		err = B_NO_ERROR;

	switch (msg) {
	case KB_READ: {
		static at_kbd_io	k_locked;
		if (!(err = acquire_sem_etc(kbsem, 1, B_CAN_INTERRUPT, 0))) {	
			cbuf_getn(kb_buffer, (char *)&k_locked, sizeof(at_kbd_io));
			*(at_kbd_io *)buf = k_locked;
		}
		return err;
	}
			
	case KB_GET_KEYBOARD_ID:
		return active_keyboard_command_get_results (KB_RDID, (uchar *)buf, 2);

	case KB_SET_LEDS: {
		led_info *led = (led_info *) buf;
		i = 0;
		if (led->caps_lock)
			i |= 4;
		if (led->num_lock)
			i |= 2;
		if (led->scroll_lock)
			i |= 1;
		return active_keyboard_command_with_data (KB_SETLED, i);
	}

	case KB_SET_KEY_REPEATING: {

		uint32 key = *(uint32 *)buf;
		if (key > (sizeof (non_repeating) * 8))
			return EINVAL;

		UNSET_KEY (non_repeating, key);
		return B_OK;
	}

	case KB_SET_KEY_NONREPEATING: {

		uint32 key = *(uint32 *)buf;
		if (key > (sizeof (non_repeating) * 8))
			return EINVAL;

		SET_KEY (non_repeating, key);
		return B_OK;
	}

	case KB_SET_KEY_REPEAT_RATE:
		set_key_repeat_info(*(int32 *)buf, -1);
		return B_NO_ERROR;

	case KB_SET_KEY_REPEAT_DELAY:
		set_key_repeat_info(-1, *(bigtime_t *)buf);
		return B_NO_ERROR;

	case KB_GET_KEY_REPEAT_RATE:
		*(int32 *)buf = typematic_rate;
		return B_NO_ERROR;

	case KB_GET_KEY_REPEAT_DELAY:
		*(bigtime_t *)buf = repeat_delay;
		return B_NO_ERROR;

	case KB_SET_CONTROL_ALT_DEL_TIMEOUT:
		control_alt_del_timeout = *(bigtime_t *)buf;
		return B_NO_ERROR;

	case KB_DELAY_CONTROL_ALT_DEL: 
               control_alt_del_timeout = *(bigtime_t *)buf; 
               if (reset_pending) 
               { 
                       cancel_timer(&reset_timer); 
                       add_timer(&reset_timer, reset_timer_handler, 
                               control_alt_del_timeout, B_ONE_SHOT_RELATIVE_TIMER); 
               } 
               return B_NO_ERROR; 

    case KB_CANCEL_CONTROL_ALT_DEL: 
            if (reset_pending) 
            { 
                    ddprintf("kb_mouse: reset cancelled\n"); 
                    cancel_timer(&reset_timer); 
                    reset_pending = FALSE; 
             } 
             return B_NO_ERROR; 

	case MS_NUM_EVENTS:
		return ps2_mouse_num_events();

	case MS_READ: {
		mouse_pos	*mp = (mouse_pos *) buf;
		err = do_ps2_mouse_read();
		if (err != B_NO_ERROR)
			return (err);	

		mp->xdelta = mouse_xdelta;
		mp->ydelta = mouse_ydelta;
		mp->wheel_delta = wheel_delta;
		mp->buttons = 0;
		if ((ms_type == MOUSE_1_BUTTON) && (mouse_buttons))
			mouse_buttons = 1;
		else if ((ms_type == MOUSE_2_BUTTON) && (mouse_buttons & 0x4)) {
			if (mouse_mapping[0] == 2)
				mouse_buttons = (mouse_buttons & 0x2) + 0x1;
			else
				mouse_buttons = (mouse_buttons & 0x1) + 0x2;
		}
		if (mouse_buttons & 0x1)
			mp->buttons |= mouse_mapping[0];
		if (mouse_buttons & 0x2)
			mp->buttons |= mouse_mapping[1];
		if (mouse_buttons & 0x4)
			mp->buttons |= mouse_mapping[2];
		mp->clicks = click_count;
		mp->modifiers = mouse_mods;
		mp->time = mouse_time;
/*		dprintf("MOUSE: buttons 0x%x, clicks = %d, modifiers = %d, time = %d\n",
				mp->buttons, mp->clicks, mp->modifiers, mp->time); */
		return B_NO_ERROR;
	}

	case MS_GETA:
		*(mouse_accel *) buf = accel;
		return B_NO_ERROR;

	case MS_SETA:
		accel = *(mouse_accel *) buf;
		return B_NO_ERROR;

	case MS_GETTYPE:
		*(mouse_type *)buf = ms_type;
		return B_NO_ERROR;

	case MS_SETTYPE: {

		mouse_type	temp_type = *(mouse_type *)buf;
		if ((temp_type < MOUSE_1_BUTTON) || (temp_type > MOUSE_3_BUTTON))
			return B_ERROR;
		ms_type = temp_type;
		return B_NO_ERROR;
	}

	case MS_GETMAP: {

		map_mouse	*map = (map_mouse *)buf;
		map->left = mouse_mapping[0];
		map->right = mouse_mapping[1];
		map->middle = mouse_mapping[2];
		return B_NO_ERROR;
	}

	case MS_SETMAP:{
		map_mouse	*map = (map_mouse *)buf;
		if ((map->left & 7) && (map->left != 3))
			mouse_mapping[0] = map->left;
		if ((map->right & 7) && (map->right != 3))
			mouse_mapping[1] = map->right;
		if ((map->middle & 7) && (map->middle != 3))
			mouse_mapping[2] = map->middle;
		return B_NO_ERROR;
	}

	case MS_GETCLICK:
		*(bigtime_t *)buf = click_speed;
		return B_NO_ERROR;

	case MS_SETCLICK:
		click_speed = *(bigtime_t *)buf;
		return B_NO_ERROR;

	default:
			return B_ERROR;
	}

	return B_NO_ERROR;
}




/* ====================================================================
/	kb_mouse_read routine (not used - keyboard and mouse data is read
/	through individual control calls)
/ */

static long
kb_mouse_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	return EINVAL;
}


/* ====================================================================
/	kb_mouse_write routine - not used.
/ */

static long
kb_mouse_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	return EINVAL;
}


/* ----------
	reset_timer_handler
----- */
static status_t
reset_timer_handler(timer *t)
{
	do_control_alt_del();			/* never returns... */
	return B_HANDLED_INTERRUPT; 	/* but just in case it does... */
}

static int sysrq = FALSE;

/* ================================================================================ */
/*	keyboard interrupt handler														*/
/* ================================================================================ */

static int32
kb_inth(void *data)
{
	int				rtntype;
	uchar			k;
	int32			err;

	k = (*isa->read_io_8) (KB + KB_STATUS);		/*read kb ctrlr status reg */
	
	if 	(k & KBS_PERR) {			/* parity error? */
		ddprintf("Keyboard inth: Parity error %.2x\n", k);
		err=parity_hook();   		/*	previously: unhandled_interrupt_hook_8042();  */
		if (err==B_OK)
			return B_HANDLED_INTERRUPT;  /*else fall through w/normal processing */
	}

	unread=0;	
		
	if (k & KBS_OBF) {						/* has output buffer been posted yet? */
		acquire_spinlock(&kb_spinlock);
		if (kbtimer.flag == true) {
			cancel_timer((timer *)&kbtimer); /* no more timer iterations	*/
			kbtimer.cnt=0;
			kbtimer.flag=false;
		}	
		release_spinlock(&kb_spinlock);
		rtntype=kb_rd_obuf();				/* yes, go read byte and process it */
		return rtntype;
	}
	
	if (k & KBS_GTO)
		return B_HANDLED_INTERRUPT; 		/* gen'l time out error. No post to buffer. */
	
	acquire_spinlock(&kb_spinlock);		 	/*output buffer not posted yet -timer will rd byte */
	if (kbtimer.flag==false) {	
		kbtimer.cnt=0;
		rtntype=add_timer((timer *)&kbtimer,kbtimer_inth,KBMS_LATENCY,B_PERIODIC_TIMER);
		if (rtntype == B_OK) {
			kbtimer.flag=true;				/*timer activated*/
		}
	} 		
	release_spinlock(&kb_spinlock);
	return B_HANDLED_INTERRUPT;
}

/* ==================================================================== */
/*	KEYBOARD TIMER INTERRUPT HANDLER									*/
/* ==================================================================== */

static int32
kbtimer_inth(timer *t) 
{
	int				rtntype;
	uchar			k;

	timer_str *tt = (timer_str *)t;
	
	unread=0;
	
	k = (*isa->read_io_8) (KB + KB_STATUS);		/* read kb ctrlr status reg */
	
	if ((k & KBS_OBF) && !(k & KBS_ODS)) { 		/* was output buffer posted? */	
		acquire_spinlock(&kb_spinlock);
		cancel_timer((timer *)tt);				/* no more timer iterations	*/
		tt->cnt=0;
		tt->flag=false;
		release_spinlock(&kb_spinlock);
		rtntype=kb_rd_obuf();					/* read output buffer of kb ctrlr */
		return rtntype;
	}
	
	acquire_spinlock(&kb_spinlock);
	if (tt->cnt++ > KBMS_TIMER_CNT){		
		cancel_timer((timer *)tt);
		tt->cnt=0;								/*reset cnt to 0 */
		tt->flag=false;
	}
	release_spinlock(&kb_spinlock);
	return B_HANDLED_INTERRUPT;		
}
/* ==================================================================== */
/*	READ KB CTRLR OUTPUT BUFFER AND PROCESS THE DATA BYTE 				*/
/* ==================================================================== */
		
static int32
kb_rd_obuf(void)	
{
	bool			key_down;
	static bool		extended_key = FALSE;
	long			scount;
	uchar			k;
	at_kbd_io		key;
	

	k = (*isa->read_io_8) (KB + KB_OBUF);		/*read byte from obuf 	*/

//	 dprintf ("#%.2x %s\n", k, extended_key ? "eT" : "eF");	
//	 check for results from command to keyboard
	
	scount = atomic_add ((long *)&waiting, -1);
	if (scount > 0) {
		keyboardResult[scount-1] = k;
		if (scount == 1) {
			release_sem_etc(kb_wait_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;
		}
		return B_HANDLED_INTERRUPT;
	}

	/* deal w/goofy pause scancodes */
	if (k == 0xe1)
		return B_HANDLED_INTERRUPT;

	/* deal w/extended scancodes */
	if (k == 0xe0) {
		extended_key = TRUE;
		return B_HANDLED_INTERRUPT;
	}
 	/* characterize the scancode */
	key_down = (k & 0x80) ? FALSE: TRUE;
	k = (k & 0x7f) + (extended_key ? 0x80 : 0);
	extended_key = FALSE;
	
 	if (debugger_keys)
 	{
		/* deal w/debugger entry */
		if (k == 0x54) {
			if (key_down) {
				sysrq = TRUE;
			} else {
				sysrq = FALSE;
			}
			return B_HANDLED_INTERRUPT;
		}
				
		if (sysrq) {							/* alt + prtscrn */
			if(key_down && (k == 32)) {			/* + d = debugger request */	
				kernel_debugger("YOU KNOW WHAT YOU DOING.\n"); //"System request\n");
				sysrq = FALSE;	
				key.scancode = 0x38;
				UNSET_KEY (key_states, k);
				key.timestamp = system_time();
				key.is_keydown = FALSE;
				cbuf_putn(kb_buffer, (char *)&key, sizeof(key));
				release_sem_etc(kbsem, 1, B_DO_NOT_RESCHEDULE);	
				return B_INVOKE_SCHEDULER;
			} 
		}
		
		if(key_down &&
		  (k == 1) && 			/* escape */
		   IS_KEY_SET(key_states, 0x1d) &&	  /* left control? */
		   IS_KEY_SET(key_states, 0x38))   {   /* left alt? */
				kernel_debugger("YOU KNOW WHAT YOU DOING.\n"); //"System request\n");
				UNSET_KEY (key_states, k);
				key.scancode = 0x38;			/*release alt key */
				UNSET_KEY (key_states, 0x38);
				key.timestamp = system_time();
				key.is_keydown = FALSE;
				cbuf_putn(kb_buffer, (char *)&key, sizeof(key));
				release_sem_etc(kbsem, 1, B_DO_NOT_RESCHEDULE);
			
				key.scancode = 0x1d;			/*release ctrl key */
				UNSET_KEY (key_states, 0x1d);
				key.timestamp = system_time();
				key.is_keydown = FALSE;
				cbuf_putn(kb_buffer, (char *)&key, sizeof(key));
				release_sem_etc(kbsem, 1, B_DO_NOT_RESCHEDULE);
				return B_HANDLED_INTERRUPT;
			}
	}

	/* deal w/ctrl-alt-del */
	if (key_down && ((k==0x53) ||(k == 0xd3))) {	/* DEL key press? */
		if ( (IS_KEY_SET(key_states, 0x1d) /*||*/		/* left control? */
		      /*IS_KEY_SET (key_states, 0x9d)*/ 		/* right control? */
             ) &&
		     (IS_KEY_SET(key_states, 0x38) /*||*/		/* left alt? */
		      /*IS_KEY_SET (key_states, 0xb8)*/			/* right alt? */
		     )
		   )
		{	
			if (!reset_pending) {
				ddprintf("Kbmouse reset requested \n");
				add_timer(&reset_timer, reset_timer_handler,control_alt_del_timeout, B_ONE_SHOT_RELATIVE_TIMER);
				reset_pending = TRUE;
			}
		}
	}
	/* filter out unwanted repeating keys */
	if (key_down) {
		if (IS_KEY_SET(key_states, k)) {
			if (IS_KEY_SET (non_repeating, k))
				return B_HANDLED_INTERRUPT;
		} else
			SET_KEY (key_states, k);
	} else {
		UNSET_KEY (key_states, k);
	}


	/* cancel reset timer if a ctrl-alt-del key is released */
	if (reset_pending && !key_down) {
		if ((k == 0x53) || (k == 0xd3)	/* DEL key released? */
		  || !IS_KEY_SET(key_states, 0x1d)			/* no left control? */
		     || !IS_KEY_SET(key_states, 0x38)		/* no left alt? */
		     )
		{
			ddprintf("kb_mouse: reset aborted \n");
			cancel_timer(&reset_timer);
			reset_pending = FALSE;
		}
	}

	//* add key to the read buffer */	
	//  dprintf ("$%.2x %s\n", k, key_down ? "kdT" : "kdF"); 
	
	key.scancode = k;
	key.timestamp = system_time();
	key.is_keydown = key_down;
	cbuf_putn(kb_buffer, (char *)&key, sizeof(key));
	release_sem_etc(kbsem, 1, B_DO_NOT_RESCHEDULE);
	
	return B_INVOKE_SCHEDULER;
}

/* ========================================================================= */
/* Controller timer: prevents unread data in obuf from locking kb & mouse,   */
/*                   and looks out for mouse hot plugging.					 */
/* ========================================================================= */
static int32
ctrlr_inth(timer *t) 
{
	uchar status;
	uchar k;
	
	status = (*isa->read_io_8) (KB + KB_STATUS);		/*read kb ctrlr status reg */
		
	if (!(status & KBS_OBF)) {			/*no char in obuf */
		unread = 0; 
		return	B_HANDLED_INTERRUPT;
	}	
	
	if (!(unread)) {		/*give kb-inth & ms-inth another chance at reading byte */
		unread=1;
		return	B_HANDLED_INTERRUPT;
	}
	
	acquire_spinlock(&ms_spinlock);
	status = (*isa->read_io_8) (KB + KB_STATUS);
	if ((status & KBS_ODS) && (mstimer.flag==true)){
		release_spinlock(&ms_spinlock);
		return B_HANDLED_INTERRUPT;          /*don't clear, let timer pick it up */
	}
	release_spinlock(&ms_spinlock);
	
	acquire_spinlock(&kb_spinlock);
	status = (*isa->read_io_8) (KB + KB_STATUS);
	if (!(status & KBS_ODS) && (kbtimer.flag==true)) {
		release_spinlock(&kb_spinlock);
		return B_HANDLED_INTERRUPT;          /*don't clear, let timer pick it up */
	}	
	release_spinlock(&kb_spinlock);
	
	
	k = (*isa->read_io_8) (KB + KB_OBUF);	/*else, read output buffer */

//ddprintf("CTRLR_INTH: status: %.2x, Unread byte %.2x \n",status,k);
	
	unread=0;
	if ((k==0xaa) && (status & KBS_ODS)) {			/* mouse plugged in */
		k = (*isa->read_io_8) (KB + KB_OBUF);		/*read next byte from cntrlr */
		if (k==0) {	
			ddprintf("CTRLR_INTH: 0x00 ==>  Mouse plugged in\n");
			resetms=1;					/* when doing the read, init the mouse */
			mouse_plugged = 0x00;		/* reset flag in int handler*/
			release_sem_etc(mssem, 1, B_DO_NOT_RESCHEDULE);	 /*let ms-read do full init of mouse */
			return	B_HANDLED_INTERRUPT;
		} else 
			return	B_HANDLED_INTERRUPT;
	}	
	return	B_HANDLED_INTERRUPT;	
}


/* ==================================================================== */
/*	MOUSE TIMER INTERRUPT HANDLER									*/
/* ==================================================================== */

static int32
mstimer_inth(timer *t) 
{
	int				rtntype;
	uchar			k;
	
	timer_str *tt = (timer_str *)t;

	unread=0;
	k = (*isa->read_io_8) (KB + KB_STATUS);		/* read kb ctrlr status reg */
	
	if ((k & KBS_OBF) && (k & KBS_ODS)) { 		/* was output buffer posted? */
		acquire_spinlock(&ms_spinlock);
		cancel_timer((timer *)tt);				/* no more timer iterations	*/
		tt->cnt=0;
		tt->flag=false;
		release_spinlock(&ms_spinlock);
		rtntype=ms_rd_obuf();					/* read output buffer of kb ctrlr */
		return rtntype;
	}
	
	if (tstcnt > MAX_EMPTY_MS_INTS) {
		if (atomic_or((long *)&msinth_installed,1)== 0) {
			remove_io_interrupt_handler (12, ms_inth, NULL);
			ddprintf("Streaming Interrupts: REMOVED MS-INTH \n");
			tstcnt=0;
			acquire_spinlock(&ms_spinlock);
			cancel_timer((timer *)tt);
			tt->cnt=0;								/*reset cnt to 0 */
			tt->flag=false;
			release_spinlock(&ms_spinlock);
			release_sem_etc(mssem, 1, B_DO_NOT_RESCHEDULE); /* read hk will disable ctrlr */
			return B_INVOKE_SCHEDULER;
		}
	}
	
	acquire_spinlock(&ms_spinlock);
	if (tt->cnt++ > KBMS_TIMER_CNT) {		
		cancel_timer((timer *)tt);
		tt->cnt=0;								/*reset cnt to 0 */
		tt->flag=false;
	}
	release_spinlock(&ms_spinlock);
	return B_HANDLED_INTERRUPT;				
}

/* ====================================================================
/	mouse interrupt handler
/  */

static int32
ms_inth(void *data)
{

	uchar		k;
	int			rtntype;
	int32		err;	
	
	k = (*isa->read_io_8) (KB + KB_STATUS);

	if (!(k & KBS_OBF))			/*interupt w/out obuf posted */
		tstcnt++;
	
	if (!(k & KBS_ODS))			/*protect against useless int*/
		return	B_UNHANDLED_INTERRUPT;

	if 	((k & KBS_PERR)) {			/* parity error? */	
		err=parity_hook();   		/*	previously: unhandled_interrupt_hook_8042();  */
		if (err==B_OK)
			return B_HANDLED_INTERRUPT;  /*else fall through w/normal processing */
	}

	unread = 0;
	
	if (k & KBS_OBF) {				/* has output buffer been posted yet? */
		acquire_spinlock(&ms_spinlock);
		if (mstimer.flag == true) {
			cancel_timer((timer *)&mstimer);	/* no more timer iterations	*/
			mstimer.cnt=0;
			mstimer.flag=false;
		}	
		release_spinlock(&ms_spinlock);
		rtntype=ms_rd_obuf();		/* yes, go read byte and process it */
		return rtntype;
	}
		
	acquire_spinlock(&ms_spinlock);	/*output buffer not posted yet -timer will rd byte */
	if (mstimer.flag==false) {	
		mstimer.cnt=0;
		rtntype=add_timer((timer *)&mstimer,mstimer_inth,KBMS_LATENCY,B_PERIODIC_TIMER);
		if (rtntype == B_OK) {
			mstimer.flag=true;		   /*timer activated*/
		}
	} 
	release_spinlock(&ms_spinlock);
	return B_HANDLED_INTERRUPT;

}

/* ==================================================================== */
/*	PROCESS BYTE FOR MOUSE FROM KB CTRLR OBUF 							*/
/* ==================================================================== */

static int32
ms_rd_obuf(void)	
{
	uchar			data;
	
	data = (*isa->read_io_8) (KB + KB_OBUF);	/*read byte from obuf */			

	tstcnt = 0;				/* reset cnt to indicate obuf posted */
	
	if (mouse_plugged == 0x01) {	/* did an 0xaa go by last time? */
		if (data == 0x00) {			/* yes? then is it 0x00 now? */
			resetms=1;				/* reset mouse upon return to do-ps2-read */ 
			ddprintf("mouse was plugged in: set flag to re-enable mouse\n");
			mouse_plugged = 0x00;		/* yes? eat it and reset flag */
			release_sem_etc(mssem, 1, B_DO_NOT_RESCHEDULE);	
			return B_HANDLED_INTERRUPT;
		}
		else
			mouse_plugged = 0x00;	/* no 0x0 after 0xaa? clear flag and cont */
	}

	if (data == 0xaa) {				/* 0xaa? might be mouse being plugged in */
		mouse_plugged = 0x01;		/* set flag but also continue in case not */
	}

	if (msstate==0)  	{				/* in the process of resetting mouse */	
		return B_HANDLED_INTERRUPT; /* don't post any bytes to cbuff */
	}

	if ((mouse_sync == 0) && !(data & 0x08)) { /* check for mouse out of sync */
		ddprintf("Mouse re-synced (bad data -> %x)\n", data);
		if(mouse_bytes != 3)
			mouse_bytes = 3;
		else
			mouse_bytes = 4;
		return B_HANDLED_INTERRUPT;
	}

	cbuf_put(ms_buffer, data);		/* put mouse byte in buffer */

	if (++mouse_sync == mouse_bytes) {		/* mouse talks in threes or fours */
		mouse_sync = 0;				/* reset counter */
		release_sem_etc(mssem, 1, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}

static void
set_mouse(int x, int y, int b)
{
	bigtime_t	now;
	now = system_time();
	if ((!mouse_buttons) && (b)) {
		if (last_click + click_speed > now && b == last_held_mouse_buttons)
			click_count++;
		else
			click_count = 1;
	}
	
	if (b) {
		last_click = now;
		last_held_mouse_buttons = b;
	}
		
	mouse_buttons = b;	         /* save mouse button state */
	mouse_xdelta = x;
	mouse_ydelta = y;
	mouse_mods = 0;
	mouse_time = now;
}

/* ===========================================================================  */
/*	Parity Error Hook														*/

static int32
parity_hook(void)
{
	uchar	status;
	uchar	tmp;

	status = (*isa->read_io_8) (KB + KB_STATUS);	/*read status byte again */
	
	if (!(status & KBS_PERR))	 { 					/*no more parity error */
		return B_ERROR;			/* return to normal processing */ 
	}
	
	if (status & KBS_OBF) {		/* if data in obuf, clear it */
		tmp = (*isa->read_io_8) (KB + KB_OBUF);	
		ddprintf("Parity or Genl Time out error; Obuf data lost: %.2x \n",tmp);
	}		
	return	B_OK;		/* since resend cmd seems to fail all the time, bypass bad data */			
}

#if 0

/* ====================================================================
/	unhandled interrupt handler		-- replaced with parity_hook
/ */

static void unhandled_interrupt_hook_8042(void)
{
	uchar	command;
	uchar	status;
	uchar	tmp;

	status = (*isa->read_io_8) (KB + KB_STATUS);
	if (!(status & (KBS_PERR | KBS_GTO))) { /* no parity or time out error */
		ddprintf("        no parity or timeout error!\n");
		if ((status & KBS_ODS) && (status & KBS_OBF)) /* mouse data? */
			ms_inth(NULL);
		else
		if (status & KBS_OBF) /* keyboard data? */
			kb_inth(NULL);
		else {
			(*isa->write_io_8) (0x398, 0x05);
			ddprintf("        KRR: %.2x\n", (*isa->read_io_8) (0x399));
			
			if (controller_command(KBC_RDCOMMAND) != B_NO_ERROR) {
				status = (*isa->read_io_8) (KB + KB_STATUS);
				ddprintf("        KBC_RDCOMMAND failed-status = %.2x\n", status);
				return;
			}
			if (get_data(&command, KBS_OBF) != B_NO_ERROR) {
				status = (*isa->read_io_8) (KB + KB_STATUS);
				ddprintf("        get_data for KBC_RDCOMMAND failed-status = %.2x\n", status);
				return;
			}
			ddprintf("        command byte=%.2x\n", command);
			kernel_debugger(NULL);
		}
		return;
	}

	do {
		tmp = (*isa->read_io_8) (KB + KB_STATUS);
		if ((tmp & KBS_ODS) && (tmp & KBS_OBF)) {
			get_data(&command, KBS_ODS);
			ddprintf("        ODS: %x\n", command);
		}
		else if (tmp & KBS_OBF) {
			get_data(&command, KBS_OBF);
			ddprintf("        OBF: %x\n", command);
		}
	} while (tmp & KBS_OBF);
			
	if (controller_command(KBC_RDCOMMAND) != B_NO_ERROR) {
		ddprintf("        KBC_RDCOMMAND failed\n");
		return;
	}
	if (get_data(&command, KBS_OBF) != B_NO_ERROR) {
		ddprintf("        get_data for KBC_RDCOMMAND failed\n");
		return;
	}
	ddprintf("        command byte=%.2x\n", command);

	if ((status & KBS_ODS) && (status & KBS_OBF)) { /* mouse */
		do {
			controller_command_with_data(KBC_WRMOUSE, MS_RESEND);
			if (get_data(&tmp, KBS_OBF) != B_NO_ERROR) {
				ddprintf("        resend command to mouse failed\n");
				return;
			}
			if (tmp == KBC_RESEND)
				ddprintf("        resend resend command to mouse\n");
			if (tmp == KBC_ACK) {
				ddprintf("        resend command to mouse OK\n");
				return;
			}
		} while (tmp == KBC_RESEND);
	}
	else
	if (status & KBS_OBF) { /* keyboard */
		if (keyboard_command(KB_RESEND) == B_NO_ERROR)
			ddprintf("        resend command to keyboard OK\n");
		else
			ddprintf("        resend command to keyboard failed\n");
	}
}

#endif

/* ----------
	driver-related structures
----- */

static device_hooks kbm_device = {
	kb_mouse_open,		/* -> open entry point */
	kb_mouse_close,		/* -> close entry point */
	kb_mouse_free,		/* -> free cookie entry point */
	kb_mouse_control,	/* -> control entry point */
	kb_mouse_read,		/* -> read entry point */
	kb_mouse_write,		/* -> write entry point */
	NULL,				/* -> select entry point */
	NULL				/* -> deselect entry point */
};


static char *kb_mouse_devices[4];

const char **
publish_devices()
{
	int curDevice = 0;

ddprintf("KB/MOUSE: Publishing Devices\n");	

	if (found_keyboard)					
		kb_mouse_devices[curDevice++] = kATKeyboardDeviceName;
	
//	if (!(ps2_mouse_found == 2))		/*GTO errors indicate BIOS won't handle hot plug properly */
 
 	kb_mouse_devices[curDevice++] = kPS2MouseDeviceName; /*lie in order to enable hotplugging */

	kb_mouse_devices[curDevice] = NULL;

	return (const char **) kb_mouse_devices;
}

device_hooks *
find_device(const char *name)
{
	return &kbm_device;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;
