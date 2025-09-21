#if vyt
	pci device enable/disable
	do i/o range check?
	improve locking for isa busses
	i/o shadowing for isa devices

	ESCD

	allocation algorithm should favor shared irqs to be nice to dynamic busses

	reduce memory footprint for all
#endif

#ifndef USER
	#include <drivers/KernelExport.h>
#endif

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#if defined(DUMP_DATA) || defined(USER)

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#endif

#include <driver_settings.h>
#include <driver_settings_p.h>

#include "config_manager.h"
#include "config_manager_p.h"
#include "config_boot.h"
#include "range.h"
#include "debug.h"

static int verbosity = 1;

#define DPRINTF(a,b) do { if (verbosity > a) dprintf b; } while (0)

/* used for sanity checking configurations */
#define MAX_NUM_RESOURCES 20

static status_t init_module(void);
static void uninit_module(void);

/************************************************************/

static
uchar mask_to_value(uint32 mask)
{
	uchar value;

	ASSERT(mask);

	if (!mask) return 0;

	for (value=0,mask>>=1;mask;value++,mask>>=1)
		;

	return value;
}

/************************************************************/

static sem_id config_manager_lock_sem = -1;
const char *const config_manager_lock_sem_name = "config_manager lock";

static status_t
lock_config_manager(void)
{
	return acquire_sem(config_manager_lock_sem);
}

static status_t
unlock_config_manager(void)
{
	return release_sem(config_manager_lock_sem);
}

static
status_t create_config_manager_lock(void)
{
	config_manager_lock_sem = create_sem(1, config_manager_lock_sem_name);

	return config_manager_lock_sem;
}

static
void destroy_config_manager_lock(void)
{
	delete_sem(config_manager_lock_sem);
}

/************************************************************/

struct bus_modules {
	const char *name;
	bus_type type;
	config_manager_bus_module_info *module;
} busses[] = {
	/* busses are processed in this order */

	/* jumpered devices should always be loaded first */
	{ B_JUMPERED_CONFIG_MANAGER_BUS, B_ISA_BUS, NULL },
	{ B_ISA_ONBOARD_CONFIG_MANAGER_BUS, B_ISA_BUS, NULL },
	{ B_PCI_CONFIG_MANAGER_BUS, B_PCI_BUS, NULL },
	{ B_ISA_PNP_CONFIG_MANAGER_BUS, B_ISA_BUS, NULL },
	{ NULL, B_UNKNOWN_BUS, NULL }
};

#define NUM_BUSSES (sizeof(busses)/sizeof(struct bus_modules) - 1)

static
void get_modules(void)
{
	int i;
	status_t err;

	for (i=0;busses[i].name;i++)
		busses[i].module = NULL;

	for (i=0;busses[i].name;i++) {
		DPRINTF(1, ("get_module(%s)\n", busses[i].name));
		if ((err = get_module(busses[i].name,
				(struct module_info **)&busses[i].module)) != B_OK) {
			dprintf("config_manager: error getting module %s (%s)\n",
					busses[i].name, strerror(err));
		} else if (!busses[i].module->get_next_device_info ||
				!busses[i].module->get_device_info_for ||
				!busses[i].module->set_device_configuration_for ||
				!busses[i].module->get_size_of_current_configuration_for ||
				!busses[i].module->get_current_configuration_for ||
				!busses[i].module->get_size_of_possible_configurations_for ||
				!busses[i].module->get_possible_configurations_for ||
				!busses[i].module->enable_device ||
				!busses[i].module->disable_device) {
			dprintf("config_manager: module %s is invalid (null function "
					"pointers)\n", busses[i].name);
			put_module(busses[i].name);
			busses[i].module = NULL;
		}
	}
}

static
void put_modules(void)
{
	int i;
	for (i=0;busses[i].name;i++)
		if (busses[i].module) {
			put_module(busses[i].name);
			busses[i].module = NULL;
		}
}

static
status_t count_resource_descriptors_of_type(
	const struct device_configuration *c, resource_type type)
{
	uint32 i, count = 0;
	
	if (!c) return EINVAL;
	if (c->num_resources > MAX_NUM_RESOURCES) {
		dprintf("configuration has invalid resource count (%ld)\n", c->num_resources);
		return EINVAL;
	}
	
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

	if (!c || !d) return EINVAL;
	if (c->num_resources > MAX_NUM_RESOURCES) {
		dprintf("configuration has invalid resource count (%ld)\n", c->num_resources);
		return EINVAL;
	}
	
	for (i=0;i<c->num_resources;i++)
		if ((c->resources[i].type == type) && (n-- == 0)) {
			if (size > sizeof(resource_descriptor))
				size = sizeof(resource_descriptor);
			memcpy(d, c->resources + i, size);
			return B_OK;
		}

	return ENOENT;
}

static
status_t remove_nth_resource_descriptor_of_type(
	struct device_configuration *c, uint32 n, resource_type type)
{
	uint32 i;

	if (!c) return EINVAL;
	if (c->num_resources > MAX_NUM_RESOURCES) {
		dprintf("configuration has invalid resource count (%ld)\n", c->num_resources);
		return EINVAL;
	}

	for (i=0;i<c->num_resources;i++)
		if ((c->resources[i].type == type) && (n-- == 0)) {
			c->num_resources--;
			for (;i<c->num_resources;i++)
				c->resources[i] = c->resources[i+1];
			return B_OK;
		}

	return ENOENT;
}

/************************************************************/

#define DCC_QUANTUM 5

#define DEVICE_CONFIGURATION_CONTAINER_MAGIC 'MCCD'

static
status_t new_device_configuration_container(
		struct device_configuration_container **c)
{
	if (!c) {
		DPRINTF(1, ("new_device_configuration_container: null pointer\n"));
		return EINVAL;
	}

	*c = malloc(sizeof(struct device_configuration_container));
	if (!(*c)) {
		dprintf("new_device_configuration_container: out of core\n");
		return ENOMEM;
	}

	(*c)->c = malloc(sizeof(struct device_configuration));
	if (!((*c)->c)) {
		free(*c);
		*c = NULL;
		dprintf("new_device_configuration_container: out of core\n");
		return ENOMEM;
	}

	DPRINTF(5, ("new_device_configuration_container allocating %lx " \
		"bytes @ %p and %lx bytes @ %p\n", \
		sizeof(struct device_configuration_container), *c, \
		sizeof(struct device_configuration), (*c)->c));

	(*c)->magic = DEVICE_CONFIGURATION_CONTAINER_MAGIC;
	(*c)->c->flags = 0;
	(*c)->num_allocated = (*c)->c->num_resources = 0;

	return B_OK;
}

static
status_t set_device_configuration_allocation_size(
		struct device_configuration_container *c, uint32 num)
{
	ASSERT(c);

	if (!c) return EINVAL;

	free(c->c);
	c->num_allocated = num;
	c->c = malloc(sizeof(struct device_configuration) + 
			num * sizeof(resource_descriptor));
	if (!(c->c)) return ENOMEM;
	c->c->flags = 0;
	c->c->num_resources = 0;

	return B_OK;
}

static
void delete_device_configuration_container(
		struct device_configuration_container **c)
{
	if (!c || !(*c)) return;

	ASSERT((*c)->magic == DEVICE_CONFIGURATION_CONTAINER_MAGIC);

	DPRINTF(5, ("delete_device_configuration_container freeing memory at " \
		"%p and %p\n", *c, (*c)->c));

	free((*c)->c);	
	free(*c);
	
	*c = NULL;
}

static
status_t add_to_device_configuration_container(
		struct device_configuration_container *c,
		resource_descriptor *resource)
{
	if (!c || !resource) return EINVAL;

	ASSERT(c->magic == DEVICE_CONFIGURATION_CONTAINER_MAGIC);
	ASSERT(c->c);
	
	if ((c->magic != DEVICE_CONFIGURATION_CONTAINER_MAGIC) ||
			(c->c == NULL))
		 return EINVAL;

	if (c->num_allocated == c->c->num_resources) {
		uchar *p = malloc(sizeof(struct device_configuration) +
				(c->num_allocated + DCC_QUANTUM) * sizeof(resource_descriptor));
		if (!p) {
			dprintf("Out of core\n");
			return ENOMEM;
		}
		DPRINTF(5, ("add_to_device_configuration_container allocating %lx "
			"bytes at %p\n", sizeof(struct device_configuration) + \
			(c->num_allocated + DCC_QUANTUM) * sizeof(resource_descriptor), p));
		memcpy(p, c->c, sizeof(struct device_configuration) +
				c->num_allocated * sizeof(resource_descriptor));
		DPRINTF(5, ("add_to_device_configuration_container freeing memory at "
			"%p->c = %p\n", c, c->c));
		free(c->c);
		c->c = (void *)p;
		c->num_allocated += DCC_QUANTUM;
	}
	
	memcpy(c->c->resources + c->c->num_resources, resource,
			sizeof(resource_descriptor));
	c->c->num_resources++;
	
	return B_OK;
}

