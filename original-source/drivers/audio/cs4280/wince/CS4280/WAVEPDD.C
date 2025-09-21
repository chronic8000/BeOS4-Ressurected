//*****************************************************************************
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//      Copyright (c) 1995-1998  Microsoft Corporation
//
//      Modified to support CS4620  *wb*
//      Modifications Copyright (c) 1998 Crystal Semiconductor Corp.
//
//  Module Name:
//
//      WAVEPDD.C
//
//  Abstract:
//      The PDD (Platform Dependent Driver) is responsible for
//      communicating with the audio circuit to start and stop playback
//      and/or recording and initialize and deinitialize the circuits.
//
//  Functions:
//      PDD_AudioGetInterruptType
//      PDD_AudioMessage
//      PDD_AudioInitialize
//      PDD_AudioDeinitialize
//      PDD_AudioPowerHandler
//      PDD_WaveProc
//
//  Notes:
//    Ver 0.92  5feb99--Changed direct access to READ/WRITE_REGISTER_ULONG
//                      via added Peek/Poke routines to insure multiple
//                      processor support. Changed volume setup to prevent
//                      AC97 clip and control wave volume not in AC97 part.
//                      Moved Start/Stop Play & minor changes for compliance
//                      with "PCI Audio Design Guide for Embedded Systems"
//                      (ver 0.5).
//
//    Ver 0.95  3mar99  Add capture.
//    Ver 0.96  25mar99 Improve capture quality.
//*****************************************************************************

#include <windows.h>
#include <types.h>
#include <memory.h>
#include <excpt.h>
#include <wavepdd.h>
#include <waveddsi.h>
#include <wavedbg.h>
#include <nkintr.h>
#include <oalintr.h>
#include "reg4620.h"
#include "cwcimage.h"
#include <mmreg.h>
#include <ceddk.h>
#include <pc.h>


//*****************************************************************************
//                            ---#defines---
//*****************************************************************************

// Note: The 2.1 ETK sample reserves, in CONFIG.BIB, 2000h(8192T) bytes at
//       0x801fc000(2 Meg - 16384bytes) for 'AUDIOBUF'.  The following
//       defines put the audio play buffer at the start of this area and
//       the record buffer 4k above it, giving 4k for play and 4k for
//       capture, totalling 8k.  An audio "DMA Page" is defined in
//       WAVEPDD.H as 2K bytes in size, half of either the play or
//       capture buffer size.  *wb*

#define AUDIO_DMA_BUFFER_BASE_PA 0x001fc000      // **Must** match 'CONFIG.BIB'-wb

     // Record buffer address =(0x001fc000(2 Meg-16k) + 2 * 2048('WAVEPDD.H')  )
#define RECORD_DMA_BUFFER_BASE (AUDIO_DMA_BUFFER_BASE + 2 * AUDIO_DMA_PAGE_SIZE)


//*****************************************************************************
//                            ---Globals---
//*****************************************************************************
DWORD dbug1, dbug2=0, dbug3=0;    // DEBUG ***** DEBUG ***** DEBUG *** DEBUG ***
DWORD gIntrAudio;                 //  *wb*  From MapIrq2SysIntr(g_Irq) in
                                  //  PDD_AudioInitialize(), extern'ed in WAVEMDD.C

DWORD  *pBA0, *pBA1;              // Linear equivalents of BA*phys.

UCHAR  g_Irq = 0xff;              // 4620 Irq --from registry--

VOID    AudioPowerOn();

BOOL fPInUse, fRInUse;            // Play, record in use.
BOOL g_Playing, g_Recording;      // Play, record have been started.

DWORD StartPlay, StartCapture;    // SP task pointers to start play, capture.

static PWAVEFORMATEX g_pwfx[2];   // [WAPI_IN=0], [WAPI_OUT=1]--(WAVEDDSI.H)
static PCM_TYPE g_pcmtype[2];     // enum PCM_TYPE_M8,M16,S8,S16=0,1,2,3(WAVEPDD.H).


static PBYTE  dma_page[4];   // *wb Array of pointers to dma buffer pages(2k).

static ULONG  dma_page_size;
ULONG         v_nVolume;

PUCHAR  m_pIoSpace = 0;      // *wb Was referenced in SBAWE32.H
PUCHAR  m_pAudioBase;


//*****************************************************************************
//                           ---Volatiles---
//*****************************************************************************
//
// These variables may be accessed by different processes or the hardware and
// are likely to change in a time critical fashion. Optimization may cause
// errors if not declared volatile.
//
volatile        BOOL    v_fMoreData[2];    // *wb More to get/send at interrupt time?
volatile static ULONG   v_nNextPage[2];    // *wb Array of Indices to dma_page array.



//******************************************************************************
//
// "GetRegistryConfig()" --Function to get the IRQ, I/O ports, & DMA channel,
//                         values from the registry as initialized by entries
//                         initialized in 'PLATFORM.REG'
//
//                  The registry keys expected are:
//                  "Irq" -- the chip's IRQ value.(e.g. 10)
//
// NOTE: 'lpRegPath' is assumed to point to a subkey of HKEY_LOCAL_MACHINE
//
// Returns ERROR_SUCCESS on success or a Win32 error code on failure
//
//******************************************************************************
DWORD
GetRegistryConfig(LPWSTR lpRegPath, UCHAR * lpdwIrq)
{
    HKEY hConfig = INVALID_HANDLE_VALUE;
    HKEY  hKeyActive = INVALID_HANDLE_VALUE;
    WCHAR szConfigReg[128];
    DWORD dwData;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwRet;

         // **********************************************************
         //  The HKLM\Drivers\Active\??  key(pointed to by
         //  'lpRegPath') was passed in from DEVICE.EXE., The subkey
         //  "Key" points to the original registry location where we
         //  can find our extra data about this device.
         // **********************************************************
                   // Get a handle to the "HKLM\Drivers\Active\??"
                   //    key passed in from DEVICE.EXE .
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, 0, &hKeyActive);

    if (dwRet != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_PDD,
                  (TEXT("WAVEDEV : GetRegistryConfig RegOpenKeyEx(%s) failed %d\r\n"),
                  lpRegPath, dwRet));
        goto EXIT;
    }

    dwSize = sizeof(szConfigReg);


          // Get the value of "Key" of where to find our data -->'szConfigReg'.
    dwRet = RegQueryValueEx(hKeyActive, TEXT("Key"), 0, &dwType, (PUCHAR)szConfigReg, &dwSize);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_PDD,
                 (TEXT("WAVEDEV : GetRegistryConfig RegQueryValueEx(Key) failed %d\r\n"),
                 dwRet));
        goto EXIT;
    }

          // Get a handle to the Key containing our data.
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szConfigReg, 0, 0, &hConfig);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_PDD,
                 (TEXT("WAVEDEV : GetRegistryConfig RegOpenKeyEx(%s) failed %d\r\n"),
                 lpRegPath, dwRet));
        goto EXIT;
    }


         // ******* Get the value of "Irq" in the open key. ******
    dwSize = sizeof(DWORD);
    dwRet = RegQueryValueEx(hConfig, TEXT("Irq"), 0, &dwType,
                            (PUCHAR)&dwData, &dwSize);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_PDD,
                 (TEXT("WAVEDEV : GetRegistryConfig RegQueryValueEx(%s) failed %d\r\n"),
                 TEXT("Irq"), dwRet));
        goto EXIT;
    }
    *lpdwIrq = (UCHAR) dwData;         // return irq# to caller.


EXIT:
    if (hConfig != INVALID_HANDLE_VALUE)
        RegCloseKey(hConfig);

    if (hKeyActive != INVALID_HANDLE_VALUE)
        RegCloseKey(hKeyActive);

    return dwRet;
} // GetRegistryConfig()


//******************************************************************************
// "Peek()-- Get a DW sp register and return it.
//******************************************************************************
DWORD Peek(DWORD *Reg)
{
    return(READ_REGISTER_ULONG(Reg));
}


//******************************************************************************
// "Poke()-- Write a DW sp register.
//******************************************************************************
void Poke(DWORD *Reg, DWORD value)
{
    WRITE_REGISTER_ULONG(Reg, value);
}


