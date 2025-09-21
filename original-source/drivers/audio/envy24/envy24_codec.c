/* Copyright 2001, Be Incorporated. All Rights Reserved */
#include "pci_helpers.h"
#if THIRD_PARTY
#include "thirdparty.h"
#endif
#include "envy24.h"

// Definitions for the Codec types. 
// TT: these don't seem to be used anywhere.
const I2S_CODEC gI2SCodec[I2SID_MAX] =
{
    // I2SID_None
    {
        0,                         // bit width
        0,
        0
    },

    // I2SID_AKM_AK4393
    {
        24,                         // bit width
        I2S_96K_GPIO,
        I2S_VOL_NONE
    },

    // I2SID_AKM_AK5393
    {
        24,                         // bit width
        I2S_96K_GPIO,
        I2S_VOL_NONE
    },
    
    // I2SID_AKM_AK4524
    {
    	24,
    	I2S_96K_GPIO,
    	I2S_VOL_NONE
    }
};

// Functional Prototypes.
void CustHw_WriteGPIO( envy24_dev * card, uint8 bMask, uint8 bData);
status_t envy24_validate_sample_rate(multi_format_info *);
uint32 envy24_convert(uint32);

//
// Set the Sample Rate in the Hardware.
// Return B_OK if successful. Otherwise, return B_ERROR.
//
status_t envy24_codec_set_sample_rate(
    							envy24_dev * card,
    							bool 		 dwInternalClkSrc, 
    							uint32		 dwSampleRate      
)
{
    bool bDoubleFreq = FALSE;
	cpu_status cpu;

	ddprintf(("envy24: envy24_codec_set_sample_rate(%p, %d, %ld)\n",
		card, dwInternalClkSrc, dwSampleRate));

    if (dwInternalClkSrc) {
        // We are using the internal clock.

        uint8 bRegValue;

        bRegValue = 0;
	    bDoubleFreq = FALSE;		// default is not frequency doubling.

        switch(dwSampleRate) {
            case 48000:
                bRegValue = bRegValue;  // no change.
                break;
            case 24000:
                bRegValue = bRegValue | 0x0001;
                break;
            case 12000:
                bRegValue = bRegValue | 0x0002;
                break;
            case 9600:
                bRegValue = bRegValue | 0x0003;
                break;
            case 32000:
                bRegValue = bRegValue | 0x0004;
                break;
            case 16000:
                bRegValue = bRegValue | 0x0005;
                break;
            case 8000:
                bRegValue = bRegValue | 0x0006;
                break;
            case 44100:
                bRegValue = bRegValue | 0x0008;
                break;
            case 22050:
                bRegValue = bRegValue | 0x0009;
                break;
            case 11025:
                bRegValue = bRegValue | 0x000A;
                break;
            case 96000:
                bDoubleFreq = TRUE;
                bRegValue = bRegValue | 0x0007;
                break;
            case 88200:
                bDoubleFreq = TRUE;
                bRegValue = bRegValue | 0x000B;
                break;
            case 64000:
                // 64k is not supported in this hw.
            default:
                // No known or unsupported sample rate.
                //DEBUG_BREAK;
                return(B_ERROR);
        }

        // Write the Multi-Track Sample Rate Register.
	 	cpu = disable_interrupts();
		acquire_spinlock(&(card->spinlock));

			PCI_IO_WR(card->mt_iobase + MT_SAMPLE_RATE_REG, bRegValue);

		release_spinlock(&(card->spinlock));
	    restore_interrupts(cpu);

		// If you have custom hardware....
		#if 0		
        {
            CUST_HW_INSTANCE_DATA *pCust;
            pCust = (CUST_HW_INSTANCE_DATA *)dwCustHwHandle;
            // Store the current sample rate.
            pCust->dwCurrentSampleRate = dwSampleRate;
        }
		#endif
    }
    else {
        // We are using the external clock.

        // Set the SPDIF or External clock to be master.
	 	cpu = disable_interrupts();
		acquire_spinlock(&(card->spinlock));

			PCI_IO_WR(card->mt_iobase + MT_SAMPLE_RATE_REG, 0x010);

		release_spinlock(&(card->spinlock));
	    restore_interrupts(cpu);

		// If you have custom hardware....
        // Tell any customer specific hardware that the sample rate
        // has changed.
		#if 0		
			IceCust_SetParam(dwIceHwHandle,
							dwCustHwHandle,
							ICECUST_SPDIF_SAMPLERATE,
            				&(dwSampleRate), sizeof(DWORD), NULL, 0);
		#endif
    }

    //
    // For the ICE Reference boards, we are using GPIO pin 0 for
    // the toggling of sample rate to the codec to support rate
    // above 48k.
    //

    if (bDoubleFreq==TRUE) {
        // We need to double the Frequency.
#if THIRD_PARTY 
        if (card->teepee.dwSubVendorID == THIRD_PARTY_ID) {
        	/* TT: weird. Seems to work fine doing either this or
        	** the stuff in the else case.
        	*/
        	ak4524_set_double_mode(card, true);
        } else
#endif
        {
	        uint8 bOldValue;
	
	        // Set the GP IO Bit 0 to '1' to double sample rate.
			CustHw_WriteGPIO(card, 0xfc, 0x03);
	
	        // For the AKM Part, we need to change the MClk/LClk Ratio to 128.
		 	cpu = disable_interrupts();
			acquire_spinlock(&(card->spinlock));
	
		        bOldValue = PCI_IO_RD( card->mt_iobase + MT_DATA_FORMAT_REG);
		        bOldValue = bOldValue | 0x08;   // Change to 128x clock. (BIT3)
		        PCI_IO_WR( card->mt_iobase + MT_DATA_FORMAT_REG, bOldValue);
	
			release_spinlock(&(card->spinlock));
		    restore_interrupts(cpu);
		}
    }
    else {
        // No need to double the Frequency.
#if THIRD_PARTY 
        if (card->teepee.dwSubVendorID == THIRD_PARTY_ID) {
        	/* TT: weird. Seems to work fine doing either this or
        	** the stuff in the else case.
        	*/
        	ak4524_set_double_mode(card, false);
        } else
#endif
        {
	        uint8 bOldValue;
	
	        // Set the GP IO Bit 0 to '0' to disable double sample rate.
			CustHw_WriteGPIO(card, 0xfc, 0x02);
	
	        // For the AKM Part, we need to change the MClk/LClk Ratio to 256.
		 	cpu = disable_interrupts();
			acquire_spinlock(&(card->spinlock));
	
		        bOldValue = PCI_IO_RD( card->mt_iobase + MT_DATA_FORMAT_REG);
		        bOldValue = bOldValue & (~0x08); //BIT3   
		        PCI_IO_WR( card->mt_iobase + MT_DATA_FORMAT_REG, bOldValue);
	
			release_spinlock(&(card->spinlock));
		    restore_interrupts(cpu);
		}
    }

    return(B_OK);

}   

