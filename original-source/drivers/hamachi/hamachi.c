/*
    hamachi.c: Packet Engines G-NIC II Ethernet driver for BeOS   
               
	russ 4/28/99	

	(c) 1997-99 Be, Inc. All rights reserved

*/

#include <Drivers.h>
#include <Errors.h>
#include <PCI.h>
#include <module.h>

#include "Resource.h"

static uint8 debug = 1;  /* 1 is the shipping default. 5 turns on all. */

/* Modules allows bus abstraction for - USB, 1394 ... */
static pci_module_info	*pci;
static char pci_name[] = B_PCI_MODULE_NAME;
#define ANY_BUS

/* Filled out for every hamachi card in machine. */
typedef struct{
	char name[128];
	uint8 irq;
	area_id memAreaID;	/* Used to delete area. */
	uchar* cardCtlBaseAddr;
	uint8 etherAddr[6];
} DEVINFOTYPE; 	
DEVINFOTYPE g_devInfo[16]; /* Support up to 16 hamachi cards. */
static char* g_DevNameList[16];
	
uint8 g_foundCards = 0; /* The number of hamachi cards in this box. */

/* The hamachi card expects the following description during
   dma rx/tx transfers. */
typedef struct{
	uint16 packetLen;
	uint16 transStatus; /* Transaction status. */
	uint32 packetAddr;
} DEVICEDESCRIPTIONTYPE;

/* Passed in during open, and then held onto by kernel and
   passed in on most kernel calls. */
typedef struct{
	char name[32];
	uint8 irq;
	uchar* cardCtlBaseAddr; /* Where to set card values in I0 memory. */
	area_id memAreaID; /* AreaIDs are stored, so they can be deleted. */
	area_id descAreaID; 
	area_id rxAreaID; 
	area_id txAreaID; 
	uchar* descBaseAddr;  /* Where descriptor lists live. */
	uchar* rxBaseAddr;	/* Where receive packets are put. */	 
	uchar* txBaseAddr;	/* Where to be transmitted packets are put. */
	uint64 rxHead; /* Buffer offset passed to user land. */
	uint64 rxTail; /* Incomming packets offset. */
	uint64 txHead; 
	uint64 txTail; 
	uint8 etherAddr[6];
	sem_id rxSemaphore;
	sem_id txSemaphore;
} DEVICETYPE;

/************************ Locally called functions. ************************/

