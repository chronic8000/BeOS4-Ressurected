/*
	This is an attempt at an IR port driver
	
	Copyright (C) 1991-95 Be Inc.  All Rights Reserved
*/
#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <ByteOrder.h>

#include "ir.h"
#include "interrupts.h"
#include <stdlib.h>


/* forward declarations */
static status_t ir_open(const char *name, uint32 flags, void **cookie);
static status_t ir_free (void *cookie);
static status_t ir_close(void *cookie);
static status_t	ir_control(void *cookie, uint32 msg, void *buf, size_t size);
static status_t	ir_read(void *cookie, off_t pos, void *buf, size_t *count);
static status_t	ir_write(void *cookie, off_t pos, const void *buf, size_t *count);


/*
	private types, definitions
*/
#define IR_COMMAND_REG   (0x368)
#define IR_RESPONSE_REG	 (0x36a)

#define IR_HOST_DATA     (0x200)  /* command register bit: data is from host */

#define IR_INT_ENABLE    (0x800)  /* these two are for the response register */
#define IR_INT_SET       (0x400)

#define IR_MSG_MASK      (0x3ff)  /* doesn't include interrupt bits */
#define IR_CONTROL_MASK  (0x300)  /* only the two control bits */
#define IR_CMD_ACK       (0x300)  /* when both bits are set it's an ack */


#define read_cmd_reg           (B_LENDIAN_TO_HOST_INT16 (*(vshort *)ird->cmd_reg) & 0x3ff)
#define read_response_reg      (B_LENDIAN_TO_HOST_INT16 (*(vshort *)ird->response_reg) & 0xfff)

#define write_cmd_reg(x)       *(vshort *)ird->cmd_reg = B_HOST_TO_LENDIAN_INT16 (ird->last_cmd=(x))
#define write_response_reg(x)  *(vshort *)ird->response_reg = B_HOST_TO_LENDIAN_INT16(x)


/*
   Commands the IR controller understands
*/
#define START_SAMPLE(x)            (0x0300 | (1 << (x & 3)))
#define START_SEND(x)              (0x0308 | (1 << (x & 3)))
#define SET_LOGICAL_LOW(x, low)    (0x0310 | (low << (x & 3)))
#define STOP_ANALYSIS(x)           (0x0318 | (1 << (x & 3)))
#define START_LISTEN(x)            (0x0320 | (1 << (x & 3)))
#define START_ANALYSIS(x)          (0x0328 | (1 << (x & 3)))
#define SEND_ANALYSIS(x)           (0x0330 | (1 << (x & 3)))
#define LOAD_SETTINGS(x)           (0x0338 | (1 << (x & 3)))

/*
   Feedback messages from the IR controller
*/
#define ACK_MASK                   (0x0ff)   /* we only compare the low byte */
#define REPEAT_MSG                 (0x3f0)   /* low nibble == ir port # */
#define RESET_DONE                 (0x3fd)
#define LAST_BYTE_SENT             (0x3fe)

#define IR_DELAY        ((bigtime_t)500)    /* number of usecs to wait when resetting */
#define IR_BAD_READ     (0xfffff)  /* not a valid value to read */



#define IR_ACK_BUFF_SIZE  32
static ushort ir_ack_buff[IR_ACK_BUFF_SIZE];
static int    ir_ack_buff_rd=0, ir_ack_buff_wr=0;

#define IR_BUFF_SIZE  1024

typedef struct {
	long     open;
	uint     state;          /* current mode (either listen or sample) */

	sem_id   ir_read_sem;              /* interlocks with interrupt handler */
	ushort   ir_buff[IR_BUFF_SIZE];    /* read input data buffer */
	int      ir_buff_rd, ir_buff_wr;

	uint     logical_low;
	short    response;
	short    last_cmd;       /* used to check ack's */
	uint     port_num;       /* 0, 1, or 2 */
	vushort *cmd_reg;        /* pointer to the command  register */
	vushort *response_reg;   /* pointer to the response register */
	int      settings[2];    /* state of listen mode settings */

} ir_data;


