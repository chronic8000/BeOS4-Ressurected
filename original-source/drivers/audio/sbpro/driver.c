/*	driver.c*/
/*	Sound Blaster Pro*/
/*	Joseph Wang*/
/*	4.4.99*/
/*	driver stub for setting up and tearing down driver*/

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sbpro.h"
#include "pcm.h"
#include "driver.h"



int32 api_version = B_CUR_DRIVER_API_VERSION;

const char *dev_name[] = {
	"audio/raw/sbpro/1",
	"audio/old/sbpro/1",
	NULL
	};

device_hooks dev_hook[] = {
	{	pcm_open,
		pcm_close,
		pcm_free,
		pcm_control,
		pcm_read,
		pcm_write,
		NULL,NULL,NULL,NULL
		},
	{	pcm_open,
		pcm_close,
		pcm_free,
		pcm_control,
		pcm_read,
		pcm_write,
		NULL,NULL,NULL,NULL
		}
	};


dma_buffer write_buffer, read_buffer;
struct isa_module_info *isa_info;
int dma8;
spinlock lock;
cpu_status status;
int port, irq, dma16, mpu;

/*driver functions*/

/*execute first time driver is loaded*/
status_t init_hardware() {
	return B_OK;
	}

/*execute each time driver is loaded*/
status_t init_driver() {
	status_t err;
	char buffer[64], *s;
	int fd, num;
	physical_entry table;
/*	init isa*/
	write_buffer.int_sem = -1;
	err=get_module(B_ISA_MODULE_NAME, (module_info **)&isa_info);
	if(err!=B_OK) return err;
/*	init sbpro*/
	port=DEFAULT_PORT;
	irq=DEFAULT_IRQ;
	dma8=DEFAULT_DMA8;
	dma16=DEFAULT_DMA16;
	mpu=DEFAULT_MPU;
	fd=open("/boot/home/config/settings/sbpro", O_RDONLY);
	if(fd<=0) goto FINISH_INIT;
	num=read(fd, buffer, sizeof(buffer)-1);	//	hplus possible overwrite
	close(fd);
	if(num > 0) {
		buffer[num] = 0;
		if((s = strchr(buffer, 'A')) != NULL) port = strtol(s + 1, NULL, 16);
		if((s = strchr(buffer, 'I')) != NULL) irq = strtol(s + 1, NULL, 10);
		if((s = strchr(buffer, 'D')) != NULL) dma8 = strtol(s + 1, NULL, 10);
		if((s = strchr(buffer, 'H')) != NULL) dma16 = strtol(s + 1, NULL, 10);
		if((s = strchr(buffer, 'P')) != NULL) mpu = strtol(s + 1, NULL, 16);
		}
FINISH_INIT:
	acquire_sl();
	err=check_hw();
	release_sl();	//	hplus leak!
	if(err!=B_OK) { return err; }
/*	create dma buffers*/
	err=write_buffer.area = create_area("audio buffer", (void **)&write_buffer.data,
			B_ANY_KERNEL_ADDRESS, DMA_BUFFER_SIZE, B_LOMEM, B_READ_AREA|B_WRITE_AREA);
	if (err < B_OK) goto BAD_INIT;	//	hplus  check!
	write_buffer.halfcount = 0;	//	hplus	double-buffering
	err = write_buffer.int_sem = create_sem(2, "sbpro interrupt");	/*	hplus double-buffering	*/
	if (err < B_OK) goto BAD_INIT;
	write_buffer.pending = 0;
	write_buffer.halfcount = 0;
	write_buffer.written = 0;
	write_buffer.size=DMA_BUFFER_SIZE;
	if(get_memory_map(write_buffer.data, write_buffer.size, &table, 1) < B_OK) goto BAD_INIT;
	write_buffer.size=B_PAGE_SIZE/2;
	if((uint32)table.address & 0xff000000) goto BAD_INIT;
	if ((((uint32)table.address)+DMA_BUFFER_SIZE) & 0xff000000) goto BAD_INIT;
	acquire_sl();
	install_io_interrupt_handler(irq, select_inth, NULL, 0);
	write_reg(SBPRO_MASTER, 0x88);
	write_reg(SBPRO_OUTPUT_MUX, 0x02);	//stereo
	release_sl();
    return B_OK;
BAD_INIT:
	delete_sem(write_buffer.int_sem);
	delete_area(write_buffer.area);
	release_sl();
	return B_ERROR;
	}

