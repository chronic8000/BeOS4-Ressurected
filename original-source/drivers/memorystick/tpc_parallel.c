#include "tpc.h"
#include "tpc_parallel_dma.h"
#include <ISA.h>
#include <KernelExport.h>

#define IPC_WRITE 	0x00
#define IPC_READ	0x80
#define IPC_BLOCK 	0x40

#define ECP_BASE	0x0378
#define ECP_IRQ     7
#define ECP_DMA     3

#define ECP_CMD		(ECP_BASE)
#define ECP_DATA 	(ECP_BASE+0x0400)
#define ECP_ECR 	(ECP_BASE+0x0402)
#define ECP_DSR 	(ECP_BASE+0x0001)
#define ECP_DCR		(ECP_BASE+0x0002)

#define SIO_INDEX 	(0x002e)
#define SIO_DATA	(0x002f)

#define POLL_INTERRUPT 1
#define USE_DMA 0

isa_module_info *isa;
dma_info_t dma_info;
#if POLL_INTERRUPT
bool got_int;
#else
static sem_id int_sem;
#endif

uint16 crc16_table[256];

typedef struct {
	timer      t;
	bigtime_t  last_empty_time;
	bool       media_present;
	bool       media_changed;
} media_check_timer_t;

media_check_timer_t media_check_timer;

static void
init_crc16()
{
	int i, b;
	uint16 r;
	for(i = 0; i < 256; i++) {
		r = i << 8;
		for(b = 0; b < 8; b++) {
			if(r & 0x8000)
				r = (r << 1) ^ 0x8005;
			else
				r <<= 1;
		}
		crc16_table[i] = r;
	}
}

#define add_crc16(crc, data) crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ (data)]
#if 0
static void
add_crc16(uint16 *crcp, uint8 data)
{
	uint16 crc = *crcp;
	*crcp = (crc << 8) ^ crc16_table[(crc >> 8) ^ data];
}
#endif

#if 0
static void
add_crc16_2(uint16 *crcp, uint8 data)
{
	int b;
	uint16 crc = *crcp;
	for(b = 0; b < 8; b++) {
		int in = !!(data & 1 << (7-b));
		int a = !!(crc & 0x8000) ^ in;
		crc = ((crc << 1) & ~0x8005) |
		      ((a ^ !!(crc & 0x4000)) << 15) |
		      ((a ^ !!(crc & 0x0002)) << 2) |
		      a;
	}
	*crcp = crc;
}
#endif
#if 0
static uint16
crc16(uint8 *data, size_t data_len)
{
	int i;
	uint16 crc = 0;
	for(i = 0; i < data_len; i++) {
#if 1
		crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ data[i]];
#else
		int b;
		for(b = 0; b < 8; b++) {
			int in = !!(data[i] & 1 << (7-b));
			int a = !!(crc & 0x8000) ^ in;
			crc = ((crc << 1) & ~0x8005) |
			      ((a ^ !!(crc & 0x4000)) << 15) |
			      ((a ^ !!(crc & 0x0002)) << 2) |
			      a;
		}
#endif
	}
	return crc;
}
#endif

static void
clear_int()
{
#if POLL_INTERRUPT
	got_int = false;
#else
	int clear_count = 0;
	while(acquire_sem_etc(int_sem, 1, B_RELATIVE_TIMEOUT, 0) == B_NO_ERROR) {
		clear_count++;
		if(clear_count > 1000)
			break;
	}
	if(clear_count > 0)
		dprintf("parallelms -- clear_int: cleared %d pending interrupts\n",
		        clear_count);
#endif
}

uint8 ecp_dcr;

static void
set_ecp_direction_in()
{
	ecp_dcr |= 0x20;
	isa->write_io_8(ECP_DCR, ecp_dcr);
}

static void
set_ecp_direction_out()
{
	ecp_dcr &= ~0x20;
	isa->write_io_8(ECP_DCR, ecp_dcr);
}

