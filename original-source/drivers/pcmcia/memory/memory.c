/*======================================================================

    A general driver for accessing PCMCIA card memory via Bulk
    Memory Services.

    This driver provides the equivalent of /dev/mem for a PCMCIA
    card's attribute and common memory.

    memory_cs.c 1.2 1998/11/06 02:58:48

    The contents of this file are subject to the Mozilla Public
    License Version 1.0 (the "License"); you may not use this file
    except in compliance with the License. You may obtain a copy of
    the License at http://www.mozilla.org/MPL/

    Software distributed under the License is distributed on an "AS
    IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
    implied. See the License for the specific language governing
    rights and limitations under the License.

    The initial developer of the original code is David A. Hinds
    <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
    are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
    
======================================================================*/

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <stdarg.h>
#include <ctype.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
#include <pcmcia/memory.h>
#include <pcmcia/mem_op.h>

#define DEVICE_NR(minor)	((minor)>>4)
#define IS_DIRECT(minor)	(((minor)&8)>>3)
#define REGION_AM(minor)	(((minor)&4)>>2)
#define REGION_NR(minor)	((minor)&7)
#define MINOR_NR(dev,dir,attr,rgn) \
(((dev)<<4)+((dir)<<3)+((attr)<<2)+(rgn))

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
static char *version =
"memory_cs.c 1.50 1998/05/28 22:41:55 (David Hinds)";
#define DEBUG(n, args) do { if (pc_debug>(n)) printk args; } while (0)
#else
#define DEBUG(n, args) do { } while (0)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* 1 = do 16-bit transfers, 0 = do 8-bit transfers */
static int word_width = 1;

/* Speed of memory accesses, in ns */
static int mem_speed = 0;

/*====================================================================*/

/* Maximum number of separate memory devices we'll allow */
#define MAX_DEV		4

/* Maximum number of partitions per memory space */
#define MAX_PART	4

/* Maximum number of outstanding erase requests per socket */
#define MAX_ERASE	8

/* Sector size -- shouldn't need to change */
#define SECTOR_SIZE	512

/* Size of the PCMCIA address space: 26 bits = 64 MB */
#define HIGH_ADDR	0x4000000

static dev_link_t *memory_attach(void);
static void memory_detach(dev_link_t *link);
static void memory_config(dev_link_t *link);
static void memory_release(u_long arg);
static int memory_event(event_t event, int priority,
			event_callback_args_t *args);

/* Each memory region corresponds to a minor device */
typedef struct minor_dev_t {		/* For normal regions */
    region_info_t	region;
    memory_handle_t	handle;
    int			open;
} minor_dev_t;

typedef struct direct_dev_t {		/* For direct access */
    int			flags;
    int			open;
    caddr_t		Base;
    u_int		Size;
    u_long		cardsize;
} direct_dev_t;

typedef struct memory_dev_t {
    dev_node_t		node;
    eraseq_handle_t	eraseq_handle;
    eraseq_entry_t	eraseq[MAX_ERASE];
    struct wait_queue	*erase_pending;
    direct_dev_t	direct;
    minor_dev_t		minor[2*MAX_PART];
} memory_dev_t;

#define MEM_WRPROT	1

static dev_info_t dev_info = "memory_cs";
static dev_link_t *dev_table[MAX_DEV] = { NULL, /* ... */ };

static cs_client_module_info *	cs;
static ds_module_info *		ds;
#define CardServices		cs->_CardServices
#define add_timer		cs->_add_timer
#define register_pccard_driver	ds->_register_pccard_driver
#define unregister_pccard_driver ds->_unregister_pccard_driver

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err;
    err.func = func; err.retcode = ret;
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    memory_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

======================================================================*/

