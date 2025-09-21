#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <USB.h>
#include <USB_printer.h>


int32 api_version = B_CUR_DRIVER_API_VERSION;


/****************************************************************/
/* Defines														*/
/****************************************************************/

#define DEBUG	0
#define ID 		"\033[32musb_printer:\033[30m ["__FUNCTION__"] "
#define F(x)	;	/* for function name */
#define M(x)	;	/* for informative messages */
#define E(x)	;	/* for errors */

#if DEBUG >= 2
#	undef M
#	define M(x) (x)
#endif

#if DEBUG >= 1
#	undef F
#	define F(x) (x)
#	undef E
#	define E(x) (x)
#endif

#define bug dprintf

#define kDeviceBaseName	"printer/usb"

#define PDEV_MAX 			32
#define DEVICE_ID_MAX_LEN	USB_PRINTER_DEVICE_ID_LENGTH
#define M_RDONLY_MASK		0x01
#define M_WRONLY_MASK		0x02
#define M_RDWR_MASK			(M_RDONLY_MASK|M_WRONLY_MASK)
#define M_IO_TIMEOUT_SHORT	1000000
#define M_IO_TIMEOUT_LONG	2000000

/****************************************************************/
/* Types														*/
/****************************************************************/

typedef volatile struct
{
	int				active;
	int32			open;
	int32			open_mode;
	usb_device		dev;
	usb_pipe		out;
	usb_pipe		in;
	
	sem_id			rd_lock;
	sem_id			rd_done;
	size_t			rd_len;
	uint32			rd_dev_status;
	
	sem_id			wr_lock;
	sem_id			wr_done;
	size_t			wr_len;
	uint32			wr_dev_status;

	bigtime_t		timeout;
	
	uint8			status_reg;
} printerdev;

typedef volatile struct
{
	int32		open_mode;
	printerdev	*dev;
} user_info;




/****************************************************************/
/* Static Prototypes											*/
/****************************************************************/

static status_t printer_open(const char *name, uint32 flags, void **cookie);
static status_t printer_free (void *cookie);
static status_t printer_close(void *cookie);
static status_t printer_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t printer_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t printer_write(void *cookie, off_t pos, const void *buf, size_t *count);

static status_t device_removed(void *cookie);
static status_t device_added(const usb_device dev, void **cookie);

static status_t update_printer_status(printerdev *p);

/****************************************************************/
/* Global Variables												*/
/****************************************************************/

static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;
sem_id dev_table_lock = -1;

static const char *printer_name[PDEV_MAX+1];

static printerdev *pdev[PDEV_MAX];

static device_hooks printer_devices = 
{
	&printer_open,
	&printer_close,
	&printer_free,
	&printer_control,
	&printer_read,
	&printer_write
};


static usb_notify_hooks notify_hooks =
{
	&device_added,
	&device_removed
};

usb_support_descriptor supported_devices[] =
{
	{ 7, 1, 0, 0, 0 }
};



static status_t add_device(usb_device dev, const usb_configuration_info *conf, int ifc)
{
	unsigned int i,j;
	printerdev *p;
	status_t result;
	F(bug(ID));
	
	if ((result = acquire_sem(dev_table_lock)) != B_OK)
		return result;

	result = usb->set_configuration(dev, conf);
	M(bug(ID "set_configuration() returned '%s'\n", strerror(result)));

	if (result == B_OK)
	{
		result = B_ERROR;
		for (i=0 ; i<PDEV_MAX ; i++)
		{
			if (pdev[i]) {
				continue;
			}
	
			result = B_NO_MEMORY;
			if ((p = (printerdev *)malloc(sizeof(printerdev))))
			{
				p->dev = dev;				
				p->active = 1;
				p->open = 0;
				p->open_mode = 0;
				p->in = NULL;
				p->out = NULL;

				M(bug(ID "conf = %p, ifc=%d, interface=%p\n", conf, ifc, conf->interface[ifc].active));

				/* find the bulk out endpoint */
				for (j=0 ; j<conf->interface[ifc].active->endpoint_count ; j++)
				{
					if (!(conf->interface[ifc].active->endpoint[j].descr->endpoint_address & 0x80))
					{
						p->out = conf->interface[ifc].active->endpoint[j].handle;
						M(bug(ID "Bulk OUT endpoint added 0x%08x / 0x%08x (/dev/" kDeviceBaseName "/%d)\n", p->dev, p->out, i));
						break;
					}
				}
				
				/* find the bulk in endpoint */
				for (j=0 ; j<conf->interface[ifc].active->endpoint_count ; j++)
				{
					if ((conf->interface[ifc].active->endpoint[j].descr->endpoint_address & 0x80))
					{
						p->in = conf->interface[ifc].active->endpoint[j].handle;
						M(bug(ID "Bulk IN endpoint added 0x%08x / 0x%08x (/dev/" kDeviceBaseName "/%d)\n", p->dev, p->in, i));
						break;
					}
				}
				pdev[i] = p;
				result = B_OK;
			}
			break;
		}
	}

	release_sem(dev_table_lock);
	return result;
}