status_t 
tpc_reset()
{
	// Reset device
	ecp_dcr = 0x08;
	isa->write_io_8(ECP_DCR, ecp_dcr);	// reset low (reset mode)
	//snooze(500000);
	snooze(50000);
	ecp_dcr = 0x00;
	isa->write_io_8(ECP_DCR, ecp_dcr);	// reset high (active forward mode)
	//snooze(1000000);
	snooze(50000);
	return B_NO_ERROR;
}

static int32
parallel_interrupt(void *data)
{
//	kprintf("got parallel int\n");
#if POLL_INTERRUPT
	got_int = true;
	return B_HANDLED_INTERRUPT;
#else
	release_sem_etc(int_sem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
#endif
}

status_t 
tpc_wait_int(bigtime_t timeout)
{
#if POLL_INTERRUPT
	bigtime_t t1;
	t1 = system_time();
	while(!got_int) {
		if(system_time() - t1 > timeout) {
			dprintf("parallelms -- tpc_wait_int: no interrupt\n");
			return B_TIMED_OUT;
		}
		if(system_time() - t1 > 1000)
			snooze(1000);
		else
			spin(2);
	}
	/*dprintf("parallelms -- tpc_wait_int: got int after %Ld us\n",
	        system_time() - t1);*/
	return B_NO_ERROR;
#else
	return acquire_sem_etc(int_sem, 1, B_RELATIVE_TIMEOUT, timeout);
#endif
}

int tpc_read_count = 0;
int tpc_read_error_count = 0;
bigtime_t tpc_read_setup_time = 0;
bigtime_t tpc_read_data_time = 0;
bigtime_t tpc_read_crc_time = 0;
uint64 tcp_read_bytes = 0;

extern void get_dma_count_and_status (int channel, int *pcount, int *pstatus);

#if USE_DMA
status_t
tpc_read_dma(uint8 tpc, uint8 *data, size_t data_len)
{
	const char *debugprefix = "parallelms -- tpc_read_dma: ";
	int i;
	//bigtime_t t1;
	uint16 crc;
	cpu_status ps;
	int retry_count = 3;
	//bigtime_t setup_time, data_time, stop_dma_time, crc_time;
	status_t err;

	if(((uint8*)dma_info.dma_buffer)[data_len+1] != 0xff) {
		((uint8*)dma_info.dma_buffer)[data_len+1] = 0xff;
	}
	else {
		((uint8*)dma_info.dma_buffer)[data_len+1] = 0xfe;
	}
retry:
	//((uint8*)dma_info.dma_buffer)[0] = 0xbe;
	//t1 = system_time();
	err = start_dma(&dma_info, data_len+2, false);
	if(err != B_NO_ERROR)
		return err;

	set_ecp_direction_out();
	clear_int();
	ps = disable_interrupts();
	/* IPC read data_len + 2 crc bytes */
	if(data_len == 512)
		isa->write_io_8(ECP_CMD, IPC_READ | IPC_BLOCK);
	else
		isa->write_io_8(ECP_CMD, IPC_READ | (data_len + 2 - 1));
	isa->write_io_8(ECP_DATA, tpc);
	//isa->write_io_8(ECP_DCR, 0x20);
	//spin(100);
	set_ecp_direction_in();
	isa->write_io_8(ECP_ECR, 0x7b);
	restore_interrupts(ps);
	//setup_time = system_time()-t1;
	//data_time = system_time();
	err = tpc_wait_int(1000);
	//err = acquire_sem_etc(int_sem, 1, B_RELATIVE_TIMEOUT, 1000);
	if(err != B_NO_ERROR) {
		dprintf("%s1ms timeout waiting for dma\n", debugprefix);
		abort_dma(&dma_info);
		isa->write_io_8(ECP_ECR, 0x77);
		err = B_TIMED_OUT;
		goto err;
	}
	//data_time = system_time() - data_time;
	//stop_dma_time = system_time();
	//dprintf("%sdma buffer now 0x%02x\n", debugprefix, ((uint8*)dma_info.dma_buffer)[0]);
	err = finish_dma(&dma_info);
	isa->write_io_8(ECP_ECR, 0x77);
	if(err != B_NO_ERROR)
		goto err;
		
	//stop_dma_time = system_time() - stop_dma_time;

	//crc_time = system_time();

	crc = 0;
	{
		uint32 *dp, *edp, *sp;
		sp = dma_info.dma_buffer;
		dp = data;
		edp = data + data_len;
		while(dp < edp) {
			uint32 sw = *sp++;
			*dp++ = sw;
			add_crc16(crc, sw & 0xff);
			sw >>= 8;
			add_crc16(crc, sw & 0xff);
			sw >>= 8;
			add_crc16(crc, sw & 0xff);
			sw >>= 8;
			add_crc16(crc, sw & 0xff);
		}
		add_crc16(crc, ((uint8*)sp)[0]);
		add_crc16(crc, ((uint8*)sp)[1]);
	}

	//crc = ((uint8*)dma_info.dma_buffer)[data_len] << 8 |
	//      ((uint8*)dma_info.dma_buffer)[data_len+1];
	//memcpy(data, dma_info.dma_buffer, data_len);

	//if(crc16(data, data_len) != crc) {
	if(crc != 0) {
		//dprintf("%scrc mismatch got 0x%04x should be 0x%04x\n",
		//        debugprefix, crc, crc16(data, data_len));
		dprintf("%scrc mismatch rem 0x%04x\n", debugprefix, crc);
		dprintf("data:");
		for(i = 0; i < data_len; i++) {
			dprintf(" %02x", data[i]);
		}
		dprintf("\n");
		goto err;
	}
	//crc_time = system_time() - crc_time;
	//dprintf("%ss %Ld us, d %Ld us, f %Ld, c %Ld us\n",
	//        debugprefix, setup_time, data_time, stop_dma_time, crc_time);
	//tpc_read_setup_time += setup_time;
	//tpc_read_data_time += data_time;
	//tpc_read_crc_time += crc_time;
	//tpc_read_count++;
	//tcp_read_bytes += data_len;
	return B_NO_ERROR;

err:
	tpc_read_error_count++;
	isa->write_io_8(ECP_ECR,0x17);  // clear FIFO
	isa->write_io_8(ECP_ECR,0x77);
	if(retry_count-- > 0) {
		dprintf("%sretry (%d retries left)\n", debugprefix, retry_count);
		snooze(1000);
		goto retry;
	}
	dprintf("%scommand failed\n", debugprefix);
	return B_ERROR;
}
#endif

#if 0
int fifo_empty_count = 0;
int fifo_full_count = 0;
int fifo_byte_count = 0;
#endif

status_t
tpc_read_pio(uint8 tpc, uint8 *data, size_t data_len)
{
	const char *debugprefix = "parallelms -- tpc_read_pio: ";
	int i;
	bigtime_t t1;
	//,t2;
	uint16 crc;
	cpu_status ps;
	int retry_count = 3;
	int fifo_count = 0;
	//bigtime_t setup_time, data_time, crc_time;

retry:
	//t1 = system_time();
	set_ecp_direction_out();
	/* IPC read data_len + 2 crc bytes */
	clear_int();
	ps = disable_interrupts();
	if(data_len == 512)
		isa->write_io_8(ECP_CMD, IPC_READ | IPC_BLOCK);
	else
		isa->write_io_8(ECP_CMD, IPC_READ | (data_len + 2 - 1));
	isa->write_io_8(ECP_DATA, tpc);
	//isa->write_io_8(ECP_DCR, 0x20);
	//spin(100);
	set_ecp_direction_in();
	restore_interrupts(ps);
	t1 = system_time();
	for(;;) {
		uint8 dsr, ecr;
		
		dsr = isa->read_io_8(ECP_DSR);
		if((dsr & 0x20) == 0) {
			if((dsr & 0x10) == 0) {
				return B_DEV_NO_MEDIA;
			}
			dprintf("%stimeout, DSR = 0x%02x\n", debugprefix, dsr);
			goto err;
		}
		ecr = isa->read_io_8(ECP_ECR);
		if((ecr & 0x01) == 0) {
			//dprintf("%sfetching data:\n", debugprefix);
			break;
		}
		if(system_time() - t1 > 100000) {
			//dprintf("%s100ms timeout ecr 0x%02x\n", debugprefix, ecr);
			goto err;
		}
		spin(2);
		//snooze(1000);
	}
	crc = 0;
	//t2 = system_time();
	//setup_time = t2-t1;
	//data_time = t2;
	fifo_count = 1;
	//dprintf("parallelms: ms_read got data after %Ld us\n", t2-t1);
	for(i = 0; i < data_len + 2; i++) {
		uint8 byte;
		if(fifo_count <= 0) {
			uint8 ecr;
			ecr = isa->read_io_8(ECP_ECR);
			if(ecr & 0x02) {
				fifo_count = 16;
				//fifo_full_count++;
			}
			else if((ecr & 0x01) == 0x00) {
				fifo_count = 1;
			}
			else {
				//fifo_empty_count++;
				t1 = system_time();
				while(ecr & 0x01) {
					if(system_time() - t1 > 100000) {
						//dprintf("%s100000ms timeout ecr 0x%02x got %d of %d "
						//        "bytes\n", debugprefix, ecr, i, data_len+2);
						goto err;
					}
					snooze(1000);
					ecr = isa->read_io_8(ECP_ECR);
				}
			}
		}
		byte = isa->read_io_8(ECP_DATA);
		fifo_count--;
		//fifo_byte_count++;
		add_crc16(crc, byte);
		if(i < data_len)
			data[i] = byte;
#if 0
		dprintf("%s0x%02x : 0x%02X crc 0x%04x\n", debugprefix, i, byte, crc);
#endif
	}
#if 0
	if(fifo_byte_count > 5140) {
		dprintf("fifo e %d f %d\n", fifo_empty_count, fifo_full_count);
		fifo_byte_count = 0;
		fifo_empty_count = 0;
		fifo_full_count = 0;
	}
#endif
	//data_time = system_time() - data_time;
	//crc_time = system_time();

	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}
	{
		uint8 dsr = isa->read_io_8(ECP_DSR);
		if((dsr & 0x10) == 0) {
			return B_DEV_NO_MEDIA;
		}
	}

	if(crc != 0) {
		dprintf("%scrc mismatch rem 0x%04x\n", debugprefix, crc);
		dprintf("data:");
		for(i = 0; i < data_len; i++) {
			dprintf(" %02x", data[i]);
		}
		dprintf("\n");
		goto err;
	}
	//crc_time = system_time() - crc_time;
#if 0
	tpc_read_setup_time += setup_time;
	tpc_read_data_time += data_time;
	tpc_read_crc_time += crc_time;
	tpc_read_count++;
	tcp_read_bytes += data_len;
#endif
	return B_NO_ERROR;

err:
	//tpc_read_error_count++;
	isa->write_io_8(ECP_ECR,0x17);  // clear FIFO
	isa->write_io_8(ECP_ECR,0x77);
	if(retry_count-- > 0) {
		//dprintf("%sretry (%d retries left)\n", debugprefix, retry_count);
		snooze(10000);
		goto retry;
	}
	dprintf("%scommand failed\n", debugprefix);
	return B_ERROR;
}