static dev_link_t *memory_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    memory_dev_t *dev;
    eraseq_hdr_t eraseq_hdr;
    int i, ret;
    
    DEBUG(0, ("memory_attach()\n"));

    for (i = 0; i < MAX_DEV; i++)
	if (dev_table[i] == NULL) break;
    if (i == MAX_DEV) {
	printk("memory_cs: no devices available\n");
	return NULL;
    }
    
    /* Create new memory card device */
    link = malloc(sizeof(struct dev_link_t));
    memset(link, 0, sizeof(struct dev_link_t));
    dev_table[i] = link;
    link->release.function = &memory_release;
    link->release.data = (u_long)link;
    
    dev = malloc(sizeof(struct memory_dev_t));
    memset(dev, 0, sizeof(memory_dev_t));
    init_waitqueue(&dev->erase_pending);
    link->priv = dev;

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &memory_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	memory_detach(link);
	return NULL;
    }

    for (i = 0; i < MAX_ERASE; i++)
	dev->eraseq[i].State = ERASE_IDLE;
    eraseq_hdr.QueueEntryCnt = MAX_ERASE;
    eraseq_hdr.QueueEntryArray = dev->eraseq;
    dev->eraseq_handle = (void *)link->handle;
    ret = CardServices(RegisterEraseQueue, &dev->eraseq_handle, &eraseq_hdr);
    if (ret != 0) {
	cs_error(link->handle, RegisterEraseQueue, ret);
	dev->eraseq_handle = NULL;
	memory_detach(link);
	return NULL;
    }
    
    return link;
} /* memory_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void memory_detach(dev_link_t *link)
{
    memory_dev_t *dev;
    int nd;

    DEBUG(0, ("memory_detach(0x%p)\n", link));
    
    /* Verify device address */
    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    if (nd == MAX_DEV)
	return;

    if (link->state & DEV_CONFIG) {
	memory_release((u_long)link);
	if (link->state & DEV_STALE_CONFIG) {
	    link->state |= DEV_STALE_LINK;
	    return;
	}
    }

    dev = (memory_dev_t *)link->priv;
    if (dev->eraseq_handle)
	CardServices(DeregisterEraseQueue, dev->eraseq_handle);
    if (link->handle)
	CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free bits */
    dev_table[nd] = NULL;
    free(dev);
    free(link);
    
} /* memory_detach */

/*======================================================================

    Figure out the size of a simple SRAM card
    
======================================================================*/

static void get_size(dev_link_t *link, direct_dev_t *direct)
{
    modwin_t mod;
    memreq_t mem;
    u_char buf[26];
    int s, t, ret;

    mod.Attributes = WIN_ENABLE | WIN_MEMORY_TYPE_CM;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS)
	cs_error(link->handle, ModifyWindow, ret);

    /* Look for wrap-around or dead end */
    mem.Page = 0;
    for (s = 12; s < 26; s++) {
	mem.CardOffset = 1<<s;
	CardServices(MapMemPage, link->win, &mem);
	buf[s] = readb(direct->Base);
	writeb(~buf[s], direct->Base);
	for (t = 12; t < s; t++) {
	    mem.CardOffset = 1<<t;
	    CardServices(MapMemPage, link->win, &mem);
	    if (readb(direct->Base) != buf[t]) {
		writeb(buf[t], direct->Base);
		break;
	    }
	}
	if (t < s) break;
	mem.CardOffset = 1<<s;
	CardServices(MapMemPage, link->win, &mem);
	if (readb(direct->Base) != (0xff & ~buf[s])) break;
	writeb(buf[s], direct->Base);
    }

    /* Restore that last byte on wrap-around */
    if (t < s) {
	mem.CardOffset = 1<<t;
	CardServices(MapMemPage, link->win, &mem);
	writeb(buf[t], direct->Base);
    }

    direct->cardsize = (t > 15) ? (1<<t) : 0;
} /* get_size */

static void print_size(u_long sz)
{
    if (sz & 0x03ff)
	printk("%ld bytes", sz);
    else if (sz & 0x0fffff)
	printk("%ld kb", sz >> 10);
    else
	printk("%ld mb", sz >> 20);
}