const char **publish_devices() {
	return dev_name;
	}

device_hooks *find_device(const char *name) {
	int i;
	for(i=0; dev_name[i]; i++) {
		if(!strncmp(name, dev_name[i], B_OS_NAME_LENGTH)) {
			return &dev_hook[i];
			}
		}
	return NULL;
	}

void uninit_driver() {
	reset_hw();
	remove_io_interrupt_handler(irq, select_inth, NULL);
	delete_area(write_buffer.area);
	delete_sem(write_buffer.int_sem);	//	hplus
	put_module(B_ISA_MODULE_NAME);
	}

/*support functions*/

status_t check_hw() {
	uint8 high, low;
	status_t err;
	err=reset_hw();
	if(err!=B_OK) return err;
/*	get the dsp version number*/
	if(write_data(SBPRO_VERSION)!=B_OK) return B_ERROR;
	if(read_data(&high)!=B_OK) return B_ERROR;
	if(read_data(&low)!=B_OK) return B_ERROR;
	if(high!=3) { dprintf("version %d.%02x is bad\n", high, low); return B_ERROR; }
/*	lock the 8-bit dma channel*/
   	isa_info->lock_isa_dma_channel(dma8);
	return B_OK;
	}

status_t reset_hw() {
/*	reset dsp*/
	int i;
	for(i=133; i>=0; i--) {
/*		poll dsp for 100usec, 3usec each poll*/
		uint8 value;
		write_io(SBPRO_RESET, 1);
		spin(SBPRO_DELAY);		//	hplus	don't call snooze() with interrupts disabled
		write_io(SBPRO_RESET, 0);
		if(read_data(&value)!=B_OK) continue;
		if(value==0xaa) {
			write_buffer.pending = 0;
			write_buffer.halfcount = 0;
			write_buffer.written = 0;
			return B_OK;
		}
	}
	return B_ERROR;
}

void write_io(int offset, uint8 value) {
	isa_info->write_io_8(port+offset, value);
	}

uint8 read_io(int offset) {
	return isa_info->read_io_8(port+offset);
	}

status_t write_data(uint8 value) {
	int i;
/*	is bit 7 high?*/
	for(i=133; i>0; i--) {	//	hplus longer bail time
		if(read_io(SBPRO_WRITE_BUFFER_STATUS)&0x80) {
			spin(SBPRO_DELAY);	//	hplus don't snooze() with interrupts disabled
			continue;
			}
		write_io(SBPRO_WRITE_DATA, value);
		return B_OK;
		}
	return B_ERROR;
	}

status_t read_data(uint8 *value) {
	int i;
/*	is bit 7 high?*/
	for(i=133; i>0; i--) {	//	hplus longer bail time
		if(read_io(SBPRO_READ_BUFFER_STATUS)&0x80) {
			*value=read_io(SBPRO_READ_DATA);
			return B_OK;
			}
		spin(SBPRO_DELAY);	//	hplus	don't call snooze() with interrupts disabled
		}
	return B_ERROR;
	}

void write_reg(uint8 reg, uint8 value) {
	write_io(SBPRO_REG, reg);
	write_io(SBPRO_DATA, value);
	}

uint8 read_reg(uint8 reg) {
	write_io(SBPRO_REG, reg);
	return read_io(SBPRO_DATA);
	}

void acquire_sl(void) {
	cpu_status temp = disable_interrupts();	//	hplus MP safe
	temp = disable_interrupts();
	acquire_spinlock(&lock);
	status = temp;
	}

void release_sl(void) {
	cpu_status temp = status;	//	hplus MP safe
	release_spinlock(&lock);
	restore_interrupts(temp);
	}

int32 select_inth(void *data) {
	return pcm_write_inth(data);
	}
