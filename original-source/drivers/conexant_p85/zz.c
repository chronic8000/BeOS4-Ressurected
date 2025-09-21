#include	<Drivers.h>
#include	<ISA.h>
#include	<PCI.h>
#include	<KernelExport.h>
#include	<driver_settings.h>
#include	<termios.h>
#include	<tty/ttylayer.h>
#include	"zz.h"

#include "ostypedefs.h"
#include "typedefs.h"

//#include "RtMgr_Ex.h"
//#include "OsMemory_Ex.h"
//#include "OsTime_Ex.h"

#include "intfctrl_ex.h"
#include "comctrl_ex.h"
#include "configcodes.h"
#include "configtypes.h"

#define ANY_BUS                         -1    
#define DRIVER_NAME	"Conexant"
#define	NZZ	8
#define	SOFT_MODEM_NO_UART 1

/* register offsets for the 16550A UART
 */
#define	RBR	0
#define	THR	0
#define	DLL	0
#define	DLH	1
#define	IER	1
#define	ISR	1
#define	FCR	2
#define	IIR	2
#define	LCR	3
#define	MCR	4
#define	LSR	5
#define	MSR	6

/* register bits
 */
#define	LSR_DR			(1 << 0)
#define	LSR_THRE		(1 << 5)
#define	LSR_TSRE		(1 << 6)
#define LCR_7BIT		0x02
#define LCR_8BIT		0x03
#define LCR_2STOP		(1 << 2)
#define LCR_PARITY_ENABLE	(1 << 3)
#define LCR_PARITY_EVEN		(1 << 4)
#define LCR_PARITY_MARK		(1 << 5)
#define LCR_BREAK		(1 << 6)
#define	LCR_DLAB		(1 << 7)
#define	IER_RDR			(1 << 0)
#define	IER_THRE		(1 << 1)
#define	IER_LS			(1 << 2)
#define	IER_MS			(1 << 3)
#define MCR_DTR			(1 << 0)
#define MCR_RTS			(1 << 1)
#define MCR_IRQ_ENABLE		(1 << 3)
#define	MSR_DCTS		(1 << 0)
#define	MSR_DDCD		(1 << 3)
#define	MSR_CTS			(1 << 4)
#define	MSR_DSR			(1 << 5)
#define	MSR_RI			(1 << 6)
#define	MSR_DCD			(1 << 7)
#define	FCR_ENABLE		(1 << 0)
#define	FCR_RX_RESET		(1 << 1)
#define	FCR_TX_RESET		(1 << 2)
#define	FCR_RX_TRIGGER_8	0x80

/* interrupt identification
 */
#define have_int( iir)		(not ((iir)&0x01))
#define THRE_int( iir)		(((iir)&0x0F) == 0x02)
#define RBR_int( iir)		(((iir)&0x0F) == 0x04)
#define	timeout_int( iir)	(((iir)&0x0F) == 0x0C)
#define LSR_int( iir)		(((iir)&0x0F) == 0x06)
#define MSR_int( iir)		(((iir)&0x0F) == 0x00)


#define ttoz( p, t)	(& (p) [((uint)(t)-(uint)(p))/sizeof( *(p))])


struct zz {
	struct tty	tty;
	struct ddrover	rover;
	uint		base,
			intr;		
};


static char *devnames[1+NZZ+1] = {
	"config/conexant",
	"ports/conexant",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};
static struct device devices[NZZ] = { 0, 0, 0, 0
};
static isa_module_info	*isa;
static tty_module_info	*tm;
static struct zz	ztab[NZZ];
static struct ddomain	zdd;

static bool	zz_service( struct tty *, struct ddrover *, uint);
static status_t	readconf( off_t, uchar *, size_t *);
static status_t	writeconf( off_t, const uchar *, size_t *);
static void	setmodes( struct zz *);
static bool	debugging( );
void		*malloc( );

#define DWORD UINT32

// Conexant Specific stuff

typedef void ( *COMM_EVENT_HANDLER)(
    HANDLE hPort,
    DWORD dwRefData,
    DWORD dwEventType,
    DWORD dwEventFlag);

typedef void ( *COMM_RW_HANDLER)(PVOID, DWORD, DWORD, DWORD);

typedef struct tagPortData
{
	DWORD		dwLastError,
				dwCommError,
				LossByte,
//				QInSize,
//				QInCount,
//				QInGet,
//				QInPut,
//				QOutSize,
//				QOutCount,
//				QOutGet,
//				QOutPut,
				dwDetectedEvents;
	BYTE		bMSRShadow;
} PortData;
typedef struct tagPortInformation
{
    PortData    Data;           // port data: has to be first
//    _DCB        ComDCB;         // device control block

//    DWORD       Port;           // Base I/O Address
//    DEVNODE     PortDevNode;    // Our dev node
//    char*       MyName;         // name of port

    PI_COM_CTRL_T       pComCtrl;       // Communication Controller interface
	HANDLE              hComCtrl;       // Communication Controller instance
    SYSTEM_INSTANCES_T  SystemInstances;

    HANDLE      hCS;            // Critical section handle
//    COMM_CONTENTION_HANDLER  ContentionHnd;  // Who handles contention?
//    DWORD       ContentionRes;  // resource to contend for
    BYTE*       pMSRShadow;     // addr of MSRegister Shadow
//    DWORD*      pRxTime;
//    DWORD       fMiscFlags;
    DWORD       EvtMask;        // Mask of events to check for
    DWORD*      pEvtDword;      // address of Event flags
    DWORD       dwOldEvents;    // Last reported events
//    DWORD       VCD_Data;
//    BOOL        fJustOpened;

//    COMM_EVENT_HANDLER pfnNotifyHandle;
//    DWORD       NotifyRefData;
    
    COMM_RW_HANDLER    pfnReadNotifyHandle;
    DWORD       ReadNotifyRefData;
    DWORD       RxTrigger;

    COMM_RW_HANDLER    pfnWriteNotifyHandle;
    DWORD       WriteNotifyRefData;
    DWORD       TxTrigger;
	BOOL		PortClosed;
	BOOL		HardwareInit;
	struct ttyfile *tf;

} PORTINFORMATION, *PPORTINFORMATION;

#define pport ((PPORTINFORMATION)hport)

// values for MiscFlags

#define MF_TxQInternal	    0x04    // have we allocated internal TxBuffer ?
#define MF_RxQInternal	    0x08    // have we allocated internal RxBuffer ?
#define MF_TxQSet           0x10    // have we set a transmit queue ?
#define MF_IgnoreCommError  0x40    // ignore pending comm error (PortRead).
#define MF_ClrTimer         0x80    // DisableTimerLogic

// events
#define EV_RXCHAR			(1<<0) // Character receivced
#define EV_TXCHAR			(1<<1) // Character transmitted
#define EV_TXEMPTY			(1<<2) // Transmit Queue Empty
#define EV_CTS				(1<<3) // CTS changed
#define EV_CTSS				(1<<4) // CTS value
#define EV_DSR				(1<<5) // DSR changed
#define EV_DSRS				(1<<6) // DSR value
#define EV_RLSD				(1<<7) // LS changed
#define EV_RLSDS			(1<<8) // LS value
#undef EV_RING // from posix/termios.h
#define EV_RING				(1<<9) // Ring

#define CN_EVENT			0x0100
#define CN_RECEIVE			0x0200

// MSR values
#define MS_CTS_ON			(1<<4)  // bit 4: CTS is set
#define MS_DSR_ON			(1<<5)  // bit 5: DSR is set
#define MS_RING_ON			(1<<6)  // bit 6: RI is set
#define MS_RLSD_ON			(1<<7)  // bit 7: CD is set

