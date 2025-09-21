
/*** TODO ***

This driver was a pretty quick cut-paste-n-modify from David's dummy.c
It needs a bunch of cleanup before it's ready for prime time

*/

/*======================================================================

    A serial PCMCIA client driver

    This is provided as an example of how to write an IO card client.
    As written, it will function as a sort of generic point enabler,
    configuring any card as that card's CIS specifies.
    
    serial.c 1.3 1998/10/03 01:20:25

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

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

/*
   All the PCMCIA modules use PCMCIA_DEBUG to control debugging.  If
   you do not define PCMCIA_DEBUG at all, all the debug code will be
   left out.  If you compile with PCMCIA_DEBUG=0, the debug code will
   be present but disabled -- but it can then be enabled for specific
   modules at load time with a 'pc_debug=#' option to insmod.
*/
#define PCMCIA_DEBUG 5

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
static const char *version =
"serial.c 1.3 1998/10/03 01:20:25 (David Hinds)";
#define DEBUG(n, args) do { if (pc_debug>(n)) printk args; } while (0)
#else
#define DEBUG(n, args) do { } while (0)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* List of interrupts to choose from */
static int irq_list[4] = { -1 };

/*====================================================================*/

/*
   The event() function is this driver's Card Services event handler.
   It will be called by Card Services when an appropriate card status
   event is received.  The config() and release() entry points are
   used to configure or release a socket, in response to card
   insertion and ejection events.  They are invoked from the serial
   event handler. 
*/

static void serial_config(dev_link_t *link);
static void serial_release(u_long arg);
static int serial_event(event_t event, int priority,
		       event_callback_args_t *args);

/*
   The attach() and detach() entry points are used to create and destroy
   "instances" of the driver, where each instance represents everything
   needed to manage one actual PCMCIA card.
*/

static dev_link_t *serial_attach(void);
static void serial_detach(dev_link_t *);

/*
   The dev_info variable is the "key" that is used to match up this
   device driver with appropriate cards, through the card configuration
   database.
*/

static dev_info_t dev_info = "serial_cs";

/*
   An array of "instances" of the serial device.  Each actual PCMCIA
   card corresponds to one device instance, and is described by one
   dev_link_t structure (defined in ds.h).
*/

#define MAX_DEVS	8
static dev_link_t *	devs[MAX_DEVS];
static int		ndevs = 0;

/*
   A dev_link_t structure has fields for most things that are needed
   to keep track of a socket, but there will usually be some device
   specific information that also needs to be kept track of.  The
   'priv' pointer in a dev_link_t structure can be used to point to
   a device-specific private data structure, like this.

   A driver needs to provide a dev_node_t structure for each device
   on a card.  In some cases, there is only one device per card (for
   example, ethernet cards, modems).  In other cases, there may be
   many actual or logical devices (SCSI adapters, memory cards with
   multiple partitions).  The dev_node_t structures need to be kept
   in a linked list starting at the 'dev' field of a dev_link_t
   structure.  We allocate them in the card's private data structure,
   because they generally shouldn't be allocated dynamically.

   In this case, we also provide a flag to indicate if a device is
   "stopped" due to a power management event, or card ejection.  The
   device IO routines can use a flag like this to throttle IO to a
   card that is not ready to accept it.
*/
   
typedef struct local_info_t {
    dev_node_t	node;
    int		stop;
} local_info_t;

/*
  BeOS bus manager declarations: we use the shorthand references to
  bus manager functions to make the code somewhat more portable across
  platforms.
*/
static cs_client_module_info *	cs = NULL;
static ds_module_info *		ds = NULL;
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

    serial_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

    The dev_link structure is initialized, but we don't actually
    configure the card at this point -- we wait until we receive a
    card insertion event.
    
======================================================================*/

