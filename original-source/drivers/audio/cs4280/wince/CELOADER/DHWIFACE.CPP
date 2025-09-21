//============================================================================
//
// DHWIFACE.CPP - DOSHWInterface object.
//
// Copyright (c) 1996-1998 Crystal Semiconductor Corp.
//
//============================================================================
#include <stdio.h>
#include "hwdefs.h"
//#include "constant.h"
#include "dhwiface.h"
//#include "defs.h"
#include "delay.h"

extern DWORD fGlobalInterruptFlag;     // defined in cwcealdr.cpp

DOSHWInterface::DOSHWInterface()
{
    ulBaseAddress0 = 0;
    ulBaseAddress1 = 0;
}    
DOSHWInterface::~DOSHWInterface()
{
}    

//****************************************************************************
//
// DOSHWInterface::Initialize performs the real object setup, since we can fail a
// member function, but not the constructor.
//
//****************************************************************************
CWRET
DOSHWInterface::Initialize(Ulong ulBaseAddr0, Ulong ulBaseAddr1)
{
//    ENTRY(4, "DOSHWInterface::Initialize()");
//    ARG("pulBaseAddr0", "0x%08lx", ulBaseAddr0);
//    ARG("pulBaseAddr1", "0x%08lx", ulBaseAddr1);

    Ulong ulCount, ulStatus;

    //
    // Set the new base address.
    //
    ulBaseAddress0 = ulBaseAddr0;
    ulBaseAddress1 = ulBaseAddr1;

    //
    // First, blast the clock control register to zero so that the PLL starts
    // out in a known state, and blast the master serial port control register
    // to zero so that the serial ports also start out in a known state.
    //
    ulCLKCR1 = 0;
    PokeBA0(BA0_CLKCR1, ulCLKCR1);
    ulSERMC1 = 0;
    PokeBA0(BA0_SERMC1, ulSERMC1);

    //
    // If we are in AC97 mode, then we must set the part to a host controlled
    // AC-link.  Otherwise, we won't be able to bring up the link.
    //
    PokeBA0(BA0_SERACC, SERACC_HSP | SERACC_CODEC_TYPE_1_03);     // 0x08
//    PokeBA0(BA0_SERACC, SERACC_HSP | SERACC_CODEC_TYPE_2_0);     // 0x09

    //
    // Drive the ARST# pin low for a minimum of 1uS (as defined in the AC97
    // spec) and then drive it high.  This is done for non AC97 modes since
    // there might be logic external to the CS461x that uses the ARST# line
    // for a reset.
    //
    PokeBA0(BA0_ACCTL, 0);
    delay(100);
    PokeBA0(BA0_ACCTL, ACCTL_RSTN);

    //
    // The first thing we do here is to enable sync generation.  As soon
    // as we start receiving bit clock, we'll start producing the SYNC
    // signal.
    //
    PokeBA0(BA0_ACCTL, ACCTL_ESYN | ACCTL_RSTN);

    //
    // Now wait for a short while to allow the AC97 part to start
    // generating bit clock (so we don't try to start the PLL without an
    // input clock).
    //
    delay(50000);

    //
    // Set the serial port timing configuration, so that
    // the clock control circuit gets its clock from the correct place.
    //
    ulSERMC1 = SERMC1_PTC_AC97;
    PokeBA0(BA0_SERMC1, ulSERMC1);

    //
    // Write the selected clock control setup to the hardware.  Do not turn on
    // SWCE yet (if requested), so that the devices clocked by the output of
    // PLL are not clocked until the PLL is stable.
    //
    ulPLLCC = PLLCC_LPF_1050_2780_KHZ | PLLCC_CDR_73_104_MHZ;
    PokeBA0(BA0_PLLCC, ulPLLCC);
    ulPLLM = 0x3a;
    PokeBA0(BA0_PLLM, ulPLLM);
    ulCLKCR2 = CLKCR2_PDIVS_8;
    PokeBA0(BA0_CLKCR2, ulCLKCR2);

    //
    // Power up the PLL.
    //
    ulCLKCR1 = CLKCR1_PLLP;
    PokeBA0(BA0_CLKCR1, ulCLKCR1);

    //
    // Wait until the PLL has stabilized.
    //
    delay(50000);

    //
    // Turn on clocking of the core so that we can setup the serial ports.
    //
    ulCLKCR1 |= CLKCR1_SWCE;
    PokeBA0(BA0_CLKCR1, ulCLKCR1);


    //
    // Fill the serial port FIFOs with silence.
    //
    if(ClearSerialFIFOs() != CWCSPUD_NOERR)
    {
//        ERROR0("ProgramPart", "HWInterface::ClearSerialFIFOs failed.");
//        RETURN(TM_CALLARGUMENTS, CWCSPUD_HARDWARE_TIMEOUT);
        printf("Initialize() Clear Serial FIFOs failed.");
        return (CWCSPUD_HARDWARE_TIMEOUT);
    }

    //
    // Set the serial port FIFO pointer to the first sample in the FIFO.
    //
//    PokeBA0(BA0_SERBSP, 0);

    //
    // Write the serial port configuration to the part.  The master
    // enable bit is not set until all other values have been written.
    //
    ulSERC1 = SERC1_SO1F_AC97 | SERC1_SO1EN;
    PokeBA0(BA0_SERC1, ulSERC1);
    ulSERC2 = SERC2_SI1F_AC97 | SERC1_SO1EN;
    PokeBA0(BA0_SERC2, ulSERC2);
    ulSERMC1 = SERMC1_PTC_AC97 | SERMC1_MSPE;
    PokeBA0(BA0_SERMC1, ulSERMC1);

    //
    // Wait for the codec ready signal from the AC97 codec.
    //
    for(ulCount = 0; ulCount < 1000; ulCount++)
    {
        //
        // First, lets wait a short while to let things settle out a bit,
        // and to prevent retrying the read too quickly.
        //
        delay(125);

        //
        // Read the AC97 status register to see if we've seen a CODEC READY
        // signal from the AC97 codec.
        //
        PeekBA0(BA0_ACSTS, &ulStatus);
        if(ulStatus & ACSTS_CRDY)
        {
            break;
        }
    }

    //
    // Make sure we sampled CODEC READY.
    //
    if(!(ulStatus & ACSTS_CRDY))
    {
        printf("Initialize() Never read Codec Ready from AC97.");
//        RETURN(TM_CALLARGUMENTS, CWCSPUD_HARDWARE_TIMEOUT);
        return (CWCSPUD_HARDWARE_TIMEOUT);
    }

    //
    // Assert the vaid frame signal so that we can start sending commands
    // to the AC97 codec.
    //
    PokeBA0(BA0_ACCTL, ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);

    //
    // Wait until we've sampled input slots 3 and 4 as valid, meaning that
    // the codec is pumping ADC data across the AC-link.
    //
    for(ulCount = 0; ulCount < 1000; ulCount++)
    {
        //
        // First, lets wait a short while to let things settle out a bit,
        // and to prevent retrying the read too quickly.
        //
//        delay(10000000L);      //clw
        delay(1000);
        //
        // Read the input slot valid register and see if input slots 3 and
        // 4 are valid yet.
        //
        PeekBA0(BA0_ACISV, &ulStatus);
        if((ulStatus & (ACISV_ISV3 | ACISV_ISV4)) ==
           (ACISV_ISV3 | ACISV_ISV4))
        {
            break;
        }
    }

    //
    // Make sure we sampled valid input slots 3 and 4.  If not, then return
    // an error.
    //
    if((ulStatus & (ACISV_ISV3 | ACISV_ISV4)) != (ACISV_ISV3 | ACISV_ISV4))
    {
//        ERROR0("ProgramPart", "Never sampled ISV3 & ISV4 from AC97.");
//        RETURN(TM_CALLARGUMENTS, CWCSPUD_HARDWARE_TIMEOUT);
        printf("Initialize() Never sampled ISV3 & ISV4 from AC97.");
        return (CWCSPUD_HARDWARE_TIMEOUT);
    }

    //
    // Now, assert valid frame and the slot 3 and 4 valid bits.  This will
    // commense the transfer of digital audio data to the AC97 codec.
    //
    PokeBA0(BA0_ACOSV, ACOSV_SLV3 | ACOSV_SLV4);

    //
    // Power down the DAC and ADC.  We will power them up (if) when we need
    // them.
    //
//    PokeBA0(BA0_AC97_POWERDOWN, 0x300);

    //
    // Turn off the Processor by turning off the software clock enable flag in 
    // the clock control register.
    //
//    ulCLKCR1 &= ~CLKCR1_SWCE;
//    PokeBA0(BA0_CLKCR1, ulCLKCR1);

    //
    // Enable interrupts on the part.
    //
//    PokeBA0(BA0_HICR, HICR_IEV | HICR_CHGM);

    //
    // Success.
    //
//    RETURN(TM_CALLARGUMENTS, CWCSPUD_NOERR);
    return (CWCSPUD_NOERR);
}

