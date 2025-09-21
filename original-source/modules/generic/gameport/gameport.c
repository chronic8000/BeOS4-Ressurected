/*	Generic gameport driver module	*/
/*	For use by sound card drivers that publish a gameport device.	*/

#include <ISA.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include "joystick_driver.h"

/* speed compensation is not generally necessary, because the joystick */
/* driver is measuring using real time, not just # cycles. "0" means the */
/* default, center value. + typically returns higher values; - returns lower */

/* lower latency will make for more overhead in reading the joystick */
/* ideally, you set this value to just short of how long it takes you */
/* to calculate and render a frame. 30 fps -> latency 33000 */

#define GAMEPORT_JOYSTICK_MIN_LATENCY 10000		/* 100 times a second! */
#define GAMEPORT_JOYSTICK_MAX_LATENCY 100000		/* 10 times a second */


#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf
#endif

const char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;


typedef struct gameport_config {
	bigtime_t			latency;
	int32				compensation;
	uint32				port;
	uchar				(*read_func)(void *);
	void				(*write_func)(void *, uchar);
	void *				back_cookie;
} gameport_config;

typedef struct open_joystick {
	struct gameport_dev *	card;
	joystick *		joy;
} open_joystick;

#define MAX_EXTENDED 4
#define MAX_LEGACY 2
typedef struct gameport_dev {
	uint32				cookie;
	int32				open_count;
	sem_id				timer_sem;
	sem_id				open_sem;
	gameport_config		config;
	bool				raw_mode;
	extended_joystick		ext[MAX_EXTENDED];
	joystick_module_info	module_info;
	joystick_module		* module;
	void					* mod_cookie;
	joystick				stick[MAX_LEGACY];
	open_joystick		open;
} gameport_dev;


#define MIN_COMP -12
#define MAX_COMP 63
#define MIDPOINT 17

static status_t joy_open(void * device, uint32 flags, void **cookie);
static status_t joy_close(void *cookie);
static status_t joy_free(void *cookie);
static status_t joy_control(void *cookie, uint32 op, void *data, size_t len);
static status_t joy_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t joy_write(void *cookie, off_t pos, const void *data, size_t *len);

static status_t 
configure_joy(
	open_joystick * stick,
	gameport_config * config,
	bool force)
{
	status_t err = B_OK;

	ddprintf("gameport: configure_joy()\n");

	/* argument checking */

	if (config->latency < GAMEPORT_JOYSTICK_MIN_LATENCY) {
		config->latency = GAMEPORT_JOYSTICK_MIN_LATENCY;
	}
	if (config->latency > GAMEPORT_JOYSTICK_MAX_LATENCY) {
		config->latency = GAMEPORT_JOYSTICK_MAX_LATENCY;
	}
	if (config->compensation < MIN_COMP) {
		config->compensation = MIN_COMP;
	}
	if (config->compensation > MAX_COMP) {
		config->compensation = MAX_COMP;
	}
	if (config->port < 0x0 || config->port > 0xffff) {	/* we should really republish here... */
		config->port = stick->card->config.port;
		err = B_BAD_VALUE;
	}

	/* configure device */

	if (force || (config->latency != stick->card->config.latency)) {
		stick->card->config.latency = config->latency;
	}
	if (force || (config->compensation != stick->card->config.compensation)) {
		stick->card->config.compensation = config->compensation;
	}
	if ((force || (config->port != stick->card->config.port)) && (stick->card->config.port != 0)) {
		stick->card->config.port = config->port;
	}

	return err;
}


static status_t
joy_open(
	void * storage,
	uint32 flags,
	void ** cookie)
{
	int p, ix;
	const char * end, *val;
	gameport_dev * dd = (gameport_dev *)storage;

	ddprintf("gameport: joy_open(%x)\n", (int)dd);

	*cookie = NULL;
	if (!dd) {
		return ENODEV;
	}

	acquire_sem(dd->open_sem);

	ddprintf("open a");
	if (dd->open_count++ == 0) {
		char name[32];
		ddprintf("b");
		sprintf(name, "gameport timer %x", dd->config.port & 0xffff);
		dd->timer_sem = create_sem(1, name);
		if (dd->timer_sem < 0) {
			ddprintf(" error %x\n", dd->timer_sem);
			dd->open_count = 0;
			return dd->timer_sem;
		}
		set_sem_owner(dd->timer_sem, B_SYSTEM_TEAM);
		/*	synchronize with other openers	*/
		ddprintf("c");
	}
	ddprintf("d ");
	*cookie = &dd->open;

	ddprintf("gameport: joy_open(%x) done\n", (int)dd);
	release_sem(dd->open_sem);

	return B_OK;
}


