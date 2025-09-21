
#include "awe_private.h"
#include <driver_settings.h>
#include <midi_driver.h>

#define DEBUG_ON //set_dprintf_enabled(TRUE)

#define DO_OLD 1
#define DO_JOY 0
#define DO_MIDI 1
#define DO_PCM 1
#define DO_MUX 1
#define DO_MIXER 1

int32 num_names;
char* names[NUM_CARDS*6+2];
int32 num_cards;
sb_card cards[NUM_CARDS];

const char isa_name[] = B_ISA_MODULE_NAME;
const char pnp_name[] = B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME;
isa_module_info *isa;
config_manager_for_driver_module_info *pnp;
static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module * mpu401;

typedef struct sb_settings sb_settings;
struct sb_settings {	//	for manual, "lame" cards
	int		base;
	int		irq;
	int		dma_p;
	int		dma_c;
	int		mpu;
};
static sb_settings settings;

static status_t
get_blaster_settings(
	const driver_parameter * parameter,
	sb_settings * settings)
{
	int ix;
	int val;
	uint32 got_mask = 0;

	settings->base = -1;
	settings->irq = -1;
	settings->dma_p = -1;
	settings->dma_c = -1;
	settings->mpu = -1;
	for (ix=0; ix<parameter->parameter_count; ix++) {
		if (!strcasecmp(parameter->parameters[ix].name, "base")) {
			if (parameter->parameters[ix].value_count > 0) {
				const char * end;
				got_mask |= 1;
				settings->base = strtol(parameter->parameters[ix].values[0], &end, 16);
				if ((settings->base < 1) || (settings->base > 65520)) {
					got_mask = 0;
				}
			}
		}
		else if (!strcasecmp(parameter->parameters[ix].name, "irq")) {
			if (parameter->parameters[ix].value_count > 0) {
				const char * end;
				got_mask |= 2;
				settings->irq = strtol(parameter->parameters[ix].values[0], &end, 10);
				if ((settings->irq < 0) || (settings->irq > 7)) {
					got_mask = 0;
				}
//				settings->irq += 8;	//	for ISA IRQs
			}
		}
		else if (!strcasecmp(parameter->parameters[ix].name, "dma_p")) {
			if (parameter->parameters[ix].value_count > 0) {
				const char * end;
				got_mask |= 4;
				settings->dma_p = strtol(parameter->parameters[ix].values[0], &end, 16);
				if ((settings->dma_p < 0) || (settings->dma_p > 7)) {
					got_mask = 0;
				}
			}
		}
		else if (!strcasecmp(parameter->parameters[ix].name, "dma_c")) {
			if (parameter->parameters[ix].value_count > 0) {
				const char * end;
				got_mask |= 8;
				settings->dma_c = strtol(parameter->parameters[ix].values[0], &end, 16);
				if ((settings->dma_c < 0) || (settings->dma_c > 7)) {
					got_mask = 0;
				}
			}
		}
		else if (!strcasecmp(parameter->parameters[ix].name, "mpu")) {
			if (parameter->parameters[ix].value_count > 0) {
				const char * end;
				got_mask |= 16;
				settings->mpu = strtol(parameter->parameters[ix].values[0], &end, 16);
				if ((settings->mpu < 1) || (settings->mpu > 65534)) {
					got_mask = 0;
				}
			}
		}
		if (got_mask == 0) {
			dprintf(CHIP_NAME ": bad value '%s'\n", parameter->parameters[ix].name);
		}
	}
	dprintf("manual setting: A%x I%d D%d H%d P%x\n", settings->base, settings->irq, 
		settings->dma_p, settings->dma_c, settings->mpu);
	return ((got_mask & 7) == 7) ? B_OK : B_BAD_VALUE;
}

static status_t
read_settings()
{
	void * handle = load_driver_settings(CHIP_NAME);
	int ix;
	const driver_settings * tree;
	status_t err = B_OK;

	dprintf(CHIP_NAME ": manual settings == 0x%x\n", handle);
	if (!get_driver_boolean_parameter(handle, "manual", false, true)) {
		dprintf(CHIP_NAME ": manual disabled\n");
		err = ENODEV;
	}
	if (!err && ((tree = get_driver_settings(handle)) == NULL)) {
		dprintf(CHIP_NAME ": no settings tree\n");
		err = ENODEV;
	}
	if (!err) {
		int ix;
		err = ENODEV;
		for (ix=0; ix<tree->parameter_count; ix++) {
			if (!strcmp(tree->parameters[ix].name, "manual")) {
				err = get_blaster_settings(&tree->parameters[ix], &settings);
				dprintf(CHIP_NAME ": get_blaster_settings returns 0x%x\n", err);
				break;
			}
		}
	}
	unload_driver_settings(handle);
	return err;
}

