#define LUCENT_DRIVER_NAME "ltmodem"
#define LUCENT_DEVICE_NAME_STUB "modem/ltmodem/"
#define PCTEL_DRIVER_NAME "pctel"
#define PCTEL_DEVICE_NAME_STUB "modem/pctel/"

#ifdef LUCENT_MODEM
#define DRIVER_NAME LUCENT_DRIVER_NAME
#define DEVICE_NAME_STUB LUCENT_DEVICE_NAME_STUB
#else
#define DRIVER_NAME PCTEL_DRIVER_NAME
#define DEVICE_NAME_STUB PCTEL_DEVICE_NAME_STUB
#endif


/* any bus, any device, any func */
#define ANY_BUS 			-1	

/* pci identifcation these need to be changed for every card */
/* used poke program to find the ids, they are for a ActionTec modem */
#define LUCENT_VENDOR_ID 0x11c1
#define LUCENT_DEVICE_ID 0x0440 /* device ID could be 440 through 44f */
#define LUCENT_DEVICE_ID_MASK 0xfff0

#define PCTEL_VENDOR_ID 0x134d
#define PCTEL_DEVICE_ID 0x7890

#define CMEDIA_VENDOR_ID 0x13f6
#define CMEDIA_DEVICE_ID 0x0211

#define DSP_TIMER_RESOLUTION 10000 /* 10000 us = 10 ms */

typedef unsigned char byte ;
typedef unsigned short word ;
typedef unsigned long dword ;

struct i387_hard_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
	long	status;		/* software status information */
};

struct i387_soft_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
	unsigned char	ftop, changed, lookahead, no_update, rm, alimit;
	struct info	*info;
	unsigned long	entry_eip;
};

union i387_union {
	struct i387_hard_struct hard;
	struct i387_soft_struct soft;
};
