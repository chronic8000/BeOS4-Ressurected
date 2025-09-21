#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <USB.h>


int32 api_version = B_CUR_DRIVER_API_VERSION;


/****************************************************************/
/* Defines														*/
/****************************************************************/

#define DEBUG	0
#define ID 		"\033[32mgempc430:\033[30m "
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

#define kDeviceBaseName	"smartcard/usb/gempc430"

#define PDEV_MAX 			32
#define TMP_WRITE_BUF_SIZE	(B_PAGE_SIZE)
#define TMP_READ_BUF_SIZE	(B_PAGE_SIZE)
#define M_RDONLY_MASK		0x01
#define M_WRONLY_MASK		0x02
#define M_RDWR_MASK			(M_RDONLY_MASK|M_WRONLY_MASK)
#define M_IO_TIMEOUT_SHORT	1000000

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
	area_id			rd_area_id;
	void			*rd_addr;
	size_t			rd_len;
	uint32			rd_dev_status;
	
	sem_id			wr_lock;
	sem_id			wr_done;
	area_id			wr_area_id;
	void			*wr_addr;
	size_t			wr_len;
	uint32			wr_dev_status;
	bool			wr_transfert_pending;

	bigtime_t		timeout;
} gempc430dev;

typedef volatile struct
{
	int32		open_mode;
	gempc430dev	*dev;
} user_info;




/****************************************************************/
/* Static Prototypes											*/
/****************************************************************/

static status_t device_open(const char *name, uint32 flags, void **cookie);
static status_t device_free (void *cookie);
static status_t device_close(void *cookie);
static status_t device_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t device_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t device_write(void *cookie, off_t pos, const void *buf, size_t *count);

static status_t device_removed(void *cookie);
static status_t device_added(const usb_device dev, void **cookie);


/****************************************************************/
/* Global Variables												*/
/****************************************************************/

static char *usb_name = B_USB_MODULE_NAME;
static usb_module_info *usb;
static sem_id dev_table_lock = -1;

static const char *gempc430_name[PDEV_MAX+1];
static gempc430dev *pdev[PDEV_MAX];

static device_hooks gempc430_devices = 
{
	&device_open,
	&device_close,
	&device_free,
	&device_control,
	&device_read,
	&device_write
};

static usb_notify_hooks notify_hooks =
{
	&device_added,
	&device_removed
};

usb_support_descriptor supported_devices[] =
{
	{ 0, 0, 0, 0x08e6, 0x430 }			// GEMPLUS / GEMPC430
};



