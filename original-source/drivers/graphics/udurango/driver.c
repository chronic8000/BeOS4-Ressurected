#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>

#include <os_p/drivers_priv.h>
#include <drivers/genpool_module.h>

#include <graphic_driver.h>
/* this is for sprintf() */
#include <stdio.h>
/* this is for string compares */
#include <string.h>

/* many things are included via this file */
#include "durango_private.h"
#include "inc210/gfx_regs.h"

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#if DEBUG > 2
#define xdprintf(a) dprintf a
#else
#define xdprintf(a)
#endif
#else
#define	ddprintf(a)
#define xdprintf(a)
#endif

#define get_pci(o, s) (*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, di.pcii.function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, di.pcii.function, (o), (s), (v))
#if 0
#define inb(a, v) v = (*pci_bus->read_io_8)(a)
#define outb(a, v) (*pci_bus->write_io_8)(a, v)
#else
#define outb(addr, val)	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(val), "d"(addr))
#define inb(addr, val)	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(val) : "d"(addr));
#endif

#define READ_REG32(offset) \
	(*(volatile unsigned long *)(di.si->regs + (offset)))

#define READ_VID32(offset) \
	(*(volatile unsigned long *)(di.si->dac + (offset)))

#define WRITE_VID32(offset, value) \
	(*(volatile unsigned long *)(di.si->dac + (offset))) = (value)


/* for def_fs */
int32	api_version = B_CUR_DRIVER_API_VERSION;

typedef struct {
	int32
		is_open;
	area_id
		shared_area;
	shared_info
		*si;
	pci_info
		pcii;

#if GXLV_DO_VBLANK
	int32
		can_interrupt;
	thread_id
		tid;	/* only used if we're faking interrupts */
#if DEBUG > 0
	uint32
		interrupt_count;
#endif
#endif
} device_info;

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(void);
static void unmap_device(void);

#if GXLV_DO_VBLANK
static void define_cursor(shared_info *si, uchar *xorMask, uchar *andMask, uint16 width, uint16 height, uint16 hot_x, uint16 hot_y);
static int mpdump(int argc, char **argv);
#endif

static device_hooks graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

static device_info		di;
static benaphore		driver_lock;
static pci_module_info  *pci_bus;
static genpool_module	*ggpm = 0;
static BPoolInfo		*gfbpi = 0;

/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	status_t result;
	system_info si;
	result = get_system_info(&si);
	if(result != B_OK)
		goto err;
	if(si.cpu_type != B_CPU_CYRIX_GXm &&
	   si.cpu_type != B_CPU_NATIONAL_GEODE_GX1)
	   	result = B_ERROR;
	/* check for the 5530 here as well? */
err:
	ddprintf(("durango init_hardware() returns %.8lx\n", result));
	return result;
}

status_t
init_driver(void) {
	long pci_index = 0;
	int found = 0;

	di.is_open = 0;

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
	{
		ddprintf(("durango init_driver() can't get PCI module :-(\n"));
		return B_ERROR;
	}
	/* find the 5530 */
	while (!found && ((*pci_bus->get_nth_pci_info)(pci_index++, &(di.pcii)) == B_NO_ERROR))
	{
		ddprintf(("GKD: checking pci index %ld, device 0x%04x_0x%04x_%.02x%.02x%.02x\n", pci_index, di.pcii.vendor_id, di.pcii.device_id,
			di.pcii.bus, di.pcii.device, di.pcii.function));
		if ((di.pcii.vendor_id == 0x1078) && (di.pcii.device_id == 0x0104) && (di.pcii.function == 0x04))
		{
			found = 1;
			break;
		}
		if((di.pcii.vendor_id == 0x100b) && (di.pcii.device_id == 0x0504) && (di.pcii.function == 0x04))
		{
			found = 1;
			break;
		}
	}
	if (!found)
	{
		put_module(B_PCI_MODULE_NAME);
		ddprintf(("durango init_driver() unable to find CS5530 device\n"));
		return B_ERROR;
	}
	/*
	 * Open the genpool module.
	 */
	if (get_module (B_GENPOOL_MODULE_NAME, (module_info **) &ggpm) != B_OK)
	{
		put_module(B_PCI_MODULE_NAME);
		ddprintf(("durango init_driver() couldn't find B_GENPOOL_MODULE\n"));
		return B_ERROR;
	}

	INIT_BEN(driver_lock);
	ddprintf(("durango init_driver() wins\n"));
	return B_OK;
}

void uninit_driver(void) {
	ddprintf(("durango uninit_driver()\n"));
	put_module(B_GENPOOL_MODULE_NAME);
	put_module(B_PCI_MODULE_NAME);
	DELETE_BEN(driver_lock);
}

static const char *device_names[2] = {
	"graphics/durango",
	NULL
};

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return device_names;
}

device_hooks *
find_device(const char *name) {
	if (strcmp(name, device_names[0]) == 0)
		return &graphics_device_hooks;
	return NULL;
}

#if CH7005_SUPPORT
static uint16 i2c_io_addr = 0x00e0; //0x0800;
static uint16 i2c_dir_addr = 0x00e1; //0x0801;
static bigtime_t i2c_timebase = 0LL;
static bigtime_t i2c_snoozed = 0LL;

#define I2C_CLOCK_IO_ADDR	i2c_io_addr
#define I2C_CLOCK_DIR_ADDR	i2c_dir_addr
#define I2C_CLOCK_BIT		0x02
#define I2C_CLOCK_DIR_BIT	0x02

#define I2C_DATA_IO_ADDR	I2C_CLOCK_IO_ADDR
#define I2C_DATA_DIR_ADDR	I2C_CLOCK_DIR_ADDR
#define I2C_DATA_BIT		0x01
#define I2C_DATA_DIR_BIT	0x01
#define I2C_DATA_SHIFT		0