static dev_link_t *serial_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    local_info_t *local;
    int ret, i;
    
    DEBUG(0, ("serial_attach()\n"));

    /* Find a free slot in the device table */
    for (i = 0; i < MAX_DEVS; i++)
	if (devs[i] == NULL) break;
    if (i == MAX_DEVS) {
	printk("serial_cs: no devices available\n");
	return NULL;
    }
    
    /* Initialize the dev_link_t structure */
    link = malloc(sizeof(struct dev_link_t));
    memset(link, 0, sizeof(struct dev_link_t));
    devs[i] = link;
    ndevs++;
    link->release.function = &serial_release;
    link->release.data = (u_long)link;

    /* Interrupt setup */
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
    if (irq_list[0] == -1)
	link->irq.IRQInfo2 = 0xdeb8;
    else
	for (i = 0; i < 4; i++)
	    link->irq.IRQInfo2 |= 1 << irq_list[i];
    link->irq.Handler = NULL;
    
    /*
      General socket configuration defaults can go here.  In this
      client, we assume very little, and rely on the CIS for almost
      everything.  In most clients, many details (i.e., number, sizes,
      and attributes of IO windows) are fixed by the nature of the
      device, and can be hard-wired here.
    */
    link->conf.Attributes = 0;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;

    /* Allocate space for private device-specific data */
    local = malloc(sizeof(local_info_t));
    memset(local, 0, sizeof(local_info_t));
    link->priv = local;

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &serial_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	serial_detach(link);
	return NULL;
    }

    return link;
} /* serial_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void serial_detach(dev_link_t *link)
{
    int i;
    
    DEBUG(0, ("serial_detach(0x%x)\n", link));
    
    /* Locate device structure */
    for (i = 0; i < MAX_DEVS; i++)
	if (devs[i] == link) break;
    if (i == MAX_DEVS) return;

    /*
       If the device is currently configured and active, we won't
       actually delete it yet.  Instead, it is marked so that when
       the release() function is called, that will trigger a proper
       detach().
    */
    if (link->state & DEV_CONFIG) {
		DEBUG(0, ("serial_cs: detach postponed, '%s' "
				  "still locked\n", link->dev->dev_name));
		link->state |= DEV_STALE_LINK;
		return;
    }

    /* Break the link with Card Services */
    if (link->handle) CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free pieces */
    devs[i] = NULL;
    if (link->priv) free(link->priv);
    free(link);
    ndevs--;
	dprintf("detach: done\n");
	
} /* serial_detach */