/* #define's for the "state" member field */
#define LISTEN_MODE     0x0001
#define SAMPLE_MODE     0x0002
#define MODE_MASK       0x0003
#define LISTENING       0x0100   /* for when we're actively doing this */
#define SAMPLING        0x0200


#define NUM_OPENS      1    /* the max # of concurrent opens for this driver */
#define NUM_IR_PORTS   3
ir_data ir_ports[NUM_IR_PORTS] =
{
	{ NUM_OPENS, 0, -1, 0, },
	{ NUM_OPENS, 0, -1, 0, },
	{ NUM_OPENS, 0, -1, 0, }
};


static sem_id ir_big_sem   = -1;   /* big lock between the different ports */
static sem_id ir_ack_sem   = -1;   /* interlocks with the interrupt handler */
static sem_id ir_write_sem = -1;

static volatile int  ir_stop_interrupts = 0;  /* to signal the int handler */

static int32 ir_int_handler(void *arg);
static int  reset_controller(ir_data *ird);
static int  set_logical_low(ir_data *ird);

static vchar *isa_io;


/*
   workaround for compiler's not supporting volatile type
*/
static void FOO(ushort x)
{
}


status_t
init_driver(void)
{
	ir_data *ird;
	int res;
	area_info	a;
	area_id		id;
	
	if (io_card_version() < 0)
		return  B_ERROR;

	id = find_area ("isa_io");
	if (id < 0)
		return B_ERROR;
	if (get_area_info (id, &a) < 0)
		return B_ERROR;

	isa_io = (vchar *) a.address;

	dprintf("ir driver: %s %s\n", __DATE__, __TIME__);
	
	ir_big_sem = ir_ack_sem = ir_write_sem = -1;
	
	ir_big_sem       = create_sem(3, "ir_big_sem");
	ir_ack_sem       = create_sem(0, "ir_ack_sem");
	ir_write_sem     = create_sem(0, "ir_write_sem");
	
	if (ir_ack_sem < B_NO_ERROR || ir_big_sem < B_NO_ERROR ||
		ir_write_sem < B_NO_ERROR) {
		delete_sem(ir_big_sem);
		delete_sem(ir_ack_sem);
		delete_sem(ir_write_sem);
		
		return B_ERROR;
	}
	
	set_sem_owner(ir_big_sem,   B_SYSTEM_TEAM);
	set_sem_owner(ir_ack_sem,   B_SYSTEM_TEAM);
	set_sem_owner(ir_write_sem, B_SYSTEM_TEAM);
	
	/* dummy up an ir_data structure so that reset_controller will work */
	ird               = &ir_ports[0];
	ird->port_num     = 0;
	ird->cmd_reg      = (vushort *)(isa_io + IR_COMMAND_REG);
	ird->response_reg = (vushort *)(isa_io + IR_RESPONSE_REG);
	ird->state        = LISTEN_MODE;
	ird->logical_low  = 1;
	ird->settings[0]  = ird->settings[1] = 0;
	ird->ir_buff_rd   = ird->ir_buff_wr  = 0;
	ird->ir_read_sem  = -1;

	/* dprintf("doing ir controller reset...\n"); */
	res = reset_controller(ird);
	if (res != B_NO_ERROR) {
		delete_sem(ir_big_sem);
		delete_sem(ir_ack_sem);
		delete_sem(ir_write_sem);

		remove_io_interrupt_handler(B_INT_IR, ir_int_handler,
									isa_io + IR_RESPONSE_REG);
		
		dprintf("couldn't reset ir controller!!\n");
		return res; /* open failed */
	}

	/* until someone does an open */
	remove_io_interrupt_handler(B_INT_IR, ir_int_handler, 
								isa_io + IR_RESPONSE_REG);
	
	/* dprintf("ir controller properly reset!\n"); */

	return B_NO_ERROR;
}