static bool hamachi_probe(bool SetGlobals)
{	
	uint16 i, j;
	uint8 irq;
	int32 phys;
	int32 size;
	uchar* cardCtlBaseAddr;
	area_id memAreaID;	
	uint32 chiprev;
	uint8 addr[6];
	pci_info pciInfo;
	
	g_foundCards = 0; /* No cards. */
		
	/* Hook up to the PCI module currently the only module
	   supported by this device. */
	if (get_module(pci_name, (module_info **) &pci))
		return B_ERROR;
		
	for (i = 0; i < 0xff; i++) {
		if ((*pci->get_nth_pci_info)(i, &pciInfo) != B_OK )
			continue;
		if (pciInfo.vendor_id != VendorID)
			continue;
		switch (pciInfo.device_id) {
			case DeviceID:{
			        if (!SetGlobals) /* Just check if card present. */
						return true;
						
					if (debug > 3){	
						dprintf("************* Hamachi Gigabit Driver ***************\n");
						dprintf("********** by Russ McMahon (russ@be.com) ***********\n");	
					}	
						
					irq = pciInfo.u.h0.interrupt_line;
					phys = pciInfo.u.h0.base_registers[0];
					
					phys = phys & ~(B_PAGE_SIZE - 1);								
					size = pciInfo.u.h0.base_register_sizes[0];
					size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);	
					
					/* This is a memory mapped device. The physical address (pci base address) has
					   a flag masked in it that tells us more about how to access the cards
					   configuration. I use Linux /proc/pci to tell me about the card, so there is
					   no checking done here. */
					memAreaID = map_physical_memory("Hamachi_gig_driver",											
											(void*)phys,
											size,
											B_ANY_KERNEL_ADDRESS,
											B_READ_AREA + B_WRITE_AREA,
											&cardCtlBaseAddr);
					
					if (memAreaID < 0)
						return (B_ERROR);
					
					cardCtlBaseAddr += (phys & (B_PAGE_SIZE - 1)) / sizeof(ulong);
					chiprev = readl(cardCtlBaseAddr + ChipRev);					
					
					dprintf("Found Hamachi device irq %d physaddr %2.2x rev %x\n", irq, phys, chiprev);						
					
					for (j = 0; j < 6; j++)
						addr[j] = readb(cardCtlBaseAddr + MacAddr + j);
						
					dprintf("Hamachi ethernet address "); 
					for (j = 0; j < 5; j++)
						dprintf("%2.2x:", addr[j]);	
					dprintf("%2.2x\n", addr[j]);
					
					/* Hardware specific info to be later associated with the cookie
					   and the actual hardware interface like eth1, 2, 3 etc. */
					sprintf(g_devInfo[g_foundCards].name, "%s%ld", k_DeviceName, g_foundCards);					
					g_devInfo[g_foundCards].irq = irq;
					g_devInfo[g_foundCards].memAreaID = memAreaID;
					g_devInfo[g_foundCards].cardCtlBaseAddr = cardCtlBaseAddr;
					/*Fill out array that will become our published devices. */
					g_DevNameList[g_foundCards] = (char *)malloc( strlen(g_devInfo[g_foundCards].name)+1);
					strcpy(g_DevNameList[g_foundCards], g_devInfo[g_foundCards].name);					
					for (j = 0; j < 6; j++)
							g_devInfo[g_foundCards].etherAddr[j] = addr[j];
					g_foundCards++;						
				}
				break;
			default:
				dprintf("Unknown device.\n");
				break;
		}
	}
	return (g_foundCards ? true : false);
}

static bool InitializeRingBuffers(DEVICETYPE* dev)
{
	uint16 i;
	area_id descAreaID;
	area_id rxAreaID;
	area_id txAreaID;
	uchar* descBaseAddr;
	uchar* rxBaseAddr;
	uchar* txBaseAddr;	
	uint32 desc_offset = 0;
	uint32 data_offset = 0;	
	DEVICEDESCRIPTIONTYPE* rxDesc;
	DEVICEDESCRIPTIONTYPE* txDesc;	
	
	
	/****************************************************
	By using create area I can be assured I get a contiguous
	block of memory that can be divided up and passed as physical
	memory to the card. As can be seen the current layout
	uses the defined ring buffer sizes with a bit of padding for
	small tweaks. Large layout changes will require chages to
	the area. 
	
	Hamachi areas.
	 
	descBaseAddr_______________
	|                          |
	| Descriptors are 8 bytes. |
	| 32 rx descriptors.       |
	| 16 tx descriptors.       |
	|__________________________|                          
	
	rxBaseAddr_________________
	|                          |
	| Data packets are 1536.   |
	| 32 rx packets.           |         
	|                          |
	|__________________________|
	
	txBaseAddr_________________
	|                          |
	| Data packets are 1536.   |
	| 16 tx packets.           |
	|                          |
	|__________________________|
		
	****************************************************/
	
	/************* Allocate 3 areas ********************/
	descBaseAddr = (uchar*)get_area("hamachi_descarea",
					((k_rxRingSize + k_txRingSize) * sizeof(DEVICEDESCRIPTIONTYPE)),	
													  &dev->descAreaID);
	if (!descBaseAddr){
		if (debug > 3)
			dprintf("%s: Problem allocating descriptor area.\n", dev->name);
		return false;
	}											
	dev->descBaseAddr = descBaseAddr;												
	rxBaseAddr = (uchar*)get_area("hamachi_rxarea", (k_rxRingSize * k_packetLen),
													  &dev->rxAreaID);
	if (!rxBaseAddr){
		if (debug > 3)
			dprintf("%s: Problem allocating rx area.\n", dev->name);
		return false;
	}			
	dev->rxBaseAddr = rxBaseAddr;
	txBaseAddr = (uchar*)get_area("hamachi_txarea", (k_txRingSize * k_packetLen),
													  &dev->txAreaID);
	if (!txBaseAddr){
		if (debug > 3)
			dprintf("%s: Problem allocating tx area.\n", dev->name);
		return false;
	}																	
	dev->txBaseAddr = txBaseAddr;
	/***************************************************/
	
	/* Divy-up-amatic. */
	for (i = 0; i < k_rxRingSize; i++){
		rxDesc = (DEVICEDESCRIPTIONTYPE*)(descBaseAddr + desc_offset);
		rxDesc->packetLen = k_packetLen;
		rxDesc->transStatus = DescOwn | DescEndPacket | DescIntr;
		rxDesc->packetAddr = (uint32)virt_to_bus((rxBaseAddr + data_offset), k_packetLen);
		
		desc_offset += sizeof(DEVICEDESCRIPTIONTYPE);
		data_offset += k_packetLen;	
	}
	
	/* Mark the last entry as wrapping the ring. */
	rxDesc->transStatus |= DescEndRing;
	
	data_offset = 0;
	/* Divy-up-amatic. */
	for (i = 0; i < k_txRingSize; i++){
		txDesc = (DEVICEDESCRIPTIONTYPE*)(descBaseAddr + desc_offset);
		txDesc->transStatus = DescEndPacket | DescIntr;
		txDesc->packetAddr = (uint32)virt_to_bus((txBaseAddr + data_offset), k_packetLen);
		
		desc_offset += sizeof(DEVICEDESCRIPTIONTYPE);
		data_offset += k_packetLen;	
	}
	
	/* Mark the last entry as wrapping the ring. */
	txDesc->transStatus |= DescEndRing;
	
	dev->rxHead = 0;
	dev->rxTail = 0;
	
	dev->txHead = 0;
	dev->txTail = 0;
	
	return true;
}