// Here the zz driver is identifying these registers in the MSR as interesting.  I have not seen
// how the conexant driver marks these changes (bit 0 is for CTS change and bit 3 is CD has changed)
// It appears ZZ is not concerened with bits 1 and 2 ( 1 DSR change; 2 RI change)
//#define	MSR_DCTS		(1 << 0)
//#define	MSR_DDCD		(1 << 3)


PORTINFORMATION _PortInfo;

typedef struct tagCookie
{
struct ttyfile Tty; // ttyfile must come first
PORTINFORMATION *PortInfo;
} cookieStruct;


void DoEventNotify(void * d, DWORD dwEvtMask);

#define DEBUGGING
#ifdef DEBUGGING
#define dbgprint dprintf
#else
#define dbgprint
#endif

void ReportEvents(PORTINFORMATION *pPort);

bool inittime=TRUE;
bool timeractive=FALSE;
bool freecalled=FALSE;
#define RECEIVE_LOCAL_BUFF_SIZE 16
// end Conexant specific stuff

/* ----------
	device_probe - find a pci device
----- */
static bool
device_probe (pci_info *p, char *bus, char *device, char *function, bool report)
{
	int i;
	int dsp_io_address, uart_io_address;
	int PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId;
	bool found_device=FALSE;

	pci_module_info		*pci;
	status_t	err;
	
	
	err = get_module(B_PCI_MODULE_NAME, (module_info **) &pci);
	if (err)
	{
		dprintf("error getting module: PCI\n");
		return found_device;
	}

	for (i = 0; (*pci->get_nth_pci_info) (i, p) == B_NO_ERROR; i++)
	{
		if ((p->vendor_id == 0x14f1) ||(p->vendor_id == 0x127a))
			if ((p->device_id == 0x1036) ||
			   (p->device_id == 0x1085) ||
			   (p->device_id == 0x1002))
			{
				found_device = TRUE;
				break;
			}
	}
		
	if (found_device)	
		if (*bus == ANY_BUS || (*bus == p->bus &&
							   *device == p->device &&
							   *function == p->function))
		{
			*bus = p->bus;
			*device = p->device;
			*function = p->function;

			{
//				dprintf("************* Conexant Soft Modem Driver *************\n");
//				dprintf("*********** by Eric Bentley (ericb@be.com) ***********\n");
				dprintf("******** Conexant version HCFPCIOCT_10.6.1.001 *******\n");
				dprintf("***** compiled on %s at %s *****\n", __DATE__, __TIME__);
				dprintf("***** compiled with Debugging output ******\n");
			    PciDeviceId = p->device_id;
				PciVendorId = p->vendor_id;
				PciSDeviceId = p->u.h0.subsystem_id;
				PciSVendorId = p->u.h0.subsystem_vendor_id;
//				dprintf(DRIVER_NAME" found device\n                            device details:\n");
//			    dprintf(DRIVER_NAME"  Vid:0x%04x  Did:0x%04x\n", PciVendorId, PciDeviceId);
//				dprintf(DRIVER_NAME" DSP interrupt:0x%0x pin:0x%x\n", p->u.h0.interrupt_line, p->u.h0.interrupt_pin);
//				dprintf(DRIVER_NAME" DSP mem address:0x%04x\n",p->u.h0.base_registers[0]);
			}	
		}	

	put_module(B_PCI_MODULE_NAME);
			
	if (found_device)
	{
//		found_device= p->u.h0.base_registers[1];
	}
	return found_device;
}


BOOL init_conexant_objects(PORTINFORMATION *pPort)
{
	PINTF_CTRL_FUNC_TABLE_T pIntfCtrl;
	PORT_EVENT_HANDLER      EvtHandler;
    PORT_CONFIG             PortConfig;

	memset(pPort,0,sizeof(PORTINFORMATION));

	pPort->PortClosed = FALSE;		
	pPort->pMSRShadow  = &(pPort->Data.bMSRShadow);
	pPort->pEvtDword   = &(pPort->Data.dwDetectedEvents);
		
	*pPort->pEvtDword  = EV_TXEMPTY;
	pPort->dwOldEvents = 0;
				
	pPort->hCS=(HANDLE)OSCriticalSectionCreate();
	OSCriticalSectionAcquire(pPort->hCS);
    // Get Interface Manager
		
	pIntfCtrl = GetInterfaceControllerFuncTable();
    if(NULL == pIntfCtrl)
    {
	    OSCriticalSectionRelease(pPort->hCS);
      	goto nohardware;
    }
	
    pPort->SystemInstances.pIntrfceCntrlFuncTable = pIntfCtrl;
		
    // Create ComCtrl Interface
    pPort->pComCtrl = pIntfCtrl->CreateInterface(BLOCK_COM_CTRL, 0);
    if(NULL == pPort->pComCtrl)
    {
	    OSCriticalSectionRelease(pPort->hCS);
       	goto nohardware;
    }

    if(COMCTRL_VERSION != pPort->pComCtrl->GetInterfaceVersion())
    {
	    OSCriticalSectionRelease(pPort->hCS);
       	goto nohardware;
   	}

   	pPort->hComCtrl = pPort->pComCtrl->Create();

   	if(NULL == pPort->hComCtrl)
   	{
   	    pIntfCtrl->DestroyInterface(pPort->pComCtrl);
	    OSCriticalSectionRelease(pPort->hCS);
       	goto nohardware;
    }
	
    EvtHandler.pfnCallback  = DoEventNotify;
    EvtHandler.pRef         = pPort;

	PortConfig.dwValidFileds    = PC_EVERYTHING;  // Set up everything
	PortConfig.dwDteSpeed       = 38400;
	PortConfig.eParity          = PC_PARITY_NONE;
	PortConfig.eDataBits        = 8;
	PortConfig.fXOutput         = FALSE;
	PortConfig.fXInput          = FALSE;
	PortConfig.fCTS             = FALSE;
	PortConfig.fRTS             = FALSE;
	PortConfig.fError           = FALSE;
	PortConfig.fNull            = FALSE;
		
//		PortConfig.cXonChar         = pPort->ComDCB.XonChar;
//		PortConfig.cXoffChar        = pPort->ComDCB.XoffChar;
//		PortConfig.cErrorChar       = pPort->ComDCB.ErrorChar;
    if( COM_STATUS_SUCCESS != pPort->pComCtrl->Configure( pPort->hComCtrl, 
                              COMCTRL_CONFIG_DEVICE_ID, (PVOID)pPort  /* a generic handle to this instance */      ) ||
			        COM_STATUS_SUCCESS != pPort->pComCtrl->Configure( pPort->hComCtrl, 
		                      COMCTRL_CONFIG_EVENT_HANDLER, (PVOID)&EvtHandler              ) ||
			        COM_STATUS_SUCCESS != pPort->pComCtrl->Init(pPort->hComCtrl)                        ||
			        COM_STATUS_SUCCESS != pPort->pComCtrl->Control( pPort->hComCtrl,
			                  COMCTRL_CONTROL_PORTCONFIG, (PVOID)&PortConfig)   )
	{
	    pPort->pComCtrl->Destroy(pPort->hComCtrl);
	    pIntfCtrl->DestroyInterface(pPort->pComCtrl);
		OSCriticalSectionRelease(pPort->hCS);
	   	goto nohardware;
	}
	
	pPort->HardwareInit = TRUE;
	return(TRUE);
	OSCriticalSectionRelease(pPort->hCS);
	nohardware:
	return(FALSE);
		
}

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */



status_t
init_hardware (void)
{
	bool		f;
	pci_info	p;
	char bus, device, function;
	
	bus = ANY_BUS;
	device =0;
	function = 0;
	f = device_probe (&p, &bus, &device , &function, TRUE);
	if (f)
	{
//		load_driver_symbols("conexant");
	}
	else
		dprintf("Conexant hardware not located\n");
	
	return (f ? B_NO_ERROR : B_ERROR);
}


