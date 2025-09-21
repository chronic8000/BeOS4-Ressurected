#include <Drivers.h>
#include <termios.h>

#include <OS.h>
#include <ttydev.h>

#if INTEL_STATIC_DRIVERS
#include <drivers_priv.h>
#endif

#define		R		0
#define		W		1
#define		C		2

typedef struct tty_desc {
	port_id		qport;
	port_id		rports[3];
	sem_id		sems[3];
} tty_desc;

static int	tty_id = 0;


static status_t
tty_control(void *_desc, uint32 msg, void *buf, size_t len)
{
	tty_desc *desc = (tty_desc*) _desc;
	tty_info		*info;
	int				status;
	int				err;

	switch (msg) {

	case TTYGETINFO:
		info = (tty_info *) buf;
		info->qport = desc->qport;
		info->rport = desc->rports[R];
		info->wport = desc->rports[W];
		info->cport = desc->rports[C];
		return B_NO_ERROR;
	case TCGETA:
		err = send_receive(desc, C, TTYGETA, NULL, 0, &status, buf);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case TCSETA:
		err = send_receive(desc, C, TTYSETA, buf, sizeof(struct termios), &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case TCSETAW:
		err = send_receive(desc, C, TTYSETAW, buf, sizeof(struct termios), &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case TCSETAF:
		err = send_receive(desc, C, TTYSETAF, buf, sizeof(struct termios), &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case TIOCGWINSZ:
		err = send_receive(desc, C, TTYGETWINSIZE, NULL, 0, &status, buf);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case TIOCSWINSZ:
		err = send_receive(desc, C, TTYSETWINSIZE, buf, sizeof(struct winsize), &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case B_SET_NONBLOCKING_IO:
		err = send_receive(desc, C, TTYNONBLOCK, NULL, 0, &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	case B_SET_BLOCKING_IO:
		err = send_receive(desc, C, TTYBLOCK, NULL, 0, &status, NULL);
		if (err >= B_NO_ERROR)
			return status;
		else
			return B_ERROR;
	}
	return B_ERROR;
}


static status_t
tty_open(const char *device, uint32 flags, void **_cookie)
{
	tty_desc **cookie = (tty_desc**) _cookie;
	tty_desc		*desc;
	char			name[32];
	int				id;

	id = tty_id++;

	desc = (tty_desc *) malloc(sizeof(tty_desc));
	if (!desc)
		goto error1;

	sprintf(name, "tty%d_query_port", id);
	desc->qport = create_port(3, name);
	if (desc->qport < 0)
		goto error2;

	sprintf(name, "tty%d_read_reply_port", id);
	desc->rports[R] = create_port(1, name);
	if (desc->rports[R] < 0)
		goto error3;

	sprintf(name, "tty%d_write_reply_port", id);
	desc->rports[W] = create_port(1, name);
	if (desc->rports[W] < 0)
		goto error4;

	sprintf(name, "tty%d_control_reply_port", id);
	desc->rports[C] = create_port(1, name);
	if (desc->rports[C] < 0)
		goto error5;

	sprintf(name, "tty%d_read_sem", id);
	desc->sems[R] = create_sem(1, name);
	if (desc->sems[R] < 0)
		goto error6;
	set_sem_owner(desc->sems[R], B_SYSTEM_TEAM);

	sprintf(name, "tty%d_write_sem", id);
	desc->sems[W] = create_sem(1, name);
	if (desc->sems[W] < 0)
		goto error7;
	set_sem_owner(desc->sems[W], B_SYSTEM_TEAM);

	sprintf(name, "tty%d_control_sem", id);
	desc->sems[C] = create_sem(1, name);
	if (desc->sems[C] < 0)
		goto error8;
	set_sem_owner(desc->sems[C], B_SYSTEM_TEAM);

	*cookie = desc;
	return B_NO_ERROR;

error8:
	delete_sem(desc->sems[W]);
error7:
	delete_sem(desc->sems[R]);
error6:
	delete_port(desc->rports[C]);
error5:
	delete_port(desc->rports[W]);
error4:
	delete_port(desc->rports[R]);
error3:
	delete_port(desc->qport);
error2:
	free(desc);	
error1:
	return B_ERROR;
}

static status_t
tty_close(void *_desc)
{
	tty_desc *desc = (tty_desc*) _desc;
	delete_port(desc->qport);
	delete_port(desc->rports[R]);
	delete_port(desc->rports[W]);
	delete_port(desc->rports[C]);
	delete_sem(desc->sems[R]);
	delete_sem(desc->sems[W]);
	delete_sem(desc->sems[C]);

/* ### this should really be done in free */
	free(desc);
	return B_NO_ERROR;
}

static status_t
tty_free(void *desc)
{
	return 0;
}


static status_t
tty_read (void *_desc, off_t pos, void *buf, size_t *len)
{
	tty_desc *desc = (tty_desc*) _desc;
	int			parms[1];
	int			status;
	int			err;

	parms[0] = *len;
	err = send_receive(desc, R, TTYREAD, &parms, sizeof(parms), &status, buf);

	if (err >= B_NO_ERROR)
		if (status >= 0) {
			*len = status;
			return 0;
		} else
			return status;
	else
		return err;
}

static status_t
tty_write (void *_desc, off_t pos, const void *buf, size_t *len)
{
	tty_desc *desc = (tty_desc*) _desc;

	int			status;
	int			err;
	ulong       count, curr;

	count = *len;
	/*
	 * bat: put in this to make gnu cat work
	 * XXX: should really check the validity of "count" bytes of "buf",
	 * but I'll let someone more knowledgable than I take care of that.
	 */
	if ((int)count < 0) {
		return (EINVAL);
	}
	while(count > 0) {
		if (count > MAX_PACKET_SIZE) {
			curr   = MAX_PACKET_SIZE;
			count -= MAX_PACKET_SIZE;
		} else {
			curr   = count;
			count  = 0;
		}
		err = send_receive(desc, W, TTYWRITE, buf, curr, &status, NULL);

		if (err <= B_NO_ERROR)
			break;
		
		buf = (void *)((ulong)buf + curr);
	}

	if (err == B_NO_ERROR)
		if (status >= 0) {
			*len = status;
			return 0;
		} else
			return status;
	else
		return err;
}

static int
send_receive(tty_desc *desc, int tp, int cmd, void *packet,	int packetsize,
	int *statusp, void *reply)
{
	int			err;
	long		status;

	if ((err = acquire_sem_etc(desc->sems[tp], 1, B_CAN_INTERRUPT, 0.0)) != B_NO_ERROR)
		return err;

	if ((err = write_port(desc->qport,cmd,packet,packetsize)) != B_NO_ERROR) {
		release_sem(desc->sems[tp]);
		return err;
	}
	
	err = read_port(desc->rports[tp], &status, reply, MAX_PACKET_SIZE);
	if (err >= B_NO_ERROR) {
		*statusp = status;
	} else if (err == B_INTERRUPTED && cmd == TTYREAD) {
		long mystatus, err2;

		do {
			err2 = write_port(desc->qport, TTYRABORT, NULL, 0);
			if (err2 != B_NO_ERROR) {
				dprintf("tty abort write_port: err2 == 0x%x\n", err2);
			}
		} while (err2 != B_NO_ERROR);


		while ((err2 = read_port(desc->rports[R], &mystatus, NULL, 0)) ==
			   B_INTERRUPTED)
			snooze(500);

		if (err2 < B_NO_ERROR || mystatus < 0) {
			dprintf("tty driver: Terminal: bad ack #1 (err %d, status %d)!\n",
					err2, mystatus);
		}


		while ((err2 = read_port(desc->rports[R], &mystatus, NULL, 0)) ==
			   B_INTERRUPTED)
			snooze(500);

		if (err2 < B_NO_ERROR || mystatus >= 0) {
			dprintf("tty driver: Terminal: bad ack #2 (err %d, status %d)!\n",
					err2, mystatus);
		}
	}

	release_sem(desc->sems[tp]);

	return err;
}

/* -----
	driver-related structures
----- */

static const char *tty_name[] = { "tty", NULL };

static device_hooks tty_device =  {
	tty_open, 			/* -> open entry point */
	tty_close, 			/* -> close entry point */
	tty_free,			/* -> free entry point */
	tty_control, 		/* -> control entry point */
	tty_read,			/* -> read entry point */
	tty_write			/* -> write entry point */
};


#if INTEL_STATIC_DRIVERS
static
#endif
const char **
publish_devices()
{
	return tty_name;
}

#if INTEL_STATIC_DRIVERS
static
#endif
device_hooks *
find_device(const char *name)
{
	return &tty_device;
}


#ifdef INTEL_STATIC_DRIVERS
driver_entry  tty_driver = {
	NULL,
	publish_devices,
	find_device,
	NULL,
	NULL
};
#endif
