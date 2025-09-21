#include "emu_private.h"
#include <memory.h>
#include <malloc.h>
#include <stdio.h>

static const char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info* pci;

static int32 num_cards;
static emu_card cards[NUM_CARDS];
static char* emu_names[NUM_CARDS+1];

static status_t emu_open(const char* name, uint32 flags, void** cookie);
static status_t emu_close(void* cookie);
static status_t emu_free(void* cookie);
static status_t emu_control(void* cookie, uint32 op, void* data, size_t len);
static status_t emu_read(void* cookie, off_t pos, void* data, size_t* len);
static status_t emu_write(void* cookie, off_t pos, const void* data, size_t* len);

static void shutdown_emu(emu_card* card);

static device_hooks emu_hooks = {
    &emu_open,
    &emu_close,
    &emu_free,
    &emu_control,
	&emu_read,
	&emu_write
};

const char**
publish_devices(void)
{
  ddprintf(CHIP_NAME ": publish_devices()\n");
  return (const char**) emu_names;
}

device_hooks*
find_device(const char* name)
{
  ddprintf(CHIP_NAME ": find_device(%s)\n", name);
  return &emu_hooks;
}


/* ----------
	lock_spinner
----- */

static cpu_status
lock_spinner (spinlock* spinner)
{
  cpu_status ps = disable_interrupts ();
  acquire_spinlock (spinner);
  return ps;
}

/* ----------
	unlock_spinner
----- */

static void
unlock_spinner (spinlock* spinner, cpu_status ps)
{
  release_spinlock (spinner);
  restore_interrupts (ps);
}

static int32
emu_interrupt(void* arg)
{
  int32 handled = B_UNHANDLED_INTERRUPT;
  emu_card* card = (emu_card*) arg;
  emu_int_data* data = &card->int_data;
  uint32 base = AUDIO_BASE(card);
  int32 current_global;
  int32 current_high = 0;
  int32 current_low = 0;
  int32 temp_global;
  bool new_int = false;

  acquire_spinlock(&card->reg_lock);

  /* Read the interrupt pending registers */
  current_global = READ_IO_32(base + INTP);
  EIEIO();
  if (current_global) {
	handled = B_HANDLED_INTERRUPT;

	if (current_global & EINTP_CL) {
	  WRITE_IO_32(base + PTRREG, CLIPL);
	  EIEIO();
	  current_low = READ_IO_32(base + DATAREG);
	  EIEIO();
	  WRITE_IO_32(base + PTRREG, CLIPH);
	  EIEIO();
	  current_high = READ_IO_32(base + DATAREG);
	  EIEIO();
	}

	/* Read the MIDI receive data if there is any */
	temp_global = current_global;

	/* If there's more than 16 bytes in the FIFO, that means that
	   more data sneaked in since we started reading.  If there are
	   more than a couple of extra bytes, there's something seriously
	   wrong! */
	while(temp_global & EINTP_RX) {
	  uint8 datum = READ_IO_8(base + MUDTA);
	  EIEIO();
	  if (card->int_sem >= 0)
		if (data->midi_bytes < 32)
		  data->midi_data[data->midi_bytes++] = datum;
	  temp_global = READ_IO_32(base + INTP);
	  EIEIO();
	}

	if (temp_global & EINTP_TX) {
	  WRITE_IO_32(base + INTE, READ_IO_32(base + INTE) & ~EINTP_TX);
	  EIEIO();
	  temp_global &= ~EINTP_TX;
	}

	/* Clear the interrupt pending registers */
	if (temp_global & EINTP_CL) {
	  WRITE_IO_32(base + PTRREG, CLIPL);
	  EIEIO();
	  WRITE_IO_32(base + DATAREG, current_low);
	  EIEIO();
	  WRITE_IO_32(base + PTRREG, CLIPH);
	  EIEIO();
	  WRITE_IO_32(base + DATAREG, current_high);
	  EIEIO();
	}
	WRITE_IO_32(base + INTP, temp_global);
	EIEIO();

	if (card->int_sem >= 0) {
	  if (data->global_data == 0) {
		new_int = true;
		data->timestamp = system_time();
	  }
	  data->global_data |= current_global;
	  data->low_channel_data |= current_low;
	  data->high_channel_data |= current_high;
	}
  }

  release_spinlock(&card->reg_lock);
  if (new_int) {
//	kprintf("*", card->int_sem);
	release_sem_etc(card->int_sem, 1, B_DO_NOT_RESCHEDULE);
	handled = B_INVOKE_SCHEDULER;
  }
//  else if (current_global & EINTP_CL) kprintf("#");

  return handled;
}