void
uninit_driver(void)
{
	/* dprintf("closing down the IR driver... bye bye\n"); */

	/* clear the int handler and delete semaphores */
	remove_io_interrupt_handler(B_INT_IR, ir_int_handler, 
								isa_io + IR_RESPONSE_REG);

	delete_sem(ir_big_sem);
	delete_sem(ir_ack_sem);
	delete_sem(ir_write_sem);

	/* dprintf("IR driver: 5000 G. we outta here.\n"); */
}


/* -----
public globals and functions
----- */

/*
	public globals
*/

static const char *ir_name[] = {
	"beboxhw/infrared/ir1",
	"beboxhw/infrared/ir2",
	"beboxhw/infrared/ir3",
	NULL
};


device_hooks ir_devices = {
	ir_open, 			/* -> open entry point */
	ir_close, 			/* -> close entry point */
	ir_free,			/* -> free cookie */
	ir_control, 		/* -> control entry point */
	ir_read,			/* -> read entry point */
	ir_write			/* -> write entry point */
};

const char**
publish_devices()
{
	return ir_name;
}

device_hooks*
find_device(const char* name)
{
	return &ir_devices;
}


static status_t
ir_open(const char *dname, uint32 flags, void **cookie)
{
	int old, dn, i, res;
	long count;
	ir_data *ird;

    for (dn = 0; dn < NUM_IR_PORTS; dn++)
        if (!strcmp(dname, ir_name[dn]))
            break;
    if (dn >= NUM_IR_PORTS)
        return B_ERROR;

	/* dprintf("ir: trying to open ir port %d (%s)\n", dn, dname); */
	ird = &ir_ports[dn];


	old = atomic_add(&ird->open, -1); /* check if we're opened   */
	if (ird->open < 0 && old <= 0) {
		atomic_add(&ird->open, 1); 	  /* undo our (failed) open  */
		return B_ERROR;
	}

	ird->port_num     = dn;
	ird->cmd_reg      = (vushort *)(isa_io + IR_COMMAND_REG);
	ird->response_reg = (vushort *)(isa_io + IR_RESPONSE_REG);
	ird->state        = LISTEN_MODE;
	ird->logical_low  = 1;
	ird->settings[0]  = ird->settings[1] = 0;
	ird->ir_read_sem  = create_sem(0, "ir_read_sem");
	ird->ir_buff_rd   = ird->ir_buff_wr  = 0;

	if (ird->ir_read_sem < B_NO_ERROR) {
		atomic_add(&ird->open, 1); 	  /* undo our (failed) open  */
		return B_ERROR;
	}
	set_sem_owner(ird->ir_read_sem, B_SYSTEM_TEAM);

	install_io_interrupt_handler(B_INT_IR, ir_int_handler,
		isa_io + IR_RESPONSE_REG, NULL);

	*cookie = (void *)ird;

	return B_NO_ERROR;
}


/* -----
ir_free
----- */
static status_t
ir_free (void *cookie)
{
	return B_NO_ERROR;
}

/*
	ir_close
*/

static status_t
ir_close(void *cookie)
{
	ir_data *ird;

	ird = (ir_data *)cookie;
	
	if (ird->open >= NUM_OPENS) {         /* should never happen */
		dprintf("gack! ird->open == %d\n", ird->open);
		return B_ERROR;
	}


	delete_sem(ird->ir_read_sem);
	atomic_add(&ird->open, 1);            /* we're done */


	return B_NO_ERROR;
}

static int
ir_get_ack(ir_data *ird, int flags)
{
	int data;
	
	if (acquire_sem_etc(ir_ack_sem, 1, flags, 250000) < 0) {
		dprintf("ir%d: interrupted an ack read\n", ird->port_num+1);
		return IR_BAD_READ;
	}

	data = (ir_ack_buff[ir_ack_buff_rd] & 0xffff);
	
	ir_ack_buff_rd = (ir_ack_buff_rd + 1) % IR_ACK_BUFF_SIZE;

	return data;
}