status_t envy24_validate_sample_rate(
									multi_format_info * pmfi)
{
	status_t err = B_OK;

	if (pmfi->output.rate != pmfi->input.rate) {
		err = B_BAD_VALUE;
	} 

	if (err == B_OK ) {
        switch(pmfi->output.rate) {
            case B_SR_48000:
            case B_SR_24000:
            case B_SR_12000:
/* Not supported by api?!           case B_SR_9600: */
            case B_SR_32000:
            case B_SR_16000:
            case B_SR_8000:
            case B_SR_44100:
            case B_SR_22050:
            case B_SR_11025:
            case B_SR_96000:
            case B_SR_88200:
				err = B_OK;
                break;
            case B_SR_64000:
                // 64k is not supported in this hw.
            default:
				err = B_BAD_VALUE;
        }

	}
	return err;
}

uint32 envy24_convert(
						uint32 sr)
{
	if (sr < 8000 ) {
        switch(sr) {
            case B_SR_48000:
				return 48000;
            case B_SR_24000:
				return 24000;
            case B_SR_12000:
				return 12000;
            case B_SR_32000:
				return 32000;
            case B_SR_16000:
				return 16000;
            case B_SR_8000:
				return 8000;
            case B_SR_44100:
				return 44100;
            case B_SR_22050:
				return 22050;
            case B_SR_11025:
				return 11025;
            case B_SR_96000:
				return 96000;
            case B_SR_88200:
				return 88200;
            case B_SR_64000:
				return 64000;
            default:
				return 0;
        }
	}
	else {
        switch(sr) {
            case 48000:
				return B_SR_48000;
            case 24000:
				return B_SR_24000;
            case 12000:
				return B_SR_12000;
            case 32000:
				return B_SR_32000;
            case 16000:
				return B_SR_16000;
            case 8000:
				return B_SR_8000;
            case 44100:
				return B_SR_44100;
            case 22050:
				return B_SR_22050;
            case 11025:
				return B_SR_11025;
            case 96000:
				return B_SR_96000;
            case 88200:
				return B_SR_88200;
            case 64000:
				return B_SR_64000;
            default:
				return 0;
        }
	}
}
