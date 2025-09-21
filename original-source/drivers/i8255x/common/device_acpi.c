#define	ACPI_D0		0
#define ACPI_D1		1
#define ACPI_D2		1
#define ACPI_D3		1

/*
 * acpi_set_power_state()
 * Description: Set the ACPI power state on the card
 */
uint16 pci_set_power_state( pci_info *pciInfo, int32 new_state ) {
	uint32	base_reg[5], rom_address;
	uint16	pci_command, power_command;
	uint8	pci_latency, pci_line_size;
	uint8	irq;
	int32	i, old_state;
	uint32	bus = pciInfo->bus;
	uint32	device = pciInfo->device;
	uint32	devfn = pciInfo->function;

	power_command = gPCIModInfo->read_pci_config( bus, device, devfn, 0xE0, 2 );
	old_state = power_command & 0x3;

	if ( old_state == new_state )
		return old_state;	// Nothing to do if we stay the same.

	if ( old_state == ACPI_D3 ) {
		/*
		 * Grab the PCI config data from the card
		 */
		pci_command = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_command, 2 );
		for ( i = 0; i < 5; i++ ) {
			base_reg[ i ] = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_base_registers + ( i * sizeof( uint32 ) ), 4 );
		}
		rom_address = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_rom_base, 4 );
		pci_latency = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_latency, 1 );
		pci_line_size = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_line_size, 1 );
		irq = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_interrupt_line, 1 );

		/*
		 * Set the new power state and write the config data out
		 */ 
		gPCIModInfo->write_pci_config( bus, device, devfn, 0xE0, 2, new_state );
		for ( i = 0; i < 5; i++ ) {
			gPCIModInfo->write_pci_config( bus, device, devfn, PCI_base_registers + ( i * sizeof( uint32 ) ), 4, base_reg[ i ] );
		}
		gPCIModInfo->write_pci_config( bus, device, devfn, PCI_rom_base, 4, rom_address );
		gPCIModInfo->write_pci_config( bus, device, devfn, PCI_latency, 1, pci_latency );
		gPCIModInfo->write_pci_config( bus, device, devfn, PCI_line_size, 1, pci_line_size );
		gPCIModInfo->write_pci_config( bus, device, devfn, PCI_interrupt_line, 1, irq );
	} else {
		gPCIModInfo->write_pci_config( bus, device, devfn, 0xE0, 2, ( power_command & ~0x0003 ) | new_state );
	}

	return old_state;
}