static void
ir_put_ack(short data)
{
	if (((ir_ack_buff_wr+1) % IR_ACK_BUFF_SIZE) != (ir_ack_buff_rd % IR_ACK_BUFF_SIZE)) {
		ir_ack_buff[ir_ack_buff_wr] = data;
		ir_ack_buff_wr = (ir_ack_buff_wr + 1) % IR_ACK_BUFF_SIZE;

		release_sem(ir_ack_sem);
	} else {
		dprintf("ir ack buffer overflow (lost data == 0x%x)!\n", data);
	}
}

static int
ir_how_many_acks(void)
{
	int rd, wr;

	rd = ir_ack_buff_rd % IR_ACK_BUFF_SIZE;
	wr = ir_ack_buff_wr % IR_ACK_BUFF_SIZE;
	
	if (wr >= rd)
		return (wr - rd);

	return ((IR_ACK_BUFF_SIZE - rd) + wr);
}


static int
ir_get_data(ir_data *ird, int flags)
{
	int data;
	
	if (acquire_sem_etc(ird->ir_read_sem, 1, flags, 0) < 0) {
		dprintf("ir%d: interrupted a read\n", ird->port_num+1);
		return IR_BAD_READ;
	}

	data = (ird->ir_buff[ird->ir_buff_rd] & 0xffff);
	
	ird->ir_buff_rd = (ird->ir_buff_rd + 1) % IR_BUFF_SIZE;

	return data;
}


static void
ir_put_data(short data)
{
	int port_num;
	ir_data *ird;

	port_num = (data & IR_CONTROL_MASK) >> 8;
	if (port_num >= NUM_IR_PORTS) {
		dprintf("bogus put data: 0x%x\n", data);
		return;
	}
	
	ird = &ir_ports[port_num];

	if (((ird->ir_buff_wr+1) % IR_BUFF_SIZE) !=
		(ird->ir_buff_rd % IR_BUFF_SIZE)) {
		ird->ir_buff[ird->ir_buff_wr] = data;
		ird->ir_buff_wr = (ird->ir_buff_wr + 1) % IR_BUFF_SIZE;

		release_sem(ird->ir_read_sem);
	} else {
//		dprintf("ir buffer overflow!\n");
	}
	
}

static int
ir_how_much_data(ir_data *ird)
{
	int rd, wr;

	rd = ird->ir_buff_rd % IR_BUFF_SIZE;
	wr = ird->ir_buff_wr % IR_BUFF_SIZE;
	
	if (wr >= rd)
		return (wr - rd);

	return ((IR_BUFF_SIZE - rd) + wr);
}

