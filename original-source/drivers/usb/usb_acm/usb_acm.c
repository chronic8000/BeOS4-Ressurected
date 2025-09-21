#include 	<OS.h>
#include 	<Drivers.h>
#include 	<KernelExport.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include 	<sys/stat.h>
#include	<termios.h>
#include	<tty/ttylayer.h>
#include	<byteorder.h>

#include 	<USB.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;

/****************************************************************/
/* Defines														*/
/****************************************************************/

#define kDeviceBaseName	"ports/usb_acm"
#define PDEV_MAX 32
#define TMP_BUF_SIZE		B_PAGE_SIZE
#define DEVICE_ID_MAX_LEN	50
#define DRIVER_NAME	"usb_acm"

/* using ... in macros is a GCC extension */
#if DEBUG
# if __GNUC__
#  define USB_ACM_DRIVER_PRINT(f...)		dprintf(DRIVER_NAME " : " f)
# else
#  warn debugging output is broken because this is not GCC
#  define USB_ACM_DRIVER_PRINT
# endif
#else /* DEBUG */
#define USB_ACM_DRIVER_PRINT
#endif

#if FULL_DEBUG
# if __GNUC__
#  define USB_ACM_DRIVER_FULL_PRINT(f...)		dprintf(DRIVER_NAME " : " f)
# else
#  warn debugging output is broken because this is not GCC
#  define USB_ACM_DRIVER_FULL_PRINT
# endif
#else /* DEBUG */
#define USB_ACM_DRIVER_FULL_PRINT
#endif

# if __GNUC__
#define USB_ACM_DRIVER_KPRINT(f...) 			dprintf(DRIVER_NAME " : " f)
# else
#  warn debugging output is broken because this is not GCC
#  define USB_ACM_DRIVER_KPRINT
# endif

/*
 * Requests.
 */

#define USB_RT_ACM		(USB_TYPE_CLASS | USB_RECIP_INTERFACE)

#define ACM_REQ_COMMAND		0x00
#define ACM_REQ_RESPONSE	0x01
#define ACM_REQ_SET_FEATURE	0x02
#define ACM_REQ_GET_FEATURE	0x03
#define ACM_REQ_CLEAR_FEATURE	0x04

#define ACM_REQ_SET_LINE	0x20
#define ACM_REQ_GET_LINE	0x21
#define ACM_REQ_SET_CONTROL	0x22
#define ACM_REQ_SEND_BREAK	0x23

/*
 * IRQs.
 */

#define ACM_IRQ_NETWORK		0x00
#define ACM_IRQ_LINE_STATE	0x20

/*
 * Output control lines.
 */

#define ACM_CTRL_DTR		0x01
#define ACM_CTRL_RTS		0x02

/*
 * Input control lines and line errors.
 */

#define ACM_CTRL_DCD		0x01
#define ACM_CTRL_DSR		0x02
#define ACM_CTRL_BRK		0x04
#define ACM_CTRL_RI		0x08

#define ACM_CTRL_FRAMING	0x10
#define ACM_CTRL_PARITY		0x20
#define ACM_CTRL_OVERRUN	0x40

/*
 * Line speed and caracter encoding.
 */

typedef struct acm_line
{
	uint32 speed;
	uint8 stopbits;
	uint8 parity;
	uint8 databits;
} __attribute__ ((packed)) acm_line;


/****************************************************************/
/* Types														*/
/****************************************************************/

/****************************************************************/
/* Static Prototypes											*/
/****************************************************************/

static status_t usb_acm_open(const char *name, uint32 flags, void **cookie);
static status_t usb_acm_free (void *cookie);
static status_t usb_acm_close(void *cookie);
static status_t usb_acm_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t usb_acm_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t usb_acm_write(void *cookie, off_t pos, const void *buf, size_t *count);

static bool	usb_acm_service( struct tty *, struct ddrover *, uint);

/****************************************************************/
/* Global Variables												*/
/****************************************************************/

typedef struct usb_acm_port
{
	struct tty	tty;
} usb_acm_port;

