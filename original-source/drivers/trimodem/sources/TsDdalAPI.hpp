#ifndef _TS_DDAL_API_H
#define _TS_DDAL_API_H

#include "common.h"

/*****************************************/
/* TsDdalAPI::Close() return code */
/*****************************************/
#define CLOSE_DONE          300
#define CLOSE_PENDING       301

union ddalEvent
{ 
    struct 
    {
        UINT32 TxDataReady       :1;
        UINT32 RxDataFree        :1;
        UINT32 McrChange         :1;
        UINT32 TxCommandReady    :1;
        UINT32 RxCommandFree     :1;
        UINT32 PurgeTxBuffer     :1;
        UINT32 PurgeRxBuffer     :1;
        UINT32 ModemReady	     :1;
        UINT32 PowerDown         :1;
        UINT32 PowerUp           :1;
        UINT32 WaitIdleState     :1;
        UINT32 Reserved          :21;
    }Bit;
    UINT32 Dword;
};

struct ddalRxStatus
{
	UINT32 ByteRx; 		// BYTE copy to Buffer
	UINT32 BytePending;	// BYTE pending in Rx buffer
};

struct ddalTxStatus
{
	UINT32 ByteTx; 		// BYTE copy to Buffer
	UINT32 BytePending;	// BYTE pending in Tx buffer
};

struct ddalDataTransfer
{
	BYTE	*Buffer;
	UINT32	Count;
};

struct ddalControlStructure
{
    BYTE   Mcr;           // modem control register 
    BYTE   Lcr;           // Line control register
    UINT16 BaudRate;      // Baud rate
};

struct ddalStatusStructure
{
    BYTE    Msr;           // modem status register 
    BYTE    Lsr;           // Line status register
    //todo : retirer les 2 membres suivants
    BYTE    Reset;         // Reset Parameters
    BYTE    IdleState;     // 1 = Controller is in IDLE state
    UINT16  DeviceRate;    // To communicate actual current rate
};


class TsDdalAPI
{
public:

    TsDdalAPI ()  { /* GNU compiler needs me! */ }
    virtual ~TsDdalAPI ()  { /* GNU compiler needs me! */ }

    /*****************************************************/
    /* The following services will be provided by the    */
    /* HOST driver to the generic modem core module .    */
    /*****************************************************/
    virtual ddalTxStatus*           GetTxBuffer(ddalDataTransfer *TxBuffer) = 0;
    virtual ddalRxStatus*           SetRxBuffer(ddalDataTransfer *RxBuffer) = 0;
    virtual void                    SetStatusStructure(ddalStatusStructure *msr) = 0;
    virtual ddalControlStructure*   GetControlStructure(void) = 0;
}; 
#endif // _TS_DDAL_API_H
