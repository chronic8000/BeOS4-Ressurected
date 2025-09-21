
#include "opti_private.h"

#define DEBUG_ON //set_dprintf_enabled(TRUE)

#define DO_OLD 1
#define DO_MIDI 1

int32 num_names;
char* names[NUM_CARDS*6+2];
int32 num_cards;
sb_card cards[NUM_CARDS];

static const char isa_name[] = B_ISA_MODULE_NAME;
isa_module_info* isa;

static const char pnp_name[] = B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME;
config_manager_for_driver_module_info* pnp;

static const char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module* mpu401;

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
	char		reg;
	cpu_status	ps = lock_spinner(&sb->wss_lock);

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
	uchar		data;
	cpu_status	ps = lock_spinner(&sb->wss_lock);

	WRITE_IO_8(sb->wss_base, offset);
	data = READ_IO_8(sb->wss_base + 1);

	data &= mask;
	data |= merge;

	WRITE_IO_8(sb->wss_base + 1, data);

	unlock_spinner (&sb->wss_lock, ps);
	return;
}


/* ----------
	mc_read reads one of the mc registers
----- */

uchar
mc_read (sb_card* sb, int offset)
{
	char		reg;
	cpu_status	ps = lock_spinner(&sb->mc_lock);

	WRITE_IO_8(sb->mc_base, offset);
	reg = READ_IO_8(sb->mc_base + 1);
	
	unlock_spinner (&sb->mc_lock, ps);
	return reg;
}


/* ----------
	mc_change changes select bits in one of the mc registers
----- */

void
mc_change (sb_card* sb, int offset, uchar mask, uchar merge)
{
	uchar		data;
	cpu_status	ps = lock_spinner(&sb->mc_lock);

	WRITE_IO_8(sb->mc_base, offset);
	data = READ_IO_8(sb->mc_base + 1);

	data &= mask;
	data |= merge;

	WRITE_IO_8(sb->mc_base + 1, data);

	unlock_spinner (&sb->mc_lock, ps);
	return;
}


/* ----------
	init_hardware does the one-time-only codec hardware
----- */