/************************************************************/

static
status_t add_to_possible_device_configurations(
		struct possible_device_configurations **possible,
		struct device_configuration *config)
{
	uint32 original_size, config_size;
	uchar *p;
	struct device_configuration *c;
	int i;
	
	if (!possible || !config || !(*possible)) return EINVAL;
	
	c = (*possible)->possible + 0;
	for (i=0;i<(*possible)->num_possible;i++)
		NEXT_POSSIBLE(c);

	original_size = (uchar *)c - (uchar *)(*possible);

	config_size = sizeof(struct device_configuration) +
			config->num_resources * sizeof(resource_descriptor);

	p = malloc(original_size + config_size);
	if (!p)
		return ENOMEM;

	DPRINTF(6, ("add_to_possible_device_configurations growing %lx bytes at " \
		"%p by %lx bytes at %p\n", original_size, *possible,
		config_size, p));

	memcpy(p, *possible, original_size);
	memcpy(p + original_size, config, config_size);
	free(*possible);
	*possible = (struct possible_device_configurations *)p;
	(*possible)->num_possible++;
	
	return B_OK;
}

/************************************************************/

/* irqs are encoded as follows:
 * each shareable PCI user grabs one of high 4 bits
 * each shareable ISA user grabs one of low 4 bits
 * non-PCI, non-ISA shareable cause all bits to be set
 */
uchar	allocated_irqs[16];

#define B_IRQ_SHAREABLE (B_IRQ_PCI_SHAREABLE | B_IRQ_ISA_SHAREABLE)

uchar	allocated_dmas;
struct range *io_ranges = NULL, *mem_ranges = NULL;

static
void dump_allocated_resources(void)
{
	/* display allocated resources */
	int i;
	dprintf("allocated dma mask: %x, allocated irq masks: ",
			allocated_dmas);
	for (i=0;i<16;i++)
		dprintf("|%2.2x", allocated_irqs[i]);
	dprintf("|\nallocated io ranges:\n");
	dump_ranges(io_ranges);
	dprintf("allocated memory ranges:\n");
	dump_ranges(mem_ranges);
}

static void
clear_allocations(void)
{
	memset(allocated_irqs, 0, sizeof(allocated_irqs));
	allocated_dmas = 0;
	if (io_ranges)
		clear_ranges(&io_ranges);
	if (mem_ranges)
		clear_ranges(&mem_ranges);
}

static
bool is_irq_available(uint32 index, uint32 flags)
{
	uchar mask;
	ASSERT((index >= 0) && (index <= 15));

	mask = (flags & B_IRQ_ISA_SHAREABLE) ? 0x0f : 0xf0;

	if (flags & B_IRQ_SHAREABLE) {
		if (allocated_irqs[index] & ~mask)
			return false;
		return ((allocated_irqs[index] & mask) != mask);
	} else
		return (allocated_irqs[index] == 0);
}

static
void _assign_irq_bit_(uint32 inum, uint32 flags)
{
	if (flags & B_IRQ_SHAREABLE) {
		uint32 i, mask = (flags & B_IRQ_ISA_SHAREABLE) ? 0x0f : 0xf0;
		for (i=1;i<0x100;i<<=1) {
			if ((i & mask) && !(allocated_irqs[inum] & i)) {
				allocated_irqs[inum] |= i;
				break;
			}
		}
	} else {
		allocated_irqs[inum] = 0xff;
	}
}

static
void _unassign_irq_bit_(uint32 inum, uint32 flags)
{
	if (flags & B_IRQ_SHAREABLE) {
		uint32 i, mask = (flags & B_IRQ_ISA_SHAREABLE) ? 0x0f : 0xf0;
		for (i=1;i<0x100;i<<=1) {
			if ((i & mask) && (allocated_irqs[inum] & i)) {
				allocated_irqs[inum] &= ~i;
				break;
			}
		}
	} else {
		ASSERT(allocated_irqs[inum] == 0xff);
		allocated_irqs[inum] = 0;
	}
}

static
status_t _assign_irq_(struct device_configuration_container *c,
		const resource_mask_descriptor *irq, uint32 inum)
{
	resource_descriptor r;

	DPRINTF(4, ("assigning irq %ld\n", inum));

	_assign_irq_bit_(inum, irq->flags);
	r.type = B_IRQ_RESOURCE;
	r.d.m.mask = 1 << inum;
	r.d.m.cookie = irq->cookie;

	return add_to_device_configuration_container(c, &r);
}

static
status_t assign_irq(struct device_configuration_container *c,
		const resource_mask_descriptor *irq)
{
	uint32 mask, index;

	DPRINTF(4, ("assign irq (mask %lx)\n", irq->mask));
	
	if (!irq->mask) return B_OK;

	if (irq->flags & B_IRQ_SHAREABLE) {
		/* first search for shared interrupts */
		/* vyt: should balance the load */
		uchar imask = (irq->flags & B_IRQ_ISA_SHAREABLE) ? 0x0f : 0xf0;
		for (mask=1,index=0;index<16;mask<<=1,index++) {
			if ((irq->mask & mask) == 0) continue;
			if ((allocated_irqs[index] & imask) == 0) continue;
			if ((allocated_irqs[index] & imask) != imask)
				return _assign_irq_(c, irq, index);
		}
	}

	for (mask=1,index=0;index<16;mask<<=1,index++)
		if ((irq->mask & mask) && (allocated_irqs[index] == 0))
			return _assign_irq_(c, irq, index);

	return B_ERROR;
}

static
status_t assign_dma(struct device_configuration_container *c,
		const resource_mask_descriptor *dma)
{
	uint32 i;

	DPRINTF(4, ("assign dma (mask %lx)\n", dma->mask));
	
	if (!dma->mask) return B_OK;
	
	for (i=1;i<0x100;i<<=1) {
		if ((dma->mask & i) && !(allocated_dmas & i)) {
			/* found unassigned dma, so add it to the configuration */
			resource_descriptor r;
			DPRINTF(4, ("assigning dma %d\n", mask_to_value(i)));
			allocated_dmas |= i;
			r.type = B_DMA_RESOURCE;
			r.d.m.mask = i;
			r.d.m.cookie = dma->cookie;
			return add_to_device_configuration_container(c, &r);
		}
	}
	return B_ERROR;
}

static
status_t assign_io_range(struct device_configuration_container *c,
		const resource_range_descriptor *r)
{
	uint32 base;

	DPRINTF(4, ("assign io range (%lx->%lx len %lx)\n", \
			r->minbase, r->maxbase, r->len));

	if (r->len == 0)
		return B_OK;

	for (base=r->minbase;base<=r->maxbase;base+=r->basealign) {
		if (is_range_unassigned(io_ranges, base, base + r->len - 1)) {
			resource_descriptor rd;
			DPRINTF(4, ("assigning io range 0x%lx -> 0x%lx\n", \
					base, base + r->len - 1));
			add_range(&io_ranges, base, base + r->len - 1);
			rd.type = B_IO_PORT_RESOURCE;
			rd.d.r.minbase = rd.d.r.maxbase = base;
			rd.d.r.basealign = 0;
			rd.d.r.len = r->len;
			rd.d.r.cookie = r->cookie;
			return add_to_device_configuration_container(c, &rd);
		}
		if (!r->basealign || (r->minbase == r->maxbase))
			break;
	}
	return B_ERROR;
}

static
status_t assign_memory_range(struct device_configuration_container *c,
		const resource_range_descriptor *r)
{
	uint32 base;

	DPRINTF(4, ("assign memory range (%lx->%lx len %lx)\n", \
			r->minbase, r->maxbase, r->len));

	if (r->len == 0)
		return B_OK;

	for (base=r->minbase;base<=r->maxbase;base+=r->basealign) {
		if (is_range_unassigned(mem_ranges, base, base + r->len - 1)) {
			resource_descriptor rd;
			DPRINTF(4, ("assigning memory range 0x%lx -> 0x%lx\n", \
					base, base + r->len - 1));
			add_range(&mem_ranges, base, base + r->len - 1);
			rd.type = B_MEMORY_RESOURCE;
			rd.d.r.minbase = rd.d.r.maxbase = base;
			rd.d.r.basealign = 0;
			rd.d.r.len = r->len;
			rd.d.r.cookie = r->cookie;
			return add_to_device_configuration_container(c, &rd);
		}
		if (!r->basealign || (r->minbase == r->maxbase))
			break;
	}
	return B_ERROR;
}

