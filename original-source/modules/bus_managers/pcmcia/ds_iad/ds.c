/*======================================================================

    PC Card Driver Services
    
    ds.c 1.1 1998/08/04 02:22:03
    
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
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#include <driver_settings.h>

//#include "wchan.h"

#define PCMCIA_DEBUG 5

#ifdef PCMCIA_DEBUG
int pc_debug = PCMCIA_DEBUG;
#define _printk printk
#define DEBUG(n, args) do { if (pc_debug>(n)) _printk args; } while (0)
#else
#define DEBUG(n, args) do { } while (0)
#endif

/*====================================================================*/

#define ddprintf dprintf
#define dxprintf

#define SOCKET_PRESENT		0x01
#define SOCKET_BUSY		0x02
#define SOCKET_REMOVAL_PENDING	0x10

typedef struct driver_info_t {
    dev_info_t dev_info;
    int        use_count;
    dev_link_t *(*attach)(void);
    void       (*detach)(dev_link_t *);
    struct driver_info_t *next;
} driver_info_t;

typedef struct socket_bind_t {
    driver_info_t	*driver;
    dev_link_t		*instance;
    struct socket_bind_t *next;
} socket_bind_t;

/* Socket state information */
typedef struct socket_info_t {
    client_handle_t	handle;
    int			state;
    struct timer_list	removal;
    socket_bind_t	*bind;
} socket_info_t;

/*====================================================================*/

/* Linked list of all registered device drivers */
static driver_info_t *root_driver = NULL;

/* Semaphore to protect changes to the device driver list */
static sem_id list_sem;

static int sockets = 0;
static socket_info_t *socket_table = NULL;


/* Device driver ID passed to Card Services */
static dev_info_t dev_info = "Driver Services";

static cs_client_module_info *cs;
#define CardServices	cs->_CardServices
#define add_timer		cs->_add_timer

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err;
    err.func = func; err.retcode = ret;
    CardServices(ReportError, handle, &err);
}


typedef struct 
{
	timer_list timer;
	char name[1];	
} repub_event;

static void repub_timer_func(u_long data)
{
	int fd;
	repub_event *ev = (repub_event *) data;
	if((fd = open("/dev",O_RDWR)) >= 0){
		write(fd,ev->name,strlen(ev->name)+1);
		close(fd);
	}
	free(ev);
}

static void republish(const char *name)
{
	repub_event *ev = (repub_event*) malloc(sizeof(repub_event)+strlen(name));
	if(ev){
		ev->timer.data = (u_long) ev;
		ev->timer.function = &repub_timer_func;
		ev->timer.expires = RUN_AT(10000);
		strcpy(ev->name,name);
		add_timer(&ev->timer);
	}
}

/*======================================================================

    Register_pccard_driver() and unregister_pccard_driver() are used
    tell Driver Services that a PC Card client driver is available to
    be bound to sockets.
    
======================================================================*/

int register_pccard_driver(dev_info_t *dev_info,
			   dev_link_t *(*attach)(void),
			   void (*detach)(dev_link_t *))
{
	driver_info_t *driver;
	socket_bind_t *b;
	int i;
    
	DEBUG(0, ("ds: register_pccard_driver('%s')\n", (char *)dev_info));
	acquire_sem(list_sem);
	for (driver = root_driver; driver; driver = driver->next)
		if (strncmp((char *)dev_info,
					(char *)driver->dev_info,
					DEV_NAME_LEN) == 0)
		break;
    
	if (!driver) {
		driver = malloc(sizeof(driver_info_t));
		strncpy(driver->dev_info, (char *)dev_info, DEV_NAME_LEN);
		driver->use_count = 0;
		driver->next = root_driver;
		root_driver = driver;
	}
	driver->attach = attach;
	driver->detach = detach;
	release_sem(list_sem);
	if (driver->use_count == 0)
		return 0;

    /* Instantiate any already-bound devices */
	for (i = 0; i < sockets; i++){
		for (b = socket_table[i].bind; b; b = b->next) {
			if (b->driver != driver) continue;
			b->instance = driver->attach();
			if (b->instance == NULL) {
				printk(KERN_NOTICE "ds: unable to create instance "
					   "of '%s'!\n", driver->dev_info);
			}
		}
	}

    return 0;
} /* register_pccard_driver */

