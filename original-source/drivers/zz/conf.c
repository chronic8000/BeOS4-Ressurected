

#include	<config_manager.h>
#include	<isapnp.h>
#include	<PCI.h>
#include	<tty/rico.h>
#include	<stdlib.h>
#include	"zz.h"


struct pcimodem {
	ushort	vendor_id;
	ushort	device_id;
	uchar	class_base;
	uchar	class_sub;
};


static void	scanisa( );
static void scanpci( );
static void adddev( uint, uint, bool);
static uint	log2( uint);

static struct device	*dev;
static uint		ndev;


void
configure( struct device d[], uint n)
{

	dev = d;
	ndev = n;
	scanisa( );
	scanpci( );
}


static void
scanisa( )
{
	config_manager_for_driver_module_info
					*pnp;
	struct device_configuration	*c;
	struct isa_device_info		idi;
	resource_descriptor		rp,
					ri;
	uint				irq,
					j;
	uint64				u	= 0;
	static char			CM[]	= B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME;

	unless (get_module( CM, (module_info **)&pnp) == B_OK)
		return;
	memset( dev, 0, ndev*sizeof( *dev));
	while ((*pnp->get_next_device_info)( B_ISA_BUS, &u, &idi.info, sizeof idi) == B_OK) {
		if ((idi.info.flags & B_DEVICE_INFO_ENABLED)
		and (idi.info.flags & B_DEVICE_INFO_CONFIGURED)
		and (idi.info.config_status == B_OK)
		and (strncmp( idi.card_name, "PC-TEL HSP ", 11) != 0)
		and (j = (*pnp->get_size_of_current_configuration_for)( u))
		and (c = malloc( j))) {
			if (((*pnp->get_current_configuration_for)( u, c, j) == B_OK)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_IRQ_RESOURCE) == 1)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_DMA_RESOURCE) == 0)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_IO_PORT_RESOURCE) >= 1)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_MEMORY_RESOURCE) == 0)
			and ((*pnp->get_nth_resource_descriptor_of_type)( c, 0, B_IO_PORT_RESOURCE, &rp, sizeof rp) == B_OK)
			and ((*pnp->get_nth_resource_descriptor_of_type)( c, 0, B_IRQ_RESOURCE, &ri, sizeof ri) == B_OK)
			and (rp.d.r.len == 8)
			and (irq = log2( ri.d.m.mask))) {
				bool certified = idi.info.devtype.base==7 && idi.info.devtype.subtype==0;
				if (((*pnp->count_resource_descriptors_of_type)( c, B_IO_PORT_RESOURCE) == 1)
				or (certified))
					adddev( rp.d.r.minbase, irq, certified);
			}
			free( c);
		}
	}
	put_module( CM);
}


struct pcimodem mtab[10] = {
	/*
	 * LUCENT----------------
	 * bus 00 device 0f function 00: vendor 11c1 device 0480 revision 00
	 *   class_base = 07 class_function = 80 class_api = 00
	 *   line_size=00 latency_timer=00 header_type = 00 BIST=00
	 *   rom_base=00000000  pci 00000000 size=00000000
	 *   interrupt_line=05 interrupt_pin=01 min_grant=fc max_latency=0e
	 *   cardbus_cis=00000040 subsystem_id=0500 subsystem_vendor_id=1668
	 *   base reg 0: host addr ffbef800 pci ffbef800 size 00000100, flags 00
	 *   base reg 1: host addr 0000f800 pci 0000f800 size 00000100, flags 01
	 *   base reg 2: host addr 0000f400 pci 0000f400 size 00000100, flags 01
	 *   base reg 3: host addr 0000fff0 pci 0000fff0 size 00000008, flags 01
	 *   base reg 4: host addr 00000000 pci 00000000 size 00000000, flags 00
	 *   base reg 5: host addr 00000000 pci 00000000 size 00000000, flags 00
	 */
	0x11C1, 0x0480, PCI_simple_communications, PCI_simple_communications_other,

	/*
	 * Add more PCI modems here, but _not_ HSP.
	 */
};


static void
scanpci( )
{
	pci_module_info	*pci;
	pci_info	pi;

	if (get_module( B_PCI_MODULE_NAME, (module_info **)&pci) == B_OK) {
		uint i = 0;
		while ((*pci->get_nth_pci_info)( i++, &pi) == B_OK) {
			uint r = 0;
			uint m = 0;
			loop {
				if ((pi.vendor_id == mtab[m].vendor_id)
				and (pi.device_id == mtab[m].device_id)
				and (pi.class_base == mtab[m].class_base)
				and (pi.class_sub == mtab[m].class_sub)
				and (pi.u.h0.interrupt_line)
				and (pi.u.h0.base_register_flags[r] & PCI_address_space)
				and (pi.u.h0.base_register_sizes[r] == 8)) {
					adddev( pi.u.h0.base_registers[r], pi.u.h0.interrupt_line, TRUE);
					break;
				}
				if (++r == nel( pi.u.h0.base_registers)) {
					r = 0;
					if (++m == nel( mtab))
						break;
				}
			}
		}
		put_module( B_PCI_MODULE_NAME);
	}
}


static void
adddev( uint base, uint irq, bool certified)
{
	static uint standard[] = {
		0x3F8, 0x2F8, 0x3E8, 0x2E8
	};
	uint	i;

	for (i=0; i<ndev; ++i)
		unless (dev[i].base)
			if ((i < nel( standard) && base == standard[i])
			or (i >= nel( standard) && certified)) {
				dev[i].base = base;
				dev[i].intr = irq;
				break;
			}
}


static uint
log2( uint n)
{
	uint	i;

	for (i=0; i<32; ++i)
		if ((1<<i) == n)
			return (i);
	return (0);
}
