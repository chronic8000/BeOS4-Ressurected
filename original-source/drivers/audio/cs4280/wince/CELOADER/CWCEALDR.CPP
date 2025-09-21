#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <i86.h>
#include "hwdefs.h"
#include "dhwiface.h"
#include "pcibios.h"
#include "pcipnp.h"
#include "cwcealdr.h"

DWORD fGlobalInterruptFlag;
BYTE * pbOrgPlayBufferAddress;
BYTE * pbPlayDataBufferAddress;
DWORD pbPlayDataBufferPhysicalAddress;

//===============================================================
//===============================================================
//===============================================================
//      MAIN
//===============================================================
//===============================================================
//===============================================================
int main(int argc, char ** argv)
{
  CSDevice *pInstance;

  unsigned long int err_code = NO_ERROR;
  long int ulCaptureSampleRate = 48000;
  long int ulPlaySampleRate = 48000;

  fGlobalInterruptFlag = 0;
  
  if (argc > 1)
  {
    ulCaptureSampleRate = atol(argv[1]);
  }
  if (argc > 2)
  {
    ulPlaySampleRate = atol(argv[2]);
  }
  
  
  //***************************************************************  
  //***************************************************************  
  //    SOFTWARE INITIALIZATION....
  //        FIND THE PART
  //        FIND THE BASE ADDRESSES
  //        FIND THE INTERRUPT
  //        SETUPP CONFIG SPACE REGISTERS
  //***************************************************************  
  //***************************************************************  
    //        
    // Create a new instance of this class.        
    //        
    if ((pInstance = new CSDevice()) == NULL)
    {
        return(ERROR);
    }

    //
    // Create a new instance of the PCI class used for examining the
    // PCI configuration of the part.
    //
    if ((pInstance->m_pPCIDevice = PciPnP::CreateInstance( 
            DEVID_CS4612,
            VENID_CIRRUS_LOGIC)) == NULL)
    {
        return(ERROR);
    }

    //
    // Create a Hardware Interface class.
    //
    pInstance->m_pHw = new DOSHWInterface();

    //
    // Set up the Pci configuration Registers for Memory access and 
    // Bus Master Access.
    //
    if(!pInstance->m_pPCIDevice->MemoryAccessEnable())
    {
        pInstance->m_pPCIDevice->SetMemoryAccess(TRUE);
    }

    if(!pInstance->m_pPCIDevice->BusMasterEnable())
    {
        pInstance->m_pPCIDevice->SetBusMaster(TRUE);        
    }

    //
    // Save the BA0/BA1 address for easy use later.
    //
    pInstance->m_ulPhysicalBA0 = pInstance->m_pPCIDevice->PhysicalAddressBA0();
    pInstance->m_ulPhysicalBA1 = pInstance->m_pPCIDevice->PhysicalAddressBA1();
    
    //
    // If we don't get non-zero values for these addresses then we need
    // to fail and exit.
    //
    if ((pInstance->m_ulPhysicalBA0 == 0) || (pInstance->m_ulPhysicalBA0 == 0))
    {
        pInstance->m_pPCIDevice->RestoreConfigSpaceRegs();
        delete pInstance;
        return(ERROR);
    }

  //***************************************************************  
  //***************************************************************  
  //    HARDWARE INITIALIZATION....
  //        SET UP CLOCKS
  //        LOAD PROCESSOR IMAGE
  //        ALLOCATE AUDIO BUFFERS
  //            UPDATE HARDWARE REGISTERS
  //***************************************************************  
  //***************************************************************  

    // Initialize the Crystal chip
    if ( pInstance->m_pHw->Initialize(pInstance->m_ulPhysicalBA0, pInstance->m_ulPhysicalBA1) )
    {
        printf("\n  failed during HW Initialize().");
        delete pInstance;
        return(ERROR);
    }

    // Reset the Processor.
    pInstance->m_pHw->ProcReset();

    // Download the Processor Image to the processor.
    if ( pInstance->m_pHw->DownloadImage() )
    {
        printf("\n  failed during DownloadImage().");
        delete pInstance;
        return(ERROR);
    }

    // Stop Play DMA.
    pInstance->m_pHw->StopPlayDMA();

    // Stop Capture DMA.
    pInstance->m_pHw->StopCaptureDMA();

    // Turn on the software clock enable flag in the clock control
    // register.
    //
//    if (pInstance->m_pHw->PeekBA0(BA0_CLKCR1, &ulTemp))
//    {
//        printf("\n  failed Peek().");
//        delete pInstance;
//        return(ERROR);
//    }
//    ulTemp |= CLKCR1_SWCE;
//    if (pInstance->m_pHw->PokeBA0(BA0_CLKCR1, ulTemp))
//    {
//        printf("\n  failed Poke().");
//        delete pInstance;
//        return(ERROR);
//    }

    // Crank up the power on the DAC and ADC
    pInstance->m_pHw->AddDACUser();
    pInstance->m_pHw->AddADCUser();

    // allocate audio buffers
    if (AllocateAudioBuffers(pInstance))
    {
        printf("\n  failed during AllocateAudioBuffers().");
        delete pInstance;
        return(ERROR);
    }

    // setup the play sample rate
    if (pInstance->m_pHw->SetPlaySampleRate(ulPlaySampleRate))
    {
        printf("\n  failed during SetPlaySampleRate(rate).");
        free((void *)pbOrgPlayBufferAddress);
        delete pInstance;
        return(ERROR);
    }

    // setup the capture sample rate
    if (pInstance->m_pHw->SetCaptureSampleRate(ulCaptureSampleRate))
    {
        printf("\n  failed during SetCaptureSampleRate(rate).");
        free((void *)pbOrgPlayBufferAddress);
        delete pInstance;
        return(ERROR);
    }
  
  
  //***************************************************************  
  //***************************************************************  
  //    START THE PROCESSOR
  //***************************************************************  
  //***************************************************************  
    // Start the CSDevice processor
    if ( pInstance->m_pHw->ProcStart() )
    {
        printf("\n  failed during HW ProcStart().");
        free((void *)pbOrgPlayBufferAddress);
        delete pInstance;
        return(ERROR);
    }

  //***************************************************************  
  //***************************************************************  
  //
  //    AC97 MIXER INITIALIZATION....
  //
  //***************************************************************  
  //***************************************************************  

    if ( SetupMixer(pInstance) != NO_ERROR )
    {
        printf("\n  failed during SetupMixer().");
        free((void *)pbOrgPlayBufferAddress);
        delete pInstance;
        return(ERROR);
    }


  //***************************************************************  
  //***************************************************************  
  //
  //    START CAPTURE AND PLAY....
  //
  //***************************************************************  
  //***************************************************************  

    // Start Capture DMA.
    pInstance->m_pHw->StartCaptureDMA();

    // Start Play DMA.
    pInstance->m_pHw->StartPlayDMA();


  //***************************************************************  
  //***************************************************************  
  //
  //    THE END....
  //
  //***************************************************************  
  //***************************************************************  
    // clean up our memory usage
    free((void *)pbOrgPlayBufferAddress);
    delete pInstance;

    printf("\n  THE END.");
    return NO_ERROR;
}