static void remove_device(printerdev *p)
{
	int i;
	
	F(bug(ID "p = 0x%08lx\n", p));
	
	for (i=0 ; i<PDEV_MAX ; i++)
		if (pdev[i] == p)
			break;

	if (i == PDEV_MAX)
	{
		M(bug(ID "removed nonexistant device!?\n"));
		return;
	}
	
	M(bug(ID "removed 0x%08x (/dev/" kDeviceBaseName "/%d)\n",p,i));
	free((void *)p);
	pdev[i] = 0;
}

static status_t device_added(usb_device dev, void **cookie)
{
	unsigned int i, j;
	const usb_configuration_info *conf;
	
	/* Verify that the device is actually a printer */
	if ((conf = usb->get_nth_configuration(dev, 0)))
	{
		for (i=0 ; i<conf->interface_count ; i++)
		{
			for (j=0 ; j<conf->interface[i].alt_count ; j++)
			{ /* We must use the interface that has bInterfaceProtocol=02h (bidir) */
				const usb_interface_info *info = &(conf->interface[i].alt[j]);
				if ((info->descr->interface_class == 0x7)		&&
					(info->descr->interface_subclass == 0x1)	&&
					(info->descr->interface_protocol == 0x2))
				{
					status_t err = usb->set_alt_interface(dev, info);
					M(bug(ID "Setting ALTERNATE interface (%s)\n", strerror(err)));					
					if (err == B_OK) {
						err = add_device(dev, conf, i);
						M(bug(ID "add_device() returned '%s'\n", strerror(err)));
						if (err == B_OK) {
							*cookie = (void*)dev;
							return B_OK;
						}
					}
				}
			}
		}
	}
	else 
	{
		M(bug(ID "device has no config 0\n"));
	}
	return B_ERROR;
}

static status_t device_removed(void *cookie)
{
	usb_device dev = (usb_device )cookie;
	int i;
	
	acquire_sem(dev_table_lock);

	for (i=0 ; i<PDEV_MAX ; i++)
	{
		if (pdev[i] && (pdev[i]->dev == dev))
		{
			pdev[i]->active = 0;
			if (pdev[i]->open == 0)
			{
				remove_device(pdev[i]);
			}
			else
			{
				M(bug(ID "device /dev/" kDeviceBaseName "/%d still open -- marked for removal\n",i));
			}
		}
	}

	release_sem(dev_table_lock);
	return B_OK;
}


_EXPORT status_t init_hardware(void)
{
	F(bug(ID "init_hardware()\n"));
	return B_OK;
}



_EXPORT status_t init_driver(void)
{
	int i;
	
	F(bug(ID "%s %s\n", __DATE__, __TIME__));
	
	M(load_driver_symbols("usb_printer"));
	
	if (get_module(usb_name,(module_info**) &usb) != B_OK)
	{
		M(bug(ID "cannot get module \"%s\"\n",usb_name));
		return B_ERROR;
	}
	
	dev_table_lock = create_sem(1,"usb_printer:devtab_lock");
	for(i=0 ; i<PDEV_MAX ; i++)
		pdev[i] = NULL;
	printer_name[0] = NULL;
	
	usb->register_driver("usb_printer", supported_devices, 1, NULL);
	usb->install_notify("usb_printer", &notify_hooks);
	M(bug(ID "init_driver() done\n"));
	return B_OK;
}

