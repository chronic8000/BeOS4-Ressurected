

#include	<config_manager.h>
#include	<isapnp.h>
#include	<tty/rico.h>
#include	"zz.h"


static uint	log2( uint);
void		*malloc( size_t);


void
configure( struct device dev[], uint ndev)
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
	static uint standard[] = {
		0x3F8, 0x2F8, 0x3E8, 0x2E8
	};

	unless (get_module( CM, (module_info **)&pnp) == B_OK)
		return;
	memset( dev, 0, ndev*sizeof( *dev));
	while ((*pnp->get_next_device_info)( B_ISA_BUS, &u, &idi, sizeof idi) == B_OK) {
		if (!(idi.info.flags & B_DEVICE_INFO_ENABLED) ||
			!(idi.info.flags & B_DEVICE_INFO_CONFIGURED) ||
			!(idi.info.config_status == B_OK))
			continue;
		if (strncmp( idi.card_name, "PC-TEL HSP ", 11) == 0) {
			dprintf( "PC-TEL HSP modem ignored\n");
			continue;
		}
		if ((j = (*pnp->get_size_of_current_configuration_for)( u))
		and (c = malloc( j))) {
			if (((*pnp->get_current_configuration_for)( u, c, j) == B_OK)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_IRQ_RESOURCE) == 1)
			and ((*pnp->count_resource_descriptors_of_type)( c, B_DMA_RESOURCE) == 0)
			and (
					((*pnp->count_resource_descriptors_of_type)( c, B_IO_PORT_RESOURCE) == 1) ||
					/* some motherboards report more than one i/o range for
					 * serial ports (i.e. 0x3f8 and 0x7f8). don't allow this
					 * for ISA PnP devices because such devices are usually
					 * soft modems */
					(((*pnp->count_resource_descriptors_of_type)( c, B_IO_PORT_RESOURCE) >= 1) && (idi.info.devtype.base == 7) && (idi.info.devtype.subtype == 0)))
			and ((*pnp->count_resource_descriptors_of_type)( c, B_MEMORY_RESOURCE) == 0)
			and ((*pnp->get_nth_resource_descriptor_of_type)( c, 0, B_IO_PORT_RESOURCE, &rp, sizeof rp) == B_OK)
			and ((*pnp->get_nth_resource_descriptor_of_type)( c, 0, B_IRQ_RESOURCE, &ri, sizeof ri) == B_OK)
			and (rp.d.r.len == 8)
			and (irq = log2( ri.d.m.mask))) {
				uint base = rp.d.r.minbase;
				uint i = 0;
				loop {
					if (base == standard[i]) {
						dev[i].base = base;
						dev[i].intr = irq;
						break;
					}
					if (++i == nel( standard)) {
						if ((idi.info.devtype.base == 7)
						and (idi.info.devtype.subtype == 0))
							for (; i<ndev; ++i)
								unless (dev[i].base) {
									dev[i].base = base;
									dev[i].intr = irq;
									break;
								}
						break;
					}
				}
			}
			free( c);
		}
	}
	put_module( CM);
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
