
#include "private.h"

#define DEBUG_ON //set_dprintf_enabled(TRUE)

#define DO_OLD 1
#define DO_MIDI 1

int32 num_names;
char* names[NUM_CARDS*6+2];
int32 num_cards;
sb_card cards[NUM_CARDS];

const char isa_name[] = B_ISA_MODULE_NAME;
const char pnp_name[] = B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME;
isa_module_info *isa;
config_manager_for_driver_module_info *pnp;

#if DO_OLD
extern device_hooks old_hooks;
#endif /* DO_OLD */
#if DO_MIDI
extern device_hooks midi_hooks;
#endif /* DO_MIDI */


static uchar
log_2(uint32 mask)
{
  uchar value;

  mask >>= 1;
  for (value = 0; mask; value++)
	mask >>= 1;

  return value;
}


/* ----------
	lock_spinner
----- */

cpu_status
lock_spinner (spinlock* spinner)
{
  cpu_status ps = disable_interrupts ();
  acquire_spinlock (spinner);
  return ps;
}


/* ----------
	unlock_spinner
----- */

void
unlock_spinner (spinlock* spinner, cpu_status ps)
{
  release_spinlock (spinner);
  restore_interrupts (ps);
}


/* ----------
	iread reads one of the wss registers
----- */

uchar
iread (sb_card* sb, int offset)
{
	uchar reg;
	cpu_status ps = lock_spinner(&sb->wss_lock);

	WRITE_IO_8(sb->wss_base, offset);
	reg = READ_IO_8(sb->wss_base + 1);
	
	unlock_spinner (&sb->wss_lock, ps);
	return reg;
}


/* ----------
	ichange changes select bits in one of the wss registers
----- */

void
ichange (sb_card* sb, int offset, uchar mask, uchar merge)
{
	uchar reg;
	cpu_status	ps = lock_spinner(&sb->wss_lock);

	WRITE_IO_8(sb->wss_base, offset);
	reg = READ_IO_8(sb->wss_base + 1);

	reg &= mask;
	reg |= merge;

	WRITE_IO_8(sb->wss_base + 1, reg);

	unlock_spinner (&sb->wss_lock, ps);
	return;
}


/* ----------
	x_read reads one of the extended registers
----- */

uchar
x_read (sb_card* sb, int offset)
{
	uchar reg;
	cpu_status ps = lock_spinner(&sb->wss_lock);

	WRITE_IO_8(sb->wss_base, 23);
	reg = READ_IO_8(sb->wss_base + 1);
	WRITE_IO_8(sb->wss_base + 1,
			   8 | (reg & 1)
			   | (offset << 4)
			   | ((offset >> 2) & 4));
	reg = READ_IO_8(sb->wss_base + 1);
	
	unlock_spinner (&sb->wss_lock, ps);
	return reg;
}


/* ----------
	x_change changes select bits in one of the extended registers
----- */

void
x_change (sb_card* sb, int offset, uchar mask, uchar merge)
{
	uchar reg;
	cpu_status ps = lock_spinner(&sb->wss_lock);

	WRITE_IO_8(sb->wss_base, 23);
	reg = READ_IO_8(sb->wss_base + 1);
	WRITE_IO_8(sb->wss_base + 1,
			   8 | (reg & 1)
			   | (offset << 4)
			   | ((offset >> 2) & 4));
	reg = READ_IO_8(sb->wss_base + 1);
	
	reg &= mask;
	reg |= merge;

	WRITE_IO_8(sb->wss_base + 1, reg);

	unlock_spinner (&sb->wss_lock, ps);
	return;
}


/* ----------
	init_hardware does the one-time-only codec hardware
----- */

status_t
init_hardware(void)
{
  DEBUG_ON;
  ddprintf(CHIP_NAME ": init_hardware()\n");
  return B_NO_ERROR;
}


void
setup_pnp(sb_card* sb, struct device_configuration* dev)
{
  int num_irqs = pnp->count_resource_descriptors_of_type(dev, B_IRQ_RESOURCE);
  int num_dmas = pnp->count_resource_descriptors_of_type(dev, B_DMA_RESOURCE);
  resource_descriptor r;

  sb->IRQ = -1;
  sb->DMA_P = -1;
  sb->DMA_C = -1;

  if (num_irqs >= 1) {
	pnp->get_nth_resource_descriptor_of_type(dev, 0, B_IRQ_RESOURCE, &r, sizeof(r));
	sb->IRQ = log_2(r.d.m.mask);
  }

  if (num_dmas > 0) {
	pnp->get_nth_resource_descriptor_of_type(dev, 0, B_DMA_RESOURCE, &r, sizeof(r));
	sb->DMA_P = log_2(r.d.m.mask);
  }
  if (num_dmas > 1) {
	pnp->get_nth_resource_descriptor_of_type(dev, 1, B_DMA_RESOURCE, &r, sizeof(r));
	sb->DMA_C = log_2(r.d.m.mask);
  }  
}
  