typedef struct usb_acm_dev
{
	int					active;
	int32				open;
	usb_device			dev;
	usb_pipe			pipe_control;
	usb_pipe			pipe_in;
	usb_pipe			pipe_out;
	
	acm_line			line;
	
	area_id				area_id_in;
	void				*area_in;
	
	area_id				area_id_out;
	void				*area_out;
	
	area_id				area_id_interrupt;
	void				*area_interrupt;

	size_t				actual_len_in;
	uint32				dev_status_in;
	
	size_t				actual_len_out;
	uint32				dev_status_out;

	uint32				actual_len_interrupt;
	uint32				dev_status_interrupt;
	
	sem_id				done_in;
	sem_id				done_out;
	
	uint16				ctrlout;				/* output control lines (DTR, RTS) */
	
	struct tty			*tty;
} usb_acm_dev;

typedef struct usb_acm_user_info
{
	struct ttyfile		file;
	struct usb_acm_dev	*dev;
} usb_acm_user_info;

static device_hooks usb_acm_devices =
{
	&usb_acm_open, 			
	&usb_acm_close, 			
	&usb_acm_free,			
	&usb_acm_control, 		
	&usb_acm_read,			
	&usb_acm_write			 
};

static struct ddomain		usb_acm_dd;
static struct usb_acm_port	usb_acm_tab;

static char *usb_name = B_USB_MODULE_NAME;

static usb_module_info	*usb = NULL;
static tty_module_info	*tm = NULL;

static sem_id dev_table_lock = -1;
static struct usb_acm_dev *pdev[PDEV_MAX];

usb_support_descriptor supported_devices[] =
{
	{ 2, 0, 0, 0, 0 }
};

static const char *usb_acm_name[PDEV_MAX+1]={NULL};

/*
 * Functions for ACM control messages.
 */

static int usb_acm_ctrl_msg(struct usb_acm_dev *acm, uint8 request, uint16 value, void *buf, size_t len)
{
	size_t act = 0;
	int retval = 0;
	status_t status = usb->send_request(acm->dev, USB_REQTYPE_CLASS|USB_REQTYPE_INTERFACE_OUT,request,value,0,len,buf,&act);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_ctrl_msg: rq: 0x%02x val: %#x len: %#lx %lx %lx\n", request, value, len,act,status);
	return retval < 0 ? retval : 0;
}

#define acm_set_control(acm, control)	usb_acm_ctrl_msg(acm, ACM_REQ_SET_CONTROL, control, NULL, 0L)
#define acm_set_line(acm, line)			usb_acm_ctrl_msg(acm, ACM_REQ_SET_LINE, 0, line, sizeof(struct acm_line))
#define acm_send_break(acm, ms)			usb_acm_ctrl_msg(acm, ACM_REQ_SEND_BREAK, ms, NULL, 0L)

#pragma mark -

void usb_acm_device_notify(void *cookie, uint32 status, void *data, uint32 actual_len);