/*====================================================================*/

int unregister_pccard_driver(dev_info_t *dev_info)
{
	driver_info_t *target, **d;
	socket_bind_t *b;
	int i;
	
	DEBUG(0, ("ds: unregister_pccard_driver('%s')\n",
			  (char *)dev_info));
	acquire_sem(list_sem);
	d = &root_driver;
	while ((*d) && (strncmp((*d)->dev_info, (char *)dev_info,
							DEV_NAME_LEN) != 0))
		d = &(*d)->next;
	if (*d == NULL) {
		release_sem(list_sem);
		return -1;
	}
	target = *d;
	if (target->use_count == 0) {
		*d = target->next;
		free(target);
	} else {
		/* Blank out any left-over device instances */
		target->attach = NULL; target->detach = NULL;
		for (i = 0; i < sockets; i++)
			for (b = socket_table[i].bind; b; b = b->next)
				if (b->driver == *d) b->instance = NULL;
	}
	release_sem(list_sem);
	return 0;
} /* unregister_pccard_driver */

/*====================================================================*/



static int bind_request(socket_info_t *s, bind_info_t *bind_info)
{
	int i = s - socket_table;
	struct driver_info_t *driver;
	socket_bind_t *b;
	bind_req_t bind_req;
	int ret;

	DEBUG(2, ("ds: bind_request(%d, '%s')\n", i,
			  (char *)bind_info->dev_info));
	acquire_sem(list_sem);
	
	for (driver = root_driver; driver; driver = driver->next)
		if (strncmp((char *)driver->dev_info,
					(char *)bind_info->dev_info,
					DEV_NAME_LEN) == 0)
		break;
	
	if (driver == NULL) {
		driver = malloc(sizeof(driver_info_t));
		strncpy(driver->dev_info, bind_info->dev_info, DEV_NAME_LEN);
		driver->use_count = 0;
		driver->next = root_driver;
		driver->attach = NULL; driver->detach = NULL;
		root_driver = driver;
	}

	for (b = s->bind; b; b = b->next)
		if (driver == b->driver) break;
	if (b != NULL) {
		bind_info->instance = b->instance;
		release_sem(list_sem);
		return EBUSY;
    }

	dprintf("ds: asking CS to bind '%s' to socket %d\n",
			(char *) &driver->dev_info, i);
	
	bind_req.Socket = i;
	bind_req.Function = bind_info->function;
	bind_req.dev_info = &driver->dev_info;
	ret = CardServices(BindDevice, &bind_req);
	if (ret != CS_SUCCESS) {
		release_sem(list_sem);
		cs_error(NULL, BindDevice, ret);
		printk("ds: unable to bind '%s' to socket %d\n",
			   (char *)dev_info, i);
		return ENODEV;
	}
    
	/* Add binding to list for this socket */
	driver->use_count++;
	b = malloc(sizeof(socket_bind_t));
    b->driver = driver;
	b->instance = NULL; 
	b->next = s->bind;
	s->bind = b;

	if (driver->attach) {
		dprintf("ds: bind attaching to driver\n");
		b->instance = driver->attach();
		if (b->instance == NULL) {
			release_sem(list_sem);
			printk("ds: unable to creat instance of '%s'!\n",
				   (char *)bind_info->dev_info);
			return ENODEV;
		}
    }

    release_sem(list_sem);
    
    return 0;
} /* bind_request */


