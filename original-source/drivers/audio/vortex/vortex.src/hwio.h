/************************************************************************/
/*                                                                      */
/*                              hwio.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef HWIO_H_INCLUDED
#define HWIO_H_INCLUDED

#include <KernelExport.h>
#include <PCI.h>
#include "audio_drv.h"

typedef struct _mem_mapped_regs_t {
	area_id areaId;
	int32 lock;
	vuchar* base;
} mem_mapped_regs_t;

/* */
uint32 pci_config_read(
						sound_card_t *card,
						uchar	offset,		/* offset in configuration space */
						uchar	size		/* # bytes to read (1, 2 or 4) */
						);

void pci_config_write(
						sound_card_t *card,
						uchar	offset,		/* offset in configuration space */
						uchar	size,		/* # bytes to write (1, 2 or 4) */
						uint32	value,		/* value to write */
						uint32	mask		/* to change the specified bits only */
						);

#endif

/* */
status_t init_mem_mapped_regs(mem_mapped_regs_t* regs, const char* name,
								void *physical_address, size_t size );
void uninit_mem_mapped_regs(mem_mapped_regs_t* regs);

uchar mapped_read_8(mem_mapped_regs_t* regs, int regno);
uint16 mapped_read_16(mem_mapped_regs_t* regs, int regno);
uint32 mapped_read_32(mem_mapped_regs_t* regs, 	int regno);
void mapped_write_8(mem_mapped_regs_t* regs, int regno, uchar value, uchar mask);
void mapped_write_16(mem_mapped_regs_t* regs, int regno, uint16 value, uint16 mask);
void mapped_write_32(mem_mapped_regs_t* regs, int regno, uint32 value, uint32 mask);



typedef struct _pci_io_ports_t {
	int32 lock;
	int base;
} pci_io_ports_t;

void setup_pci_io_ports(pci_io_ports_t* ports, int base);
uint8 pci_io_port_read_8(pci_io_ports_t* ports, int offset);		
void pci_io_port_write_8(pci_io_ports_t* ports, int offset, 
								uint8 value, uint8 mask);	


