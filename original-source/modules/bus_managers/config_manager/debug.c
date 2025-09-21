#include <drivers/KernelExport.h>

#include <string.h>

#include "config_manager.h"
#include "config_manager_p.h"
#include "debug.h"

#ifdef DEBUG

int _assert_(char *a, int b, char *c)
{
        dprintf("tripped assertion in %s/%d (%s)\n", a, b, c);
#ifdef USER
        debugger("tripped assertion");
#else
        kernel_debugger("tripped assertion");
#endif
        return 0;
}

#endif

/************************************************************/

static
status_t count_resource_descriptors_of_type(
	const struct device_configuration *c, resource_type type)
{
	uint32 i, count = 0;
	
	if (!c) return EINVAL;
	
	for (i=0;i<c->num_resources;i++)
		if (c->resources[i].type == type)
			count++;
	return count;
}

static
status_t get_nth_resource_descriptor_of_type(
	const struct device_configuration *c, uint32 n, resource_type type,
	resource_descriptor *d, uint32 size)
{
	uint32 i;

	if (!d || !d) return EINVAL;
	
	for (i=0;i<c->num_resources;i++)
		if ((c->resources[i].type == type) && (n-- == 0)) {
			if (size > sizeof(resource_descriptor))
				size = sizeof(resource_descriptor);
			memcpy(d, c->resources + i, size);
			return B_OK;
		}

	return ENOENT;
}

/************************************************************/

static
void dump_mask(uint32 mask)
{
	bool first = true;
	int i = 0;
	dprintf("[");
	if (!mask)
		dprintf("none");
	for (;mask;mask>>=1,i++)
		if (mask & 1) {
			dprintf("%s%d", first ? "" : ",", i);
			first = false;
		}
	dprintf("]");
}

void dump_device_configuration(const struct device_configuration *c)
{
	int i, num;
	resource_descriptor r;
	
	num = count_resource_descriptors_of_type(c, B_IRQ_RESOURCE);
	if (num) {
		dprintf("irq%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IRQ_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		dprintf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_DMA_RESOURCE);
	if (num) {
		dprintf("dma%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_DMA_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		dprintf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_IO_PORT_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IO_PORT_RESOURCE,
					&r, sizeof(resource_descriptor));
			dprintf("\n  io range:  min %lx max %lx align %lx len %lx",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}

	num = count_resource_descriptors_of_type(c, B_MEMORY_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_MEMORY_RESOURCE,
					&r, sizeof(resource_descriptor));
			dprintf("\n  mem range: min %lx max %lx align %lx len %lx",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}
	dprintf("\n");
}

const char *base_desc[] = {
	"Legacy",
	"Mass Storage Controller",
	"NIC",
	"Display Controller",
	"Multimedia Device",
	"Memory Controller",
	"Bridge Device",
	"Communication Device",
	"Generic System Peripheral",
	"Input Device",
	"Docking Station",
	"CPU",
	"Serial Bus Controller"
};

struct subtype_descriptions {
	uchar base, subtype;
	char *name;
} subtype_desc[] = {
	{ 0, 0, "non-VGA" },
	{ 0, 0, "VGA" },
	{ 1, 0, "SCSI" },
	{ 1, 1, "IDE" },
	{ 1, 2, "Floppy" },
	{ 1, 3, "IPI" },
	{ 1, 4, "RAID" },
	{ 2, 0, "Ethernet" },
	{ 2, 1, "Token Ring" },
	{ 2, 2, "FDDI" },
	{ 2, 3, "ATM" },
	{ 3, 0, "VGA/8514" },
	{ 3, 1, "XGA" },
	{ 4, 0, "Video" },
	{ 4, 1, "Audio" },
	{ 5, 0, "RAM" },
	{ 5, 1, "Flash" },
	{ 6, 0, "Host" },
	{ 6, 1, "ISA" },
	{ 6, 2, "EISA" },
	{ 6, 3, "MCA" },
	{ 6, 4, "PCI-PCI" },
	{ 6, 5, "PCMCIA" },
	{ 6, 6, "NuBus" },
	{ 6, 7, "CardBus" },
	{ 7, 0, "Serial" },
	{ 7, 1, "Parallel" },
	{ 8, 0, "PIC" },
	{ 8, 1, "DMA" },
	{ 8, 2, "Timer" },
	{ 8, 3, "RTC" },
	{ 9, 0, "Keyboard" },
	{ 9, 1, "Digitizer" },
	{ 9, 2, "Mouse" },
	{10, 0, "Generic" },
	{11, 0, "386" },
	{11, 1, "486" },
	{11, 2, "Pentium" },
	{11,16, "Alpha" },
	{11,32, "PowerPC" },
	{11,48, "Coprocessor" },
	{12, 0, "IEEE 1394" },
	{12, 1, "ACCESS" },
	{12, 2, "SSA" },
	{12, 3, "USB" },
	{12, 4, "Fibre Channel" },
	{255,255, NULL }
};

void dump_device_info(const struct device_info *c, 
		const struct device_configuration *current,
		const struct possible_device_configurations *possible)
{
	int i;
	const struct device_configuration *C;

	switch (c->bus) {
		case B_ISA_BUS : dprintf("ISA"); break;
		case B_PCI_BUS : dprintf("PCI"); break;
		default : dprintf("unknown"); break;
	}
	dprintf(" bus: ");
	dprintf((c->devtype.base < 13) ? base_desc[c->devtype.base] : "Unknown");
	if (c->devtype.subtype == 0x80)
		dprintf(" (Other)");
	else {
		struct subtype_descriptions *s = subtype_desc;
		
		while (s->name) {
			if ((c->devtype.base == s->base) && (c->devtype.subtype == s->subtype))
				break;
			s++;
		}
		dprintf(" (%s)", (s->name) ? s->name : "Unknown");
	}
	dprintf(" [%x|%x|%x]\n", c->devtype.base, c->devtype.subtype, c->devtype.interface);

	if (!(c->flags & B_DEVICE_INFO_ENABLED))
		dprintf("Device is disabled\n");

	if (c->flags & B_DEVICE_INFO_CONFIGURED) {
		if (c->config_status == B_OK) {
			dprintf("Current configuration: ");
			dump_device_configuration(current);
		} else {
			dprintf("Current configuration error.\n");
		}
	} else {
		dprintf("Currently unconfigured.\n");
	}

	dprintf("%lx configurations\n", possible->num_possible);
	C = possible->possible + 0;
	for (i=0;i<possible->num_possible;i++) {
		dprintf("\nPossible configuration #%d: ", i);
		dump_device_configuration(C);
		NEXT_POSSIBLE(C);
	}

	dprintf("\n\n");
}

/************************************************************/