static status_t
zz_open( const char *n, uint32 f, void **d)
{
	cookieStruct   *tf;
	struct zz	*zp;
	struct ddrover	*r;
	int		i,
			s;

	i = 0;
	until (streq( n, devnames[i]))
		unless (devnames[++i])
		{
			return (B_ERROR);
			}
	unless (i) {
		*d = 0;
		return (B_OK);
	}
	--i;
	s = ENOMEM;
	zp = ztab + i;
/* don't leave this in */
//	if (f==0)
//		f = 0x182;
	if (!_PortInfo.HardwareInit)
			return (B_ERROR);
		
	if (tf = malloc( sizeof *tf)) {
		tf->PortInfo = &_PortInfo;
		*d = tf;
		tf->Tty.tty = &zp->tty;
		tf->Tty.flags = f;
		if (r = (*tm->ddrstart)( 0)) {
			(*tm->ddacquire)( r, &zdd);
			if (!tf->PortInfo->tf)
			{
				tf->PortInfo->tf = (struct ttyfile *)tf;
			}	
			else
			{
				free( tf);
				return(B_BUSY);
			 }

			if (debugging( i))
				s = B_BUSY;
			else
			{
				s = (*tm->ttyopen)( (struct ttyfile*)tf, r, zz_service);
			}
			(*tm->ddrdone)( r);
		}
		unless (s == B_OK)
			free( tf);
	}
	inittime=FALSE;

dprintf ("zz_open() successfull\n");
	return (s);
}


static status_t
zz_close( void *d)
{

	struct ddrover	*r;
	status_t	s;

	dprintf("zz_close()\n");
	unless (d)
	{
		return (B_OK);
	}
	unless (r = (*tm->ddrstart)( 0))
	{
		return (ENOMEM);
	}
	s = (*tm->ttyclose)( d, r);
	(*tm->ddrdone)( r);

	return (s);
}


static status_t
zz_free( void *d)
{
	struct ddrover	*r;
	status_t	s;

	unless (d)
		return (B_OK);
	if (r = (*tm->ddrstart)( 0)) {
		s = (*tm->ttyfree)( d, r);
		(*tm->ddrdone)( r);
	}
	else
		s = ENOMEM;
	
	_PortInfo.tf =0;
	free( d);
	
	dprintf("zz_free() complete\n");
	return (s);
}


static status_t
zz_read( void *d, off_t o, void *buf, size_t *count)
{
	struct ddrover	*r;
	status_t	s;
	
	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	if (d)
		s = (*tm->ttyread)( d, r, buf, count);
	else {
		(*tm->ddacquire)( r, &zdd);
		s = readconf( o, buf, count);
	}
	(*tm->ddrdone)( r);
	return (s);
}

void ReportEvents(PORTINFORMATION *pPort)
{
//	struct tty *tp = d->Tty.tty;
//	struct tty *tp = pPort->tf->tty;

	struct tty *tp;
	struct zz *zp;
   	HANDLE hps;

//   kprintf("\n pPort is 0x%08x\n", pPort);
	if (!pPort->tf)
	{
		return; /* there is no tf setup yet so we can't process anything */
		}
		
	tp = pPort->tf->tty;
	zp = ttoz( ztab, tp);
//kprintf("ReportEvents() - pPort - 0x%08x\n", pPort);
//kprintf("ReportEvents() - pPort->tf = 0x%08x\n",pPort->tf);
   
//kernel_debugger("kd - report events\n");   
//	dbgprint("ReportEvents 0x%x\n", *pPort->pEvtDword);
    
	hps = (HANDLE)OSDisableInterrupts(); // ttyilock is a spinlock
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);

    if( (*pPort->pEvtDword & pPort->EvtMask) != pPort->dwOldEvents)
    {
    	DWORD event = *pPort->pEvtDword & pPort->EvtMask;

//		dbgprint("ReportEvents 0x%x\t", *pPort->pEvtDword);

		if (event & EV_CTS)
		{
			(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWCTS, *pPort->pMSRShadow&MSR_CTS);
		}
		if (event & EV_DSR)
		{
		}
		if (event & EV_RLSD)
		{
			(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWDCD, *pPort->pMSRShadow&MSR_DCD);
		}
		
        pPort->dwOldEvents = *pPort->pEvtDword & pPort->EvtMask;

//        dbgprint("%d PortDriver eventCallback %d\n",OSGetSystemTime(), *pPort->pEvtDword);
    }
    (*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	OSRestoreInterrupts( hps );
}

void ReadFromMC(PORTINFORMATION *pPort)
{
	HANDLE hport = (HANDLE)pPort;
//    DWORD dwInitialSize;
//    DWORD dwLinSize;
    DWORD dwSize = 0;
    char lbuf[RECEIVE_LOCAL_BUFF_SIZE];
    char c;

	struct tty *tp;
	struct zz *zp;
   	HANDLE hps;

//kprintf("\n pPort is 0x%08x\n", pPort);
	tp = pPort->tf->tty;
   	zp = ttoz( ztab, tp);

    OSCriticalSectionAcquire( pport->hCS );
	hps = (HANDLE)OSDisableInterrupts(); // ttyilock is a spinlock
	
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);
	
	while (TRUE)
	{
		dwSize = pport->pComCtrl->Read(pport->hComCtrl, (void*)&lbuf, RECEIVE_LOCAL_BUFF_SIZE);
		if (!dwSize) // no more data
			break;
		
        *pport->pEvtDword |= EV_RXCHAR;	
		c=0;	
		while(dwSize--) // we still have data in local buffer
		{
			(*tm->ttyin)(tp, &zp->rover, lbuf[c++]);
		}
	}

    (*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	
	OSRestoreInterrupts( hps );

    OSCriticalSectionRelease( pport->hCS );
    
}

/* Copies available data from ttybuffer to driver buffer until no bytes left or no room available */
void WriteToMC(PORTINFORMATION *pPort)
{
	struct tty *tp;
	struct zz *zp; 
	HANDLE  hport;
	int c;
	
	HANDLE hps;
	hport = (HANDLE)pPort;
	tp = pPort->tf->tty;
   	zp = ttoz( ztab, tp);

    OSCriticalSectionAcquire( pport->hCS );
	hps = (HANDLE)OSDisableInterrupts(); // ttyilock is a spinlock
	
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);
	
    while(TRUE)
    {
// efb - we could get a number of characters, one at a time from tty, up to the number of chars
// under the watermark and then call ComCtrl->Write only once.    
    
	    c= (*tm->ttyout)(tp, &zp->rover);
		if (c < 0)
		{
			*pport->pEvtDword |= EV_TXEMPTY;
			break; // no data available from tty
		}
//	    dprintf (">%c>",c);
		if(!pport->pComCtrl->Write(pport->hComCtrl, (void*)(&c), 1))
		{
			break; // no room in the ComController buffer
		}
		*pport->pEvtDword |= EV_TXCHAR;

    }

    (*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	
	OSRestoreInterrupts( hps );
    OSCriticalSectionRelease( pport->hCS );
}


static status_t
zz_write( void *d, off_t o, const void *buf, size_t *count)
{
	struct ddrover	*r;
	status_t	s;
	DWORD NumBWrite = *count; /* number of bytes we will transfer */
//dprintf("zz_write()\n");	
	unless (r = (*tm->ddrstart)( 0)) {
		*count = 0;
		return (ENOMEM);
	}
	if (d)
	{
		s = (*tm->ttywrite)( d, r, buf, count);
	}
	else {
		(*tm->ddacquire)( r, &zdd);
		s = writeconf( o, buf, count);
	}
	(*tm->ddrdone)( r);
	/* Now that the data has been copied to the tty layer, start the copying
	  from tty to the modem driver buffer.  If it will not fit completely, the
      event notification routine will send the rest when notified. */
	{
//		HANDLE  hport = (HANDLE)&((cookieStruct*)d)->PortInfo;
	
	    WriteToMC(((cookieStruct *)d)->PortInfo);
	    ReportEvents(((cookieStruct *)d)->PortInfo);
	}
	
	return (s);
}


