/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : Lib_Local.h    1.74
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Basic definitions  
 *
 *  Last update              : 13:40:25 - 97/07/15
 *
 *  Description              : 
 */

#ifndef Lib_Local_INCLUDED
#define Lib_Local_INCLUDED


/*--------------------------------- Includes: ------------------------------*/


#include "tmtypes.h"

typedef void   (*Lib_Local_EltFunc)        ( Pointer );
typedef void   (*Lib_Local_PairFunc)       ( Pointer, Pointer  );
typedef void   (*Lib_Local_TripleFunc)     ( Pointer, Pointer, Pointer );


typedef Int  (*Lib_Local_EltResFunc)     ( Pointer );
typedef Int  (*Lib_Local_PairResFunc)    ( Pointer, Pointer );
typedef Int  (*Lib_Local_TripleResFunc)  ( Pointer, Pointer, Pointer );


typedef Bool  (*Lib_Local_BoolResFunc)     ( Pointer, Pointer );


/*
 * Standard macros
 */

#define LMAX(a, b)		((a) > (b) ? (a) : (b))
#define LMIN(a, b)		((a) < (b) ? (a) : (b))
#define LMAX3(a, b, c)		((a) > (b) ? LMAX(a,c) : LMAX(b,c))
#define LMIN3(a, b, c)		((a) < (b) ? LMIN(a,c) : LMIN(b,c))
#define LMAX4(a, b, c, d)	(LMAX(LMAX(a,b),LMAX(c,d)))
#define LMIN4(a, b, c, d)	(LMIN(LMIN(a,b),LMIN(c,d)))
#define LEQSTR(s, t)		(strcmp((s), (t)) == 0)
#define LABS(a)			((a) > 0 ? (a) : -(a))
#define LODD(x)         	 ((x)&1)
#define LEVEN(x)       		(!LODD(x))
#define LSWAP(x,y,T)    	 { T h= x; x=y; y= h; }
#define LIS_POW_2(x)		(((x) & -(x)) == (x))
#define LEQUIV(b1,b2)   	( ((b1)==False) == ((b2)==False) )
#define LIMPLIES(b1,b2)         ( (!(b1)) || (b2) )
#define LROUND_UP(v,bs)         ((((((UInt)v)) + ((UInt)(bs))-1 ) \
                                     / ((UInt)(bs))) * ((UInt)(bs)))
#define LROUND_DOWN(v,bs)       ((((((UInt)v))) \
                                     / ((UInt)(bs))) * ((UInt)(bs)))

#define LBOOLPAIR(a,b)          ( 2*(a?1:0) + (b?1:0) )

#define LOFFSET_OF(type, member)  ((size_t)(&((type *)0)->member))

#define LIENDIAN_SWAP2(x) \
           (  ( ( ((UInt)(x)) & 0x000000ff ) <<  8 ) \
           |  ( ( ((UInt)(x)) & 0x0000ff00 ) >>  8 ) \
           )

#define LIENDIAN_SWAP4(x) \
           (  ( ( ((UInt)(x)) & 0x000000ff ) << 24 ) \
           |  ( ( ((UInt)(x)) & 0x0000ff00 ) <<  8 ) \
           |  ( ( ((UInt)(x)) & 0x00ff0000 ) >>  8 ) \
           |  ( ( ((UInt)(x)) & 0xff000000 ) >> 24 ) \
           )


#define LAENDIAN_SWAP2(x) ((Pointer)LIENDIAN_SWAP2(x))

#define LAENDIAN_SWAP4(x) ((Pointer)LIENDIAN_SWAP4(x))

#define	Nelts(vec)	(sizeof(vec) / sizeof((vec)[0]))

/*
 * Round Int to next multiple of the 'page size'
 * PRECONDITION: page size is a power of 2
 */

#define LIALIGN(size) ( (((Int)(size))+(MAX_ALIGNMENT-1))  \
                                & (~(MAX_ALIGNMENT-1))         \
                                )    

#define LAALIGN(size) ((Pointer)LIALIGN(size))    



/*
 * Copy string into allocated memory          
 */

#define COPY_STRING(string) \
           ( (string) != NULL                                     \
           ? strcpy( Lib_Mem_MALLOC(strlen(string)+1), (string) ) \
           : NULL                                                 \
           )


/* 
 * Current message reporter. It is 
 * required for the correct working of
 * the libraries that REPORTing any fatal
 * results in raising the error value
 * as an exception using the Lib_Exception
 * interface:
 */