static
status_t assign_resource(struct device_configuration_container *c,
		const resource_descriptor *r)
{
	ASSERT(c); ASSERT(r);

	if (!c || !r) {
		DPRINTF(0, ("assign_resource: null pointer\n"));
		return EINVAL;
	}
	
	DPRINTF(5, ("assign_resource: type %x (%p) (%lx)\n", \
			r->type, r, *(uint32 *)r));

	switch (r->type) {
		case B_IRQ_RESOURCE :
			return assign_irq(c, &(r->d.m));
		case B_DMA_RESOURCE :
			return assign_dma(c, &(r->d.m));
		case B_IO_PORT_RESOURCE :
			return assign_io_range(c, &(r->d.r));
		case B_MEMORY_RESOURCE :
			return assign_memory_range(c, &(r->d.r));
		default :
			dprintf("Invalid resource type (%x)\n", r->type);
			ASSERT(0);
	}
	
	return EINVAL;
}

/**********************************************************************/

static
status_t unacquire_configuration(const struct device_configuration *c)
{
	int i, num;
	resource_descriptor r;

	ASSERT(c);
	if (!c) return EINVAL;

	num = count_resource_descriptors_of_type(c, B_IRQ_RESOURCE);
	for (i=0;i<num;i++) {
		if (get_nth_resource_descriptor_of_type(c, i, B_IRQ_RESOURCE,
				&r, sizeof(resource_descriptor)) == B_OK)
			_unassign_irq_bit_(mask_to_value(r.d.m.mask), r.d.m.flags);
	}
	num = count_resource_descriptors_of_type(c, B_DMA_RESOURCE);
	for (i=0;i<num;i++) {
		if (get_nth_resource_descriptor_of_type(c, i, B_DMA_RESOURCE,
				&r, sizeof(resource_descriptor)) == B_OK)
			allocated_dmas &= ~r.d.m.mask;
	}
	num = count_resource_descriptors_of_type(c, B_IO_PORT_RESOURCE);
	for (i=0;i<num;i++) {
		if (get_nth_resource_descriptor_of_type(c, i, B_IO_PORT_RESOURCE,
				&r, sizeof(resource_descriptor)) == B_OK)
			remove_range(&io_ranges, r.d.r.minbase,
					r.d.r.minbase + r.d.r.len - 1);
	}
	num = count_resource_descriptors_of_type(c, B_MEMORY_RESOURCE);
	for (i=0;i<num;i++) {
		if (get_nth_resource_descriptor_of_type(c, i, B_MEMORY_RESOURCE,
				&r, sizeof(resource_descriptor)) == B_OK)
			remove_range(&mem_ranges, r.d.r.minbase,
					r.d.r.minbase + r.d.r.len - 1);
	}

	return B_OK;
}

/**********************************************************************/

static
void forcibly_acquire_configuration(
		const struct device_configuration *requested)
{
	int i;
	struct device_configuration_container *temp;

	ASSERT(requested);
	if (!requested) return;
	
	if (new_device_configuration_container(&temp) < 0)
		return;

	for (i=0;i<requested->num_resources;i++)
		assign_resource(temp, requested->resources + i);

	delete_device_configuration_container(&temp);
}

static
status_t acquire_configuration(const struct device_configuration *requested,
		struct device_configuration **assigned)
{
	int i;
	status_t err;
	struct device_configuration_container *temp;

	if (!requested || !assigned)
		return EINVAL;

	if ((err = new_device_configuration_container(&temp)) < 0)
		return err;

	ASSERT(temp);

	for (i=0;i<requested->num_resources;i++) {
		DPRINTF(5, ("acquire_configuration: temp = %p, i = %x/%lx, type = %x\n",\
			temp, i, requested->num_resources, requested->resources[i].type));

		if ((err = assign_resource(temp, requested->resources + i)) != B_OK) {
			switch (requested->resources[i].type) {
				case B_IRQ_RESOURCE :
					if (verbosity > 0) {
						int j;
						dprintf("error acquiring irq: req mask %lx "
							"(alloc mask: ", requested->resources[i].d.m.mask);
						for (j=0;j<16;j++)
							dprintf("|%2.2x", allocated_irqs[j]);
						dprintf("|)\n");
					}
					break;
				case B_DMA_RESOURCE :
					DPRINTF(0, ("error acquiring dma: req mask %lx (alloc mask %x)\n", \
						requested->resources[i].d.m.mask, allocated_dmas));
					break;
				case B_IO_PORT_RESOURCE :
				case B_MEMORY_RESOURCE :
					DPRINTF(0, ("error acquiring %s range: " \
						"min %lx max %lx step %lx len %lx\n", \
						(requested->resources[i].type == B_IO_PORT_RESOURCE) ? \
						"io" : "memory",
						requested->resources[i].d.r.minbase, \
						requested->resources[i].d.r.maxbase, \
						requested->resources[i].d.r.basealign, \
						requested->resources[i].d.r.len));
					break;
				default :
					dprintf("unknown resource type %x\n",
							requested->resources[i].type);
			}
			unacquire_configuration(temp->c);
			delete_device_configuration_container(&temp);
			return err;
		}
	}

	*assigned = temp->c;
	free(temp);

	DPRINTF(1, ("Configuration successful\n"));
	if (verbosity > 1) dump_device_configuration(*assigned);

	return B_OK;
}

static
status_t acquire_any_configuration(
		const struct possible_device_configurations *possible,
		struct device_configuration **assigned)
{
	int i;
	const struct device_configuration *c;

	if (!possible || !assigned) return EINVAL;

	c = possible->possible + 0;

	for (i=0;i<possible->num_possible;i++) {
		if (acquire_configuration(c, assigned) == B_OK)
			return B_OK;

		NEXT_POSSIBLE(c);
	}

	return B_ERROR;
}

/***************************************************************************/

#define DIN_MAGIC 'ENID'

struct device_info_node {
	uint32 magic;
	struct device_info info;
	struct possible_device_configurations *possible;
	struct device_configuration_container *assigned;
	uint32 bus_index;
	uint32 id;

	/* current permutation state */
	uint32 current_permutation_index;
	struct device_configuration *current_permutation;

	struct device_info_node *next, *prev;
};

static
status_t count_permutations(
		const struct device_configuration *c)
{
	uint32 num, temp;
	int i, j;

	if (!c) return EINVAL;

	num = 1;
	for (i=0;i<c->num_resources;i++) {
		switch (c->resources[i].type) {
			case B_IRQ_RESOURCE:
			case B_DMA_RESOURCE:
				temp = 0;
				for (j=1;j<0x10000;j<<=1)
					if (c->resources[i].d.m.mask & j)
						temp++;
				num *= temp;
				break;
			case B_IO_PORT_RESOURCE:
			case B_MEMORY_RESOURCE:
				break;
			default:
				dprintf("Unknown resource type (%x)\n", c->resources[i].type);
				break;
		}
	}

	return num;
}

static
status_t count_device_permutations(
		const struct possible_device_configurations *possible)
{
	int i;
	status_t count = 0, error;
	const struct device_configuration *c;

	if (!possible) return EINVAL;

	c = possible->possible + 0;
	for (i=0;i<possible->num_possible;i++) {
		error = count_permutations(c);
		if (error > 0) count += error;
		DPRINTF(5, ("possible configuration # %x/%lx has %ld permutations\n",
				i, possible->num_possible, error));
		NEXT_POSSIBLE(c);
	}

	return count ? count : 1;
}