/*======================================================================

    memory_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    ethernet device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, h, a) \
while ((last_ret=CardServices(last_fn=(fn),h,a))!=0) goto cs_failed
#define CS_CHECK2(fn, h, a, b) \
while ((last_ret=CardServices(last_fn=(fn),h,a,b))!=0) goto cs_failed

static void memory_config(dev_link_t *link)
{
    memory_dev_t *dev;
    minor_dev_t *minor;
    region_info_t region;
    cs_status_t status;
    win_req_t req;
    int nd, i, last_ret, last_fn, attr, ret, nr[2];

    DEBUG(0, ("memory_config(0x%p)\n", link));

    /* Configure card */
    link->state |= DEV_CONFIG;

    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    
    dev = (memory_dev_t *)link->priv;

    /* Allocate a small memory window for direct access */
    if (word_width)
	req.Attributes = WIN_DATA_WIDTH_16;
    else
	req.Attributes = WIN_DATA_WIDTH_8;
    req.Base = req.Size = 0;
    req.AccessSpeed = mem_speed;
    link->win = (window_handle_t)link->handle;
    CS_CHECK(RequestWindow, &link->win, &req);
    /* Get write protect status */
    CS_CHECK(GetStatus, link->handle, &status);
    
    dev->direct.Base = ioremap(req.Base, req.Size);
    dev->direct.Size = req.Size;
    dev->direct.cardsize = 0;

    for (attr = 0; attr < 2; attr++) {
	nr[attr] = 0;
	minor = dev->minor + attr*MAX_PART;
	region.Attributes =
	    (attr) ? REGION_TYPE_AM : REGION_TYPE_CM;
	ret = CardServices(GetFirstRegion, link->handle, &region);
	while (ret == CS_SUCCESS) {
	    minor->region = region;
	    minor++; nr[attr]++;
	    ret = CardServices(GetNextRegion, link->handle, &region);
	}
    }
    
#if __BEOS__
    sprintf(dev->node.dev_name, "bus/pcmcia/memory/mem%d", nd);
#else
    sprintf(dev->node.dev_name, "memory/mem%d", nd);
#endif
    dev->node.major = dev->node.minor = 0;
    link->dev = &dev->node;
    link->state &= ~DEV_CONFIG_PENDING;
    
    printk("memory_cs: mem%d:", nd);
    if ((nr[0] == 0) && (nr[1] == 0)) {
	cisinfo_t cisinfo;
	if ((CardServices(ValidateCIS, link->handle, &cisinfo)
	     == CS_SUCCESS) && (cisinfo.Chains == 0)) {
	    get_size(link, &dev->direct);
	    printk(" anonymous: ");
	    if (dev->direct.cardsize == 0) {
		dev->direct.cardsize = HIGH_ADDR;
		printk("unknown size");
	    } else {
		print_size(dev->direct.cardsize);
	    }
	} else {
	    printk(" no regions found.");
	}
    } else {
	for (attr = 0; attr < 2; attr++) {
	    minor = dev->minor + attr*MAX_PART;
	    if (attr && nr[0] && nr[1])
		printk(",");
	    if (nr[attr])
		printk(" %s", attr ? "attribute" : "common");
	    for (i = 0; i < nr[attr]; i++) {
		printk(" ");
		print_size(minor[i].region.RegionSize);
	    }
	}
    }
    printk("\n");
    return;

cs_failed:
    cs_error(link->handle, last_fn, last_ret);
    memory_release((u_long)link);
    return;
} /* memory_config */

/*======================================================================

    After a card is removed, memory_release() will unregister the 
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void memory_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;
    int nd;

    DEBUG(0, ("memory_release(0x%p)\n", link));

    for (nd = 0; nd < MAX_DEV; nd++)
	if (dev_table[nd] == link) break;
    if (link->open) {
	DEBUG(0, ("memory_cs: release postponed, 'mem%d'"
		  " still open\n", nd));
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    link->dev = NULL;
    if (link->win) {
	memory_dev_t *dev = link->priv;
	iounmap(dev->direct.Base);
	CardServices(ReleaseWindow, link->win);
    }
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	memory_detach(link);
    
} /* memory_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.  A CARD_REMOVAL event
    also sets some flags to discourage the driver from trying
    to talk to the card any more.
    
======================================================================*/