//=============================================================== 
//===============================================================
//=============================================================== 
//      AllocateAudioBuffers
//===============================================================
//=============================================================== 
//===============================================================

BOOL AllocateAudioBuffers(CSDevice *pCSDevice )
{
                // TEST BUFFER

   // Get twice as much memory as needed to more ensure we don't have a 64k crossover.
   pbOrgPlayBufferAddress = (BYTE *)malloc( ( 4096 + 4095 ) );
   if ( pbOrgPlayBufferAddress == NULL )
   {
      return( ERROR );
   }

   pbPlayDataBufferPhysicalAddress = ( (unsigned long)FP_SEG(pbOrgPlayBufferAddress) << 4 ) + FP_OFF(pbOrgPlayBufferAddress);
   pbPlayDataBufferPhysicalAddress += 4095;
   pbPlayDataBufferPhysicalAddress &= 0xFFFFF000;

   pbPlayDataBufferAddress = (BYTE *)MK_FP(pbPlayDataBufferPhysicalAddress >> 4, 0);

    // poke the play buffer address down to the processor
    pCSDevice->m_pHw->Poke( BA1_PBA, pbPlayDataBufferPhysicalAddress );

    // poke the capture buffer address down to the processor
    pCSDevice->m_pHw->Poke( BA1_CBA, pbPlayDataBufferPhysicalAddress + 4096/2 );

   return(NO_ERROR);
} // AllocateAudioBuffers()