static
status_t acquire_next_bit(struct device_info_node *dev,
		resource_descriptor *resource, uint32 rindex,
		bool first)
{
	uint32 mask, index;

	if (first) {
		mask = 1;
		index = 0;
	} else {
		mask = dev->assigned->c->resources[rindex].d.m.mask << 1;
		index = mask_to_value(mask);
	}

	while (mask < 0x10000) {
		if (mask & resource->d.m.mask) {
			if (resource->type == B_IRQ_RESOURCE) {
				if (is_irq_available(index, resource->d.m.flags)) {
					_assign_irq_bit_(index, resource->d.m.flags);
					DPRINTF(3, ("Acquired IRQ %ld for resource #%ld\n",index,rindex));
					dev->assigned->c->resources[rindex] = *resource;
					dev->assigned->c->resources[rindex].d.m.mask = mask;
					return B_OK;
				}
			} else if (resource->type == B_DMA_RESOURCE) {
				if ((allocated_dmas & mask) == 0) {
					DPRINTF(3, ("Acquired DMA %ld for resource #%ld\n",index,rindex));
					allocated_dmas |= mask;

					dev->assigned->c->resources[rindex] = *resource;
					dev->assigned->c->resources[rindex].d.m.mask = mask;
					return B_OK;
				}
			} else {
				dprintf("Invalid resource type passed to " \
						"acquire_next_bit (%x)\n", resource->type);
			}
		}
		mask <<= 1; index++;
	}

	DPRINTF(3, ("No bit found for resource #%ld (%d)\n", rindex, first));

	return ENOENT;
}

/* aquire next available configuration from current device_configuration */
static
status_t _acquire_next_device_permutation_(struct device_info_node *dev,
		bool first)
{
	status_t error;
	int32 i, direction;
	resource_descriptor *resource, *end;

	DPRINTF(3, ("Scanning device %lx|%lx configuration space %ld\n", \
			dev->bus_index, dev->id, dev->current_permutation_index));

	if (first) {
		i = 0;
		direction = 1;
	} else {
		i = dev->current_permutation->num_resources - 1;
		direction = -1;
	}
	resource = dev->current_permutation->resources + i;
	end = dev->current_permutation->resources + 
			dev->current_permutation->num_resources;

	while (1) {
		if (i < 0) break;

		/* use this test to avoid problems from a signed/unsigned comparison */
		if (resource >= end) {
			ASSERT(resource == end);

			DPRINTF(3, ("Found a good device permutation for %lx|%lx\n", \
					dev->bus_index, dev->id));
			return B_OK;
		}

		if (direction == 1) {
			DPRINTF(3, ("Advancing the device permutation for %lx|%lx (%ld/%ld)\n", \
					dev->bus_index, dev->id, i+1, dev->current_permutation->num_resources));
			if ((resource->type == B_IRQ_RESOURCE) ||
				(resource->type == B_DMA_RESOURCE)) {
				error = acquire_next_bit(dev, resource, i, first);
				if (error < 0) {
					i--; resource--;
					direction = -1;
					continue;
				}
			}
			/* stuff in front is all new to us */
			first = true;
			i++; resource++;
			continue;
		}

		/* go back */

		DPRINTF(3, ("Backing up the device permutation for %lx|%lx (%ld/%ld)\n", \
				dev->bus_index, dev->id, i+1, dev->current_permutation->num_resources));

		/* when we go back forwards, we want to pick up where we left off */
		first = false;

		/* undo damage */
		if (resource->type == B_IRQ_RESOURCE) {
			DPRINTF(3, ("Freeing IRQ %d for %lx|%lx\n", \
				mask_to_value(dev->assigned->c->resources[i].d.m.mask), \
				dev->bus_index, dev->id));
			_unassign_irq_bit_(
					mask_to_value(dev->assigned->c->resources[i].d.m.mask),
					dev->assigned->c->resources[i].d.m.flags);
			direction = 1;
		} else if (resource->type == B_DMA_RESOURCE) {
			DPRINTF(3, ("Freeing DMA %d for %lx|%lx\n", \
				mask_to_value(dev->assigned->c->resources[i].d.m.mask), \
				dev->bus_index, dev->id));
			allocated_dmas &= ~dev->assigned->c->resources[i].d.m.mask;
			direction = 1;
		} else {
			i--; resource--;
		}
	}

	DPRINTF(3, ("No good permutations for %lx|%lx (%ld/%ld)\n", \
			dev->bus_index, dev->id, i+1, dev->current_permutation->num_resources));

	return ENOENT;
}

/* acquire next available device configuration from all possible configurations,
 *or return ENOENT if it is impossible to do so */
static
status_t acquire_next_device_permutation(struct device_info_node *dev,
		bool first)
{
	status_t error;

	ASSERT(dev);

	if (dev->possible->num_possible == 0) {
		dev->assigned->c->num_resources = 0;
		return (first) ? B_OK : ENOENT;
	}

	if (first) {
		status_t error;

		dev->current_permutation_index = 0;
		dev->current_permutation = dev->possible->possible + 0;

		/* allocate space for resources */
		if ((error = set_device_configuration_allocation_size(
				dev->assigned, dev->current_permutation->num_resources)) < B_OK)
			return error;

		dev->assigned->c->num_resources = 
				dev->current_permutation->num_resources;
	}

	ASSERT(dev->current_permutation_index <= dev->possible->num_possible);
	if (dev->current_permutation_index >= dev->possible->num_possible)
		return ENOENT;

	DPRINTF(3, ("Continuing searching permutation for device " \
			"%lx|%lx (%lx/%lx)\n", \
			dev->bus_index, dev->id, \
			dev->current_permutation_index+1, dev->possible->num_possible));
	if (first) {
		/* allocate space for resources */
		if ((error = set_device_configuration_allocation_size(
				dev->assigned, dev->current_permutation->num_resources)) < B_OK)
			return error;

		dev->assigned->c->num_resources = 
				dev->current_permutation->num_resources;
	}

	error = _acquire_next_device_permutation_(dev, first);
	if (error == B_OK) return B_OK;

	while (++dev->current_permutation_index < dev->possible->num_possible) {
		DPRINTF(3, ("Moving on to the next permutation for device " \
				"%lx|%lx (%lx/%lx)\n", \
				dev->bus_index, dev->id, \
				dev->current_permutation_index+1, dev->possible->num_possible));
		NEXT_POSSIBLE(dev->current_permutation);

		/* allocate space for resources */
		if ((error = set_device_configuration_allocation_size(
				dev->assigned, dev->current_permutation->num_resources)) < B_OK)
			return error;

		dev->assigned->c->num_resources = 
				dev->current_permutation->num_resources;

		error = _acquire_next_device_permutation_(dev, true);
		if (error == B_OK) return B_OK;
	}

	DPRINTF(3, ("There are no more permutations for device %lx|%lx\n", \
		dev->bus_index, dev->id));

	return ENOENT;
}

static
status_t acquire_the_range(const resource_descriptor *source,
		resource_descriptor *target)
{
	uint32 base;
	struct range **r;

	if (source->type == B_IO_PORT_RESOURCE)
		r = &io_ranges;
	else if (source->type == B_MEMORY_RESOURCE)
		r = &mem_ranges;
	else
		return EINVAL;

	*target = *source;

	if (source->d.r.len == 0) return B_OK;

	for (base=source->d.r.minbase;
			base<=source->d.r.maxbase;
			base += source->d.r.basealign) {
		if (is_range_unassigned(*r, base, base + source->d.r.len - 1)) {
			add_range(r, base, base + source->d.r.len - 1);
			target->d.r.minbase = target->d.r.maxbase = base;
			target->d.r.basealign = 0;
			DPRINTF(3, ("Assigning range %lx->%lx\n", \
				base, base + source->d.r.len - 1));
			return B_OK;
		}
		if (!(source->d.r.basealign) || 
				(source->d.r.minbase == source->d.r.maxbase))
			break;
	}

	DPRINTF(3, ("No ranges found %lx->%lx %lx/%lx\n", source->d.r.minbase, source->d.r.maxbase, source->d.r.len, source->d.r.basealign));

	return ENOENT;
}

static
status_t release_the_range(resource_descriptor *d)
{
	struct range **r;

	if (d->type == B_IO_PORT_RESOURCE)
		r = &io_ranges;
	else if (d->type == B_MEMORY_RESOURCE)
		r = &mem_ranges;
	else
		return EINVAL;

	if (d->d.r.len == 0) return B_OK;

	return remove_range(r, d->d.r.minbase,
			d->d.r.minbase + d->d.r.len - 1);
}

static
void release_the_ranges(struct device_info_node *n)
{
	int i;

	for (i=0;i<n->assigned->c->num_resources;i++) {
		switch (n->assigned->c->resources[i].type) {
			case B_IO_PORT_RESOURCE :
			case B_MEMORY_RESOURCE :
				release_the_range(n->assigned->c->resources + i);
				break;
			default:
				break;
		}
	}
}

