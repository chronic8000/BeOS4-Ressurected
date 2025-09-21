/* IDE protocols
*/

#include <checkmalloc.h>

#include <IDE.h>
#include <KernelExport.h>
#include "protocol.h"

#define ATA_BSY		0x80
#define ATA_DRDY	0x40
#define ATA_DRQ		0x08
#define ATA_ERR		0x01

#define DE(l) if(l<=3)
#define DF(l) if(0)

status_t
int_nbsy_wait(ide_bus_info *bus, uint32 cookie, bigtime_t timeout)
{
	status_t err;
	bigtime_t starttime, inttime;

	starttime = system_time();
	err = bus->intwait(cookie, timeout);
	if(err == B_NO_ERROR) {
		inttime = system_time();
		do {
			if((bus->get_altstatus(cookie) & ATA_BSY) == 0) {
				DF(3) dprintf("IDE -- intwait: BSY is low %Ld us after int\n",
				              system_time()-inttime);
				return B_NO_ERROR;
			}
			if((system_time()-inttime) > 5000) {
				snooze(2000);
			}
		} while(system_time()-starttime < timeout);
		DE(2) dprintf("IDE -- int_nbsy_wait: got int, drive busy\n");
		return B_ERROR;
	}
	return err;
}

status_t
wait_status(ide_bus_info *bus, uint32 cookie,
	status_t (*predicate)(uint8 status, bigtime_t start_time,
	                      bigtime_t sample_time, bigtime_t timeout),
	bigtime_t timeout)
{
	status_t retval;
	bigtime_t start_time = system_time();
	
	DF(3) dprintf("IDE -- wait_status: %x %ld 0x%08x %Ld\n",
	              (unsigned)bus, cookie, (unsigned)predicate, timeout);
	do {
		bigtime_t sample_time;
		uint8 sample;
		spin(1);
		sample_time = system_time();
		sample = bus->get_altstatus(cookie);
		retval = predicate(sample, start_time, sample_time, timeout);
		if((retval == B_BUSY) &&
		   ((sample_time-start_time) > 5000)) {
			snooze((sample_time-start_time)/10);
		}
	} while(retval == B_BUSY);
	DF(3) dprintf("IDE -- wait_status: retval = 0x%lx\n", retval);

	return retval;
}

