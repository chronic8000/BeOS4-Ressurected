#include "pci24.h"
#include "dspcode.h"


void
ClearInterrupt(
    ULONG doClearRequest
);


BOOL
DownloadDSPCode
(
    PDEVICE_CONTEXT pDeviceContext
)
{
    ULONG     tem;
    ULONG     sizeOfCode = sizeof(dspCode)/4;
    DSPMemPtr pBase = pDeviceContext->pDeviceBase;  //alias the regs for typeing ease.
    ULONG     dspControlReg = 0x00000900;       // This sets up the DSP for ***24-bit transfers Left-Justified***
    ULONG     dspStatusReg = 0;

    ASSERT( pDeviceContext );

    TakeListLock( pDeviceContext );
    
    // Reset the DSP.
    // This is needed in case of a warm boot. If the DSP code
    // is already downloaded and running, this will put it in
    // the proper state. If the DSP code hasn't been downloaded
    // yet, this is a harmless command.
//    SendHostCommand( kHstCmdReset, pBase );
    
    // wait until the DSP says it's ready (or times out).
    WaitOnStatus( pBase );
    
    // Set up 24-bit transfers (LS 24-bits of a 23-bit word = program word)
    pBase->HostCTR = dspControlReg;
    
    WaitOnStatus( pBase );

    // Tell the DSP how many words are coming its way
    pBase->HostRXS = sizeOfCode;
    
    WaitOnStatus( pBase );
    
    // Tell the DSP the bootload start address
    pBase->HostRXS = 0x0;
    
    WaitOnStatus( pBase );
    
    // download the code
    for(tem=0; tem < sizeOfCode; tem++)
    {
        pBase->HostRXS = dspCode[tem];
        WaitOnStatus( pBase );
    }
    DPF( 3, "Download done" );
    
    // The entire host program has been sent, now read the HostSTR
    dspControlReg = pBase->HostCTR;
    dspControlReg = dspControlReg | kHF0;
    pBase->HostCTR = dspControlReg;

    dspStatusReg = pBase->HostSTR;
    DPF1(3, "Status reg = %X\n", dspStatusReg );
    WaitOnStatus( pBase );
    
    DPF(3,  "Turn off interrupts if they are on" );
    SendHostCommand( kHstCmdXmitDisable, pBase );
    WaitOnStatus( pBase );

    SendHostCommand( kHstCmdRecvDisable, pBase );
    WaitOnStatus( pBase );
    
    // set DSP buffer size to 0x1000
    DPF(3,  "Setting 0x1000 buffer size" );
	SendHostCommand( kHstCmdBuffer1000, pBase );
    WaitOnStatus( pBase );

    // set DSP's transmit sample rate to 44k 
    DPF(3,  "Setting 44k output rate" );
    SendHostCommand( kHstCmdXmt44, pBase );
    WaitOnStatus( pBase );

    // set DSP's receiver sample rate to 44k
    DPF(3,  "Setting 44k input rate" );
    SendHostCommand( kHstCmdRcv44, pBase );
    WaitOnStatus( pBase );
    
//    // turn on playthru (monitoring) for testing
    DPF(3,  "Setting monitoring OFF" );
    SendHostCommand( kHstCmdPlaythruDisable, pBase );
    WaitOnStatus( pBase );
    

    DPF(3,  "Done setting up DSP" );
    
    ReleaseListLock( pDeviceContext );
    
    return TRUE;
}




void 
CopyDataToDSP
(
PDEVICE_CONTEXT pDeviceContext
)
{
    DSPMemPtr pBase = pDeviceContext->pDeviceBase;  //alias the regs for typing ease.

    // How many samples do we need to download
    // to the dSP?  
    ULONG samplesNeeded = 0x1000;

    // Untill we are done.  
    while( samplesNeeded )
    {
        PUCHAR  pData;
        ULONG   dataSize;
        
        // get the buffer we're currently working on.
        WaveOutListGetWorkingData( pDeviceContext, &pData, &dataSize );

        // Is there a buffer in the queue?      
        if( pData )
        {
            ULONG samplesToCopy;
            ULONG tem;
            
            // yeah.. how many bytes are left to play in it?
            // Convert byes to samples.
            // TODO: Get the bitness of the data from the header,
            // and make this function bit-depth independent.
            ULONG samplesHave = dataSize / 2;

            // How many samples do we copy out of this buffer...
            samplesToCopy = min( samplesNeeded, samplesHave );
            

            // Copy the data out of this buffer.            
            for(tem = 0; tem < samplesToCopy; tem++ )
            {
                ULONG sample;

                // TODO: Fix this copy code not to be hard-coded for 16-bits.
                // TODO: Get Nathan to fix the DSP code so I don't have to swab the bytes.
                
                // Get the hi-byte out of the sample
                sample = *(pData + 1);
                sample <<= 8;
                
                // Get the low byte out of the sample
                sample |= *(pData);
                
                // Make it look like 32-bit data
                // TODO: no hard coded stuff
                sample <<= 8;
                
                // get to the next sample
                pData += 2;

//				sample = 0x010203;

                // Wait on the device...                
                WaitOnStatus( pBase );
                // write the sample             
                pBase->HostTXR = sample;

            }
            
            // Update the number of samples we've done so far..
            samplesNeeded -= samplesToCopy;
            
            // Tell the list how much data we used
            WaveOutListUpdateCopyPosition( pDeviceContext, samplesToCopy * 2);
            

            // Do our getPosition hack.         
            pDeviceContext->bytesCopied += samplesToCopy * 2;
            
            // Keep going on the next buffer, or maybe we're done
            // for this interrupt...
        
        } else {
            // We have no more buffers of app data to write..
            // either we are starved for data, or the app has
            // finished sending all the data and is waiting
            // for it to play out... either way...
            ULONG tem;

            // Fill the buffer with quite...
           for(tem=0; tem < samplesNeeded; tem ++ )
           {
               WaitOnStatus( pBase );
               pBase->HostTXR = 0x0;
           }
            
            // we're done...
            samplesNeeded = 0;
        }
    }
    
        
}