static int memory_event(event_t event, int priority,
			event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;
    memory_dev_t *dev;
    eraseq_entry_t *erase;

    DEBUG(1, ("memory_event(0x%06x)\n", event));
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
	if (link->state & DEV_CONFIG) {
	    link->release.expires = RUN_AT(HZ/20);
	    add_timer(&link->release);
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	memory_config(link);
	break;
    case CS_EVENT_ERASE_COMPLETE:
	erase = (eraseq_entry_t *)(args->info);
	wakeup((struct wait_queue **)&erase->Optional);
	dev = (memory_dev_t *)(link->priv);
	wakeup(&dev->erase_pending);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	/* get_lock(link); */
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	/* free_lock(link); */
	break;
    }
    return 0;
} /* memory_event */

/*======================================================================

    This gets a memory handle for the region corresponding to the
    minor device number.
    
======================================================================*/

static status_t memory_open(const char *name, uint32 flags,
			    void **cookie)
{
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    open_mem_t open;
    int minor, ret;
    
    DEBUG(0, ("memory_open(%s)\n", name));

    if ((strncmp(name, "memory/mem", 10) != 0) ||
	!isdigit(name[10]) || (strlen(name) < 12)) return ENODEV;
    if ((name[11] != 'a') && (name[11] != 'c')) return ENODEV;
    if (strlen(name) == 12)
	minor = MINOR_NR(name[10]-'0', 1, (name[11]=='a'), 0);
    else if ((strlen(name) == 13) && isdigit(name[12]))
	minor = MINOR_NR(name[10]-'0', 0, (name[11]=='a'), name[12]-'0');
    else
	return ENODEV;
    *cookie = (int *)NULL + minor;
    
    link = dev_table[DEVICE_NR(minor)];
    if (!DEV_OK(link))
	return ENODEV;
    
    dev = (memory_dev_t *)link->priv;

    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0)) {
	if (((flags & O_ACCMODE) != O_RDONLY) &&
	    (dev->direct.flags & MEM_WRPROT))
	    return EROFS;
	dev->direct.open++;
    } else {
	minor_dev = &dev->minor[REGION_NR(minor)];
	if (minor_dev->region.RegionSize == 0)
	    return ENODEV;
	if (minor_dev->handle == NULL) {
	    minor_dev->handle = (memory_handle_t)link->handle;
	    open.Attributes = minor_dev->region.Attributes;
	    open.Offset = minor_dev->region.CardOffset;
	    ret = CardServices(OpenMemory, &minor_dev->handle, &open);
	    if (ret != CS_SUCCESS)
		return ENOMEM;
	}
	minor_dev->open++;
    }
    
    link->open++;
    return 0;
} /* memory_open */

/*====================================================================*/

static status_t memory_close(void *cookie)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    
    DEBUG(0, ("memory_close(%d)\n", minor));

    link = dev_table[DEVICE_NR(minor)];
    dev = (memory_dev_t *)link->priv;
    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0)) {
	dev->direct.open--;
    } else {
	minor_dev = &dev->minor[REGION_NR(minor)];
	minor_dev->open--;
	if (minor_dev->open == 0) {
	    CardServices(CloseMemory, minor_dev->handle);
	    minor_dev->handle = NULL;
	}
    }

    link->open--;
    return B_OK;
} /* memory_close */

static status_t memory_free(void *cookie)
{
    return B_OK;
}

/*======================================================================

    Read for character-mode device
    
======================================================================*/

