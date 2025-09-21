#include <Drivers.h>
#include <stdlib.h>
#include <KernelExport.h>
#include <ISA.h>
#include <config_manager.h>
#include <isapnp.h>
#include <parallel_driver.h>
#include "parallel.h"


static uint32 log2(uint n);
static int find_location(par_port_info *port_info, int io_base);


#ifdef __INTEL__

status_t configure(par_port_info *port_info)
{
	int device_index = 0;
	uint64 u = 0;
	const char	*CM = B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME;

	par_port_info *d;
	config_manager_for_driver_module_info *pnp;
	struct device_configuration *device;
	struct device_info idi;
	resource_descriptor	rp;
	uint j;
	int i;
	status_t err;

	/* Initialize the ports */
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		d = port_info+i;
		d->exists = false;
		d->io_base = 0;
		d->type = 0;
		d->open = 0;
		d->open_mode = 0;
		d->irq_sem = -1;
		d->hw_lock = -1;
		d->buffer = 0;
		d->fifosize = 0;
		d->threshold_read = 0;
		d->threshold_write = 0;
		d->irq_num = M_IRQ_NONE;
		d->device_dma.dma_channel = -1;
	}

	/* Regular parallel ports */
	port_info[0].io_base = 0x378;
	port_info[1].io_base = 0x278;
	port_info[2].io_base = 0x3BC;


	if ((err = get_module(CM, (module_info **)&pnp)) != B_OK)
		return err;

	while ((*pnp->get_next_device_info)(B_ISA_BUS, &u, &idi, sizeof(idi)) == B_OK)
	{
		if (!(idi.flags & B_DEVICE_INFO_ENABLED) ||
			!(idi.flags & B_DEVICE_INFO_CONFIGURED) ||
			!(idi.config_status == B_OK))
			continue;

		if ((idi.devtype.base != 7) || (idi.devtype.subtype != 1))
			continue; /* Not a parallel port */
				
		DD(bug("parallel: config:\n"));

		if ((j = (*pnp->get_size_of_current_configuration_for)(u)) == 0)
			continue; /* Should never happen */

		if ((device = (struct device_configuration *)malloc(j)) == 0)
			continue; /* Should not happen */
		

		if ((*pnp->get_current_configuration_for)(u, device, j) == B_OK)
		{ /* Got _current_ configuration of this device */

			int nb_pio = (*pnp->count_resource_descriptors_of_type)(device, B_IO_PORT_RESOURCE);
			int nb_irq = (*pnp->count_resource_descriptors_of_type)(device, B_IRQ_RESOURCE);
			int nb_dma = (*pnp->count_resource_descriptors_of_type)(device, B_DMA_RESOURCE);

			DD(bug("parallel: config: count B_IO_PORT_RESOURCE  %d\n", nb_pio));
			DD(bug("parallel: config: count B_IRQ_RESOURCE      %d\n", nb_irq));
			DD(bug("parallel: config: count B_DMA_RESOURCE      %d\n", nb_dma));

			if (nb_pio >= 1)
			{ /* this device has at least an IO port */
				if ((*pnp->get_nth_resource_descriptor_of_type)(device, 0, B_IO_PORT_RESOURCE, &rp, sizeof(rp)) != B_OK)
					goto error;

				DD(bug("parallel: config: IO address : 0x%x\n", rp.d.r.minbase));					

				/* Try to find this device in our device list */
				device_index = find_location(port_info, rp.d.r.minbase);
				if (device_index < 0)
					goto error;

				d = &(port_info[device_index]);
				d->exists = true;
				d->io_base = rp.d.r.minbase;

				if (nb_pio == 2)
				{ /* This is probably an ECP port */
					if ((*pnp->get_nth_resource_descriptor_of_type)(device, 1, B_IO_PORT_RESOURCE, &rp, sizeof(rp)) != B_OK)
						goto error;
					if (d->io_base + SDFIFO == rp.d.r.minbase)
						d->type |= (PAR_TYPE_ECP | PAR_TYPE_REVERSE_CHANNEL);
					DD(bug("parallel: config: Seems to be an ECP\n"));
				}

				if (nb_irq == 1)
				{ /* Now, find the IRQ */
					resource_descriptor r_irq;
					if ((*pnp->get_nth_resource_descriptor_of_type)(device, 0, B_IRQ_RESOURCE, &r_irq, sizeof(r_irq)) != B_OK)
						goto error;
					d->irq_num = log2(r_irq.d.m.mask);
					DD(bug("parallel: config: selecting IRQ : %d\n", d->irq_num));
				}
																		
				if (nb_dma == 1)
				{ /* Now find the DMA */
					resource_descriptor r_dma;
					if ((*pnp->get_nth_resource_descriptor_of_type)(device, 0, B_DMA_RESOURCE, &r_dma, sizeof(r_dma)) != B_OK)
						goto error;
					d->device_dma.dma_channel = log2(r_dma.d.m.mask);
					DD(bug("parallel: config: selecting DMA : %d\n", d->device_dma.dma_channel));
				}
			}
		}
error:
		free (device);				
	}

	put_module(CM);
	return B_OK;
}

#else

status_t configure(par_port_info *port_info)
{
	int i;

	/* Initialize the ports */
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		par_port_info *d;
		d = port_info+i;
		d->exists = false;
		d->io_base = 0;
		d->type = 0;
		d->irq_sem = -1;
		d->hw_lock = -1;
		d->buffer = 0;
		d->fifosize = 0;
		d->threshold_read = 0;
		d->threshold_write = 0;
		d->irq_num = M_IRQ_NONE;
		d->device_dma.dma_channel = -1;
	}

	/* BeBox's Parallel port data are hardcoded (no BIOS) */
	port_info[0].exists = true;
	port_info[0].io_base = 0x378;
	port_info[0].type = (PAR_TYPE_ECP | PAR_TYPE_REVERSE_CHANNEL);
	port_info[0].irq_num = 7;
	port_info[0].device_dma.dma_channel = 3;

	return B_OK;
}

#endif



static uint32 log2(uint n)
{
	uint i;
	for (i=0 ; i<32 ; i++)
	{
		if ((1<<i) & n)
			return (i);
	}
	return 0;
}


static int find_location(par_port_info *port_info, int io_base)
{
	int i;
	int almost = -1;
	for (i=0 ; i<NUM_PORTS ; i++)
	{
		if (port_info[i].exists == false)
		{
			if (port_info[i].io_base == io_base)
				return i;
			if (port_info[i].io_base == 0)
				almost = i;
		}
	}
	
	/* this device was not found: this is not a regular device!*/
	if  (almost == -1)
	{
		E(bug("parallel: config: IO Base 0x%x is not a regular parallel port address!\n", io_base));
	
		/* Give it another place */			
		for (i=0 ; i<NUM_PORTS ; i++)
			if (port_info[i].exists == false)
				return i;

		E(bug("parallel: config: cannot handle more parallel devices!\n"));
	}
	
	return almost;
}

