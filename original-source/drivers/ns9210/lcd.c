
#include <KernelExport.h>

#include <Drivers.h>
#include <PCI.h>

static pci_module_info *pci;
static pci_info dev;

#define GPIO_DIR   0x90
#define GPIO_DATA  0x91
#define GPIO_CTL   0x92

#define GPIO_BIT0  0x01
#define GPIO_BIT1  0x02
#define GPIO_BIT2  0x04
#define GPIO_BIT3  0x08
#define GPIO_BIT4  0x10
#define GPIO_BIT5  0x20
#define GPIO_BIT6  0x40
#define GPIO_BIT7  0x80

#define SCLK  GPIO_BIT2
#define SDIN  GPIO_BIT7
#define SCS   GPIO_BIT6
#define SDOUT GPIO_BIT5

#define SMASK (GPIO_BIT0|GPIO_BIT1|GPIO_BIT3|GPIO_BIT4)

#define SDIR  (SCLK|SDIN|SCS)           // these lines should be outputs

#define WR(ofs,val) (pci->write_pci_config(dev.bus,dev.device,dev.function,ofs,1,val))
#define RD(ofs) (pci->read_pci_config(dev.bus,dev.device,dev.function,ofs,1))

#define OUTB(addr,val) (pci->write_io_8(addr,val))

#define SET(bits) WR(GPIO_DATA,(RD(GPIO_DATA)&SMASK)|(bits))

#include "data.h"

/* 32 sync, cs+, 1 start, 3 command, 8 addr, 8 data, 1 stop, 16 sync */
#define STREAM_LEN ((32 + 1 + 1 + 3 + 8 + 8 + 1 + 16))

static uchar bits[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

void 
dstn_write(uchar addr, uchar data)
{
	cpu_status ps;
	int i;
	uchar stream[STREAM_LEN], *p;
	
	p = stream;	
	
	/* sync pulses */
	for(i=0;i<32;i++) { p[0] = 0; p++; }
	
	/* turn on CS */
	p[0] = SCS; 
	
	/* start bit */
	p[1] = SCS | SDIN;
	
	/* command 0 0 1 */
	p[2] = SCS;
	p[3] = SCS;
	p[4] = SCS | SDIN;
	p += 5;
	
	/* addr */
	for(i=7;i>=0;i--){
		if(bits[i] & addr) {
			p[0] = SCS | SDIN;
		} else {
			p[0] = SCS;
		}
		p++;
	}
	
	/* data */
	for(i=7;i>=0;i--){
		if(bits[i] & data) {
			p[0] = SCS | SDIN;
		} else {
			p[0] = SCS;
		}
		p++;
	}
	
	/* hold CS for one more toggle */
	p[0] = SCS; p++;
	
	/* sync pulses */
	for(i=0;i<16;i++){ p[0] = 0; p++;	}	
	
	ps = disable_interrupts();
	for(i=0;i<STREAM_LEN;i++) {
		SET(stream[i]);
		SET(stream[i] | SCLK);
		SET(stream[i]);
	}
	restore_interrupts(ps);
}

void lcd_program(uchar *data, int length)
{
	cpu_status ps;
	
	ps = disable_interrupts();
	while(length){
		OUTB(0x3d4, data[0]);
		OUTB(0x3d5, data[1]);
		data += 2;
		length--;
	}
	restore_interrupts(ps);
}

void dstn_program(uchar *data, int length)
{
	/* set some bits that should be set */
	//WR(0x43,RD(0x43) | 0x44);
	WR(GPIO_DIR,(RD(GPIO_DIR)&SMASK)|SDIR);
	while(length){
		dstn_write(data[0],data[1]);
		data += 2;
		length --;
	}
}

static status_t 
device_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}

static status_t
device_close(void *cookie)
{
	lcd_program(lcd_data,sizeof(lcd_data)/2);
	snooze(1000 * 6 * 256);
	dstn_program(dstn_data,sizeof(dstn_data)/2);
	return B_OK;
}

static status_t
device_free(void *cookie)
{
	return B_OK;
}

static status_t
device_control(void *cookie, uint32 op, void *data, size_t len)
{
	return B_OK;
}

static status_t
device_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	return B_OK;
}

static status_t
device_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	return B_OK;
}


static device_hooks hooks = 
{
	&device_open,
	&device_close,
	&device_free,
	&device_control,
	&device_read,
	&device_write
};
	
status_t
init_driver(void)
{
	int i;
	dprintf("lcd: init_driver()\n");
	if(get_module(B_PCI_MODULE_NAME,(module_info**)&pci)){
		return B_ERROR;
	}
	
	for(i=0;pci->get_nth_pci_info(i,&dev)==B_OK;i++){
		if((dev.vendor_id == 0x1078) &&
		   (dev.device_id == 0x0100)){
			dprintf("lcd: Device @ %d/%d/%d\n",
					dev.bus,dev.device,dev.function);
			return B_OK;
		}
	}
	
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}

void uninit_driver(void)
{
	dprintf("lcd: uninit_driver()\n");
	put_module(B_PCI_MODULE_NAME);
}

device_hooks *find_device(const char *name)
{
	return &hooks;
}

const char *names[] = 
{
	"misc/lcd",
	NULL
};

const char **publish_devices()
{
	return names;
}