static status_t direct_read(void *cookie, off_t pos,
			    void *data, size_t *count)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    char *buf = data;
    memory_dev_t *dev;
    direct_dev_t *direct;
    size_t size, read, from, nb;
    int ret;
    modwin_t mod;
    memreq_t mem;

    DEBUG(2, ("direct_read(%d, 0x%lx, %ld)\n", minor,
	      (u_long)pos, (u_long)*count));

    if (!DEV_OK(link))
	return ENODEV;
    dev = (memory_dev_t *)link->priv;
    direct = &dev->direct;
    
    /* Boundary checks */
    size = (IS_DIRECT(minor)) ? HIGH_ADDR : direct->cardsize;
    if (pos >= size)
	return 0;
    if (*count > size - pos)
	*count = size - pos;

    mod.Attributes = WIN_ENABLE;
    mod.Attributes |= (REGION_AM(minor)) ? WIN_MEMORY_TYPE_AM : 0;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, ModifyWindow, ret);
	return EIO;
    }
    
    mem.CardOffset = pos & ~(direct->Size-1);
    mem.Page = 0;
    from = pos & (direct->Size-1);
    for (read = 0; *count > 0; *count -= nb, read += nb) {
	ret = CardServices(MapMemPage, link->win, &mem);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return EIO;
	}
	nb = (from+*count > direct->Size) ? direct->Size-from : *count;
	copy_pc_to_user(buf, direct->Base+from, nb);
	buf += nb;
        from = 0;
	mem.CardOffset += direct->Size;
    }
    *count = read;
    return B_OK;
} /* direct_read */

static status_t memory_read(void *cookie, off_t pos,
			    void *buf, size_t *count)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    memory_dev_t *dev = (memory_dev_t *)link->priv;
    minor_dev_t *minor_dev = &dev->minor[REGION_NR(minor)];
    mem_op_t req;
    int ret;

    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0))
	return direct_read(cookie, pos, buf, count);
    
    DEBUG(2, ("memory_read(%d, 0x%lx, %ld)\n", minor,
	      (u_long)pos, (u_long)*count));
    
    req.Attributes = MEM_OP_BUFFER_USER;
    req.Offset = pos;
    req.Count = *count;
    ret = CardServices(ReadMemory, minor_dev->handle, &req, buf);
    if (ret == CS_SUCCESS)
	return B_OK;
    else
	return B_ERROR;
} /* memory_read */

/*======================================================================

    Erase a memory region.  This is used by the write routine for
    suitably aligned and sized blocks.  It is also used for the
    MEMERASE ioctl().
    
======================================================================*/

static int memory_erase(int minor, u_long f_pos, size_t count)
{
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    memory_dev_t *dev = link->priv;
    minor_dev_t *minor_dev = &dev->minor[REGION_NR(minor)];
    int i, ret;

    /* Find a free erase slot, or wait for one to become available */
    for (;;) {
	for (i = 0; i < MAX_ERASE; i++)
	    if (!ERASE_IN_PROGRESS(dev->eraseq[i].State)) break;
	if (i < MAX_ERASE) break;
	DEBUG(2, ("waiting for erase slot...\n"));
	wsleep(&dev->erase_pending);
	if (signal_pending(current))
	    return EAGAIN;
    }

    /* Queue a new request */
    dev->eraseq[i].State = ERASE_QUEUED;
    dev->eraseq[i].Handle = minor_dev->handle;
    dev->eraseq[i].Offset = f_pos;
    dev->eraseq[i].Size = count;
    ret = CardServices(CheckEraseQueue, dev->eraseq_handle);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, CheckEraseQueue, ret);
	return EIO;
    }

    /* Wait for request to complete */
    init_waitqueue((struct wait_queue **)&dev->eraseq[i].Optional);
    if (ERASE_IN_PROGRESS(dev->eraseq[i].State))
	wsleep((struct wait_queue **)&dev->eraseq[i].Optional);
    if (dev->eraseq[i].State != ERASE_PASSED)
	return EIO;
    return 0;
}

/*======================================================================

    Write for character-mode device
    
======================================================================*/