status_t
tpc_read(uint8 tpc, uint8 *data, size_t data_len)
{
	const char *debugprefix = "parallelms -- tpc_read: ";
	if(tpc == TPC_READ_PAGE_DATA && data_len != 512) {
		dprintf("%sTPC_READ_PAGE_DATA data len %ld wrong\n",
		        debugprefix, data_len);
		return B_ERROR;
	}
	if(tpc != TPC_READ_PAGE_DATA && data_len > 0x40 - 2) {
		dprintf("%sdata len %ld too big\n", debugprefix, data_len);
		return B_ERROR;
	}
	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}
	
#if 0
	if(tpc_read_count % 1000 == 0 && tpc_read_count > 0 && tcp_read_bytes > 0) {
		dprintf("tcp_read: count %d error %d avg time: s %Ld ns dpb %Ld ns cpb %Ld ns\n",
		        tpc_read_count, tpc_read_error_count,
		        tpc_read_setup_time * 1000 / tpc_read_count,
		        tpc_read_data_time  * 1000 / tcp_read_bytes,
		        tpc_read_crc_time  * 1000 / tcp_read_bytes);
	}
#endif
#if USE_DMA
	if(data_len == 512)
		return tpc_read_dma(tpc, data, data_len);
	else
#endif
		return tpc_read_pio(tpc, data, data_len);
}