#define I2C_400KBPS_SNOOZE	((1000000 / 400000) / 2)	/* clocking out is an up/down transition, so /2 */

static void set_clock_dir_i2c(int state)
{
	uint8 data;

	/* get current state */
	inb(I2C_CLOCK_DIR_ADDR, data);

	if (state)
	{
		/* set data direction to output via read-modify-write */
		outb(I2C_CLOCK_DIR_ADDR, data | I2C_CLOCK_DIR_BIT);
	}
	else
	{
		/* set data direction to input via read-modify-write */
		outb(I2C_CLOCK_DIR_ADDR, data & ~I2C_CLOCK_DIR_BIT);
	}	
}


static void set_clock_i2c(int state)
{
	uint8 clock;
	bigtime_t t = i2c_timebase + I2C_400KBPS_SNOOZE;
	/* don't exceed the 400kbps data rate */
	while (system_time() < t)
		i2c_snoozed++/* spin */;

	/* change clock via read-modify-write */
	inb(I2C_CLOCK_IO_ADDR, clock);
	if (state) clock |= I2C_CLOCK_BIT;
	else clock &= ~I2C_CLOCK_BIT;
	outb(I2C_CLOCK_IO_ADDR, clock);
	
	/* new timebase */
	i2c_timebase = system_time();
}

static void set_data_dir_i2c(int state)
{
	uint8 data;

	/* get current state */
	inb(I2C_DATA_DIR_ADDR, data);

	if (state)
	{
		/* set data direction to output via read-modify-write */
		outb(I2C_DATA_DIR_ADDR, data | I2C_DATA_DIR_BIT);
	}
	else
	{
		/* set data direction to input via read-modify-write */
		outb(I2C_DATA_DIR_ADDR, data & ~I2C_DATA_DIR_BIT);
	}	
}

static void set_data_i2c(int state)
{
	uint8 data;

	/* change data via read-modify-write */
	inb(I2C_DATA_IO_ADDR, data);
	if (state) data |= I2C_DATA_BIT;
	else data &= ~I2C_DATA_BIT;
	outb(I2C_DATA_IO_ADDR, data);
	
	//xdprintf((" set_data_i2c(%d)\n", state));
}

static int get_data_i2c(void)
{
	uint8 data;

	/* read data */
	inb(I2C_DATA_IO_ADDR, data);
	/* clear all but data bit */
	data &= I2C_DATA_BIT;
	/* return data bit in low order bit */
	return (data >> I2C_DATA_SHIFT);
}

static int outb_i2c(uint8 data)
{
	int i;
	
	/* start out with a low clock setting */
	set_clock_i2c(0);

	/* set data direction bit for output */
	set_data_dir_i2c(1);

	//xdprintf(("outb_i2c() start of byte\n"));
	
	/* clock out one byte */
	for (i = 0; i < 8; i++)
	{
		/* set the data value */
		set_data_i2c((data & 0x80) >> 7);
		/* shift in the next bit to send */
		data <<= 1;
		/* transition the clock to send the bit */
		set_clock_i2c(1);
		set_clock_i2c(0);
	}
	//xdprintf(("outb_i2c() end of byte\n"));
	
	/* prep ack bit */
	set_data_i2c(1);
	/* set data direction bit for input */
	set_data_dir_i2c(0);
	/* transition clock */
	set_clock_i2c(1);
	/* get ack bit */
	i = get_data_i2c();
	/* transition clock to complete ack */
	set_clock_i2c(0);
	xdprintf(("outb_i2c() - ack: %d\n", i));
	return i;
}

static uint8 inb_i2c(void)
{
	/* read a byte from the i2c bus */
	uint8 data = 0;
	int i;

	/* set data direction bit for input */
	set_data_dir_i2c(0);

	/* clock in one byte */
	for (i = 0; i < 8; i++)
	{
		/* transition the clock to get the next bit */
		set_clock_i2c(1);
		snooze(I2C_400KBPS_SNOOZE);
		/* shift up to place next bit */
		data <<= 1;
		/* get the data bit */
		data |= get_data_i2c() & 0x01;
		/* clock low to end cycle */
		set_clock_i2c(0);
	}
	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* transition clock */
	set_clock_i2c(1);
	/* clock out ack bit */
	set_data_i2c(0);
	/* clock low to end cycle */
	set_clock_i2c(0);
	
	/* return the goods */
	return data;
}

static void write_ch7005(uint8 index, uint8 data)
{
	int count = 10;
restart:
	if (count-- == 0) return;

	/* set clock direction bit to output */
	set_clock_dir_i2c(1);

	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* set start condition */
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	set_data_i2c(0);
	set_clock_i2c(0);

	xdprintf(("select ch7005 for write\n"));
	/* select chip: ch7005 is 0x75, R/W == 0 for write */
	/* shift left 1 for R/W bit */
	if (outb_i2c((0x75 << 1) | 0x00)) goto restart;
	xdprintf(("write index %.2x\n", (int) index));
	/* write the register index */
	if (outb_i2c(index)) goto restart;
	xdprintf(("write data %.2x\n", (int) data));
	/* write the register data */
	if (outb_i2c(data)) goto restart;

	/* set stop condition */
	set_data_i2c(0);
	set_clock_i2c(0);
	set_clock_i2c(1);
	set_data_i2c(1);
}