static status_t direct_write(void *cookie, off_t pos,
			     const void *data, size_t *count)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    const char *buf = data;
    memory_dev_t *dev;
    direct_dev_t *direct;
    size_t size, wrote, to, nb;
    int ret;
    modwin_t mod;
    memreq_t mem;
    
    DEBUG(2, ("direct_write(%d, 0x%lx, %ld)\n", minor,
	      (u_long)pos, (u_long)*count));
    
    if (!DEV_OK(link))
	return ENODEV;
    dev = (memory_dev_t *)link->priv;
    direct = &dev->direct;
    
    /* Check for write protect */
    if (direct->flags & MEM_WRPROT)
	return EROFS;

    /* Boundary checks */
    size = (IS_DIRECT(minor)) ? HIGH_ADDR : direct->cardsize;
    if (pos >= size)
        return ENOSPC;
    if (*count > size - pos)
	*count = size - pos;

    mod.Attributes = WIN_ENABLE;
    mod.Attributes |= (REGION_AM(minor)) ? WIN_MEMORY_TYPE_AM : 0;
    mod.AccessSpeed = 0;
    ret = CardServices(ModifyWindow, link->win, &mod);
    if (ret != CS_SUCCESS) {
	cs_error(link->handle, ModifyWindow, ret);
	return EIO;
    }
    
    mem.CardOffset = pos & ~(direct->Size-1);
    mem.Page = 0;
    to = pos & (direct->Size-1);
    for (wrote = 0; *count > 0; *count -= nb, wrote += nb) {
	ret = CardServices(MapMemPage, link->win, &mem);
	if (ret != CS_SUCCESS) {
	    cs_error(link->handle, MapMemPage, ret);
	    return EIO;
	}
	nb = (to+*count > direct->Size) ? direct->Size-to : *count;
	copy_user_to_pc(direct->Base+to, buf, nb);
	buf += nb;
        to = 0;
	mem.CardOffset += direct->Size;
    }
    *count = wrote;
    return B_OK;
} /* direct_write */

static status_t memory_write(void *cookie, off_t pos,
			     const void *buf, size_t *count)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link = dev_table[DEVICE_NR(minor)];
    memory_dev_t *dev = (memory_dev_t *)link->priv;
    minor_dev_t *minor_dev = &dev->minor[REGION_NR(minor)];
    mem_op_t req;
    int ret;

    if (IS_DIRECT(minor) || (dev->direct.cardsize > 0))
	return direct_write(cookie, pos, buf, count);
    
    DEBUG(2, ("memory_write(%d, 0x%lx, %ld)\n", minor,
	      (u_long)pos, (u_long)count));

    if ((minor_dev->region.BlockSize > 1) &&
	((pos & (minor_dev->region.BlockSize-1)) == 0) &&
	((*count & (minor_dev->region.BlockSize-1)) == 0)) {
	ret = memory_erase(minor, pos, *count);
	if (ret != 0)
	    return ret;
    }
    
    req.Attributes = 0;
    req.Offset = pos;
    req.Count = *count;
    ret = CardServices(WriteMemory, minor_dev->handle, &req, buf);
    if (ret == CS_SUCCESS)
	return B_OK;
    else
	return EIO;
} /* memory_write */

/*======================================================================

    IOCTL calls for getting device parameters.

======================================================================*/