/* ----------
	complete_mode_change completes a pending mode change, then
	waits until the autocalibration sequence has completed
----- */

status_t
complete_mode_change (sb_card* sb)
{
	int i;
	uchar val;
	cpu_status	ps = lock_spinner(&sb->wss_lock);

	for (i = 0; READ_IO_8(sb->wss_base) & 0x80; i++)
	  if (i > 10000)					/* wait till chip readable */
		goto bail;						/* for as long as 1 second */
	  else
		spin (100);

	val = READ_IO_8(sb->wss_base); 	/* complete mode change */
	val &= ~CS_index_mce;
	WRITE_IO_8(sb->wss_base, val);

	for (i = 0; READ_IO_8(sb->wss_base) & 0x80; i++)
	  if (i > 10000)					/* wait till chip readable */
		goto bail;						/* for as long as 1 second */
	  else
		spin (100);

	unlock_spinner (&sb->wss_lock, ps);

	/* !!! should prevent others from touching h/w till this completes !!! */

	for (i = 0; iread (sb, CS_error_init) & CS_estat_aci; i++)
	  if (i > 10000)				/* wait till autocal done */
		return B_ERROR;				/* for as long as 1 second */
	  else
		spin (100);

	return B_NO_ERROR;

bail:
	unlock_spinner (&sb->wss_lock, ps);
	return B_ERROR;
}


/* ----------
	setup_wss
----- */

status_t
setup_wss(sb_card* sb, struct device_configuration* config, int32 base)
{
  int32 i;

  static struct {
	uchar	offset;			/* offset to indirect register */
	uchar	mask;			/* mask to apply to current value */
	uchar	merge;			/* new data to merge w/masked value */
  } inittab[] = {
	CS_interface_config,		0x00, 0x00,	/* all dma, autocal disabled, all disabled */
	CS_pin_control,				0x3B, 0x00,	/* ints disabled */
	CS_alt_feature_enable_1,	0x3E, 0x00,	/* no timer, use last on overrun, 2V output max */
	CS_alt_feature_status,		0x80, 0x00,	/* clear all interrupts */
	CS_capture_data_format,		0x0F, 0xD0,	/* 44.1 kHz, 16 bit stereo */
	CS_clock_play_data_format,	0x00, 0xDB	/* xtal2, 44.1 Khz, 16bit stereo */
  };

  sb->wss_base = base;
  setup_pnp(sb, config);

  for (i = sb->wss_base; i < sb->wss_base + 4; i++)
	ddprintf("%x = %02x\n", i, READ_IO_8(i));

  /* wait for chip to wake up for as long as 0.1 second */
  for (i = 0; READ_IO_8(sb->wss_base) & 0x80; i++)
	if (i > 1000)
	  return B_ERROR;
	else
	  spin (100);

  ichange(sb, CS_mode_id | CS_index_mce, 0x8F, 0x60);	/* MODE 3 */
  x_change(sb, 11, 0, 0xc0);							/* clear IFSE */

  /* initialize some indirect registers, w/mode change enabled */

  for (i = 0; i < sizeof (inittab)/sizeof(inittab[0]); i++)
	ichange (sb, inittab[i].offset | CS_index_mce,
			 inittab[i].mask, inittab[i].merge);

  if (complete_mode_change(sb) != B_OK)
	return B_ERROR;

  return find_low_memory(sb);
}


/* ----------
	teardown_card is called from uninit_driver
----- */

static void
teardown_card(sb_card* sb)
{
  area_info ainfo;

  ddprintf(CHIP_NAME ": teardown_card(%x)\n", sb);

  /* we let the contiguous low mem area live on! */
  if (!get_area_info(sb->dma_area, &ainfo))
	if (ainfo.copy_count > 1)
	  delete_area(sb->dma_area);
}