static uint8 read_ch7005(uint8 index)
{
	uint8 data;
	int count = 10;
restart:
	if (count-- == 0) return 0xff;


	/* set clock direction bit to output */
	set_clock_dir_i2c(1);

	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* set start condition */
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	set_data_i2c(0);
	set_clock_i2c(0);

	xdprintf(("writing CH7005 id\n"));
	/* select chip: ch7005 is 0x75, R/W == 0 for write */
	/* shift left 1 for R/W bit, shift left another 1 for ACK bit */
	if (outb_i2c((0x75 << 1) | 0x00)) goto restart;
	/* ensure that the auto-increment bit is not set in index */
	index &= ~0x40;
	xdprintf(("writing register index %.2x\n", (int)index));
	/* write the register index */
	if (outb_i2c(index)) goto restart;

	xdprintf(("restart connection\n"));
	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* reset start condition for change in data direction */
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	set_data_i2c(0);
	set_clock_i2c(0);

	xdprintf(("select CH7005 for read\n"));
	/* select chip: ch7005 is 0x75, R/W == 1 for read */
	if (outb_i2c((0x75 << 1) | 0x01)) goto restart;
	/* get the data byte */
	xdprintf(("reading byte...\n"));
	data = inb_i2c();
	xdprintf(("set stop condition\n"));
	/* set stop condition */
	set_data_i2c(0);
	set_clock_i2c(0);
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	
	/* return the goods */
	return data;
}

#if 0
/*
	This code only reads (at most) 0x0b sequential indexes.
	I don't know why.
	Perhaps it's a bug/feature of the CH7005?
*/

uint8 * read_many_ch7005(uint8 index, uint8 *data, uint8 count)
{
	/* set clock direction bit to output */
	set_clock_dir_i2c(1);

	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* set start condition */
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	set_data_i2c(0);
	set_clock_i2c(0);

	xdprintf(("writing CH7005 id\n"));
	/* select chip: ch7005 is 0x75, R/W == 0 for write */
	/* shift left 1 for R/W bit, shift left another 1 for ACK bit */
	outb_i2c((0x75 << 1) | 0x00);
	xdprintf(("writing register index %.2x\n", (int)index));
	/* write the register index */
	/* ensure that the auto-increment bit is set in index */
	outb_i2c(index | 0x40);

	xdprintf(("restart connection\n"));
	/* set data direction bit for output */
	set_data_dir_i2c(1);
	/* reset start condition for change in data direction */
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	set_data_i2c(0);
	set_clock_i2c(0);

	xdprintf(("select CH7005 for read\n"));
	/* select chip: ch7005 is 0x75, R/W == 1 for read */
	outb_i2c((0x75 << 1) | 0x01);
	/* get the data byte */
	ddprintf(("reading bytes...\n"));
	while (count--)
	{
		*data++ = inb_i2c();
		ddprintf(("read_many(0x%.2x) = 0x%.2x\n", index++, *(data - 1)));
	}
	xdprintf(("set stop condition\n"));
	/* set stop condition */
	set_data_i2c(0);
	set_clock_i2c(0);
	set_clock_i2c(1);
	set_data_i2c(1);
	snooze(I2C_400KBPS_SNOOZE);
	
	/* return the goods */
	return data;
}
#endif
#endif