//
// Called from the OS when
// an interrupt happens.
//
ULONG
IRQHandler
(
    ULONG IRQHandle,
    ULONG VMHandle,
    PDEVICE_CONTEXT pDeviceContext
)
{
    DSPMemPtr pBase = pDeviceContext->pDeviceBase;
    ULONG returnValue = FALSE;
    ULONG DspStatus = pBase->HostSTR;

    ASSERT( !pDeviceContext->inIsr );
    
    pDeviceContext->inIsr = TRUE;
    
    
    // The the interrupt come from out DSP?
    if( DspStatus & kHINT )
    {
        // yes
        // Is this a record interrupt?
        if( DspStatus & kRecReq )
        {
            // TODO: Record...
            DPF( 0, "record interrupt");
        }
        // Is this a playback interrupt?
        if( DspStatus & kPlayReq )
        {
            ULONG numToComplete;
            
            CopyDataToDSP( pDeviceContext );
            
            // did we have any to complete?
            numToComplete = WaveOutListUpdateInterruptCompleteCount( pDeviceContext );
            
            if( numToComplete )
            {
                DoFinishedCallBacks( pDeviceContext );
            } else {
                // no.. then we didn't call ring3,
                // which means we haven't EOI'd the pic.
                // Do it now.
                ClearInterrupt( 0 );
            }
            
        }
        returnValue = TRUE;
    }
#ifdef DEBUG
    else {
        DPF(0,"bogus random interrupt");
    }
#endif  
    
    pDeviceContext->inIsr = FALSE;
    
    ASSERT( !pDeviceContext->inIsr );
    
    return returnValue;
}




// Clear the interrupt
// w/ a flag to request a clear_int_request
void
ClearInterrupt(
    ULONG doClearRequest
)
{
    PDEVICE_CONTEXT pDeviceContext = GetDeviceContext();
    
    // Does the caller want to clear a standing int request?
    if( doClearRequest )
    {
        // do it
        VPICD_Clear_Int_Request( pDeviceContext->VPICDHandle, pDeviceContext->clientVM );
    }
    // Wiat on the device.
    WaitOnStatus( pDeviceContext->pDeviceBase );
    // send it.
    SendHostCommand( kHstIntAckPlay, pDeviceContext->pDeviceBase );
    // EOI the pic.
    VPICD_Phys_EOI( pDeviceContext->VPICDHandle );
}   




//
// Spin on the status bit until it's set.
// (Meaning the device is ready for more goo.
//
ULONG
WaitOnStatus
(
    DSPMemPtr pBaseAddress
)
{
    // Pick a timeout value
    // TODO: Tune value too machine.
    ULONG timeoutValue = 100000;
    ULONG IOValue;
    
    do
    {
        // Read the registers
        IOValue = pBaseAddress->HostSTR;
        timeoutValue--;
        // Is the bit set?
    } while( ((IOValue & kHTRQ) != kHTRQ ) && timeoutValue );
    // then we're done.

    ASSERT( timeoutValue || !"Timeout waitOnStatus" );
    return IOValue;
}   





//
// Send Host Command to DSP
//
BOOL
SendHostCommand
(
    ULONG command, 
    DSPMemPtr pBaseAddress
)
{
    // Pick a timeout value
    // TODO: Tune value too machine.
    ULONG timeoutValue = 10000000;
    ULONG IOValue;
    
    do{
        IOValue = pBaseAddress->HostCVR;
        timeoutValue--;
    } while( (IOValue & 0x1) && timeoutValue );
    
    ASSERT( timeoutValue || !"Timeout SendHostCommand" );
    
    // Write the command even if we did timeout
    pBaseAddress->HostCVR = command;

   
    return timeoutValue;
}


