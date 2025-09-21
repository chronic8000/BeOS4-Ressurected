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
 *  Module name              : tmprof.h    0.0
 *
 *  Module type              : INTERFACE
 *
 *  Title                    : header file profiler library
 *
 *  Author                   : Ciaran O'Donnell
 *
 *  Last update              : 98/5/19
 *
 *  Reviewed                 : NONE
 *
 *  Revision history         : NONE
 *
 *  Description              :  
 *         API of profiler library expoted to user. This goes in
 *         <tmlib/tmprof.h>
 */

#ifndef _TMprof_h
#define	_TMprof_h



/*
 * header and log information is in little endian format
 */

#define	_cand(x) ((x) << 24 | ((x) & 0xFF00) << 8 | (((x) >> 8) & 0xFF00) | (x) >> 24)

static long int long_one = 1;         /* used to determine host's endianness */
#define is_host_le()    (*(char *)&long_one != 0)
#define canon(x)        (is_host_le() ? (x) : _cand(x))




/*
 * profileHdr - trace file header
 *
 * The profiler library writes files in trace file format ("mon.out" format). 
 * The output is binary and is a concatenation of trace logs from
 * consecutive profile runs.
 *
 * Each log has a header (128 bytes) followed by the trace data
 * (header.nevt * 8 bytes) 
 */
#define PROFILE_HDRSIZ	sizeof(struct profileHdr)
#define	PROFILE_ARGSIZ	64

struct profileHdr
{
  char	version[16];		/* version string (compatibility only) */
  char	class;			/* trace class (see below) */
  char	opt;			/* profiling options (see below)
  char	_fill1[2];		/* filler */
  unsigned long	nevt;		/* length of trace buffer (x8 bytes) */
  unsigned long start[2];	/* start time (cycles) */
  unsigned long stop[2];	/* end time (cycles)  */
  unsigned long time[2];	/* start and endi time (seconds, UNIX format) */
  unsigned long base;		/* base address of text */
  unsigned long magic;		/* address of exit() (magic number) */
  unsigned long nbytes; 	/* length of in memory trace buffer (bytes) */
  char _fill2[4];  	         /* reserved for expansion */
  char args[PROFILE_ARGSIZ];	/* 64..128 */
};

/* coding for Hdr.class */

#define	TRACE_MM	1	/* instruction cache / data cache cycles */
#define	TRACE_NOMM	0	/* decision tree executions / instruction cycles */

/* coding for Hdr.opt */

#define	OPT_LE		1	/* see above */	
#define	OPT_DBG		2	/* debugging printf */

/*
 * profileEvt - trace log entry structure
 *     Each log entry corresponds to a distinct spc.
 */
struct profileEvt
{
  unsigned char	tag;			/* event type (see below) */
  unsigned char	spc[3];			/* spc for this entry, LSB first **/
  unsigned int	data;			/* number of event occurrences, LSB first) */
} ;

#define profileEvtSz(hdr)	((hdr)->nevt*sizeof(struct profileEvt))


		/* tags */

#define	TAG_DTXECS	0		/* decision tree executions */
#define	TAG_INSTC	1		/* instruction cycles */
#define	TAG_CACHEC	2		/* total of I- and D- cache cycles */
#define	TAG_CACHECN	3	 	/* (not used) */
#define TAG_ICACHEC	4		/* I-cache cycles (subtract from EVS_CACHEC) */
#define	NTAG		5		/* total number of events */


/*
 * profileInit - initialize hardware and buffer pointers for profiling
 * 
 * hdr :  control buffer (set up by profileArgs)
 *     .class     information to be measured (cache, instruction cycles)
 *     .nbytes    size of profile buffer (bytes)
 *
 * result : 0 if OK, otherwise error code
 */

int profileInit(struct profileHdr *hdr, int *buf) ;

/*
 * profileStart
 * Starts profiling. an exception is taken for every decision tree
 * HW is actived to count cache events
 *
 *   hdr : pointer to control block
 */

void profileStart(struct profileHdr *hdr) ;

/*
 * profileStop
 * Stops profiling (TFE exception)
 * Stops HW timer
 *
 *   hdr : pointer to control block
 */
void  profileStop(struct profileHdr *hdr) ;

/*
 * profileFlush
 *  Convert buffer from internal -> external format for use by tmprof
 *   Internal format is a hash table. 16 bytes/entry. 1 entry/decision tree,
 *   identified by SPC (SORCE PROGRAM COUNTER)). 3 words of data/entry (
 *   #instruction cycles, #cache events)
 *   
 *   External format is a linear table. Entries are tagged and identified
 *   by spc. The tag corresponds to the data type (same as above). 8 bytes/entry.
 *   One word of data per entry, corresponds to the tag
 *  
 * Must be called after profileStop()
 * Can be used by standalone applications to extract profile data for tmprof
 *
 * hdr : control buffer pointer
 * buf : trace buffer
 */ 

void profileFlush(struct profileHdr *hdr, int *buf) ;


/*
 * profileDtrees
 *  Calculate the size of the trace buffer. There needs to be at least one
 *  entry per decision tree plus space for hashing.
 * Number of decision trees is calculated as the ratio of the size of the
 * text segment divided by the estimated decision tree size (PROFILE_DTREESIZ)
 *
 *   result: number of bytes necessary for the profile buffer (estimate)
 * NB: this does not include locked text
 */

int profileDtrees() ;

#define	PROFILE_DTREESIZ	100		/* estimated from MPEG 1 */

/*
 * profileArgs - initialize profiling options
 *
 *  hdr  :   control buffer
 *  argc :   argument count
 *  argv : vector of command line arguments
 *
 * Profiling specific options are removed from the arguments
 * The new argument count is returned
 */

int profileArgs(struct profileHdr *hdr, int argc, char **argv) ;

/*
 * profileExit
 * (Internal entry point)
 * write result to file, called from C library
 *
 *  fd : file descriptor
 */

int profileExit(struct profileHdr *hdr, int fd, int *buf) ;


/*
 * profileDbg
 * (Internal entry point)
 * prints a debugging message to stderr
 * Debugging flag in option byte must be set
 *
 * hdr      :  control block pointer
 * format   :  printf style format string
 * (others) :  parameters a la printf
 */
void	profileDbg(struct profileHdr *hdr, char const *format, ...)  ;


/* event categories */


#define PROFILE_NOTIMER		1	/* timer could not be allocated */
#define	PROFILE_NOTPOW2		2	/* trace buffer not a power of 2 */
#define PROFILE_CANTWRITE	3	/* write failed in profileExit */
#define PROFILE_NOMEM		4








#endif