static status_t
get_int_data(emu_card* card, emu_int_data* data)
{
	emu_int_data local_data;
	cpu_status ps = lock_spinner(&card->reg_lock);
	local_data = card->int_data;
	memset(&card->int_data, 0, sizeof(card->int_data));
	unlock_spinner(&card->reg_lock, ps);
	*data = local_data;
	return B_OK;
}

status_t
init_hardware(void)
{
  dprintf(CHIP_NAME ": init_hardware()\n");
  return B_NO_ERROR;
}

static status_t
setup_emu(emu_card* card)
{
  pci_info* info = &card->info;

  ddprintf(CHIP_NAME ": base address 0x%08x\n",
  			info->u.h0.base_registers[0]);

  if (info->u.h0.interrupt_line == 15) {
  	dprintf(CHIP_NAME ": cannot use assigned IRQ15\n");
  	return B_ERROR;
  }

  WRITE_PCI_CONFIG(info->bus, info->device, info->function, 4, 2, 5);
  ddprintf(CHIP_NAME ": PCI CR 0x%04x\n",
    	READ_PCI_CONFIG(info->bus, info->device, info->function, 4, 2));

  card->reg_lock = 0;
  card->int_sem = B_BAD_SEM_ID;
  memset(&card->int_data, 0, sizeof(card->int_data));

  return B_NO_ERROR;
}

status_t
init_driver(void)
{
  int i;
  pci_info info;

  dprintf(CHIP_NAME ": init_driver()\n");
  
  if (get_module(pci_name, (module_info**) &pci))
	return ENOSYS;

  for (i = 0; num_cards < NUM_CARDS; i++) {
	if (GET_NTH_PCI_INFO(i, &info) != B_OK)
	  break;
  	if (info.vendor_id == EMU_VENDOR_ID)
  	  if (info.device_id == EMU_DEVICE_ID) {
		memset(&cards[num_cards], 0, sizeof(cards[0]));
		cards[num_cards].info = info;
		if (setup_emu(&cards[num_cards]) == B_OK)
		  ++num_cards;
		else
		  dprintf("Setup of " CHIP_NAME "_%d failed\n", num_cards + 1);
 	  }
  }

  for (i = 0; i < num_cards; i++) {
	sprintf(cards[i].name, "audio/" CHIP_NAME "/%d", i + 1);
	emu_names[i] = cards[i].name;
  }
  emu_names[num_cards] = NULL;

  if (num_cards)
  	return B_OK;

  put_module(pci_name);
  return B_ERROR;
}

void
uninit_driver(void)
{
  dprintf(CHIP_NAME ": uninit_driver()\n");
  put_module(pci_name);
}

static status_t
emu_open(const char *name, uint32 flags, void **cookie)
{
	emu_card* card = &cards[0];
	status_t err;

	ddprintf(CHIP_NAME ": emu_open()\n");
	if (atomic_add (&card->open_count, 1) == 0) {
		int32 irq = card->info.u.h0.interrupt_line;
		ddprintf("installing IRQ%d\n", irq);
		err = install_io_interrupt_handler(irq, emu_interrupt, card, 0);
		if (err < 0) {
			emu_close((void*) card);
			return err;
		}
	}

	*cookie = card;
	return B_NO_ERROR;
}

static status_t
emu_close(void *cookie)
{
	emu_card* card = (emu_card*) cookie;
	status_t err;

	ddprintf(CHIP_NAME ": emu_close()\n");

	if (atomic_add(&card->open_count, -1) == 1) {
		shutdown_emu(card);
		ddprintf("uninstalling IRQ%d\n", card->info.u.h0.interrupt_line);
		err = remove_io_interrupt_handler(card->info.u.h0.interrupt_line,
										  emu_interrupt, card);
		if (err < 0)
			return err;
	}

	return B_NO_ERROR;
}

static status_t
emu_free(void *cookie)
{
	ddprintf(CHIP_NAME ": emu_free()\n");
	return B_NO_ERROR;
}

static status_t
emu_read(void *cookie, off_t pos, void *data, size_t *len)
{
	ddprintf(CHIP_NAME ": emu_read(%d)\n", *len);
	*len = 0;
	return B_NO_ERROR;
}

static status_t
emu_write(void *cookie, off_t pos, const void *data, size_t *len)
{
	ddprintf(CHIP_NAME ": emu_write(d)\n", *len);
	return B_NO_ERROR;
}

static void
read_io_n(emu_reg_spec* reg, uint32 addr)
{
	switch(reg->bytes) {
	case 1: reg->data = READ_IO_8(addr); break;
	case 2: reg->data = READ_IO_16(addr); break;
	case 4: reg->data = READ_IO_32(addr); break;
	}
	EIEIO();
}

