static void set_bus_mastering(pci_info *info)
{
	// Turn on Memory Address Decode, and Bus Mastering
	uint32 cmd = gPCIModInfo->read_pci_config(info->bus, info->device, info->function, PCI_command, 2);
	cmd |= PCI_command_io | PCI_command_memory | PCI_command_master;
	gPCIModInfo->write_pci_config(info->bus, info->device, info->function, PCI_command, 2, cmd);
}

/*
 * enable_addressing
 *
 * Set up registers for DMA.
 */
static area_id map_pci_memory(device_info_t *dev, size_t MemoryReg, void**addr)
{
	ulong base, size, offset;
	area_id ret;
	
	base = dev->pciInfo->u.h0.base_registers[MemoryReg];
	size = dev->pciInfo->u.h0.base_register_sizes[MemoryReg];
	
	// Round down to nearest page boundary
	base &= ~(B_PAGE_SIZE-1);
	
	// Adjust the size
	offset = dev->pciInfo->u.h0.base_registers[MemoryReg] - base;
	size += offset;
	
	// Round up to nearest page boundary
	size = (size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	
	ret = map_physical_memory(
		kDevName " Regs",
		(void *)base, size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		addr
	);
	if (ret<B_OK)
	{
		DEBUG_DEVICE(ERR,"map_pci_memory(): Attempt to map %ld bytes at 0x%lx failed!\n",
		dev,size,base);
		return ret;
	}
	*addr = (void*)((char*)*addr + offset);
	
	return ret;
}