static
status_t selectp(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t select_timeout)
{
	DF(3) dprintf("IDE -- selectp: %d %Ld %Ld %Ld\n",
	              status, start_time, sample_time, select_timeout);
	if((status & (ATA_BSY | ATA_DRQ)) == 0)
		return B_NO_ERROR;
	if((sample_time-start_time) > select_timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

#if 0   // not really used
static
status_t device_resetp(uint8 status, bigtime_t start_time,
	bigtime_t sample_time, bigtime_t device_reset_timeout)
{
	if((status & ATA_BSY) == 0) {
		return (status & ATA_ERR ) ? B_ERROR : B_NO_ERROR;
	}
	if((sample_time-start_time) > device_reset_timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

static
status_t device_reset(ide_bus_info *bus, uint32 cookie)
{
	status_t err;
	ide_task_file tf;
	
	tf.write.command = 0x08; // DEVICE RESET
	bus->write_command_block_regs(cookie, &tf, ide_mask_command);

	err = wait_status(bus, cookie, device_resetp, 6000000);
	if(err != B_NO_ERROR) {
		dprintf("IDE -- device_reset: reset failed\n");
	}
	return err;
}
#endif

static void
clear_pending_interrupts(ide_bus_info *bus, uint32 cookie,
                        const char *debugprefix)
{
	int cleared_interrupts = 0;
	while(bus->intwait(cookie, 0) == B_NO_ERROR && cleared_interrupts < 10) {
		cleared_interrupts++;
		snooze(1000);
	}
	if(cleared_interrupts)
		DE(2) dprintf("%s cleared %d pending interrupt%s\n", debugprefix,
		              cleared_interrupts, cleared_interrupts>1 ? "s" : "");
}

status_t
send_ata(ide_bus_info *bus, uint32 cookie, ide_task_file *tf, bool drdy_required)
{
	const char *debugprefix = "IDE -- send_ata:";
	uint8 status;
	status_t err;

	DF(2) dprintf("%s\n", debugprefix);

	if (wait_status(bus, cookie, selectp, 50000) != B_NO_ERROR) {
		status = bus->get_altstatus(cookie);
		if(status & (ATA_BSY | ATA_DRQ)) {
			if(status != 0xff) {
				DE(4) dprintf("%s warning BUS BUSY status 0x%02x ... ignore\n",
				              debugprefix, bus->get_altstatus(cookie));
			}
			else {
				DE(4) dprintf("%s warning bad device selected\n", debugprefix);
			}
		}
	}


	DF(3) dprintf("%s select drive\n", debugprefix);
	err = bus->write_command_block_regs(cookie, tf, ide_mask_device_head);
	if(err != B_NO_ERROR)
		return err;

	if(wait_status(bus, cookie, selectp, 50000) != B_NO_ERROR) {
		status = bus->get_altstatus(cookie);
		if(status == 0xff) {
			DE(1) dprintf("%s drive select failed no device\n", debugprefix);
			return B_ERROR;
		}
		else {
			DE(1) dprintf("%s drive select failed status 0x%02x ... reset bus\n",
			              debugprefix, status);
			if(ide_reset(bus, cookie) != B_NO_ERROR) {
				return B_ERROR;
			}
			err = bus->write_command_block_regs(cookie, tf, ide_mask_device_head);
			if(err != B_NO_ERROR)
				return err;
			if(wait_status(bus, cookie, selectp, 50000) != B_NO_ERROR) {
				DE(1) dprintf("%s drive select failed\n", debugprefix);
				return B_ERROR;
			}
		}
	}

	if (drdy_required) {
		/* 
		 * For ATA, we can rely on DRDY being set. Not so for ATAPI.
		 */
		status = bus->get_altstatus(cookie);
		if((status & ATA_DRDY) == 0) {
			DE(4) dprintf("%s drive not ready\n", debugprefix);
			return B_BUSY;
		}
	}

	DF(3) dprintf("IDE -- send_ata: write task file\n");
	err = bus->write_command_block_regs(cookie, tf, (ide_reg_mask)
	                                    (ide_mask_all &
	                                     ~( ide_mask_device_head |
	                                        ide_mask_command)));
	if(err != B_NO_ERROR)
		return err;

	clear_pending_interrupts(bus, cookie, debugprefix);

	err = bus->write_command_block_regs(cookie, tf, ide_mask_command);
	if(err != B_NO_ERROR)
		return err;

	return B_NO_ERROR;
}


#if 0
static status_t
reset_up_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t timeout)
{
	if(status == 0xff) {	// empty bus
		dprintf("IDE -- reset: empty bus\n");
		return B_ERROR;
	}
	if(status & ATA_BSY) {
		dprintf("IDE -- reset: BSY set at time %Ld\n", sample_time - start_time);
		return B_NO_ERROR;
	}
	if((sample_time - start_time) > 2200)
		dprintf("IDE -- reset: timeout setting BSY\n");
		return B_ERROR;		// some devices may take 2 ms to set BSY
	return B_BUSY;
}
#endif

static status_t
resetp(uint8 status, bigtime_t start_time, bigtime_t sample_time,
	bigtime_t timeout)
{
	bigtime_t elapsed_time = sample_time - start_time;
//	if((status & (ATA_BSY | ATA_DRDY)) == ATA_DRDY)
	if((status & ATA_BSY) == 0)
		return B_NO_ERROR;
	if(elapsed_time > timeout)	// 31 sec 
		return B_TIMEOUT;

	if(elapsed_time > 200000)	// share CPU resource.
		snooze(50000);

	return B_BUSY;
}

status_t
ide_reset(ide_bus_info *bus, uint32 cookie)
{
	const char *debugprefix = "IDE -- reset:";
	status_t err;
	uint8	status;
	
	dprintf("%s\n", debugprefix);
	bus->write_device_control(cookie, 0x0c); // SRST = 1, nIEN = 0

	spin(5);
//	err = wait_status(cookie, reset_up_p);
	
	bus->write_device_control(cookie, 0x08); // SRST = 0, nIEN = 0
//	if(err != B_NO_ERROR) {
//		dprintf("%s no drives\n", debugprefix);
//		return err;
//	}

	status = bus->get_altstatus(cookie);
	err = wait_status(bus, cookie, resetp, 1000000);
	if(err != B_NO_ERROR) {
		if(status != bus->get_altstatus(cookie)) {
			err = wait_status(bus, cookie, resetp, 31000000);
		}
		else {
			err = wait_status(bus, cookie, resetp, 1000);
		}
	}

	if(err != B_NO_ERROR) {
		dprintf("%s drives never got ready. Status: 0x%02x\n", debugprefix,
		        bus->get_altstatus(cookie));
		return err;
	}

	clear_pending_interrupts(bus, cookie, debugprefix);
	return B_NO_ERROR;
}
#if 0

	// read error register
	if((get_error(cookie) & 0x7f) == 0x01) {
//		bus_info[cookie].device_present[0] = true;
		dprintf("IDE -- reset: found master device\n");
	}

	// select device 1
	if(wait_status(cookie, selectp, 100000) != B_NO_ERROR) {
		dprintf("IDE -- reset: could not select device 1. Status: 0x%02x\n",
		        get_altstatus(cookie));
		return B_ERROR;
	}
	isa->write_io_8(command_block_base_addr+6, 0x10);
	if(wait_status(cookie, selectp, 100000) != B_NO_ERROR) {
		isa->write_io_8(command_block_base_addr+6, 0x00); // select device 0
		dprintf("IDE -- reset: selection of device 1 failed. Status: 0x%02x\n",
		        get_altstatus(cookie));
		return B_NO_ERROR;
	}
	if((get_error(cookie) & 0x7f) == 0x01) {
		bus_info[cookie].device_present[1] = true;
		dprintf("IDE -- reset: found slave device\n");
	}
	dprintf("IDE -- reset: done\n");

	return B_NO_ERROR;
}
#endif