//****************************************************************************
//
// HWInterface::ClearSerialFIFOs fills the serial FIFOs with silence.  This is
// called by the interrupt service routine so it can not block.
//
//****************************************************************************
CWRET
DOSHWInterface::ClearSerialFIFOs(void)
{
    Ulong ulIdx, ulLoop, ulStatus;

//    ENTRY(TM_OBJECTCALLS, "HWInterface::ClearSerialFIFOs()\r\n");

    //
    // See if the devices are powered down.  If so, we must power them up first
    // or they will not respond.
    //
    if(!(ulCLKCR1 & CLKCR1_SWCE))
    {
        PokeBA0(BA0_CLKCR1, ulCLKCR1 | CLKCR1_SWCE);
    }

    //
    // We want to clear out the serial port FIFOs so we don't end up playing
    // whatever random garbage happens to be in them.  We fill the sample FIFOs
    // with zero (silence).
    //
    PokeBA0(BA0_SERBWP, 0);

    //
    // Fill all 256 sample FIFO locations.
    //
    for(ulIdx = 0; ulIdx < 256; ulIdx++)
    {
        //
        // Make sure the previous FIFO write operation has completed.
        //
        for(ulLoop = 0; ulLoop < 5; ulLoop++)
        {
            delay(100);
            PeekBA0(BA0_SERBST, &ulStatus);
            if(!(ulStatus & SERBST_WBSY))
            {
                break;
            }
        }
        if(ulStatus & SERBST_WBSY)
        {
            if(!(ulCLKCR1 & CLKCR1_SWCE))
            {
                PokeBA0(BA0_CLKCR1, ulCLKCR1);
            }
//            ERROR0("ClearSerialFIFOs", "Previous write has not completed.");
            //
            // Ignore this status and contiue, as returning will not allow 
            // Spud to load on windows restart (some machines)
            // RETURN(TM_CALLARGUMENTS, CWCSPUD_HARDWARE_TIMEOUT);
            //
        }

        //
        // Write the serial port FIFO index.
        //
        PokeBA0(BA0_SERBAD, ulIdx);

        //
        // Tell the serial port to load the new value into the FIFO location.
        //
        PokeBA0(BA0_SERBCM, SERBCM_WRC);
    }

    //
    // Now, if we powered up the devices, then power them back down again.
    // This is kinda ugly, but should never happen.
    //
    if(!(ulCLKCR1 & CLKCR1_SWCE))
    {
        PokeBA0(BA0_CLKCR1, ulCLKCR1);
    }

    //
    // Success.
    //
//    RETURN(TM_CALLARGUMENTS, CWCSPUD_NOERR);
    return (CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::Peek reads a word from the specified location in the CS461x's
// address space (based on the part's base address one register).
//
//****************************************************************************
CWRET
DOSHWInterface::Peek(Ulong ulByteOffset, Ulong *pulValue)
{
//    ENTRY(4, "DOSHWInterface::Peek()");
//    ARG("ulOffset", "0x%08lx", ulOffset);

    // Read a dword of BA1 memory
    *pulValue = ((DWORD *)ulBaseAddress1)[ulByteOffset/4];

    // Display the value
//    ARG("Value Read", "0x%08lx", *pulValue);

    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::Poke write a word to the specified location in the CS461x's
// address space (based on the part's base address one register).
//
//****************************************************************************
CWRET
DOSHWInterface::Poke(Ulong ulByteOffset, Ulong ulValue)
{
//    ENTRY(4, "DOSHWInterface::Poke()");
//    ARG("ulOffset", "0x%08lx",ulOffset);
//    ARG("ulValue", "0x%08lx",ulValue);

    // Poke a value of BA1 memory
    ((DWORD *)ulBaseAddress1)[ulByteOffset/4] = ulValue;

    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::PeekBA0 reads a word from the specified location in the
// CS461x's address space (based on the part's base address zero register).
//
//****************************************************************************
CWRET
DOSHWInterface::PeekBA0(Ulong ulByteOffset, Ulong *pulValue)
{

//    ENTRY(4, "DOSHWInterface::PeekBA0()");
//    ARG("ulOffset", "0x%08lx",ulOffset);

    if (ulByteOffset < BA0_AC97_RESET)
    {
        // Read the value of BA0 memory
        *pulValue = ((DWORD *)ulBaseAddress0)[ulByteOffset/4];
    }
    else
    {
        ReadAC97(ulByteOffset, pulValue);
    }

    // Display the value
//    ARG("Value Read", "0x%08lx", *pulValue);

    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::PokeBA0 writes a word to the specified location in the
// CS461x's address space (based on the part's base address zero register).
//
//****************************************************************************
CWRET
DOSHWInterface::PokeBA0(Ulong ulByteOffset, Ulong ulValue)
{
//    ENTRY(4, "DOSHWInterface::PokeBA0()");
//    ARG("ulOffset","0x%08lx", ulOffset);
//    ARG("ulValue", "0x%08lx",ulValue);


    if (ulByteOffset < BA0_AC97_RESET)
    {
        // Write the specified word to the CS461x's memory.
        ((DWORD *)ulBaseAddress0)[ulByteOffset/4] = ulValue;
    }
    else
    {
        WriteAC97(ulByteOffset, ulValue);
    }

    // Success.
    return(CWCSPUD_NOERR);
}


//****************************************************************************
//
// DOSHWInterface::DoTransfer performs a DMA transfer using the CS461x's host
// initiated DMA channel.  Control returns immediately after the DMA is
// started; execution may be delayed while waiting for a running DMA to
// complete before the new DMA is initiated.
//
//****************************************************************************
CWRET
DOSHWInterface::DoTransfer(Ulong __far * fpulSrc, Ulong ulByteDestOffset, Ulong ulByteLength)
{
    DWORD dwByteCounter;

//    ENTRY(4, "DOSHWInterface::DoTransfer()");
//    ARG("fpulSrc", "%Fp",fpulSrc);
//    ARG("ulDst", "0x%08lx",  ulDst);
//    ARG("ulCount", "0x%08lx", ulLength);


    // We do not allow DMAs from host memory to host memory (although the DMA
    // can do it) and we do not allow DMAs which are not a multiple of 4 bytes
    // in size (because that DMA can not do that).  Return an error if either
    // of these conditions exist.
    if(ulByteLength & 0x3)
        return(CWCSPUD_INVALID_DMA);

    /// Check the destination address that it is a multiple of 4
    if(ulByteDestOffset & 0x3)
        return(CWCSPUD_INVALID_DMA);

    // Write the buffer out.
    for(dwByteCounter=0 ; dwByteCounter < ulByteLength ; dwByteCounter+=4)
        ((DWORD *)ulBaseAddress1)[(ulByteDestOffset + dwByteCounter)/4] = fpulSrc[dwByteCounter/4];

    // Success.
    return(CWCSPUD_NOERR);
}


//****************************************************************************
//
// DOSHWInterface::ReadAC97 reads a word from the specified location in the
// CS461x's address space (based on the part's base address zero register).
//
// 1. Write ACCAD = Command Address Register = 46Ch for AC97 register address
// 2. Write ACCDA = Command Data Register = 470h    for data to write to AC97 register, 0 on reads
// 3. Write ACCTL = Control Register = 460h for initiating the write
// 4. Read ACCTL = 460h, DCV should be reset by now and 460h = 17h
// 5. if DCV not cleared, break and return error
// 6. Read ACSTS = Status Register = 464h, check VSTS bit
//****************************************************************************
CWRET
DOSHWInterface::ReadAC97(Ulong ulOffset, Ulong *pulValue)
{
//    ENTRY(4, "DOSHWInterface::ReadAC97()");
//    ARG("ulOffset","0x%08lx", ulOffset);
    Ulong ulCount, ulStatus;
    
    //
    // Make sure that there is not data sitting around from a previous
    // uncompleted access. ACSDA = Status Data Register = 47Ch
    //
    PeekBA0(BA0_ACSDA, &ulStatus);  

    //
    // Setup the AC97 control registers on the CS461x to send the
    // appropriate command to the AC97 to perform the read.
    // ACCAD = Command Address Register = 46Ch
    // ACCDA = Command Data Register = 470h
    // ACCTL = Control Register = 460h
    // set DCV - will clear when process completed
    // set CRW - Read command
    // set VFRM - valid frame enabled
    // set ESYN - ASYNC generation enabled
    // set RSTN - ARST# inactive, AC97 codec not reset
    //
    
    //
    // Get the actual AC97 register from the offset
    //
    PokeBA0(BA0_ACCAD, ulOffset - BA0_AC97_RESET);
    PokeBA0(BA0_ACCDA, 0);                 
    PokeBA0(BA0_ACCTL, ACCTL_DCV | ACCTL_CRW | ACCTL_VFRM |
                       ACCTL_ESYN | ACCTL_RSTN); 

    //
    // Wait for the read to occur.
    //
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
        //
        // First, we want to wait for a short time.
        //
        delay(25);

        //
        // Now, check to see if the read has completed.
        // ACCTL = 460h, DCV should be reset by now and 460h = 17h
        //
        PeekBA0(BA0_ACCTL, &ulStatus);
        if(!(ulStatus & ACCTL_DCV))
        {
            break;
        }
    }

    //
    // Make sure the read completed.
    //
    if(ulStatus & ACCTL_DCV)
    {
        return(1);
    }

    //
    // Wait for the valid status bit to go active.
    //
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
        //
        // Read the AC97 status register.
        // ACSTS = Status Register = 464h
        //
        PeekBA0(BA0_ACSTS, &ulStatus);

        //
        // See if we have valid status.
        // VSTS - Valid Status
        //
        if(ulStatus & ACSTS_VSTS)
        {
            break;
        }

        //
        // Wait for a short while.
        //
        delay(25);
    }

    //
    // Make sure we got valid status.
    //
    if(!(ulStatus & ACSTS_VSTS))
    {
        return(1);
    }

    //
    // Read the data returned from the AC97 register.
    // ACSDA = Status Data Register = 474h 
    //
    PeekBA0(BA0_ACSDA, pulValue); 

//    ARG("ulValue", "0x%08lx",*pulValue);

    //
    // Success.
    //
    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::WriteAC97 writes a word to the specified location in the
// CS461x's address space (based on the part's base address zero register).
//
// 1. Write ACCAD = Command Address Register = 46Ch for AC97 register address
// 2. Write ACCDA = Command Data Register = 470h    for data to write to AC97 register
// 3. Write ACCTL = Control Register = 460h for initiating the write
// 4. Read ACCTL = 460h, DCV should be reset by now and 460h = 07h
// 5. if DCV not cleared, break and return error
//
//****************************************************************************
CWRET
DOSHWInterface::WriteAC97(Ulong ulOffset, Ulong ulValue)
{
//    ENTRY(4, "DOSHWInterface::WriteAC97()");
//    ARG("ulOffset","0x%08lx", ulOffset);  
//    ARG("ulValue", "0x%08lx",ulValue);
    Ulong ulCount, ulStatus;

    // Setup the AC97 control registers on the CS461x to send the
    // appropriate command to the AC97 to perform the read.
    // ACCAD = Command Address Register = 46Ch
    // ACCDA = Command Data Register = 470h
    // ACCTL = Control Register = 460h
    // set DCV - will clear when process completed
    // reset CRW - Write command
    // set VFRM - valid frame enabled
    // set ESYN - ASYNC generation enabled
    // set RSTN - ARST# inactive, AC97 codec not reset
    
    //
    // Get the actual AC97 register from the offset
    //
    PokeBA0(BA0_ACCAD, ulOffset - BA0_AC97_RESET);
    PokeBA0(BA0_ACCDA, ulValue);
    PokeBA0(BA0_ACCTL, ACCTL_DCV | ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);

    //
    // Wait for the write to occur.
    //
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
        //
        // First, we want to wait for a short time.
        //
        delay(25);

        //
        // Now, check to see if the write has completed.
        // ACCTL = 460h, DCV should be reset by now and 460h = 07h
        //
        PeekBA0(BA0_ACCTL, &ulStatus);
        if(!(ulStatus & ACCTL_DCV))
        {
            break;
        }
    }

    //
    // Make sure the write completed.
    //
    if(ulStatus & ACCTL_DCV)
    {
        return(1);
    }

    //
    // Success.
    //
    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::AddDACUser powers on the DAC.
//
//****************************************************************************
void
DOSHWInterface::AddDACUser()
{
    Ulong ulTemp, ulCount;
    //
    // Power on the DACs on the AC97 codec.  We turn off the DAC
    // powerdown bit and write the new value of the power control
    // register.
    //
    PeekBA0(BA0_AC97_POWERDOWN, &ulTemp);
    ulTemp &= 0xFDFF;
    PokeBA0(BA0_AC97_POWERDOWN, ulTemp);

    //
    // Now, we wait until we sample a DAC ready state.
    //
    for(ulCount = 0; ulCount < 32; ulCount++)
    {
        //
        // First, lets wait a short while to let things settle out a
        // bit, and to prevent retrying the read too quickly.
        //
        delay(125);

        //
        // Read the current state of the power control register.
        //
        PeekBA0(BA0_AC97_POWERDOWN, &ulTemp);

        //
        // If the DAC ready state bit is set, then stop waiting.
        //
        if(ulTemp & 0x2)
        {
            break;
        }
    }

    //
    // The DACs are now calibrated, so we can unmute the DAC output.
    //
    PokeBA0(BA0_AC97_PCM_OUT_VOLUME, 0x0808);
}


//****************************************************************************
//
// DOSHWInterface::AddADCUser powers on the ADC.
//
//****************************************************************************
void
DOSHWInterface::AddADCUser()
{
    Ulong ulTemp, ulCount;
    //
    // Power on the DACs on the AC97 codec.  We turn off the DAC
    // powerdown bit and write the new value of the power control
    // register.
    //
    PeekBA0(BA0_AC97_POWERDOWN, &ulTemp);
    ulTemp &= 0xFEFF;
    PokeBA0(BA0_AC97_POWERDOWN, ulTemp);

    //
    // Now, we wait until we sample a DAC ready state.
    //
    for(ulCount = 0; ulCount < 32; ulCount++)
    {
        //
        // First, lets wait a short while to let things settle out a
        // bit, and to prevent retrying the read too quickly.
        //
        delay(125);

        //
        // Read the current state of the power control register.
        //
        PeekBA0(BA0_AC97_POWERDOWN, &ulTemp);

        //
        // If the ADC ready state bit is set, then stop waiting.
        //
        if(ulTemp & 0x1)
        {
            break;
        }
    }
}

//****************************************************************************
//
// DOSHWInterface::ProcReset performs a soft-reset of the DSP.
//
//****************************************************************************
CWRET
DOSHWInterface::ProcReset(void)
{
    Ulong ulIdx;

//    ENTRY(TM_OBJECTCALLS, "DOSHWInterface::Reset()\r\n");

    //
    // Write the reset bit of the SP control register.
    //
    Poke(BA1_SPCR, SPCR_RSTSP);

    //
    // Write the control register.
    //
    Poke(BA1_SPCR, SPCR_DRQEN);

    //
    // Clear the trap registers.
    //
    for(ulIdx = 0; ulIdx < 8; ulIdx++)
    {
        Poke(BA1_DREG, DREG_REGID_TRAP_SELECT + ulIdx);
        Poke(BA1_TWPR, 0xFFFF);
    }

    Poke(BA1_DREG, 0);

    //
    // Set the frame timer to reflect the number of cycles per frame.
    //
    Poke(BA1_FRMT, 0xadf);

    //
    // Success.
    //
    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::ProcStart begins execution of the Processor.
//
//****************************************************************************
CWRET 
DOSHWInterface::ProcStart(void)
{
    Ulong ulTemp, ulCnt;

    //
    // Set the frame timer to reflect the number of cycles per frame.
    //
    Poke(BA1_FRMT, 0xadf);

    //
    // Turn on the run, run at frame, and DMA enable bits in the local copy of
    // the SP control register.
    //
    Poke(BA1_SPCR, SPCR_RUN | SPCR_RUNFR | SPCR_DRQEN);

    //
    // Wait until the run at frame bit resets itself in the SP control
    // register.
    //
    for(ulCnt = 0; ulCnt < 25; ulCnt++)
    {
        //
        // Wait a little bit, so we don't issue PCI reads too frequently.
        //
//        delay(1000);
        delay(125);
        //
        // Fetch the current value of the SP status register.
        //
        Peek(BA1_SPCR, &ulTemp);

        //
        // If the run at frame bit has reset, then stop waiting.
        //
        if(!(ulTemp & SPCR_RUNFR))
        {
            break;
        }
    }

    //
    // If the run at frame bit never reset, then return an error.
    //
    if(ulTemp & SPCR_RUNFR)
    {
        printf("\n  Start(): SPCR_RUNFR never reset.");
        return(CWCSPUD_HARDWARE_TIMEOUT);
    }

    //
    // Success.
    //
    return(CWCSPUD_NOERR);
}


// this is 3*1024 for parameter, 3.5*1024 for sample and 2*3.5*1024 for code since 
// each instruction is 40 bits and takes two dwords 
#define INKY_BA1_DWORD_SIZE (13 * 1024 + 512)
// this is parameter, sample, and code
#define INKY_MEMORY_COUNT 3
struct BA1struct 
{
    struct
    {
        Ulong ulDestByteOffset,
        ulSourceByteSize;
    } MemoryStat[INKY_MEMORY_COUNT];
    
    Ulong BA1Array[INKY_BA1_DWORD_SIZE];
};

#include "cwcimage.h"

//****************************************************************************
//
// DOSHWInterface::DownloadImage copies the task image stored in this object to the
// specified address.
//
//****************************************************************************
CWRET
DOSHWInterface::DownloadImage()
{
    Ulong ulCnt1, ulSourceOffset;
    CWRET ret;

    for (ulCnt1 = 0, ulSourceOffset = 0; ulCnt1 < INKY_MEMORY_COUNT; ulCnt1++)
    {
        
        // DMA this block from host memory to the appropriate
        // memory on the CSDevice.
        if ( DoTransfer((Ulong __far *)(BA1Struct.BA1Array + ulSourceOffset), 
                       BA1Struct.MemoryStat[ulCnt1].ulDestByteOffset, 
                       BA1Struct.MemoryStat[ulCnt1].ulSourceByteSize) != CWCSPUD_NOERR) 
        {
            return(ret);
        }
        ulSourceOffset += BA1Struct.MemoryStat[ulCnt1].ulSourceByteSize/4;
    }
    // Success.
    return(CWCSPUD_NOERR);
}

#define GOF_LENGTH 40
#define GOF_PER_SEC 200

//****************************************************************************
//
// DOSHWInterface::SetSampleRateConverter initializes a sample rate
// conversion block
//
//****************************************************************************
CWRET
DOSHWInterface::SetPlaySampleRate(Ulong ulInRate)
{
    Ulong ulTemp1, ulTemp2;
    Ulong ulPhiIncr;
    Ulong ulCorrectionPerGOF, ulCorrectionPerSec;

    Ulong ulOutRate = 48000;

//    ENTRY(TM_OBJECTCALLS, "DOSHWInterface::SetPlaySampleRate()\r\n");
//    ARG(TM_CALLARGUMENTS, "ulInRate", ulInRate);

    //
    // Compute the values used to drive the actual sample rate conversion.
    // The following formulas are being computed, using inline assembly
    // since we need to use 64 bit arithmetic to compute the values:
    //
    //     ulPhiIncr = floor((Fs,in * 2^26) / Fs,out)
    //     ulCorrectionPerGOF = floor((Fs,in * 2^26 - Fs,out * ulPhiIncr) /
    //                                GOF_PER_SEC)
    //     ulCorrectionPerSec = Fs,in * 2^26 - Fs,out * phiIncr -
    //                          GOF_PER_SEC * ulCorrectionPerGOF
    //
    // i.e.
    //
    //     ulPhiIncr:ulOther = dividend:remainder((Fs,in * 2^26) / Fs,out)
    //     ulCorrectionPerGOF:ulCorrectionPerSec =
    //         dividend:remainder(ulOther / GOF_PER_SEC)
    //
    ulTemp1 = ulInRate << 16;
    ulPhiIncr = ulTemp1 / ulOutRate;
    ulTemp1 -= ulPhiIncr * ulOutRate;
    ulTemp1 <<= 10;
    ulPhiIncr <<= 10;
    ulTemp2 = ulTemp1 / ulOutRate;
    ulPhiIncr += ulTemp2;
    ulTemp1 -= ulTemp2 * ulOutRate;
    ulCorrectionPerGOF = ulTemp1 / GOF_PER_SEC;
    ulTemp1 -= ulCorrectionPerGOF * GOF_PER_SEC;
    ulCorrectionPerSec = ulTemp1;

    //
    // Fill in the SampleRateConverter control block.
    //
    Poke( BA1_PSRC,
          ((ulCorrectionPerSec << 16) & 0xFFFF0000) | (ulCorrectionPerGOF & 0xFFFF));
    Poke( BA1_PPI, ulPhiIncr);

    //
    // Success.
    //
    return( CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::StopPlayDMA stops play DMA
//
//****************************************************************************
void
DOSHWInterface::StopPlayDMA(void)
{
    Ulong ulTemp;

    //
    // Save the current value of the Play Control register.
    //
    Peek(BA1_PCTL, &ulTemp);
    ulPCTL = ulTemp & 0xffff0000;             // save only upper 16 bits

    //
    // Stop Play DMA
    //
    Poke(BA1_PCTL, (ulTemp & 0x0000ffff));  // write 0 to upper 16-bits

    return;
}

//****************************************************************************
//
// DOSHWInterface::StartPlayDMA starts play DMA
//
//****************************************************************************
void
DOSHWInterface::StartPlayDMA(void)
{
    Ulong ulTemp;

    //
    // Get the current value of the Play Control register.
    //
    Peek(BA1_PCTL, &ulTemp);
    ulTemp &= 0x0000ffff;

    //
    // Restore the original value of the Play Control register.
    //
    Poke(BA1_PCTL, (ulPCTL | ulTemp));

    return;
}

//****************************************************************************
//
// DOSHWInterface::SetCaptureSampleRate initializess the decimating sample rate
// converter
//
//****************************************************************************

CWRET
DOSHWInterface::SetCaptureSampleRate(Ulong ulOutRate)
{
    Ulong ulPhiIncr, ulCoeffIncr, ulTemp1, ulTemp2;
    Ulong ulCorrectionPerGOF, ulCorrectionPerSec, ulInitialDelay;

    Ulong dwFrameGroupLength, dwCnt;

    Ulong ulInRate = 48000;

//    ENTRY(TM_OBJECTCALLS, "DOSHWInterface::SetCaptureSampleRate()\r\n");
//    ARG(TM_CALLARGUMENTS, "ulOutRate", ulOutRate);

    //
    // We can only decimate by up to a factor of 1/9th the hardware rate.
    // Return an error if an attempt is made to stray outside that limit.
    //
    if((ulOutRate * 9) < ulInRate)
    {
        printf( "\nSetCaptureSampleRate(): Requested output rate is below 1/9th the input rate.");
        return(CWCSPUD_INVALID_PARAMETER);
    }

    //
    // We can not capture at at rate greater than the Input Rate (48000).
    // Return an error if an attempt is made to stray outside that limit.
    //
    if(ulOutRate > ulInRate)
    {
        printf( "\nSetCaptureSampleRate(): Requested output rate is greater than the input rate.");
        return(CWCSPUD_INVALID_PARAMETER);
    }

    //
    // Compute the values used to drive the actual sample rate conversion.
    // The following formulas are being computed, using inline assembly
    // since we need to use 64 bit arithmetic to compute the values:
    //
    //     ulCoeffIncr = -floor((Fs,out * 2^23) / Fs,in)
    //     ulPhiIncr = floor((Fs,in * 2^26) / Fs,out)
    //     ulCorrectionPerGOF = floor((Fs,in * 2^26 - Fs,out * ulPhiIncr) /
    //                                GOF_PER_SEC)
    //     ulCorrectionPerSec = Fs,in * 2^26 - Fs,out * phiIncr -
    //                          GOF_PER_SEC * ulCorrectionPerGOF
    //     ulInitialDelay = ceil((24 * Fs,in) / Fs,out)
    //
    // i.e.
    //
    //     ulCoeffIncr = neg(dividend((Fs,out * 2^23) / Fs,in))
    //     ulPhiIncr:ulOther = dividend:remainder((Fs,in * 2^26) / Fs,out)
    //     ulCorrectionPerGOF:ulCorrectionPerSec =
    //         dividend:remainder(ulOther / GOF_PER_SEC)
    //     ulInitialDelay = dividend(((24 * Fs,in) + Fs,out - 1) / Fs,out)
    //
    ulTemp1 = ulOutRate << 16;
    ulCoeffIncr = ulTemp1 / ulInRate;
    ulTemp1 -= ulCoeffIncr * ulInRate;
    ulTemp1 <<= 7;
    ulCoeffIncr <<= 7;
    ulCoeffIncr += ulTemp1 / ulInRate;
    ulCoeffIncr ^= 0xFFFFFFFF;
    ulCoeffIncr++;
    ulTemp1 = ulInRate << 16;
    ulPhiIncr = ulTemp1 / ulOutRate;
    ulTemp1 -= ulPhiIncr * ulOutRate;
    ulTemp1 <<= 10;
    ulPhiIncr <<= 10;
    ulTemp2 = ulTemp1 / ulOutRate;
    ulPhiIncr += ulTemp2;
    ulTemp1 -= ulTemp2 * ulOutRate;
    ulCorrectionPerGOF = ulTemp1 / GOF_PER_SEC;
    ulTemp1 -= ulCorrectionPerGOF * GOF_PER_SEC;
    ulCorrectionPerSec = ulTemp1;
    ulInitialDelay = ((ulInRate * 24) + ulOutRate - 1) / ulOutRate;

    //
    // Fill in the VariDecimate control block.
    //
    Poke(BA1_CSRC, 
        ((ulCorrectionPerSec << 16) & 0xFFFF0000) | (ulCorrectionPerGOF & 0xFFFF));

    Poke(BA1_CCI, ulCoeffIncr);

    Poke(BA1_CD, 
        (((BA1_VARIDEC_BUF_1 + (ulInitialDelay << 2)) << 16) & 0xFFFF0000) | 0x80);
    Poke(BA1_CPI, ulPhiIncr);


    //
    // Figure out the frame group length for the write back task.  Basically,
    // this is just the factors of 24000 (2^6*3*5^3) that are not present in
    // the output sample rate.
    //
    dwFrameGroupLength = 1;
    for(dwCnt = 2; dwCnt <= 64; dwCnt *= 2)
    {
        if(((ulOutRate / dwCnt) * dwCnt) !=
           ulOutRate)
        {
            dwFrameGroupLength *= 2;
        }
    }
    if(((ulOutRate / 3) * 3) !=
       ulOutRate)
    {
        dwFrameGroupLength *= 3;
    }
    for(dwCnt = 5; dwCnt <= 125; dwCnt *= 5)
    {
        if(((ulOutRate / dwCnt) * dwCnt) !=
           ulOutRate)
        {
            dwFrameGroupLength *= 5;
        }
    }

    //
    // Fill in the WriteBack control block.
    //
    Poke(BA1_CFG1, dwFrameGroupLength);

    Poke(BA1_CFG2, (0x00800000 | dwFrameGroupLength));

    Poke(BA1_CCST, 0x0000FFFF);

    Poke(BA1_CSPB, ((65536 * ulOutRate) / 24000));

    Poke((BA1_CSPB + 4), 0x0000FFFF);

    //
    // Success.
    //
    return(CWCSPUD_NOERR);
}

//****************************************************************************
//
// DOSHWInterface::StopCaptureDMA stops capture DMA
//
//****************************************************************************
void
DOSHWInterface::StopCaptureDMA(void)
{
    Ulong ulTemp;

    //
    // Save the current value of the Capture Control register.
    //
    Peek(BA1_CCTL, &ulTemp);
    ulCCTL = ulTemp & 0x0000ffff;             // save only lower 16 bits

    //
    // Stop Capture DMA
    //
    Poke(BA1_CCTL, (ulTemp & 0xffff0000));  // write lower 16-bits to 0

    return;
}

//****************************************************************************
//
// DOSHWInterface::StartCaptureDMA starts capture DMA
//
//****************************************************************************
void
DOSHWInterface::StartCaptureDMA(void)
{
    Ulong ulTemp;

    //
    // Get the current value of the Capture Control register.
    //
    Peek(BA1_CCTL, &ulTemp);
    ulTemp &= 0xffff0000;

    //
    // Restore the saved value (lower 16-bits) of the Capture Control register.
    //
    Poke(BA1_CCTL, (ulTemp | ulCCTL));

    return;
}



/**************************************************************************

   NOTE: Use this if you are enabling interrupts.

   __interrupt LooptestIRQServiceHandler() - The interrupt service routine.

   Description:

      If installed as the interrupt service routine, will service the
      interrupt.

      This service routine will determine if the interrupt has occurred due
      to the play count running out, or the capture buffer is full, by looking
      at the CI bit in I24.

      FYI, If an interrupt for play, the PI bit in I24 and INT bit in R2 will
      be set.  Can clear playback by writing a 0 to the PI bit in I24, or
      write to R2.

      If an interrupt for capture, the CI bit and INT bit in R2 will be set.
      Can write to the Status Register (R2) or write a 0 to the CI bit in I24
      to clear it.  Since we are really only concerned about capture, this is
      the only bit we're looking at.

      We are not concerned about an interrupt for play, other than to clear
      the interrupt in the codec so it can release the IRQ line, allowing
      the DMA chip to autoinit and load the play buffer over again.

      In either case, an acknowledge of the end of interrupt will be under-
      taken by sending 0x20 to the 8259 interrupt command port, and to both
      8259 command ports if a cascaded higher-numbered interrupt is the
      current assignment.

      This handler will set a flag if the interrupt is the capture interrupt
      (CI is set), indicating the capture buffer is full/transfer is complete.

      For debugging purposes, a interrupt count variable is available to count
      them.  You can use it for either play or capture interrupt count.

   Parameters: None

   Returns: None

 **************************************************************************/

//void (__interrupt __far LooptestIRQServiceHandler)(void)
//{
    //   WORD AltStatusContents = 0;

    // Read the Interrupt Status Register to clear the interrupt
//    PeekBA0(BA0_HISR, &fCaptureBufferFull);
    // EOI to the PCI part....reenables interrupts
//    PokeBA0(BA0_HICR, HICR_CHGM | HICR_IEV);

/*
   // Get the contents of the Alternate Status Register and check who's interrupt it is.
   _outp( WSSIndexAddressReg, ALT_FEATURE_STATUS );
   AltStatusContents = _inp( WSSIndexedDataReg );

   dwInterruptCount++;

   if ( AltStatusContents & PLAY_INTERRUPT_BIT )
      {
      // Signal EOI for WSS codec to release IRQ line/end interrupt
      _outp( StatusReg, 0 );
      dwPlayIRQCount++;
      }
   else if ( AltStatusContents & CAPTURE_INTERRUPT_BIT )
      {
      // Signal EOI for WSS codec to release IRQ line/end interrupt
      _outp( StatusReg, 0 );
      fCaptureBufferFull = TRUE;
      dwCaptureIRQCount++;
      }
   else
      {
      // Signal EOI for WSS codec to release IRQ line/end interrupt
      _outp( StatusReg, 0 );
      }
*/

   // Signal EOI (end of interrupt) to PIC
//   if ( IrqNumber > 8 )
//      {
       // Send EOI/Acknowlege interrupt for 8259-2 (must do both for higher interrupts)
//      outp(INT_CMD_PORT_8259_2, EOI_8259_COMMAND);
//      }

//   outp(INT_CMD_PORT_8259_1, EOI_8259_COMMAND);    // Send EOI/Acknowlege interrupt for 8259-1

//} // LooptestIRQServiceHandler()

/****************************************************************************

   PICInterruptEnableBitUtility() - Sets up the 8259 Programmable Interrupt
                    Controller (PIC) to enable, disable or an
                    check the status of a specified interrupt
                    enable (IE) bit.

   Description:

      Sets the interrupt bit for a particular interrupt number to the
      ENABLED state (0) or the DISABLED state (1) in the 8259 PIC's IMR
      register.  Preserves the other contents (bits) read from the PIC's
      IMR register.

   Parameters:

      int hw_interrupt  - Hardware interrupt number (IRQ)
      int mode          - DISABLE = 0: Disable the IE bit (set to 1)
              ENABLE  = 1: Enable the IE bit (set to 0)
              QUERY   = 2: Return ENABLED (IRQ bit is 0)
                          DISABLED (IRQ bit is 1)

   Returns: int IRQ_Status  -1 = ERROR
                 0 = DISABLED
                 1 = ENABLED



*****************************************************************************/
/*
int PICInterruptEnableBitUtility( int hw_interrupt, int mode )
{
   int mask;
   int mask_8259;
   int IRQ_Status = -1;


 if ( hw_interrupt < 8 )  // PIC 1
 {
      // Get proper IE bit mask for the IRQ
      switch ( hw_interrupt )
     {
     case 0:
        mask = 0x01;
        break;
     case 1:
        mask = 0x02;
        break;
     case 2:
        mask = 0x04;
        break;
     case 3:
        mask = 0x08;
        break;
     case 4:
        mask = 0x10;
        break;
     case 5:
        mask = 0x20;
        break;
     case 6:
        mask = 0x40;
        break;
     case 7:
        mask = 0x80;
        break;
     default:
        mask = 0x00;
        break;

     } // switch


      if ( mode == ENABLE )
     {
     // Read the PIC IMR to get the IE states
     mask_8259 = inp( MASK_PORT_8259_PIC_1 );

     // Set the proper IE bit to enabled state (0)
     mask_8259 = mask_8259 & (~mask);

     outp( MASK_PORT_8259_PIC_1, mask_8259 );

     } // if ENABLE


      if ( mode == DISABLE )
     {
     // Read the PIC IMR to get the IE states
     mask_8259 = inp( MASK_PORT_8259_PIC_1 );

     // Bitwise OR to set IE bit to disabled (1)
     mask_8259 = mask_8259 | mask;

     outp( MASK_PORT_8259_PIC_1, mask_8259 );

     } // if DISABLE

     // Read the PIC IMR (interrupt mask reg)
     mask_8259 = inp( MASK_PORT_8259_PIC_1 );

     // Check proper bit for true
     mask_8259 &= mask;          // Enabled state of bit is 0 in PIC IMR
     if ( !mask_8259 )           // After bitwise AND, should equal 0 if enabled
        IRQ_Status = ENABLED;
     else
        IRQ_Status = DISABLED;

 } // if hw_interrupt < 8


 if ( hw_interrupt >= 8 )  // PIC 2
 {
      // Get proper IE bit mask for the IRQ
      switch ( hw_interrupt )
     {
     case 8:
        mask = 0x01;
        break;
     case 9:
        mask = 0x02;
        break;
     case 10:
        mask = 0x04;
        break;
     case 11:
        mask = 0x08;
        break;
     case 12:
        mask = 0x10;
        break;
     case 13:
        mask = 0x20;
        break;
     case 14:
        mask = 0x40;
        break;
     case 15:
        mask = 0x80;
        break;
     default:
        mask = 0x00;
        break;

     }  // switch


      if ( mode == ENABLE )
     {
     // Read the PIC IMR to get the IE states
     mask_8259 = inp( MASK_PORT_8259_PIC_2 );

     // Set the proper IE bit to enabled state (0)
     mask_8259 = mask_8259 & (~mask);

     outp( MASK_PORT_8259_PIC_2, mask_8259 );

     } // if ENABLE


     if ( mode == DISABLE )
    {
     // Read the PIC IMR to get the IE states
     mask_8259 = inp( MASK_PORT_8259_PIC_2 );

     // Bitwise OR to set IE bit to disabled (1)
     mask_8259 = mask_8259 | mask;

     outp( MASK_PORT_8259_PIC_2, mask_8259 );

     } // if DISABLE

     // Read the PIC IMR (interrupt mask reg)
     mask_8259 = inp( MASK_PORT_8259_PIC_2 );

     // Check proper bit for true
     mask_8259 &= mask;          // Enabled state of bit is 0 in PIC IMR
     if ( !mask_8259 )           // After bitwise AND, should equal 0 if was enabled
        IRQ_Status = ENABLED;
     else
        IRQ_Status = DISABLED;

      } // if hw_interrupt >= 8
//   if (Debug)
//   {
//        if (IRQ_Status == ENABLED)
//        printf("\nIRQ %d is UNMASKED.\n",hw_interrupt);
//    else printf("\nIRQ %d is MASKED.\n",hw_interrupt);
//   } // if (Debug)
   return IRQ_Status;

} // PICInterruptEnableBitUtility()

*/