static
status_t range_check_node(struct device_info_node *n)
{
	struct device_configuration *c;
	int32 i;
	uint32 temp;

	DPRINTF(3, ("Assigning ranges for device %lx|%lx\n", n->bus_index, n->id));

	if (n->possible->num_possible == 0) return B_OK;
	c = n->current_permutation;
	for (i=0;i<c->num_resources;i++) {
		switch (c->resources[i].type) {
			case B_IRQ_RESOURCE :
			case B_DMA_RESOURCE :
				temp = n->assigned->c->resources[i].d.m.mask;
				n->assigned->c->resources[i] = c->resources[i];
				n->assigned->c->resources[i].d.m.mask = temp;
				break;
			case B_IO_PORT_RESOURCE :
			case B_MEMORY_RESOURCE :
				if (acquire_the_range(c->resources + i,
						n->assigned->c->resources + i) == B_OK)
					break;

				/* undo our damage */
				for (--i;i>=0;i--) {
					switch (c->resources[i].type) {
						case B_IO_PORT_RESOURCE :
						case B_MEMORY_RESOURCE :
							release_the_range(n->assigned->c->resources + i);
							break;
						default:
							break;
					}
				}
				return EINVAL;
			default :
				n->assigned->c->resources[i] = c->resources[i];
				break;
		}
	}

	return B_OK;
}

static
status_t assign_ranges_to_devices(struct device_info_node *c)
{
	/* this uses a greedy algorithm. got a problem with that? */
	while (c) {
		if (range_check_node(c) != B_OK) {
			while ((c = c->prev) != NULL)
				release_the_ranges(c);
			return EINVAL;
		}
		c = c->next;
	}
	return B_OK;
}

static void
free_device_info_list(struct device_info_node *head)
{
	struct device_info_node *p, *c = head;

	while (c) {
		p = c;
		c = c->next;
		if (p->possible) free(p->possible);
		if (p->assigned) delete_device_configuration_container(&(p->assigned));
		free(p);
	}
}

static
bool is_range_available(resource_range_descriptor *r, struct range *ranges)
{
	uint32 base;
	
	if (r->len == 0) return true;

	for (base=r->minbase;base<=r->maxbase;base+=r->basealign) {
		if (is_range_unassigned(ranges, base, base + r->len - 1))
			return true;
		if (!(r->basealign) || 
				(r->minbase == r->maxbase))
			break;
	}

	return false;
}

static
bool is_resource_available(resource_descriptor *r)
{
	uint32 i, j;
	bool result;
	
	switch (r->type) {
		case B_IRQ_RESOURCE :
			if (r->d.m.mask == 0) return true;
			for (i=1,j=0;j<16;i<<=1,j++)
				if (r->d.m.mask & i)
					if (is_irq_available(j, r->d.m.flags))
						return true;
			DPRINTF(3, ("Device has an IRQ conflict\n"));
			return false;
		case B_DMA_RESOURCE :
			if (r->d.m.mask == 0) return true;
			result = ((~allocated_dmas & r->d.m.mask) != 0);
			if (!result) DPRINTF(3, ("Device has a DMA conflict\n"));
			return result;
		case B_IO_PORT_RESOURCE :
			result = is_range_available(&(r->d.r), io_ranges);
			if (!result) DPRINTF(3, ("Device has an IO conflict\n"));
			return result;
		case B_MEMORY_RESOURCE :
			result = is_range_available(&(r->d.r), mem_ranges);
			if (!result) DPRINTF(3, ("Device has a memory conflict\n"));
			return result;
		default:
			return false;
	}
}

static
status_t assign_configurations_to_devices(struct device_info_node **head)
{
	struct device_info_node *c, *removed_list = NULL;
	int32 permutations = 1;
	bool first_time;
	status_t error;

	if (!head) return EINVAL;

	/* first assign resources for already-configured devices and take them
	 * out of the list */

	c = *head;
	while (c) {
		const struct device_configuration *config;
		struct device_info_node *n;
		uint32 type;

		error = match_config_manager_settings_for(c->info.bus, c->info.id,
					&type, (const void **)&config);
		if (error) type = 0xffffffff;

		if (type == CONFIG_TYPE_DISABLE_DEVICE) {
			config_manager_bus_module_info *module;
			module = busses[c->bus_index].module;
			if (module && module->disable_device) {
				dprintf("disabling device %lx|%lx as per user's request\n",
						c->bus_index,c->id);
				error = (module->disable_device)(c->id, B_DEV_DISABLED_BY_USER);
				if (error) {
					dprintf("error disabling device (%lx)\n", error);
					if (c->info.flags & B_DEVICE_INFO_CONFIGURED)
						forcibly_acquire_configuration(c->assigned->c);
				}
			}
		} else if (type == CONFIG_TYPE_SET_CONFIGURATION) {
			forcibly_acquire_configuration(config);

			DPRINTF(2, ("setting configuration for device bus %x id %lx\n", \
					c->info.bus, c->id));
			if ((busses[c->bus_index].module->set_device_configuration_for == NULL) ||
				((c->info.config_status =
					busses[c->bus_index].module->set_device_configuration_for(
					c->id, config)) < 0)) {
				dprintf("error writing configuration to device bus %x id %lx\n",
						c->info.bus, c->id);
				dump_device_configuration(config);
				if (busses[c->bus_index].module->disable_device != NULL)
					busses[c->bus_index].module->disable_device(
							c->id, B_DEV_CONFIGURATION_ERROR);
			}
		} else if (c->info.flags & B_DEVICE_INFO_CAN_BE_DISABLED) {
			DPRINTF(3, ("device %lx has %ld permutations\n", \
					c->id, count_device_permutations(c->possible)));
			permutations *= count_device_permutations(c->possible);

			/* ignore current configuration for now */
/*			c->info.flags &= ~B_DEVICE_INFO_CONFIGURED;*/
	
			c = c->next;
			
			continue;
		} else if (c->info.flags & B_DEVICE_INFO_ENABLED) {
			forcibly_acquire_configuration(c->assigned->c);
		}

		n = c->next;

		if (c == *head) *head = n;
		if (c->prev) c->prev->next = n;
		if (n) n->prev = c->prev;

		if (c->possible) free(c->possible);
		if (c->assigned) delete_device_configuration_container(&(c->assigned));
		free(c);

		c = n;
	}

	DPRINTF(3, ("%ld global permutations\n", permutations));

	/* prune data in remaining devices */
	c = *head;
	while (c) {
		struct device_configuration *p;
		struct device_info_node *n;
		uint32 i, index;

		n = c->next;
		
		p = c->possible->possible + 0;
		for (index=0;index<c->possible->num_possible;index++) {
			for (i=0;i<p->num_resources;i++)
				if (is_resource_available(p->resources + i) == false)
					break;
			if (i < p->num_resources) {
				/* prune this guy! */
				struct device_configuration *p2;

				DPRINTF(3, ("pruned device %lx|%lx configuration %ld\n", \
						c->bus_index, c->id, index));
				c->possible->num_possible--;
				if (index < c->possible->num_possible) {
					uchar *s, *t;
					t = (uchar *)p;
					p2 = p;
					NEXT_POSSIBLE(p2);
					s = (uchar *)p2;
					for (i=index;i<c->possible->num_possible;i++)
						NEXT_POSSIBLE(p2);
					while (s < (uchar *)p2)
						*(t++) = *(s++);
					index--;
				}
				if (c->possible->num_possible == 0) {
					config_manager_bus_module_info *module;

					module = busses[c->bus_index].module;
					ASSERT(module && module->disable_device);

					dprintf("pruning device %lx|%lx\n", c->bus_index,c->id);
					error = (module->disable_device)(c->id, B_DEV_RESOURCE_CONFLICT);
					if (error) {
						dprintf("error disabling device (%lx)\n", error);
						if (c->info.flags & B_DEVICE_INFO_CONFIGURED)
							forcibly_acquire_configuration(c->assigned->c);
					}

					if (c == *head) *head = (*head)->next;
					if (c->prev) c->prev->next = c->next;
					if (c->next) c->next->prev = c->prev;
		
					if (c->possible) free(c->possible);
					if (c->assigned) delete_device_configuration_container(&(c->assigned));
					free(c);

					break;
				}
			} else {
				NEXT_POSSIBLE(p);
			}
		}
		
		c = n;
	}

try_again:
	first_time = true;

	c = *head;

	DPRINTF(3, ("Now trying to find a suitable configuration for all devices.\n"));

	/* try all the permutations one by one */
	while (1) {
		if (verbosity > 4) dump_allocated_resources();

		/* any devices to configure? */
		if (!(*head)) goto found_configuration;

		/* bump up the current permutation */
		if (first_time)
			DPRINTF(3, ("Going to acquire the first device permutation for " \
					 "%lx|%lx.\n", c->bus_index, c->id));
		else
			DPRINTF(3, ("Bump up the permutation for device %lx|%lx\n", \
					 c->bus_index, c->id));
		error = acquire_next_device_permutation(c, first_time);
		first_time = false;
		if (error < 0)
			DPRINTF(3, ("Error code %lx\n", error));
		else
			DPRINTF(3, ("Successfully acquired device permutation for " \
				 "%lx|%lx.\n", c->bus_index, c->id));

		/* if no more configurations, we have to wind back */
		if (error == ENOENT) goto backtrack;

		/* if we failed due to some catastrophic error, bomb out */
		if (error < B_OK) {
			dprintf("error in config_manager (%lx)\n", error);
			return error;
		}

		DPRINTF(3, ("Now setting the following devices to their first " \
				 "permutations.\n"));
		/* set all the following ones to the first permutation */
		while (c->next) {
			c = c->next;
			DPRINTF(3, ("Going to acquire the first device permutation for " \
					 "%lx|%lx.\n", c->bus_index, c->id));
			error = acquire_next_device_permutation(c, true);
			if (error < 0)
				DPRINTF(3, ("Error code %lx\n", error));
			else
				DPRINTF(3, ("Successfully acquired device permutation for " \
					 "%lx|%lx.\n", c->bus_index, c->id));
			/* back track if the first permutation fails */
			if (error == ENOENT) goto backtrack;
			if (error < B_OK) {
				dprintf("error in config_manager (%lx)\n", error);
				return error;
			}
		}

		/* assign ranges to the devices */
		DPRINTF(3, ("Now assigning ranges to devices\n"));
		if (assign_ranges_to_devices(*head) < B_OK) {
			DPRINTF(3, ("Range conflict.  Trying more configurations.\n"));
			continue;
		}

found_configuration:
		/* if they fit, we are done! */
		DPRINTF(3, ("Satisfactory global configuration found\n"));

		/* process devices in the removed list */
		c = removed_list;
		while (c) {
			struct device_info_node *n;
			struct device_configuration *temp;

			n = c->next;

			/* try to acquire it */
			if (acquire_any_configuration(c->possible, &temp) == B_OK) {
				dprintf("was able to configure %lx|%lx after all\n", \
						c->bus_index, c->id);

				if (c->assigned->c) free(c->assigned->c);
				c->assigned->c = temp;

				c->next = *head;
				*head = c;
			} else {
				config_manager_bus_module_info *module;
				module = busses[c->bus_index].module;
				ASSERT(module && module->disable_device);
				dprintf("disabling device %lx|%lx to resolve a resource conflict\n",
						c->bus_index, c->id);
				error = (module->disable_device)(c->id, B_DEV_RESOURCE_CONFLICT);
				if (error) dprintf("error disabling device (%lx)\n", error);

				if (c->possible) free(c->possible);
				if (c->assigned) delete_device_configuration_container(&(c->assigned));
				free(c);
			}
			
			c = n;
		}

		return B_OK;

backtrack:
		/* on failure, move to next configuration state */
		if (!(c->prev)) break;

		c = c->prev;
		DPRINTF(3, ("Backing up to device %lx|%lx\n", c->bus_index, c->id));
	}

	/* no possible configurations, so take out devices one by one */
	/* until a configuration presents itself */

	if (*head == NULL) return B_OK;

	c = *head;
	while (c->next) c = c->next;
	if (c->next) c = c->next;

	if (c->prev)
		c->prev->next = NULL;
	else
		*head = NULL;

	/* move it to the removed list */
	c->next = removed_list;
	removed_list = c;

	DPRINTF(0, ("No configurations found for all devices.\n" \
			"Removing the last device (%lx|%lx) from the chain and trying again.\n", \
			c->bus_index, c->id));

	if (verbosity > 2) dump_allocated_resources();

	goto try_again;
}