_EXPORT void uninit_driver(void)
{
	int i;

	F(bug(ID));
	
	usb->uninstall_notify("usb_printer");

	/* Make sure there is no device left */
	acquire_sem(dev_table_lock);
	for (i=0 ; i<PDEV_MAX ; i++)
	{
		if (pdev[i])
		{
			dprintf(ID "pdev %x still exists?!\n", pdev[i]);
			remove_device(pdev[i]);
		}
	}

	release_sem_etc(dev_table_lock, 1, B_DO_NOT_RESCHEDULE); /* No need to resched, here */
	delete_sem(dev_table_lock);

	put_module(usb_name);
	for (i=0 ; printer_name[i] ; i++)
		free((char*) printer_name[i]);
	M(bug(ID "uninit_driver() done\n"));
}

_EXPORT const char** publish_devices()
{
	int i,j;
	
	F(bug(ID));
	
	for(i=0 ; printer_name[i] ; i++)
		free((char *)printer_name[i]);
	
	acquire_sem(dev_table_lock);
	for(i=0,j=0 ; i<PDEV_MAX ; i++)
	{
		if (pdev[i] && pdev[i]->active)
		{
			if ((printer_name[j] = (char *)malloc( strlen(kDeviceBaseName)+1+3+1 )))
			{
				sprintf((char *)printer_name[j], kDeviceBaseName "/%u", i);
				M(bug(ID "publishing %s\n", printer_name[j]));
				j++;
			}
		}
	}
	release_sem(dev_table_lock);
	printer_name[j] = NULL;
	return printer_name;
}

_EXPORT device_hooks* find_device(const char* name)
{
	F(bug(ID "find_device(%s)\n", name));
	return &printer_devices;
}

static status_t printer_open(const char *dname, uint32 flags, void **cookie)
{
	int i;
	int current;
	int32 open_mask;
	user_info *info;
	status_t err = B_ERROR;
	
	F(bug(ID "printer_open(""%s"", 0x%08lx, 0x%08lx)\n", dname, flags, cookie));
	
	i = atoi(dname + strlen(kDeviceBaseName) + 1);
	
	if ((err = acquire_sem(dev_table_lock)) != B_OK)
		return err;
	
	if (pdev[i])
	{ /* This device exists... */
		err = B_ERROR;
		*cookie = NULL; /* Set the cookie to NULL in case of error */
		if (pdev[i]->active == 0)
		{
			M(bug(ID "device is going offline, cannot open.\n"));
			err = EBADF;
			goto error;
		}

		/* and is active, open it with the requested access mode if valid */
		switch (flags & O_ACCMODE)
		{
			case O_RDONLY:	// We can always open the driver in O_RDONLY
				pdev[i]->open_mode |= (open_mask = M_RDONLY_MASK);
				break;
				
			case O_WRONLY: // We cannot open the driver for writting if it is already opened for writting
				if ((pdev[i]->open_mode & M_WRONLY_MASK) == 0) {
					pdev[i]->open_mode |= (open_mask = M_WRONLY_MASK);
					break;
				}
				err = EBUSY;
				goto error_already_opened;
		
			case O_RDWR: // We cannot open the driver for writting if it is already opened for writting
				if ((pdev[i]->open_mode & M_WRONLY_MASK) == 0) {
					pdev[i]->open_mode |= (open_mask = M_RDWR_MASK);
					break;
				}
				err = EBUSY;
				goto error_already_opened;

			default:
				err = EINVAL;
				goto error_already_opened;
		}

		/* Record user specific data */
		info = (user_info *)malloc(sizeof(user_info));
		if (info == 0)
		{ /* Not enough memory : Abort */
			err = ENOMEM;
			goto error;
		}

		/* No error allowed from here */
		err = B_OK;
		info->dev = pdev[i];
		info->open_mode = open_mask;

		pdev[i]->open++;

		/* Init the device if needed */
		if (pdev[i]->open == 1)
		{	/* The device is opened for the first time */
			char name[64];
			pdev[i]->timeout = M_IO_TIMEOUT_SHORT;
			pdev[i]->wr_dev_status = 0;

			sprintf(name, "usb_printer:%d:rd_lock", i);
			pdev[i]->rd_lock = create_sem(1, name);

			sprintf(name, "usb_printer:%d:wr_lock", i);
			pdev[i]->wr_lock = create_sem(1, name);
	
			sprintf(name, "usb_printer:%d:rd_done", i);
			pdev[i]->rd_done = create_sem(0, name);
						
			sprintf(name, "usb_printer:%d:wr_done", i);
			pdev[i]->wr_done = create_sem(0, name);
		}


		/* Set the cookie */
		*cookie = (void *)info;
	}
	else
	{
		F(bug(ID "printer_open(""%s"", 0x%08lx, 0x%08lx), no device\n", dname, flags, cookie));
		err = ENODEV;
	}
	
error_already_opened:
error:
	release_sem(dev_table_lock);
	F(bug(ID "printer_open(""%s"", 0x%08lx, 0x%08lx), err = %lx, info = %p\n", dname, flags, cookie, err, info));
	return err;
}