static void add_device(usb_device dev, usb_configuration_info *conf)
{
	unsigned int i = 0;
	uint16 ii=0;
	char name[32];
	struct usb_acm_dev *p = NULL;
	struct usb_interface_list *comm_interface = NULL;
	struct usb_interface_list *data_interface = NULL;
	bool communication_ok = false;
	bool data_ok = false;
	
	USB_ACM_DRIVER_FULL_PRINT("add_device\n");
	USB_ACM_DRIVER_FULL_PRINT("add_device : conf->interface_count = %ld\n",conf->interface_count);

	// checking the interfaces
	for (ii=0;ii<conf->interface_count;ii++)
	{
		struct usb_interface_list *pt_interface = &conf->interface[ii];
		
		if ((pt_interface->active->descr->interface_class == 2) && (pt_interface->active->descr->interface_subclass == 2) && (pt_interface->active->descr->num_endpoints == 1))
		{
			USB_ACM_DRIVER_PRINT("add_device : Communication Interface found\n");
			communication_ok = true;
			comm_interface = pt_interface;
		}
		if ((pt_interface->active->descr->interface_class == 10) && ((pt_interface->active->descr->interface_subclass == 2) || (pt_interface->active->descr->interface_subclass == 0)) && (pt_interface->active->descr->num_endpoints == 2))
		{
			USB_ACM_DRIVER_PRINT("add_device : Data Interface found\n");
			data_ok = true;
			data_interface = pt_interface;
		}

		if ((data_ok) && (communication_ok))
			break;
	}
	
	if ((data_ok) && (communication_ok))
	{
		acquire_sem(dev_table_lock);
		for(i=0;i<PDEV_MAX;i++)
		{
			if(!pdev[i])
			{
				if ((p = (struct usb_acm_dev *)malloc(sizeof(struct usb_acm_dev))))
				{
					p->dev = dev;
					p->active = 1;
					p->open = 0;
					sprintf(name,"usb_acm:%d:done_in",i);
					p->done_in = create_sem(0,name);
					sprintf(name,"usb_acm:%d:done_out",i);
					p->done_out = create_sem(0,name);
					p->ctrlout = 3;
					p->tty = NULL;
					p->area_id_in = create_area("usb_acm:area_in", (void **)&(p->area_in), 
										 B_ANY_KERNEL_ADDRESS, TMP_BUF_SIZE, B_CONTIGUOUS, 
										 B_READ_AREA | B_WRITE_AREA);
					USB_ACM_DRIVER_FULL_PRINT("add_device : area_in : %p %ld\n",p->area_in,p->area_id_in);
					p->area_id_out = create_area("usb_acm:area_out", (void **)&(p->area_out), 
										 B_ANY_KERNEL_ADDRESS, TMP_BUF_SIZE, B_CONTIGUOUS, 
										 B_READ_AREA | B_WRITE_AREA);
					USB_ACM_DRIVER_FULL_PRINT("add_device : area_out : %p %ld\n",p->area_out,p->area_id_out);
					p->area_id_interrupt = create_area("usb_acm:area_interrupt", (void **)&(p->area_interrupt), 
										 B_ANY_KERNEL_ADDRESS, TMP_BUF_SIZE, B_CONTIGUOUS, 
										 B_READ_AREA | B_WRITE_AREA);
					USB_ACM_DRIVER_FULL_PRINT("add_device : area_interrupt : %p %ld\n",p->area_interrupt,p->area_id_interrupt);

					usb->set_configuration(dev,conf);

					p->line.speed = B_HOST_TO_LENDIAN_INT32(9600);
					p->line.databits = 8;
					p->line.stopbits = 2;
					p->line.parity = 0;
					acm_set_line(p, &p->line);

					// looking for endpoints
					p->pipe_control = comm_interface->active->endpoint[0].handle;
					p->pipe_out = data_interface->active->endpoint[0].handle;
					p->pipe_in = data_interface->active->endpoint[1].handle;
					
					USB_ACM_DRIVER_PRINT("add_device : ACM USB Modem found\n");
					USB_ACM_DRIVER_PRINT("add_device : endpoints %p / %p / %p\n", p->pipe_control,p->pipe_in,p->pipe_out);
				
					acm_set_control(p, p->ctrlout);
					pdev[i] = p;
				}
				break;
			}
		} 
		release_sem(dev_table_lock);
	}
}

static void remove_device(usb_acm_dev *p)
{
	int i = 0;
	USB_ACM_DRIVER_FULL_PRINT("remove_device\n");

	for(i=0;i<PDEV_MAX;i++)
		if(pdev[i] == p)
			break;
	if (i == PDEV_MAX)
	{
		return;
	}

	delete_area(p->area_id_in);
	delete_area(p->area_id_out);
	delete_area(p->area_id_interrupt);
	delete_sem(p->done_in);
	delete_sem(p->done_out);
	free((void *)p);
	pdev[i] = 0;
}

static struct usb_acm_dev *find_usb_acm_device(struct tty *file)
{
	uint16 i=0;
	for(i=0;i<PDEV_MAX;i++)
		if (pdev[i])
		{
			if (pdev[i]->tty == file)
			{
				return pdev[i];
			}
		}
	return NULL;
}

#pragma mark -

static status_t usb_acm_device_added(usb_device dev, void **cookie)
{
	const usb_configuration_info *conf = NULL;
	const usb_device_descriptor *desc = usb->get_device_descriptor(dev);
	uint16 i=0;
	
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added\n");
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added : vendor_id = %x\n",desc->vendor_id);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added : device_class = %x\n",desc->device_class);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added : device_subclass = %x\n",desc->device_subclass);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added : device_protocol = %x\n",desc->device_protocol);
	
	if((desc->device_class == 0x02) && (desc->device_subclass == 0x00) && (desc->device_protocol == 0x00))
	{
		while((conf = usb->get_nth_configuration(dev,i++)))
		{
			USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_added : conf->interface_count = %ld\n",conf->interface_count);
			if(conf->interface_count == 2)
			{
				add_device(dev, (usb_configuration_info *)conf);
			   	*cookie = (void *)dev;
			}
		}
		return B_OK;
	}
	else
	{
		USB_ACM_DRIVER_KPRINT("device has no config 0\n");
	}
	return B_ERROR;
}