/**********************************************************************/

#ifdef DUMP_DATA
static	
status_t dump_configurations(const struct device_info_node *list)
{
	int fd, size, i;
	const struct device_info_node *c = list;
	const struct device_configuration *d;

	DPRINTF(3, ("Writing configurations to files\n"));

	while (c) {
		char fname[64];

		sprintf(fname, "/boot/home/devices/device.%x.%x", c->bus_index, c->id);

		creat(fname, 0644);
		fd = open(fname, O_WRONLY | O_TRUNC);
		if (fd < 0) {
			dprintf("Error opening file '%s'\n", fname);
			return fd;
		}
		write(fd, c, sizeof(struct device_info_node));
		/* write possible */
		d = c->possible->possible + 0;
		for (i=0;i<c->possible->num_possible;i++)
			NEXT_POSSIBLE(d);
		size = (uchar *)d - (uchar *)(c->possible);
		write(fd, &size, sizeof(int));
		write(fd, c->possible, size);
		/* write assigned */
		write(fd,c->assigned,sizeof(struct device_configuration_container));
		write(fd, c->assigned->c, sizeof(struct device_configuration) +
				c->assigned->num_allocated * sizeof(resource_descriptor));
		close(fd);

		c = c->next;
	}
	
	return B_OK;
}
#endif

static
status_t load_all_devices(struct device_info_node **head)
{
	int i;
	status_t size, err;
	struct device_info_node *c;
	
	for (i=0;busses[i].name;i++) {
		/* add all device_info's to the list */
		uint32 cookie;

		DPRINTF(3, ("reading devices for bus '%s'\n", busses[i].name));

		if (!busses[i].module) continue;

		if (busses[i].module->get_next_device_info == NULL)
			continue;

		cookie = 0;
		while (1) {
			c = malloc(sizeof(struct device_info_node));
			if (!c) {
				DPRINTF(3, ("Error allocating memory for device_info_node\n"));
				return ENOMEM;
			}
			c->magic = DIN_MAGIC;

			err = busses[i].module->get_next_device_info(&cookie, &c->info,
					sizeof(struct device_info));
			if (err < B_OK) {
				free(c);
				break;
			}

			size = busses[i].module->get_size_of_possible_configurations_for(
					cookie);
			if (size < B_OK) {
				free(c);
				continue;
			}
			
			c->possible = malloc(size);
			if (!c->possible) {
				DPRINTF(3, ("Error allocating memory for possible configurations\n"));
				free(c);
				return ENOMEM;
			}

			err = busses[i].module->get_possible_configurations_for(
					cookie, c->possible, size);
			if (err < B_OK) {
				free(c->possible);
				free(c);
				continue;
			}

			size = busses[i].module->get_size_of_current_configuration_for(
					cookie);
			if (size < B_OK) {
				free(c->possible);
				free(c);
				continue;
			}

			if (new_device_configuration_container(&(c->assigned)) < B_OK) {
				DPRINTF(3, ("Error allocating memory for configuration container\n"));
				free(c->possible);
				free(c);
				return ENOMEM;
			}

			if ((err = set_device_configuration_allocation_size(
					c->assigned, 
					(size - sizeof(struct device_configuration)) /
					sizeof(resource_descriptor))) < B_OK) {
				DPRINTF(3, ("Error resizing configuration container\n"));
				delete_device_configuration_container(&(c->assigned));
				free(c->possible);
				free(c);
				return ENOMEM;
			}

			err = busses[i].module->get_current_configuration_for(cookie,
					c->assigned->c, size);
			if (err < B_OK) {
				delete_device_configuration_container(&(c->assigned));
				free(c->possible);
				free(c);
				continue;
			}
			
			c->bus_index = i;
			/* id must be the value passed returned by get_next_device_info */
			c->id = cookie;

#if 0
			/* add the device to the head of the list */
			c->next = *head;
			if (c->next) c->next->prev = c;
			c->prev = NULL;
			*head = c;
#else
			/* add the device to the end of the list */
			{
				struct device_info_node *temp;

				c->next = NULL;
				temp = *head;
				if (temp == NULL) {
					c->prev = NULL;
					*head = c;
				} else {
					while (temp->next) temp = temp->next;
					temp->next = c;
					c->prev = temp;
				}
			}
#endif
			if (verbosity > 2) {
				dprintf("---------------------------------------\n");
				dump_device_info(&c->info, c->assigned->c, c->possible);
			}
		}
	}

	return B_OK;
}