static void
unpack_eisa_id(EISA_PRODUCT_ID pid, unsigned char* str)
{
        str[0] = ((pid.b[0] >> 2) & 0x1F) + 'A' - 1;
        str[1] = ((pid.b[1] & 0xE0)>>5) | ((pid.b[0] & 0x3) << 3) + 'A' - 1;
        str[2] = (pid.b[1] & 0x1F) + 'A' - 1;
        str[3] = '\0';
}

static void
dump_eisa_id(char* mes, uint32 id)
{
        unsigned char vendor[4];
		EISA_PRODUCT_ID pid;

		pid.id = id;
        unpack_eisa_id(pid, vendor);
        dprintf("%s vendor %s, product# %x%x, revision %x\n",
				mes, vendor, pid.b[2], pid.b[3]>>4, pid.b[3] & 0xf);
}


static bool
compatible_p(struct isa_device_info* dev,
			 uint8 ch0, uint8 ch1, uint8 ch2,
			 int prod, int rev)
{
  int i;
  EISA_PRODUCT_ID id;

  MAKE_EISA_PRODUCT_ID(&id, ch0, ch1, ch2, prod, rev);

  if (id.id == dev->vendor_id)
	return true;
  if (id.id == dev->logical_device_id)
	return true;

  for (i = 0; i < dev->num_compatible_ids; i++)
	/* require strict identity */
	if (id.id == dev->compatible_ids[i])
	  return true;

  return false;
}


static struct device_configuration*
allocate_config(uint64 cookie,
				resource_descriptor* port_res,
				resource_descriptor* irq_res,
				int32 min_base,
				int32 max_base,
				int32 base_mask,
				int32 min_len)
{
  struct device_configuration* config;
  int size = pnp->get_size_of_current_configuration_for(cookie);

  if (size <= 0)
	return NULL;
  config = (struct device_configuration*) malloc(size);
  if (config == NULL)
	return NULL;

  if (pnp->get_current_configuration_for(cookie, config, size) == B_OK) {
	int i = 0;
	if (port_res == NULL)
	  return config;
	while (pnp->get_nth_resource_descriptor_of_type(config, i++, B_IO_PORT_RESOURCE,
													port_res, sizeof(*port_res))
		   == B_OK) {

	  if (port_res->d.r.minbase >= min_base
		  && port_res->d.r.minbase <= max_base
		  && (port_res->d.r.minbase & base_mask) == 0
		  && port_res->d.r.len >= min_len) {
		if (irq_res == NULL)
		  return config;
		if (pnp->get_nth_resource_descriptor_of_type(config, 0, B_IRQ_RESOURCE,
													 irq_res, sizeof(*irq_res))
			  == B_OK)
		  return config;
	  }
	}
  }

  free(config);
  return NULL;
}


static bool
same_card_p(sb_card* sb, struct isa_device_info* dev)
{
  return (sb->card_id_0 == dev->info.id[0]
		  && sb->card_id_1 == dev->info.id[1]
		  && sb->card_csn == dev->csn);
}


static void
find_control_port(sb_card* sb)
{
  uint64 cookie = 0;
  struct isa_device_info dev;
  struct device_configuration* config;
  resource_descriptor port_res;

  while(pnp->get_next_device_info(B_ISA_BUS, &cookie, &dev.info, sizeof(dev))
		== B_OK)
	if (dev.info.flags & B_DEVICE_INFO_ENABLED)
	  if (same_card_p(sb, &dev))
		if (compatible_p(&dev, 'C', 'S', 'C', 0x001, 0)
			|| compatible_p(&dev, 'C', 'S', 'C', 0x011, 0)) {
		  config = allocate_config(cookie, &port_res, NULL, 0x120, 0xfff, 7, 0);
		  if (config) {
			sb->ctrl_base = port_res.d.r.minbase;
			free(config);
		  }
		}
}


static void
find_midi_port(sb_card* sb)
{
  uint64 cookie = 0;
  struct isa_device_info dev;
  struct device_configuration* config;
  resource_descriptor port_res;
  resource_descriptor irq_res;

#if DO_MIDI
  while (pnp->get_next_device_info(B_ISA_BUS, &cookie, &dev.info, sizeof(dev))
		 == B_OK)
	if (dev.info.flags & B_DEVICE_INFO_ENABLED)
	  if (same_card_p(sb, &dev))
		if (compatible_p(&dev, 'C', 'S', 'C', 0x000, 3)
			|| compatible_p(&dev, 'C', 'S', 'C', 0x010, 3)) {
		  config = allocate_config(cookie, &port_res, &irq_res, 0x330, 0x3e0, 7, 0);
		  if (config) {
			sb->midi.base = port_res.d.r.minbase;
			sb->midi.IRQ = log_2(irq_res.d.m.mask);
			free(config);
		  }
		}
#endif
}