//=============================================================== 
//===============================================================
//=============================================================== 
//      SetupMixer()
//===============================================================
//=============================================================== 
//===============================================================
DWORD SetupMixer(CSDevice *pCSDevice)
{
    unsigned long int ulRetErr = NO_ERROR;
    unsigned int StereoVolume; // default is un-muted and full attenuation
    
    char InputLine[32] = "LINE_IN";
    int  InputVolume   =  0;      // no gain 0-0xf : 0-+22.5 dB in 1.5dB steps
    char OutputLine[32] = "LINE_OUT";
    int  OutputVolume = 8;     // 0 = no atten; 3f = 94.5 dB atten
    int  OutputChannel = 3;    // stereo
    
    // INITIALIZE AC97 REGISTERS
    
    //
    // INPUT MUX
    //
    if(strcmp(InputLine, "") != 0)
    {
        if (strcmp(InputLine, "MIC") == 0)
        {
            //
            // Write 0x0000 to 1Ah Record Select to enable the MIC input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, MIC)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "CD") == 0)
        {
            //
            // Write 0x0101 to 1Ah Record Select to enable the CD input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, CD)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "VIDEO") == 0)
        {
            //
            // Write 0x0202 to 1Ah Record Select to enable the VIDEO input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, VIDEO)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "AUX") == 0)
        {
            //
            // Write 0x0303 to 1Ah Record Select to enable the AUX input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, AUX)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "LINE_IN") == 0)
        {
            //
            // Write 0x0404 to 1Ah Record Select to enable the LINE input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, LINE)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "STEREO_MIX") == 0)
        {
            //
            // Write 0x0505 to 1Ah Record Select to enable the STEREO_MIX input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, STEREO_MIX)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "MONO_MIX") == 0)
        {
            //
            // Write 0x0606 to 1Ah Record Select to enable the MONO_MIX input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, MONO_MIX)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(InputLine, "PHONE") == 0)
        {
            //
            // Write 0x0707 to 1Ah Record Select to enable the PHONE input through the mux
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_SELECT, PHONE)) != NO_ERROR)
                return(ulRetErr);
        }

        // Write InputVolume to 1Ch Record gain to un-mute input and adjust after 
        // the mux, muted and 0 db is default
        if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(
                                            BA0_AC97_RECORD_GAIN,
                                            (InputVolume || (InputVolume<<8)))) 
                                            != NO_ERROR)
            return(ulRetErr);

    }
    else // if(strInputLine == ""), mute the record input
    {
        // Write  a 0x8000 to 1Ch Record gain to mute input
        if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_RECORD_GAIN, 0x8000)) != NO_ERROR)
            return(ulRetErr);
    }

    //
    // OUTPUT
    //
    if(strcmp(OutputLine, "") != 0) 
    {
        if(OutputChannel == 1) // left=muted, right=volume
            StereoVolume = 0x3f00 | OutputVolume;
        if(OutputChannel == 2) // left=volume, right=muted
                StereoVolume = (OutputVolume << 8) | 0x003f;
        if(OutputChannel == 3) // left=volume, right=volume
                StereoVolume = (OutputVolume << 8) | OutputVolume;

        // set selected output line to output volume and mute all other outputs
        if (strcmp(OutputLine, "LINE_OUT") == 0)
        {
            //
            // Write OutputVolume to 02h Master Volume to un-mute and adjust atenuation on left and 
            // right.
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0( BA0_AC97_MASTER_VOLUME, StereoVolume)) != NO_ERROR)
                return(ulRetErr);