static int unbind_request(socket_info_t *s, bind_info_t *bind_info)
{
	int i = s - socket_table;
	socket_bind_t **b, *c;

	DEBUG(2, ("ds: unbind_request(%d, '%s')\n", i,
			  (char *)bind_info->dev_info));
	acquire_sem(list_sem);
	for (b = &s->bind; *b; b = &(*b)->next)
		if (strncmp((char *)(*b)->driver->dev_info,
					(char *)bind_info->dev_info,
					DEV_NAME_LEN) == 0)
		break;
	if (*b == NULL) {
		release_sem(list_sem);
		return ENODEV;
	}
	c = *b;
	c->driver->use_count--;
	if (c->driver->detach) {
		if (c->instance) {
			c->driver->detach(c->instance);
			c->instance = NULL;
		}
	} else {
		if (c->driver->use_count == 0) {
			driver_info_t **d;
			for (d = &root_driver; *d; d = &((*d)->next))
				if (c->driver == *d) break;
			*d = (*d)->next;
			free(c->driver);
		}
	}
    *b = c->next;
    free(c);
    release_sem(list_sem);
    
	republish(bind_info->dev_info);
    return 0;
} /* unbind_request */

	
static int 
get_tuple(socket_info_t *s, cisdata_t code, 
		  tuple_t *tuple, cisparse_t *parse)
{
	uchar data[255];
	tuple->TupleData = (cisdata_t*) data;
	tuple->TupleDataMax = 255;
		
	tuple->DesiredTuple = code;
	tuple->Attributes = 0;
	if(CardServices(GetFirstTuple, s->handle, tuple)) return -1;
	tuple->TupleOffset = 0;
	if(CardServices(GetTupleData, s->handle, tuple)) return -1;
	if(CardServices(ParseTuple, s->handle, tuple, parse)) return -1;
	return 0;
}

static status_t
locate_driver(cistpl_vers_1_t *vers, 
			  cistpl_manfid_t *manfid,
			  cistpl_funcid_t *funcid,
			  bind_info_t *bind_info,
			  const char *settings_file)
{
	const driver_settings *settings;
	driver_parameter *p,*p2;
	
	void *handle;
	char *bind;
	int match = 0;
	int i,j,k,c;
	
	if(!(handle = load_driver_settings(settings_file))){
		dxprintf("ds: cannot obtain settings\n");
		return -1;
	}
	settings = get_driver_settings(handle);
	
	for(i=0;i<settings->parameter_count;i++){
		p = settings->parameters + i;
		if(strcmp(p->name,"card")){
			dxprintf("ds: unknown setting type '%s'\n", p->name);
			continue;
		}  
		if(p->value_count){
			dxprintf("ds: checking card \"%s\"\n",p->values[0]);
		}
		if(!(c = p->parameter_count)) continue;
		p = p->parameters;
		bind = NULL;
		for(j=0;j<c;j++,p++){
			if(!strcmp(p->name,"bind") && p->value_count){
				bind = p->values[0];
				dxprintf("ds: bind %s\n",bind);
				continue;
			}
			if(!strcmp(p->name,"manfid") && (p->value_count==2) && manfid){
				ushort manf = strtoul(p->values[0],NULL,0);
				ushort card = strtoul(p->values[1],NULL,0);
				if((manf == manfid->manf) && (card == manfid->card)){
					match = 1;
				}
				dxprintf("ds: manfid 0x%04x, 0x%04x\n",manf,card);
				continue;
			}
			if(!strcmp(p->name,"version") && vers){
				dxprintf("ds: version\n");
				for(k=0;k<p->value_count;k++){
					if(k>=vers->ns){
						match = 1;
						break;
					}
					dxprintf("ds: \"%s\" == \"%s\"?\n",
							p->values[k],vers->str+vers->ofs[k]);
					if(!strcmp(p->values[k],"*")) continue;
					if(strcmp(p->values[k],vers->str+vers->ofs[k])){
						break;
					}
				}
				if(k == p->value_count) match = 1;
			}
		}
		if(match && bind){
			strcpy((char*)&bind_info->dev_info,bind);
			unload_driver_settings(handle);
			return 0;
		}
	}
	
	unload_driver_settings(handle);		
	return -1;
}

static int
is_cardbus(socket_info_t *s)
{
	cs_status_t status;
	memset(&status,0,sizeof(cs_status_t));
	if(CardServices(GetStatus,s->handle,&status)) return 0;
	if(status.CardState & CS_EVENT_CB_DETECT) return 1;
	return 0;
}