static void
write_io_n(emu_reg_spec* reg, uint32 addr)
{
	switch(reg->bytes) {
	case 1: WRITE_IO_8(addr, reg->data); break;
	case 2: WRITE_IO_16(addr, reg->data); break;
	case 4: WRITE_IO_32(addr, reg->data); break;
	}
	EIEIO();
}

static void
read_indirect(emu_card* card, emu_reg_spec* reg, uint32 areg, uint32 dreg)
{
	cpu_status ps = lock_spinner(&card->reg_lock);
	WRITE_IO_32(AUDIO_BASE(card) + areg, reg->index);
	EIEIO();
	read_io_n(reg, AUDIO_BASE(card) + dreg);
	unlock_spinner(&card->reg_lock, ps);
}

static void
write_indirect(emu_card* card, emu_reg_spec* reg, uint32 areg, uint32 dreg)
{
	cpu_status ps = lock_spinner(&card->reg_lock);
	WRITE_IO_32(AUDIO_BASE(card) + areg, reg->index);
	EIEIO();
	write_io_n(reg, AUDIO_BASE(card) + dreg);
	unlock_spinner(&card->reg_lock, ps);
}

static status_t
alloc_area(emu_area_spec* spec)
{
	ddprintf("alloc_area(0x%x)\n", spec->size);
	spec->area = create_area("Emu10k memory", &spec->addr,
								B_ANY_ADDRESS,
								spec->size, 0,
								B_READ_AREA | B_WRITE_AREA);

	return (spec->area < 0 ? spec->area : B_OK);
}

static status_t
alloc_contiguous(emu_area_spec* spec)
{
	physical_entry map;
	status_t err;

	ddprintf("alloc_contiguous(0x%x)\n", spec->size);
	spec->area = create_area("Emu10k contiguous memory", &spec->addr,
								B_ANY_ADDRESS,
								spec->size, B_CONTIGUOUS,
								B_READ_AREA | B_WRITE_AREA);
	if (spec->area < 0)
		return spec->area;

	err = get_memory_map(spec->addr, B_PAGE_SIZE, &map, 1);
	ddprintf("Area %d:  0x%08x  physical:  0x%08x\n",
		spec->area, spec->addr, map.address);
	if (err < 0 || 0x8000000 & (uint32) map.address){
		delete_area(spec->area);
		return B_ERROR;
	}
	spec->phys_addr = map.address;

	return B_OK;
}

static status_t
lock_range(emu_area_spec* spec)
{
	int n;
	int p;
	status_t err;
	physical_entry* map;

	ddprintf("lock_range(0x%x, 0x%x)\n", spec->addr, spec->size);
	err = lock_memory(spec->addr, spec->size, 0);
	if (err < 0) {
		dprintf("lock_range: lock_memory() failed.\n");
		return err;
	}

	if (spec->map_entries == 0)
		return B_OK;

	map = (physical_entry*) malloc(spec->map_entries * sizeof(*map));
	if (map == NULL) {
		dprintf("lock_range: malloc() failed.\n");
		return B_NO_MEMORY;
	}

	err = get_memory_map(spec->addr, spec->size, map, spec->map_entries);
	if (err < 0) {
		dprintf("lock_range: get_memory_map() failed.\n");
		free(map);
		return err;
	}

	n = 0;
	for (p = 0; p < spec->map_entries; p++) {
		map[n].size -= 4096;
		while ((int32) map[n].size < 0) {
			if (++n >= spec->map_entries)
				goto exuent;
			map[n].size -= 4096;
		}
		//ddprintf("0x%x\n", map[n].address);
		spec->phys_map[p] = map[n].address;
		map[n].address = (char*) map[n].address + 4096;
	}

exuent:
	free(map);
	return B_OK;
}

static status_t
unlock_range(emu_area_spec* spec)
{
	status_t err = unlock_memory(spec->addr, spec->size, 0);
	if (err < 0) {
		dprintf("unlock_range: lock_memory() failed.\n");
		return err;
	}
	return B_OK;
}

static void
set_int_sem(emu_card* card, sem_id sem)
{
	cpu_status ps = lock_spinner(&card->reg_lock);
	card->int_sem = sem;
	memset(&card->int_data, 0, sizeof(card->int_data));
	unlock_spinner(&card->reg_lock, ps);
}

