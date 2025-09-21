//============================================================================
//
// DHWIFACE.H - Definitions for the DOSHWInterface object.
//
// Copyright (c) 1996-1998 Crystal Semiconductor Corp.
//
//============================================================================

#ifndef _H_DOSHWIFACE
#define _H_DOSHWIFACE

//============================================================================
//
// DOSHWInterface is a representation of a particular CS461x part.  It contains
// all the code involved in direct access of the part, including host
// initiated DMA.
//
//============================================================================
class DOSHWInterface
{
    private:
        //
        // ulCLKCR1, ulCLKCR2, and ulPLLM, ulPLCC are the current values of the
        // CS461x clock control registers, as far as the device driver is
        // concerned.  This is used to keep track of the current value (so we
        // don't have to do a read/modify/write when changing values) and so
        // that we know the correct value to program it with when doing an APM
        // resume.
        //
        Ulong ulCLKCR1;
        Ulong ulCLKCR2;
        Ulong ulPLLM;
        Ulong ulPLLCC;

        //
        // ulSERMC1, ulSERMC2, ulSERC1, ulSERC2, ulSERC3, ulSERC4, and ulSERC5
        // are the current values of the CS461x serial port configuration
        // registers, as far as the device driver is concerned.  This is used
        // so we know the correct value to program it with when doing an APM
        // resume.
        //
        Ulong ulSERMC1;
//        Ulong ulSERMC2;
        Ulong ulSERC1;
        Ulong ulSERC2;
//        Ulong ulSERC3;
//        Ulong ulSERC4;
//        Ulong ulSERC5;

        //
        // ulHISR is the value of the host interrupt status register that was
        // read by the interrupt service routine.
        //
//        Ulong ulHISR;

        Ulong ulBaseAddress0;
        Ulong ulBaseAddress1;

        // ulPCTL for storing original Play Control Register contents
        // ulCCTL for storing original Capture Control Register contents
        Ulong ulPCTL;
        Ulong ulCCTL;

        // Setup CS461x registers to read from a AC97 codec, calls PokeBA0 which
        // actually does the necessary register writes in order to read from 
        // the AC97 codec. 
        // 
        CWRET ReadAC97(Ulong ulOffset, Ulong *pulValue);

        // Setup CS461x registers to write to a AC97 codec, calls PokeBA0 which
        // actually does the necessary register writes. 
        // 
        CWRET WriteAC97(Ulong ulOffset, Ulong ulValue);

    public:
    
        //
        // Constructor.
        //
        DOSHWInterface();
        
        //
        // Destructor.
        //
        ~DOSHWInterface();

        //
        // Initialize performs the real object setup, since we can fail a
        // member function, but not the constructor.
        //
        CWRET Initialize(Ulong pulBaseAddr0, Ulong pulBaseAddr1);

        //
        // ClearSerialFIFOs fills the serial FIFOs with silence.  This is
        // called by the interrupt service routine so it can not block.
        //
        CWRET ClearSerialFIFOs(void);

        //
        // Peek reads a word from the specified location in the CS461x's
        // address space (based on the part's base address one register).
        //
        CWRET Peek(Ulong ulOffset, Ulong *pulValue);

        //
        // Poke write a word to the specified location in the CS461x's address
        // space (based on the part's base address one register).
        //
        CWRET Poke(Ulong ulOffset, Ulong ulValue);

        //
        // PeekBA0 reads a word from the specified location in the CS461x's
        // address space (based on the part's base address zero register).
        //
        CWRET PeekBA0(Ulong ulOffset, Ulong *pulValue);

        //
        // PokeBA0 write a word to the specified location in the CS461x's
        // address space (based on the part's base address zero register).
        //
        CWRET PokeBA0(Ulong ulOffset, Ulong ulValue);

        //
        // DoTransfer performs a DMA transfer using the CS461x's host initiated
        // DMA channel.  Control returns immediately after the DMA is started;
        // execution may be delayed while waiting for a running DMA to complete
        // before the new DMA is initiated.
        //
        CWRET DoTransfer(Ulong __far * ulSrc, Ulong ulDst, Ulong ulCount);

        //
        // AddDACUser increments the number of users of the DACs, and powers
        // on the DACs if necessary.
        //
        void AddDACUser(void);

        //
        // AddADCUser increments the number of users of the ADCs, and powers
        // on the ADCs if necessary.
        //
        void AddADCUser(void);

        // Start begins execution of the processor.
        CWRET ProcStart(void);

        // Resets of the processor.
        CWRET ProcReset(void);

        //
        // DownloadImage copies the task image stored in this object to the
        // specified address.
        //
        CWRET DownloadImage();

        //
        // SetPlaySampleRate initializes a sample rate conversion block
        //
        CWRET SetPlaySampleRate(Ulong ulInRate);

        //
        // StopPlayDMA stops play DMA 
        //
        void StopPlayDMA(void);

        //
        // StartPlayDMA starts play DMA 
        //
        void StartPlayDMA(void);

        //
        // SetCaptureSampleRate initializess the decimating sample rate converter
        //
        CWRET SetCaptureSampleRate(Ulong ulOutRate);

        //
        // StopCaptureDMA stops play DMA 
        //
        void StopCaptureDMA(void);

        //
        // StartCaptureDMA starts play DMA 
        //
        void StartCaptureDMA(void);

//        int PICInterruptEnableBitUtility( int hw_interrupt, int mode );

//        void   (__interrupt __far LooptestIRQServiceHandler)(void);



};

#endif // _H_DOSHWIFACE