#if DO_OLD
extern device_hooks old_hooks;
#endif /* DO_OLD */
#if DO_MIDI
extern device_hooks midi_hooks;
#endif /* DO_MIDI */
#if DO_JOY
extern device_hooks joy_hooks;
#endif /* DO_JOY */
#if DO_PCM
extern device_hooks pcm_hooks;
#endif /* DO_PCM */
#if DO_MUX
extern device_hooks mux_hooks;
#endif /* DO_MUX */
#if DO_MIXER
extern device_hooks mixer_hooks;
#endif /* DO_MIXER */


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
	dsp_read_byte
----- */

uchar
dsp_read_byte(sb_card* sb)
{
  uchar byte;
  bigtime_t bail = system_time() + 150;
  while (0 == (0x80 & READ_IO_8(sb->base + SB_DSP_Read_Status)))
	if (system_time() > bail) {
	  dprintf ("sound blaster:  read timed out.\n");
	  return 0;
	}

  byte = READ_IO_8(sb->base + SB_DSP_Read);
  ddprintf("DSP reading 0x%02x\n",byte);
  return byte;
}


/* ----------
	dsp_write_byte
----- */

void
dsp_write_byte(sb_card* sb, uchar byte)
{
  bigtime_t bail = system_time() + 150;
  while (0x80 & READ_IO_8(sb->base + SB_DSP_Write_Status))
	if (system_time() > bail) {
	  dprintf ("sound blaster:  write timed out.\n");
	  return;
	}

  ddprintf("DSP writing 0x%02x\n",byte);
  WRITE_IO_8(sb->base + SB_DSP_Write, byte);
}


/* ----------
	dsp_read_n (high byte first order)
----- */

int32
dsp_read_n(sb_card* sb, uchar command, uchar n)
{
  int i;
  int bytes = 0;
  cpu_status ps = lock_spinner(&sb->dsp_lock);

  dsp_write_byte(sb, command);
  for (i = 0; i < n; i++) {
	bytes <<= 8;
	bytes += dsp_read_byte(sb);
  }

  unlock_spinner (&sb->dsp_lock, ps);
  return bytes;
}


/* ----------
	dsp_write_n (low byte first order)
----- */

void
dsp_write_n(sb_card* sb, uchar command, uchar n, int32 data)
{
  int i;
  cpu_status ps = lock_spinner(&sb->dsp_lock);
  ddprintf("dsp_write_n(0x%x, 0x%x, 0x%x)\n",command,n,data);

  dsp_write_byte(sb, command);
  for (i = 0; i < n; i++) {
	dsp_write_byte(sb, data);
	data >>= 8;
  }

  unlock_spinner (&sb->dsp_lock, ps);
}


/* ----------
	mixer_reg reads one of the mixer registers
----- */

uchar
mixer_reg (sb_card* sb, uchar reg)
{
  uchar byte;
  cpu_status ps = lock_spinner(&sb->mix_lock);

  WRITE_IO_8(sb->base + SB_Mix_Address, reg);
  byte = READ_IO_8(sb->base + SB_Mix_Data);

  unlock_spinner(&sb->mix_lock, ps);
  return byte;
}


/* ----------
	mixer_change changes select bits in one of the mixer registers
----- */

void
mixer_change (sb_card* sb, uchar reg, uchar mask, uchar merge)
{
  uchar byte;
  cpu_status ps = lock_spinner(&sb->mix_lock);

  WRITE_IO_8(sb->base + SB_Mix_Address, reg);
  byte = READ_IO_8(sb->base + SB_Mix_Data);
  byte &= mask;
  byte |= merge;
  WRITE_IO_8(sb->base + SB_Mix_Data, byte);

  unlock_spinner(&sb->mix_lock, ps);
}


/* ----------
	find_codec - checks if codec is present, sets up pointers
----- */