static status_t
zz_control( void *d, uint32 com, void *buf, size_t len)
{
	struct ddrover	*r;
	status_t	s;
//dprintf("zz_control()\n");
	unless (d)
		return (ENODEV);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttycontrol)( d, r, com, buf, len);
	(*tm->ddrdone)( r);
	return (s);
}

#if 0
static void
put_register_8 (uint port, uint data)
{
dprintf("put_register_8()\n");
//	(*isa->write_io_8)( port, data);
}

static uint
get_register_8 (uint port )
{
dprintf("get_register_8()\n");
	return (0xffff/*(*isa->read_io_8)( port)*/);
}
#endif

void DoEventNotify(void * pPort, DWORD dwEvtMask)
{
	HANDLE hport = (HANDLE)pPort;

//    dbgprint("DoEventNotify 0x%x\t", dwEvtMask);

    if(dwEvtMask & COMCTRL_EVT_RXCHAR) // There is data available for the app on the modem
    {
       	if (pport->tf)
	        ReadFromMC(pPort);
    }

    if( dwEvtMask & COMCTRL_EVT_TXCHAR ||
        dwEvtMask & COMCTRL_EVT_TXEMPTY   )  // There is room for data from the app, on the modem
    {
    	if (pport->tf)
		 /* there must be a tf setup in order to process anything */
			WriteToMC(pPort);
    }

    if(dwEvtMask & COMCTRL_EVT_CTS) // CTS change
    {
//        dbgprint("DoEventNotify CTS %s\n", (dwEvtMask & COMCTRL_EVT_CTSS) ? "on" : "off");
        *pport->pEvtDword |= EV_CTS;
        if(dwEvtMask & COMCTRL_EVT_CTSS)
        {
            *pport->pMSRShadow |= MS_CTS_ON;
            *pport->pEvtDword |= EV_CTSS;
        }
        else
        {
            *pport->pMSRShadow &= ~MS_CTS_ON;
            *pport->pEvtDword &= ~EV_CTSS;
        }
    }

    if(dwEvtMask & COMCTRL_EVT_DSR) // DSR change
    {
        *pport->pEvtDword |= EV_DSR;
        if(dwEvtMask & COMCTRL_EVT_DSRS)
        {
            *pport->pMSRShadow |= MS_DSR_ON;
            *pport->pEvtDword |= EV_DSRS;
        }
        else
        {
            *pport->pMSRShadow &= ~MS_DSR_ON;
            *pport->pEvtDword &= ~EV_DSRS;
        }
    }

    if(dwEvtMask & COMCTRL_EVT_RLSD) // LS change
    {
        *pport->pEvtDword |= EV_RLSD;
        if(dwEvtMask & COMCTRL_EVT_RLSDS)
        {
            *pport->pEvtDword |= EV_RLSDS;
            *pport->pMSRShadow |= MS_RLSD_ON;
        }
        else
        {
            *pport->pMSRShadow &= ~MS_RLSD_ON;
            *pport->pEvtDword &= ~EV_RLSDS;
        }
    }
    
    if(dwEvtMask & COMCTRL_EVT_RING) // My telephone is ringing!
    {
        *pport->pEvtDword |= EV_RING;
        *pport->pMSRShadow |= MS_RING_ON;
    }
    else // if(EV_RingTe) (?)
    {
        *pport->pEvtDword &= ~EV_RING;
        *pport->pMSRShadow &= ~MS_RING_ON;
    }
    ReportEvents(pPort);
}

#if 0
void zz_event_interrupt( void *p, DWORD dwEvtMask)
{

dprintf("zz_event_interrupt()\n");
#ifdef make_this_event_handler
	uint	iir,
			num_empty_chars_space_in_tx_buff,
			i;

	struct tty *tp = p;
	struct zz *zp = ttoz( ztab, tp);
	uint h = B_UNHANDLED_INTERRUPT;
	(*tm->ddrstart)( &zp->rover);
	(*tm->ttyilock)( tp, &zp->rover, TRUE);
	loop {
		iir = get_register_8( zp->base+IIR);

		h = B_HANDLED_INTERRUPT;
//		if (THRE_int( iir)){
if (num_empty_chars_space_in_tx_buff=io_get_dte_tx_count()){
			for (i=0; i<num_empty_chars_space_in_tx_buff; ++i) {
				int c = (*tm->ttyout)( tp, &zp->rover);
				if (c < 0) {
					put_register_8( zp->base+IER, IER_MS|IER_LS|IER_RDR);
					break;
				}
				put_register_8( zp->base+THR, c);
			}
		}
		unless (have_int( iir))
			break;

//else - We can process both send and receive
		if ((RBR_int( iir))
		or (timeout_int( iir))) {
			for (i=0; get_register_8( zp->base+LSR) & LSR_DR; ++i)
				(*tm->ttyin)( tp, &zp->rover, get_register_8( zp->base+RBR));
			/*
			 * defend against bogus hardware
			 */
			unless (i)
				get_register_8( zp->base+RBR);
		}
		else if (LSR_int( iir)) {
			uint lsr = get_register_8( zp->base+LSR);
		}
		else if (MSR_int( iir)) {
			uint msr = get_register_8( zp->base+MSR);
			if (msr & MSR_DDCD)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWDCD, msr&MSR_DCD);
			if (msr & MSR_DCTS)
				(*tm->ttyhwsignal)( tp, &zp->rover, TTYHWCTS, msr&MSR_CTS);
		}
	}
	(*tm->ttyilock)( tp, &zp->rover, FALSE);
	(*tm->ddrdone)( &zp->rover);
	return (B_HANDLED_INTERRUPT);
#endif	
}
#endif

static bool
zz_service( struct tty *tp, struct ddrover *r, uint com)
{

	cpu_status	ps;
	struct zz *zp = ttoz( ztab, tp);
	HANDLE hport = &_PortInfo;
	
//dprintf("zz_service()\n");
	if 	(!(pport->PortClosed))
	switch (com) {
	case TTYENABLE:  // Enable Port
//		dprintf("TTYENABLE\n");
		unless ((zp->base = devices[zp-ztab].base)
		and (zp->intr = devices[zp-ztab].intr))
		{
			return (FALSE);
		}
		setmodes( zp);
		
//		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, get_register_8( zp->base+MSR)&MSR_DCD);
//		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, get_register_8( zp->base+MSR)&MSR_CTS);
		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, *pport->pMSRShadow&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, *pport->pMSRShadow&MSR_CTS);

//		put_register_8( zp->base+MCR, MCR_DTR|MCR_RTS|MCR_IRQ_ENABLE);
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_SETDTR, 0);
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_SETRTS, 0);

//		put_register_8( zp->base+IER, IER_MS|IER_LS|IER_RDR);
//		events: Receive Data, Recive line status, Modem Status : bits 0,2,3
        pport->EvtMask = EV_RXCHAR | EV_RLSD | EV_CTS | EV_DSR | EV_RLSD;
		
		break;
	case TTYDISABLE:
//		dprintf("TTYDISABLE\n");
//		put_register_8( zp->base+IER, 0);
        pport->EvtMask = 0;
//		put_register_8( zp->base+MCR, 0);
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_CLRDTR, 0);
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_CLRRTS, 0);

		break;
	case TTYSETMODES: // set baud/parity/stopbits
//		dprintf("TTYSETMODES\n");
		setmodes( zp);
		break;
	case TTYISTOP:  // Clear RTS
