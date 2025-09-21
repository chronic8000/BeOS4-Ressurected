#ifndef DDALAPI_H
#define DDALAPI_H

#include "TsDdalAPI.hpp"

#define TX_DATA_READY 	0000001
#define RX_DATA_FREE 	0000002
#define MCR_CHANGE 	    0000004
#define PURGE_TX_BUFFER	0000010
#define PURGE_RX_BUFFER	0000020
#define MODEM_READY	    0000040

class TsDdal_BeOS : public TsDdalAPI
{
public:

    TsDdal_BeOS() {  }
    virtual ~TsDdal_BeOS() {  }

    /*****************************************************/
    /* The following services will be provided by the    */
    /* HOST driver to the generic modem core module .    */
    /*****************************************************/
    virtual ddalTxStatus*           GetTxBuffer(ddalDataTransfer *TxBuffer);
    virtual ddalRxStatus*           SetRxBuffer(ddalDataTransfer *RxBuffer);
    virtual void                    SetStatusStructure(ddalStatusStructure *msr);
    virtual ddalControlStructure*   GetControlStructure(void);
}; 

/* Called by serial.c to set event */
extern void           SetGmcEvent(unsigned event);
extern void           StopGmc(int);
extern unsigned short StartGmc(void);
extern int            InitGmcModules(void);
extern void           CleanupGmcModules(void);

#endif
