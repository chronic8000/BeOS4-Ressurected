/*
	Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.
*/

#ifndef _IR_H
#define _IR_H


/*
   These are ioctl() commands you can pass to the IR driver.  Only
   the SEND_ANALYSIS and LOAD_SETTINGS commands need an argument
   (which is a buffer to put/get two integers from).
   
*/   
enum {
	IR_RESET               = B_DEVICE_OP_CODES_END + 1,
	IR_SET_SAMPLE_MODE,
	IR_SET_LISTEN_MODE,
	IR_SET_LOGICAL_LOW,
	IR_START_ANALYSIS,
	IR_STOP_ANALYSIS,
	IR_SEND_ANALYSIS,
	IR_LOAD_SETTINGS
};

#endif  /* _IR_H */