/* Called from interrrupt handler. Process incoming packets. */
void ReceivePackets(DEVICETYPE* dev)
{
	uchar* base;
	DEVICEDESCRIPTIONTYPE* rxDesc;
	uint32 countPackets = 0;
	uint32 ringOffset;
	
	/* Unlike the transmit where a empty buffer is used to stop the ring from
	   wrapping ( see create tx semaphore), here we use some tricky masks to
	   ensure the rx tail does not walk over haed. */
	uint32 ringsizemask = k_rxRingSize - 1;
	uint16 limit = ((ringsizemask - (dev->rxHead - dev->rxTail)) & ringsizemask);
	
	ringOffset = dev->rxTail * sizeof(DEVICEDESCRIPTIONTYPE);	
	base = dev->descBaseAddr; 
	rxDesc = (DEVICEDESCRIPTIONTYPE*)(base + ringOffset);
	
	/* The card has cleared the DescOwn flag and this tells us it is our
	   packet now. In the future the loop should attempt to process as many packets
	   as possible during an interrupt. */
	while (limit--){
		if (debug > 4)
			dprintf("%s: Receiving packets. Status %2.2x Length %d\n", dev->name, rxDesc->transStatus,
																rxDesc->packetLen);
		
		/* The card has cleared the DescOwn flag and this tells us it is our
	   	   packet now. */				
		if (rxDesc->transStatus & DescOwn)
			break;
		
		if (rxDesc->transStatus & DescEndPacket){			
			countPackets++;	
						
			/* Next up. */
			dev->rxTail = ((dev->rxTail+1) % k_rxRingSize);
			ringOffset = dev->rxTail * sizeof(DEVICEDESCRIPTIONTYPE);	
			rxDesc = (DEVICEDESCRIPTIONTYPE*)(base + ringOffset);
		}		
	}
			
	if (countPackets)
		release_sem_etc(dev->rxSemaphore, countPackets, B_DO_NOT_RESCHEDULE);
	
	/* Reset receiver. */
	writew(1, dev->cardCtlBaseAddr + RxCmd);
}

