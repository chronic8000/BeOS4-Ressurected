/* Copyright 2001, Be Incorporated. All Rights Reserved */
#include <PCI.h>

extern pci_module_info	*pci;

/*  PCI_CFG_RD/WR functions */
inline uint32
PCI_CFG_RD ( 	uchar	bus,
				uchar	device,	
				uchar	function,
				uchar	offset,
				uchar	size )
{									
	return (*pci->read_pci_config) (bus, device, function, offset, size);
}

inline void
PCI_CFG_WR ( 	uchar	bus,
				uchar	device,	
				uchar	function,
				uchar	offset,
				uchar	size,
				uint32	value )
{					
	(*pci->write_pci_config) (bus, device, function, offset, size, value);
}

/* PCI_IO_RD/WR functions */
inline uint8
PCI_IO_RD (int offset)
{
	return (*pci->read_io_8) (offset);
}

inline uint16
PCI_IO_RD_16 (int offset)
{
	return (*pci->read_io_16) (offset);
}

inline uint32
PCI_IO_RD_32 (int offset)
{
	return (*pci->read_io_32) (offset);
}

inline void
PCI_IO_WR (int offset, uint8 val)
{
	(*pci->write_io_8) (offset, val);
}

inline void
PCI_IO_WR_16 (int offset, uint16 val)
{
	(*pci->write_io_16) (offset, val);
}

inline void
PCI_IO_WR_32 (int offset, uint32 val)
{
	(*pci->write_io_32) (offset, val);
}