/*
	first load in configurations for all devices
	then assign configurations to devices in order:
		already configurations (jumpered, system motherboard devices, PCI,
			configured pnp)
		configurable devices in order:
			fixed configuration
			dependent configurations only
			all others
	write configurations to unconfigured devices
*/

static
status_t configure_all_devices(void)
{
	status_t err;
	struct device_info_node *c, *devices = NULL;

	init_config_manager_settings();

	/* signal start */

	DPRINTF(3, ("configure_all_devices()\n"));

	err = load_all_devices(&devices);
	if (err) goto error1;

#ifdef DUMP_DATA
	dump_configurations(devices);
#endif

	DPRINTF(3, ("Calling assign configurations to devices.\n"));
	err = assign_configurations_to_devices(&devices);

	if (verbosity > 1) dump_allocated_resources();

	DPRINTF(1, ("Writing configurations to devices\n"));

	/* write configurations to the devices */
	c = devices;
	while (c) {
		DPRINTF(2, ("setting configuration for device bus %x id %lx\n", \
				c->info.bus, c->id));
		if ((busses[c->bus_index].module->set_device_configuration_for == NULL) ||
			((c->info.config_status =
				busses[c->bus_index].module->set_device_configuration_for(
				c->id, c->assigned->c)) < 0)) {
			dprintf("error writing configuration to device bus %x id %lx\n",
					c->info.bus, c->id);
			dump_device_configuration(c->assigned->c);
			if (busses[c->bus_index].module->disable_device != NULL)
				busses[c->bus_index].module->disable_device(
						c->id, B_DEV_CONFIGURATION_ERROR);
		}
		c = c->next;
	}

error1:
	/* signal completion */

	DPRINTF(1, ("Freeing device info list\n"));
	free_device_info_list(devices);

	uninit_config_manager_settings();

	if (err)
		dprintf("config_manager: error in configure_all_devices (%s)\n",
				strerror(err));

	return err;
}

/**************************************************************************/

const char areaname[] = "config manager resources";

static
uint32 checksum(void *vp, uint32 len)
{
	uint32 c = 0;
	uint32 *p = (uint32 *)vp;
	
	/* length was in bytes, now in words */
	len = (len + 3) / 4;

	while (len--)
		c += *(p++);

	return c;
}

static
status_t save_resource_info(void)
{
	area_id aid;
	uchar *ptr, *start;
	int i;
	uint32 realsize, size;

	DPRINTF(1, ("Saving used resource information into %s\n", areaname));

	realsize = 2 * sizeof(uint32); /* checksum and length field */
	realsize += 16 + sizeof(uchar); /* irqs and dmas */
	realsize += ranges_save_area_size(io_ranges);
	realsize += ranges_save_area_size(mem_ranges);

	/* round up size to the nearest page */
	size = (realsize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	aid = find_area(areaname);
	if (aid >= 0) delete_area(aid);
	aid = create_area(areaname, (void **)&start, B_ANY_KERNEL_ADDRESS, size,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (aid < 0)
		return aid;

	ptr = start + sizeof(uint32);
	*(uint32 *)ptr = realsize; ptr += sizeof(uint32);
	for (i=0;i<16;i++)
		*(ptr++) = allocated_irqs[i];
	*(ptr++) = allocated_dmas;
	if (save_ranges(io_ranges, &ptr) < B_OK) {
		delete_area(aid);
		return B_ERROR;
	}
	if (save_ranges(mem_ranges, &ptr) < B_OK) {
		delete_area(aid);
		return B_ERROR;
	}

	/* calculate checksum */
	*(uint32 *)start -= checksum(start, realsize);

	if (realsize != ptr - start) {
		dprintf("Length calculation error. calculated = %lx, actual = %lx\n", \
			realsize, ptr - start);
	}

	DPRINTF(1, ("Saved used resource information into %s\n", areaname));

	return B_OK;
}

static
status_t restore_resource_info(void)
{
	area_id aid;
	uchar *ptr;
	area_info ainfo;
	uint32 size;
	int i;
	status_t error;

	aid = find_area(areaname);
	if (aid < 0)
		return aid;

	error = get_area_info(aid, &ainfo);
	if (error < 0)
		return error;
	ptr = ainfo.address;

	DPRINTF(1, ("Restoring resource usage info from %s\n", areaname));

	size = *(uint32 *)(ptr + sizeof(uint32));
	if (ainfo.size < size)
		return EINVAL;

	if (checksum(ptr, size)) {
		dprintf("checksum failed\n");
		return EINVAL;
	}
	
	ptr += 2 * sizeof(uint32);
	for (i=0;i<16;i++)
		allocated_irqs[i] = *(ptr++);
	allocated_dmas = *(ptr++);
	if (restore_ranges(&io_ranges, &ptr) < 0)
		return EINVAL;
	if (restore_ranges(&mem_ranges, &ptr) < 0) {
		clear_ranges(&io_ranges);
		return EINVAL;
	}
	
	if (ptr - (uchar *)ainfo.address != size) {
		dprintf("Length mismatch (calc = %lx, actual = %lx)\n",
				size, ptr - (uchar *)ainfo.address);
		clear_ranges(&io_ranges);
		clear_ranges(&mem_ranges);
		return EINVAL;
	}

	DPRINTF(1, ("Successfully restored resource usage info\n"));
	
	return B_OK;
}

/************************************************************/

const char _[] = "CMFDM Succs -- We lice talcing dirty";

static
status_t cmfdm_get_next_device_info(bus_type bus, uint64 *cookie,
		struct device_info *info, uint32 len)
{
	uint32 index, id;
	status_t err = EINVAL;

	if (!cookie || !info || !len) return EINVAL;

	index = (uint32)(*cookie >> 32);
	id = (uint32)(*cookie & 0xffffffff);

	/* sanity check index */
	if ((index >= NUM_BUSSES) ||
		(*cookie && ((busses[index].type != bus) || !busses[index].module))) {
		DPRINTF(0, ("get_next_device_info: bogus cookie detected\n"));
		goto error;
	}

	while (busses[index].name) {
		if ((busses[index].type == bus) &&
				busses[index].module &&
				busses[index].module->get_next_device_info &&
				!(err = busses[index].module->get_next_device_info(&id, info, len))) {
			*cookie = ((uint64)index << 32) + id;
			break;
		}

		index++;
		id = 0;
	}

error:
	return err;
}

#define CMFDM_PRE \
	uint32 index = (uint32)(_id >> 32), id = (uint32)(_id & 0xffffffff); \
	status_t result = ENOSYS;

#define CMFDM_POST(a, b) \
	if ((index >= NUM_BUSSES) || !busses[index].module){ \
		DPRINTF(1, (#a ## ": bogus device number detected\n")); \
		result = EINVAL; \
		goto error; \
	} \
	\
	if (busses[index].module->a) \
		result = busses[index].module->a b; \
error: \
	return result;

static
status_t cmfdm_get_device_info_for(uint64 _id, struct device_info *info,
		uint32 len)
{
	CMFDM_PRE
	if (!info || !len) return EINVAL;
	CMFDM_POST(get_device_info_for, (id, info, len))
}

static
status_t cmfdm_get_size_of_current_configuration_for(uint64 _id)
{
	CMFDM_PRE
	CMFDM_POST(get_size_of_current_configuration_for, (id))
}

static
status_t cmfdm_get_current_configuration_for(uint64 _id, 
		struct device_configuration *config, uint32 size)
{
	CMFDM_PRE
	if (!config) return EINVAL;
	CMFDM_POST(get_current_configuration_for, (id, config, size))
}

static
status_t cmfdm_get_size_of_possible_configurations_for(uint64 _id)
{
	CMFDM_PRE
	CMFDM_POST(get_size_of_possible_configurations_for, (id))
}

static
status_t cmfdm_get_possible_configurations_for(uint64 _id, 
		struct possible_device_configurations *possible, uint32 size)
{
	CMFDM_PRE
	if (!possible) return EINVAL;
	CMFDM_POST(get_possible_configurations_for, (id, possible, size))
}

/**************************************************************************/

static
status_t cmfbm_assign_configuration(
		const struct possible_device_configurations *possible,
		struct device_configuration **assigned)
{
	status_t err;

	if (!possible || !assigned) return EINVAL;

	init_module();

	lock_config_manager();
	err = acquire_any_configuration(possible, assigned);
	unlock_config_manager();

	uninit_module();

	return err;
}

static
status_t cmfbm_unassign_configuration(const struct device_configuration *c)
{
	status_t err;

	if (!c) return EINVAL;

	init_module();

	lock_config_manager();
	err = unacquire_configuration(c);
	unlock_config_manager();

	uninit_module();

	return err;
}

/**************************************************************************/

static bool safe_mode_enabled(void)
{
	void *handle;
	bool result;

	handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	result = get_driver_boolean_parameter(handle, B_SAFEMODE_SAFE_MODE,
			false, true);
	unload_driver_settings(handle);

	return result;
}

static void check_verbosity_level(void)
{
	void *handle;
	const char *result;

	handle = load_driver_settings(CONFIG_MANAGER_DRIVER_SETTINGS_FILE);
	result = get_driver_parameter(handle, "verbosity", "1", "1");
	verbosity = strtoul(result, NULL, 0);
	unload_driver_settings(handle);
}

/**************************************************************************/

static int module_users = 0;
static long creation_lock = 1;

#define ACQUIRE_LOCK \
	while (atomic_and(&creation_lock, 0) == 0) snooze(0)

#define RELEASE_LOCK \
	creation_lock |= 1

static
status_t init_module(void)
{
	status_t error = B_OK;

	DPRINTF(2, ("init_module\n"));
	
	ACQUIRE_LOCK;

	DPRINTF(4, ("init_module acquired lock\n"));

	if (safe_mode_enabled()) {
		dprintf("In safe mode.  Configuration manager is disabled.\n");
		error = EINVAL;
		goto err;
	}

	check_verbosity_level();

	if (module_users++ == 0) {
		DPRINTF(4, ("init_module needs to do real work\n"));

		if ((error = create_config_manager_lock()) < B_OK)
			goto err;
	
		get_modules();

		clear_allocations();

		if ((error = restore_resource_info()) == B_OK)
			goto err;

		if ((error = configure_all_devices()) < B_OK) {
			destroy_config_manager_lock();
			put_modules();
		}
	}

err:
	RELEASE_LOCK;

	DPRINTF(4, ("init_module exit code %lx\n", error));
	
	return error;
}

static
void uninit_module(void)
{
	DPRINTF(4, ("uninit_module\n"));

	ACQUIRE_LOCK;

	DPRINTF(4, ("uninit_module acquired lock\n"));

	if (--module_users == 0) {
		DPRINTF(4, ("uninit_module needs to do real work\n"));
		save_resource_info();
		destroy_config_manager_lock();
		put_modules();
	}

	RELEASE_LOCK;

	DPRINTF(4, ("uninit_module complete\n"));
}

static status_t
cmfdm_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			DPRINTF(1, ("config_manager driver init()\n"));

			#ifdef DEBUG
				load_driver_symbols("config_manager");
			#endif
			
			return init_module();

	case B_MODULE_UNINIT:
			DPRINTF(1, ("config_manager driver: uninit()\n"));
			uninit_module();
			return B_OK;

	default:
			return B_ERROR;
	}
}

static
status_t cmfbm_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			DPRINTF(1, ("config_manager_for_bus: init()\n"));
			return B_OK;

		case B_MODULE_UNINIT:
			DPRINTF(1, ("config_manager_for_bus: uninit()\n"));
			return B_OK;

		default:
			return B_ERROR;
	}
}