/* Called from interrrupt handler. Hardware has processed outgoing packets. */
void TransmitPacketsDone(DEVICETYPE* dev)
{
	uchar* base;
	DEVICEDESCRIPTIONTYPE* txDesc;
	uint32 countPackets = 0;
	uint32 rxChunkSize;
	uint32 ringOffset;
	
	base = dev->descBaseAddr;
	ringOffset = dev->txHead * sizeof(DEVICEDESCRIPTIONTYPE);	 
	rxChunkSize = sizeof(DEVICEDESCRIPTIONTYPE) * k_rxRingSize; /* Skip over rx descriptors. */
	txDesc = (DEVICEDESCRIPTIONTYPE*)(base + rxChunkSize + ringOffset);
		
	/* The card has cleared the DescOwn flag and this tells us it is our
	   packet now. */
	while (dev->txHead != dev->txTail){	 
	   if (debug > 4)
			dprintf("%s: Transmitted packets. Status %2.2x Length %d\n", dev->name, txDesc->transStatus,	
															txDesc->packetLen);
				
		if (txDesc->transStatus & DescOwn)
			break;
		
		if (txDesc->transStatus & DescEndPacket){															
			countPackets++; 				
				
			/* Move head. */
			dev->txHead = ((dev->txHead+1) % k_txRingSize);
			rxChunkSize = sizeof(DEVICEDESCRIPTIONTYPE) * k_rxRingSize; /* Skip over rx descriptors. */
			ringOffset = dev->txHead * sizeof(DEVICEDESCRIPTIONTYPE);
			txDesc = (DEVICEDESCRIPTIONTYPE*)(base + rxChunkSize + ringOffset);
		}		
	}	
	
	if (countPackets){
		if (debug > 4)
			dprintf("Transmited packet count %d\n", countPackets);
		release_sem_etc(dev->txSemaphore, countPackets, B_DO_NOT_RESCHEDULE);		
	}
}

/* Device interrupt handler. All packets are recieved here
   and processed here. */
bool hamachi_interrupt(void* data)
{
	DEVICETYPE* dev = (DEVICETYPE*)data; /* Sometimes cookie/data */ 
	int32 handled = 0; 
	uint16 limit = 100;
	uint32 status;
	uchar* cardCtlBaseAddr = dev->cardCtlBaseAddr; /* Easier to follow. */
	uchar* base;
	DEVICEDESCRIPTIONTYPE* txDesc;
	uint32 rxChunkSize;
	uint32 ringOffset;
	
	while (limit--){
		status = readl(cardCtlBaseAddr + InterruptClear);
		
		if (debug > 4)
			kprintf("%s: In interrupt. Limit %d Handled %d Status %2.2x\n",
												dev->name, limit, handled, status);
	
		if (status == 0)
			break;	
		
		if (status & IntrRxDone)
			ReceivePackets(dev);
			
		if (status & IntrTxDone)
			TransmitPacketsDone(dev);
		
		if (status & (LinkChange | NegotiationChange))
			if (readw(cardCtlBaseAddr + AutoNegStatus))
				writeb(0x01, cardCtlBaseAddr + LEDCtrl);	/* Green light. Go! */		
			else
				writeb(0x03, cardCtlBaseAddr + LEDCtrl);	
				
		handled = 1; 
	}

    /* This condition means there was an interrupt for us, but we did not
       handle it. */
	if (limit <= 0)
		dprintf("%s: Spin out. Limit %d Handled %d Status %2.2x\n",
												dev->name, limit, handled, status);
	return handled;
}

/* The init_hardware function is called when the driver is first
   loaded on OS boot. */ 
status_t init_hardware(void)
{
	bool foundCard;
			
	/* Check if there is a hamachi card. This probe
	   passes in false so that there is no saved
	   information. */		
	foundCard = hamachi_probe(false);
	
	return (foundCard ? B_NO_ERROR : B_ERROR);
}

status_t init_driver(void)
{
	bool foundCard;
	
	/* Check if there is a hamachi card and save
	   global information. */
	foundCard = hamachi_probe(true);
	
	return (foundCard ? B_NO_ERROR : B_ERROR);
}