static status_t usb_acm_device_removed(void *cookie)
{
	usb_device dev = (usb_device) cookie;
	int i;

	USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_removed\n");
	
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++)
	{
		if(pdev[i] && (pdev[i]->dev == dev))
		{
			USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_removed %d\n",i);
			pdev[i]->active = 0;
			if(pdev[i]->open == 0)
			{
				remove_device(pdev[i]);
			}
			else
			{
//				M(bug(ID "device /dev/" kDeviceBaseName "/%d still open -- marked for removal\n",i));
			}
		}
	}
	release_sem(dev_table_lock);
	return B_OK;
}

void usb_acm_notify_interrupt(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	if (p)
	{
		uint32 i=0;
		USB_ACM_DRIVER_KPRINT("usb_acm_notify_interrupt : %p %lx %p %ld\n",cookie,status,data,actual_len);
		p->dev->actual_len_interrupt = actual_len;
		p->dev->dev_status_interrupt = status;
	}
}

void usb_acm_device_notify_in(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	if (p)
	{
		p->dev->actual_len_in = actual_len;
		p->dev->dev_status_in = status;
		USB_ACM_DRIVER_FULL_PRINT("usb_acm_device_notify_in : %p %lx %p %ld\n",cookie,status,data,actual_len);
		release_sem(p->dev->done_in); /* It is called from a kernel thread */
	}
}

void usb_acm_device_notify_out(void *cookie, uint32 status, void *data, uint32 actual_len)
{
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	if (p)
	{
		p->dev->actual_len_out = actual_len;
		p->dev->dev_status_out = status;
		USB_ACM_DRIVER_FULL_PRINT("usb_acm:usb_acm_device_notify_out : %p %lx %p %ld\n",cookie,status,data,actual_len);
		release_sem(p->dev->done_out); /* It is called from a kernel thread */
	}
}

static int32 usb_acm_device_thread(void *data)
{
	struct usb_acm_user_info *p = (struct usb_acm_user_info *)data;
	status_t error = 0;
	while(true)
	{
		p->dev->actual_len_in = 0;
		p->dev->dev_status_in = 0;
		if ((error = usb->queue_bulk(p->dev->pipe_in, p->dev->area_in, TMP_BUF_SIZE, usb_acm_device_notify_in, (void *)p)) != B_OK)
		{
			USB_ACM_DRIVER_KPRINT("usb_acm_device_thread : queue_bulk error : %lx\n",error);
			return 0;
		}
	
		if ((error = acquire_sem_etc(p->dev->done_in, 1, B_CAN_INTERRUPT, 0)) != B_OK)
		{
			USB_ACM_DRIVER_KPRINT("usb_acm_device_thread : acquire_sem_etc : error %08lx (%s)\n", error, strerror(error));
			return 0;
		}
	
		if (p->dev->dev_status_in)
		{
			USB_ACM_DRIVER_KPRINT("usb_acm_device_thread : dev_status error !!!\n");
			error = EIO;
			return 0;
		}
			
		if (p->dev->actual_len_in > 0)
		{
			size_t i = 0;
			struct ddrover *rover = NULL;
			
			unless (rover = (*tm->ddrstart)(NULL))
			{
				USB_ACM_DRIVER_KPRINT("usb_acm_device_thread : ddrstart problem !!!\n");
				return (ENOMEM);
			}
			(*tm->ttyilock)(&usb_acm_tab.tty, rover, true);
			for (i=0;i<p->dev->actual_len_in;i++)
			{
				(*tm->ttyin)(&usb_acm_tab.tty, rover, ((uint8 *)p->dev->area_in)[i]);
			}
			(*tm->ttyilock)(&usb_acm_tab.tty, rover, false);
			(*tm->ddrdone)(rover);
		}
	}
	return 0;
}

const uint32 acm_tty_speed[] = { 
	0, 50, 75, 110, 134, 150, 200, 300, 600,
	1200, 1800, 2400, 4800, 9600, 19200, 38400,
	57600, 115200, 230400, 460800, 500000, 576000,
	921600, 1000000, 1152000, 1500000, 2000000,
	2500000, 3000000, 3500000, 4000000
};

#define CBAUDEX 	0010000
#define CMSPAR		010000000000