static status_t map_device() {
	shared_info *si = di.si;
	uint8 CCR3, GCR;
	cpu_status	cs;
	uint32 gx_base, cx5530_base, cx5530_size;

#if CH7005_SUPPORT
	{
		uint8 val;
		uint16 ioaddr;
		
		outb(0x02E, 0x06);
		inb(0x02F, val);
		ddprintf(("CSN: %.2x\n", val));
		outb(0x02E, 0x07);
		inb(0x02F, val);
		ddprintf(("LDN: %.2x\n", val));
		outb(0x02E, 0x20);
		inb(0x02F, val);
		ddprintf(("SRID: %.2x\n", val));
		outb(0x02E, 0x21);
		inb(0x02F, val);
		ddprintf(("Config 1: %.2x\n", val));
		outb(0x02E, 0x22);
		inb(0x02F, val);
		ddprintf(("Config 2: %.2x\n", val));
		outb(0x02E, 0x27);
		inb(0x02F, val);
		ddprintf(("ID reg: %.2x\n", val));
		outb(0x02E, 0x07);
		outb(0x02F, 0x07);	// logical device 7 (GPIO)
		inb(0x02F, val);
		ddprintf(("LDN: %.2x\n", val));
		outb(0x02E, 0x30);
		inb(0x02F, val);
		ddprintf(("Activate: %.2x\n", val));
		outb(0x02E, 0x60);
		inb(0x02F, val);
		ioaddr = val;
		ioaddr <<= 8;
		ddprintf(("IO addr MSB: %.2x\n", val));
		outb(0x02E, 0x61);
		inb(0x02F, val);
		ioaddr |= val;
		ddprintf(("IO addr LSB: %.2x\n", val));
		inb(ioaddr+1, val);
		ddprintf(("direction reg: %.2x\n", val));
#if 0
		inb(ioaddr, val);
		ddprintf(("direction reg: %.2x\n", val));
#endif
		i2c_io_addr = ioaddr++;
		i2c_dir_addr = ioaddr;
		ddprintf(("i2c_io_addr:  0x%04x\ni2c_dir_addr: 0x%04x\n", i2c_io_addr, i2c_dir_addr));
	}
#endif

#if 0
	{
#if 0
	uint8 result;
	for (i2c_io_addr = 0x800; i2c_io_addr < 0x801; i2c_io_addr += 0x02)
	{
		i2c_dir_addr = i2c_io_addr + 1;
		/* probe for chrontel chip */
		result = read_ch7005(0x25);
		if (result != 0xff) ddprintf(("i2c_io_addr: %.4x : %.2x\n", i2c_io_addr, result));
		//if (result == 0x3a)
		{
			ddprintf(("Read CH7005 index 0x25.  Expecting 0x3a, got 0x%.2x\n", result));
			ddprintf(("Writing CH07005 index 0x04 to zero...\n"));
			write_ch7005(0x04, 0x00);
			result = read_ch7005(0x04);
			ddprintf(("Read CH7005 index 0x04.  Expecting 0x00, got 0x%.2x\n", result));
			if (result == 0x00) break;
		}
#if 0
			break;
		ddprintf(("Read CH7005 index 0x3f.  Expecting 0x25, got 0x%.2x\n", read_ch7005(0x3f)));
#endif
	}
	if ((i2c_io_addr < 0x1000) && (result == 0x00))
#endif
#if 0
	{
		uint8 data[255];
		uint8 index;
		ddprintf(("HOT DAMN!\n"));
		//read_many_ch7005(0x17, data, 0x12);
		for (index = 0x0; index < 0x2A; index++)
		{
			switch (index)
			{
			case 0x02:
			case 0x05:
			case 0x0c:
			case 0x0f:
			case 0x12:
			case 0x16:
				break;
			default:
				ddprintf(("read_one(0x%.2x) = 0x%.2x\n", index, read_ch7005(index)));
				break;
			}
		}
	}
	ddprintf(("i2c_snoozed: %Lu\n", i2c_snoozed));
#endif
	{
		static const uint8 data[0x22] = {
		0x8D, // 0x00 800x600 NTSC 7/10
		0x38, // 0x01
		0x00, // 0x02 NOT USED
		0xB5, // 0x03 Video Bandwidth
		0x00, // 0x04 input data format
		0x00, // 0x05 NOT USED
		0x10, // 0x06 Clock mode
		0xB8, // 0x07 Start Active Video
		0x00, // 0x08 Position overflow
		0x7F, // 0x09 Black level (7F is neutral)
		0x3D, // 0x0A Horiz. pos.
		0x0F, // 0x0B Verti. pos.
		0x00, // 0x0C NOT USED
		0x00, // 0x0D sync polarity
		0x0B, // 0x0E Power managment (output select) 
		0x00, // 0x0F NOT USED
		0x00, // 0x10 Connection detect
		0x03, // 0x11 Contrast enhancement (0x03 is neutral)
		0x00, // 0x12 NOT USED
		0x00, // 0x13 PLL Overflow (02)
		0x00, // 0x14 PLL M (59)
		0x01, // 0x15 PLL N (2e)
		0x00, // 0x16 NOT USED
		0x00, // 0x17 Buffered clock output (14 MHz)
		0x01, // 0x18 SubCarrier values (not used in slave mode, but...)
		0x0A, // 0x19 for mode 24, there are two values:
		0x02, // 0x1A normal dot crawl: 428,554,851
		0x03, // 0x1B no dot crawl    : 438,556,645
		0x1D, // 0x1C (also, buffered clock out enable)
		0x07, // 0x1D
		0x0E, // 0x1E
		0x05, // 0x1F
		0x0A, // 0x20 PLL control
		0x01 // 0x21 CIV control (automatic)
		};
		static const uint8 mask[0x22] = {
		0x00, // 0x00 800x600 NTSC 7/10
		0x00, // 0x01
		0xFF, // 0x02 NOT USED
		0x00, // 0x03 Video Bandwidth
		0x90, // 0x04 input data format
		0xFF, // 0x05 NOT USED
		0x20, // 0x06 Clock mode
		0x00, // 0x07 Start Active Video
		0xF8, // 0x08 Position overflow
		0x00, // 0x09 Black level (7F is neutral)
		0x00, // 0x0A Horiz. pos.
		0x00, // 0x0B Verti. pos.
		0xFF, // 0x0C NOT USED
		0xF0, // 0x0D sync polarity
		0xE0, // 0x0E Power managment (output select) 
		0xFF, // 0x0F NOT USED
		0xF0, // 0x10 Connection detect
		0xF8, // 0x11 Contrast enhancement (0x03 is neutral)
		0xFF, // 0x12 NOT USED
		0xE0, // 0x13 PLL Overflow
		0x00, // 0x14 PLL M
		0x00, // 0x15 PLL N
		0xFF, // 0x16 NOT USED
		0xC0, // 0x17 Buffered clock output (14 MHz)
		0xF0, // 0x18 SubCarrier values (not used in slave mode, but...)
		0xF0, // 0x19 for mode 24, there are two values:
		0xF0, // 0x1A normal dot crawl: 428,554,851
		0xE0, // 0x1B no dot crawl    : 438,556,645
		0xE0, // 0x1C (also, buffered clock out enable)
		0xF0, // 0x1D
		0xF0, // 0x1E
		0xF0, // 0x1F
		0xC0, // 0x20 PLL control
		0xE0  // 0x21 CIV control (automatic)
		};
		uint8 index;

		// reset the CH7005C
		write_ch7005(0x0E, 0x00);
		snooze(10000);
		write_ch7005(0x0E, 0x08);
		for (index = 0x0; index < 0x22; index++)
		{
			if (mask[index] != 0xff)
			{
				uint8 abyte = read_ch7005(index);
				ddprintf(("read_one(0x%.2x) = 0x%.2x", index, abyte));
				abyte &= mask[index];
				abyte |= data[index];
				ddprintf((" data 0x%.2x", data[index]));
				write_ch7005(index, abyte);
				ddprintf((" wrote 0x%.2x, got back 0x%.2x\n", abyte, read_ch7005(index)));
			}
		}
	}
	}
#endif

#define GX_CCR_INDEX	0x22
#define GX_CCR_DATA		0x23
#define GX_CCR3			0xC3
#define GX_CCR3_MAPEN	0x10
#define GX_GCR			0xB8
#define GX_GCR_BASE		0x03
#define GX_GCR_SCRATCH	0x0C
#define GX_VGACTL		0xB9
#define GX_VGAM_0		0xBA
#define GX_VGAM_1		0xBB
#define GX_VGAM_2		0xBC
#define GX_VGAM_3		0xBD

#if 1
	/* turn of SoftVGA's text mode: from durango source */
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0), "d"(0x3c4));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0), "d"(0x3c5));
#endif

	ddprintf(("grabbing GCR\n"));
	/* disable interrupts */
	cs = disable_interrupts();

	/* save the CCR3 reg */
	//outb(GX_CCR_INDEX, GX_CCR3);
	//CCR3 = inb(GX_CCR_DATA);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(CCR3) : "d"(GX_CCR_DATA));

	/* set MAPEN in CCR3 */
	//outb(GX_CCR_INDEX, GX_CCR3);
	// outb(GX_CCR_DATA, CCR3 | GX_CCR3_MAPEN);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(CCR3 | GX_CCR3_MAPEN), "d"(GX_CCR_DATA));