void uninit_driver(void)
{
 	uint8 i;
	void *item;

	for (i=0; i < g_foundCards; i++)
		delete_area(g_devInfo[i].memAreaID);
	
	/* Free device name list. */
	for(i=0; (item=g_DevNameList[i]); i++)
		free(item);			
			
	put_module(pci_name);	
}

/*	The hamachi_open function is called when the user does an open on
    the devices special file name. */
static status_t hamachi_open(const char *name, uint32 flags, void** cookie)
{
	DEVICETYPE* dev;
	uchar* cardCtlBaseAddr;
	uchar* base;
	uint32 txChunkSize;
	uint32 rxChunkSize;	
	uint16 i, t;
	
	dev = (DEVICETYPE*)malloc(sizeof(DEVICETYPE));	
	if (!dev) /* Most likely no memory. */
		return B_ERROR;
	
	/* Save for later. This is our persistant data. Our cookie.
	   Take the early probe information for each card and put
	   it into our dev structure. */
	for(i = 0; i < g_foundCards; i++)
		if(strcmp(name, g_DevNameList[i]) == 0 ){
			sprintf(dev->name, g_devInfo[i].name); 
			dev->memAreaID = g_devInfo[i].memAreaID;	
			dev->cardCtlBaseAddr = g_devInfo[i].cardCtlBaseAddr;
			dev->irq = g_devInfo[i].irq;
			for (t = 0; t < 6; t++)
				dev->etherAddr[t] =  g_devInfo[i].etherAddr[t];
			break;
		}
		
    dev->rxSemaphore = create_sem(0, "rxSemaphore");
    /* The transmit semaphore is created initially with the size of the
       transmit buffer - 1. The minus 1 ensures that tail will never step on
       head. There is a buffer that is wasted, but not a big deal.*/
	dev->txSemaphore = create_sem(k_txRingSize -1, "txSemaphore");
			
	cardCtlBaseAddr = dev->cardCtlBaseAddr; /* Easier to follow. */
	
	/* Initialize the card. */						
	writeb(1, cardCtlBaseAddr + ChipReset);
	writew(0x0400, cardCtlBaseAddr + AutoNegChngCtrl);
	writew(0x08E0, cardCtlBaseAddr + AutoNegAdvertise);
	writew(0x1000, cardCtlBaseAddr + AutoNegCtrl);
		
	if (install_io_interrupt_handler(dev->irq, &hamachi_interrupt, dev, 0)
															!= B_NO_ERROR)
		return B_ERROR;
		
	if (!InitializeRingBuffers(dev))
		return B_ERROR;
				
	base = dev->descBaseAddr; /* The begining of hamachi contiguous memory. */
		
	if (debug > 3)
		dprintf("%s: Contiguous area memaddr %2.2x\n", dev->name, base);
			
	/* Set the cards ring buffers. A bit of arithmatic here sorry.
	   Start of rx descriptor = base. The length is size of descriptor
	   struct times RX_RING_SIZE. */
	rxChunkSize = sizeof(DEVICEDESCRIPTIONTYPE) * k_rxRingSize;
	txChunkSize = sizeof(DEVICEDESCRIPTIONTYPE) * k_txRingSize;
		
	writel(virt_to_bus(base, rxChunkSize), cardCtlBaseAddr + RxPtr);
	writel(virt_to_bus((base + rxChunkSize), txChunkSize), cardCtlBaseAddr + TxPtr);
		
	writew(0x0028, cardCtlBaseAddr + FIFOcfg);
	writew(0x0001, cardCtlBaseAddr + RxChecksum);
	writew(0x8000, cardCtlBaseAddr + MACCnfg); /* Soft MAC reset. */
	writew(0x215F, cardCtlBaseAddr + MACCnfg);
	writew(0x000C, cardCtlBaseAddr + FrameGap0);
	writew(0x1018, cardCtlBaseAddr + FrameGap1);
	writew(0x0780, cardCtlBaseAddr + MACCnfg2);
	/* Enable hardware flow control. */
	writel(0x0030FFFF, cardCtlBaseAddr + FlowCtrl);
	writew(1536, cardCtlBaseAddr + MaxFrameSize);
		
	writew(0x0400, cardCtlBaseAddr + AutoNegChngCtrl); /* this was set above??? */
	writeb(0x00001, cardCtlBaseAddr + LEDCtrl);
	writel(0x00080000, cardCtlBaseAddr + TxIntrCtrl);
	writel(0x00000020, cardCtlBaseAddr + RxIntrCtrl);
		
	/* Set the Address mode. */
	writew(0x0001, cardCtlBaseAddr + AddrMode);
		
	/* Enable the interrupts. */
	writel(0x80878787, cardCtlBaseAddr + InterruptEnable);
	writew(0x0000, cardCtlBaseAddr + EventStatus);
		
	/* Start the DMA channels. */
	writew(0x0015, cardCtlBaseAddr + RxDMACtrl);
	writew(0x0015, cardCtlBaseAddr + TxDMACtrl);
	writew(0x0001, cardCtlBaseAddr + RxCmd);
		
	if (debug > 3)
		dprintf("%s: Open done. Status: Rx %x Tx %x.\n", dev->name, readw(cardCtlBaseAddr + RxStatus),
													readw(cardCtlBaseAddr + TxStatus));																	
		
	*cookie = dev;
	
	return B_OK;
}