static status_t
emu_control(void *cookie, uint32 op, void *data, size_t len)
{
	emu_card* card = (emu_card*) cookie;
	emu_reg_spec* reg = (emu_reg_spec*) data;

	switch (op) {
	case EMU_READ_SE_REG:
		read_io_n(reg, AUDIO_BASE(card) + reg->index);
		break;
	case EMU_WRITE_SE_REG:
		write_io_n(reg, AUDIO_BASE(card) + reg->index);
		break;
	case EMU_READ_SE_PTR:
		read_indirect(card, reg, PTRREG, DATAREG);
		break;
	case EMU_WRITE_SE_PTR:
		write_indirect(card, reg, PTRREG, DATAREG);
		break;
	case EMU_READ_AC97:
		read_indirect(card, reg, AC97A, AC97D);
		break;
	case EMU_WRITE_AC97:
		write_indirect(card, reg, AC97A, AC97D);
		break;
	case EMU_READ_MODEM_REG:
	case EMU_WRITE_MODEM_REG:
		/* not implemented */
		break;
	case EMU_READ_JOYSTICK:
		WRITE_IO_8(JOY_BASE(card), 0);
		EIEIO();
		/* fall through */
	case EMU_READ_JOY_REG:
		read_io_n(reg, JOY_BASE(card) + reg->index);
		break;
	case EMU_WRITE_JOY_REG:
		write_io_n(reg, JOY_BASE(card) + reg->index);
		break;
	case EMU_ALLOC_AREA:
		ddprintf("EMU_ALLOC_AREA\n");
		return alloc_area((emu_area_spec*) data);
	case EMU_ALLOC_CONTIGUOUS_AREA:
		ddprintf("EMU_ALLOC_CONTIGUOUS_AREA\n");
		return alloc_contiguous((emu_area_spec*) data);
	case EMU_LOCK_RANGE:
		ddprintf("EMU_LOCK_RANGE\n");
		return lock_range((emu_area_spec*) data);
	case EMU_UNLOCK_RANGE:
		ddprintf("EMU_UNLOCK_RANGE\n");
		return unlock_range((emu_area_spec*) data);
	case EMU_GET_SUBSYSTEM_ID:
		ddprintf("EMU_GET_SUBSYSTEM_ID\n");
		*(uint32*)data = card->info.u.h0.subsystem_id;
		break;
	case EMU_GET_INTERRUPT_SEM:
		ddprintf("EMU_GET_INTERRUPT_SEM\n");
		*(sem_id*) data = card->int_sem;
		break;
	case EMU_SET_INTERRUPT_SEM:
		ddprintf("EMU_SET_INTERRUPT_SEM\n");
		set_int_sem(card, *(sem_id*) data);
		break;
	case EMU_GET_INTERRUPT_DATA:
		//ddprintf("EMU_GET_INTERRUPT_DATA\n");
		return get_int_data(card, (emu_int_data*) data);
		break;
	default:
		dprintf("emu_control(): unrecognized ioctl code: 0x%08x\n", op);
	}

	return B_NO_ERROR;
}

static void
wait_a_clock(emu_card* card)
{
  emu_reg_spec reg;
  int32 last;
  int32 i;

  reg.bytes = 4;
  read_io_n(&reg, AUDIO_BASE(card) + WC);
  last = reg.data >> 6;
  for (i = 0; i < 20; i++) {
	read_io_n(&reg, AUDIO_BASE(card) + WC);
	if ((reg.data >> 6) != last)
	  break;
	snooze(500);
  }
}

static void
shutdown_emu(emu_card* card)
{
  emu_reg_spec reg;
  int32 v;
  int32 i;

  ddprintf(CHIP_NAME ": shutdown_emu()\n");

  reg.bytes = 2;
  for (v = 0; v < 64; v++) {
	reg.index = VEDS | v;
	reg.data = 0x7f;
	write_indirect(card, &reg, PTRREG, DATAREG);
  }
  wait_a_clock(card);

  reg.bytes = 4;
  for (v = 0; v < 64; v++) {
	reg.index = VTFT | v;
	reg.data = 0xffff;
	write_indirect(card, &reg, PTRREG, DATAREG);
	reg.index = PTAB | v;
	read_indirect(card, &reg, PTRREG, DATAREG);
	reg.data &= 0xffff;
	write_indirect(card, &reg, PTRREG, DATAREG);
  }

  /* Note: The comments in _stopStereoVoice() suggest that the
	 following is not good enough. */

  for (v = 0; v < 64; v++) {
	reg.data = 0;
	reg.index = CCR | v;
	write_indirect(card, &reg, PTRREG, DATAREG);
	reg.index = CPF | v;
	write_indirect(card, &reg, PTRREG, DATAREG);
  }

  /* disable interrupts */
  reg.data = 0;
  reg.bytes = 4;
  write_io_n(&reg, AUDIO_BASE(card) + INTE);
}