//******************************************************************************
// "ReadAC97" -- Reads a word from the specified location in the
//               CS4620's address space(based on the BA0 register).
//
// 1. Write ACCAD = Command Address Register = 46Ch for AC97 register address
// 2. Write ACCDA = Command Data Register = 470h for data to write to AC97 register,
//                                            0h for reads.
// 3. Write ACCTL = Control Register = 460h for initiating the write
// 4. Read ACCTL = 460h, DCV should be reset by now and 460h = 17h
// 5. if DCV not cleared, break and return error
// 6. Read ACSTS = Status Register = 464h, check VSTS bit
//****************************************************************************
BOOL ReadAC97(DWORD ulOffset, DWORD *pulValue)
{
    DWORD ulCount, ulStatus;

         // Make sure that there is not data sitting
         // around from a previous uncompleted access.
         // ACSDA = Status Data Register = 47Ch
    ulStatus = Peek(pBA0+BA0_ACSDA/4);

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
         // Get the actual AC97 register from the offset
         //
    Poke(pBA0+BA0_ACCAD/4, ulOffset - BA0_AC97_RESET);
    Poke(pBA0+BA0_ACCDA/4, 0);
    Poke(pBA0+BA0_ACCTL/4,  ACCTL_DCV | ACCTL_CRW | ACCTL_VFRM |
                            ACCTL_ESYN | ACCTL_RSTN);
         //
         // Wait for the read to occur.
         //
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
              // First, we want to wait for a short time.
        Sleep(1);

              // Now, check to see if the read has completed.
              // ACCTL = 460h, DCV should be reset by now and 460h = 17h
        if( !(Peek(pBA0+BA0_ACCTL/4) & ACCTL_DCV))
            break;
    }
         // Make sure the read completed.
    if(Peek(pBA0+BA0_ACCTL/4) & ACCTL_DCV)
        return(1);

         // Wait for the valid status bit to go active.
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
              // Read the AC97 status register.
              // ACSTS = Status Register = 464h
         ulStatus = Peek(pBA0+BA0_ACSTS/4);

              // See if we have valid status.
              // VSTS - Valid Status
         if(ulStatus & ACSTS_VSTS)
             break;

              // Wait for a short while.
         Sleep(1);
    }

         // Make sure we got valid status.
    if(!(ulStatus & ACSTS_VSTS))
        return(1);

         // Read the data returned from the AC97 register.
         // ACSDA = Status Data Register = 474h
     *pulValue = Peek(pBA0+BA0_ACSDA/4);

         // Success.
    return(0);
}


//****************************************************************************
//
// "WriteAC97()"-- writes a word to the specified location in the
// CS461x's address space (based on the part's base address zero register).
//
// 1. Write ACCAD = Command Address Register = 46Ch for AC97 register address
// 2. Write ACCDA = Command Data Register = 470h    for data to write to AC97 register
// 3. Write ACCTL = Control Register = 460h for initiating the write
// 4. Read ACCTL = 460h, DCV should be reset by now and 460h = 07h
// 5. if DCV not cleared, break and return error
//
//****************************************************************************
BOOL WriteAC97(DWORD ulOffset, DWORD ulValue)
{
    DWORD ulCount, ulStatus;

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

         // Get the actual AC97 register from the offset
    Poke(pBA0+BA0_ACCAD/4, ulOffset - BA0_AC97_RESET);
    Poke(pBA0+BA0_ACCDA/4, ulValue);
    Poke(pBA0+BA0_ACCTL/4, ACCTL_DCV | ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);

         // Wait for the write to occur.
    for(ulCount = 0; ulCount < 10; ulCount++)
    {
             // First, we want to wait for a short time.
        Sleep(1);

             // Now, check to see if the write has completed.
             // ACCTL = 460h, DCV should be reset by now and 460h = 07h
        ulStatus = Peek(pBA0+BA0_ACCTL/4);
        if(!(ulStatus & ACCTL_DCV))
            break;
    }

         // Make sure the write completed.
    if(ulStatus & ACCTL_DCV)
        return(1);
         // Success.
    return(0);
}


//******************************************************************************
// "PDD_AudioGetInterruptType()"  -- called from the MDD.  The MDD fields
//                                   audio interrupt and calls this routine.
//                                                                     *wb*
//******************************************************************************
AUDIO_STATE
PDD_AudioGetInterruptType(VOID)
{
    DWORD IntStatus, temp1;
    AUDIO_STATE RetStatus=0;

    IntStatus = Peek(pBA0+BA0_HISR/4);
    if(v_fMoreData[WAPI_OUT] == TRUE || v_fMoreData[WAPI_IN] == TRUE)
       Poke(pBA0+BA0_HICR/4, 0x00000003);  // Do local EOI, reenable interrupts.

    if(!(IntStatus & 0x03))          // If there's not a play(0x01) or capture(0x02)
        return AUDIO_STATE_IGNORE;   //   int pending, return "ignore".

        // Had an interrupt--build proper 'AUDIO_STATE'
        // If expecting more play data and had a play interrupt
    if(v_fMoreData[WAPI_OUT] == TRUE)
    {
        if(IntStatus & 0x01)
            RetStatus |= AUDIO_STATE_OUT_PLAYING;
    }
    else
    {
        RetStatus |= AUDIO_STATE_OUT_STOPPED;
        if(g_Playing == TRUE)
        {
           g_Playing = FALSE;
           WriteAC97(BA0_AC97_PCM_OUT_VOLUME, 0x8000);  // Mute PCM out.
                // Replace high word with zero to stop play.
           temp1 = Peek(pBA1+BA1_STARTSTOP_PLAY/4) & 0x0000ffff;
           Poke(pBA1+BA1_STARTSTOP_PLAY/4, temp1); // Stop play task.
        }
    }

    if(v_fMoreData[WAPI_IN] == TRUE)
    {
       if(IntStatus & 0x02)
          RetStatus |= AUDIO_STATE_IN_RECORDING;
    }
    else
    {
        RetStatus |= AUDIO_STATE_IN_STOPPED;
        if(g_Recording == TRUE)
        {
           g_Recording = FALSE;
           WriteAC97(BA0_AC97_RECORD_GAIN, 0x8000);  // Mute PCM in.
                // Replace low word with zero to stop capture.
           temp1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4) & 0xffff0000;
           Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, temp1); // Stop Record task.
        }
    }
    return RetStatus;        // Return combined play/record state.

} // PDD_AudioGetInterruptType()



//******************************************************************************
// "SetPlaySampleRate()" -- Calculate & store SP sample rate parameters
//                          corresponding to specified samples per second.
//******************************************************************************
void SetPlaySampleRate(DWORD PlayRate)
{
    DWORD temp1, temp2;
    DWORD PhsIncr, CorrectionPerGOF, CorrectionPerSec;
    DWORD OutRate=48000;


         // Calculate and store the values for the
         // correction & phase increment regs.
         // Algorithm from DOSHWInterface::SetPlaySampleRate():
         //    PhsIncr = floor( (PlayRate * 2^26)/ OutRate )
         //    CorrectionPerGOF = floor((PlayRate*2^26 - OutRate * PhsIncr) /
         //                              GOF_PER_SEC)
         //    CorrectionPerSec = PlayRate*2^26 - OutRate*PhsIncr -
         //                          GOF_PER_SEC * CorrectionPerGOF

    temp1 = PlayRate << 16;
    PhsIncr = temp1 / OutRate;
    temp1   -= PhsIncr * OutRate;
    temp1 <<= 10;
    PhsIncr <<= 10;
    temp2 = temp1/OutRate;
    PhsIncr += temp2;
    temp1 -= temp2*OutRate;
    CorrectionPerGOF = temp1/GOF_PER_SEC;
    temp1 -= CorrectionPerGOF * GOF_PER_SEC;
    CorrectionPerSec = temp1;

         // Set BA1_PLAY_SAMPLE_RATE_CORRECTION_REG(288)(0 @ default 48K)
         // and BA1_PlAY_PHASE_INCREMENT_REG(2b4)(0x08000000 @ default 48k)
    Poke(pBA1+BA1_PLAY_SAMPLE_RATE_CORRECTION_REG/4,
       ((CorrectionPerSec << 16) & 0xFFFF0000) | (CorrectionPerGOF & 0x0000FFFF));

    Poke(pBA1+BA1_PLAY_PHASE_INCREMENT_REG/4, PhsIncr);

         // Make SRC the child of the master mixer.
    Poke(pBA1+BA1_MASTER_MIX_CHILD_REG/4, (PROC_SRC_SCB <<16) & 0xFFFF0000);

     return;
}

