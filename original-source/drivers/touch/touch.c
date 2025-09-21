#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>

static status_t 	driver_open(const char *name, uint32 flags, void **cookiep);
static status_t 	driver_close(void *cookie);
static status_t		driver_free(void *_cookie);
static status_t 	driver_control(void *cookie, uint32 op, void *data, size_t len);
static status_t 	driver_read(void *_cookie, off_t position, void *data, size_t *numBytes);
static status_t 	driver_write(void *_cookie, off_t position, const void *data, size_t *numBytes);

pci_module_info *pci;
pci_info		pm_dev;
sem_id			sem;
cpu_status		saved_ps;

static const char *names[] = { "misc/cop8", NULL };

static device_hooks hooks = {
	driver_open,
	driver_close,
	driver_free,
	driver_control,
	driver_read,
	driver_write,
	NULL,
	NULL,
	NULL,
	NULL	
};

static status_t
scan(void)
{
	int			i;

	for(i=0; pci->get_nth_pci_info(i, &pm_dev) == B_OK; i++)
		if ((pm_dev.vendor_id == 0x1078) && (pm_dev.device_id == 0x0100))
			return B_OK;
	return ENODEV;
}

status_t
init_driver()
{
	status_t	err;

	err = get_module(B_PCI_MODULE_NAME, (module_info **) &pci);
	if (err)
		goto error1;
	err = scan();
	if (err)
		goto error2;
dprintf("touch: found device at %d %d %d\n", pm_dev.bus, pm_dev.device, pm_dev.function);
	sem = create_sem(1, "touchscreen");
	if (sem <= 0)
		goto error2;
dprintf("call configureHIBInterface\n");
	configureHIBInterface();
dprintf("configureHIBInterface returned\n");
	return B_OK;

error2:
	put_module(B_PCI_MODULE_NAME);
error1:
	return err;
}

void
uninit_driver(void)
{
	delete_sem(sem);
	put_module(B_PCI_MODULE_NAME);
}

const char **
publish_devices(void)
{
	return names;
}

device_hooks *
find_device(const char *name)
{
	return &hooks;
}


static status_t 
driver_open(const char *name, uint32 flags, void **cookiep)
{
	*cookiep = NULL;
	return B_OK;
}

static status_t
driver_free(void *_cookie)
{
	return B_OK;
}

static status_t 
driver_read(void *_cookie, off_t position, void *data, size_t *numBytes)
{
	int			i;
	int			len;
	char		*ptr;
	char		pos[4];

	acquire_sem(sem);
	readPenStatus(&pos[0]);
	release_sem(sem);

	len = 4;
	if (*numBytes < len)
		len = *numBytes;
	for(i=0; i<len; i++)
		((char *)data)[i] = pos[i];
	*numBytes = len;
	return B_OK;
}

static status_t
driver_close(void *cookie)
{
	return B_OK;
}

static status_t
driver_control(void *cookie, uint32 op, void *data, size_t len)
{
	return EINVAL;
}

static status_t
driver_write(void *_cookie, off_t position, const void *data, size_t *numBytes)
{
	uint8		*ptr = data;
	char		contrast;
	char		brightness;

	if(*numBytes < 2)
		return B_ERROR;
	*numBytes = 2;

	contrast = ptr[0]+0x80-'A';
	brightness = ptr[1]+0x80-'A';

	acquire_sem(sem);
	SetPWM(contrast, brightness);
	release_sem(sem);

	return B_OK;
}


uint8
PCIC_IN_8(int dummy, uint8 offset)
{
	uint8 val;
	val = pci->read_pci_config(pm_dev.bus, pm_dev.device, pm_dev.function, offset, 1);
//dprintf("PCIC_IN_8 0x%x got 0x%x\n", offset, val);
	return val;
}

void
pm_config_set(uint8 offset, uint8 clearmask, uint8 setmask)
{

	uint8 val;

//dprintf("pm_config_set( 0x%x, 0x%x, 0x%x )\n", offset,setmask,clearmask);
	val = pci->read_pci_config(pm_dev.bus, pm_dev.device, pm_dev.function, offset, 1);
//dprintf("pm_config_set old val 0x%x\n", val);
	val &= ~clearmask;
	val |= setmask;
//dprintf("pm_config_set new val 0x%x\n", val);
	pci->write_pci_config(pm_dev.bus, pm_dev.device, pm_dev.function, offset, 1, val);
}

void
disable_nesting(void)
{
	saved_ps = disable_interrupts();
}

void
enable_nesting(void)
{
	restore_interrupts(saved_ps);
}