/*
	ir_read
*/
static status_t
ir_read(void *cookie, off_t pos, void *_buf, size_t *count)
{
	ulong i, j;
	int data, storing = 0;
	ir_data *ird=(ir_data *)cookie;
	char *buf = (char *)_buf;

	ir_stop_interrupts = 0; 

	if (ird->state & LISTEN_MODE) {
		if (acquire_sem_etc(ir_big_sem, 1, B_CAN_INTERRUPT, 0) < 0)
			return B_IO_ERROR;

		/* dprintf("doing a LISTEN READ for %d bytes\n", *count); */
		ird->state |= LISTENING;
		start_listen(ird);
	} else if (ird->state & SAMPLE_MODE) {
		if (acquire_sem_etc(ir_big_sem, 3, B_CAN_INTERRUPT, 0) < 0)
			return B_IO_ERROR;
		
		/* dprintf("doing a SAMPLE READ for %d bytes\n", *count); */
		ird->state |= SAMPLING;
		start_sample(ird);
	} else {
		dprintf("ir: bogus state dude (%d)....\n", ird->state);
		return B_IO_ERROR;
	}
	
	for(i=0; i < *count; ) {
		if ((data = ir_get_data(ird, B_CAN_INTERRUPT)) == IR_BAD_READ) {
			dprintf("aborting a read after %d bytes\n", i);
			break;
		}

		if ((ird->state & LISTEN_MODE) || storing || (data & 0xff) != 0) {
			buf[i] = (char)(data & 0xff);
			storing = 1;
			i++;
		} 
	}

	*count = i;

/* dprintf("ir%d: cleaning up after read...\n", ird->port_num+1); */
	ir_stop_interrupts = 1;   	/* cleared by the interrupt handler */
	for(j=0; j < 1000 && ir_stop_interrupts != 0; j++)
		snooze(10);
	if (j >= 1000)
		dprintf("ir: hmmm, int_handler couldn't shut off interrupts!\n");


	reset_controller(ird);         /* the only clean way to exit a read... */

	if (ird->state & LISTEN_MODE) {
		ird->state &= ~LISTENING;
		release_sem(ir_big_sem);
	} else { /* we're in sample mode */
		ird->state &= ~SAMPLING;
		release_sem_etc(ir_big_sem, 3, 0);
	}
	
	return 0;
}


/*
	ir_write
*/

int in_write_mode = 0;

static status_t
ir_write(void *cookie, off_t pos, const void *_buf, size_t *count)
{
	ir_data	*ird=(ir_data *)cookie;
	const char *buf = (char *)_buf;
	ulong i, j, first=16;
	ushort tmp;

	/* dprintf("ir%d: writing %d bytes:\n", ird->port_num+1, *count); */
	
	if (*count < 16)
		first = *count;

	if (acquire_sem_etc(ir_big_sem, 3, B_CAN_INTERRUPT, 0) < B_NO_ERROR)
		return 0;
	
	/*
	   prime the IR controller with up to the first 16 bytes of data
	*/
	in_write_mode = 1;
	for(i=0; i < first; i++, buf++) {
		tmp = IR_HOST_DATA | ((*buf) & 0xff);
		write_response_reg(0);         /* clears out the upper bits */
		write_cmd_reg(tmp);

		if (acquire_sem_etc(ir_write_sem, 1, B_CAN_INTERRUPT, 0) < 0)
			break;
	}
	in_write_mode = 0;

	/*
	   Now start it sending that data
	*/   
	if (start_send(ird) < 0) {
		dprintf("start_send from inside of write appears to have failed.\n");
		release_sem_etc(ir_big_sem, 3, 0);
		return B_IO_ERROR;
	}

	/*
	   and now finish sending the rest of the data
	*/
	in_write_mode = 1;
	for(; i < *count; i++, buf++) {
		tmp = IR_HOST_DATA | ((*buf) & 0xff);
		write_response_reg(0);         /* clears out the upper bits */
		write_cmd_reg(tmp);

		if (acquire_sem_etc(ir_write_sem, 1, B_CAN_INTERRUPT, 0) < 0)
			break;
	}

	*count = i;
	in_write_mode = 0;

	tmp = ir_get_ack(ird, B_CAN_INTERRUPT);
	if ((tmp & IR_MSG_MASK) != LAST_BYTE_SENT) {
		dprintf("ir: last data interrupt got 0x%x instead of 0x%x\n",
				tmp, LAST_BYTE_SENT);
		release_sem_etc(ir_big_sem, 3, 0);
		return B_IO_ERROR;
	}

	release_sem_etc(ir_big_sem, 3, 0);
	return 0;
}


static int
get_ack(ir_data *ird)
{
	ird->response = ir_get_ack(ird, B_CAN_INTERRUPT);

	if ((ird->response & ACK_MASK) != (ird->last_cmd & ACK_MASK)) {
		dprintf("ir%d: bad ack (expected: 0x%x, response = 0x%x)\n",
				ird->port_num+1, ird->last_cmd, ird->response);
	}
	return ((ird->response & ACK_MASK) == (ird->last_cmd & ACK_MASK));
}