//******************************************************************************
// "SetCaptureSPValues()" -- Initialize record SP values.
//****************************************************************************** */
void SetCaptureSPValues(void)
{
    DWORD i;
    for(i=0; i<sizeof(InitArray)/sizeof(struct InitStruct); i++)
       Poke(pBA1+InitArray[i].off, InitArray[i].val);
} // SetCaptureSPValues()


//******************************************************************************
// "SetCaptureSampleRate()" -- Initialize the record sample rate converter.
//******************************************************************************
void SetCaptureSampleRate(DWORD dwOutRate)
{
    DWORD dwPhiIncr, dwCoeffIncr, dwTemp1, dwTemp2;
    DWORD dwCorrectionPerGOF, dwCorrectionPerSec, dwInitialDelay;

    DWORD dwFrameGroupLength, dwCnt;

    DWORD dwInRate = 48000;

         //
         // Compute the values used to drive the actual sample rate conversion.
         // The following formulas are being computed, using inline assembly
         // since we need to use 64 bit arithmetic to compute the values:
         //
         //     dwCoeffIncr = -floor((Fs,out * 2^23) / Fs,in)
         //     dwPhiIncr = floor((Fs,in * 2^26) / Fs,out)
         //     dwCorrectionPerGOF = floor((Fs,in * 2^26 - Fs,out * dwPhiIncr)/
         //                                GOF_PER_SEC)
         //     dwCorrectionPerSec = Fs,in * 2^26 - Fs,out * phiIncr -
         //                          GOF_PER_SEC * dwCorrectionPerGOF
         //     dwInitialDelay = ceil((24 * Fs,in) / Fs,out)
         //
         // i.e.
         //
         //     dwCoeffIncr = neg(dividend((Fs,out * 2^23) / Fs,in))
         //     dwPhiIncr:dwOther = dividend:remainder((Fs,in * 2^26) / Fs,out)
         //     dwCorrectionPerGOF:dwCorrectionPerSec =
         //         dividend:remainder(dwOther / GOF_PER_SEC)
         //     dwInitialDelay = dividend(((24 * Fs,in) + Fs,out - 1) / Fs,out)
         //
    dwTemp1 = dwOutRate << 16;
    dwCoeffIncr = dwTemp1 / dwInRate;
    dwTemp1 -= dwCoeffIncr * dwInRate;
    dwTemp1 <<= 7;
    dwCoeffIncr <<= 7;
    dwCoeffIncr += dwTemp1 / dwInRate;
    dwCoeffIncr ^= 0xFFFFFFFF;
    dwCoeffIncr++;
    dwTemp1 = dwInRate << 16;
    dwPhiIncr = dwTemp1 / dwOutRate;
    dwTemp1 -= dwPhiIncr * dwOutRate;
    dwTemp1 <<= 10;
    dwPhiIncr <<= 10;
    dwTemp2 = dwTemp1 / dwOutRate;
    dwPhiIncr += dwTemp2;
    dwTemp1 -= dwTemp2 * dwOutRate;
    dwCorrectionPerGOF = dwTemp1 / GOF_PER_SEC;
    dwTemp1 -= dwCorrectionPerGOF * GOF_PER_SEC;
    dwCorrectionPerSec = dwTemp1;
    dwInitialDelay = ((dwInRate * 24) + dwOutRate - 1) / dwOutRate;

         //
         // Fill in the VariDecimate control block.
         //
    Poke(pBA1+BA1_CAPTURE_SAMPLE_RATE_CORRECTION_REG/4,
        ((dwCorrectionPerSec << 16) & 0xFFFF0000) | (dwCorrectionPerGOF & 0xFFFF));

    Poke(pBA1+BA1_CAPTURE_COEFFICIENT_INCREMENT_REG/4, dwCoeffIncr);

    Poke(pBA1+BA1_CAPTURE_DELAY_REG/4,
        (((BA1_VARIDEC_BUF_1 + (dwInitialDelay << 2)) << 16) & 0xFFFF0000) | 0x80);
    Poke(pBA1+BA1_CAPTURE_PHASE_INCREMENT_REG/4, dwPhiIncr);

         // Figure out the frame group length for the write back
         // task. This is just the factors of 24000 (2^6*3*5^3)
         // that are not present in the output sample rate.
    dwFrameGroupLength = 1;
    for(dwCnt = 2; dwCnt <= 64; dwCnt *= 2)
        if(((dwOutRate / dwCnt) * dwCnt) != dwOutRate)
            dwFrameGroupLength *= 2;

    if(((dwOutRate / 3) * 3) != dwOutRate)
        dwFrameGroupLength *= 3;

    for(dwCnt = 5; dwCnt <= 125; dwCnt *= 5)
        if(((dwOutRate / dwCnt) * dwCnt) != dwOutRate)
            dwFrameGroupLength *= 5;

         //
         // Fill in the WriteBack control block.
         //
    Poke(pBA1+BA1_CAPTURE_FRAME_GROUP_1_REG/4, dwFrameGroupLength);

    Poke(pBA1+BA1_CAPTURE_FRAME_GROUP_2_REG/4, (0x00800000 | dwFrameGroupLength));

    Poke(pBA1+BA1_CAPTURE_CONSTANT_REG/4, 0x0000FFFF);

    Poke(pBA1+BA1_CAPTURE_SPB_ADDRESS/4, ((65536 * dwOutRate) / 24000));

    Poke((pBA1+BA1_CAPTURE_SPB_ADDRESS/4 + 1), 0x0000FFFF);

    return;
}