/*======================================================================

    serial_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, h, a) \
while ((last_ret=CardServices(last_fn=(fn),h,a))!=0) goto cs_failed
#define CS_CHECK2(fn, h, a, b) \
while ((last_ret=CardServices(last_fn=(fn),h,a,b))!=0) goto cs_failed

#define CFG_CHECK(fn, h, a) \
if (CardServices(fn, h, a) != 0) goto next_entry
#define CFG_CHECK2(fn, h, a, b) \
if (CardServices(fn, h, a, b) != 0) goto next_entry

static void serial_config(dev_link_t *link)
{
    client_handle_t handle;
    tuple_t tuple;
    cisparse_t parse;
    local_info_t *dev;
    int last_fn, last_ret, nd;
    u_char buf[64];
    win_req_t req;
    memreq_t map;
    
    handle = link->handle;
    dev = link->priv;

    DEBUG(0, ("serial_config(0x%x)\n", link));

    /*
       This reads the card's CONFIG tuple to find its configuration
       registers.
    */
    tuple.DesiredTuple = CISTPL_CONFIG;
    tuple.Attributes = 0;
    tuple.TupleData = buf;
    tuple.TupleDataMax = sizeof(buf);
    tuple.TupleOffset = 0;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    CS_CHECK(GetTupleData, handle, &tuple);
    CS_CHECK2(ParseTuple, handle, &tuple, &parse);
    link->conf.ConfigBase = parse.config.base;
    link->conf.Present = parse.config.rmask[0];
    
    /* Configure card */
    link->state |= DEV_CONFIG;

    /*
      In this loop, we scan the CIS for configuration table entries,
      each of which describes a valid card configuration, including
      voltage, IO window, memory window, and interrupt settings.

      We make no assumptions about the card to be configured: we use
      just the information available in the CIS.  In an ideal world,
      this would work for any PCMCIA card, but it requires a complete
      and accurate CIS.  In practice, a driver usually "knows" most of
      these things without consulting the CIS, and most client drivers
      will only use the CIS to fill in implementation-defined details.
    */
    tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
    CS_CHECK(GetFirstTuple, handle, &tuple);
    while (1) {
	cistpl_cftable_entry_t dflt = { 0 };
	cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
	CFG_CHECK(GetTupleData, handle, &tuple);
	CFG_CHECK2(ParseTuple, handle, &tuple, &parse);

	if (cfg->index == 0) goto next_entry;
	link->conf.ConfigIndex = cfg->index;
	
	/* Does this card need audio output? */
	if (cfg->flags & CISTPL_CFTABLE_AUDIO) {
	    link->conf.Attributes |= CONF_ENABLE_SPKR;
	    link->conf.Status = CCSR_AUDIO_ENA;
	}
	
	/* Use power settings for Vcc and Vpp if present */
	/*  Note that the CIS values need to be rescaled */
	if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM))
	    link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
	else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM))
	    link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;
	    
	if (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM))
	    link->conf.Vpp1 = link->conf.Vpp2 =
		cfg->vpp1.param[CISTPL_POWER_VNOM]/10000;
	else if (dflt.vpp1.present & (1<<CISTPL_POWER_VNOM))
	    link->conf.Vpp1 = link->conf.Vpp2 =
		dflt.vpp1.param[CISTPL_POWER_VNOM]/10000;
	
	/* Do we need to allocate an interrupt? */
	if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1)
	    link->conf.Attributes |= CONF_ENABLE_IRQ;
	
	/* IO window settings */
	link->io.NumPorts1 = link->io.NumPorts2 = 0;
	if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
	    cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
	    link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
	    if (!(io->flags & CISTPL_IO_8BIT))
		link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
	    if (!(io->flags & CISTPL_IO_16BIT))
		link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
	    link->io.BasePort1 = io->win[0].base;
	    link->io.NumPorts1 = io->win[0].len;
	    if (io->nwin > 1) {
		link->io.Attributes2 = link->io.Attributes1;
		link->io.BasePort2 = io->win[1].base;
		link->io.NumPorts2 = io->win[1].len;
	    }
	}

	/* This reserves IO space but doesn't actually enable it */
	CFG_CHECK(RequestIO, link->handle, &link->io);

	/*
	  Now set up a common memory window, if needed.  There is room
	  in the dev_link_t structure for one memory window handle,
	  but if the base addresses need to be saved, or if multiple
	  windows are needed, the info should go in the private data
	  structure for this device.

	  Note that the memory window base is a physical address, and
	  needs to be mapped to virtual space with ioremap() before it
	  is used.
	*/
	if ((cfg->mem.nwin > 0) || (dflt.mem.nwin > 0)) {
	    cistpl_mem_t *mem =
		(cfg->mem.nwin) ? &cfg->mem : &dflt.mem;
	    req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM;
	    req.Base = mem->win[0].host_addr;
	    req.Size = mem->win[0].len;
	    req.AccessSpeed = 0;
	    link->win = (window_handle_t)link->handle;
	    CFG_CHECK(RequestWindow, &link->win, &req);
	    map.Page = 0; map.CardOffset = mem->win[0].card_addr;
	    CFG_CHECK(MapMemPage, link->win, &map);
	}
	/* If we got this far, we're cool! */
	break;
	
    next_entry:
	if (cfg->flags & CISTPL_CFTABLE_DEFAULT) dflt = *cfg;
	CS_CHECK(GetNextTuple, handle, &tuple);
    }
    
    /*
       Allocate an interrupt line.  Note that this does not assign a
       handler to the interrupt, unless the 'Handler' member of the
       irq structure is initialized.
    */
    if (link->conf.Attributes & CONF_ENABLE_IRQ)
	CS_CHECK(RequestIRQ, link->handle, &link->irq);
	
    /*
       This actually configures the PCMCIA socket -- setting up
       the I/O windows and the interrupt mapping, and putting the
       card and host interface into "Memory and IO" mode.
    */
    CS_CHECK(RequestConfiguration, link->handle, &link->conf);

    /*
      At this point, the dev_node_t structure(s) need to be
      initialized and arranged in a linked list at link->dev.
    */
    for (nd = 0; nd < MAX_DEVS; nd++)
	if (devs[nd] == link) break;
    sprintf(dev->node.dev_name, "bus/pcmcia/serial/serial%d", nd);
    dev->node.major = dev->node.minor = 0;
    link->dev = &dev->node;

    /* Finally, report what we've done */
    printk("%s: index 0x%02x: Vcc %d.%d",
	   dev->node.dev_name, link->conf.ConfigIndex,
	   link->conf.Vcc/10, link->conf.Vcc%10);
    if (link->conf.Vpp1)
	printk(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
    if (link->conf.Attributes & CONF_ENABLE_IRQ)
	printk(", irq %d", link->irq.AssignedIRQ);
    if (link->io.NumPorts1)
	printk(", io 0x%04x-0x%04x", link->io.BasePort1,
	       link->io.BasePort1+link->io.NumPorts1-1);
    if (link->io.NumPorts2)
	printk(" & 0x%04x-0x%04x", link->io.BasePort2,
	       link->io.BasePort2+link->io.NumPorts2-1);
    if (link->win)
	printk(", mem 0x%06lx-0x%06lx", req.Base,
	       req.Base+req.Size-1);
    printk("\n");
    
    link->state &= ~DEV_CONFIG_PENDING;
    return;

cs_failed:
    cs_error(link->handle, last_fn, last_ret);
    serial_release((u_long)link);

} /* serial_config */