#if 0
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(ccr3_after) : "d"(GX_CCR_DATA));
#endif

	/* find GX_BASE and scratch pad info */
	//outb(GX_CCR_INDEX, GX_GCR);
	//GCR = inb(GX_CCR_DATA);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_GCR), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(GCR) : "d"(GX_CCR_DATA));

#if 0
	/* move the graphics stuff to 0x8000000 */
	GCR = (GCR & ~GX_GCR_BASE) | 0x03;
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_GCR), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GCR), "d"(GX_CCR_DATA));
#endif

#if 0
	if ((GCR & 0x03) == 0)
	{
		/* uh, it's off.  make a wild guess */
		GCR |= 0x01;
		__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_GCR_BASE), "d"(GX_CCR_INDEX));
		__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GCR), "d"(GX_CCR_DATA));
	}
#endif

#if 0
	/* turn off SMI for VGA */
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_VGACTL), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0x00),      "d"(GX_CCR_DATA));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_VGAM_0), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0x00),      "d"(GX_CCR_DATA));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_VGAM_1), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0x00),      "d"(GX_CCR_DATA));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_VGAM_2), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0x00),      "d"(GX_CCR_DATA));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_VGAM_3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(0x00),      "d"(GX_CCR_DATA));
#endif

	/* restore MAPEN setting */
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(CCR3),    "d"(GX_CCR_DATA));

	/* restore interrupt state */
	restore_interrupts(cs);
	ddprintf(("orig CCR3: %.2x\n", CCR3));
	ddprintf(("got GCR: %.2x\n", GCR));
	
	/* someone disabled the device, and we're not going to change it */
	if ((GCR & 0x03) == 0)
	{
		ddprintf(("Yikes! gxm graphics disabled!\n"));
		return B_ERROR;
	}
	si->GCR = GCR;
	gx_base = 0x40000000 * (GCR & GX_GCR_BASE);
	ddprintf(("mapping regs at %.8lx\n", gx_base));
	/* map the areas */
	si->regs_area = map_physical_memory(
		"gxlv regs",
		(void *) gx_base,
		0x10000,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void*)&(si->regs));
	/* return the error if there was some problem */
	if (si->regs_area < 0)
	{
		ddprintf(("durango map_device() couldn't map %.8lx as gxlv regs\n", gx_base));
		return si->regs_area;
	}

	si->fb_area = map_physical_memory(
		"gxlv framebuffer",
		(void *) (gx_base + 0x00800000),
#if 0
		0x8000000,				// FFB: FIXME - get real size
		// 0x200000,
#else
		4 * 1024 * 1024,
#endif
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		&(si->framebuffer));
	/* bail on error */
	if (si->fb_area < 0)
	{
		ddprintf(("durango map_device() couldn't map %.8lx as gxlv framebuffer\n", gx_base + 0x00800000));
		goto delete_regs;
	}
	si->framebuffer_dma = (void *)(gx_base + 0x00800000);
	ddprintf(("framebuffer mapped at %p, dma %p\n", si->framebuffer, si->framebuffer_dma));

#if 0
	/* NO SMIs */
	(*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x81, 1,
		(*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x81, 1) & ~0x80);
	(*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x82, 1,
		(*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x82, 1) & ~0x80);
	(*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x83, 1,
		(*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x83, 1) & ~0x0C);
#endif

	/* ensure that we can read the CS5530 regs without generating an SMI */
	ddprintf(("PCI function configuration register 3 before: 0x%02lx\n", (*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1)));
	(*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1,
		(*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1) & ~0x02);
	ddprintf(("PCI function configuration register 3 after: 0x%02lx\n", (*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1)));
	//ddprintf(("bus/def/func: %.02x/%.02x/%.02x\n", di.pcii.bus, di.pcii.device, di.pcii.function));
	/* find out where the BIOS mapped it */
	cx5530_base = get_pci(PCI_base_registers, 4) & ~0x0fff;
	ddprintf(("read cx5530_base of %.8lx\n", cx5530_base));
	/* determine size */
	set_pci(PCI_base_registers, 4, 0xffffffff);
	cx5530_size = ~(get_pci(PCI_base_registers, 4) & ~0x0fff) + 1;
	ddprintf(("cx5530_size is %.8lx\n", cx5530_size));
	/* restore address */
	set_pci(PCI_base_registers, 4, cx5530_base);
	ddprintf(("set PCI_base_registers to %.8lx\n", cx5530_base));

	/* enable access */
	set_pci(PCI_command, sizeof(uint16), PCI_command_memory); //(get_pci(PCI_command, sizeof(uint16)) | PCI_command_memory) & PCI_command_memory);

#if 1
	/* give SoftVGA the registers again */
	(*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1,
		(*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, 0, 0x42, 1) | 0x02);
#endif

	ddprintf(("mapping CS5530 at %.8lx with size %.8lx\n", cx5530_base, cx5530_size));
	si->dac_area = map_physical_memory(
		"CS5530 graphics",
		(void *) cx5530_base,
		((cx5530_size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1)),
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void*)&(si->dac));

	/* bail on error */
	if (si->dac_area < 0)
	{
		ddprintf(("durango map_device() couldn't map %.8lx as CS5530 graphics\n", cx5530_base));
		goto delete_fb;
	}

	ddprintf(("mapped CS5530 graphics at %p\n", si->dac));