status_t
init_hardware(void)
{
  DEBUG_ON;
  dprintf(CHIP_NAME ": init_hardware()\n");
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
setup_wss(sb_card* sb, struct device_configuration* config)
{
  uchar val;
  int32 i;

  static const char interrupt_bits[16] = {
    0, 0, 0, 0, 0, 0x28, 0, 0x08,
	0, 0x10, 0x18, 0x20, 0, 0, 0, 0
  };
  static const char dma_c_bits[4] = {
    4, 4, 0, 0
  };
  static const char dma_p_bits[4] = {
    1, 2, 0, 3
  };

  static struct {
	uchar	offset;			/* offset to indirect register */
	uchar	mask;			/* mask to apply to current value */
	uchar	merge;			/* new data to merge w/masked value */
  } inittab[] = {
	CS_interface_config,		0x30, 0x08,	/* all dma, autocal enabled, all disabled */
	CS_pin_control,				0x3B, 0x00,	/* ints disabled */
	CS_capture_data_format,		0x0F, 0xD0,	/* 44.1 kHz, 16 bit stereo */
	CS_clock_play_data_format,	0x00, 0xDB,	/* xtal2, 44.1 Khz, 16bit stereo */
	16, 0x80, 0x80,			/* AUXL mute */
	17, 0x80, 0x80,			/* AUXR mute */
	22, 0x00, 0x00,			/* OUTL max */
	23, 0x00, 0x00,			/* OUTR max */
  };

  setup_pnp(sb, config);

  mc_change(sb, 5, ~0x20, 0x20);	/* Codec Expanded Mode */
  mc_change(sb, 6, 0xfc, 0x02);		/* WSS Mode */
  mc_change(sb, 1, 0xf0, 0x07);		/* enable gameport, disable CD */

  for (i = 1; i < 27; i++)
	ddprintf("MCIR%d  %02x\n", i, mc_read(sb, i));

  for (i = sb->wss_base - 4; i < sb->wss_base + 4; i++)
	ddprintf("%x = %02x\n", i, READ_IO_8(i));

  /* wait for chip to wake up for as long as 0.1 second */
  for (i = 0; READ_IO_8(sb->wss_base) & 0x80; i++)
	if (i > 1000)
	  return B_ERROR;
	else
	  spin (100);

  val = interrupt_bits[sb->IRQ & 15];
  if (sb->DMA_C >= 0)
	val |= dma_c_bits[sb->DMA_C & 3];
  if (sb->DMA_P >= 0)
	val |= dma_p_bits[sb->DMA_P & 3];
  ddprintf ("WSS config setup: write %.2x to MCIR3\n", val);
  mc_change(sb, 3, 0, val);	/* IRQ and DMA channels */

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

#if DO_MIDI
  (*mpu401->delete_device)(sb->midi.driver);
#endif
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


typedef enum {RES_MC, RES_WSS, RES_MPU} res_type;

static struct device_configuration*
allocate_config(uint64 cookie,
				resource_descriptor* port_res,
				resource_descriptor* irq_res,
				res_type type)
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
	  int32 base = port_res->d.r.minbase;
	  int32 len = port_res->d.r.len;
	  if ((type == RES_MC
		   && (base & 0xfffffe00) == 0xe00 && (base & 0xc) == 0xc && len == 4)
		  || (type == RES_WSS
			  && (base == 0x534 || base == 0x644 || base == 0xe84 || base == 0xf44))
		  || (type == RES_MPU
			  && (base & ~0x30) == 0x300 && len == 2))
	  {
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
		if (compatible_p(&dev, 'O', 'P', 'T', 0x000, 2)) {
		  config = allocate_config(cookie, &port_res, &irq_res, RES_MPU);
		  if (config) {
			if ((*mpu401->create_device)(port_res.d.r.minbase, &sb->midi.driver,
										 0, midi_interrupt_op, &sb->midi) == B_OK) {
			  sb->midi.IRQ = log_2(irq_res.d.m.mask);
			  sb->midi.card = sb;
			}
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
	  if (compatible_p(&dev, 'O', 'P', 'T', 0x931, 0)) {
		config = allocate_config(*cookie, &port_res, NULL, RES_MC);
		if (config) {
		  sb->mc_base = port_res.d.r.minbase + 2;
		  mc_change(sb, 3, 0, 5);
		  free(config);
		  config = allocate_config(*cookie, &port_res, NULL, RES_WSS);
		  if (config) {
			sb->wss_base = port_res.d.r.minbase;
			setup_wss(sb, config);
			sb->card_id_0 = dev.info.id[0];
			sb->card_id_1 = dev.info.id[1];
			sb->card_csn = dev.csn;
			free(config);
		  }
		  else
		  	ddprintf("WSS not found\n");
		}
		else
		  ddprintf("MC not found\n");
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
	return ENOSYS;
  if (get_module(pnp_name, (module_info **) &pnp) != B_OK) {
	dprintf("failed to get_module(pnp)\n");
	put_module(isa_name);
	return ENOSYS;
  }
#if DO_MIDI
  if (get_module(mpu401_name, (module_info **) &mpu401)) {
	put_module(pnp_name);
	put_module(isa_name);
	return ENOSYS;
  }
#endif

  memset(cards, 0, sizeof(cards));

  for (num_cards = 0; num_cards < NUM_CARDS; num_cards++) {
	sb = &cards[num_cards];
	sprintf(sb->name, "%s/%d", CHIP_NAME, num_cards+1);
	find_next_wss_port(sb, &cookie);
	if (!sb->wss_base)
	  break;
	find_midi_port(sb);
	make_device_names(sb);
  }

  if (!num_cards) {
	put_module(isa_name);
	put_module(pnp_name);
#if DO_MIDI
	put_module(mpu401_name);
#endif
	return ENODEV;
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
#if DO_MIDI
  put_module(mpu401_name);
#endif
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
	if (sb->midi.card)
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
	dprintf(CHIP_NAME ": successfully found low area at 0x%x\n", where.address);
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

  return (const char **)names;
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


int32	api_version = B_CUR_DRIVER_API_VERSION;

static int32
int_handler(void* data)
{
  int32 handled = B_UNHANDLED_INTERRUPT;
  sb_card* sb = (sb_card*) data;
  uchar status = mc_read(sb, 11);

  WRITE_IO_8(sb->wss_base + CS_status, 0);	/* clear interrupt */

  //ddprintf("int_handler:  0x%02x\n", status);

  if (status & 4)							/* playback interrupt? */
	handled = pcm_interrupt(&sb->pcm.p);

  if (status & 8)							/* capture interrupt? */
	handled = pcm_interrupt(&sb->pcm.c);

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