static status_t memory_ioctl(void *cookie,
			     u_int cmd, u_long arg)
{
    int minor = (int *)cookie - (int *)NULL;
    dev_link_t *link;
    memory_dev_t *dev;
    minor_dev_t *minor_dev;
    erase_info_t erase;
    device_geometry *g;
    u_int size;
    int ret = 0;

    link = dev_table[DEVICE_NR(minor)];
    if (!DEV_OK(link))
	return ENODEV;
    dev = (memory_dev_t *)link->priv;
    minor_dev = &dev->minor[REGION_NR(minor)];

    switch (cmd) {
    case B_GET_GEOMETRY:
	g = (device_geometry *)arg;
	if (dev->direct.cardsize)
	    g->sectors_per_track = dev->direct.cardsize/SECTOR_SIZE;
	else if (IS_DIRECT(minor))
	    g->sectors_per_track = HIGH_ADDR;
	else
	    g->sectors_per_track = minor_dev->region.RegionSize/SECTOR_SIZE;
	g->cylinder_count = g->head_count = 1;
	g->bytes_per_sector = SECTOR_SIZE;
	g->removable = 1;
	g->read_only = 0;
	g->device_type = B_DISK;
	g->write_once = 0;
	break;
    case B_GET_DEVICE_SIZE:
	if (dev->direct.cardsize)
	    *(size_t *)arg = dev->direct.cardsize;
	else if (IS_DIRECT(minor))
	    *(size_t *)arg = HIGH_ADDR;
	else
	    *(size_t *)arg = minor_dev->region.RegionSize;
	break;
#if 0
    case BLKFLSBUF:
	if (!suser()) return EACCES;
	if (!(inode->i_rdev)) return EINVAL;
	fsync_dev(inode->i_rdev);
	invalidate_buffers(inode->i_rdev);
	break;
#endif
    case MEMGETINFO:
	if (!IS_DIRECT(minor)) {
	    copy_to_user((region_info_t *)arg, &minor_dev->region,
			 sizeof(struct region_info_t));
	} else ret = EINVAL;
	break;
    case MEMERASE:
	if (!IS_DIRECT(minor)) {
	    copy_from_user(&erase, (erase_info_t *)arg,
			   sizeof(struct erase_info_t));
	    ret = memory_erase(minor, erase.Offset, erase.Size);
	} else ret = EINVAL;
	break;
    default:
	ret = EINVAL;
    }
    
    return ret;
} /* memory_ioctl */

/*====================================================================*/

static device_hooks memory_device_hooks = {
    &memory_open,
    &memory_close,
    &memory_free,
    NULL,
    &memory_read,
    &memory_write
};

/*====================================================================*/

static char *names[MAX_DEV * (MAX_PART+1) + 1];

status_t init_hardware(void)
{
    return B_OK;
}

status_t init_driver(void)
{
    client_reg_t client_reg;
    servinfo_t serv;
    bind_req_t bind;
    int i, ret;
    
    DEBUG(0, ("%s\n", version));
    if (get_module(CS_CLIENT_MODULE_NAME,
		   (struct module_info **)&cs) != B_OK)
	return B_ERROR;
    if (get_module(DS_MODULE_NAME,
		   (struct module_info **)&ds) != B_OK)
	return B_ERROR;
    
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
	printk("memory_cs: Card Services release does not match!\n");
	return B_ERROR;
    }
    register_pccard_driver(&dev_info, &memory_attach, &memory_detach);
    
    return B_OK;
}

void uninit_driver(void)
{
    int i;
    dev_link_t *link;

    DEBUG(0, ("memory_cs: unloading\n"));
    unregister_pccard_driver(&dev_info);
    for (i = 0; i < MAX_DEV; i++) {
	link = dev_table[i];
	if (link) {
	    if (link->state & DEV_CONFIG)
		memory_release((u_long)link);
	    memory_detach(link);
	}
    }
    if (names) {
	for (i = 0; names[i]; i++) free(names[i]);
    }
    if (ds) put_module(DS_MODULE_NAME);
    if (cs) put_module(CS_CLIENT_MODULE_NAME);
}

const char **publish_devices(void)
{
    int i, nd, nr;
    memory_dev_t *dev;
    
    if (names) {
	for (i = 0; names[i]; i++) free(names[i]);
    }
    for (nd = i = 0; nd < MAX_DEV; nd++) {
	if (dev_table[nd] == NULL) continue;
	dev = dev_table[nd]->priv;
	names[i] = malloc(B_OS_NAME_LENGTH);
	sprintf(names[i++], "memory/mem%da", nd);
	names[i] = malloc(B_OS_NAME_LENGTH);
	sprintf(names[i++], "memory/mem%dc", nd);
	for (nr = 0; nr < 2*MAX_PART; nr++) {
	    if (dev->minor[nr].region.RegionSize == 0) continue;
	    names[i] = malloc(B_OS_NAME_LENGTH);
	    sprintf(names[i++], "memory/mem%d%c%d", nd,
		    ((nr & 4) ? 'a' : 'c'), (nr & 3));
	}
    }
    names[i] = NULL;
    return (const char **)names;
}

device_hooks *find_device(const char *name)
{
    return &memory_device_hooks;
}