#define ddprintf 

static int
reset_controller(ir_data *ird)
{
	int res, i, ret = B_NO_ERROR, count;

	/* disable interrupts first in case it is spewing */
	remove_io_interrupt_handler(B_INT_IR, ir_int_handler,
								isa_io + IR_RESPONSE_REG);
	write_response_reg(0);
	snooze(100);
	
	/* now eat up any outstanding acks/data */
	ddprintf("%d bytes of unread data and %d unread acks\n",
			 ir_how_much_data(ird), ir_how_many_acks());

	count = ir_how_much_data(ird);
	for(i=0; i < count; i++) {
		ddprintf("reading %d of %d data words\n", i, count);
		if (ir_get_data(ird, B_CAN_INTERRUPT) == IR_BAD_READ)
			break;
	}
	   
	count = ir_how_many_acks();
	for(i=0; i < count; i++) {
		ddprintf("reading %d of %d acks\n", i, count);
		if (ir_get_ack(ird, B_CAN_INTERRUPT) == IR_BAD_READ)
			break;
	}

	write_response_reg(0xffff);
	spin(IR_DELAY);
	write_cmd_reg(0xffff);
	spin(IR_DELAY);
	write_cmd_reg(0x0000);
	spin(IR_DELAY);
	write_response_reg(0x0000);

	install_io_interrupt_handler(B_INT_IR, ir_int_handler,
		isa_io + IR_RESPONSE_REG, NULL);
	spin(IR_DELAY);


	res = ir_get_ack(ird, B_CAN_INTERRUPT | B_TIMEOUT);

	if ((res & IR_MSG_MASK) != RESET_DONE) {
		dprintf("reset: did not get RESET_DONE (res == 0x%x)\n", res);
		return B_TIMED_OUT;
	}

	
	for (i=0; i < NUM_IR_PORTS; i++) {
		if (ir_ports[i].open == NUM_OPENS)  /* do this only for opened ports */
			continue;
		
		ret = load_settings(&ir_ports[i], &(ir_ports[i].settings[0]));
		if (ret < 0)
			break;
		ret = set_logical_low(&ir_ports[i]);
		if (ret < 0)
			break;

		if (&ir_ports[i] != ird && (ir_ports[i].state & LISTENING)) {
			/* dprintf("turning on listen mode for port %d\n", i+1); */
			start_listen(&ir_ports[i]);
		}
	}

	return ret;
}


static int
start_sample(ir_data *ird)
{
	write_cmd_reg(START_SAMPLE(ird->port_num));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	return B_NO_ERROR;
}

static int
start_send(ir_data *ird)
{
	write_response_reg(0xb00);
	write_cmd_reg(START_SEND(ird->port_num));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	return B_NO_ERROR;
}


static int
set_logical_low(ir_data *ird)
{
	write_cmd_reg(SET_LOGICAL_LOW(ird->port_num, ird->logical_low));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	return B_NO_ERROR;
}

static int
start_listen(ir_data *ird)
{
	write_cmd_reg(START_LISTEN(ird->port_num));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	return B_NO_ERROR;
}

static int
stop_analysis(ir_data *ird)
{
	write_cmd_reg(STOP_ANALYSIS(ird->port_num));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	return B_NO_ERROR;
}


static int
start_analysis(ir_data *ird)
{
	write_cmd_reg(START_ANALYSIS(ird->port_num));
	
	if (!get_ack(ird)) {
		dprintf("start_analysis: bad ack 0x%x (0x%x)\n", ird->response,
				read_response_reg);
		return B_IO_ERROR;
	}

	return B_NO_ERROR;
}