static status_t printer_free(void *cookie)
{
	status_t s;
	printerdev *p = ((user_info *)cookie)->dev;
	int32 open_mode = ((user_info *)cookie)->open_mode;
	
	F(bug(ID "printer_free(0x%08lx)\n", cookie));

	if ((s = acquire_sem(dev_table_lock)) == B_OK)
	{
		/* this works because the driver can be opened for write only once */
		if (open_mode & M_WRONLY_MASK)
			p->open_mode &= ~M_WRONLY_MASK;
		p->open--;
		if (p->open == 0)
		{ /* This is the last opened driver */
			usb->cancel_queued_transfers(p->in);
			usb->cancel_queued_transfers(p->out);

			delete_sem(p->rd_lock);
			delete_sem(p->rd_done);
			delete_sem(p->wr_lock);
			delete_sem(p->wr_done);
	
			/* the driver is not opened anymore */
			p->open_mode = 0;
	
			/* If the device was marked for removal, remove it now! */
			if (p->active == 0)
				remove_device(p);
		}
		
		release_sem(dev_table_lock);
	}
	
	/* Free the user cookie */
	free((void *)cookie);	
	return B_OK;
}

static status_t printer_close(void *cookie)
{
// TODO: Should I cancel the current transfert here?
	printerdev *p = ((user_info *)cookie)->dev;
	F(bug(ID "printer_close(0x%08lx)\n", cookie));
	return B_OK;
}


void cb_rd_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{ /* !!! Here, we receive a "printerdev *" */
	printerdev *p = ((printerdev *)cookie);
	p->rd_len = actual_len;
	p->rd_dev_status = status;
	M(bug(ID "READ CALLBACK : queue_bulk (status=%08lx, size=%lu)\n", p->rd_dev_status, p->rd_len));
	release_sem(p->rd_done);
}

void cb_wr_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{ /* !!! Here, we receive a "printerdev *" */
	printerdev *p = ((printerdev *)cookie);
	p->wr_len = actual_len;
	p->wr_dev_status = status;
	M(bug(ID "WRITE CALLBACK : queue_bulk (status=%08lx, size=%lu)\n", p->wr_dev_status, p->wr_len));
	release_sem(p->wr_done);
}


