
/*
 * $Log:   P:/user/amir/lite/vcs/flsystem.c_v  $
 * 
 *    Rev 1.10   03 Nov 1997 16:09:48   danig
 * flAddLongToFarPointer
 * 
 *    Rev 1.9   05 Oct 1997 15:31:08   ANDRY
 * flSwapBytes() added
 *
 *    Rev 1.8   19 Aug 1997 20:04:16   danig
 * Andray's changes
 *
 *    Rev 1.7   24 Jul 1997 18:11:48   amirban
 * Changed to flsystem.c
 *
 *    Rev 1.6   07 Jul 1997 15:21:48   amirban
 * Ver 2.0
 *
 *    Rev 1.5   29 Aug 1996 14:18:04   amirban
 * Less assembler
 *
 *    Rev 1.4   18 Aug 1996 13:48:08   amirban
 * Comments
 *
 *    Rev 1.3   09 Jul 1996 14:37:02   amirban
 * CPU_i386 define
 *
 *    Rev 1.2   16 Jun 1996 14:02:38   amirban
 * Use int 1C instead of int 8
 *
 *    Rev 1.1   09 Jun 1996 18:16:20   amirban
 * Added removeTimer
 *
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

#include "flbase.h"
#include <time.h>
#include <stdlib.h>

#include <dos.h>
#include <conio.h>

#ifdef CPU_i386
  #pragma inline
#endif

#define VEC_NO	0x1c

static unsigned multiplier = 1193*2;

/*--------------------------------------------------------------------------*

Name		readtimer - read the complemented value of timer 0

Description	Obtain the complement of the value in timer 0.	The
		complement is used so that the timer will appear to
		count up rather than down.  The value returned will
		range from 0 to 0xffff.

*---------------------------------------------------------------------------*/

static unsigned readtimer (void)
{
  unsigned ret;

  disable();
  outp(0x43,0);			/* Latch timer 0 */
  __emit__(0xeb,0);		/* Waste some time */
  ret = inp(0x40);	        /* Read counter LSB */
  __emit__(0xeb,0);		/* Waste some time */
  ret += inp(0x40) << 8;	/* Read counter MSB */
  enable();

  return ~ret;				/* Need ascending counter */
}

/*--------------------------------------------------------------------------*

Name		timerInit - initialize multiplier for delay function

Description	Determine the multiplier required to convert milliseconds
		to an equivalent interval timer value.	Interval timer 0
		is normally programmed in mode 3 (square wave), where
		the timer is decremented by two every 840 nanoseconds;
		in this case the multiplier is 2386.  However, some
		programs and device drivers reprogram the timer in mode 2,
		where the timer is decremented by one every 840 ns; in this
		case the multiplier is halved, i.e. 1193.

		When the timer is in mode 3, it will never have an odd value.
		In mode 2, the timer can have both odd and even values.
		Therefore, if we read the timer 100 times and never
		see an odd value, it's a pretty safe assumption that
		it's in mode 3.  This is the method used in timer_init.

*---------------------------------------------------------------------------*/

static void timerInit(void)
{
  int i;

  /* Check that multiplier is correct */
  for (i = 0; i < 100; i++)
    if ((readtimer() & 1) == 0) {	/* readtimer() returns complement */
      multiplier = 1193;
      return;
    }
}


/* Wait for specified number of milliseconds */
void flDelayMsecs(unsigned milliseconds)
{
  unsigned long stop;
  unsigned cur, prev;

  stop = (prev = readtimer()) + (milliseconds * (unsigned long) multiplier);

  while ((cur = readtimer()) < stop) {
    if (cur < prev) {     /* Check for timer wraparound */
      if (stop < 0x10000L)
	break;
      stop -= 0x10000L;
    }
    prev = cur;
  }
}


#if POLLING_INTERVAL > 0

#define MSEC_PER_TICK	55

static interrupt void far (*oldTimerVector)(void) = 0;
static void (*intervalRoutine)(void);
static unsigned interval;
static unsigned leftInInterval;


static interrupt void clockTickInterrupt(void)
{
  (*oldTimerVector)();
  if (leftInInterval <= MSEC_PER_TICK) {
    leftInInterval = interval;
#ifdef CPU_i386
    asm pushad;
#endif
    enable();
    (*intervalRoutine)();
#ifdef CPU_i386
    asm popad;
#endif
  }
  else
    leftInInterval -= MSEC_PER_TICK;
}