static int32
find_codec (int32 reg)
{
  int i;
  int base = reg;
  uchar status;
  area_info a;
  area_id id;

  ddprintf("find_codec(0x%x)\n",reg);

  if (platform() == B_MAC_PLATFORM)
	return 0;

  WRITE_IO_8(base + SB_DSP_Reset, 1);
  spin(100);
  WRITE_IO_8(base + SB_DSP_Reset, 0);

  /* wait for chip to wake up for as long as 0.01 second */
  for (i = 100; i > 0; i--)
	if (READ_IO_8(base + SB_DSP_Read_Status) & 0x80)
	  break;
	else
	  spin (100);

  if (i == 0) {
	dprintf("read_io_8(0x%x) = 0x%02x\n", base + SB_DSP_Read_Status,
			(uchar) READ_IO_8(base + SB_DSP_Read_Status));
	dprintf("find_codec(0x%x):  read status timed out.\n", reg);
	return 0;
  }
  status = READ_IO_8(base + SB_DSP_Read);
  if (status != 0xaa) {
	dprintf("read_io_8(0x%x) = 0x%02x\n", base + SB_DSP_Read, status);
	return 0;
  }

  return base;
}
	

/* ----------
	init_hardware does the one-time-only codec hardware
----- */

status_t
init_hardware(void)
{
  int i;
  int base;

  DEBUG_ON;
  dprintf(CHIP_NAME ": init_hardware()\n");

  return B_NO_ERROR;
}


static void
setup_pnp(sb_card* sb, struct device_configuration* dev)
{
  int num_irqs = pnp->count_resource_descriptors_of_type(dev, B_IRQ_RESOURCE);
  int num_dmas = pnp->count_resource_descriptors_of_type(dev, B_DMA_RESOURCE);
  resource_descriptor r;
  int high_channel = -1;
  int low_channel = -1;
  int n;

  sb->IRQ = -1;
  sb->DMA_P = -1;
  sb->DMA_C = -1;

  if (num_irqs > 0) {
	pnp->get_nth_resource_descriptor_of_type(dev, 0, B_IRQ_RESOURCE, &r, sizeof(r));
	sb->IRQ = log_2(r.d.m.mask);
  }

  if (num_dmas > 0) {
	pnp->get_nth_resource_descriptor_of_type(dev, 0, B_DMA_RESOURCE, &r, sizeof(r));
	low_channel = log_2(r.d.m.mask);
  }
  if (num_dmas > 1) {
	pnp->get_nth_resource_descriptor_of_type(dev, 1, B_DMA_RESOURCE, &r, sizeof(r));
	n = log_2(r.d.m.mask);
	if (n > low_channel)
	  high_channel = n;
	else {
	  high_channel = low_channel;
	  low_channel = n;
	}
  }

  if (high_channel < 4)
	sb->DMA_P = low_channel;
  else {
	sb->DMA_P = high_channel;
	sb->DMA_C = low_channel;
  }

  ddprintf(CHIP_NAME ": IRQ = %d, DMA_C = %d, DMA_P = %d\n",
		   sb->IRQ, sb->DMA_C, sb->DMA_P);
}
  
void midi_interrupt_op(int32 op, void * data);

void
setup_midi(sb_card *card)
{
	card->midi.card = card; /* duh */
	
	if(card->midi.base && mpu401){
		if ((*mpu401->create_device)(card->midi.base, &card->midi.driver, 0, 
									 midi_interrupt_op, &card->midi) < B_OK){
			card->midi.driver = NULL;
		}						 
	} else {
		card->midi.driver = NULL;
	}
}

void
teardown_midi(sb_card *card)
{
	if(card->midi.base && card->midi.driver){
		(*mpu401->delete_device)(card->midi.driver);
	}
}

/* ----------
	setup_sblaster is called from init_driver
----- */

static status_t
setup_sblaster(sb_card* sb, struct device_configuration* dev, int32 base)
{
  int i;

  ddprintf(CHIP_NAME ": setup_sblaster(%x)\n", sb);

  sb->base = base;
  if (sb->base == 0 || find_codec(sb->base) == 0)
	return B_ERROR;

  sb->version = dsp_read_n(sb, SB_DSP_GetVersion, 2);
  dprintf("Sound Blaster DSP version %d.%02d found at 0x%x.\n",
		  sb->version >> 8, sb->version & 0xff, sb->base);
  if (sb->version < 0x400) {
	dprintf("  Unsupported version!\n");
	return B_ERROR;
  }
  mixer_change(sb, 0, 0, 1);	/* mixer reset */
  dprintf("SB Reg 0x81 = 0x%02x\n",	mixer_reg(sb, 0x81));

  setup_pnp(sb, dev);
  return find_low_memory(sb);
}


/* ----------
	teardown_sblaster is called from uninit_driver
----- */