/*======================================================================

    After a card is removed, serial_release() will unregister the
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void serial_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;

    DEBUG(0, ("serial_release(0x%x)\n", link));

    /*
       If the device is currently in use, we won't release until it
       is actually closed, because until then, we can't be sure that
       no one will try to access the device or its data structures.
    */
    if (link->open) {
	DEBUG(1, ("serial_cs: release postponed, '%s' still open\n",
		  link->dev->dev_name));
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    /* Unlink the device chain */
    link->dev = NULL;

    /*
      In a normal driver, additional code may be needed to release
      other kernel data structures associated with this device. 
    */
    
    /* Don't bother checking to see if these succeed or not */
    if (link->win)
	CardServices(ReleaseWindow, link->win);
    CardServices(ReleaseConfiguration, link->handle);
    if (link->io.NumPorts1)
	CardServices(ReleaseIO, link->handle, &link->io);
    if (link->irq.AssignedIRQ)
	CardServices(ReleaseIRQ, link->handle, &link->irq);
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	serial_detach(link);
    
} /* serial_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.

    When a CARD_REMOVAL event is received, we immediately set a
    private flag to block future accesses to this device.  All the
    functions that actually access the device should check this flag
    to make sure the card is still present.
    
======================================================================*/

static int serial_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, ("serial_event(0x%06x)\n", event));
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
#if 0
		if (link->state & DEV_CONFIG) {
	    ((local_info_t *)link->priv)->stop = 1;
	    link->release.expires = RUN_AT(HZ/20);
	    add_timer(&link->release);
	}
#endif
	{ /* republish! */
		int fd;
		if((fd = open("/dev",O_WRONLY)) >= 0){
			write(fd,"serial",6);
			close(fd);
		}
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	serial_config(link);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	/* Mark the device as stopped, to block IO until later */
	((local_info_t *)link->priv)->stop = 1;
	if (link->state & DEV_CONFIG)
	    CardServices(ReleaseConfiguration, link->handle);
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	if (link->state & DEV_CONFIG)
	    CardServices(RequestConfiguration, link->handle, &link->conf);
	((local_info_t *)link->priv)->stop = 0;
	/*
	  In a normal driver, additional code may go here to restore
	  the device state and restart IO. 
	*/
	break;
    }
    return 0;
} /* serial_event */

/*======================================================================

    The user-mode PC Card device interface

    Since this driver doesn't actually do anything, most of these
    routines are just stubs.
    
======================================================================*/

static status_t serial_open(const char *name, uint32 flags,
			   void **cookie)
{
    int nd;

    /* This would need to be more sophisticated if each instance
       managed more than one device. */
    for (nd = 0; nd < MAX_DEVS; nd++)
	if ((devs[nd] != NULL) &&
	    (strcmp(devs[nd]->dev->dev_name, name) == 0))
	    break;
    if (nd == MAX_DEVS)
	return -ENODEV;
    DEBUG(0, ("serial_open('%s') = %x\n", name, devs[nd]));
    *cookie = devs[nd];
    
    return B_OK;
} /* serial_open */