static void usb_acm_setmodes( struct usb_acm_dev *info)
{
	struct acm_line newline;
	struct termios termios = info->tty->t;
	int newctrl = info->ctrlout;

	newline.speed = B_HOST_TO_LENDIAN_INT32(acm_tty_speed[(termios.c_cflag & CBAUD & ~CBAUDEX) + (termios.c_cflag & CBAUDEX ? 15 : 0)]);
	newline.stopbits = termios.c_cflag & CSTOPB ? 2 : 0;
	newline.parity = termios.c_cflag & PARENB ?
		(termios.c_cflag & PARODD ? 1 : 2) + (termios.c_cflag & CMSPAR ? 2 : 0) : 0;
	newline.databits = acm_tty_speed[(termios.c_cflag & CSIZE) >> 4];
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_setmodes : speed = %ld\n",newline.speed);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_setmodes : stopbits = %d\n",newline.stopbits);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_setmodes : parity = %d\n",newline.parity);
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_setmodes : databits = %d\n",newline.databits);
	
	if (!newline.speed)
	{
		newline.speed = info->line.speed;
		newctrl &= ~ACM_CTRL_DTR;
	}
	else
		newctrl |=  ACM_CTRL_DTR;

	if (newctrl != info->ctrlout)
		acm_set_control(info, info->ctrlout = newctrl);

	if (memcmp(&info->line, &newline, sizeof(struct acm_line)))
	{
		memcpy(&info->line, &newline, sizeof(struct acm_line));
		acm_set_line(info, &info->line);
	}
}

static usb_notify_hooks notify_hooks = 
{
	&usb_acm_device_added,
	&usb_acm_device_removed
};

#pragma mark -

_EXPORT status_t init_hardware(void)
{
	USB_ACM_DRIVER_FULL_PRINT("init_hardware\n");
	return B_OK;
}                   

_EXPORT status_t init_driver(void)
{
	int i = 0;
	status_t error = 0;
	
	USB_ACM_DRIVER_FULL_PRINT("init_driver\n");
	
	error = get_module(B_TTY_MODULE_NAME,(module_info**) &tm);
	if(error != B_OK)
	{
		USB_ACM_DRIVER_KPRINT("init_driver : (%lx) cannot get module \"%s\"\n",error,B_TTY_MODULE_NAME);
		return B_ERROR;
	}

	error = get_module(usb_name,(module_info**) &usb);
	if(error != B_OK)
	{
		USB_ACM_DRIVER_KPRINT("init_driver : (%lx) cannot get module \"%s\"\n",error,usb_name);
		return B_ERROR;
	}

	dev_table_lock = create_sem(1,"usb_acm:devtab_lock");
	for(i=0 ; i<PDEV_MAX ; i++)
		pdev[i] = NULL;
	usb_acm_name[0] = NULL;
		
	if (tm)
		(*tm->ttyinit)(&usb_acm_tab.tty, true);
	else
	{
		return B_ERROR;
	}

	load_driver_symbols("usb_acm");
	load_driver_symbols("tty");
	
	usb->register_driver("usb_acm",supported_devices,1, NULL);
	usb->install_notify("usb_acm",&notify_hooks);
	return B_OK;
}

_EXPORT void uninit_driver(void)
{
	int i;
	
	USB_ACM_DRIVER_FULL_PRINT("uninit_driver\n");
	
	usb->uninstall_notify("usb_acm");
	
	acquire_sem(dev_table_lock);
	for(i=0;i<PDEV_MAX;i++)
	{
		if(pdev[i])
		{
			remove_device(pdev[i]);
		}
	}
	release_sem_etc(dev_table_lock, 1, B_DO_NOT_RESCHEDULE); /* No need to resched, here */
	delete_sem(dev_table_lock);
	for(i=0;usb_acm_name[i];i++)
		free((char*) usb_acm_name[i]);
	put_module(usb_name);
	put_module(B_TTY_MODULE_NAME);
}

_EXPORT const char** publish_devices()
{
	int i,j;

	USB_ACM_DRIVER_FULL_PRINT("publish_devices\n");

	for(i=0;usb_acm_name[i];i++)
		free((char *) usb_acm_name[i]);
	
	acquire_sem(dev_table_lock);
	for(i=0,j=0;i<PDEV_MAX;i++)
	{
		if(pdev[i] && pdev[i]->active)
		{
			if((usb_acm_name[j] = (char *) malloc(strlen(kDeviceBaseName)+4)))
			{
				sprintf((char *)usb_acm_name[j], kDeviceBaseName "_%u", j);
				USB_ACM_DRIVER_FULL_PRINT("publish_devices : publishing %s\n",usb_acm_name[j]);
				j++;
			}
		}
	}
	release_sem(dev_table_lock);
	usb_acm_name[j] = NULL;
		
	return usb_acm_name;
}