static status_t printer_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	printerdev *p = ((user_info *)cookie)->dev;
	size_t sz = *count;
	const size_t orig_size = *count;
	status_t s = B_OK;
	char *rd_addr = (char *)buf;
	bool canceled = false;
	
	F(bug(ID "printer_read(0x%08lx (dev=0x%08lx), %ld, 0x%08lx, %ld)\n", cookie, p, (long)pos, buf, (long)*count));
	
	if (p->active == 0) {
		*count = 0;
		return EIO;
	}

	/* lock the user data into memory */
	if ((s = lock_memory(buf, orig_size, B_DMA_IO|B_READ_DEVICE)) != B_OK)
		return s;

	*count = 0;
	s = acquire_sem_etc(p->rd_lock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
	if (s != B_OK)
		return s;
	
	while ((sz > 0) && (!canceled))
	{
		p->rd_len = 0;
		p->rd_dev_status = 0;
		if ((s = usb->queue_bulk(p->in, rd_addr, sz, cb_rd_notify, (void *)p)) != B_OK)
		{
			E(bug(ID "dev 0x%08x status %d\n", cookie, s));
			break;
		}
		
		do
		{
			s = acquire_sem_etc(p->rd_done, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, p->timeout);
			if (s != B_OK)
			{
				canceled = true;
				usb->cancel_queued_transfers(p->in);
				E(bug(ID "cancel_queued_transfers() called\n"));
			}
		} while (s == B_TIMED_OUT);
		
		if (s != B_OK)
			break;

		if (p->rd_dev_status)
		{ /* There was an error */
			s = EIO;
			break;
		}
		
		/* update counters */
		 if (p->rd_len < sz) {
			*count += p->rd_len;
			break;
		 }
		 
		rd_addr		+= p->rd_len;
		*count		+= p->rd_len;
		sz			-= p->rd_len;
	}
	
	M(bug(ID "%lu bytes read.\n", (uint32)*count));
	release_sem(p->rd_lock);

	/* unlock the user data */
	unlock_memory(buf, orig_size, B_DMA_IO|B_READ_DEVICE);

	return s;
}

static status_t printer_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	const user_info *uinfo = (user_info *)cookie;
	printerdev *p = uinfo->dev;
	size_t sz = *count;
	const size_t orig_size = *count;
	status_t s;
	char *wr_addr = (char *)buf;
	
	F(bug(ID "printer_write(0x%08lx, %ld, 0x%08lx, %ld)\n", cookie, (long)pos, wr_addr, (long)*count));
	
	if (p->active == 0) {
		*count = 0;
		return EIO;
	}

	/* Nothing to send */
	if (sz <= 0)
		return B_OK;
	
	/* lock the user data into memory */
	if ((s = lock_memory((void *)buf, orig_size, 0)) != B_OK)
		return s;

	*count = 0;
	s = acquire_sem_etc(p->wr_lock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
	if (s != B_OK)
		return s;

	while (sz > 0)
	{
		p->wr_dev_status = 0;
		M(bug(ID "queue_bulk(%08lx, %lu)\n", wr_addr, sz));
		if ((s = usb->queue_bulk(p->out, wr_addr, sz, cb_wr_notify, (void *)p)) != B_OK)
		{	E(bug(ID "*** dev 0x%08x status %d\n", cookie, s));
			break;
		}

		/* Wait the end of the transfert */
		s = acquire_sem_etc(p->wr_done, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, p->timeout);
		if (s != B_OK) { 
			const status_t err = s;
			E(bug(ID "*** acquire_sem_etc() %08lx (%s)\n", s, strerror(s)));
			if ((s = usb->cancel_queued_transfers(p->out)) == B_OK) {
				if (err == B_TIMED_OUT) { /* The problem was a timeout, wait cancel transfert to finish */
					s = acquire_sem_etc(p->wr_done, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, M_IO_TIMEOUT_LONG);
					M(bug(ID "acquire_sem_etc(wr_done) -> %s!\n", strerror(s)));
					if (s != B_OK) {
						M(bug(ID "cancel_queued_transfers() FAILED [%s]!\n", strerror(s)));
						break;
					}
					*count += p->wr_len;
				}
			} else {
				M(bug(ID "cancel_queued_transfers() FAILED [%s]!\n", strerror(s)));	
			}
			/* There was an I/O error, go back to userland... */
			break;
		} else {
			if (p->wr_dev_status)
			{ /* there was an error, and the semaphore was released */
				E(bug(ID "*** error in queue_bulk() = %08lx\n", p->wr_dev_status));
				*count += p->wr_len;
				s = B_OK;
				break;
			}
			/* Everything went fine, so continue ... */
		}

		/* Update counters */
		M(bug(ID "queue_bulk OK(status=%08lx, size=%lu)\n", p->wr_dev_status, p->wr_len));
		wr_addr		+= p->wr_len;
		*count		+= p->wr_len;
		sz			-= p->wr_len;
	}
	
	M(bug(ID "%lu bytes written.\n", (uint32)*count));	
	release_sem(p->wr_lock);

	/* unlock the user data */
	unlock_memory((void *)buf, orig_size, 0);

	return s;
}