static status_t
joy_close(
	void * cookie)
{
	ddprintf("gameport: joy_close()\n");
	/*	do nothing	*/
	return B_OK;
}


static status_t
joy_free(
	void * cookie)
{
	open_joystick * stick = (open_joystick *)cookie;

	ddprintf("gameport: joy_free()\n");

	acquire_sem(stick->card->open_sem);

	if (stick->card->module != NULL) {
		(*stick->card->module->crumble_alt)(stick->card->mod_cookie, stick->card->config.back_cookie,
				stick->card->config.read_func, stick->card->config.write_func);
		put_module(stick->card->module_info.module_name);
		stick->card->module = NULL;
	}

	if (atomic_add(&stick->card->open_count, -1) == 1) 	{
		delete_sem(stick->card->timer_sem);
	}

	release_sem(stick->card->open_sem);

	return B_OK;	/* already done in close */
}


static status_t
joy_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	open_joystick * stick = (open_joystick *)cookie;
	status_t err = B_BAD_VALUE;
	gameport_config config = stick->card->config;

	ddprintf("gameport: joy_control()\n");

	switch (iop) {
	case B_JOYSTICK_GET_SPEED_COMPENSATION:
		memcpy(data, &config.compensation, sizeof(int32));
		return B_OK;
	case B_JOYSTICK_SET_SPEED_COMPENSATION:
		memcpy(&config.compensation, data, sizeof(int32));
		err = B_OK;
		break;
	case B_JOYSTICK_GET_MAX_LATENCY:
		memcpy(data, &config.latency, sizeof(long long));
		return B_OK;
	case B_JOYSTICK_SET_MAX_LATENCY:
		memcpy(&config.latency, data, sizeof(long long));
		err = B_OK;
		break;
	case B_JOYSTICK_SET_DEVICE_MODULE:
		ddprintf("gameport: B_JOYSTICK_SET_DEVICE_MODULE(%s, %s)\n", 
			((joystick_module_info *)data)->module_name, 
			((joystick_module_info *)data)->device_name);
		if (stick->card->module) {
			ddprintf("gameport: closing old module %x\n", stick->card->module);
			(*stick->card->module->crumble_alt)(stick->card->mod_cookie, stick->card->config.back_cookie,
					stick->card->config.read_func, stick->card->config.write_func);
			put_module(stick->card->module_info.module_name);
		}
		if (get_module(((joystick_module_info *)data)->module_name, (module_info **)&stick->card->module)) {
			ddprintf("gameport: could not find new module\n");
			err = B_ENTRY_NOT_FOUND;
			stick->card->module = NULL;
		}
		else {
			ddprintf("gameport: new module is %x\n", stick->card->module);
			memcpy(&stick->card->module_info, data, sizeof(stick->card->module_info));
			if (stick->card->module->_is_zero_ != 0) {	/*	old module	*/
				ddprintf("gameport: module is too old\n");
				err = B_MISMATCHED_VALUES;
			}
			else {
				err = (*stick->card->module->configure_alt)(stick->card->config.back_cookie,
					stick->card->config.read_func, stick->card->config.write_func, &stick->card->module_info, 
					sizeof(stick->card->module_info), &stick->card->mod_cookie);
			}
			if (err < B_OK) {
				ddprintf("gameport: could not configure new module\n");
				put_module(stick->card->module_info.module_name);
				stick->card->module = NULL;
			}
		}
		if (!err) return B_OK;
		break;
	case B_JOYSTICK_GET_DEVICE_MODULE:
		if (!stick->card->module) {
			err = B_NO_INIT;
		}
		else {
			if (len == 0) len = sizeof(stick->card->module_info);
			if (len > sizeof(stick->card->module_info)) len = sizeof(stick->card->module_info);
			memcpy(data, &stick->card->module_info, len);
			err = len;
		}
		if (err >= B_OK) return B_OK;
		break;
	case B_JOYSTICK_SET_RAW_MODE:
		stick->card->raw_mode = *(bool *)data;
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK) {
		err = configure_joy(stick, &config, false);
	}
	return err;
}