static void add_device(usb_device dev, const usb_configuration_info *conf, int ifc)
{
	unsigned int i,j;
	gempc430dev *p;
	
	F(bug(ID "add_device()\n"));
	
	acquire_sem(dev_table_lock);

	for(i=0 ; i<PDEV_MAX ; i++)
	{
		if (!pdev[i])
		{
			if ((p = (gempc430dev *)malloc(sizeof(gempc430dev))))
			{
				p->dev = dev;				
				p->active = 1;
				p->open = 0;
				p->open_mode = 0;
				
				p->rd_area_id = create_area(	"gempc430:rd_area",
					(void **)&(p->rd_addr), B_ANY_KERNEL_ADDRESS,
					TMP_READ_BUF_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
				
				p->wr_area_id = create_area(	"gempc430:wr_area",
					(void **)&(p->wr_addr), B_ANY_KERNEL_ADDRESS,
					TMP_WRITE_BUF_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);

				M(bug("gempc430:%d: %d @ 0x%08x\n", i, p->rd_area_id, p->rd_addr));
				M(bug("gempc430:%d: %d @ 0x%08x\n", i, p->wr_area_id, p->wr_addr));
				
				usb->set_configuration(dev, conf);
				
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
			}
			break;
		}
	}
	release_sem(dev_table_lock);
}

static void remove_device(gempc430dev *p)
{
	int i;
	
	F(bug(ID "remove_device(0x%08lx)\n", p));
	
	for (i=0 ; i<PDEV_MAX ; i++)
		if (pdev[i] == p)
			break;

	if (i == PDEV_MAX)
	{
		M(bug(ID "removed nonexistant device!?\n"));
		return;
	}
	
	M(bug(ID "removed 0x%08x (/dev/" kDeviceBaseName "/%d)\n",p,i));
	delete_area(p->rd_area_id);
	delete_area(p->wr_area_id);
	free((void *)p);
	pdev[i] = 0;
}

static status_t device_added(usb_device dev, void **cookie)
{
	const usb_configuration_info *conf;	
	if ((conf = usb->get_nth_configuration(dev, 0)))
	{
		add_device(dev, conf, 0);
		*cookie = (void*)dev;
		return B_OK;
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
	
	F(bug(ID "%s %s, init_driver()\n", __DATE__, __TIME__));
	
	M(load_driver_symbols("gempc430"));
	
	if (get_module(usb_name,(module_info**) &usb) != B_OK)
	{
		M(bug(ID "cannot get module \"%s\"\n",usb_name));
		return B_ERROR;
	}
	
	dev_table_lock = create_sem(1,"gempc430:devtab_lock");
	for(i=0 ; i<PDEV_MAX ; i++)
		pdev[i] = NULL;
	gempc430_name[0] = NULL;
	
	usb->register_driver("gempc430", supported_devices, 1, NULL);
	usb->install_notify("gempc430", &notify_hooks);
	M(bug(ID "init_driver() done\n"));
	return B_OK;
}

_EXPORT void uninit_driver(void)
{
	int i;

	F(bug(ID "uninit_driver()\n"));
	
	usb->uninstall_notify("gempc430");

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
	for (i=0 ; gempc430_name[i] ; i++)
		free((char*) gempc430_name[i]);
	M(bug(ID "uninit_driver() done\n"));
}

_EXPORT const char** publish_devices()
{
	int i,j;
	
	F(bug(ID "publish_devices()\n"));
	
	for(i=0 ; gempc430_name[i] ; i++)
		free((char *)gempc430_name[i]);
	
	acquire_sem(dev_table_lock);
	for(i=0,j=0 ; i<PDEV_MAX ; i++)
	{
		if (pdev[i] && pdev[i]->active)
		{
			if ((gempc430_name[j] = (char *)malloc( strlen(kDeviceBaseName)+1+3+1 )))
			{
				sprintf((char *)gempc430_name[j], kDeviceBaseName "/%u", i);
				M(bug(ID "publishing %s\n", gempc430_name[j]));
				j++;
			}
		}
	}
	release_sem(dev_table_lock);
	gempc430_name[j] = NULL;
	return gempc430_name;
}

_EXPORT device_hooks* find_device(const char* name)
{
	F(bug(ID "find_device(%s)\n", name));
	return &gempc430_devices;
}

static status_t device_open(const char *dname, uint32 flags, void **cookie)
{
	int i;
	int current;
	int32 open_mask;
	user_info *info;
	status_t err = B_ERROR;
	
	F(bug(ID "device_open(""%s"", 0x%08lx, 0x%08lx)\n", dname, flags, cookie));
	
	i = atoi(dname + strlen(kDeviceBaseName) + 1);
	
	if ((err = acquire_sem(dev_table_lock)) != B_OK)
		return err;
	
	if (pdev[i])
	{ /* This device exists... */
		*cookie = NULL; /* Set the cookie to NULL in case of error */
		if (pdev[i]->active == 0)
		{
			M(bug(ID "device is going offline, cannot open.\n"));
			err = EBADF;
			goto error;
		}

		pdev[i]->open_mode |= (open_mask = M_RDWR_MASK);

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
			char name[32];
			pdev[i]->timeout = M_IO_TIMEOUT_SHORT;
			pdev[i]->wr_dev_status = 0;
			pdev[i]->wr_transfert_pending = false;

			sprintf(name, "gempc430:%d:rd_lock", i);
			pdev[i]->rd_lock = create_sem(1, name);

			sprintf(name, "gempc430:%d:wr_lock", i);
			pdev[i]->wr_lock = create_sem(1, name);
	
			sprintf(name, "gempc430:%d:rd_done", i);
			pdev[i]->rd_done = create_sem(0, name);
						
			sprintf(name, "gempc430:%d:wr_done", i);
			pdev[i]->wr_done = create_sem(0, name);
		}


		/* Set the cookie */
		*cookie = (void *)info;
	}
	
error_already_opened:
error:
	release_sem(dev_table_lock);
	return err;
}


static status_t device_free(void *cookie)
{
	status_t s;
	gempc430dev *p = ((user_info *)cookie)->dev;
	int32 open_mode = ((user_info *)cookie)->open_mode;
	
	F(bug(ID "device_free(0x%08lx)\n", cookie));

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

static status_t device_close(void *cookie)
{
// TODO: Should I cancel the current transfert here?
	gempc430dev *p = ((user_info *)cookie)->dev;
	F(bug(ID "device_close(0x%08lx)\n", cookie));
	return B_OK;
}


void cb_rd_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{ /* !!! Here, we receive a "gempc430dev *" */
	gempc430dev *p = ((gempc430dev *)cookie);
	p->rd_len = actual_len;
	p->rd_dev_status = status;
	release_sem(p->rd_done);
}

void cb_wr_notify(void *cookie, uint32 status, void *data, uint32 actual_len)
{ /* !!! Here, we receive a "gempc430dev *" */
	gempc430dev *p = ((gempc430dev *)cookie);
	p->wr_len = actual_len;
	p->wr_dev_status = status;
	p->wr_transfert_pending = false;
	release_sem(p->wr_done);
}


static status_t device_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	gempc430dev *p = ((user_info *)cookie)->dev;
	int sz = *count;
	status_t s = B_OK;
	bool canceled = false;
	
	F(bug(ID "device_read(0x%08lx (dev=0x%08lx), %ld, 0x%08lx, %ld)\n", cookie, p, (long)pos, buf, (long)*count));
	
	if (p->active == 0)
		return EIO;

	*count = 0;
	s = acquire_sem_etc(p->rd_lock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
	if (s != B_OK)
		return s;
	
	while ((sz > 0) && (!canceled))
	{
		p->rd_len = 0;
		p->rd_dev_status = 0;
		if ((s = usb->queue_bulk(p->in, p->rd_addr, TMP_READ_BUF_SIZE, cb_rd_notify, (void *)p)) != B_OK)
		{
			E(bug(ID "dev 0x%08x status %d\n", cookie, s));
			break;
		}
		
		do
		{
			s = acquire_sem_etc(p->rd_done, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, p->timeout);
			if (s == B_TIMED_OUT)
			{
				canceled = true;
				usb->cancel_queued_transfers(p->in);
				E(bug(ID "usb->cancel_queued_transfers() called\n"));
			}
		} while (s == B_TIMED_OUT);
		
		
		if (p->rd_dev_status)
		{ /* There was an error */
			E(bug(ID "device_read: queue_bulk() error %d\n", p->rd_dev_status));
			s = EIO;
			break;
		}
		
		if (p->rd_len == 0)
			break; /* Nothing read */
		
		/* Move the data in the user's buffer */
		memcpy(buf, p->rd_addr, p->rd_len);
		(uint8 *)buf	+= p->rd_len;
		*count			+= p->rd_len;
		sz				-= p->rd_len;
		
		if (p->rd_len < TMP_READ_BUF_SIZE)
			break; /* Nothing else to read */
	}
	
	M(bug(ID "%lu bytes read.\n", (uint32)*count));
	
	if (canceled)
	{
		*count = 0;
		s = EIO;
	}
	
	release_sem(p->rd_lock);
	return s;
}

static status_t device_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	const user_info *uinfo = (user_info *)cookie;
	gempc430dev *p = uinfo->dev;
	int sz = *count;
	status_t s = B_OK;
	
	F(bug(ID "device_write(0x%08lx, %ld, 0x%08lx, %ld)\n", cookie, (long)pos, buf, (long)*count));
	
	if (p->active == 0)
		return EIO;

	/* Nothing to send */
	if (sz <= 0)
		return B_OK;
	
	*count = 0;
	s = acquire_sem_etc(p->wr_lock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, B_INFINITE_TIMEOUT);
	if (s != B_OK)
		return s;

	if (p->wr_transfert_pending == true)
	{ // the previous transfert is not finished
		E(bug(ID "a transfert is still pending\n"));
		release_sem(p->wr_lock);
		return B_OK;
	}

	// Usually return B_WOULD_BLOCK unless we were in 'wr_transfert_pending'
	// In this case the wr_done is acquired which is what we want
	acquire_sem_etc(p->wr_done, 1, B_RELATIVE_TIMEOUT, 0);

	while (sz > 0)
	{
		/* Nb of bytes left to copy */
		const size_t size_to_copy = (sz > TMP_WRITE_BUF_SIZE ? TMP_WRITE_BUF_SIZE : sz);

		/* Copy the data, and send them to USB stack */
		memcpy(p->wr_addr, buf, size_to_copy);
		M(bug(ID "queue_bulk(%08lx, %lu)\n", p->wr_addr, size_to_copy));

		p->wr_transfert_pending = true;
		p->wr_dev_status = 0;
		if ((s = usb->queue_bulk(p->out, p->wr_addr, size_to_copy, cb_wr_notify, (void *)p)) != B_OK)
		{	E(bug(ID "dev 0x%08x status %d\n", cookie, s));
			p->wr_transfert_pending = false;
			break;
		}

		s = acquire_sem_etc(p->wr_done, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, p->timeout);

		if (p->wr_dev_status)
		{ /* there was an error, and the semaphore was released */
			E(bug(ID "error in queue_bulk() = %08lx\n", p->wr_dev_status));
			*count += p->wr_len;
			s = B_OK; /* As we read the counter, tell there is no error */
			break;
		}
		else if (s != B_OK)
		{ 
			E(bug(ID "acquire_sem_etc() %08lx (%s)\n", s, strerror(s)));

			// Don't cancel the current transfert and report that the whole block was successfully sent
			s = B_OK; /* And tell there was no error */
			*count	+= size_to_copy;
			break;
		}

		/* Update counters */
		(uint8 *)buf	+= size_to_copy;
		*count			+= size_to_copy;
		sz				-= size_to_copy;
	}
	
	M(bug(ID "%lu bytes written.\n", (uint32)*count));	
	release_sem(p->wr_lock);
	return s;
}


static status_t device_control(void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_DEV_INVALID_IOCTL;
}
