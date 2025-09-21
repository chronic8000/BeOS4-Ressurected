
/* gameport.h -- specifics for S3-based PCI audio cards */
/* $Id$ */

#if !defined(_GAMEPORT_H)
#define _GAMEPORT_H

#if !defined(_DRIVERS_H)
	#include <Drivers.h>
#endif /* _DRIVERS_H */
#if !defined(_SUPPORT_DEFS_H)
	#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */
#if !defined(_OS_H)
	#include <OS.h>
#endif	/* _OS_H */


#define GAMEPORT_DEFAULT_ISA_PORT 0x201
#define GAMEPORT_ALTERNATE_ISA_PORT 0x209


#if !defined(_JOYSTICK_DRIVER_H)
	#include <joystick_driver.h>
#endif



#endif /* _GAMEPORT_H */

