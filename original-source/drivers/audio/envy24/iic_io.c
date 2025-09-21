/* Based on work by F.M.Birth. */

#include "iic.h"
#include "thirdparty.h"
#include "pci_helpers.h"

void iic_set_iic(envy24_dev* card, uint8 sda, uint8 scl)
{
	cpu_status	cpu;
	uint8 reg;
	
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));
	
	// Set direction: SDA and SCL to output
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DIRECTION);
	reg = PCI_IO_RD(card->ctlr_iobase + ICE_DATA_REG);
	reg |= (ICE_PIN_SCL | ICE_PIN_SDA);	// 1 -> output
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	// set write mask: 0 means corresponding bit can be written
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_WRITEMASK);
	reg = ~(ICE_PIN_SCL | ICE_PIN_SDA);	// writable
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	// set line state
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	reg = ICE_PIN_RW;
	if (sda) reg += ICE_PIN_SDA;
	if (scl) reg += ICE_PIN_SCL;
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);
}

uint8 iic_get_sda(envy24_dev* card)
{
	cpu_status	cpu;
	uint8 reg;
	uint8 sda;
	
	cpu = disable_interrupts();
	acquire_spinlock(&(card->spinlock));
		
	// set write mask: 0 means corresponding bit can be written
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_WRITEMASK);
	reg = ~(ICE_PIN_RW);	// writable
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	// set RW line LOW
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	reg = 0;	// writeable
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	// Set direction: SDA to input and SCL to output
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DIRECTION);
	reg = PCI_IO_RD(card->ctlr_iobase + ICE_DATA_REG);
	reg |= ICE_PIN_SCL;	// SCL output = 1
	reg &= ~ICE_PIN_SDA;	// SDA input = 0
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	// Get SDA line state
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	sda = PCI_IO_RD(card->ctlr_iobase + ICE_DATA_REG);
	
	// set RW line HIGH
	PCI_IO_WR(card->ctlr_iobase + ICE_INDEX_REG, ICE_IDXREG_GPIO_DATA);
	reg = ICE_PIN_RW;	// writeable
	PCI_IO_WR(card->ctlr_iobase + ICE_DATA_REG, reg);
	
	release_spinlock(&(card->spinlock));
    restore_interrupts(cpu);
    
    return ((sda & ICE_PIN_SDA) == ICE_PIN_SDA);
}