//		dprintf("TTYSTOP\n");
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_CLRRTS, 0);
		break;
	case TTYIRESUME: // Set RTS
//		dprintf("TTYRESUME\n");
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_SETRTS, 0);
		break;
	case TTYOSTART: // Turn on Transmit interrupts
//		dprintf("TTYSTART\n");
//		put_register_8( zp->base+IER, IER_MS|IER_LS|IER_THRE|IER_RDR);
//		events: Receive Data, Recive line status, Transmit Data, Modem Status : bits 0,2,1,3
        pport->EvtMask = EV_RXCHAR | EV_RLSD | EV_CTS | EV_DSR | EV_TXCHAR | EV_RLSD;
		break;
	case TTYOSYNC: //efb
//		dprintf("TTYSYNC\n");
		return(true);
//		return ((get_register_8( zp->base+LSR)&LSR_THRE+LSR_TSRE) == LSR_THRE+LSR_TSRE);
	case TTYSETBREAK: //efb
//		dprintf("TTYSETBREAK\n");
//		put_register_8( zp->base+LCR, get_register_8( zp->base+LCR)|LCR_BREAK);
		break;
	case TTYCLRBREAK: //efb
//		dprintf("TTYCLRBREAK\n");
//		put_register_8( zp->base+LCR, get_register_8( zp->base+LCR)&~LCR_BREAK);
		break;
	case TTYSETDTR: // Set DTR
//		dprintf("TTYSETDTR\n");
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_SETDTR, 0);
		break;
	case TTYCLRDTR: // Clear DTR
//		dprintf("TTYCLRDTR\n");
//		PortPurge(hport, 1);	// Purge Rx
		pport->pComCtrl->Control(pport->hComCtrl, COMCTRL_CONTROL_CLRDTR, 0);
//		PortPurge(hport, 0);	// Purge Tx
		break;
	case TTYGETSIGNALS: // Read DSR and RI from MSR shadow
//		dprintf("TTYGETSIGNALS\n");
		(*tm->ttyhwsignal)( tp, r, TTYHWDCD, *pport->pMSRShadow&MSR_DCD);
		(*tm->ttyhwsignal)( tp, r, TTYHWCTS, *pport->pMSRShadow&MSR_CTS);
		(*tm->ttyhwsignal)( tp, r, TTYHWDSR, *pport->pMSRShadow &MSR_DSR);
		(*tm->ttyhwsignal)( tp, r, TTYHWRI, *pport->pMSRShadow &MSR_RI);
	}
	return (TRUE);
}


static void
setmodes( struct zz *zp)
{
	struct tty *tp = &zp->tty;
	UINT32 speed = tp->t.c_cflag & CBAUD;
	PORT_CONFIG PortConfig = {0};
	HANDLE hport =  &_PortInfo;

	// set DTE speed
	/* Valid speeds for Conexant hardware
	110		300		600		1200	2400
	4800	9600	14400	19200	38400
	56000	57600	115200	128000	256000 */

//	if (speed <= 110)
//		PortConfig.dwDteSpeed = 110;
//	else
/* speeds are defined in termios.h
	1 = 50
	2 = 75
	3 = 110
	4 = 134
	5 = 150
	6 = 200
	7 = 300
	8 = 600
	9 = 1200
	10 = 1800
	11 = 2400
	12 = 4800
	13 = 9600
	14 = 19200
	15 = 38400
	16 = 57600
	17 = 115200
*/
	switch (speed)
	{
		case B0:
		case B50:
		case B75:
		case B110:
			PortConfig.dwDteSpeed = 110;
			break;
		case B134:
		case B150:
		case B200:
		case B300:
			PortConfig.dwDteSpeed = 300;
			break;
		case B600:
			PortConfig.dwDteSpeed = 600;
			break;
		case B1200:
			PortConfig.dwDteSpeed = 1200;
			break;
		case B1800:
		case B2400:
			PortConfig.dwDteSpeed = 2400;
			break;
		case B4800:
			PortConfig.dwDteSpeed = 4800;
			break;
		case B9600:
			PortConfig.dwDteSpeed = 9600;
			break;
		case B19200:
			PortConfig.dwDteSpeed = 19200;
			break;
		case B38400:
			PortConfig.dwDteSpeed = 38400;
			break;
		case B57600:
			PortConfig.dwDteSpeed = 57600;
			break;
		case B115200:
			PortConfig.dwDteSpeed = 115200;
			break;
		case B230400:
		default:
			PortConfig.dwDteSpeed = 128000;
			break;
	}
	
	// set parity	
	if (tp->t.c_cflag & PARENB)
	{
		if (tp->t.c_cflag & PARODD)
		PortConfig.eParity          = PC_PARITY_ODD;
		else
		PortConfig.eParity          = PC_PARITY_EVEN;
	}
	else
		PortConfig.eParity          = PC_PARITY_NONE;

	// set data bits	
	PortConfig.eDataBits = ((tp->t.c_cflag & CS8) ? PC_DATABITS_8 :PC_DATABITS_7);
			
	// set stop bits
	
	// Don't know how to set stop bits with  Conexant	
//		if (tp->t.c_cflag & CSTOPB)
//			// 2 stop bits!

	// set the modes
    PortConfig.dwValidFileds = PC_DTE_SPEED | PC_PARITY | PC_DATA_BITS;
	if (!(COM_STATUS_SUCCESS ==	pport->pComCtrl->Control(
							pport->hComCtrl, COMCTRL_CONTROL_PORTCONFIG, (PVOID)&PortConfig)))
			dbgprint("Error setting modes with pComCtrl->Control\n");
		
}


static bool
debugging( uint i)
{

	if (platform( ) == B_BEBOX_PLATFORM) {
		if (devices[i].base == 0x388)
			return (debug_output_enabled( ));
	}
	else {
		static uint32 base = -1UL;
		if (base == -1UL) {
			void *handle;
			handle = load_driver_settings("kernel");
			base = strtoul(get_driver_parameter(handle, "serial_debug_port",
					"0x3f8", "0x3f8"), NULL, 0);
			unload_driver_settings(handle);
		}
		if (devices[i].base == base)
			return (debug_output_enabled( ));
	}
	return (FALSE);
}


static status_t
readconf( off_t o, uchar *buf, size_t *count)
{

	uint n = 0;
	while ((n < *count)
	and (o+n < sizeof( devices))) {
		buf[n] = ((uchar *)devices)[(uint)o+n];
		++n;
	}
	*count = n;
	return (B_OK);
}


static status_t
writeconf( off_t o, const uchar *buf, size_t *count)
{

	uint n = 0;
	while ((n < *count)
	and (o+n < sizeof( devices))) {
		((uchar *)devices)[(uint)o+n] = buf[n];
		++n;
	}
	*count = n;
	return (B_OK);
}


const char	**
publish_devices( )
{
	static char	*list[1+NZZ+1];
	uint		i;

	uint j = 0;
	list[j++] = devnames[0];
	for (i=0; i<NZZ; ++i)
		if ((devices[i].base)
		and (i < nel( devnames)))
			if ((platform( ) == B_BEBOX_PLATFORM)
			and (i == 4))
				list[j++] = "ports/com3";
			else if ((platform( ) == B_BEBOX_PLATFORM)
			and (i == 5))
				list[j++] = "ports/com4";
			else
				list[j++] = devnames[i+1];
	list[j] = 0;
	return ((const char **)list);
}


device_hooks	*
find_device( const char * dev)
{
	static device_hooks dh = {
		zz_open, zz_close, zz_free, zz_control, zz_read, zz_write
	};

	dev = dev;
	return (&dh);
}


static const char	DEVCONF[]	= "be_lucent_conf";