//******************************************************************************
// "private_InitPCI()" -- Called only from PDD_AudioInitialize.
//******************************************************************************
BOOL
private_InitPCI(VOID)
{
   ULONG BA0phys, BA1phys;                  // Phys addrs decoded by PCI part.
   UCHAR  pnpIRQ;                           // IRQ as set by 'BIOS'
   ULONG  inIoSpace=0;                      // Set by HalTranslateBusAddress
   PCI_SLOT_NUMBER     slotNumber;
   PCI_COMMON_CONFIG   pciConfig;
   int                 bus, device, function;
   int                 length, fUseExistingSettings=1;
   BOOL                bFoundIt, rc;

   DWORD temp1, temp2, offset, count,i;

   bFoundIt = FALSE;

   for(bus = 0; bus < PCI_MAX_BUS; bus++)
   {
      for (device = 0; device < PCI_MAX_DEVICES; device++)
      {
         slotNumber.u.bits.DeviceNumber = device;

         for (function = 0; function < PCI_MAX_FUNCTION; function++)
         {
            slotNumber.u.bits.FunctionNumber = function;

            length = HalGetBusData(PCIConfiguration, bus, slotNumber.u.AsULONG,
                                   &pciConfig, sizeof(pciConfig) -
                                   sizeof(pciConfig.DeviceSpecific));

            if (length == 0)
                break;

            if( (pciConfig.VendorID == 0x1013) &&      // We've found
                (pciConfig.DeviceID == 0x6003))        //  Crystal Part.
            {
               RETAILMSG(1,(TEXT("CS4620: CS4620 PCI Audio Device,")
                            TEXT("VendorId: %04X, DeviceId: %04X\r\n"),
                            pciConfig.VendorID, pciConfig.DeviceID));
               bFoundIt = TRUE;
               pnpIRQ  = pciConfig.u.type0.InterruptLine;
               BA0phys = pciConfig.u.type0.BaseAddresses[0];
               BA1phys = pciConfig.u.type0.BaseAddresses[1];

               DEBUGMSG(ZONE_PDD,
                        (TEXT("CS4620: PCI config: Irq = 0x%X, BA0 = 0x%.8x, ")
                         TEXT("BA1 = 0x%.8x\r\n"), pnpIRQ, BA0phys, BA1phys));

                    // As of ETK v2.1 MS comments in PLATFORM.REG(OHCI device)
                    // and in the sample code for this(USB) device(the only
                    // sample PCI driver) say that "We don't have routines
                    // to modify PCI IRQ and mem base....". The sample driver
                    // gives the option to fail if registry and HalGetBusData
                    // give different values.
                    // For this driver(CS4620) we'll accept whatever memory
                    // addresses HalGetBusData gives us and fail if the
                    // Registry- and HalGetBusData-IRQ's differ.   *wb
               if(pnpIRQ != (UCHAR) g_Irq)
               {
                   RETAILMSG(1,(TEXT("CS4620: PnPIRQ NOT! registry IRQ:")
                                TEXT("PnP %u, (reg: %u.)\r\n"), pnpIRQ,
                                (UCHAR)g_Irq));

                   bFoundIt = FALSE;
               }
               break;
            }  //endif..VendorID..
            if (function == 0 && !(pciConfig.HeaderType & 0x80))
               break;
         } //endfor(function = 0....
         if (bFoundIt || length == 0)
            break;
      } // endfor(device = ...

      if (bFoundIt || (length == 0 && device == 0))
          break;
   } //endfor(bus = 0...

   if (!bFoundIt)
   {
       DEBUGMSG(ZONE_ERROR,
                (TEXT("CS4620: Error, can't find Crystal PCI Audio Device\r\n")));

       return FALSE;
   }

   DEBUGMSG(ZONE_PDD,
            (TEXT("CS4620: Revision Stepping value = 0x%02X\r\n"),
             pciConfig.RevisionID));


           //
           // Map both Base registers into the local address space
           //
   pBA0 = (PDWORD)VirtualAlloc(0,
                               4096,             // 4K for BA0
                               MEM_RESERVE,
                               PAGE_NOACCESS);

   rc = VirtualCopy((LPVOID) pBA0,
                    (LPVOID)(BA0phys/256),
                     4096,                       // 4K for BA0
                     PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL);
           // Test mapping.
   if(!rc || !pBA0)
   {
       DEBUGMSG(1, (TEXT("CS4620: Error mapping BA0.\r\n")));
       return FALSE;
   }

   pBA1 = (PDWORD)VirtualAlloc(0,
                               1048576,          // 1Meg for BA1
                               MEM_RESERVE,
                               PAGE_NOACCESS);

   rc = VirtualCopy((LPVOID) pBA1,
                    (LPVOID)(BA1phys/256),
                     1048576,                    // 1Meg for BA1
                     PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL);

   if(!rc || !pBA1)
   {
       DEBUGMSG(1, (TEXT("CS4620: Error mapping BA1.\r\n")));
       return FALSE;
   }


#ifdef DEBUG
    DebugBreak();
#endif //DEBUG

        //**********************************************
        //  Set up the clocks(Initialize()--from main())
        //**********************************************
        // Clear the clock control reg so PLL starts in a known state.
   Poke(pBA0+BA0_CLKCR1/4, 0);

        // Clear the master serial port reg to put ports in a known state.
   Poke(pBA0+BA0_SERMC1/4, 0);

        // Set the part to a 'host-controlled AC-Link'
   Poke(pBA0+BA0_SERACC/4, (SERACC_HSP | SERACC_CODEC_TYPE_1_03)); // 0x08

        // Drive the ARST# pinlow for a min of 1 microsecond.  This
        // is done for non AC97 modes since there might be external
        // logic that uses the ARST# line for a reset.
   Poke(pBA0+BA0_ACCTL/4, 0);
   Sleep(1);            // 1 mil;
   Poke(pBA0+BA0_ACCTL/4, ACCTL_RSTN);

        // Enable sync generation.
   Poke(pBA0+BA0_ACCTL/4, ACCTL_ESYN | ACCTL_RSTN);

        // Now wait 'for a short while' to allow the  AC97
        // part to start generating bit clock. (so we don't
        // Try to start the PLL without an input clock.)
   Sleep(50);

        // Set the serial port timing configuration, so that the
        // clock control circuit gets its clock from the right place.
   Poke(pBA0+BA0_SERMC1/4, SERMC1_PTC_AC97);

        // Write the selected clock control setup to the hardware.  Don't
        // turn on SWCE yet, so thqt the devices clocked by the output of
        // PLL are not clocked until the PLL is stable.
   Poke(pBA0+BA0_PLLCC/4, PLLCC_LPF_1050_2780_KHZ | PLLCC_CDR_73_104_MHZ);
   Poke(pBA0+BA0_PLLM/4,  0X3A);
   Poke(pBA0+BA0_CLKCR2/4, CLKCR2_PDIVS_8);

        // Power up the PLL.
   Poke(pBA0+BA0_CLKCR1/4, CLKCR1_PLLP);

        // Wait for the PLL to stabilize.
   Sleep(50);

        // Turn on clocking of the core so we can set up serial ports.
   Poke(pBA0+BA0_CLKCR1/4, CLKCR1_PLLP | CLKCR1_SWCE);

        //**************************************************************
        // Set up the serial ports(ClearSerialFIFOs()from Initialize()).
        //**************************************************************
        //  Clear the serial fifo's to silence(0).
   Poke(pBA0+BA0_SERBWP/4, 0);      // Clear the backdoor sample data.

        // Fill all 256 sample FIFO locations.
   for(temp1=0; temp1<256; temp1++)
   {
            // Make sure the previous FIFO write is complete.
       for(temp2=0; temp2<5; temp2++)
       {
           Sleep(1);              // Wait a mil.
           if( !(Peek(pBA0+BA0_SERBST/4) & SERBST_WBSY) )  // If write not busy,
               break;    // break from 'for' loop.
       }
            // Write the serial port FIFO index.
       Poke(pBA0+BA0_SERBAD/4, temp1);
            // Load the new value into the FIFO location.
       Poke(pBA0+BA0_SERBCM/4, SERBCM_WRC);
   } //endfor(temp1..

        //  Back in Initialize(), (Returned from ClearSerialFIFOs())
        //  Write the serial port configuration to the part.  The Master
        //  Enable bit isn't set until all other values have been written.
   Poke(pBA0+BA0_SERC1/4, SERC1_SO1F_AC97 | SERC1_SO1EN);
   Poke(pBA0+BA0_SERC2/4, SERC2_SI1F_AC97 | SERC1_SO1EN);
   Poke(pBA0+BA0_SERMC1/4, SERMC1_PTC_AC97 | SERMC1_MSPE);

        //*********************************************
        // Set up the AC97 codec.(back in Initialize())
        //*********************************************
        // Wait for the codec ready signal from the AC97 codec.
   for(temp1=0; temp1<1000; temp1++)
   {
            // Delay a mil to let things settle out and
            // to prevent retrying the read too quickly.
       Sleep(1);
       if( Peek(pBA0+BA0_ACSTS/4) & ACSTS_CRDY )    // If ready,
           break;                                   //   exit the 'for' loop.
   }

   if( !(Peek(pBA0+BA0_ACSTS/4) & ACSTS_CRDY) )     // If never came ready,
       return FALSE;                                //   exit initialization.

        // Assert the 'valid frame' signal so we can
        // begin sending commands to the AC97 codec.
   Poke(pBA0+BA0_ACCTL/4, ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);

        // Wait until we've sampled input slots 3 & 4 as valid, meaning
        // that the codec is pumping ADC data across the AC link.
   for(temp1=0; temp1<1000; temp1++)
   {
            // Delay a mil to let things settle out and
            // to prevent retrying the read too quickly.
       Sleep(1);

            // Read the input slot valid register;  See
            // if input slots 3 and 4 are valid yet.
       if( (Peek(pBA0+BA0_ACISV/4) & (ACISV_ISV3 | ACISV_ISV4) ) ==
                  (ACISV_ISV3 | ACISV_ISV4)  )
            break;    // Exit the 'for' if slots are valid.
   }

            // If we never got valid data, exit initialization.
   if( (Peek(pBA0+BA0_ACISV/4) & (ACISV_ISV3 | ACISV_ISV4) ) !=
                  (ACISV_ISV3 | ACISV_ISV4)  )
           return FALSE;     // If no valid data, exit initialization.

        // Start digital data transfer of audio data to the codec.
   Poke(pBA0+BA0_ACOSV/4, ACOSV_SLV3 | ACOSV_SLV4);

        //********************************************
        // Return from Initialize()-back in main()
        // Reset the processor(ProcReset()-from main()
        //********************************************
   Poke(pBA1+BA1_SPCR/4, SPCR_RSTSP);    // Write the SP cntrl reg reset bit.
   Poke(pBA1+BA1_SPCR/4, SPCR_DRQEN);    // Write the SP cntrl reg DMAreqenbl bit.

         // Clear the trap registers.
   for(temp1=0; temp1<8; temp1++)
   {
       Poke(pBA1+BA1_DREG/4, DREG_REGID_TRAP_SELECT+temp1);
       Poke(pBA1+BA1_TWPR/4, 0xFFFF);
   }
   Poke(pBA1+BA1_DREG/4, 0);

         // Set the frame timer to reflect the number of cycles per frame.
   Poke(pBA1+BA1_FRMT/4, 0xadf);

        //*********************************************************
        // Download the Processor Image(DownLoadImage()-from main()
        //**********************************************************
   temp2 = 0;           // offset into BA1Array(CWCIMAGE.H)
   for(i=0; i<INKY_MEMORY_COUNT; i++)
   {
       offset =  BA1Struct.MemoryStat[i].ulDestByteOffset;
       count  =  BA1Struct.MemoryStat[i].ulSourceByteSize;
       for(temp1=(offset)/4; temp1<(offset+count)/4; temp1++)
           Poke(pBA1+temp1, BA1Struct.BA1Array[temp2++]);
   }

        //*****************************************
        //  Save STARTSTOP_PLAY contents, replace
        //  high word with zero to stop PLAY.
        //*****************************************
   temp1 = Peek(pBA1+BA1_STARTSTOP_PLAY/4);
   StartPlay = temp1 & 0xffff0000;
   Poke(pBA1+BA1_STARTSTOP_PLAY/4, temp1 & 0x0000ffff);     // Stop play.

        //*******************************************
        //  Save STARTSTOP_CAPTURE contents, replace
        //  low word with zero to stop CAPTURE.
        //*******************************************
   temp1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4);
   StartCapture = temp1 & 0x0000ffff;
   Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, temp1 & 0xffff0000);  // Stop capture.

        //******************************************
        // Power on the DAC(AddDACUser()from main())
        //******************************************
   ReadAC97(BA0_AC97_POWERDOWN, &temp1);
   WriteAC97(BA0_AC97_POWERDOWN, temp1 &= 0xfdff);

        // Wait until we sample a DAC ready state.
   for(temp2=0; temp2<32; temp2++)
   {
            // Let's wait a mil to let things settle.
       Sleep(1);
            // Read the current state of the power control reg.
       ReadAC97(BA0_AC97_POWERDOWN, &temp1);
            // If the DAC ready state bit is set, stop waiting.
       if(temp1 & 0x2)
            break;
   }

        //******************************************
        // Power on the ADC(AddADCUser()from main())
        //******************************************
   ReadAC97(BA0_AC97_POWERDOWN, &temp1);
   WriteAC97(BA0_AC97_POWERDOWN, temp1 &= 0xfeff);

        // Wait until we sample a DAC ready state.
   for(temp2=0; temp2<32; temp2++)
   {
            // Let's wait a mil to let things settle.
       Sleep(1);
            // Read the current state of the power control reg.
       ReadAC97(BA0_AC97_POWERDOWN, &temp1);
            // If the ADC ready state bit is set, stop waiting.
       if(temp1 & 0x1)
            break;
   }

       //********************************************************
       // Map the audio buffers into the local address space
       // Map the 8K reserved in CONFIG.BIB. Use it as 2
       // 4K buffers, one for play, one for record.
       // The physical address of the buffers has the label
       // 'AUDIO_DMA_BUFFER_BASE_PA', which will be the physical
       // address of the play buffer.  This address + 4K will
       // be the physical address of the record buffer.
       // (AllocateAudioBuffers()---from main()
       //********************************************************
       // Get the (virtual(x86-linear)) address of the buffers.
   dma_page[0] = (PUCHAR)VirtualAlloc(0,
                                      AUDIO_DMA_PAGE_SIZE * 4,
                                      MEM_RESERVE,
                                      PAGE_NOACCESS);
   if (!dma_page[0])
      return FALSE;

   temp1 = VirtualCopy((LPVOID) dma_page[0],
                       (LPVOID)(AUDIO_DMA_BUFFER_BASE_PA | 0x80000000),
                       AUDIO_DMA_PAGE_SIZE * 4,
                       PAGE_READWRITE | PAGE_NOCACHE);

   if (temp1 != TRUE )
   {
      ERRMSG("private_InitPCI: VirtualCopy Failed");
      return FALSE;
   }

   v_nVolume       = 0xFFFFFFFF;
   dma_page[1]     = dma_page[0] + AUDIO_DMA_PAGE_SIZE;
   dma_page[2]     = dma_page[1] + AUDIO_DMA_PAGE_SIZE;
   dma_page[3]     = dma_page[2] + AUDIO_DMA_PAGE_SIZE;
   dma_page_size   = AUDIO_DMA_PAGE_SIZE;

        // Send the physical play buffer address to the processor.
   Poke(pBA1+BA1_PLAY_BUFFER_ADDRESS/4, AUDIO_DMA_BUFFER_BASE_PA);

        // Send the physical capture buffer address to the processor.
   Poke(pBA1+BA1_CAPTURE_BUFFER_ADDRESS/4, AUDIO_DMA_BUFFER_BASE_PA +
                                           AUDIO_DMA_PAGE_SIZE * 2);

        //**********************************************
        // Start the processor(ProcStart()--from main())
        //**********************************************
        // Set the frame timer to reflect
        // the number of cycles per frame.
   Poke(pBA1+BA1_FRMT/4, 0xadf);

        // Turn on the 'run', 'run at frame' and
        // 'DMA enable' bits in the  SP Control Reg.
   Poke(pBA1+BA1_SPCR/4, SPCR_RUN | SPCR_RUNFR | SPCR_DRQEN);

        // Wait until the 'run at frame' bit
        // is reset in the SP control reg.
   for(temp1=0; temp1 < 25; temp1++)
   {
      Sleep(1);
      temp2 = Peek(pBA1+BA1_SPCR/4);
           // If 'run at frame' bit
           // is zero, quit waiting.
      if(!(temp2 & SPCR_RUNFR))
         break;
   }

        // If 'run at frame' never went
        // to zero, return error.
   if(temp2 & SPCR_RUNFR)
      return FALSE;

        //**************************************
        // Unmute the Master and Alternate
        // (headphone) volumes.  Set to max.
        //**************************************
   Poke(pBA1+BA1_PLAYBACK_VOLUME/4, 0xFFFFFFFF);   // Max atten of playback vol.
   WriteAC97(BA0_AC97_HEADPHONE_VOLUME, 0);
   WriteAC97(BA0_AC97_MASTER_VOLUME, 0);


   return TRUE;
}  //private_InitPCI()