static status_t printer_control(void *cookie, uint32 msg, void *arg1, size_t len)
{
	size_t act, i;
	status_t s;
	user_info *uinfo = (user_info *)cookie;
	printerdev *p = uinfo->dev;
	
	F(bug(ID "printer_control(0x%08lx, %ld, 0x%08lx, %ld)\n", cookie, msg, arg1, len));

	switch (msg)
	{
		case USB_PRINTER_SET_STATUS_CAPABILITY:
		{
			return B_OK;
		}
		case USB_PRINTER_GET_DEVICE_ID:
		{
			uchar *x;
			size_t act;
			if (arg1 == NULL)
				return B_BAD_VALUE;

			if ((x = (uchar *)malloc(DEVICE_ID_MAX_LEN)))
			{
				int tries=3;
			
				// First request only the size of the command
				if (usb->send_request(p->dev, 0xA1, 0, 0, 0, 2, x, &act) || (act<2)) {
					M(bug(ID "GET_DEVICE_ID fails\n"));
					free(x);
					return B_IO_ERROR;
				}
				
				i = (x[0] << 8) | x[1];
				if (i > DEVICE_ID_MAX_LEN-2)
					i = (DEVICE_ID_MAX_LEN-2);
				
				if (i<=2) {
					M(bug(ID "1) GET_DEVICE_ID returns empty ID\n"));
					strcpy(arg1, "");						
					free(x);
					return B_OK;
				}
				
				// Now get the device id itself
				do {
					if (usb->send_request(p->dev, 0xA1, 0, 0, 0, i, x, &act)) {
						M(bug(ID "GET_DEVICE_ID fails\n"));
						free(x);
						return B_IO_ERROR;
					}
					if (act<=2) {
						// The printer didn't answer. Wait a little and try again.
						snooze(250000);
					}
				} while ((act<=2) && (tries-->0));
					
				if (act > 2) {
					x[i] = 0;						
					strcpy(arg1, x+2);						
					M(bug(ID "GET_DEVICE_ID = %s\n", x+2));
				} else {
					M(bug(ID "2) GET_DEVICE_ID returns empty ID (len=%ld, act=%ld)\n", i, act));
					strcpy(arg1, "");						
					free(x);
					return B_OK;
				}
				free(x);
			}
			return B_OK;
		}

		case USB_PRINTER_GET_PORT_STATUS:
		{
			status_t result;
			if (arg1 == NULL)
				return B_BAD_VALUE;
			if ((result = update_printer_status(p)) != B_OK)
				return result;
			*((char *)arg1) = p->status_reg;
			return B_OK;
		}

		case USB_PRINTER_SOFT_RESET:
		{
			size_t act;
			if (usb->send_request(p->dev, 0xA1, 2, 0, 0, 0, NULL, &act))
				return B_IO_ERROR;
			return B_OK;
		}
	}
	
	return B_DEV_INVALID_IOCTL;
}

status_t update_printer_status(printerdev *p)
{
	int i;
	status_t result = B_OK;
	size_t act;
	uint8 *status = (uint8 *)&(p->status_reg);

	for (i=0 ; i<5 ; i++)
	{ // Sometimes those bad HP printers returns an error here, so you have to try again... Allooooo?
		if ((result = usb->send_request(p->dev, 0xA1, 1, 0, 0, 1, status, &act)) == B_OK)
			break;
		M(bug(ID "USB_PRINTER_GET_PORT_STATUS: err=%08lx (%s)\n", result, strerror(result)));
	}

	if (result != B_OK) {
		return result;
	}

	M(bug(ID "USB_PRINTER_GET_PORT_STATUS: len=%ld, status=%02X\n", act, (int)*status));

	return B_OK;
}