status_t
tpc_write(uint8 tpc, const uint8 *data, size_t data_len)
{
	const char *debugprefix = "parallelms -- tpc_write: ";
	int i;
	uint16 crc;
	uint8 dsr;
	cpu_status ps;

	if(tpc == TPC_WRITE_PAGE_DATA && data_len != 512) {
		dprintf("%sTPC_READ_PAGE_DATA data len %ld wrong\n",
		        debugprefix, data_len);
		return B_ERROR;
	}
	if(tpc != TPC_WRITE_PAGE_DATA && data_len > 0x40 - 2) {
		dprintf("%sdata len %ld too big\n", debugprefix, data_len);
		return B_ERROR;
	}
	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}

	//crc = crc16(data, data_len);
	crc = 0;
	set_ecp_direction_out();
	isa->write_io_8(ECP_ECR, 0x67);
	clear_int();

	ps = disable_interrupts();
	/* IPC write data_len + 2 crc bytes */
	if(data_len == 512)
		isa->write_io_8(ECP_CMD, IPC_WRITE | IPC_BLOCK);
	else
		isa->write_io_8(ECP_CMD, IPC_WRITE | (data_len + 2 - 1));
	isa->write_io_8(ECP_DATA, tpc);
	restore_interrupts(ps);
	for(i = 0; i < data_len; i++) {
		isa->write_io_8(ECP_DATA, data[i]);
		add_crc16(crc, data[i]);
	}
	isa->write_io_8(ECP_DATA, crc >> 8);
	isa->write_io_8(ECP_DATA, crc & 0xff);
	spin(2);
	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}
	dsr = isa->read_io_8(ECP_DSR);
	if((dsr & 0x20) == 0) {
		if((dsr & 0x10) == 0) {
			return B_DEV_NO_MEDIA;
		}
		dprintf("%stimeout, DSR = 0x%02x\n", debugprefix, dsr);
		return B_ERROR;
	}
	return B_NO_ERROR;
}

