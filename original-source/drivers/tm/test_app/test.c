#include <stdio.h>
#include <OS.h>
#include "tmmanapi.h"

//extern Int global_file_descriptor_sdram;
//extern Int global_file_descriptor_mmio;

/*
function not declared elsewhere
*/
void     Init_TMMan_API_Lib	();
TMStatus tmmEventWait		(UInt32 EventHandle);
TMStatus tmmMessageWait		(UInt32 MessageHandle);



int main(void)
{
  UInt32          DSPNumber = 0;
  UInt32          DSPHandle = 0;
  UInt32          nrOfDsps  = 0;
  tmmanDSPInfo    DSPInfo;
  tmmanVersion    dummy_tmmanVersion;
  TMStatus        tmStatus;
  UInt32          i;
  unsigned char * buffer;
  unsigned long * SDRAM_buffer;
  UInt8*          FirstHalfPtr      = Null;
  UInt32          FirstHalfSize;
  UInt8*          SecondHalfPtr     = Null;
  UInt32          SecondHalfSize;
  /*
  initialize to 0 to avoid warning, NOT CHECKED
  */
  UInt32          SynchronizationHandle = 0;
  UInt32          SynchronizationFlags  = 0;
  UInt32          EventToHostHandle = 0;
  UInt32          EventToTMHandle   = 0;
  UInt32          MessageHandle     = 0;
  tmmanPacket     Packet_to_tm;
  tmmanPacket     Packet_to_host;
  UInt32          Address;
  UInt32          SharedMemoryHandle = 0;
  /*
  unused variable
  UInt32          PhysicalAddressPointer = 0;
  */
  UInt32*         addressSharedMemory;
  void *          user_space_SDRAMAddress;
  char*           charSharedMemory;
  Init_TMMan_API_Lib();

 
  printf("test.c: main: tmmanDSPOpen is now going to be called.\n");
/*  global_file_descriptor_sdram = open("/dev/tm0",O_RDWR);
  global_file_descriptor_mmio  = open("/dev/tm1",O_RDWR);
  printf("test.c: main: global_file_descriptor_sdram is 0x%x.\n"
        , global_file_descriptor_sdram);  
  printf("test.c: main: global_file_descriptor_mmio  is 0x%x.\n"
        , global_file_descriptor_mmio);
*/  
  tmStatus = tmmanDSPOpen(DSPNumber, &DSPHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanDSPOpen returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main:tmmanDSPOpen has returned with DSPHandle is 0x%lx.\n"
             ,DSPHandle);
 
  printf("test.c: main:tmmanDSPNegotiateVersion is now going to be called.\n");
  dummy_tmmanVersion.Major=50; // DL: We have to chose them big enough.
  dummy_tmmanVersion.Minor=50;
  tmmanNegotiateVersion(constTMManModuleHostKernel,&dummy_tmmanVersion);
  printf("test.c: main: The version.major is %ld and version.minor is %ld\n"
              , dummy_tmmanVersion.Major, dummy_tmmanVersion.Minor);
  
  printf("test.c: main: tmmanDSPGetInfo is now going to be called.\n");
  if(tmmanDSPGetInfo(DSPHandle,&DSPInfo))
  {
    printf("test.c: main.c: tmmanDSPGetInfo failed.\n");
  }
  user_space_SDRAMAddress = (void *)(DSPInfo.SDRAM.MappedAddress);
  printf("test.c: main: SDRAM.MappedAddress   is 0x%lx\n",DSPInfo.SDRAM.MappedAddress);
  printf("test.c: main: SDRAM.PhysicalAddress is 0x%lx\n",DSPInfo.SDRAM.PhysicalAddress);
  printf("test.c: main: SDRAM.Size            is 0x%lx\n",DSPInfo.SDRAM.Size);
  printf("test.c: main: MMIO.MappedAddress    is 0x%lx\n",DSPInfo.MMIO.MappedAddress);
  printf("test.c: main: MMIO.PhysicalAddress  is 0x%lx\n",DSPInfo.MMIO.PhysicalAddress);
  printf("test.c: main: MMIO.Size             is 0x%lx\n",DSPInfo.MMIO.Size);
  printf("test.c: main: TMClassRevisionID     is 0x%lx\n",DSPInfo.TMClassRevisionID);
  printf("test.c: main: TMSubSystemID         is 0x%lx\n",DSPInfo.TMSubSystemID);
  printf("test.c: main: DSPNumber             is 0x%lx\n",DSPInfo.DSPNumber);
  printf("test.c: main: TMDeviceVendorID      is 0x%lx\n",DSPInfo.TMDeviceVendorID);
  printf("test.c: main: BridgeDeviceVendorID  is 0x%lx\n",DSPInfo.BridgeDeviceVendorID);
  printf("test.c: main: BridgeClassRevisionID is 0x%lx\n",DSPInfo.BridgeClassRevisionID);
  printf("test.c: main: BridgeSubsystemID     is 0x%lx\n",DSPInfo.BridgeSubsystemID);

  printf("test.c: main: tmmanDSPGetNum is now going to be called.\n");
  nrOfDsps = tmmanDSPGetNum();
  printf("test.c: main: tmmanDSPGetNum returned %ld\n",nrOfDsps);
  

  
  printf("test.c: main: tmmanDSPLoad is now going to be called.\n");
  tmStatus = tmmanDSPLoad(DSPHandle, constTMManDefault, "/boot/home/ledtest.out");
  if(tmStatus)
  {
    printf("test.c: main: tmmanDSPLoad returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main: tmmanDSPLoad has returned.\n");

  printf("test.c: main: tmmanEventCreate is now going to be called.\n");
  tmStatus = tmmanEventCreate(DSPHandle
                             ,"ev_to_ho"
                             ,SynchronizationHandle
                             ,SynchronizationFlags
                             ,&EventToHostHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanEventCreate returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main: EventToHostHandle is 0x%lx\n", EventToHostHandle);

  tmStatus = tmmanEventCreate(DSPHandle
                             ,"ev_to_tm"
                             ,SynchronizationHandle
                             ,SynchronizationFlags
                             ,&EventToTMHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanEventCreate returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main: EventToTMHandle is 0x%lx\n", EventToTMHandle);

  tmStatus = tmmanMessageCreate( DSPHandle
                               , "testMsg"
                               , SynchronizationHandle
                               , SynchronizationFlags
                               , &MessageHandle
                               );
  if(tmStatus)
  {
    printf("test.c: main: tmmanMessageCreate returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main: MessageHandle is 0x%lx\n", MessageHandle);
 
  tmStatus = tmmanSharedMemoryCreate(DSPHandle
                                    ,"testshmem"
                                    , 256
                                    , &Address
                                    , &SharedMemoryHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanSharedMemoryCreate returned error 0x%lx\n", tmStatus);
  }
  printf("test.c: main: SharedMemoryHandle is 0x%lx\n", SharedMemoryHandle);
  printf("test.c: main: shared memory:  Address is 0x%lx\n", Address);
 
  printf("test.c: main: testing the mapping of the shared memory\n");
 


  addressSharedMemory = (UInt32 *)Address;
charSharedMemory= (char *) (Address);
/*  printf("test.c: main: addressSharedMemory is 0x%x\n", addressSharedMemory);
  printf("test.c: main: PhysicalAddressPointer is 0x%x\n",PhysicalAddressPointer);
  addressSharedMemory[0]=500;
  addressSharedMemory[1]=600;
  addressSharedMemory[2]=700;
  
  printf("test.c: main: The shared memory contains %d, %d and %d.\n"
        , addressSharedMemory[0]
        , addressSharedMemory[1]
        , addressSharedMemory[2] );
	
  tmmanSharedMemoryGetAddress(SharedMemoryHandle, &PhysicalAddressPointer);
  printf("test.c: main: The PhysicalAddressPointer of the SharedMemory is 0x%x", PhysicalAddressPointer);
 */
  printf("test.c: main: tmmanDSPStart is now going to be called.\n");
    
  SDRAM_buffer = ((unsigned long *)user_space_SDRAMAddress);  
  buffer =  (unsigned char *)(user_space_SDRAMAddress + 0x400000); 

  tmStatus = tmmanDSPStart(DSPHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanDSPStart returned error 0x%lx\n", tmStatus);
  }
  else
  {
    printf("test.c: main: tmmanDSPStart returned successfully\n");
  }
  printf("test.c: main: user_space_SDRAMAddress is 0x%p\n",user_space_SDRAMAddress);

  fflush(Null);   
  printf("test.c: main: waiting for the event ev_to_ho.\n");
  
  tmStatus = tmmEventWait(EventToHostHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmEventWait returned error 0x%lx\n", tmStatus);
  }
  else
  { 
    printf("test.c: main: tmmEventWait returned successfully\n");
  }  
 
   printf("test.c: main: sending the event ev_to_tm.\n");
  tmStatus = tmmanEventSignal(EventToTMHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanEventSignal returned error 0x%lx\n", tmStatus);
  }
  else
  { 
    printf("test.c: main: tmmanEventSignal returned successfully\n");
  }  
 
  //------- message testing ---------------

  Packet_to_tm.Argument[0] = 5;
  Packet_to_tm.Argument[1] = 6;
  Packet_to_tm.Argument[2] = 7;
  
  
  snooze(2000);

  tmStatus = tmmanMessageSend(MessageHandle, &Packet_to_tm);
  if(tmStatus)
  {
    printf("test.c: main: tmmanMessageSend returned error 0x%lx\n", tmStatus);
  }
  else
  { 
    printf("test.c: main: tmmanMessageSend returned successfully\n");
  }  
  printf("test.c: main: tmmMessageWait is now going to be called.\n");
  tmStatus = tmmMessageWait(MessageHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmMessageWait returned error 0x%lx\n", tmStatus);
  }
  else
  { 
    printf("test.c: main: tmmMessageWait returned successfully\n");
  }  
  tmStatus = tmmanMessageReceive(MessageHandle, &Packet_to_host);
  if(tmStatus)
  {
    printf("test.c: main: tmmanMessageReceive returned error 0x%lx\n", tmStatus);
  }
  else
  { 
    printf("test.c: main: tmmanMessageReceive returned successfully\n");
    printf("test.c: main: the receive message contains %ld, %ld and %ld\n",
                         Packet_to_host.Argument[0],
                         Packet_to_host.Argument[1],
                         Packet_to_host.Argument[2]);
  }  



  snooze(3000);
 

  addressSharedMemory = (UInt32 *)Address;
  printf("test.c: main: addressSharedMemory is 0x%p\n", addressSharedMemory);
  printf("test.c: main: The shared memory contains %ld, %ld and %ld.\n"
        , addressSharedMemory[0]
        , addressSharedMemory[1]
        , addressSharedMemory[2] );
  printf("test.c: main: The shared memory contains %s\n",charSharedMemory);
  //tmmanSharedMemoryGetAddress(SharedMemoryHandle, &PhysicalAddressPointer);
  //printf("test.c: main: PhysicalAddressPointer is 0x%x\n",PhysicalAddressPointer);
  addressSharedMemory[0]=500;
  addressSharedMemory[1]=600;
  addressSharedMemory[2]=700;
  
  printf("test.c: main: The shared memory contains %ld, %ld and %ld.\n"
        , addressSharedMemory[0]
        , addressSharedMemory[1]
        , addressSharedMemory[2] );
	
 // tmmanSharedMemoryGetAddress(SharedMemoryHandle, &PhysicalAddressPointer);
  //printf("test.c: main: The PhysicalAddressPointer of the SharedMemory is 0x%x", PhysicalAddressPointer);
 

  //printf("test.c: main: charSharedMemory is %p,\n", charSharedMemory);
  //sprintf(charSharedMemory, "aaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbccccccccccccccc");
  
  printf("test.c: main: Printing test buffer:\n");
 

  for(i=0;i<5;i++)
  {
    printf("address 0x%p has the value 0x%x\n", (buffer+i), buffer[i]);
  }
 
  
  printf("test.c: main: tmmanDebugDPBuffers is now going to be called.\n");
 
  tmStatus = tmmanDebugDPBuffers( DSPHandle
                                , &FirstHalfPtr
                                , &FirstHalfSize
                                , &SecondHalfPtr
                                , &SecondHalfSize);
  if(tmStatus)
  {
    printf("test.c: main: tmmanDebugDPBuffers returned error 0x%lx\n", tmStatus);
  }  

  printf("test.c: main: DP: FirstHalfPtr   is 0x%p\n",FirstHalfPtr  );
  printf("test.c: main: DP: FirstHalfSize  is 0x%lx\n",FirstHalfSize );
  printf("test.c: main: DP: SecondHalfPtr  is 0x%p\n",SecondHalfPtr );
  printf("test.c: main: DP: SecondHalfSize is 0x%lx\n",SecondHalfSize);

  printf("test.c: main: The debugDPbuffer contains: \n");
  if(FirstHalfPtr)
    {
      printf("test.c: DP_BUFFER: %lx %p",FirstHalfSize, FirstHalfPtr);
    }
  else
    {
      if(SecondHalfPtr)
      {
       printf("test.c: DP_BUFFER: %lx %p",SecondHalfSize, SecondHalfPtr);
      }
    }


  /*  printf("test.c: main: tmmanDebugTargetBuffers is now going to be called.\n");
  FirstHalfPtr  = Null;
  SecondHalfPtr = Null;
  tmStatus = tmmanDebugTargetBuffers( DSPHandle
                                    , &FirstHalfPtr
                                    , &FirstHalfSize
                                    , &SecondHalfPtr
                                    , &SecondHalfSize);
  if(tmStatus)
  {
    printf("test.c: main: tmmanDebugTargetBuffers returned error 0x%x\n", tmStatus);
  }  

  printf("test.c: main: Target: FirstHalfPtr   is 0x%x\n",FirstHalfPtr  );
  printf("test.c: main: Target: FirstHalfSize  is 0x%x\n",FirstHalfSize );
  printf("test.c: main: Target: SecondHalfPtr  is 0x%x\n",SecondHalfPtr );
  printf("test.c: main: Target: SecondHalfSize is 0x%x\n",SecondHalfSize);

  printf("test.c: main: The debugTargetbuffer contains: \n");
 if(FirstHalfPtr)
    {
      printf("test.c: DP_BUFFER: %s", FirstHalfPtr);
    }
  else
    {
      if(SecondHalfPtr)
      {
       printf("test.c: DP_BUFFER: %s", SecondHalfPtr);
      }
    }
  */


  printf("test.c: main: tmmanEventDestroy is now going to be called.\n");
 
  if(EventToHostHandle != 0x0)
  { 
    tmStatus = tmmanEventDestroy(EventToHostHandle);
    if(tmStatus)
    {
      printf("test.c: main: tmmanEventDestroy returned error 0x%lx\n", tmStatus);
    }  
    else
    {
      printf("test.c: main: tmmanEventDestroy returned successfully.\n");
    }
  }

  if(EventToTMHandle != 0x0)
  {
    tmStatus = tmmanEventDestroy(EventToTMHandle);
    if(tmStatus)
    {
      printf("test.c: main: tmmanEventDestroy returned error 0x%lx\n", tmStatus);
    }  
    else
    {
      printf("test.c: main: tmmanEventDestroy returned successfully.\n");
    }
  }
  if(MessageHandle != 0x0)
  {
    tmStatus = tmmanMessageDestroy(MessageHandle);
    if(tmStatus)
    {
      printf("test.c: main: tmmanMessageDestroy returned error 0x%lx\n", tmStatus);
    }  
    else
    {
      printf("test.c: main: tmmanMessageDestroy returned successfully.\n");
    }
  }
  if(SharedMemoryHandle != 0x0)   
  { 
     tmStatus = tmmanSharedMemoryDestroy(SharedMemoryHandle);
     if(tmStatus)
     {
       printf("test.c: main: tmmanSharedMemoryDestroy returned error 0x%lx\n", tmStatus);
     }  
     else
     {
       printf("test.c: main: tmmanSharedMemoryDestroy returned successfully.\n");
     }   
  }




  printf("test.c: main: tmmanDSPStop is now going to be called.\n");
  tmStatus = tmmanDSPStop(DSPHandle);
  if(tmStatus)
  {
    printf("test.c: main: tmmanDSPStop returned error 0x%lx\n", tmStatus);
  }
    fflush(Null);
  snooze(1000000);
  return 0;
}












