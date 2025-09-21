/* Copyright 2001, Be Incorporated. All Rights Reserved */
/* includes */
#include "pci_helpers.h"
#include "envy24.h"

/* Function to read the E2Prom from the I2C bus    */
/* DANGER, WILL ROBINSON, DANGER                   */
/* ONLY call this function from a serialized       */
/* routine as no spinlocks or semaphores are used! */

uint8 read_eeprom(uint32 icebase, uint8 prom_offset)
{
    uint8  bStatus;
    int32 iSanity;

    /* Make sure the bus is Idle. */
    bStatus = 0x03;
    iSanity = 10000;
    while ((bStatus & 0x03)!=0) {
        bStatus = PCI_IO_RD(icebase+ICE_I2C_PORT_CONTROL_REG);

        iSanity--;
        if (iSanity<0) {
            /* Error: Hw is not responding. */
            dprintf("envy24: i2c bus not idle\n");
            //?? how do we handle this case?
            return(0);
        }
    }

    /* Rev1 workaround. We wait for an additional 1.5 milliseconds. */
	snooze(1500);

    /* Write the E2Prom Offset. */
    PCI_IO_WR(icebase+ICE_I2C_PORT_ADDR_REG, prom_offset);

    /* Write the Device Address and trigger the Read. */
    PCI_IO_WR(icebase+ICE_I2C_DEVICE_ADDR_REG, ICE_I2C_E2PROM_ADDR&0xFE);

    /* Make sure the bus is Idle. */
    bStatus = 0x03;
    iSanity = 10000;
    while ((bStatus & 0x03)!=0) {
        bStatus = PCI_IO_RD(icebase+ICE_I2C_PORT_CONTROL_REG);

        iSanity--;
        if (iSanity<0) {
            /* Error: Hw is not responding. */
            dprintf("envy24: i2c bus not idle.\n");
            //?? how do we handle this case?
            return(0);
        }
    }

    return(PCI_IO_RD(icebase+ICE_I2C_PORT_DATA_REG));

}  

/* Function to read the E2Prom from the I2C bus. */
status_t read_eeprom_contents(uint32 icebase, char * prom, uint8 size)
{
    uint8 i;
    
	i = read_eeprom( icebase, 4);
    /* Read the size and version. */
	if ( i > size) {
		dprintf("envy24: size mismatch in eeprom\n");
		size = i;
	}

    if (read_eeprom( icebase, 5) !=0x01 ) {
		dprintf("envy24: version mismatch in eeprom\n");
        return B_ERROR;
    }

    /*  Read the entire Prom contents. */
    for (i=0; i< size ;i++) {
        prom[i] = read_eeprom( icebase, i);
    }
    return B_OK;
}


// Read the initial state of GPIO and Program the GPIO to the
// initial state.
// Must be called from a thread safe environment
void InitGpIo(envy24_dev * card)
{
	// Read the power strapped value of GPIO.
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	card->bGpIoInitValue = PCI_IO_RD(card->ctlr_iobase + ICE_DATA_REG);
	
	//
	// Write the GPIO initial state.
	//
	
	// Write the Write Mask.
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_WRITEMASK);
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, card->teepee.bGPIOInitMask);
	
	// Write the Data.
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, card->teepee.bGPIOInitState);
	
	// Write the Direction.
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DIRECTION);
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, card->teepee.bGPIODirection);
	
}

//-----------------------------------------------------------------------------
// CustHw_WriteGPIO:
// Write to GPIO port. Only bits that are unmasked as writable are affected.
// Note that the mask is reverse of the orinary notion, with value 0 defined as
// writable.
//-----------------------------------------------------------------------------
void CustHw_WriteGPIO( envy24_dev * card, uint8 bMask, uint8 bData)
{
    uint8    	bOrigDir;       // Original setting for direction registor
	cpu_status	cpu;
	
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));

	    // Enable GPIO write mask: 0 means corresponding bit can be written
		PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_WRITEMASK);
		PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, bMask);

	    // Save setting for dir reg
		// This isn't used but are there timing considerations?
		PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DIRECTION);
		bOrigDir = PCI_IO_RD(card->ctlr_iobase + ICE_DATA_REG);

#ifdef DBG
	    // Make sure the bits we want to set is driven as output
	    // Note: 1 => output
	    if ((bOrigDir & (uint8)~bMask) != (uint8)~bMask) {
	        _asm int 3;
	    }
#endif

	    // Write data
		PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
		PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, bData);
	
	    // Turn back on the read only mask
		PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_WRITEMASK);
		PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, 0xff);

	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);

}   // end of CustHw_WriteGPIO()

void set_gain(envy24_dev * card, uint16 channel)
{

	cpu_status	cpu;
	
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));

		PCI_IO_WR(card->mt_iobase + MT_VOL_IDX_REG, (uint8) channel);
		PCI_IO_WR_16(card->mt_iobase + MT_VOL_CTL_REG, card->volume[channel]);

	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);
}


static uint16 shifty[10] = { 0, 8, 2, 10, 4, 12, 6, 14, 0, 2 }; 
static uint16 valuable[12]={ 0, 2, 2,  2, 2,  2, 2,  2, 2, 3, 3, 1}; 						

void set_mux(envy24_dev * card, uint16 ctl_idx)
{
	uint16 reg;
	cpu_status	cpu;

	int		offset;
	uint16	shift = shifty[ctl_idx];	//find the appropriate bits
	uint16  value = 1;					//value for reg 30 or 32h 
	uint16  idx = 0;        			//value from 0 - 11 (the 12 inputs to the mux)

//	dprintf("envy24: set_mux\n");
	
	while ( value < 0x1000 && !(value & card->mux[ctl_idx])) {
		value = value << 1;
		idx++;
	}
	value = valuable[idx];

	if (ctl_idx < 8) {
		offset = MT_ROUTE_CTL_REG_PSDOUT;
	}
	else {
		offset = MT_ROUTE_CTL_REG_SPDOUT;
	}
				
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));

		reg = PCI_IO_RD_16(card->mt_iobase + offset);
		reg = reg & ~(3 << shift);	//clear
		reg |= (value << shift);	//set						
		PCI_IO_WR_16(card->mt_iobase + offset, reg);

		if (value > 1 && offset == MT_ROUTE_CTL_REG_SPDOUT) {
			//more_processing_required for SPDOUT
			shift = ((shift) ? 12 : 8);
			reg	= reg & ~(0xf << shift);	//clear
			reg |= (idx - 1) << shift;      //set--idx is channel number!
			PCI_IO_WR_16(card->mt_iobase + offset, reg);
		}
		else if (value > 1) {
			//more_processing_required for PSDOUT
			//this is the nasty one....
			uint32 big_reg = PCI_IO_RD_32(card->mt_iobase + MT_CAP_ROUTE_REG);
			shift = (ctl_idx - 1) << 2;				// * 4
			big_reg = big_reg & (~0xf << shift);	//clear
			big_reg |= ((uint32)(idx - 1)) << shift;
			PCI_IO_WR_32(card->mt_iobase + MT_CAP_ROUTE_REG, big_reg);
		}
	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);
		
	return;	
}


void set_enable(envy24_dev * card, uint16 enable)
{
	cpu_status	cpu;
	
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));

		PCI_IO_WR(card->mt_iobase + MT_DMIX_MONITOR_ROUTE_REG, enable ? 0x01 : 0x0 );

	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);
		
	return;	
}