static void
teardown_sblaster(sb_card* sb)
{
  area_info ainfo;

  ddprintf(CHIP_NAME ": teardown_sblaster(%x)\n", sb);

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

  if (EQUAL_EISA_PRODUCT_ID(id, (EISA_PRODUCT_ID) dev->vendor_id))
	return true;
  if (EQUAL_EISA_PRODUCT_ID(id, (EISA_PRODUCT_ID) dev->logical_device_id))
	return true;

  for (i = 0; i < dev->num_compatible_ids; i++)
	if (EQUAL_EISA_PRODUCT_ID(id, (EISA_PRODUCT_ID) dev->compatible_ids[i]))
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


static void
find_next_sb_port(sb_card* sb, uint64* cookie)
{
  struct isa_device_info dev;
  struct device_configuration* config;
  resource_descriptor port_res;

  while (pnp->get_next_device_info(B_ISA_BUS, cookie, &dev.info, sizeof(dev))
		 == B_OK) {
	//dump_eisa_id("VendorID:", dev.vendor_id);
	//dump_eisa_id("DeviceID:", dev.logical_device_id);
	if (dev.num_compatible_ids > B_MAX_COMPATIBLE_IDS) {
		dprintf(CHIP_NAME ": find_next_sb_port(): ");
		dprintf("config_mgr version skew detected.\n");
		break;
	}
	if (dev.info.flags & B_DEVICE_INFO_ENABLED)
	  if (compatible_p(&dev, 'C', 'T', 'L', 0x003, 0)
		  || compatible_p(&dev, 'C', 'T', 'L', 0x004, 0)) {
		config = allocate_config(*cookie, &port_res, NULL, 0x220, 0x280, 0x1f, 0);
		if (config) {
		  if (setup_sblaster(sb, config, port_res.d.r.minbase) != B_OK)
			sb->base = 0;
		  free(config);
		}
		if (sb->base) {
#if DO_MIDI
		  config = allocate_config(*cookie, &port_res, NULL, 0x300, 0x330, 0xf, 0);
		  if (config) {
			sb->midi.base = port_res.d.r.minbase;
			free(config);
		  }
#endif
		}
	  }
  }
}


static status_t
make_manual_sb(		//	copied from setup_sblaster
	sb_card * sb,
	const sb_settings * info)
{
  int i;

  ddprintf(CHIP_NAME ": make_manual_sb(%x @ %x)\n", sb, info->base);

  sb->base = info->base;
  if ((sb->base == 0) || (find_codec(sb->base) == 0))
	return B_ERROR;

  sb->version = dsp_read_n(sb, SB_DSP_GetVersion, 2);
  dprintf("manual Sound Blaster DSP version %d.%02d found at 0x%x.\n",
		  sb->version >> 8, sb->version & 0xff, sb->base);
  if (sb->version < 0x400) {
		dprintf("  Unsupported version!\n");
		return B_ERROR;
  }
  mixer_change(sb, 0, 0, 1);	/* mixer reset */
	spin(10);
	mixer_change(sb, 0, 0, 0);	/* done resetting */

  dprintf("SB Reg 0x81 = 0x%02x\n",	mixer_reg(sb, 0x81));

	sb->midi.base = info->mpu;
	sb->IRQ = info->irq;
	sb->DMA_P = info->dma_p;
	sb->DMA_C = info->dma_c;

  return find_low_memory(sb);
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
	
  if (get_module(mpu401_name, (module_info **) &mpu401) != B_OK) {
  	/* rather than fail, let's just not publish MPU401 support */
	dprintf(CHIP_NAME ": No MPU401 module found.\n");
  	mpu401 = NULL;
  }	
  if (get_module(pnp_name, (module_info **) &pnp) != B_OK) {
	dprintf("failed to get_module(pnp)\n");
	if (mpu401) {
		put_module(mpu401_name);
	}
	put_module(isa_name);
	return B_ERROR;
  }

  memset(cards, 0, sizeof(cards));
  for (num_cards = 0; num_cards < NUM_CARDS; num_cards++) {
	sb = &cards[num_cards];
	sprintf(sb->name, "%s/%d", CHIP_NAME, num_cards+1);
	find_next_sb_port(sb, &cookie);
	if (!sb->base)
	  break;
	setup_midi(sb);
	make_device_names(sb);
  }

	if (!num_cards) {
		if (read_settings() == B_OK) {
			if (make_manual_sb(sb, &settings) == B_OK) {
				setup_midi(sb);
				make_device_names(sb);
				num_cards++;
			}
		}
	}

  if (!num_cards) {
	put_module(isa_name);
	put_module(pnp_name);
	if (mpu401) {
		put_module(mpu401_name);
	}
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

	for (i = 0; i < n; i++){
		teardown_midi(&cards[i]);
		teardown_sblaster(&cards[i]);
	}
	
  	memset(&cards, 0, sizeof(cards));

	put_module(isa_name);
	put_module(pnp_name);
	if (mpu401) {
		put_module(mpu401_name);
	}	
}

void
make_device_names(sb_card* sb)
{
	char* name = sb->name;
	sprintf(name, CHIP_NAME "/%d", sb - cards + 1);

	num_names = 0;

#if DO_OLD
	sprintf(sb->old.name, "audio/old/%s", name);
	names[num_names++] = sb->old.name;
#endif /* DO_OLD */
#if DO_MIDI
	if (mpu401) {
		sprintf(sb->midi.name, "midi/%s", name);
		if (sb->midi.base)
		  names[num_names++] = sb->midi.name;
	}	  
#endif /* DO_MIDI */
#if DO_JOY
	sprintf(sb->joy.name1, "joystick/%s/1", name);
	names[num_names++] = sb->joy.name1;
	sprintf(sb->joy.name2, "joystick/%s/2", name);
	names[num_names++] = sb->joy.name2;
#endif /* DO_JOY */
#if DO_PCM
	sprintf(sb->pcm.name, "audio/raw/%s", name);
	names[num_names++] = sb->pcm.name;
#endif /* DO_PCM */
#if DO_MUX
	sprintf(sb->mux.name, "audio/mux/%s", name);
	names[num_names++] = sb->mux.name;
#endif /* DO_MUX */
#if DO_MIXER
	sprintf(sb->mix.name, "audio/mix/%s", name);
	names[num_names++] = sb->mix.name;
#endif /* DO_MIXER */
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
#endif /* DO_OLD */
#if DO_MIDI
	if (mpu401) {
		if (!strcmp(cards[i].midi.name, name))
		  return &midi_hooks;
	} 
#endif /* DO_MIDI */
#if DO_JOY
	if (!strcmp(cards[i].joy.name1, name))
	  return &joy_hooks;
	if (!strcmp(cards[i].joy.name2, name))
	  return &joy_hooks;
#endif /* DO_JOY */
#if DO_PCM
	if (!strcmp(cards[i].pcm.name, name))
	  return &pcm_hooks;
#endif /* DO_PCM */
#if DO_MUX
	if (!strcmp(cards[i].mux.name, name))
	  return &mux_hooks;
#endif /* DO_MUX */
#if DO_MIXER
	if (!strcmp(cards[i].mix.name, name))
	  return &mixer_hooks;
#endif /* DO_MIXER */
	}
	ddprintf(CHIP_NAME ": find_device(%s) failed\n", name);
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;

static int32
sblaster_interrupt(void* data)
{
  sb_card* sb = (sb_card*) data;
  uchar int_status;
  int32 handled = B_HANDLED_INTERRUPT;
	/* We have to assume handled because this is an edge-triggered, ISA interrupt
	** that may occur before we can process it.  If we return unhandled we get 
	** disabled periodially, causing general unhappiness
	*/

  //analyser (0x124, 0);

  int_status = mixer_reg(sb, 0x82);

  if (int_status & DMA_INT_8_BIT) {
	vuchar ack = READ_IO_8(sb->base + SB_DSP_DMA_8_Ack);
	pcm_interrupt(sb, &sb->pcm.c);
	handled = B_INVOKE_SCHEDULER;
  }
  if (int_status & DMA_INT_16_BIT) {
	vuchar ack = READ_IO_8(sb->base + SB_DSP_DMA_16_Ack);
	pcm_interrupt(sb, &sb->pcm.p);
	handled = B_INVOKE_SCHEDULER;
  }

#if DO_MIDI
  if (int_status & DMA_INT_MIDI_BIT) {
	midi_interrupt(sb);
	handled = B_INVOKE_SCHEDULER;
  }
#endif
  return handled;
}


void
increment_interrupt_handler(sb_card* sb)
{
  if (atomic_add(&sb->inth_count, 1) == 0)
	install_io_interrupt_handler(sb->IRQ, sblaster_interrupt, sb, 0);
}


void
decrement_interrupt_handler(sb_card* sb)
{
  if (atomic_add(&sb->inth_count, -1) == 1)
	remove_io_interrupt_handler(sb->IRQ, sblaster_interrupt, sb);
}