//******************************************************************************
// "PDD_AudioInitialize()"----
//******************************************************************************
BOOL
PDD_AudioInitialize(DWORD dwIndex )
{
    ULONG nTmpVal = 0;
    DWORD dwRet;

    DEBUGMSG(ZONE_PDD, (TEXT("+PDD_AudioInitialize\r\n")));
//  FUNC_WPDD("PDD_AudioInitialize");

#ifdef DEBUG
    DebugBreak();
#endif //DEBUG

    if((dwRet = GetRegistryConfig((LPWSTR) dwIndex, &g_Irq))
        != ERROR_SUCCESS)
    {
        DEBUGMSG(1, (TEXT("WAVEDEV.DLL : GetRegistryConfig failed %d\r\n"),dwRet));
        return FALSE;
    }

    gIntrAudio = MapIrq2SysIntr(g_Irq);     // 'gIntrAudio' required by MDD *wb*

    if(!private_InitPCI())
    {
        DEBUGMSG(1, (TEXT("PDD_AudioInitialize : No CS4620 Found\r\n")));
        return FALSE;
    }


   AudioPowerOn();
   DEBUGMSG(1, (TEXT("-PDD_AudioInitialize\r\n")));
   return TRUE;
} // PDD_AudioInitialize()



//*****************************************************************************
// "AudioPowerOn()"--
// Routine for powering on Audio hardware.  It's called from
// within power handler, so should not make any system calls.
//
//*****************************************************************************
VOID
AudioPowerOn()
{
   v_nNextPage[WAPI_OUT] = 0;
   v_nNextPage[WAPI_IN]  = 2;
   v_fMoreData[WAPI_OUT] = FALSE;
   v_fMoreData[WAPI_IN]  = FALSE;

} // AudioPowerOn()