#define REPORT Lib_Msg_report
#include "Lib_Messages.h"


/*
 * Checking macro. This macro interfaces to
 * the message reporter, has as argument a condition
 * and a parameter list for the current report routine. 
 * It unconditionally checks for the condition
 * to be True and reports the message when this
 * is not the case.
 * Depending the message mentioned in the parmlist, 
 * failing checks are fatal or not.
 */

#define CHECK(condition, parmlist) \
		if (!(condition)) {		                            \
			REPORT parmlist;	                            \
		} else

/*
 * Assertion macro. This macro is similar to the previous
 * one, with the exception that it can be removed from the
 * final application by defining the NDEBUG macro.
 */

#ifdef assert
#undef assert
#endif

#ifdef NDEBUG

#define ASSERT(condition, parmlist) 
#define assert(c) 

#else

#define ASSERT(condition, parmlist) \
	if (!(condition)) {		                            	\
	    REPORT(LibMsg_Assertion_Failed, __FILE__, __LINE__);	\
	    REPORT parmlist;	                            		\
	}

#define assert(condition) \
	if (!(condition)) {		                            	     \
	    REPORT(LibMsg_assertion_failed, #condition, __FILE__, __LINE__); \
	}


#endif   // DL NDEBUG




/*----------------------------- Type definitions: --------------------------*/



//#include <psos.h>
//#include <prepc.h>
#include <stdio.h>
#include <stdarg.h>

//#include <stdlib.h>
//#include <math.h>
//#include <string.h>
#include <setjmp.h>

//#include <fcntl.h>

/*
#ifndef UNDER_CE
  #include <signal.h>
  #include <errno.h>
  #include <time.h>
#endif // UNDER_CE
*/

/* ======================== */
#if defined (__TCS__)
/* ======================== */

#if	   !defined(__cplusplus)
#include    <tm1/mmio.h>
#include    <ops/custom_ops.h>
#include    <ops/custom_defs.h>
#endif	/* !defined(__cplusplus) */

#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


custom_op void iclr(void);


/* ======================== */
#elif defined (_WIN32) && !defined (_MW_IDE_WIN)   // DL: __TCS__
/* ======================== */

#ifdef UNDER_CE
#define close   fclose
#define read    fread
#define write   fwrite
#define lseek   fseek

#else        //DL: UNDER_CE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <io.h>


#define stat	_stat
#define open    _open
#define close   _close
#define read    _read
#define write   _write
#define lseek   _lseek



extern int   close ( int fd );
extern int   read  ( int fd, void * buffer, unsigned int size);
extern int   write ( int fd, const void * buffer, unsigned int size);
extern long  lseek ( int fd, long offset, int whence);

#endif // UNDER_CE

/* ======================== */
#elif defined (__MWERKS__)      // DL : __TCS__
/* ======================== */

#ifndef __MRC__
    #include <unistd.h>
#endif      // DL: __MRC__

#include <types.h>
#include <stat.h>

/* ======================== */
#elif defined (_HPUX_SOURCE)      // DL: __TCS__
/* ======================== */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
extern int close ( );
extern int read  ( );
extern int write ( );

/* ======================== */
#elif defined (sparc)              // DL: __TCS__
/* ======================== */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <unistd.h>


/* ======================== */
#elif defined (__DCC__)          // DL: __TCS__
/* ======================== */

#define close   fclose
#define read    fread
#define write   fwrite
#define lseek   fseek



/* ======================== */
#else                               // DL: __TCS__
/* ======================== */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

// DL: This is used in our case:
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>



#endif



/* ======================== */
#if defined (__TCS__)
/* ======================== */
#ifdef __BIG_ENDIAN__
#define LCURRENT_ENDIAN BigEndian
#else
#define LCURRENT_ENDIAN LittleEndian
#endif

#define MAX_ALIGNMENT 4
#define inline

/* ======================== */
#elif defined (_WIN32)                    // DL: __TCS__ (2)
/* ======================== */

#define LCURRENT_ENDIAN LittleEndian
#define MAX_ALIGNMENT 4
#define restrict


/* ======================== */
#elif defined (__MWERKS__)               // DL: __TCS__ (2)
/* ======================== */

#define LCURRENT_ENDIAN BigEndian
#define MAX_ALIGNMENT 4
#define restrict


/* ======================== */
#elif defined (sparc)                  // DL: __TCS__ (2)
/* ======================== */

#define LCURRENT_ENDIAN BigEndian
#define MAX_ALIGNMENT 4
#define restrict


/* ======================== */
#elif defined (_HPUX_SOURCE)             // DL: __TCS__ (2)
/* ======================== */

#define LCURRENT_ENDIAN BigEndian
#define MAX_ALIGNMENT 8
#define restrict

/* ======================== */
#elif defined (__DCC__)                    // DL: __TCS__ (2)
/* ======================== */
#define LCURRENT_ENDIAN BigEndian
#define MAX_ALIGNMENT 4
#define restrict

/* ======================== */
#else                                     // DL: __TCS__ (2)
/* ======================== */

// DL:?? #define LCURRENT_ENDIAN BigEndian
#define LCURRENT_ENDIAN LittleEndian
#define MAX_ALIGNMENT 4
#define restrict


#endif                            // DL: __TCS__ (2)


#ifndef O_BINARY
#define O_BINARY 0
#endif







/* 
 * Redirections for memory management calls;
 * consistent use of this allows an easy plug-in
 * of a custom memory manager with special
 * handling when the blocksizes are known:
 */

#if 0

#include "Lib_Memspace.h"

#define DEF_SPACE		   Lib_Memspace_default_space

#define Lib_Mem_FREE(p)            Lib_Memspace_free((Pointer)(p))
#define Lib_Mem_CFREE(p)           Lib_Memspace_free((Pointer)(p))
#define Lib_Mem_FREE_SIZE(p,s)     Lib_Mem_FREE(p)
#define Lib_Mem_FAST_FREE(p)       Lib_Mem_FREE(p)
#define Lib_Mem_MALLOC(s)          ((Pointer)Lib_Memspace_malloc(DEF_SPACE, s))
#define Lib_Mem_NEW(p)             p= Lib_Mem_MALLOC(sizeof(*p))
#define Lib_Mem_REALLOC(p,s)       Lib_Memspace_realloc(p, s)
#define Lib_Mem_CALLOC(n,s)        Lib_Memspace_calloc(DEF_SPACE, n, s)
#define Lib_Mem_address_hash(x)    _Lib_StdFuncs_int_hash((Int)(x))

/*
#define Lib_Mem_address_hash(x)    Lib_Memspace_address_hash(x)
*/

#else            // DL: 0

#define Lib_Mem_FREE(p)            free((Pointer)(p))
#define Lib_Mem_CFREE(p)           free((Pointer)(p))
#define Lib_Mem_FREE_SIZE(p,s)     Lib_Mem_FREE(p)
#define Lib_Mem_FAST_FREE(p)       Lib_Mem_FREE(p)
#define Lib_Mem_NEW(p)             p= (Pointer)malloc(sizeof(*p))
#define Lib_Mem_MALLOC(s)          ((Pointer)malloc(s))
#define Lib_Mem_REALLOC(p,s)       realloc(p, s)
#define Lib_Mem_CALLOC(n,s)        calloc(n, s)
#define Lib_Mem_address_hash(x)    _Lib_StdFuncs_int_hash((Int)(x))

#endif         // DL: 0


/*
 * Function versions of MALLOC and FREE:
 */

extern Address
Lib_Mem_MALLOC_fun( Int size );

extern void
Lib_Mem_FREE_fun( Address address );




#define Lib_Mem_LIST_FAST_FREE(p) \
    {				\
	Pointer *_z= (Pointer*)(p);	\
	while (_z) {			\
	    Pointer _h=*_z;		\
	    Lib_Mem_FREE((Pointer)_z);	\
	    _z=_h;			\
	}				\
    }

#define Lib_Mem_LIST_NEW(p,s)  \
    {					\
	Pointer _result= NULL;		\
	Int  _i= s;			\
					\
	while (_i--) {			\
	    Pointer *_z= (Pointer*)Lib_Mem_MALLOC(sizeof(*p));\
	    *_z = _result;		\
	    _result= (Pointer)_z;	\
	}				\
				 	\
	p= _result;			\
    }




/*
 * Function         : Greatest Common Denominator
 *                    Least Common Multiple
 * Parameters       : a,b  (I) arguments
 * Function result  : (sic)
 */
UInt GCD( UInt a, UInt b );
UInt LCM( UInt a, UInt b );



#endif /* Lib_Local_INCLUDED */
