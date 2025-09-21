
/* gameport.h -- specifics for S3-based PCI audio cards */
/* $Id$ */

#if !defined(_SPEAKER_H)
#define _SPEAKER_H

#if !defined(_DRIVERS_H)
	#include <Drivers.h>
#endif /* _DRIVERS_H */
#if !defined(_SUPPORT_DEFS_H)
	#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */
#if !defined(_OS_H)
	#include <OS.h>
#endif	/* _OS_H */


enum {
	/* arg -> pointer to long long */
	SPEAKER_SET_BYTE_RATE = B_DEVICE_OP_CODES_END + 1
};

#define SPEAKER_ISA_PORT 0x61


#endif /* _SPEAKER_H */