//*****************************************************************************
//  "PDD_AudioPowerHandler()"---
//
// Handle any power up/down issues here.
//
//*****************************************************************************
VOID
PDD_AudioPowerHandler(BOOL power_down)
{
} // PDD_AudioPowerHandler()



//*****************************************************************************
//  "PDD_AudioMessage()"---
//
// Handle any custom WAVEIN or WAVEOUT messages here
//
//*****************************************************************************
DWORD
PDD_AudioMessage(UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    return(MMSYSERR_NOTSUPPORTED);
} // PDD_AudioMessage()


//*****************************************************************************
//  "PDD_AudioDeinitialize()"---
//
// The driver is being unloaded. Deinitialize. Free up memory, etc.
//
//*****************************************************************************
VOID
PDD_AudioDeinitialize(VOID)
{
    VirtualFree ((void*) dma_page[0], 0, MEM_RELEASE);
} // PDD_AudioDeinitialize()


//*****************************************************************************
// "private_AudioFillBuffer()"--
//      Note: pwh->dwBytesRecorded is used as a 'buffer done'
//            indicator by the MDD.  At interrupt time, the MDD
//            returns 'done' buffers to the wave interface.     *wb*
//*****************************************************************************
VOID private_AudioFillBuffer(PWAVEHDR pwh, INT16 UNALIGNED * pBuffer)
{
    BYTE *pSrc, *pTarg;           // Running pointers to source & target buffers.
    BYTE  silence;
    DWORD toFillCnt;              // Byte count unfilled in DMA(target) buffer.
    DWORD filledCnt=0;            // Count of bytes moved to DMA(target) buffer.
    DWORD srcCnt;                 // Count of available bytes in source buffer.
    DWORD i;
    PWAVEHDR pwhTemp;

    pTarg = (BYTE *) pBuffer;
    toFillCnt = AUDIO_DMA_PAGE_SIZE;

    while(toFillCnt > 0)     // While there's space in the DMA buffer.
    {
        if(pwh == NULL)      // If the wave header pointer is bad, silence.
        {
            if( (g_pcmtype[WAPI_OUT] == PCM_TYPE_M8)
               || (g_pcmtype[WAPI_OUT] == PCM_TYPE_S8) )
                silence = 0x80;
            else
                silence = 0x00;

            for(i=0; i<toFillCnt; i++, pTarg++)
               *pTarg = silence;
            break;           // Exit from the while loop.
        }

              // 'pwh' isn't NULL;  use it for source.
        pSrc = pwh->lpData + pwh->dwBytesRecorded;
        srcCnt = pwh->dwBufferLength - pwh->dwBytesRecorded;

              // If enough data is available in the source buffer
              // to fill the DMA buffer, do the fill and return.
        if(srcCnt > toFillCnt)
        {
            for(i=0; i<toFillCnt; i++, pTarg++, pSrc++)
               *pTarg = *pSrc;
            pwh->dwBytesRecorded += toFillCnt;   // Update header info.
            break;                // We're done.  Exit from while().
        }

              // There isn't enough data in the current source buffer
              // to completely fill the target buffer.  Take what's
              // available from the current source buffer, point to
              // the next source, and continue with the fill process.
        for(i=0; i<srcCnt; i++, pTarg++, pSrc++)
           *pTarg = *pSrc;
        pwhTemp = pwh->lpNext;    // Put in temp for interrupt-safe access.
        pwh->dwBytesRecorded = pwh->dwBufferLength;   // Set 'buffer-done'
        toFillCnt -= srcCnt;      // Adjust to-be-filled count.
        pwh = pwhTemp;
    } // endwhile

} // private_AudioFillBuffer()


//*****************************************************************************
// "private_AudioGetBuffer()"--
//      pwh points to chain of wave headers(->user buffers)
//      pBuffer is pointer to DMA buffer.
//      Transfer data from DMA buffer to wave header buffer.
//      Note: Data in DMA buffer is always 16-bit stereo.
//            If request is for 'mono' take the left sample.
//            If request is for '8-bit', convert as required.
//*****************************************************************************
VOID private_AudioGetBuffer(PWAVEHDR pwh, BYTE * pBuffer)
{
   DWORD i, sample = 0;
   BYTE  *pTarg;
   PPCMWAVEFORMAT pFormat = (PPCMWAVEFORMAT) g_pwfx[WAPI_IN];

   FUNC_VERBOSE("PDD_AudioGetBuffer");

   if(pwh == NULL)           // If passed pointer is NULL, return.
      return;

        // For each sample in the DMA buffer, convert as
        // required & transfer to the wave header buffer.
   for(i=0; i < AUDIO_DMA_PAGE_SIZE/4; i++)
   {
           // If header buffer is full, point to next header buffer.
      if(pwh->dwBytesRecorded >= pwh->dwBufferLength)
         pwh = pwh->lpNext;

      if(pwh == NULL)        // If new pointer is NULL, return.
         return;

//    sample = (DWORD)pBuffer[i*4];    // Get 16-bit stereo sample.
      sample =  *( (DWORD *)(pBuffer+(i*4)) );

      pTarg = pwh->lpData + pwh->dwBytesRecorded;

      switch(g_pcmtype[WAPI_IN])
      {
         case PCM_TYPE_M8:
            *pTarg = (BYTE)((sample >> 24) + 128);  // Use left sample & convert.
            pwh->dwBytesRecorded++;
            break;

         case PCM_TYPE_S8:
            *pTarg = (BYTE)((sample >> 24) + 128);       // Left sample, convert.
            *(pTarg+1) = (BYTE)((sample & 0xff) + 128);  // Right sample, convert.
            pwh->dwBytesRecorded += 2;
            break;

         case PCM_TYPE_M16:
            *((WORD *) pTarg) = (WORD) (sample >> 16);   // Use left sample.
            pwh->dwBytesRecorded += 2;
            break;

         case PCM_TYPE_S16:
                // Replace low word with zero to stop capture.
            *((DWORD *) pTarg) = sample;         // Move 16-bit stereo sample.
            pwh->dwBytesRecorded += 4;

            break;
      } //endswitch
   } // endfor(each sample..

#ifdef DEBUG                                                 // *** DEBUG ***
//#defineSTOPONINTERRUPT                                     // *** DEBUG ***
#ifdef STOPONINTERRUPT                                       // *** DEBUG ***
   dbug1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4) & 0xffff0000;  // *** DEBUG ***
   Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, dbug1);                // *** DEBUG ***
      // Put breakpoint on following instruction.            // *** DEBUG ***
      // Start the capture task back up.                     // *** DEBUG ***
   dbug1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4) & 0xffff0000;  // *** DEBUG ***
   Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, dbug1 | StartCapture); // *** DEBUG ***
#endif //STOPONINTERRUPT                                     // *** DEBUG ***
#endif //DEBUG                                               // *** DEBUG ***

} // private_AudioGetBuffer


//*****************************************************************************
// "private_WaveOutEndOfData()"--
//*****************************************************************************
VOID
private_WaveOutEndOfData()
{
    UCHAR silence;
    FUNC_WPDD("+PDD_WaveOutEndOfData");
         //
         // There is no more data coming...
         //
    v_fMoreData[WAPI_OUT] = FALSE;
         // (We turn off PEN in GetInterruptType if v_fMoreData[WAPI_OUT]=FALSE.)

    if( (g_pcmtype[WAPI_OUT] == PCM_TYPE_M8)
       || (g_pcmtype[WAPI_OUT] == PCM_TYPE_S8) )
        silence = 0x80;
    else
        silence = 0x00;

    memset (dma_page[v_nNextPage[WAPI_OUT]], silence, dma_page_size);

       // (Turn off PEN in GetInterruptType if v_fMoreData[WAPI_OUT]=FALSE.)
    FUNC_WPDD("-PDD_WaveOutEndOfData");
} // private_WaveOutEndOfData()




