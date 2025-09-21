#define DRIVER_NAME "ltmodem"
#define DEVICE_NAME_STUB "modem/ltmodem/"

/* any bus, any device, any func */
#define ANY_BUS 			-1	

/* pci identifcation these need to be changed for every card */
/* used poke program to find the ids, they are for a ActionTec modem */
#define LUCENT_VENDOR_ID 0x11c1
#define LUCENT_DEVICE_ID_LOW 0x0440 /* device ID could be 440 through 44f */
#define LUCENT_DEVICE_ID_HIGH 0x045a /* device ID could be 440 through 44f */

#define XIRCOM_VENDOR_ID 0x115d
#define XIRCOM_DEVICE_ID_LOW 0x0
#define XIRCOM_DEVICE_ID_HIGH 0xf

//#define DSP_TIMER_RESOLUTION 10000 /* 10000 us = 10 ms */
#define DSP_TIMER_RESOLUTION 3330 /* 3330 us = 3.33 ms */

typedef unsigned char byte ;
typedef unsigned short word ;
typedef unsigned long dword ;

extern void modem_sleep( void);
#define VERSION_STR		"5.94"

#ifdef DEBUGGING
#define IDSTRING "V.90 Data+Fax Modem Version " VERSION_STR " Debug"
#else
#define IDSTRING "V.90 Data+Fax Modem Version " VERSION_STR
#endif