static status_t hamachi_control(void* cookie, uint32 msg, void* arg, size_t len)
{
	DEVICETYPE* dev = (DEVICETYPE*)cookie;
	uint8 i;
	
	switch (msg) {				
		case ETHER_GETADDR:{
			for (i = 0; i < 6; i++)
				*((uint8*)arg+i) = dev->etherAddr[i];
			}
			return B_OK;
		
		case ETHER_INIT:
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			*(uint32 *)arg = 1514;
			return B_OK;
			
		case ETHER_ADDMULTI:
			return B_ERROR; /* Not supported yet. */
		
		case ETHER_SETPROMISC:
			return B_ERROR; /* Not supported yet. */
		
		case ETHER_NONBLOCK:	
			return B_OK; /* Not supported yet. */
		
		default:
			return B_ERROR;	
					
	}	
}

static status_t hamachi_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	DEVICETYPE* dev = (DEVICETYPE*)cookie;
	status_t status;
	uchar* pRxPacket;
	DEVICEDESCRIPTIONTYPE* rxDesc;
	uint32 rxRingOffset;
	uint32 descRingOffset;
	int32 frameStatus;	
		
	status = acquire_sem_etc(dev->rxSemaphore, 1, B_CAN_INTERRUPT, 0);
	
	if (status != B_NO_ERROR){
		if (debug > 4)
			dprintf("Unable to acquire read semaphore.\n");				
		return status;
	}
	
	/* GetAtHead. */
	rxRingOffset = dev->rxHead * k_packetLen;
	pRxPacket = (uchar*)dev->rxBaseAddr+rxRingOffset;
		
	descRingOffset = dev->rxHead * sizeof(DEVICEDESCRIPTIONTYPE);	
	rxDesc = (DEVICEDESCRIPTIONTYPE*)(dev->descBaseAddr + descRingOffset);
		
	/* Go to end of received packet and see if frame OK. Hamachi card
	   sticks statistics at end of packet. The actual transfered bytes
	   are also at the end. */	
	frameStatus = *((int32*)((pRxPacket + rxDesc->packetLen) - 12));
	if (!(frameStatus & 0x0380000)){ /* Frame OK bits. */	
		*len = (*((uint16*)((pRxPacket + rxDesc->packetLen) - 12))) - 4; 
		memcpy(buf, pRxPacket, *len);
	}
	else{
		if (debug > 4)
			dprintf("There was a frame status receive error.\n");
	}	
		
	/* Do this last so card does not use until after packet has been copied. */
	rxDesc->packetLen = k_packetLen; /* Tell card how many bytes it can copy. */
	rxDesc->transStatus |= DescOwn; 
			
	dev->rxHead = ((dev->rxHead+1) % k_rxRingSize);
	
	/* Reset receiver. */	
	writew(1, dev->cardCtlBaseAddr + RxCmd);
				
	return B_OK;		
}