static int
send_analysis(ir_data *ird, void *udata)
{
	int *data = (int *)udata;

	write_cmd_reg(SEND_ANALYSIS(ird->port_num));

	if (!get_ack(ird)) {
		dprintf("send_analysis: bad ack 0x%x (0x%x)\n", ird->response,
				read_response_reg);
		return B_IO_ERROR;
	}

	data[0] = ir_get_ack(ird, B_CAN_INTERRUPT) & 0xff;  /* ir address  */
	data[1] = ir_get_ack(ird, B_CAN_INTERRUPT) & 0xff;  /* ir settings */

	ird->settings[0] = data[0];
	ird->settings[1] = data[1];

	return B_NO_ERROR;
}

static int
load_settings(ir_data *ird, void *udata)
{
	int *data = (int *)udata;
	
	ddprintf("issuing a LOAD_SETTINGS (0x%x, 0x%x) command for port %d\n",
			data[0], data[1], ird->port_num+1);
	write_cmd_reg(LOAD_SETTINGS(ird->port_num));
	
	if (!get_ack(ird))
		return B_IO_ERROR;

	ddprintf("loading control address.\n");
	write_cmd_reg((short)data[0] | IR_HOST_DATA); /* remote control address */

	if (!get_ack(ird))
		return B_IO_ERROR;

	ddprintf("loading control settings.\n");
	write_cmd_reg((short)data[1] | IR_HOST_DATA); /* remote control settings */

	if (!get_ack(ird))
		return B_IO_ERROR;

	ird->settings[0] = data[0] & 0xff;
	ird->settings[1] = data[1] & 0xff;

	ddprintf("done loading settings.\n------------------------------------\n");

	return B_NO_ERROR;
}


static status_t
ir_control(void *cookie, uint32 msg, void *buf, size_t len)
{
	int ret=B_NO_ERROR;
	ir_data *ird=(ir_data *)cookie;
	
	/* dprintf("ir: got control msg: 0x%x\n", msg); */

	if (acquire_sem_etc(ir_big_sem, 3, B_CAN_INTERRUPT, 0) < B_NO_ERROR)
		return B_INTERRUPTED;

	switch (msg) {
	case IR_RESET:
		ret = reset_controller(ird);
		break;

	case IR_SET_SAMPLE_MODE:
		ird->state = (ird->state & ~MODE_MASK) | SAMPLE_MODE;
		break;

	case IR_SET_LISTEN_MODE:
		ird->state = (ird->state & ~MODE_MASK) | LISTEN_MODE;
		break;

	case IR_SET_LOGICAL_LOW:
		ret = set_logical_low(ird);
		break;

	case IR_START_ANALYSIS:
		ret = start_analysis(ird);
		break;

	case IR_STOP_ANALYSIS:
		ret = stop_analysis(ird);
		break;

	case IR_SEND_ANALYSIS:
		ret = send_analysis(ird, buf);
		break;

	case IR_LOAD_SETTINGS:
		ret = load_settings(ird, buf);
		break;

	default:
		ret = B_ERROR;
		break;
	}

	release_sem_etc(ir_big_sem, 3, 0);

	return ret;
}




static int32
ir_int_handler(void *arg)
{
	vshort *response_reg=(short *)arg;
	static int counter=0;
	ushort data;
	
	spin(10);

	data = B_LENDIAN_TO_HOST_INT16(*response_reg);
	__eieio();

	if (ir_stop_interrupts) {
		remove_io_interrupt_handler(B_INT_IR, ir_int_handler,
									isa_io + IR_RESPONSE_REG);
		*(vshort *)response_reg = 0;
		__eieio();
		ir_stop_interrupts = 0;
	} else if ((data & IR_CONTROL_MASK) == 0 && in_write_mode) {
		release_sem(ir_write_sem);
	} else if ((data & IR_CONTROL_MASK) == IR_CMD_ACK) {
		ir_put_ack((short)data);
	} else {
		ir_put_data((short)data);
	}

	return B_HANDLED_INTERRUPT;
}