//            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_HEADPHONE_VOLUME, 0x8000)) != NO_ERROR)
//                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_HEADPHONE_VOLUME, StereoVolume)) != NO_ERROR)
                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME_MONO, 0x8000)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(OutputLine, "ALT_LINE_OUT") == 0)
        {
            //
            // Write a 0x8000 to 04h Alternate Volume to mute and 0db gain on left and
            // right
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME, 0x8000)) != NO_ERROR)
                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0( BA0_AC97_HEADPHONE_VOLUME, StereoVolume)) != NO_ERROR)
                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME_MONO, 0x8000)) != NO_ERROR)
                return(ulRetErr);
        }
        else if (strcmp(OutputLine, "MONO_OUT") == 0)
        {
            //
            // Write a 0x8000 to 06h Master Volume Mono muted and 0db gain 
            //
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME, 0x8000)) != NO_ERROR)
                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_HEADPHONE_VOLUME, 0x8000)) != NO_ERROR)
                return(ulRetErr);
            if ((ulRetErr = pCSDevice->m_pHw->PokeBA0( BA0_AC97_MASTER_VOLUME_MONO, OutputVolume)) != NO_ERROR)
                return(ulRetErr);
        }

    }
    else  // if (OutputLine = 0) then mute all outputs
    {
        if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME, 0x8000)) != NO_ERROR)
            return(ulRetErr);
        if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_HEADPHONE_VOLUME, 0x8000)) != NO_ERROR)
            return(ulRetErr);
        if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MASTER_VOLUME_MONO, 0x8000)) != NO_ERROR)
            return(ulRetErr);
    }

    //
    // Write a 0x8000 to 0Ah PC_BEEP Volume to output mixer muted and 0db gain 
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_PC_BEEP_VOLUME, 0x8000)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    //Write a 0x8008 to 0Ch Phone Volume to output mixer muted and 0db gain 
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_PHONE_VOLUME, 0x8008)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x8008 to 0Eh MIC Volume to input mixer muted and 0db gain 
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_MIC_VOLUME, 0x8008)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x8808 to 10h Line-in Volume to input mixer muted and 0db gain 
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_LINE_IN_VOLUME, 0x8808)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x8808 to 12h CD Volume to input mixer to mute and 0db gain on
    // left and right
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_CD_VOLUME, 0x8808)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x8808 to 14h Video Volume to input mixer to mute and 0db gain
    // on left and right
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_VIDEO_VOLUME, 0x8808)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x8808 to 16h Aux Volume to input mixer to mute and 0db gain on
    // left and right
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_AUX_VOLUME, 0x8808)) != NO_ERROR)
    {
        return(ulRetErr);
    }

    //
    // Write a 0x0000 to 18h PCM out Volume to um-mute and 12db gain on left and
    // right (0x0808 == 0dB)
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_PCM_OUT_VOLUME, 0x0000)) != NO_ERROR)
    {
        return(ulRetErr);
    }
   

    //
    // Write  a 0x0000 to 20h General Purpose Register, 5 input sources mixed 
    // at mono out mixer, mic1 input selected and loopback disabled.
    //
    if ((ulRetErr = pCSDevice->m_pHw->PokeBA0(BA0_AC97_GENERAL_PURPOSE, 0x0000)) != NO_ERROR)
    {
        return(ulRetErr);
    }
 
    return(ulRetErr);

} // SetupMixer()


//===========================================================================
// CSDevice::CSDevice                                         PRIVATE
//---------------------------------------------------------------------------
// Then now non-public contructor, use CreateInstance.
//===========================================================================
CSDevice::CSDevice()
{
    m_pPCIDevice = NULL;    
    m_pHw = NULL;    
}

//===========================================================================
// CSDevice::~CSDevice                                         PUBLIC
//---------------------------------------------------------------------------
// Like a destructor or something.
//===========================================================================
CSDevice::~CSDevice()
{
    if (m_pHw)
    {
        delete m_pHw;   
    }

    if (m_pPCIDevice)
    {
        delete m_pPCIDevice;
    }
}