static status_t hamachi_write(void *cookie, off_t pos, const void *buf, size_t *len)
{
	DEVICETYPE* dev = (DEVICETYPE*)cookie;
	status_t status;
	uchar* pTxPacket;
	DEVICEDESCRIPTIONTYPE* txDesc;
	uint32 rxChunkSize;
	uint32 txRingOffset;
	uint32 descRingOffset;
		
	status = acquire_sem_etc(dev->txSemaphore, 1, B_CAN_INTERRUPT, 0);
		
	if (status != B_NO_ERROR){
		if (debug > 4)
			dprintf("Unable to acquire write semaphore.\n");			
		return status;
	}
	
	/* AddToTail. */
	txRingOffset = dev->txTail * k_packetLen;
		
	memcpy(dev->txBaseAddr+txRingOffset, buf, *len);
	
	descRingOffset = dev->txTail * sizeof(DEVICEDESCRIPTIONTYPE);
	dev->txTail = ((dev->txTail+1) % k_txRingSize);
	
	rxChunkSize = sizeof(DEVICEDESCRIPTIONTYPE) * k_rxRingSize; /* Skip over rx descriptors. */
	txDesc = (DEVICEDESCRIPTIONTYPE*)(dev->descBaseAddr + rxChunkSize + descRingOffset);						
	txDesc->packetLen = *len;
	/* The descriptor offsets are the same as the  packet offsets. */
	txDesc->transStatus |= DescOwn | DescEndPacket;
			
	writew(1, dev->cardCtlBaseAddr + TxCmd);

	return B_OK;	
}

static status_t hamachi_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	return B_OK;
}

static status_t hamachi_deselect(void *cookie, uint8 event, selectsync *sync)
{
	return B_OK;
}

static status_t hamachi_close(void* cookie)
{
	DEVICETYPE* dev = (DEVICETYPE*)cookie;
	char* cardCtlBaseAddr = dev->cardCtlBaseAddr; /* Easier to follow. */
	
	if (debug > 3)
		dprintf("%s: Close called. Shutting down card.\n", dev->name);
	
	writel(0x0000, cardCtlBaseAddr + InterruptEnable);	
	writew(0x0002, cardCtlBaseAddr + RxCmd);
	writew(0x0002, cardCtlBaseAddr + TxCmd);
	writeb(0x0000, cardCtlBaseAddr + LEDCtrl);
	
	return B_OK;
}

static status_t hamachi_free(void* cookie)
{
	DEVICETYPE* dev = (DEVICETYPE*)cookie;	
	
	if (debug > 3)
		dprintf("%s: Free called. Doing some clean up...\n", dev->name);
	
    if (remove_io_interrupt_handler(dev->irq, hamachi_interrupt, dev) !=
														B_NO_ERROR)
		dprintf("%s: Unable to remove interrupt handler.\n", dev->name);;

	delete_area(dev->descAreaID);
	delete_area(dev->rxAreaID);
	delete_area(dev->txAreaID);
	
	free(dev);
	
	return B_OK;
}

/****************** Kernel Hookup *******************/

int32	api_version = B_CUR_DRIVER_API_VERSION;

static device_hooks device_hooks_struct = {
	hamachi_open, 			/* -> open entry point */
	hamachi_close, 			/* -> close entry point */
	hamachi_free,			/* -> free cookie */
	hamachi_control, 		/* -> control entry point */
	hamachi_read,			/* -> read entry point */
	hamachi_write,			/* -> write entry point */
	hamachi_select,			/* -> start select */
	hamachi_deselect,		/* -> stop select */
	/*device_readv,*/		/* -> scatter-gather read from the device */
	/*device_writev*/		/* -> scatter-gather write to the device */	
};

const char** publish_devices()
{
    g_DevNameList[g_foundCards] = NULL;
	return (const char **)g_DevNameList;
}

device_hooks* find_device(const char* name)
{
	int32 	i;
	char 	*item;
	
	/* Find device name. */
	for( i=0; (item=g_DevNameList[i]); i++)
	{
		if(strcmp(name, item) == 0)
		{
			return &device_hooks_struct;
		}
	}
	
	return NULL; 
}