#if 0

/* test code, send packet and turn off power */

status_t
tpc_write_drop_power(uint8 tpc, const uint8 *data, size_t data_len)
{
	const char *debugprefix = "parallelms -- tpc_write_drop_power: ";
	int i;
	uint16 crc;
	uint8 dsr;
	cpu_status ps;

	if(tpc == TPC_WRITE_PAGE_DATA && data_len != 512) {
		dprintf("%sTPC_READ_PAGE_DATA data len %ld wrong\n",
		        debugprefix, data_len);
		return B_ERROR;
	}
	if(tpc != TPC_WRITE_PAGE_DATA && data_len > 0x40 - 2) {
		dprintf("%sdata len %ld too big\n", debugprefix, data_len);
		return B_ERROR;
	}
	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}

	//crc = crc16(data, data_len);
	crc = 0;
	set_ecp_direction_out();
	isa->write_io_8(ECP_ECR, 0x67);
	clear_int();

	ps = disable_interrupts();
	/* IPC write data_len + 2 crc bytes */
	if(data_len == 512)
		isa->write_io_8(ECP_CMD, IPC_WRITE | IPC_BLOCK);
	else
		isa->write_io_8(ECP_CMD, IPC_WRITE | (data_len + 2 - 1));
	isa->write_io_8(ECP_DATA, tpc);
	restore_interrupts(ps);
	for(i = 0; i < data_len; i++) {
		isa->write_io_8(ECP_DATA, data[i]);
		add_crc16(crc, data[i]);
	}
	isa->write_io_8(ECP_DATA, crc >> 8);
	isa->write_io_8(ECP_DATA, crc & 0xff);

	ecp_dcr = 0x08;
	isa->write_io_8(ECP_DCR, ecp_dcr);	// reset low (reset mode)
	return B_NO_ERROR;

	spin(2);
	if(media_check_timer.media_changed) {
		/* return error until tpc_get_media_status is called */
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}
	dsr = isa->read_io_8(ECP_DSR);
	if((dsr & 0x20) == 0) {
		if((dsr & 0x10) == 0) {
			return B_DEV_NO_MEDIA;
		}
		dprintf("%stimeout, DSR = 0x%02x\n", debugprefix, dsr);
		return B_ERROR;
	}
	return B_NO_ERROR;
}
#endif