/*====================================================================*/

static status_t serial_close(void *cookie)
{
    return B_OK;
} /* serial_close */

static status_t serial_free(void *cookie)
{
    return B_OK;
} /* serial_free */

static status_t serial_read(void *cookie, off_t pos,
			   void *data, size_t *len)
{
    dev_link_t *link = cookie;
	if((pos == 0) && (*len >= 4)){
		unsigned char *out = (unsigned char *) data;
		out[0] = link->io.BasePort1 & 0xff;
		out[1] = (link->io.BasePort1 >> 8) & 0xff;
		out[2] = link->irq.AssignedIRQ;
		out[3] = 0;
		*len = 4;
		return B_OK;
	} else {
		*len = 0;
		return B_OK;
	}
} /* serial_read */

static status_t serial_write(void *cookie, off_t pos,
			    const void *data, size_t *len)
{
    return B_OK;
} /* serial_write */

/*====================================================================*/

static device_hooks serial_device_hooks = {
    &serial_open,
    &serial_close,
    &serial_free,
    NULL,
    &serial_read,
    &serial_write
};

/*====================================================================*/

static char **names = NULL;

status_t init_hardware(void)
{
	dprintf("serial: init hw\n");
    return B_OK;
}

static status_t init_cs(void)
{
    client_reg_t client_reg;
    servinfo_t serv;
    bind_req_t bind;
    int i, ret;
    
	for(i=0;i<MAX_DEVS;i++) devs[i] = NULL;
	
    DEBUG(0, ("%s\n", version));
    if (get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs) != B_OK) {
		cs = NULL;
		return B_ERROR;
	}
		   
    if (get_module(DS_MODULE_NAME, (struct module_info **)&ds) != B_OK) {
		put_module(CS_CLIENT_MODULE_NAME);
		cs = NULL;
		ds = NULL;
		return B_ERROR;
	}
	
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) {
		printk("serial_cs: Card Services release does not match!\n");
		return B_ERROR;
    }
    
    /* When this is called, Driver Services will "attach" this driver
       to any already-present cards that are bound appropriately. */
    register_pccard_driver(&dev_info, &serial_attach, &serial_detach);
    
    return B_OK;
}

static void uninit_cs(void)
{
    int i;
	if(cs && ds){
		unregister_pccard_driver(&dev_info);
		for (i = 0; i < MAX_DEVS; i++){
			if (devs[i]) {
				if (devs[i]->state & DEV_CONFIG) serial_release((u_long)devs[i]);
				serial_detach(devs[i]);
			}	
		}
		if (names != NULL) free(names);
		if (cs) put_module(CS_CLIENT_MODULE_NAME);
		if (ds) put_module(DS_MODULE_NAME);
		cs = NULL; ds = NULL;
		names = NULL;
		dprintf("serial: uninit done\n");
	}
}

status_t init_driver(void)
{
    int ret;
	dprintf("serial: init\n");
	 
	ret = init_cs();
    /* Only maintain connection to CS if needed.  This is important for
       drivers that also manage non-PCMCIA device instances. */
    if (ndevs == 0) uninit_cs();
    return ret;
}

void uninit_driver(void)
{
	dprintf("serial: uninit\n");
    uninit_cs();
}

const char **publish_devices(void)
{
    int i, nd;
	dprintf("serial: publish\n");
    /* Establish connection with CS if needed */
    if (ndevs == 0) init_cs();
    if (ndevs == 0) uninit_cs();
    if (names != NULL) free(names);
    names = malloc((ndevs+1) * sizeof(char *));
    for (nd = i = 0; nd < MAX_DEVS; nd++) {
	if (devs[nd] == NULL) continue;
	if (!(devs[nd]->state & DEV_PRESENT)) continue;
	names[i++] = devs[nd]->dev->dev_name;
    }
    names[i] = NULL;
    return (const char **)names;
}

device_hooks *find_device(const char *name)
{
	dprintf("serial: find devs\n");
    return &serial_device_hooks;
}