#if 0
//#if CH7005_SUPPORT
	/* make sure the flat-panel is enabled */
	{
		uint32 t = READ_VID32(CS5530_DISPLAY_CONFIG);
		if (!(t & CS5530_DCFG_FP_ON))
		{
			// transition power up bits
			// data and power off
			t &= ~(CS5530_DCFG_FP_DATA_EN | CS5530_DCFG_FP_PWR_EN);
			WRITE_VID32(CS5530_DISPLAY_CONFIG, t);
			snooze(1000);
			// power on
			t |= CS5530_DCFG_FP_PWR_EN;
			WRITE_VID32(CS5530_DISPLAY_CONFIG, t);
		}
	}
#endif
	/* all is well */
	return B_OK;

delete_fb:
	delete_area(si->fb_area);
	si->fb_area = -1;

delete_regs:
	delete_area(si->regs_area);
	si->regs_area = -1;
	/* in any case, return the result */
	return B_ERROR;
}

static void unmap_device() {
	shared_info *si = di.si;

	ddprintf(("unmap_device() begins...\n"));
	ddprintf(("  regs_area: %ld\n  fb_area: %ld\n", si->regs_area, si->fb_area));

	if (si->regs_area >= 0) delete_area(si->regs_area);
	if (si->fb_area >= 0) delete_area(si->fb_area);
	if (si->dac_area >= 0) delete_area(si->dac_area);
	si->regs_area = si->fb_area = si->dac_area = -1;
	si->framebuffer = NULL;
	si->framebuffer_dma = NULL;
	si->regs = NULL;
	si->dac = NULL;
	ddprintf(("unmap_device() ends.\n"));
}

static void CalcMemSize(void)
{
#define BC_DRAM_TOP 0x8000
	uint32 gfx_top;
	uint32 dram_top = READ_REG32(BC_DRAM_TOP);
	ddprintf(("BC_DRAM_TOP: %.8lx\n", dram_top));
	dram_top >>= 17;
	dram_top &= 0x7ff;
	dram_top++;
	dram_top *= 128 * 1024;
	gfx_top = dram_top + 128 * 1024;
	while (gfx_top & ((4 * 1024 * 1024) - 1))
		gfx_top += 128 * 1024;
	ddprintf(("gfx_top = %lx\n", gfx_top));
	di.si->mem_size = gfx_top - dram_top;
	ddprintf(("mem_size: %ld, %.8lx\n", di.si->mem_size, di.si->mem_size));
}

static status_t InitGenPool(void)
{
	status_t result = B_NO_MEMORY;
	if ((gfbpi = (ggpm->gpm_AllocPoolInfo)()))
	{
		gfbpi->pi_Pool_AID	= di.si->fb_area;
		gfbpi->pi_Pool		= di.si->framebuffer_dma;
		gfbpi->pi_Size		= di.si->mem_size;
		strcpy (gfbpi->pi_Name, "Geode Framebuffer");
		if ((result = (ggpm->gpm_CreatePool)(gfbpi, 8, 0)) < B_OK)
		{
			(ggpm->gpm_FreePoolInfo)(gfbpi);
			gfbpi = NULL;
		}
	}
	return result;
}

static void UninitGenPool(void)
{
	(ggpm->gpm_DeletePool)(gfbpi->pi_PoolID);
	(ggpm->gpm_FreePoolInfo)(gfbpi);
	gfbpi = NULL;
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	shared_info *si;
#if GXLV_DO_VBLANK
	thread_id	thid;
	thread_info	thinfo;
#endif
	status_t	result = B_OK;
	(void)flags;

	ddprintf(("DURANGO open_hook(%s, %ld, 0x%08lx)\n", name, flags, (uint32)cookie));

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	if (strcmp(name, device_names[0]) != 0)
		return B_ERROR;

	ACQUIRE_BEN(driver_lock);
	if (di.is_open) goto mark_as_open;

	/* create the shared area */
	ddprintf(("shared area size: %ld, %.8lx\n", 
		(sizeof(shared_info)+sizeof(accelerant_info)+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1),
		(sizeof(shared_info)+sizeof(accelerant_info)+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1)));
	di.shared_area = create_area("durango shared", (void**)&di.si, B_ANY_KERNEL_ADDRESS,
		(sizeof(shared_info)+sizeof(accelerant_info)+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1),
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (di.shared_area < 0) {
		/* return the error */
		result = di.shared_area;
		ddprintf(("failed to create shared area: %.8lx, %s\n", result, strerror(result)));
		goto done;
	}

	/* save a few dereferences */
	si = di.si;

	/* map the device */
	result = map_device();
	if (result < 0) goto free_shared;
	result = B_OK;

	CalcMemSize();
	if (InitGenPool() != B_OK) goto unmap;

#if GXLV_DO_VBLANK
	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, name);
	if (si->vblank < 0) {
		result = si->vblank;
		goto unmap;
	}

	/* change the owner of the semaphores to the opener's team */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);
#endif

mark_as_open:
	/* mark the device open */
	di.is_open++;

	/* send the cookie to the opener */
	*cookie = &di;
	
	goto done;

#if GXLV_DO_VBLANK
delete_the_sem:
	delete_sem(si->vblank);
#endif

unmap:
	unmap_device();

free_shared:
	/* clean up our shared area */
	delete_area(di.shared_area);
	di.shared_area = -1;
	di.si = NULL;

done:

	RELEASE_BEN(driver_lock);

	/* all done, return the status */
	ddprintf(("open_hook returning 0x%08lx\n", result));
	return result;
}