static status_t
configure_parallel_port()
{
	const char *debugprefix = "parallelms -- configure_parallel_port: ";
	/* turn off RTS on COM2 as this signal for some reason are wired to the
	   Select signal on the parallel port preventing media detection if
	   driven */
	isa->write_io_8(0x2f8 + 4, isa->read_io_8(0x2f8 + 4) & 0xfd);

	isa->write_io_8(SIO_INDEX, 07);
	isa->write_io_8(SIO_DATA, 0x08);
	{
		uint16 pmp;
		uint8 active;
		isa->write_io_8(SIO_INDEX,0x30);
		active = isa->read_io_8(SIO_DATA);
		//dprintf("SIO device 8, active: 0x%02x\n", active);
		if((active & 0x01) == 0) {
			active |= 0x01;
			isa->write_io_8(SIO_DATA, active);
		}
		isa->write_io_8(SIO_INDEX,0x60);
		//dprintf("SIO device 8, ba msb: 0x%02x\n", isa->read_io_8(SIO_DATA));
		pmp = isa->read_io_8(SIO_DATA) << 8;
		isa->write_io_8(SIO_INDEX,0x61);
		//dprintf("SIO device 8, ba lsb: 0x%02x\n", isa->read_io_8(SIO_DATA));
		pmp |= isa->read_io_8(SIO_DATA);
		
#if 0
		isa->write_io_8(pmp,0);
		dprintf("SIO device 8, offset 0: 0x%02x\n", isa->read_io_8(pmp+1));
		isa->write_io_8(pmp,1);
		dprintf("SIO device 8, offset 1: 0x%02x\n", isa->read_io_8(pmp+1));
		isa->write_io_8(pmp,2);
		dprintf("SIO device 8, offset 2: 0x%02x\n", isa->read_io_8(pmp+1));
		isa->write_io_8(pmp,3);
		dprintf("SIO device 8, offset 3: 0x%02x\n", isa->read_io_8(pmp+1));
#endif
		isa->write_io_8(pmp,4);
//		dprintf("SIO device 8, offset 4: 0x%02x\n", isa->read_io_8(pmp+1));
		isa->write_io_8(pmp+1, isa->read_io_8(pmp+1) | 0x02);
#if 0
		isa->write_io_8(pmp,5);
		dprintf("SIO device 8, offset 5: 0x%02x\n", isa->read_io_8(pmp+1));
		isa->write_io_8(pmp,6);
		dprintf("SIO device 8, offset 6: 0x%02x\n", isa->read_io_8(pmp+1));
#endif
	}

	//
	//  see page 138 of SuperI/O documentation for details
	//
	// set Super I/O device 4 (Parallel Port)
	isa->write_io_8(SIO_INDEX, 07);
	isa->write_io_8(SIO_DATA, 0x04);

	{ /* check config */
		uint16 base;
		uint8 irq;
		uint8 dma;
		isa->write_io_8(SIO_INDEX, 0x30);
		if((isa->read_io_8(SIO_DATA) & 0x01) == 0)
			dprintf("%sdevice 4 (parallel port) is not active\n", debugprefix);
		isa->write_io_8(SIO_INDEX, 0x60);
		base = isa->read_io_8(SIO_DATA);
		isa->write_io_8(SIO_INDEX, 0x61);
		base = base << 8 | isa->read_io_8(SIO_DATA);
		isa->write_io_8(SIO_INDEX, 0x70);
		irq = isa->read_io_8(SIO_DATA);
		if(irq != ECP_IRQ) {
			dprintf("%sirq is wrong, %d\n", debugprefix, irq);
			isa->write_io_8(SIO_DATA, ECP_IRQ);
		}
		isa->write_io_8(SIO_INDEX, 0x71);
		dprintf("%sirq type 0x%02x\n", debugprefix, isa->read_io_8(SIO_DATA));
		isa->write_io_8(SIO_DATA, 0x03);
		dprintf("%sirq type 0x%02x\n", debugprefix, isa->read_io_8(SIO_DATA));

		isa->write_io_8(SIO_INDEX, 0x74);
		dma = isa->read_io_8(SIO_DATA);
		if(dma != ECP_DMA) {
			dprintf("%sdma channel is wrong, %d\n", debugprefix, dma);
			//isa->write_io_8(SIO_DATA, ECP_DMA);
			isa->write_io_8(SIO_DATA, ECP_DMA);
		}
		dma_info.dma_channel = ECP_DMA;
		//else {
		//	dma_info.dma_channel = 1;
		//	isa->write_io_8(SIO_DATA, 1);
		//}
		
		if(base != ECP_BASE)
			dprintf("%sbase is wrong 0x%04x\n", debugprefix, base);
	}
#if 0
	isa->write_io_8(SIO_INDEX, 0x30);
	dprintf("DEV4 active reg (0x30):  0x%02x\n", isa->read_io_8(SIO_DATA));
	isa->write_io_8(SIO_INDEX, 0x60);
	dprintf("DEV4 base addr msb (0x60):  0x%02x\n", isa->read_io_8(SIO_DATA));
	isa->write_io_8(SIO_INDEX, 0x61);
	dprintf("DEV4 base addr lsb (0x61):  0x%02x\n", isa->read_io_8(SIO_DATA));
#endif

	// set to ECP MODE
	isa->write_io_8(SIO_INDEX,0xf0);
//	dprintf("old mode: 0x%02x\n", isa->read_io_8(SIO_DATA));
	isa->write_io_8(SIO_DATA,0x12);
	isa->write_io_8(ECP_DCR, 0xc4);
	isa->write_io_8(SIO_INDEX,0xf0);
//	dprintf("old mode: 0x%02x\n", isa->read_io_8(SIO_DATA));
	isa->write_io_8(SIO_DATA,0xf2);
	
	// Configure ECR to ECP mode & disable IRQ and DMA
	isa->write_io_8(ECP_ECR,0x17);  // clear FIFO
	isa->write_io_8(ECP_ECR,0x77);

	// ecr =
	//isa->read_io_8(ECP_ECR);

	isa->write_io_8(ECP_BASE+0x403, 0);
	//dprintf("ECP CR0: 0x%02x\n", isa->read_io_8(ECP_BASE+0x404));
	isa->write_io_8(ECP_BASE+0x403, 2);
	//dprintf("ECP CR2: 0x%02x\n", isa->read_io_8(ECP_BASE+0x404));
	isa->write_io_8(ECP_BASE+0x404, isa->read_io_8(ECP_BASE+0x404) & ~0x40);
	
	isa->write_io_8(ECP_BASE+0x403, 4);
	//dprintf("ECP CR4: 0x%02x\n", isa->read_io_8(ECP_BASE+0x404));
	isa->write_io_8(ECP_BASE+0x403, 5);
	//dprintf("ECP PPCf0: 0x%02x\n", isa->read_io_8(ECP_BASE+0x404));
	isa->write_io_8(ECP_BASE+0x404, isa->read_io_8(ECP_BASE+0x404) & 0xfb);
	return B_NO_ERROR;
}

