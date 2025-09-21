#ifndef DDALAPI_H
#define DDALAPI_H

#define TX_DATA_READY 	0000001
#define RX_DATA_FREE 	0000002
#define MCR_CHANGE 	    0000004
#define PURGE_TX_BUFFER	0000010
#define PURGE_RX_BUFFER	0000020
#define MODEM_READY	    0000040

/* Called by serial.c to set event */
extern void           SetGmcEvent(unsigned event);
extern void           StopGmc(int);
extern unsigned short StartGmc(void);
extern int            InitGmcModules(void);
extern void           CleanupGmcModules(void);

#endif