static void
find_next_wss_port(sb_card* sb, uint64* cookie)
{
  struct isa_device_info dev;
  struct device_configuration* config;
  resource_descriptor port_res;

  while (pnp->get_next_device_info(B_ISA_BUS, cookie, &dev.info, sizeof(dev))
		 == B_OK) {
	//dump_eisa_id("VendorID:", dev.vendor_id);
	//dump_eisa_id("DeviceID:", dev.logical_device_id);
	if (dev.num_compatible_ids > B_MAX_COMPATIBLE_IDS) {
		dprintf(CHIP_NAME ": find_next_wss_port(): ");
		dprintf("config_mgr version skew detected.\n");
		break;
	}
	if (dev.info.flags & B_DEVICE_INFO_ENABLED)
	  if (compatible_p(&dev, 'C', 'S', 'C', 0x000, 0)
		  || compatible_p(&dev, 'C', 'S', 'C', 0x010, 0)) {
		config = allocate_config(*cookie, &port_res, NULL, 0x530, 0xfff, 3, 4);
		if (config) {
		  setup_wss(sb, config, port_res.d.r.minbase);
		  sb->card_id_0 = dev.info.id[0];
		  sb->card_id_1 = dev.info.id[1];
		  sb->card_csn = dev.csn;
		  free(config);
		}
	  }
  }
}


/* ----------
	init_driver is called when the driver is loaded.
----- */

status_t
init_driver(void)
{
  sb_card* sb;
  uint64 cookie = 0;

  dprintf(CHIP_NAME ": init_driver()\n");

  if (get_module(isa_name, (module_info **) &isa) != B_OK)
	return B_ERROR;
  if (get_module(pnp_name, (module_info **) &pnp) != B_OK) {
	dprintf("failed to get_module(pnp)\n");
	put_module(isa_name);
	return B_ERROR;
  }

  memset(cards, 0, sizeof(cards));

  for (num_cards = 0; num_cards < NUM_CARDS; num_cards++) {
	sb = &cards[num_cards];
	sprintf(sb->name, "%s/%d", CHIP_NAME, num_cards+1);
	find_next_wss_port(sb, &cookie);
	if (!sb->wss_base)
	  break;
	find_control_port(sb);
	find_midi_port(sb);
	make_device_names(sb);
  }

  if (!num_cards) {
	put_module(isa_name);
	put_module(pnp_name);
	return B_ERROR;
  }

  return B_OK;
}


/* ----------
	uninit_driver - clean up before driver is unloaded
----- */

void
uninit_driver (void)
{
  int i;
  int n = num_cards;
  num_cards = 0;

  dprintf(CHIP_NAME ": uninit_driver()\n");

  for (i = 0; i < n; i++)
	teardown_card(&cards[i]);

  memset(&cards, 0, sizeof(cards));

  put_module(isa_name);
  put_module(pnp_name);
}

void
make_device_names(sb_card* sb)
{
	char* name = sb->name;
	sprintf(name, CHIP_NAME "/%d", sb - cards + 1);

#if DO_OLD
	sprintf(sb->old.name, "audio/old/%s", name);
	names[num_names++] = sb->old.name;
#endif /* DO_OLD */
#if DO_MIDI
	sprintf(sb->midi.name, "midi/%s", name);
	if (sb->midi.base)
	  names[num_names++] = sb->midi.name;
#endif /* DO_MIDI */
	names[num_names] = NULL;
}