/* ----------
	read_hook - does nothing, gracefully
----- */
static status_t
read_hook (void* dev, off_t pos, void* buf, size_t* len)
{
	(void)dev; (void)pos; (void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}


/* ----------
	write_hook - does nothing, gracefully
----- */
static status_t
write_hook (void* dev, off_t pos, const void* buf, size_t* len)
{
	(void)dev; (void)pos; (void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}

/* ----------
	close_hook - does nothing, gracefully
----- */
static status_t
close_hook (void* dev)
{
	(void)dev;
	ddprintf(("MKD close_hook(%08lx)\n", (uint32)dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
#if GXLV_DO_VBLANK
	shared_info	*si = di.si;
	//vuchar *regs = si->regs;
#endif
	(void)dev;

	ddprintf(("DURANGO free_hook() begins...\n"));

	ACQUIRE_BEN(driver_lock);

	/* not the last one? bail */
	if (--di.is_open) goto unlock;

#if GXLV_DO_VBLANK
	/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
	delete_sem(si->vblank);
	si->vblank = -1;
#endif

	UninitGenPool();
#if 1
	/* free regs and framebuffer areas */
	unmap_device();
#else
	ddprintf(("abandoned the regs and frame buffer areas\n"));
#endif

#if 1
	ddprintf(("1 second snooze()\n"));
	snooze(1 * 1000000);
	ddprintf(("finished snooze()\n"));
#endif

#if 1
	/* clean up our shared area */
	delete_area(di.shared_area);
	di.shared_area = -1;
	di.si = NULL;
	ddprintf(("shared area deleted\n"));
#else
	ddprintf(("abandoned the shared area\n"));
#endif

unlock:
	/* unlock */
	RELEASE_BEN(driver_lock);

	ddprintf(("DURANGO free_hook() ends.\n"));
	/* all done */
	return B_OK;
}

status_t set_power_save(uint32 state)
{
	uint32 temp32;

	ACQUIRE_BEN(driver_lock);
	/* non-zero == power-save mode */
	if (state)
	{
		// DMPS off state, plus power down DAC
		temp32 = READ_VID32(CS5530_DISPLAY_CONFIG);
		temp32 &= ~(CS5530_DCFG_HSYNC_EN | CS5530_DCFG_VSYNC_EN | CS5530_DCFG_DAC_BL_EN | CS5530_DCFG_DAC_PWDNX);
		WRITE_VID32(CS5530_DISPLAY_CONFIG, temp32);
		// power down PLL
		temp32 = READ_VID32(CS5530_DOT_CLK_CONFIG);
		temp32 |= (1 << 8);
		WRITE_VID32(CS5530_DOT_CLK_CONFIG, temp32);

#if CH7005_SUPPORT
		/* power managment: 0x08 == don't reset, 0x01 == power down */
		write_ch7005(0x0E, 0x09);
#endif
	}
	/* else normal mode */
	else
	{
		// power up PLL
		temp32 = READ_VID32(CS5530_DOT_CLK_CONFIG);
		temp32 &= ~(1 << 8);
		WRITE_VID32(CS5530_DOT_CLK_CONFIG, temp32);
		// DPMS on state, plus power up DAC
		temp32 = READ_VID32(CS5530_DISPLAY_CONFIG);
		temp32 |= (CS5530_DCFG_HSYNC_EN | CS5530_DCFG_VSYNC_EN | CS5530_DCFG_DAC_BL_EN | CS5530_DCFG_DAC_PWDNX);
		WRITE_VID32(CS5530_DISPLAY_CONFIG, temp32);
#if CH7005_SUPPORT
		/* power managment: 0x08 == don't reset, 0x03 == normal operation */
		write_ch7005(0x0E, 0x0B);
#endif
	}
	RELEASE_BEN(driver_lock);
	return B_OK;
}

#if CH7005_SUPPORT
static const uint8 ch7005_reg_masks[0x3E] = {
	0x00, // 0x00 800x600 NTSC 7/10
	0x00, // 0x01
	0xFF, // 0x02 NOT USED
	0x00, // 0x03 Video Bandwidth
	0x90, // 0x04 input data format
	0xFF, // 0x05 NOT USED
	0x20, // 0x06 Clock mode
	0x00, // 0x07 Start Active Video
	0xF8, // 0x08 Position overflow
	0x00, // 0x09 Black level (7F is neutral)
	0x00, // 0x0A Horiz. pos.
	0x00, // 0x0B Verti. pos.
	0xFF, // 0x0C NOT USED
	0xF0, // 0x0D sync polarity
	0xE0, // 0x0E Power managment (output select) 
	0xFF, // 0x0F NOT USED
	0x00, // 0x10 Connection detect
	0xF8, // 0x11 Contrast enhancement (0x03 is neutral)
	0xFF, // 0x12 NOT USED
	0xE0, // 0x13 PLL Overflow
	0x00, // 0x14 PLL M
	0x00, // 0x15 PLL N
	0xFF, // 0x16 NOT USED
	0xC0, // 0x17 Buffered clock output (14 MHz)
	0xF0, // 0x18 SubCarrier values (not used in slave mode, but...)
	0xF0, // 0x19 for mode 24, there are two values:
	0xF0, // 0x1A normal dot crawl: 428,554,851
	0xE0, // 0x1B no dot crawl    : 438,556,645
	0xE0, // 0x1C (also, buffered clock out enable)
	0xF0, // 0x1D
	0xF0, // 0x1E
	0xF0, // 0x1F
	0xC0, // 0x20 PLL control
	0xE0, // 0x21 CIV control (automatic)
	0xFF, // 0x22
	0xFF, // 0x23
	0xFF, // 0x24
	0xFF, // 0x25
	0xFF, // 0x26
	0xFF, // 0x27
	0xFF, // 0x28
	0xFF, // 0x29
	0xFF, // 0x2A
	0xFF, // 0x2B
	0xFF, // 0x2C
	0xFF, // 0x2D
	0xFF, // 0x2E
	0xFF, // 0x2F
	0xFF, // 0x30
	0xFF, // 0x31
	0xFF, // 0x32
	0xFF, // 0x33
	0xFF, // 0x34
	0xFF, // 0x35
	0xFF, // 0x36
	0xFF, // 0x37
	0xFF, // 0x38
	0xFF, // 0x39
	0xFF, // 0x3A
	0xFF, // 0x3B
	0xFF, // 0x3C
	0xF8  // 0x3D
};
#endif

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	shared_info	*si = di.si;
	status_t result = B_DEV_INVALID_IOCTL;
	/* avoid unused warnings */
	(void)len; (void)dev;

	/* ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len)); */
	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			/* strcpy(sig, "application/x-vnd.Be-webpad.accelerant"); */
			strcpy(sig, "durango.accelerant");
			result = B_OK;
		} break;
		
#if _SUPPORTS_FEATURE_POWER_SAVE
		case B_SET_POWERSAVE: {
			result = set_power_save(*(uint32 *) buf);
		} break;
#endif
	
		/* PRIVATE ioctl from here on */
		case DURANGO_GET_PRIVATE_DATA: {
			durango_get_private_data *gpd = (durango_get_private_data *)buf;
			if (gpd->magic == DURANGO_PRIVATE_DATA_MAGIC) {
				gpd->si = si;
				result = B_OK;
			}
		} break;
		case DURANGO_DEVICE_NAME: {
			durango_device_name *gdn = (durango_device_name *)buf;
			if (gdn->magic == DURANGO_PRIVATE_DATA_MAGIC) {
				strcpy(gdn->name, device_names[0]);
				result = B_OK;
			}
		} break;
#if CH7005_SUPPORT
		case DURANGO_WRITE_CH7005: {
			durango_ch7005_io *gci = (durango_ch7005_io *)buf;
			if (gci->magic == DURANGO_PRIVATE_DATA_MAGIC) {
				uint8 index = gci->index;
				uint8 count = gci->count;
				uint8 *data = gci->data;
				uint8 force = count & 0x80;
				count &= 0x7f;
				while (count--)
				{
					if (force || (ch7005_reg_masks[index] != 0xff))
						write_ch7005(index, *data);
					data++;
					index++;
				}
				result = B_OK;
			}
		} break;
		case DURANGO_READ_CH7005: {
			durango_ch7005_io *gci = (durango_ch7005_io *)buf;
			if (gci->magic == DURANGO_PRIVATE_DATA_MAGIC) {
				uint8 index = gci->index;
				uint8 count = gci->count;
				uint8 *data = gci->data;
				uint8 force = count & 0x80;
				count &= 0x7f;
				while (count--)
				{
					if (force || (ch7005_reg_masks[index] != 0xff))
						*data = read_ch7005(index);
					data++;
					index++;
				}
				result = B_OK;
			}
		} break;
#endif
		case DURANGO_LOCK: {
			durango_set_bool_state *sbs = (durango_set_bool_state *)buf;
			if (sbs->magic == DURANGO_PRIVATE_DATA_MAGIC) {
				if (sbs->do_it)
				{
					ACQUIRE_BEN(driver_lock);
				}
				else
				{
					RELEASE_BEN(driver_lock);
				}
				result = B_OK;
			}
		} break;
		case DURANGO_ALLOC : {
			durango_alloc_mem *am = (durango_alloc_mem *)buf;
			if (am->magic == DURANGO_PRIVATE_DATA_MAGIC)
			{
				am->ms->ms_PoolID = gfbpi->pi_PoolID;
				result = (ggpm->gpm_AllocByMemSpec)(am->ms);
				ddprintf(("result: %.08lx, allocated %.08lx bytes at %.08lx\n", result, am->ms->ms_MR.mr_Size, am->ms->ms_MR.mr_Offset));
			}
		} break;
		case DURANGO_FREE : {
			durango_alloc_mem *am = (durango_alloc_mem *)buf;
			if (am->magic == DURANGO_PRIVATE_DATA_MAGIC)
			{
				am->ms->ms_PoolID = gfbpi->pi_PoolID;
				result = (ggpm->gpm_FreeByMemSpec)(am->ms);
			}
		} break;
#if TWEAK_FIFO
#define READ_REG32(offset) \
	(*(volatile unsigned long *)(di.si->regs + (offset)))

#define WRITE_REG32(offset, value) \
	(*(volatile unsigned long *)(di.si->regs + (offset))) = (value)

#define DURANGO_TWEAK_FIFO (DURANGO_LOCK + 1000)

		case DURANGO_TWEAK_FIFO: {
			uint32 unlock = READ_REG32(DC_UNLOCK);
			uint32 start = ((uint32 *)buf)[0];
			uint32 end = ((uint32 *)buf)[1];
			uint32 gcfg;

			ddprintf(("DURANGO_TWEAK_FIFO: %ld, %ld\n", ((uint32 *)buf)[0], ((uint32 *)buf)[1]));

			WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
			gcfg = READ_REG32(DC_GENERAL_CFG);

			if (start > 14) start = 14;
			if (start == 0) start = 1;
			if (end <= start) end = start + 1;
			if (end > 15) end = 15;
			((uint32 *)buf)[0] = start;
			((uint32 *)buf)[1] = end;
			gcfg &= ~0x0000ff00;
			gcfg |= (start & 0xf) <<  8;
			gcfg |= (end   & 0xf) << 12;

			WRITE_REG32(DC_GENERAL_CFG, gcfg);
			WRITE_REG32(DC_UNLOCK, unlock);

			result = B_OK;
		} break;
#endif
	}
	return result;
}