static void
handle_insertion(socket_info_t *s)
{
	int res;
	cisinfo_t ci;
	bind_info_t bind_info;
//	dprintf("ds: card insertion\n");

	memset(&bind_info,0,sizeof(bind_info));
			
	if(is_cardbus(s)){ // is_cardbus
		//dprintf("ds: cardbus card\n");
		strcpy((char*)&bind_info.dev_info,"cb_enabler");
		bind_request(s,&bind_info);
		return;
	} else {
		tuple_t tuple;
		cisparse_t parse;
		cistpl_vers_1_t *vers = NULL;
		cistpl_manfid_t manfid = { 0, 0	};
		cistpl_funcid_t funcid = { 0xff, 0xff };		

		res = CardServices(ValidateCIS, s->handle,&ci);
		if(res) {
			ddprintf("ds: no ValidateCIS / %x\n",res);
			return;
		}
		
		ddprintf("ds: pcmcia info scan...\n");		
		if(!get_tuple(s,CISTPL_FUNCID,&tuple,&parse)){
			memcpy(&funcid,&parse.funcid,sizeof(funcid));
			ddprintf("ds: funcid %02x,%02x\n",funcid.func,funcid.sysinit);
		}
		if(!get_tuple(s,CISTPL_MANFID,&tuple,&parse)){
			memcpy(&manfid,&parse.manfid,sizeof(manfid));
			ddprintf("ds: manfid %04x,%04x\n",manfid.manf,manfid.card);
		}
		if(!get_tuple(s,CISTPL_VERS_1,&tuple,&parse)){
			int i;
			vers = &parse.version_1;
			for(i=0;i<vers->ns;i++){
				ddprintf("ds: vers[%d] = \"%s\"\n",i,vers->str+vers->ofs[i]);
			}
		}
		
		if(locate_driver(vers,&manfid,&funcid,&bind_info,"pcmcia") &&
		   locate_driver(vers,&manfid,&funcid,&bind_info,"pcmcia.default")){
			dprintf("ds: no matching driver to bind.\n");
		} else {
			dprintf("ds: matched to '%s'. binding.\n",
					(char*)&bind_info.dev_info);
			bind_request(s,&bind_info);
			republish(bind_info.dev_info);
		}
	}
}

static void
handle_removal(socket_info_t *s)
{
	bind_info_t bind_info;
	//dprintf("ds: card removal\n");
	
	while(s->bind){
		memset(&bind_info,0,sizeof(bind_info));
		dprintf("ds: unbinding driver '%s'\n",
				(char*)&s->bind->driver->dev_info);
		memcpy(&bind_info.dev_info,&s->bind->driver->dev_info,
			   sizeof(dev_info_t));
		unbind_request(s,&bind_info);
	}	
}

static void 
handle_event(socket_info_t *s, event_t event)
{
	bind_info_t bind_info;
	int socket = s - socket_table;
	
//	dprintf("ds: handle_event s[%d], %d\n",socket,event);
	
	switch(event){
    case CS_EVENT_CARD_INSERTION:
		handle_insertion(s);
		break;		
		
    case CS_EVENT_CARD_REMOVAL:
		handle_removal(s);
		break;
	}
	
}


static void 
timer_removal(u_long sn)
{
    socket_info_t *s = &socket_table[sn];
    handle_event(s, CS_EVENT_CARD_REMOVAL);
    s->state &= ~SOCKET_REMOVAL_PENDING;
}


static int 
ds_event(event_t event, int priority, event_callback_args_t *args)
{
    socket_info_t *s = (socket_info_t*) args->client_data;
	
    switch (event) {	
    case CS_EVENT_CARD_REMOVAL:
		s->state &= ~SOCKET_PRESENT;
		if (!(s->state & SOCKET_REMOVAL_PENDING)) {
			s->state |= SOCKET_REMOVAL_PENDING;
			s->removal.expires = RUN_AT(HZ/10);
			add_timer(&s->removal);
		}
		break;
	
    case CS_EVENT_CARD_INSERTION:
		s->state |= SOCKET_PRESENT;
		handle_event(s, event);
		break;

//    case CS_EVENT_EJECTION_REQUEST:
//		return handle_request(s, event);
//		break;
	
    default:
		handle_event(s, event);
		break;
    }

    return 0;
} /* ds_event */

