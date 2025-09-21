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

/*	Customization for BeOS. Copyright (c) Be Inc. 2000	*/

#include "flbase.h"
#include <stdlib.h>
#include <lock.h>



/* Wait for specified number of milliseconds */
void flDelayMsecs(unsigned milliseconds)
{
	snooze(1000*milliseconds);
}

void flsleep(unsigned long msec)
{
	snooze(1000*msec);
}

#if POLLING_INTERVAL > 0

/* ATTENTION: this timer code depends on "C" calling conventions for function pointer casts. */
static void (*interval_routine)(void);

/* Install an interval timer */
FLStatus flInstallTimer(void (*routine)(void), unsigned int intervalMsec)
{
	if(intervalMsec < 100)
		intervalMsec = 100;
	
	if(interval_routine != NULL)      /* timer already active, let's kill it */
	{
		if(unregister_kernel_daemon((void (*)(void *, int))interval_routine, NULL) != B_OK)
			return flGeneralFailure;   /* the old timer refuses to die */
		interval_routine = NULL;
	}
	
	if(B_OK == register_kernel_daemon((void (*)(void *, int)) routine, NULL, intervalMsec/100))
	{
		interval_routine = routine;
		return flOK;
	}
	else
		return flGeneralFailure; 
}


#ifdef EXIT

/* Remove an interval timer */
void flRemoveTimer(void)
{
	if(interval_routine)
	{

	unregister_kernel_daemon((void (*)(void *, int))interval_routine, NULL);
	
    (*interval_routine)();	/* Call it twice to shut down everything */
    (*interval_routine)();
    interval_routine = NULL;
  }
}

#endif	/* EXIT */

#endif	/* POLLING_INTERVAL */

#if 0
/* Return current DOS time */
unsigned flCurrentTime(void)
{
/*
  struct time t;

  gettime(&t);

  return (t.ti_hour << 11) | (t.ti_min << 5) | t.ti_sec;
*/
	return 0;
}


/* Return current DOS date */
unsigned flCurrentDate(void)
{
/*
  struct date d;

  getdate(&d);

  return ((d.da_year - 1980) << 9) | (d.da_mon << 5) | d.da_day;
*/
	return 0;
}
#endif


void flSysfunInit(void)
{
	srand((unsigned)system_time());
}


/* Return a random number from 0 to 255 */
unsigned flRandByte(void)
{
	return (rand() >> 8) & 0xFF;	/* The LSB byte is less random */
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
	return new_lock(mutex, "DOCHmtx");
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
	free_lock(mutex);
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
	LOCK((*mutex));
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
  UNLOCK((*mutex));
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
#if 0	/* Nobody uses it. */
void  flSwapBytes (char FAR1 *buf,
                   int        len)
{
  register int   i;
  register char  tmp;

  len &= ~1;     /* even number of bytes */

  for (i = 0;  i < len;  i += 2)
    { tmp = buf[i];   buf[i] = buf[i+1];  buf[i+1] = tmp;  }
}
#endif	

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

#if 0	/* implemented as the macro in flsystem.h */
void FAR0*  flAddLongToFarPointer(void FAR0 *ptr, unsigned long offset)
{
  return (void*) (((uint8*)ptr) + offset);
}
#endif



void* my_memcpy(void* dest, const void* src, size_t n)
{
	char* 		d = dest;
	const char* s = src;
	
	for(; n>0; --n)
		*d++ = *s++;
	return dest;
}


void* my_memset(void* dest, int c,  size_t n)
{
	char* 				d;
	const unsigned char uc = c;

	for(d = dest; n>0; --n)
		*d++= uc;
	return dest;
}


int my_memcmp(const void* src1, const void* src2, size_t n)
{
	const unsigned char *s1, *s2;
	
	for(s1=src1, s2=src2; n>0; ++s1, ++s2, --n)
		if(*s1 != *s2)
			return ((*s1 < *s2) ? -1 : 1);
	return 0;

}