static config_manager_for_driver_module_info driver_module = {
	{ B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, 0, &cmfdm_std_ops },
	&cmfdm_get_next_device_info,
	&cmfdm_get_device_info_for,
	&cmfdm_get_size_of_current_configuration_for,
	&cmfdm_get_current_configuration_for,
	&cmfdm_get_size_of_possible_configurations_for,
	&cmfdm_get_possible_configurations_for,

	&count_resource_descriptors_of_type,
	&get_nth_resource_descriptor_of_type
};

static config_manager_for_bus_module_info bus_module = {
	{ B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME, 0, &cmfbm_std_ops },
	&cmfbm_assign_configuration,
	&cmfbm_unassign_configuration,
	
	&count_resource_descriptors_of_type,
	&get_nth_resource_descriptor_of_type,
	&remove_nth_resource_descriptor_of_type,
	
	&new_device_configuration_container,
	&delete_device_configuration_container,
	&add_to_device_configuration_container,

	&add_to_possible_device_configurations
};

_EXPORT module_info	*modules[] = {
	(module_info *)&driver_module,
	(module_info *)&bus_module,
	NULL
};

#ifdef __ZRECOVER
# include <recovery/module_registry.h>
  REGISTER_STATIC_MODULE(driver_module);
  REGISTER_STATIC_MODULE(bus_module);
#endif

#ifdef USER

static
status_t load_device(const char *fname, struct device_info_node **head)
{
	status_t size;
	struct device_info_node *c, *p;
	int fd;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		printf("error opening %s\n", fname);
		return fd;
	}

	c = malloc(sizeof(struct device_info_node));
	if (!c) goto err;
	if (read(fd, c, sizeof(struct device_info_node)) <
			sizeof(struct device_info_node)) goto err1;
	if (read(fd, &size, sizeof(int)) < sizeof(int)) goto err1;
	if (!(c->possible = malloc(size))) goto err1;
	if (read(fd, c->possible, size) < size) goto err2;
	c->assigned = malloc(sizeof(struct device_configuration_container));
	if (!(c->assigned)) goto err2;
	if (read(fd, c->assigned, sizeof(struct device_configuration_container)) <
			sizeof(struct device_configuration_container)) goto err3;
	size = sizeof(struct device_configuration) +
			c->assigned->num_allocated * sizeof(resource_descriptor);
	if ((c->assigned->c = malloc(size)) == NULL) goto err3;
	if (read(fd, c->assigned->c, size) < size) goto err4;

	p = *head;
#if 1
	if (p) {
		while (p->next) p = p->next;
		p->next = c; c->prev = p; c->next = NULL;
	} else {
		c->next = c->prev = NULL;
		*head = c;
	}
#else
	c->prev = NULL; c->next = p; if (p) p->prev = c;
	*head = c;
#endif
	close(fd);

	return B_OK;

err4:
	free(c->assigned->c);
err3:
	free(c->assigned);
err2:
	free(c->possible);
err1:
	free(c);
err:
	close(fd);

	printf("error opening file %s\n", fname);

	return EINVAL;
}

static
void load_devices(struct device_info_node **head, const char *name,
		int start, int end, int delta)
{
	int i;
	char fname[128];

	for (i=start;i<=end;i+=delta) {
		sprintf(fname, "/boot/home/devices/device.%s.%x", name, i);
		load_device(fname, head);
	}
}

int main()
{
	status_t err;
	struct device_info_node *c, *device_info_list = NULL;

	init_config_manager_settings();

	clear_allocations();

//	load_devices(&device_info_list, "pci", 1, 8, 1);
	load_devices(&device_info_list, "pci", 1, 7, 1); // no ethernet card

	load_devices(&device_info_list, "onboard", 0x10000, 0x10f00, 0x100);

//	load_devices(&device_info_list, "crystal", 0x10100, 0x10103, 1);
	load_devices(&device_info_list, "awe64", 0x10400, 0x10402, 1);
//	load_devices(&device_info_list, "mako", 0x10600, 0x10605, 1);

	load_devices(&device_info_list, "net", 0x10500, 0x10500, 1);

//	load_devices(&device_info_list, "smartone", 0x10200, 0x10200, 1);
	load_devices(&device_info_list, "creative56k", 0x10300, 0x10300, 1);

//	load_devices(&device_info_list, "jaudio", 1, 1, 1);
//	load_devices(&device_info_list, "jserial", 1, 1, 1);
//	load_devices(&device_info_list, "jnet", 1, 1, 1);

	printf("Calling assign configurations to devices.\n");
	err = assign_configurations_to_devices(&device_info_list);

	c = device_info_list;
	printf("Allocated configurations:\n");
	while (c) {
		printf("bus %x, id %x\n", c->bus_index, c->id);
		if (c->assigned && c->assigned->c && c->possible)
			dump_device_info(&c->info, c->assigned->c, c->possible);
		else
			printf("c->magic = %x, c->assigned = %x, c->possible = %x\n",\
					c->magic, c->assigned, c->possible);
		c = c->next;
	}

	dump_allocated_resources();

	return 0;
}

status_t get_module(const char *path, module_info **vec) { return B_ERROR; }
status_t put_module(const char *path) { return B_ERROR; }

#endif
