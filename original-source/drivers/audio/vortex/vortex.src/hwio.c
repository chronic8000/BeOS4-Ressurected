/************************************************************************/
/*                                                                      */
/*                              hwio.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include "hwio.h"
#include "debug.h"

#define PCI_WRITE_DEBUG 0

extern pci_module_info *pci;


/***************************************************
 * PCI Configuration Register Read/Write Functions *
 ***************************************************/

/* 
	Reads <size> bytes from a pci configuration register 
	*/ 
uint32 pci_config_read(
						sound_card_t *card,
						uchar	offset,		/* offset in configuration space */
						uchar	size		/* # bytes to read (1, 2 or 4) */
						)
{
	uint32 ret;
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	ret = (*pci->read_pci_config) (card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function, offset, size);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
	return ret;
}
	
/* 
	Writes <size> bytes to a pci configuration register 
	*/ 
void pci_config_write(
						sound_card_t *card,
						uchar	offset,		/* offset in configuration space */
						uchar	size,		/* # bytes to write (1, 2 or 4) */
						uint32	value,		/* value to write */
						uint32	mask		/* to change the specified bits only */
						)
{ 
	uint32 old;
	cpu_status cp;
	if (mask == 0) return;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	if (mask != 0xffffffff) {
		old = (*pci->read_pci_config) (card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function, offset, size);
		value = (value & mask) | (old & ~mask);
	}

	(*pci->write_pci_config) (card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function, offset, size, value);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

}

/***********************************************
 * Memory Mapped Register Read/Write Functions *
 ***********************************************/
status_t init_mem_mapped_regs(mem_mapped_regs_t* regs, const char* name,
								void *physical_address, size_t size )
{
	void * base;
	regs->lock = 0;
	regs->areaId = -1;
	regs->areaId = map_physical_memory(name, 
							physical_address,
							size,
							B_ANY_KERNEL_ADDRESS, 
							B_READ_AREA | B_WRITE_AREA,
							&base
							);
	if (regs->areaId < 0) 
	{
		ddprintf("mapping PCI Audio Memory failed!\n"); 
		return regs->areaId;
	}
	regs->base = (vuchar*)base;
	return B_OK;
}

void uninit_mem_mapped_regs(mem_mapped_regs_t* regs)
{
	if(regs->areaId >= 0) delete_area(regs->areaId);
}

/* 
	Read a byte from a memory mapped register 
	*/ 
uchar mapped_read_8(mem_mapped_regs_t* regs, int regno)
{
	uchar ret;
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	ret = *(regs->base + regno);

	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
	return ret;
}

/* 
	Read a word from a memory mapped register 
	*/ 
uint16 mapped_read_16(mem_mapped_regs_t* regs, int regno)
{
	uint16 ret;
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	ret = *((vuint16 *) (regs->base + regno));

	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
	return ret;
}

/* 
	Read a double word from a memory mapped register 
	*/ 
uint32 mapped_read_32(mem_mapped_regs_t* regs, 	int regno)
{
	uint32 ret;
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	ret =  *((vuint32 *) (regs->base + regno));

	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
	return ret;
}

/* 
	Write a byte to a memory mapped register 
	*/ 
void mapped_write_8(mem_mapped_regs_t* regs,
					 int regno, uchar value, uchar mask)
{
	cpu_status cp = 0;
	
	if (mask == 0) return;

	cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	if (mask != 0xff) {
		uchar old = *(regs->base + regno);
		value = (value&mask)|(old&~mask);
	}
	*(regs->base + regno) = value;

	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
}

/* 
	Write a word to a memory mapped register 
	*/ 
void mapped_write_16(mem_mapped_regs_t* regs,
						int regno, uint16 value, uint16 mask)
{
	cpu_status cp = 0;
	
	if (mask == 0) return;

	cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	if (mask != 0xffff) {
		uint16 old = *((vuint16 *) (regs->base + regno));
		value = (value&mask)|(old&~mask);
	}
	*((vuint16 *) (regs->base + regno)) = value;

	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
}

/* 
	Write a double word to a memory mapped register 
	*/ 
void mapped_write_32(mem_mapped_regs_t* regs,
						int regno, uint32 value, uint32 mask)
{
	cpu_status cp = 0;
	
	if (mask == 0) return;

	cp = disable_interrupts();
	acquire_spinlock(&(regs->lock));

	if (mask != 0xffffffff) {
		uint32 old = *((vuint32 *) (regs->base + regno));
		value = (value&mask)|(old&~mask);
	}
	*((vuint32 *) (regs->base + regno)) = value;
	
	release_spinlock(&(regs->lock));
	restore_interrupts(cp);
}



//PCI Port I/O Functions
void setup_pci_io_ports(pci_io_ports_t* ports, int base)
{
	ports->base = base;
	ports->lock = 0;
}

uint8 pci_io_port_read_8(pci_io_ports_t* ports, int offset)
{
	uint8 ret;
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(ports->lock));
	
	ret = (*pci->read_io_8)(ports->base + offset);

	release_spinlock(&(ports->lock));
	restore_interrupts(cp);
	return ret;
}

void pci_io_port_write_8(pci_io_ports_t* ports, int offset, uint8 value, uint8 mask)
{
	cpu_status cp;
	
	if (mask == 0) return;

	cp = disable_interrupts();
	acquire_spinlock(&(ports->lock));

	if (mask != 0xff) {
		uint8 old = (*pci->read_io_8)(ports->base + offset);
		value = (value & mask)|(old & ~mask);
	}

	(*pci->write_io_8)(ports->base + offset, value);

	release_spinlock(&(ports->lock));
	restore_interrupts(cp);

}