_EXPORT device_hooks* find_device(const char* name)
{
	return &usb_acm_devices;
}

#pragma mark -

static status_t usb_acm_open(const char *dname, uint32 flags, void **cookie)
{	
	struct usb_acm_user_info	*user_info = NULL;
	struct ddrover				*r = NULL;
	struct usb_acm_port			*zp = NULL;
	status_t					error = ENOMEM;
	int 						i = -1;
	thread_id					id = -1;

	i = atoi(dname + strlen(kDeviceBaseName) + 1);
	*cookie = 0;

	USB_ACM_DRIVER_FULL_PRINT("usb_acm_open : %d\n",i);

	acquire_sem(dev_table_lock);
	if (pdev[i])
	{
		USB_ACM_DRIVER_FULL_PRINT("usb_acm_open : pdev[%d] = %p %lx\n",i,pdev[i],flags);
		if (pdev[i]->active)
		{
			if ((user_info = malloc( sizeof (struct usb_acm_user_info))))
			{
				memset(user_info,0,sizeof(struct usb_acm_user_info));
				zp = &usb_acm_tab;
				
				// user info
				*cookie = user_info;
				user_info->dev = pdev[i];
				
				// tty stuff
				user_info->file.tty = &zp->tty;
				user_info->file.flags = flags;
				pdev[i]->tty = user_info->file.tty;

				if ((r = (*tm->ddrstart)(0)))
				{
					(*tm->ddacquire)( r, &usb_acm_dd);
					error = (*tm->ttyopen)(&user_info->file, r, usb_acm_service);
					(*tm->ddrdone)(r);
					
					id = spawn_kernel_thread(usb_acm_device_thread,"usb_acm_device_thread",B_NORMAL_PRIORITY,user_info);
					if (id != -1)
						resume_thread(id);
					else
					{
						USB_ACM_DRIVER_KPRINT("usb_acm_open : user_info->dev->sem_thread_id = %lx\n",id);
					}
					user_info->dev->ctrlout = ACM_CTRL_DTR | ACM_CTRL_RTS;
					acm_set_control(user_info->dev, user_info->dev->ctrlout);
					
					usb->queue_interrupt(user_info->dev->pipe_control, user_info->dev->area_interrupt, TMP_BUF_SIZE, usb_acm_notify_interrupt, user_info);
				}
				unless (error == B_OK)
				{
					USB_ACM_DRIVER_KPRINT("usb_acm_open : error2 = %lx\n",error);
					free(user_info);
				}
			}
			else
				USB_ACM_DRIVER_KPRINT("usb_acm_open : error malloc\n");
		}
		else
			USB_ACM_DRIVER_KPRINT("usb_acm_open : pdev[i]->active == 0\n");
	}
	else
		USB_ACM_DRIVER_KPRINT("usb_acm_open : pdev[i] == NULL\n");
	release_sem(dev_table_lock);
	return error;
}

static status_t usb_acm_close(void *cookie)
{
	struct ddrover	*r = NULL;
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	status_t s = B_OK;
	
	USB_ACM_DRIVER_FULL_PRINT("usb_acm_close : %p\n",cookie);
	
	usb->cancel_queued_transfers(p->dev->pipe_in);
	usb->cancel_queued_transfers(p->dev->pipe_out);
	usb->cancel_queued_transfers(p->dev->pipe_control);
	
	unless (cookie)
		return (B_OK);
	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttyclose)(&p->file, r);
	(*tm->ddrdone)( r);
	return s;
}

static status_t usb_acm_free (void *cookie)
{
	struct ddrover	*r = NULL;
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	status_t s = B_OK;
	
	USB_ACM_DRIVER_FULL_PRINT("usb_acm : usb_acm_free : %p\n",cookie);
	unless (cookie)
		return (B_OK);
		
	if ((r = (*tm->ddrstart)( 0)))
	{
		s = (*tm->ttyfree)( &p->file, r);
		(*tm->ddrdone)( r);
	}
	else
		s = ENOMEM;
	free(cookie);
	return (s);
}

