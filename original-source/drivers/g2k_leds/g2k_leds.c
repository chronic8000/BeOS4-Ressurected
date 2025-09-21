#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <scsi.h>
#include <malloc.h>
#include <ByteOrder.h>
#include <ISA.h>

#define POWER_LED_IO_ADDR 0xe0
#define POWER_LED_MASK 0x10

#define RTC_INDEX 0x70
#define RTC_DATA 0x71

#define RTC_CRA 0x0a
#define RTC_CRA_BANK2_VALUE 0x40

#define RTC_BANK2_APCR4 0x4a
#define RTC_BANK2_APCR4_LED_MASK 0xc0
#define RTC_BANK2_APCR4_LED_SHIFT 6

isa_module_info *isa;

status_t
init_driver()
{
	status_t err;
	dprintf("g2k_leds: init_driver\n");
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR)
		goto err1;
	return B_NO_ERROR;

//err2:
//	put_module(B_ISA_MODULE_NAME);
err1:
	dprintf("g2k_leds: init_driver failed\n");
	return err;
}

void
uninit_driver()
{
	dprintf("g2k_leds: uninit_driver\n");
	put_module(B_ISA_MODULE_NAME);
}

status_t
g2k_leds_open(const char *name, uint32 flags, void **cookie)
{
	return B_NO_ERROR;
}

status_t
g2k_leds_close(void *cookie)
{
	return B_NO_ERROR;
}

status_t
g2k_leds_free(void *cookie)
{
	return B_NO_ERROR;
}

status_t
g2k_leds_read(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	uint8 power, mail;
	cpu_status ps;
	if(pos != 0) {
		*numBytes = 0;
		return B_NO_ERROR;
	}
	if(*numBytes < 32) {
		*numBytes = 0;
		return B_ERROR;
	}
	power = !!(isa->read_io_8(POWER_LED_IO_ADDR) & POWER_LED_MASK);
	ps = disable_interrupts(); /* not safe on mp systems */
	isa->write_io_8(RTC_INDEX, RTC_CRA);
	isa->write_io_8(RTC_DATA, RTC_CRA_BANK2_VALUE);
	isa->write_io_8(RTC_INDEX, RTC_BANK2_APCR4);
	mail = (isa->read_io_8(RTC_DATA) & RTC_BANK2_APCR4_LED_MASK) >>
	       RTC_BANK2_APCR4_LED_SHIFT;
	restore_interrupts(ps);
	sprintf(data, "Power %d, Mail %d\n", power, mail);
	*numBytes = strlen(data);
	return B_NO_ERROR;
}

status_t
g2k_leds_write(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	const uint8 *str = data;
	if(pos == 0 && *numBytes == 3) {
		if(str[0] == 'p' && str[2] == '\n') {
			if(str[1] == '0') {
				isa->write_io_8(POWER_LED_IO_ADDR,
				                isa->read_io_8(POWER_LED_IO_ADDR) &
				                ~POWER_LED_MASK);
				return B_NO_ERROR;
			}
			else if(str[1] == '1') {
				isa->write_io_8(POWER_LED_IO_ADDR,
				                isa->read_io_8(POWER_LED_IO_ADDR) |
				                POWER_LED_MASK);
				return B_NO_ERROR;
			}
		}
		else if(str[0] =='m' && str[2] == '\n') {
			int mail = str[1] - '0';
			if(mail >= 0 && mail <= 2) {
				cpu_status ps;
				ps = disable_interrupts(); /* not safe on mp systems */
				isa->write_io_8(RTC_INDEX, RTC_CRA);
				isa->write_io_8(RTC_DATA, RTC_CRA_BANK2_VALUE);
				isa->write_io_8(RTC_INDEX, RTC_BANK2_APCR4);
				isa->write_io_8(RTC_DATA, isa->read_io_8(RTC_DATA) &
				                          ~RTC_BANK2_APCR4_LED_MASK |
				                          mail << RTC_BANK2_APCR4_LED_SHIFT);
				restore_interrupts(ps);
			}
		}
	}
	dprintf("g2k_leds: write failed\n");
	*numBytes = 0;
	return B_ERROR;
}

status_t
g2k_leds_control(void *cookie, uint32 op, void *data, size_t len)
{
	dprintf("g2k_leds: control %lx\n", op);
	return B_ERROR;
}

static device_hooks g2k_leds_device_hooks = {
	&g2k_leds_open,
	&g2k_leds_close,
	&g2k_leds_free,
	&g2k_leds_control, /* control */
	&g2k_leds_read,
	&g2k_leds_write, /* write */
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	NULL, /* writev */
};

int32 api_version = B_CUR_DRIVER_API_VERSION; 
static const char *device_name_list[] = {
	"misc/g2k_leds",
	NULL
};

const char **
publish_devices()
{
	return device_name_list;
}

device_hooks *
find_device(const char *name)
{
	dprintf("g2k_leds: find device, %s\n", name);
	return &g2k_leds_device_hooks;
}
