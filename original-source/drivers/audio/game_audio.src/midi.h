/************************************************************************/
/*                                                                      */
/*                              midi.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _MIDI_H
#define _MIDI_H

#include "midi_driver.h"

typedef struct _midi_dev {
	struct _sound_card_t *card;
	void *		driver;
	void *		cookie;
	int32		count;
} midi_dev;

extern generic_mpu401_module * mpu401;

extern bool midi_interrupt(sound_card_t *);
extern void midi_interrupt_op(int32, void *);


#endif