static status_t
ds_init()
{
	adjust_t adj;
	client_reg_t client_reg;
	servinfo_t serv;
	bind_req_t bind;
	socket_info_t *s;
	int i, ret;
	status_t err;
	
	err = get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs);
	if(err) return err;

	CardServices(GetCardServicesInfo, &serv);
	if (serv.Revision != CS_RELEASE_CODE) {
		printk("ds: Card Services release does not match!\n");
		put_module(CS_CLIENT_MODULE_NAME);
		return B_ERROR;
	}

	sockets = serv.Count;
    socket_table = (socket_info_t*) malloc(sockets*sizeof(socket_info_t));
	
    for (i = 0, s = socket_table; i < sockets; i++, s++) {
		s->state = 0;
		s->handle = NULL;
		s->removal.prev = s->removal.next = NULL;
		s->removal.data = i;
		s->removal.function = &timer_removal;
		s->bind = NULL;
    }
    
    /* Set up hotline to Card Services */
	client_reg.dev_info = bind.dev_info = &dev_info;
	client_reg.Attributes = INFO_MASTER_CLIENT;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_EJECTION_REQUEST | CS_EVENT_INSERTION_REQUEST |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &ds_event;
	client_reg.Version = 0x0210;
	for (i = 0; i < sockets; i++) {
		bind.Socket = i;
		bind.Function = BIND_FN_ALL;
		ret = CardServices(BindDevice, &bind);
		if (ret != CS_SUCCESS) {
			cs_error(NULL, BindDevice, ret);
			break;
		}
		client_reg.event_callback_args.client_data = &socket_table[i];
		ret = CardServices(RegisterClient, &socket_table[i].handle,
						   &client_reg);
		if (ret != CS_SUCCESS) {
			cs_error(NULL, RegisterClient, ret);
			break;
		}
	}

	//XXXbjs read from config file
	adj.Action = ADD_MANAGED_RESOURCE;
	adj.Resource = RES_MEMORY_RANGE;
	adj.Attributes = 0;
	adj.resource.memory.Base = 0xc0000;
	adj.resource.memory.Size = 0x40000;
	ret = CardServices(AdjustResourceInfo, socket_table[0].handle,&adj);
	
	adj.Action = ADD_MANAGED_RESOURCE;
	adj.Resource = RES_MEMORY_RANGE;
	adj.Attributes = 0;
	adj.resource.memory.Base = 0x60000000;
	adj.resource.memory.Size = 0x10000000;
	ret = CardServices(AdjustResourceInfo, socket_table[0].handle,&adj);

	adj.Action = ADD_MANAGED_RESOURCE;
	adj.Resource = RES_IO_RANGE;
	adj.Attributes = 0;
	adj.resource.io.BasePort = 0x100;
	adj.resource.io.NumPorts = 0x700;
	adj.resource.io.IOAddrLines = 0;
	ret = CardServices(AdjustResourceInfo, socket_table[0].handle,&adj);
	
	return B_OK;
}

static status_t std_ops(int32 op, ...)
{
    switch (op) {
	case B_MODULE_INIT:
		return ds_init();
    case B_MODULE_UNINIT:
		return B_ERROR;
	}
    return B_OK;
}

static status_t get_handle(int socket, client_handle_t *handle)
{
	if((socket < 0) || (socket >= sockets)) {
		return B_ERROR;
	} else {
		*handle = socket_table[socket].handle;
		return B_OK;
	}
}

static ds_module_info ds_info = {
    { { DS_MODULE_NAME, 0, &std_ops }, NULL },
    &register_pccard_driver,
    &unregister_pccard_driver,
	&get_handle
};

_EXPORT module_info *modules[] = {
    (module_info *)&ds_info,
    NULL
};