status_t
init_driver(void)
{
	int		f;
	pci_info	ppci;
	char		bus, device, function;
	int			PciDeviceId, PciVendorId, PciSDeviceId, PciSVendorId;
	port_id		p;
	status_t	s;
	uint		i;
	int32		_;
	status_t	err;
	status_t	status;
	
	err = get_module( B_ISA_MODULE_NAME, (module_info **)&isa);
	if (err)
		goto error2;
	err = get_module( B_TTY_MODULE_NAME, (module_info **)&tm);
	if (err)
		goto error3;

	bus = ANY_BUS;
	device =0;
	function = 0;
	f = device_probe (&ppci, &bus, &device , &function, FALSE);
	if (!f) {
		dprintf("No Conexant hardware detected\n");
		err = B_ERROR;
		goto error4;
	}

	p = find_port( DEVCONF);
	if (p < 0) {
		p = create_port( 1, DEVCONF);
		if (p < B_OK) {
			err = B_ERROR;
			goto error7;
		}
		set_port_owner( p, B_SYSTEM_TEAM);
		{
			static struct device d[] = {
				0, 4, 0, 0
			};
			d[0].base = 1;
			memcpy( devices, d, sizeof d);
		}
	}


	if (port_count( p))
		read_port( p, &_, devices, sizeof devices);
	publish_devices( );
	ddbackground( &zdd);
	for (i=0; i<nel( ztab); ++i)
		(*tm->ttyinit)( &ztab[i].tty, FALSE);
	init_conexant_objects(&_PortInfo);
	dprintf("init_driver() completed\n");
	return B_OK;

error7:
error6:
error5:
error4:
	put_module(B_TTY_MODULE_NAME);
error3:
	put_module(B_ISA_MODULE_NAME);
error2:
error1:
	dprintf("Leaving init_driver() with error\n");
	return err;
}


void
uninit_driver(void)
{
	port_id		p;
	status_t	status;

	p = find_port( DEVCONF);
	unless (p < 0)
		write_port( p, 0, devices, sizeof devices);

	put_module( B_TTY_MODULE_NAME);
	put_module( B_ISA_MODULE_NAME);
	{
	    PPORTINFORMATION pPort = &_PortInfo;
	    pPort->pComCtrl->Close(pPort->hComCtrl);
	    pPort->pComCtrl->Destroy(pPort->hComCtrl);
	    pPort->SystemInstances.pIntrfceCntrlFuncTable->DestroyInterface(pPort->pComCtrl);
		pPort->PortClosed = TRUE;		
	    OSCriticalSectionDestroy(pPort->hCS);
  	}
  	
/* HACK HACK HACK alert!!!*/
/* Conexant code does not always free the timer!*/
	freecalled=TRUE;
	while (timeractive)
		snooze(10000);
	snooze(10000);
/* end of hack alert */

	dprintf("uninit driver()\n");
}

