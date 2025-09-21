/************************************************************************/
/*                                                                      */
/*                              joy.h	                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _JOY_H
#define _JOY_H

#include "joystick_driver.h"


typedef struct _joy_dev {
	struct _sound_card_t *card;
	void *		driver;
//	char		name1[64];
	void *		cookie;
	int32		count;
} joy_dev;

extern generic_gameport_module * gameport;

#endif