//*****************************************************************************
// "private_WaveOutStart()"--
//*****************************************************************************
VOID
private_WaveOutStart(PWAVEHDR pwh )
{
    DWORD playFormat, DWCount=4, temp;

    FUNC_WPDD("+PDD_WaveOutStart");

    v_fMoreData[WAPI_OUT] = TRUE;        // we expect to have more data coming...
    v_nNextPage[WAPI_OUT] = 0;           // Set to first 'page' of DMA buffer.

    private_AudioFillBuffer(pwh, (short*) dma_page[0]);
    private_AudioFillBuffer(pwh, (short*) dma_page[1]);

         // Help keep the keyclicks even.(MS)
    Sleep(1);

         // Set SRC corrections in the processor.
    SetPlaySampleRate((g_pwfx[WAPI_OUT])->nSamplesPerSec );

         // ********************************************
         // Read the Playback Format & Interrupt Enable
         // register & modify it for the current format.
         // ********************************************
    playFormat = Peek(pBA1+BA1_PFIE/4);

    if( (g_pwfx[WAPI_OUT])->nChannels == 2)
    {
        playFormat &= ~PFIE_MONO;                // Turn off mono bit.
        DWCount *= 2;                            // Double DMA count.
    }
    else
        playFormat |= PFIE_MONO;                 // Turn on mono bit.

    if( (g_pwfx[WAPI_OUT])->wBitsPerSample == 16)
    {
        playFormat &= ~PFIE_8BIT;                // Turn off '8-bit' & 'sign change'.
        DWCount *= 2;                            // Double DMA count.
    }
    else
        playFormat |= PFIE_8BIT;                 // Turn on '8-bit' & 'sign change'.

    playFormat &= ~PFIE_INT_DISABLE;             // Turn off 'interrupt disable'.

    Poke(pBA1+BA1_PFIE/4, playFormat);           // Write modified PFIE to the SP.
    Poke(pBA0+BA0_HICR/4, 0x00000003);           // Enable sp interrupts.

    temp = Peek(pBA1+BA1_PLAY_DCW/4) & 0xfffffe00;  // Get DCW; AND out Count.
    Poke(pBA1+BA1_PLAY_DCW/4, temp | --DWCount);    // Set DMA Control Word Count

        // Set the physical play buffer address in the processor.
    Poke(pBA1+BA1_PLAY_BUFFER_ADDRESS/4, AUDIO_DMA_BUFFER_BASE_PA);

    g_Playing = TRUE;                         // Set playing active flag.

         // Start the Playback task.
    temp = Peek(pBA1+BA1_STARTSTOP_PLAY/4) & 0x0000ffff;
    Poke(pBA1+BA1_STARTSTOP_PLAY/4, temp |= StartPlay);
    WriteAC97(BA0_AC97_PCM_OUT_VOLUME, 0x0c0c);      // -6db PCM out(clip) (.92)

    FUNC_WPDD("-PDD_WaveOutStart");
} // private_WaveOutStart()


//*****************************************************************************
// "private_WaveOutContinue()"--
//*****************************************************************************
VOID
private_WaveOutContinue(PWAVEHDR pwh )
{
    FUNC_VERBOSE("+PDD_WaveOutContinue");

    private_AudioFillBuffer (pwh, (short*) dma_page[v_nNextPage[WAPI_OUT]]);

//    PrintBuffer(dma_page[v_nNextPage]);

    v_nNextPage[WAPI_OUT] = (!v_nNextPage[WAPI_OUT] ? 1 : 0);

    FUNC_VERBOSE("-PDD_WaveOutContinue");
} // private_WaveOutContinue()



//*****************************************************************************
// "private_WaveOutRestart()"--
//*****************************************************************************
MMRESULT
private_WaveOutRestart(PWAVEHDR pwh)
{
    FUNC_VERBOSE("+PDD_WaveOutRestart");

    private_WaveOutStart (pwh);

    FUNC_VERBOSE("-PDD_WaveOutRestart");
    return(MMSYSERR_NOERROR);
} // private_WaveOutRestart()



//*****************************************************************************
// "private_WaveOutPause()"--
//*****************************************************************************
MMRESULT
private_WaveOutPause(VOID)
{
    FUNC_VERBOSE("+PDD_WaveOutPause");

    private_WaveOutEndOfData();

    FUNC_VERBOSE("-PDD_WaveOutPause");
    return(MMSYSERR_NOERROR);
} // private_WaveOutPause()



//*****************************************************************************
// "private_WaveOutStop()"--
//*****************************************************************************
VOID
private_WaveOutStop()
{
    DWORD  temp1;
    FUNC_WPDD("+private_WaveOutStop");

    WriteAC97(BA0_AC97_PCM_OUT_VOLUME, 0x8000);  // Mute PCM out. (.92)
          // Replace high word with zero to stop play.
    temp1 = Peek(pBA1+BA1_STARTSTOP_PLAY/4) & 0x0000ffff;
    Poke(pBA1+BA1_STARTSTOP_PLAY/4, temp1); // Disconnect play task.

    FUNC_WPDD("-private_WaveOutStop");
} // private_WaveOutStop()


//*****************************************************************************
// "private_WaveInStart()" --
//*****************************************************************************
VOID private_WaveInStart( PWAVEHDR pwh )
{
    DWORD temp1;
    FUNC_WPDD("PDD_WaveInStart");

    v_fMoreData[WAPI_IN] = TRUE;        // We expect to have more data coming...
    v_nNextPage[WAPI_IN] = 2;           // Next page for capture data.

         // Set inital values in the SP.
    SetCaptureSPValues();

         // Set SRC data in the SP
    SetCaptureSampleRate(g_pwfx[WAPI_IN]->nSamplesPerSec);

         // Select LINE IN as input.  This is ARBITRARY
         // and should be changed as required.
//  WriteAC97(BA0_AC97_RECORD_SELECT, 0x0404);  // Select LINE IN.

         // Select MIC in --- arbitrary.
//  WriteAC97(BA0_AC97_RECORD_SELECT, 0x0000);    // Select MIC IN(default).
//  WriteAC97(BA0_AC97_RECORD_GAIN_MIC, 0x0f0f);  // Select 22.5 db boost

         // Input gain is also ARBITRARY and
         // should be changed as required
    WriteAC97(BA0_AC97_RECORD_GAIN, 0x0808);    // Set input gain(= 0db).

         // Send the physical capture buffer address to the SP.
    Poke(pBA1+BA1_CAPTURE_BUFFER_ADDRESS/4, AUDIO_DMA_BUFFER_BASE_PA +
                                           AUDIO_DMA_PAGE_SIZE * 2);
    g_Recording = TRUE;                         // Set recording active flag.
    Poke(pBA0+BA0_HICR/4, 0x00000003);          // Enable sp interrupts.
         // Start the capture task.
    temp1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4) & 0xffff0000;
    Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, temp1 | StartCapture); // Start Record task.
}



//*****************************************************************************
// "private_WaveInContinue()" --
//*****************************************************************************
VOID private_WaveInContinue( PWAVEHDR pwh )
{
    FUNC_VERBOSE("+PDD_WaveInContinue");

    private_AudioGetBuffer(pwh, (BYTE*) dma_page[v_nNextPage[WAPI_IN]]);
    v_nNextPage[WAPI_IN] = (v_nNextPage[WAPI_IN] == 3) ? 2 : 3;

    FUNC_VERBOSE("-PDD_WaveInContinue");
}


//*****************************************************************************
// "private_WaveInStop()" --
//*****************************************************************************
VOID private_WaveInStop()
{
    DWORD temp1;
    FUNC_WPDD("+PDD_WaveInStop");

    v_fMoreData[WAPI_IN] = FALSE;

    WriteAC97(BA0_AC97_RECORD_GAIN, 0x8000);  // Mute Record input.

         // Replace low word with zero to stop capture.
    temp1 = Peek(pBA1+BA1_STARTSTOP_CAPTURE/4) & 0xffff0000;
    Poke(pBA1+BA1_STARTSTOP_CAPTURE/4, temp1); // Stop Record task.

    FUNC_WPDD("-PDD_WaveInStop");
}


//*****************************************************************************
// "private_WaveInRestart()" --
//*****************************************************************************
VOID private_WaveInRestart()
{
    FUNC_WPDD("+PDD_WaveInRestart");
    private_WaveInStart( NULL );
    FUNC_WPDD("-PDD_WaveInRestart");
}


//*****************************************************************************
// "private_WaveStandby()"--
//*****************************************************************************
VOID
private_WaveStandby(WAPI_INOUT apidir )
{
    //
    // We are going to be idle. so power off what we can.
    //
} // private_WaveStandby()