int32	api_version	= B_CUR_DRIVER_API_VERSION;
#if 0
void print_ecode (CFGMGR_CODE eCode)
{
	switch (eCode)
	{
	    case CFGMGR_MS_PARAMS:
	    	dprintf ("ecode = CFGMGR_MS_PARAMS\n");
	    	break;
	    case CFGMGR_EC_PARAMS:
	    	dprintf ("ecode = CFGMGR_EC_PARAMS\n");
	    	break;
	    case CFGMGR_COMPRESSION_PARAMS:
	    	dprintf ("ecode = CFGMGR_COMPRESSION_PARAMS\n");
	    	break;
	    case CFGMGR_RETRAIN_ENABLED:
	    	dprintf ("ecode = CFGMGR_RETRAIN_ENABLED\n");
	    	break;
	    case CFGMGR_CADENCE_PARAMS:
	    	dprintf ("ecode = CFGMGR_CADENCE_PARAMS\n");
	    	break;
	    case CFGMGR_FORCE_RENEG_UP:
	    	dprintf ("ecode = CFGMGR_FORCE_RENEG_UP\n");
	    	break;
	    case CFGMGR_FORCE_RENEG_DOWN:
	    	dprintf ("ecode = CFGMGR_FORCE_RENEG_DOWN\n");
	    	break;
	    case CFGMGR_IN_V34FAX:
	    	dprintf ("ecode = CFGMGR_IN_V34FAX\n");
	    	break;
	    case CFGMGR_NRINGS_TO_ANSWER:
	    	dprintf ("ecode = CFGMGR_NRINGS_TO_ANSWER\n");
	    	break;
	    case CFGMGR_RING_COUNTER:
	    	dprintf ("ecode = CFGMGR_RING_COUNTER\n");
	    	break;
	    case CFGMGR_ESCAPE_CHAR:
	    	dprintf ("ecode = CFGMGR_ESCAPE_CHAR\n");
	    	break;
	    case CFGMGR_CR_CHAR:
	    	dprintf ("ecode = CFGMGR_CR_CHAR\n");
	    	break;
	    case CFGMGR_LF_CHAR:
	    	dprintf ("ecode = CFGMGR_LF_CHAR\n");
	    	break;
	    case CFGMGR_BS_CHAR:
	    	dprintf ("ecode = CFGMGR_BS_CHAR\n");
	    	break;
	    case CFGMGR_BLIND_DIAL_WAIT_TIME:
	    	dprintf ("ecode = CFGMGR_BLIND_DIAL_WAIT_TIME\n");
	    	break;
	    case CFGMGR_CARRIER_WAIT_TIME:
	    	dprintf ("ecode = CFGMGR_CARRIER_WAIT_TIME\n");
	    	break;
	    case CFGMGR_PAUSE_DELAY_TIME:
	    	dprintf ("ecode = CFGMGR_PAUSE_DELAY_TIME\n");
	    	break;
	    case CFGMGR_CARDET_RESPONSE_TIME:
	    	dprintf ("ecode = CFGMGR_CARDET_RESPONSE_TIME\n");
	    	break;
	    case CFGMGR_HANGUP_DELAY_TIME:
	    	dprintf ("ecode = CFGMGR_HANGUP_DELAY_TIME\n");
	    	break;
	    case CFGMGR_DTMF_DURATION:
	    	dprintf ("ecode = CFGMGR_DTMF_DURATION\n");
	    	break;
	    case CFGMGR_ESCAPE_PROMPT_DELAY:
	    	dprintf ("ecode = CFGMGR_ESCAPE_PROMPT_DELAY\n");
	    	break;
	    case CFGMGR_TEST_TIMER:
	    	dprintf ("ecode = CFGMGR_TEST_TIMER\n");
	    	break;
	    case CFGMGR_FLASH_DELAY_TIME:
	    	dprintf ("ecode = CFGMGR_FLASH_DELAY_TIME\n");
	    	break;
	    case CFGMGR_INACTIVITY_TIME:
	    	dprintf ("ecode = CFGMGR_INACTIVITY_TIME\n");
	    	break;
	    case CFGMGR_DATA_COMPRESSION:
	    	dprintf ("ecode = CFGMGR_DATA_COMPRESSION\n");
	    	break;
	    case CFGMGR_EXTENDED_RESULT_CODES:
	    	dprintf ("ecode = CFGMGR_EXTENDED_RESULT_CODES\n");
	    	break;
	    case CFGMGR_SREG_LAST:
	    	dprintf ("ecode = CFGMGR_SREG_LAST\n");
	    	break;
	    case CFGMGR_DIALSTRING:
	    	dprintf ("ecode = CFGMGR_DIALSTRING\n");
	    	break;
	    case CFGMGR_PULSE_DIAL_CONFIG:
	    	dprintf ("ecode = CFGMGR_PULSE_DIAL_CONFIG\n");
	    	break;
	    case CFGMGR_TONE_DIAL_CONFIG:
	    	dprintf ("ecode = CFGMGR_TONE_DIAL_CONFIG\n");
	    	break;
	    case CFGMGR_DIALTONE_WAIT_TIME:
	    	dprintf ("ecode = CFGMGR_DIALTONE_WAIT_TIME\n");
	    	break;
	    case CFGMGR_DIAL_MODE:
	    	dprintf ("ecode = CFGMGR_DIAL_MODE\n");
	    	break;
	    case CFGMGR_USE_S7_WHEN_W:
	    	dprintf ("ecode = CFGMGR_USE_S7_WHEN_W\n");
	    	break;
	    case CFGMGR_INVERT_CALLING_TONE:
	    	dprintf ("ecode = CFGMGR_INVERT_CALLING_TONE\n");
	    	break;
	    case CFGMGR_CALL_PROGRESS_TONE_DEBOUNCE:
	    	dprintf ("ecode = CFGMGR_CALL_PROGRESS_TONE_DEBOUNCE\n");
	    	break;
	    case CFGMGR_CALL_MODE:
	    	dprintf ("ecode = CFGMGR_CALL_MODE\n");
	    	break;
	    case CFGMGR_CALL_PROGRESS_TIMING:
	    	dprintf ("ecode = CFGMGR_CALL_PROGRESS_TIMING\n");
	    	break;
	    case CFGMGR_CALL_PROGRESS_FLAGS:
	    	dprintf ("ecode = CFGMGR_CALL_PROGRESS_FLAGS\n");
	    	break;
	    case CFGMGR_V8BIS_CONTROL:
	    	dprintf ("ecode = CFGMGR_V8BIS_CONTROL\n");
	    	break;
	    case CFGMGR_V8BIS_RESULT:
	    	dprintf ("ecode = CFGMGR_V8BIS_RESULT\n");
	    	break;
	    case CFGMGR_FIRST_CONNECTION_AFTER_TRAINING:
	    	dprintf ("ecode = CFGMGR_FIRST_CONNECTION_AFTER_TRAINING\n");
	    	break;
	    case CFGMGR_PREFERRED_FLEX_VERSION:
	    	dprintf ("ecode = CFGMGR_PREFERRED_FLEX_VERSION\n");
	    	break;
	    case CFGMGR_V90_ENABLED:
	    	dprintf ("ecode = CFGMGR_V90_ENABLED\n");
	    	break;
	    case CFGMGR_V70_ALLOWED:
	    	dprintf ("ecode = CFGMGR_V70_ALLOWED\n");
	    	break;
	    case CFGMGR_ATRESULT_OK:
	    	dprintf ("ecode = CFGMGR_ATRESULT_OK\n");
	    	break;
	    case CFGMGR_ATRESULT_CONNECT:
	    	dprintf ("ecode = CFGMGR_ATRESULT_CONNECT\n");
	    	break;
	    case CFGMGR_ATRESULT_RING:
	    	dprintf ("ecode = CFGMGR_ATRESULT_RING\n");
	    	break;
	    case CFGMGR_ATRESULT_NOCARRIER:
	    	dprintf ("ecode = CFGMGR_ATRESULT_NOCARRIER\n");
	    	break;
	    case CFGMGR_ATRESULT_ERROR:
	    	dprintf ("ecode = CFGMGR_ATRESULT_ERROR\n");
	    	break;
	    case CFGMGR_ATRESULT_NODIALTONE:
	    	dprintf ("ecode = CFGMGR_ATRESULT_NODIALTONE\n");
	    	break;
	    case CFGMGR_ATRESULT_BUSY:
	    	dprintf ("ecode = CFGMGR_ATRESULT_BUSY\n");
	    	break;
	    case CFGMGR_ATRESULT_NOANSWER:
	    	dprintf ("ecode = CFGMGR_ATRESULT_NOANSWER\n");
	    	break;
	    case CFGMGR_ATRESULT_DELAYED:
	    	dprintf ("ecode = CFGMGR_ATRESULT_DELAYED\n");
	    	break;
	    case CFGMGR_ATRESULT_BLACKLISTED:
	    	dprintf ("ecode = CFGMGR_ATRESULT_BLACKLISTED\n");
	    	break;
	    case CFGMGR_ATRESULT_FAX:
	    	dprintf ("ecode = CFGMGR_ATRESULT_FAX\n");
	    	break;
	    case CFGMGR_ATRESULT_DATA:
	    	dprintf ("ecode = CFGMGR_ATRESULT_DATA\n");
	    	break;
	    case CFGMGR_ATRESULT_FCERROR:
	    	dprintf ("ecode = CFGMGR_ATRESULT_FCERROR\n");
	    	break;
	    case CFGMGR_MCR_B103:
	    	dprintf ("ecode = CFGMGR_MCR_B103\n");
	    	break;
	    case CFGMGR_MCR_B212:
	    	dprintf ("ecode = CFGMGR_MCR_B212\n");
	    	break;
	    case CFGMGR_MCR_V21:
	    	dprintf ("ecode = CFGMGR_MCR_V21\n");
	    	break;
	    case CFGMGR_MCR_V22:
	    	dprintf ("ecode = CFGMGR_MCR_V22\n");
	    	break;
	    case CFGMGR_MCR_V22B:
	    	dprintf ("ecode = CFGMGR_MCR_V22B\n");
	    	break;
	    case CFGMGR_MCR_V23:
	    	dprintf ("ecode = CFGMGR_MCR_V23\n");
	    	break;
	    case CFGMGR_MCR_V32:
	    	dprintf ("ecode = CFGMGR_MCR_V32\n");
	    	break;
	    case CFGMGR_MCR_V32B:
	    	dprintf ("ecode = CFGMGR_MCR_V32B\n");
	    	break;
	    case CFGMGR_MCR_V34:
	    	dprintf ("ecode = CFGMGR_MCR_V34\n");
	    	break;
	    case CFGMGR_MCR_K56:
	    	dprintf ("ecode = CFGMGR_MCR_K56\n");
	    	break;
	    case CFGMGR_MCR_V90:
	    	dprintf ("ecode = CFGMGR_MCR_V90\n");
	    	break;
	    case CFGMGR_ER_NONE:
	    	dprintf ("ecode = CFGMGR_ER_NONE\n");
	    	break;
	    case CFGMGR_ER_LAPM:
	    	dprintf ("ecode = CFGMGR_ER_LAPM\n");
	    	break;
	    case CFGMGR_ER_ALT:
	    	dprintf ("ecode = CFGMGR_ER_ALT\n");
	    	break;
	    case CFGMGR_DR_ALT:
	    	dprintf ("ecode = CFGMGR_DR_ALT\n");
	    	break;
	    case CFGMGR_DR_V42B:
	    	dprintf ("ecode = CFGMGR_DR_V42B\n");
	    	break;
	    case CFGMGR_DR_NONE:
	    	dprintf ("ecode = CFGMGR_DR_NONE\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I0:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I0\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I1:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I1\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I2:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I2\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I3:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I3\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I4:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I4\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I5:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I5\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I6:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I6\n");
	    	break;
	    case CFGMGR_ATMESSAGE_I7:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_I7\n");
	    	break;
	    case CFGMGR_ATMESSAGE_MANUFACTURER:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_MANUFACTURER\n");
	    	break;
	    case CFGMGR_ATMESSAGE_MODEL:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_MODEL\n");
	    	break;
	    case CFGMGR_ATMESSAGE_REVISION:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_REVISION\n");
	    	break;
	    case CFGMGR_ATMESSAGE_SERIAL_NUM:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_SERIAL_NUM\n");
	    	break;
	    case CFGMGR_ATMESSAGE_GOI:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_GOI\n");
	    	break;
	    case CFGMGR_ATMESSAGE_CAPABILITIES:
	    	dprintf ("ecode = CFGMGR_ATMESSAGE_CAPABILITIES\n");
	    	break;
	    case CFGMGR_FAXMESSAGE_MANUFACTURER:
	    	dprintf ("ecode = CFGMGR_FAXMESSAGE_MANUFACTURER\n");
	    	break;
	    case CFGMGR_FAXMESSAGE_MODEL:
	    	dprintf ("ecode = CFGMGR_FAXMESSAGE_MODEL\n");
	    	break;
	    case CFGMGR_FAXMESSAGE_REVISION:
	    	dprintf ("ecode = CFGMGR_FAXMESSAGE_REVISION\n");
	    	break;
	    case CFGMGR_DTE_ECHO:
	    	dprintf ("ecode = CFGMGR_DTE_ECHO\n");
	    	break;
	    case CFGMGR_DTE_CONFIG:
	    	dprintf ("ecode = CFGMGR_DTE_CONFIG\n");
	    	break;
	    case CFGMGR_SYNC_MODE:
	    	dprintf ("ecode = CFGMGR_SYNC_MODE\n");
	    	break;
	    case CFGMGR_RLSD_BEHAVIOR:
	    	dprintf ("ecode = CFGMGR_RLSD_BEHAVIOR\n");
	    	break;
	    case CFGMGR_DTR_BEHAVIOR:
	    	dprintf ("ecode = CFGMGR_DTR_BEHAVIOR\n");
	    	break;
	    case CFGMGR_SPEAKER_VOLUME:
	    	dprintf ("ecode = CFGMGR_SPEAKER_VOLUME\n");
	    	break;
	    case CFGMGR_SPEAKER_CONTROL:
	    	dprintf ("ecode = CFGMGR_SPEAKER_CONTROL\n");
	    	break;
	    case CFGMGR_PULSE_MAKE_BREAK:
	    	dprintf ("ecode = CFGMGR_PULSE_MAKE_BREAK\n");
	    	break;
	    case CFGMGR_RING_BURST:
	    	dprintf ("ecode = CFGMGR_RING_BURST\n");
	    	break;
	    case CFGMGR_ANSWER_TONE_DETECTOR:
	    	dprintf ("ecode = CFGMGR_ANSWER_TONE_DETECTOR\n");
	    	break;
	    case CFGMGR_BELL_TONE_DETECTOR:
	    	dprintf ("ecode = CFGMGR_BELL_TONE_DETECTOR\n");
	    	break;
	    case CFGMGR_DTMF_GENERATOR:
	    	dprintf ("ecode = CFGMGR_DTMF_GENERATOR\n");
	    	break;
	    case CFGMGR_MODULATION_REPORT:
	    	dprintf ("ecode = CFGMGR_MODULATION_REPORT\n");
	    	break;
	    case CFGMGR_BUSY_TONE_CADENCE:
	    	dprintf ("ecode = CFGMGR_BUSY_TONE_CADENCE\n");
	    	break;
	    case CFGMGR_RING_CADENCE:
	    	dprintf ("ecode = CFGMGR_RING_CADENCE\n");
	    	break;
	    case CFGMGR_REORDER_TONE_CADENCE:
	    	dprintf ("ecode = CFGMGR_REORDER_TONE_CADENCE\n");
	    	break;
	    case CFGMGR_SDIAL_TONE_CADENCE:
	    	dprintf ("ecode = CFGMGR_SDIAL_TONE_CADENCE\n");
	    	break;
	    case CFGMGR_FAX_CALL_TONE_CADENCE:
	    	dprintf ("ecode = CFGMGR_FAX_CALL_TONE_CADENCE\n");
	    	break;
	    case CFGMGR_FAX_AUTO_ANSWER:
	    	dprintf ("ecode = CFGMGR_FAX_AUTO_ANSWER\n");
	    	break;
	    case CFGMGR_OEM_FLAGS:
	    	dprintf ("ecode = CFGMGR_OEM_FLAGS\n");
	    	break;
	    case CFGMGR_OEM_FILTERS:
	    	dprintf ("ecode = CFGMGR_OEM_FILTERS\n");
	    	break;
	    case CFGMGR_OEM_SPKR_MUTE_DELAY:
	    	dprintf ("ecode = CFGMGR_OEM_SPKR_MUTE_DELAY\n");
	    	break;
	    case CFGMGR_OEM_READ_MASK:
	    	dprintf ("ecode = CFGMGR_OEM_READ_MASK\n");
	    	break;
	    case CFGMGR_OEM_THRESHOLD:
	    	dprintf ("ecode = CFGMGR_OEM_THRESHOLD\n");
	    	break;
	    case CFGMGR_COUNTRY_CODE:
	    	dprintf ("ecode = CFGMGR_COUNTRY_CODE\n");
	    	break;
	    case CFGMGR_PREVIOUS_COUNTRY_CODE:
	    	dprintf ("ecode = CFGMGR_PREVIOUS_COUNTRY_CODE\n");
	    	break;
	    case CFGMGR_COUNTRY_NAME:
	    	dprintf ("ecode = CFGMGR_COUNTRY_NAME\n");
	    	break;
	    case CFGMGR_COUNTRY_CODE_LIST:
	    	dprintf ("ecode = CFGMGR_COUNTRY_CODE_LIST\n");
	    	break;
	    case CFGMGR_COUNTRY_STRUCT:
	    	dprintf ("ecode = CFGMGR_COUNTRY_STRUCT\n");
	    	break;
	    case CFGMGR_FILTERS:
	    	dprintf ("ecode = CFGMGR_FILTERS\n");
	    	break;
	    case CFGMGR_DTMF:
	    	dprintf ("ecode = CFGMGR_DTMF\n");
	    	break;
	    case CFGMGR_RING_PARAMS:
	    	dprintf ("ecode = CFGMGR_RING_PARAMS\n");
	    	break;
	    case CFGMGR_RLSDOFFSET:
	    	dprintf ("ecode = CFGMGR_RLSDOFFSET\n");
	    	break;
	    case CFGMGR_THRESHOLD:
	    	dprintf ("ecode = CFGMGR_THRESHOLD\n");
	    	break;
	    case CFGMGR_TXLEVELS:
	    	dprintf ("ecode = CFGMGR_TXLEVELS\n");
	    	break;
	    case CFGMGR_RELAYS:
	    	dprintf ("ecode = CFGMGR_RELAYS\n");
	    	break;
	    case CFGMGR_SPEED_ADJUST:
	    	dprintf ("ecode = CFGMGR_SPEED_ADJUST\n");
	    	break;
	    case CFGMGR_SREG_LIMITS:
	    	dprintf ("ecode = CFGMGR_SREG_LIMITS\n");
	    	break;
	    case CFGMGR_PR_IGNORE_TIME:
	    	dprintf ("ecode = CFGMGR_PR_IGNORE_TIME\n");
	    	break;
	    case CFGMGR_CID_TYPE:
	    	dprintf ("ecode = CFGMGR_CID_TYPE\n");
	    	break;
	    case CFGMGR_PROFILE_STORED:
	    	dprintf ("ecode = CFGMGR_PROFILE_STORED\n");
	    	break;
	    case CFGMGR_PROFILE_FACTORY:
	    	dprintf ("ecode = CFGMGR_PROFILE_FACTORY\n");
	    	break;
	    case CFGMGR_POUND_UD:
	    	dprintf ("ecode = CFGMGR_POUND_UD\n");
	    	break;
	    case CFGMGR_ATPARSER_ONLINE:
	    	dprintf ("ecode = CFGMGR_ATPARSER_ONLINE\n");
	    	break;
	    case CFGMGR_LAST:
	    	dprintf ("ecode = CFGMGR_LAST\n");
	    	break;
	    default:
			dprintf ("ecode = %d: (UNKNOWN)\n", eCode );
	}
}
#endif