static status_t
update_sticks(
	gameport_dev * joy)
{
	int cntx1 = 0, cnty1 = 0, cntx2 = 0, cnty2 = 0;
	int ctr = 0;
	uchar val;
	bigtime_t then;
	cpu_status cp;
	status_t err = B_OK;

	ddprintf("gameport: update_sticks()\n");

	then = system_time();

	if (joy->module) {
		err = (*joy->module->read_alt)(joy->mod_cookie, joy->config.back_cookie,
			joy->config.read_func, joy->config.write_func, joy->ext,
			sizeof(extended_joystick)*joy->module_info.num_sticks);
		joy->stick[0].timestamp = then;
		joy->stick[1].timestamp = then;
	}
	else {
		/*** hplus: a better way to poll would be to use timer interrupts */
	
		cp = disable_interrupts();
	
		(*joy->config.write_func)(joy->config.back_cookie, 0xff);
		while (true) {
			val = (*joy->config.read_func)(joy->config.back_cookie);
			if (!(val&0xf)) {
				break;
			}
			ctr += MIDPOINT+joy->config.compensation;
			if (ctr > 4095) {
				break;
			}
			if (val&0x1) {
				cntx1 = ctr;
			}
			if (val&0x2) {
				cnty1 = ctr;
			}
			if (val&0x4) {
				cntx2 = ctr;
			}
			if (val&0x8) {
				cnty2 = ctr;
			}
			while (system_time() <= then)
				/* busy-wait */ ;
			then += 1;
		}
	
		restore_interrupts(cp);
	
		joy->stick[0].timestamp = then;
		joy->stick[0].horizontal = 4095-cntx1;
		joy->stick[0].vertical = 4095-cnty1;
		joy->stick[0].button1 = !!(val&0x10);
		joy->stick[0].button2 = !!(val&0x20);
		joy->stick[1].timestamp = then;
		joy->stick[1].horizontal = 4095-cntx2;
		joy->stick[1].vertical = 4095-cnty2;
		joy->stick[1].button1 = !!(val&0x40);
		joy->stick[1].button2 = !!(val&0x80);
	}
	return err;
}


static status_t
joy_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	open_joystick * stick = (open_joystick *)cookie;
	status_t err;
	size_t to_read = *nread;
	cpu_status cp;
	bool dis = false;

	if (!stick->card->raw_mode) {
		ddprintf("gameport: joy_read()\n");
	}

	if (to_read < 1600) {	/*	disable for up to 1.6 milliseconds (shudder!)	*/
		dis = true;
	}
	*nread = 0;
	err = acquire_sem(stick->card->timer_sem);
	if (err < B_OK) {
		ddprintf("gameport: bad semaphore acquire %d\n", stick->card->timer_sem);
		return err;
	}
	if (stick->card->raw_mode) {
		bigtime_t then = system_time(), now;
		if (dis) {
			cp = disable_interrupts();
		}
		while (to_read > 0) {
			*(uint8 *)data = (*stick->card->config.read_func)(stick->card->config.back_cookie);
			while (then >= (now = system_time()))
				/* don't hog bus */;
			then = now;
			data = ((char *)data)+1;
			to_read--;
			(*nread)++;
		}
		if (dis) {
			restore_interrupts(cp);
		}
		ddprintf("gameport: raw read done (%ld bytes)\n", *nread);
		goto all_ok;
	}

	if (stick->joy->timestamp < system_time()+stick->card->config.latency) {
		err = update_sticks(stick->card);
	}
	if (stick->card->module) {
		if (to_read > sizeof(extended_joystick)*stick->card->module_info.num_sticks) 
			to_read = sizeof(extended_joystick)*stick->card->module_info.num_sticks;
		memcpy(data, stick->card->ext, to_read);
		*nread = to_read;
	}
	else {
		if (to_read > sizeof(joystick)*2) to_read = sizeof(joystick)*2;
		memcpy(data, stick->joy, to_read);
		*nread = to_read;
	}
all_ok:
	release_sem(stick->card->timer_sem);
	return err;
}


static status_t
joy_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	open_joystick * stick = (open_joystick *)cookie;
	status_t err;
	size_t to_write = *nwritten;
	bigtime_t then, now;

	ddprintf("gameport: joy_write()\n");

	if (stick->card->raw_mode) {
		err = acquire_sem(stick->card->timer_sem);
		if (err < B_OK) {
			ddprintf("gameport: bad semaphore acquire %d\n", stick->card->timer_sem);
			return err;
		}
		*nwritten = to_write;
		then = system_time();
		while (to_write-- > 0) {
			(*stick->card->config.write_func)(stick->card->config.back_cookie, *(char *)cookie);
			while ((now = system_time()) <= then)
				;
			then = now;
		}
		*nwritten -= (to_write+1);
		release_sem(stick->card->timer_sem);
	}
	else {
		*nwritten = 0;
		return B_UNSUPPORTED;
	}
	return B_OK;
}


