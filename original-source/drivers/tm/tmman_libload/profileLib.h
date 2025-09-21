/*
 *  COPYRIGHT (c) 1996,1997,1998 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : profHW.h    0.0
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : header file for hardware
 *
 *  Author                   : Ciaran O'Donnell
 *
 *  Last update              : 98/5/14
 *
 *  Reviewed                 : NONE
 *
 *  Revision history         : NONE
 *
 *  Description              :  
 *
 *  This file contains hardware header information, specifically regarding
 *  the timers and TFE bit, as well as shared data structures for recording
 *  profile data
 *  
 *  This file is shared between the TFE interrupt service routine (mcount.c)
 *  and the main level profiler library (init_profile.c) 
 */

#define	TFE_BIT		0x04000000		/* trap on first exit bit, PCSW */
#define	CS_BIT		0x00000800		/* count stalls bit, PCSW */
#define	IEN_BIT		0x00000400		/* interrupt enable bit, PCSW */

 
/*
 * structure of a hardware timer
 * see chapter 3 of the databook
 */
struct timer {
  int	tmodulus;				/* wrap around value (period) */
  int	tvalue;					/* current value */
  int	tctl;					/* control register: - TMR_ RUN */ 
}  ;

#define	TMR_RUN	1	  /* databook, chapter 3 - run bit */	
#define	TMR_CACHE1	5 /* databook, chapter 3, table, Timer Source Selections */  
#define CACHE_INSTC	2 /*databook Table 5-11, Trackable Cache Performance Events */



struct profileHW {
  int clock;			
  int csbit;		/* offset 4 assumed in assembly */
  int	*table;
  int	*end;
  struct timer *ptimer;
  struct profileHdr *hdr;	/* do not remove this */
  int mask;
  unsigned long timer;		/* reserved for internal use */
} ;

extern struct profileHW _profileHW;

/*
 * structure of a trace buffer entry
 */

#define BUF_SPC		0	/* SPC at the time of the exception */
#define	BUF_DTXECS	1	/* number of decision tree executions */
#define	BUF_CYCLES	2	/* number of cycles as measured by CCYCLES */
#define	BUF_EVENTS	3	/* number of timer events */
#define	NBUF		4	/* size of a trace buffer entry (words) */
#define	ENTRYSZ		16	/* size of a trace buffer entry (bytes) */

#define	TMR_CACHE1	5 /* databook, chapter 3, table, Timer Source Selections */  
#define CACHE_INSTC	2 /*databook Table 5-11, Trackable Cache Performance Events */