static status_t usb_acm_read(void *cookie, off_t pos, void *buf, size_t *count)
{
	status_t s = B_OK;
	struct ddrover	*r = NULL;
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	
	unless (r = (*tm->ddrstart)( 0))
	{
		*count = 0;
		USB_ACM_DRIVER_KPRINT("usb_acm_read : ddrstart problem !!!\n");
		return (ENOMEM);
	}
	if (cookie)
	{
		s = (*tm->ttyread)( cookie, r, buf, count);
	}
	else
	{
		(*tm->ddacquire)( r, &usb_acm_dd);
	}
	(*tm->ddrdone)( r);
	return s;
}

static status_t usb_acm_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	status_t s = B_OK;
	struct usb_acm_user_info *p = (struct usb_acm_user_info *)cookie;
	int sz = *count;
	
	*count = 0;
	while (sz > 0)
	{
		const size_t size_to_copy = (sz>TMP_BUF_SIZE ? TMP_BUF_SIZE : sz);
		memcpy(p->dev->area_out, buf, size_to_copy);

		p->dev->actual_len_out = 0;
		p->dev->dev_status_out = 0;
		if ((s = usb->queue_bulk(p->dev->pipe_out, p->dev->area_out, size_to_copy, usb_acm_device_notify_out, (void *)p)) != B_OK)
		{
			USB_ACM_DRIVER_KPRINT("usb_acm_write : dev %p status %lx\n", cookie, s);
			break;
		}

		if ((s = acquire_sem_etc(p->dev->done_out, 1, B_CAN_INTERRUPT, 0)) != B_OK)
		{
			USB_ACM_DRIVER_KPRINT("usb_acm_write : acquire_sem_etc() %lx (%s)\n", s, strerror(s));
			break;
		}

		(uint8 *)buf += p->dev->actual_len_out;
		*count += p->dev->actual_len_out;
		sz -= p->dev->actual_len_out;
	}
	return s;
}

static status_t usb_acm_control (void *cookie, uint32 msg, void *data, size_t len)
{
	struct ddrover	*r = NULL;
	struct usb_acm_user_info *p = ((struct usb_acm_user_info *)cookie);
	status_t s = B_OK;

	USB_ACM_DRIVER_FULL_PRINT("usb_acm : usb_acm_control : %p %lx %p %ld\n",cookie,msg,data,len);

	unless (cookie)
		return (ENODEV);

	unless (r = (*tm->ddrstart)( 0))
		return (ENOMEM);
	s = (*tm->ttycontrol)(&p->file, r, msg, data, len);
	(*tm->ddrdone)( r);

	return (s);
}

bool usb_acm_service(struct tty *tp, struct ddrover *r, uint com)
{
	struct usb_acm_dev *dev = find_usb_acm_device(tp);
	if (dev)
	{
		USB_ACM_DRIVER_FULL_PRINT("usb_acm : usb_acm_service : 0x%x\n",com);
		switch (com)
		{
			case TTYENABLE:
				(*tm->ttyhwsignal)( tp, r, TTYHWDCD, 0x0);
				(*tm->ttyhwsignal)( tp, r, TTYHWCTS, 0x10);
				dev->ctrlout = ACM_CTRL_DTR | ACM_CTRL_RTS;
				acm_set_control(dev, dev->ctrlout);
				break;
			case TTYDISABLE:
				(*tm->ttyhwsignal)( tp, r, TTYHWDCD, false);
				dev->ctrlout = 0;
				acm_set_control(dev, dev->ctrlout);
				break;
			case TTYISTOP:
				(*tm->ttyhwsignal)( tp, r, TTYHWCTS, false);
				break;
			case TTYIRESUME:
				(*tm->ttyhwsignal)( tp, r, TTYHWCTS, true);
				break;
			case TTYGETSIGNALS:
				(*tm->ttyhwsignal)( tp, r, TTYHWDCD, true);
				(*tm->ttyhwsignal)( tp, r, TTYHWCTS, true);
				(*tm->ttyhwsignal)( tp, r, TTYHWDSR, true);
				(*tm->ttyhwsignal)( tp, r, TTYHWRI, false);
				break;
			case TTYSETMODES:
				usb_acm_setmodes(dev);
				break;
			case TTYOSTART:
			case TTYOSYNC:
			case TTYSETBREAK:
			case TTYCLRBREAK:
			case TTYSETDTR:
			case TTYCLRDTR:
			default:
				break;
		}
		return true;
	}
	return false;
}