/* Install an interval timer */
FLStatus flInstallTimer(void (*routine)(void), unsigned int intervalMsec)
{
  void far * far *timerVector = (void far * far *) MK_FP(0, VEC_NO * 4);

  if (oldTimerVector)		/* timer already active */
    return flOK;

  intervalRoutine = routine;
  leftInInterval = interval = intervalMsec;

  (void far *) oldTimerVector = *timerVector;
  disable();
  *timerVector = MK_FP(_CS,clockTickInterrupt);
  enable();
  return flOK;
}


#ifdef EXIT

/* Remove an interval timer */
void flRemoveTimer(void)
{
  if (oldTimerVector) {
    void far * far *timerVector = (void far * far *) MK_FP(0, VEC_NO * 4);

    disable();
    *timerVector = oldTimerVector;
    oldTimerVector = NULL;
    enable();

    (*intervalRoutine)();	/* Call it twice to shut down everything */
    (*intervalRoutine)();
    intervalRoutine = NULL;
  }
}

#endif	/* EXIT */

#endif	/* POLLING_INTERVAL */


/* Return current DOS time */
unsigned flCurrentTime(void)
{
  struct time t;

  gettime(&t);

  return (t.ti_hour << 11) | (t.ti_min << 5) | t.ti_sec;
}


/* Return current DOS date */
unsigned flCurrentDate(void)
{
  struct date d;

  getdate(&d);

  return ((d.da_year - 1980) << 9) | (d.da_mon << 5) | d.da_day;
}


void flSysfunInit(void)
{
  timerInit();
}


/* Return a random number from 0 to 255 */
unsigned flRandByte(void)
{
  return (readtimer() >> 1) & 0xff;
}


/*----------------------------------------------------------------------*/
/*      	       f l C r e a t e M u t e x			*/
/*									*/
/* Creates or initializes a mutex					*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flCreateMutex(FLMutex *mutex)
{
  *mutex = 0;
  return flOK;
}

/*----------------------------------------------------------------------*/
/*      	       f l D e l e t e M u t e x			*/
/*									*/
/* Deletes a mutex.							*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDeleteMutex(FLMutex *mutex)
{
}

/*----------------------------------------------------------------------*/
/*      	        f l T a k e M u t e x				*/
/*									*/
/* Try to take mutex, if free.						*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/* Returns:                                                             */
/*	int		: TRUE = Mutex taken, FALSE = Mutex not free	*/
/*----------------------------------------------------------------------*/

int flTakeMutex(FLMutex *mutex)
{
  (*mutex)++;
  if (*mutex > 1) {
    (*mutex)--;
    return FALSE;
  }

  return TRUE;
}


/*----------------------------------------------------------------------*/
/*      	          f l F r e e M u t e x				*/
/*									*/
/* Free mutex.								*/
/*									*/
/* Parameters:                                                          */
/*	mutex		: Pointer to mutex				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flFreeMutex(FLMutex *mutex)
{
  (*mutex)--;
}


unsigned char flInportb(unsigned portId)
{
  return inportb(portId);
}


void flOutportb(unsigned portId, unsigned char value)
{
  outportb(portId,value);
}


/*----------------------------------------------------------------------*/
/*              f l S w a p B y t e s                                   */
/*									*/
/* Swap bytes in the buffer.                                            */
/*                                                                      */
/* Parameters:                                                          */
/*      buf             : buffer to swap bytes                          */
/*      len             : buffer size in bytes (must be multiple of 2)  */
/*                                                                      */
/*----------------------------------------------------------------------*/

void  flSwapBytes (char FAR1 *buf,
                   int        len)
{
  register int   i;
  register char  tmp;

  len &= ~1;     /* even number of bytes */

  for (i = 0;  i < len;  i += 2)
    { tmp = buf[i];   buf[i] = buf[i+1];  buf[i+1] = tmp;  }
}

/*----------------------------------------------------------------------*/
/*                 f l A d d L o n g T o F a r P o i n t e r            */
/*									*/
/* Add unsigned long offset to the far pointer                          */
/*									*/
/* Parameters:                                                          */
/*      ptr             : far pointer                                   */
/*      offset          : offset in bytes                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

void FAR0*  flAddLongToFarPointer(void FAR0 *ptr, unsigned long offset)
{
  return physicalToPointer( pointerToPhysical(ptr) + offset, 0,0 );
}