static int32
check_media_timer_hook(timer *t)
{
	//const char *debugprefix = "parallelms -- check_media_timer_hook: ";
	media_check_timer_t *mct = (media_check_timer_t *)t;
	uint8 dsr;
	dsr = isa->read_io_8(ECP_DSR);
	if((dsr & 0x10) == 0) {
		if(mct->media_present) {
			ecp_dcr = 0x08;
			isa->write_io_8(ECP_DCR, ecp_dcr);	// turn off power

			mct->media_changed = true;
			//dprintf("%smemory stick removed\n", debugprefix);
		}
		mct->media_present = false;
	}
	else {
		//if(!mct->media_present)
		//	dprintf("%smemory stick inserted, debugprefix\n");
		mct->media_present = true;
	}
	return B_HANDLED_INTERRUPT;
}

status_t 
tpc_get_media_status(bigtime_t last_checked)
{
	uint8 dsr;
	
	if(media_check_timer.media_changed) {
		media_check_timer.media_changed = false;
		media_check_timer.last_empty_time = system_time();
	}
	if(media_check_timer.last_empty_time > last_checked) {
		if(media_check_timer.media_present)
			return B_DEV_MEDIA_CHANGED;
		else
			return B_DEV_NO_MEDIA;
	}

	dsr = isa->read_io_8(ECP_DSR);
	if((dsr & 0x10) == 0) {
		return B_DEV_NO_MEDIA;
	}

	return B_NO_ERROR;
}