status_t
find_low_memory(sb_card* sb)
{
	size_t low_size = 2 * MAX_DMA_BUF_SIZE;
	physical_entry where;
	status_t err;
	size_t trysize;
	area_id curarea;
	void* addr;
	char name[DEVNAME];

	sprintf(name, "%s_low", sb->name);
	trysize = low_size;
	curarea = find_area(name);
	if (curarea >= 0) {
		ddprintf(CHIP_NAME ": cloning previous area...\n");
		curarea = clone_area(name, &addr, B_ANY_KERNEL_ADDRESS, 
							 B_READ_AREA | B_WRITE_AREA, curarea);
	}
	if (curarea >= 0) {	/* area there from previous run */
		area_info ainfo;
		ddprintf(CHIP_NAME ": testing likely candidate...\n");
		if (get_area_info(curarea, &ainfo)) {
			ddprintf(CHIP_NAME ": no info\n");
			goto allocate;
		}
		/* test area we found */
		trysize = ainfo.size;
		addr = ainfo.address;
		if (trysize < low_size) {
			ddprintf(CHIP_NAME ": too small (%x)\n", trysize);
			goto allocate;
		}
		if (get_memory_map(addr, trysize, &where, 1) < B_OK) {
			ddprintf(CHIP_NAME ": no memory map\n");
			goto allocate;
		}
		if ((uint32)where.address & 0xff000000) {
			ddprintf(CHIP_NAME ": bad physical address\n");
			goto allocate;
		}
		if (ainfo.lock < B_FULL_LOCK || where.size < low_size) {
			ddprintf(CHIP_NAME ": lock not contiguous\n");
			goto allocate;
		}
		ddprintf("physical %x  logical %x\n", where.address, ainfo.address);
		goto a_o_k;
	}

allocate:
	if (curarea >= 0) {
		delete_area(curarea); /* clone didn't work */
	}
	ddprintf(CHIP_NAME ": allocating new low area\n");
	curarea = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, trysize,
						  B_LOMEM, B_READ_AREA | B_WRITE_AREA);
	ddprintf(CHIP_NAME ": create_area(%x) returned %x logical %x\n", 
			 trysize, curarea, addr);
	if (curarea >= 0) {
	  err = get_memory_map(addr, low_size, &where, 1);
	  if (err >= 0)
		ddprintf(CHIP_NAME ": physical %x\n", where.address);
	  if (err < 0
		  || ((uint32) where.address) & 0xff000000
		  || ((uint32) where.address + low_size - 1) & 0xff000000) {
		delete_area(curarea);
		curarea = (err < 0 ? err : B_ERROR);
	  }
	}
	if (curarea < 0) {
	  dprintf(CHIP_NAME ": failed to create low_mem area\n");
	  return curarea;
	}

a_o_k:
	ddprintf(CHIP_NAME ": successfully found low area at 0x%x\n", where.address);
	sb->dma_area = curarea;
	sb->dma_addr = (char*) addr;
	sb->dma_phys_addr = (void*) where.address;
	memset(sb->dma_addr, 0, low_size);
	return B_OK;
}


const char**
publish_devices(void)
{
  ddprintf(CHIP_NAME ": publish_devices()\n");

  return (const char**) names;
}


device_hooks*
find_device(const char* name)
{
  int i;

  ddprintf(CHIP_NAME ": find_device(%s)\n", name);

  for (i = 0; i < num_cards; i++) {
#if DO_OLD
	if (!strcmp(cards[i].old.name, name))
	  return &old_hooks;
#endif /* DO_MIDI */
#if DO_MIDI
	if (!strcmp(cards[i].midi.name, name))
	  return &midi_hooks;
#endif /* DO_MIDI */
	}
	ddprintf(CHIP_NAME ": find_device(%s) failed\n", name);
	return NULL;
}


static int32
int_handler(void* data)
{
  int32 handled = B_UNHANDLED_INTERRUPT;
  sb_card* sb = (sb_card*) data;
  uchar status = iread (sb, CS_alt_feature_status);

  //analyser (0x124, 0);
  //ddprintf(int_handler:  0x%02x\n", status);

  if (status & CS_alt_status_pi) {			/* playback interrupt? */
	ichange (sb, CS_alt_feature_status,		/* clear interrupt */
			 ~CS_alt_status_pi, 0x00);
	pcm_interrupt(&sb->pcm.p);
	handled = B_INVOKE_SCHEDULER;
  }

  if (status & CS_alt_status_ci) {			/* capture interrupt? */
	ichange (sb, CS_alt_feature_status,		/* clear interrupt */
			 ~CS_alt_status_ci, 0x00);
	pcm_interrupt(&sb->pcm.c);
	handled = B_INVOKE_SCHEDULER;
  }

  return handled;
}


void
increment_interrupt_handler(sb_card* sb)
{
  if (atomic_add(&sb->inth_count, 1) == 0) {
	ddprintf("installing IRQ%d handler for %s\n", sb->IRQ, sb->name);
	install_io_interrupt_handler(sb->IRQ, int_handler, sb, 0);
  }
}


void
decrement_interrupt_handler(sb_card* sb)
{
  if (atomic_add(&sb->inth_count, -1) == 1) {
	ddprintf("uninstalling IRQ%d handler for %s\n", sb->IRQ, sb->name);
	remove_io_interrupt_handler(sb->IRQ, int_handler, sb);
  }
}