//*****************************************************************************
// "private_WaveGetDevCaps()"--
//*****************************************************************************
MMRESULT
private_WaveGetDevCaps(WAPI_INOUT apidir, PVOID pCaps, UINT wSize)
{
    PWAVEINCAPS pwic = pCaps;
    PWAVEOUTCAPS pwoc = pCaps;

    MMRESULT mmRet = MMSYSERR_NOERROR;

    FUNC_WPDD("+PDD_WaveGetDevCaps");

    if (pCaps == NULL)
    {
             //
             // If pCaps == NULL, we are requesting if the driver PDD
             // is capable of this mode at all.  In other words, if
             // APIDIR == WAPI_IN, we return no error if input is
             // supported, and MMSYSERR_NOTSUPPORTED otherwise.  Since
             // Odo has both, we return MMSYSERR_NOERROR regardless.
        return(MMSYSERR_NOERROR);
    }
         //
         // Fill in the DevCaps here.
         //
    pwoc->wMid = MM_CRYSTAL;           // (MMREG.H)  Replace MM_MICROSOFT.
    pwoc->wPid = (apidir == WAPI_OUT ? MM_CRYSTAL_CS4232_WAVEOUT
                                     : MM_CRYSTAL_CS4232_WAVEIN );  // (MMREG.H)
    pwoc->vDriverVersion = 0x0001;
    wsprintf (pwoc->szPname, TEXT("CE/PC Audio (%hs)"), __DATE__);
    pwoc->dwFormats =   WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 |   // 11km8/16
                        WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16 |   // 11ks8/16
                        WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 |   // 22km8/16
                        WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16 |   // 22ks8/16
                        WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16 |   // 44km8/16
                        WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16;    // 44ks8/16

    pwoc->wChannels = 2;
    if(apidir == WAPI_OUT)
        pwoc->dwSupport = WAVECAPS_VOLUME;

    FUNC_WPDD("-PDD_WaveGetDevCaps");

    return(mmRet);
}  // private_WaveGetDevCaps()



//*****************************************************************************
// "private_WaveOpen()"--
//*****************************************************************************
MMRESULT
private_WaveOpen(WAPI_INOUT apidir, LPWAVEFORMATEX lpFormat,
                 BOOL fQueryFormatOnly)
{
    MMRESULT mmRet = MMSYSERR_NOERROR;

    FUNC_WPDD("+PDD_WaveOpen");

         //
         // Allow PCM, mono or stereo, 8 or 16 bit
         // at any frequency supported by the CS4235/9
         //
    if ((lpFormat->wFormatTag  == WAVE_FORMAT_PCM)      // If PCM
         &&  (lpFormat->nChannels         == 1   ||     //   and 1 or
              lpFormat->nChannels         == 2)         //       2 channels,
         &&  (lpFormat->wBitsPerSample    == 8   ||     //    and 8 or
               lpFormat->wBitsPerSample   == 16)        //        16 bit
         &&  (lpFormat->nSamplesPerSec    <= 48000))    //    and rate < 48000
                                                        //    OK so far.

    {
       if(apidir == WAPI_IN)   //  If record requested,
       {
           if(lpFormat->nSamplesPerSec < 5333)          // Check min record rate.
               mmRet = WAVERR_BADFORMAT;

       }
       else                   // play requested
       {
           if(lpFormat->nSamplesPerSec < 4150)          // Check min play rate.
               mmRet = WAVERR_BADFORMAT;
       }  // endif-else(apidir....)
    }  // endif

    if (fQueryFormatOnly || mmRet != MMSYSERR_NOERROR)
        goto EXIT;

         //
         // NOTE : If the hardware can only support either input or output one
         //        at a time, the driver should return MMSYSERR_ALLOCATED here.
         //
    if(apidir == WAPI_IN)   //  If record requested,
    {
        if (fRInUse)       //  and record in use,
        {
            mmRet = MMSYSERR_ALLOCATED;
            goto EXIT;
        }
        fRInUse = TRUE;
    }
    else                   // play requested
    {
        if (fPInUse)       // if play in use,
        {
            mmRet = MMSYSERR_ALLOCATED;
            goto EXIT;
        }
        fPInUse = TRUE;
    } // endif-else(apidir...)

         // Open with the given format.
    g_pwfx[apidir] = lpFormat;

    if (g_pwfx[apidir]->wBitsPerSample == 8)
    {
        if (g_pwfx[apidir]->nChannels == 1)
            g_pcmtype[apidir] = PCM_TYPE_M8;
        else
            g_pcmtype[apidir] = PCM_TYPE_S8;
    }
    else
    {
        if (g_pwfx[apidir]->nChannels == 1)
            g_pcmtype[apidir] = PCM_TYPE_M16;
        else
            g_pcmtype[apidir] = PCM_TYPE_S16;
    } // endif-else(g_pwfx...)

EXIT:
    FUNC_WPDD("-PDD_WaveOpen");
    return(mmRet);
} // private_WaveOpen()


//*****************************************************************************
//   "PDD_WaveProc()"----
//*****************************************************************************
MMRESULT
PDD_WaveProc(WAPI_INOUT apidir, DWORD dwCode,
             DWORD dwParam1,    DWORD dwParam2)
{
    MMRESULT mmRet = MMSYSERR_NOERROR;
    DWORD lVol, rVol;

    switch (dwCode)
    {
        case WPDM_CLOSE:          // (1--WAVEDDSI.H)
            if(apidir == WAPI_OUT)
                fPInUse = FALSE;
            else
            {
                fRInUse = FALSE;
                v_fMoreData[WAPI_IN] = FALSE;    // Only record ending notice.
            }
            break;

        case WPDM_CONTINUE:       // (2)
            if(apidir == WAPI_OUT)
               private_WaveOutContinue((PWAVEHDR) dwParam1);
            else
               private_WaveInContinue((PWAVEHDR) dwParam1);
            break;

        case WPDM_ENDOFDATA:      // (3)
            if(apidir == WAPI_OUT)
               private_WaveOutEndOfData();
            else
               mmRet=MMSYSERR_NOTSUPPORTED;
            break;

        case WPDM_GETDEVCAPS:     // (4)
            mmRet = private_WaveGetDevCaps(apidir, (PVOID) dwParam1, (UINT) dwParam2);
            break;

        case WPDM_GETVOLUME:      // (5)
            *((PULONG) dwParam1) = v_nVolume;
            break;

        case WPDM_OPEN:           // (6)
            mmRet = private_WaveOpen(apidir, (LPWAVEFORMATEX) dwParam1, (BOOL) dwParam2);
            break;

        case WPDM_PAUSE:          // (7)
            if(apidir == WAPI_OUT)
               private_WaveOutPause();
            else
               mmRet=MMSYSERR_NOTSUPPORTED;
            break;

        case WPDM_RESTART:        // (8)
            if (apidir == WAPI_OUT)
                private_WaveOutRestart((PWAVEHDR) dwParam1);
            else
                mmRet = MMSYSERR_NOTSUPPORTED;
            break;

        case WPDM_SETVOLUME:      // (9)
            v_nVolume = (ULONG) dwParam1;
                // Low order word = left volume, Hi order word = right volume.
                // 0x0000 = silence; 0xffff = full volume.
            lVol = (v_nVolume & 0x0000ffff) >> 1;  // Keep 15 bits left vol
            rVol = (v_nVolume & 0xfffe0000) >> 17; // Keep 15 bits right vol.

            lVol = ((lVol*lVol) >> 1) & 0x7fff0000;    // Square volume for
            rVol = ((rVol*rVol) >> 17) & 0x00007fff;   // better human response.

                // Convert volume to attenuation.
            lVol ^= 0xffff0000;     // Invert volume info & turn on bit 31.
            rVol ^= 0x0000ffff;     // Invert volume info & turn on bit 15.

                // Store the new volume in the audio chip.
            Poke(pBA1+BA1_PLAYBACK_VOLUME/4, lVol | rVol);
            break;

        case WPDM_STANDBY:        // (10)
            private_WaveStandby(apidir);
            break;

        case WPDM_START:          // (11)
            if (apidir == WAPI_OUT)
                private_WaveOutStart((PWAVEHDR) dwParam1);
            else
                private_WaveInStart((PWAVEHDR) dwParam1);
            break;

        case WPDM_STOP:           // (12)
            if (apidir == WAPI_OUT)
                private_WaveOutStop();
            else
                private_WaveInStop();
            break;

        default :
            mmRet = MMSYSERR_NOTSUPPORTED;
            break;
    } // endswitch(dwCode)

    return(mmRet);
} // PDD_WavePROC()