status_t 
tpc_init()
{
	status_t err;
	
	err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa);
	if(err != B_NO_ERROR)
		goto err1;
	
	configure_parallel_port();

	ecp_dcr = 0x08;
	isa->write_io_8(ECP_DCR, ecp_dcr);	// turn off power

	err = install_io_interrupt_handler(ECP_IRQ, parallel_interrupt, NULL, 0);
	if(err != B_NO_ERROR)
		goto err2;
	
	err = create_dma_area(&dma_info);
	if(err != B_NO_ERROR)
		goto err3;

#if POLL_INTERRUPT
	got_int = false;
#else
	err = int_sem = create_sem(0, "memory stick interrupt sem");
	if(err < B_NO_ERROR)
		goto err4;
#endif
	media_check_timer.last_empty_time = 0;
	media_check_timer.media_present = false;
	err = add_timer(&media_check_timer.t, check_media_timer_hook,
	                100000, B_PERIODIC_TIMER);
	if(err != B_NO_ERROR)
		goto err5;

	init_crc16();
	return B_NO_ERROR;

//err6:
//	cancel_timer(&media_check_timer.t);
err5:
#if POLL_INTERRUPT == 0
	delete_sem(int_sem);
err4:
#endif
	delete_dma_area(&dma_info);
err3:
	remove_io_interrupt_handler(ECP_IRQ, parallel_interrupt, NULL);
err2:
	put_module(B_ISA_MODULE_NAME);
err1:
	return err;
}

void 
tpc_uninit()
{
//	dprintf("intcount %ld\n", intcount);
	cancel_timer(&media_check_timer.t);
#if POLL_INTERRUPT == 0
	delete_sem(int_sem);
#endif
	delete_dma_area(&dma_info);
	remove_io_interrupt_handler(ECP_IRQ, parallel_interrupt, NULL);
	put_module(B_ISA_MODULE_NAME);
}