static uchar
my_read(
	void * dev)
{
	return (*isa->read_io_8)(((gameport_dev *)dev)->config.port);
}

static void
my_write(
	void * dev,
	uchar byte)
{
	(*isa->write_io_8)(((gameport_dev *)dev)->config.port, byte);
}

static status_t
create_device(
	int port,
	void ** out_storage)
{
	gameport_dev * dd;
	char name[32];

	if (!out_storage) return B_BAD_VALUE;
	ddprintf("gameport: create_device()\n");
	*out_storage = malloc(sizeof(gameport_dev));
	if (!*out_storage) {
		return ENOMEM;
	}
	dd = (gameport_dev *)*out_storage;
	memset(dd, 0, sizeof(*dd));
	dd->cookie = 'gpdv';
	sprintf(name, "gameport %x open_sem", port & 0xffff);
	dd->open_sem = create_sem(1, name);
	if (dd->open_sem < B_OK) {
		status_t err = dd->open_sem;
		free(*out_storage);
		*out_storage = NULL;
		return err;
	}
	set_sem_owner(dd->open_sem, B_SYSTEM_TEAM);
	dd->open.card = dd;
	dd->open.joy = dd->stick;
	dd->config.latency = 30000LL;
	dd->config.port = port;
	dd->config.read_func = my_read;
	dd->config.write_func = my_write;
	dd->config.back_cookie = dd;
	return B_OK;
}

static status_t
create_device_alt(
	void * r_cookie,
	uchar (*read_func)(void * cookie),
	void (*write_func)(void * cookie, uchar value),
	void ** out_storage)
{
	gameport_dev * dd;
	char name[32];

	ddprintf("gameport: create_device_alt()\n");

	if (!out_storage) return B_BAD_VALUE;

	*out_storage = malloc(sizeof(gameport_dev));
	if (!*out_storage) {
		return ENOMEM;
	}
	dd = (gameport_dev *)*out_storage;
	memset(dd, 0, sizeof(*dd));
	dd->cookie = 'gpdv';
	sprintf(name, "gameport 0x%x open_sem", (int)r_cookie);
	dd->open_sem = create_sem(1, name);
	if (dd->open_sem < B_OK) {
		status_t err = dd->open_sem;
		free(*out_storage);
		*out_storage = NULL;
		return err;
	}
	set_sem_owner(dd->open_sem, B_SYSTEM_TEAM);
	dd->open.card = dd;
	dd->open.joy = dd->stick;
	dd->config.latency = 30000LL;
	dd->config.port = 0;
	dd->config.read_func = read_func;
	dd->config.write_func = write_func;
	dd->config.back_cookie = r_cookie;
	ddprintf("gameport: create_device_alt() done\n");
	return B_OK;
}

static status_t
delete_device(
	void * storage)
{
	gameport_dev * dd = (gameport_dev *)storage;
	ddprintf("gameport: delete_device()\n");
	if (dd) {
		if (dd->cookie != 'gpdv') {
			dprintf("bad pointer to gameport:delete_device(): %x\n", dd);
		}
		else {
			delete_sem(dd->open_sem);
			free(storage);
		}
	}
	return B_OK;
}



static status_t
init()
{
	if (get_module(isa_name, (module_info **) &isa))
		return B_ERROR;
	return B_OK;
}

static status_t
uninit()
{
	put_module(isa_name);
	return B_OK;
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		/* do nothing */
		;
	}
	return -1;
}


static generic_gameport_module g_gameport[2] = {
	{
		{
			B_GAMEPORT_MODULE_NAME_V1,
			0,
			std_ops
		},
		create_device,
		delete_device,
		joy_open,
		joy_close,
		joy_free,
		joy_control,
		joy_read,
		joy_write
	}, 
	{
		{
			B_GAMEPORT_MODULE_NAME,
			0,
			std_ops
		},
		create_device,
		delete_device,
		joy_open,
		joy_close,
		joy_free,
		joy_control,
		joy_read,
		joy_write,
		create_device_alt
	}, 
};

_EXPORT generic_gameport_module *modules[] = {
        &g_gameport[0],
		&g_gameport[1],
        NULL
};

