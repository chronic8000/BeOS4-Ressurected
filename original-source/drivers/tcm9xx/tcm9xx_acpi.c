/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_acpi.c
 * Description:	ACPI (Advanced Configuration and Power Interface) support
 * Changes:		Feb 21, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

/*
 * tcm9xx_ACPI_set_WOL()
 * Description:	Change from D0 (active) to D3 (sleep).
 * Note:		The Cyclone forgets all PCI config info during the transition
 *				from D0(active) to D3(sleep)! 
 *
 */
void tcm9xx_ACPI_set_WOL( device_info_t *device ) {
    SET_WINDOW( 7 );
    /* Power up on: 1==Downloaded Filter, 2==Magic Packets, 4==Link Status. */
    write16( device->base + 0x0c, 2 );
    /* The RxFilter must accept the WOL frames. */
    write16( device->base + Command, SetRxFilter | RxStation | RxMulticast | RxBroadcast );
    write16( device->base + Command, RxEnable );
    /* Change the power state to D3; RxEnable doesn't take effect. */
    gPCIModInfo->write_pci_config( device->pciInfo->bus, device->pciInfo->device,
                                   device->pciInfo->function, 0xe0, 2, 0x8103 );
}

/*
 * tcm9xx_ACPI_wake()
 * Description:	Change from D3 (sleep) to D0 (active).
 * Note:		Since the Cyclone forgets all PCI config info during the 
 * 				D0->D3 transition, we must restore the settings ourselves. 
 *
 */
void tcm9xx_ACPI_wake( int bus, int device, int devfn ) {
    uint32 base0, base1, romaddr;
    uint16 pci_command, pwr_command;
    uint8 pci_latency, pci_cacheline, irq;

    pwr_command = gPCIModInfo->read_pci_config( bus, device, devfn, 0xe0, 2 );
    if ( ( pwr_command & 3 ) == 0 )
        return ;

    pci_command = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_command, 2 );
    base0 = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_base_registers, 4 );
    base1 = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_base_registers + sizeof( int32 ), 4 );
    romaddr = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_rom_base, 4 );
    pci_latency = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_latency, 1 );
    pci_cacheline = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_line_size, 1 );
    irq = gPCIModInfo->read_pci_config( bus, device, devfn, PCI_interrupt_line, 1 );

    gPCIModInfo->write_pci_config( bus, device, devfn, 0xe0, 2, 0x0000 );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_base_registers, 4, base0 );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_base_registers + sizeof( int32 ), 4, base1 );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_rom_base, 4, romaddr );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_interrupt_line, 1, irq );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_latency, 1, pci_latency );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_line_size, 1, pci_cacheline );
    gPCIModInfo->write_pci_config( bus, device, devfn, PCI_command, 2, pci_command | 5 );
}
