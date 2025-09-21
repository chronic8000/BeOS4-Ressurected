#include "atapi_devicelist.h"

#include <IDE.h>
#include <KernelExport.h>
#include <ide_device_info.h>
#include <Drivers.h>
#include <scsi.h>
#include <CAM.h>
#include <Mime.h>				/* For ICON #defines */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ide_settings.h>
#include <block_io.h>
#include <driver_settings.h>
#include "atapi_cmds.h"
#include "atapi.h"
#include "atapi_changer.h"

#define ATAPI_CMDLEN	12

#if _BUILDING_kernel || BOOT
#include <drivers_priv.h>
#endif

#if defined(__MWERKS__)
#define UNUSED(x)
#define UNUSED_ARG(x)
#else
#define UNUSED(x) x
#define UNUSED_ARG(x) x = x;
#endif

extern unsigned char icon_cd[];
extern unsigned char icon_cd_mini[];
extern unsigned char icon_disk[];
extern unsigned char icon_disk_mini[];
extern unsigned char icon_removable[];
extern unsigned char icon_removable_mini[];

static status_t play_audio_track(device_handle *h, scsi_play_track *track_info);
static status_t get_volume(device_handle *h, scsi_volume *volume);
static status_t set_volume(device_handle *h, scsi_volume *volume);

ide_module_info *ide;

static bool default_dma_enable;

/*------------------------------------------------------------------------------
** debug output
*/

#include <debugoutput.h>

int debug_level_error = 3;
int debug_level_info = 1;
int debug_level_flow = 0;

static status_t
read_settings_global()
{
	const char *debugprefix = DEBUGPREFIX "read_settings_global:";
	const char *drivername = "atapi";
	void *settingshandle;
	const driver_settings *s;

	settingshandle = load_driver_settings(drivername);
	if(settingshandle == 0)
		return B_ERROR;
	s = get_driver_settings(settingshandle);
	if(s) {
		int i;
		for(i = 0; i < s->parameter_count; i++) {
			driver_parameter *gp = &s->parameters[i];
			if(strcmp(gp->name, "global") == 0) {
				int j;
				for(j = 0; j < gp->parameter_count; j++) {
					driver_parameter *p = &gp->parameters[j];
					if(strcmp(p->name, "debug_level_error") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_error takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_error = strtol(p->values[0], NULL, 10);
					}
					else if(strcmp(p->name, "debug_level_info") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_info takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_info = strtol(p->values[0], NULL, 10);
					}
					else if(strcmp(p->name, "debug_level_flow") == 0) {
						if(p->value_count != 1) {
							DE(2) dprintf("%s debug_level_flow takes one arguments\n", debugprefix);
							continue;
						}
						debug_level_flow = strtol(p->values[0], NULL, 10);
					}
					else {
						DE(3) dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
					}
				}
			}
		}
	}
	unload_driver_settings(settingshandle);
	return B_NO_ERROR;
}

/*------------------------------------------------------------------------------
** wait_status predicates
*/

static status_t
nbusy_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
        bigtime_t timeout)
{
	if ((status & ATA_BSY) == 0)
		return B_NO_ERROR;
	if ((sample_time - start_time) > timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

static status_t
busy_drq_p(uint8 status, bigtime_t start_time, bigtime_t sample_time,
		   bigtime_t timeout)
{
	if ((status & (ATA_DRQ | ATA_BSY)) == ATA_DRQ)
		return B_NO_ERROR;
	if ((sample_time - start_time) > timeout)
		return B_TIMED_OUT;
	return B_BUSY;
}

/*------------------------------------------------------------------------------
** Intwait then poll for BSY to go low -- handles bad drives
*/

static status_t
int_nbsy_wait(ide_bus_info *bus, uint32 cookie, bigtime_t timeout)
{
	status_t err;
	bigtime_t starttime;

	starttime = system_time();
	err = bus->intwait(cookie, timeout);
	if(err == B_NO_ERROR) {
		err = ide->wait_status(bus, cookie, nbusy_p,
		                       starttime + timeout - system_time());
		if(err != B_NO_ERROR)
			DE(3) dprintf("IDE ATAPI -- int_nbsy_wait: got int, drive busy\n");
	}
	return err;
}

/*------------------------------------------------------------------------------
** Send packet
*/

/*
** pio_vec does the pio transfer to an iovec
*/

static status_t
pio_vec(device_handle *h, const iovec *vec, size_t veccount,
        uint32 startbyte, size_t len, bool write)
{
	device_entry	*d = h->d;
	ide_bus_info	*bus = d->bus;
	uint32 			buscookie = d->buscookie;

	uint32	bytesleft = len;
	uint8	*buf;
	uint32	transfersize;
	uint8	tmpbuf[2];
	bool	remainder = false;
	
	while(bytesleft > 0) {
		while(veccount > 0 && vec->iov_len <= startbyte) {
			startbyte -= vec->iov_len;
			vec++;
			veccount--;
		}
		if(veccount == 0) {
			return B_BAD_VALUE;
		}
		buf = (uint8*)vec->iov_base+startbyte;
		transfersize = min(vec->iov_len-startbyte, bytesleft);
		if(write) {
			if(remainder) {
				remainder = false;
				tmpbuf[1] = buf[0];
				transfersize = 1;
				bus->write_pio_16(buscookie, (uint16*)tmpbuf, 1);
			}
			else {
				bus->write_pio_16(buscookie, (uint16*)buf, transfersize/2);
				if(transfersize&1) {
					tmpbuf[0] = buf[transfersize-1];
					remainder = true;
				}
			}
		}
		else {
			if(remainder) {
				remainder = false;
				buf[0] = tmpbuf[1];
				transfersize = 1;
			}
			else {
				bus->read_pio_16(buscookie, (uint16*)buf, transfersize/2);
				if(transfersize&1) {
					bus->read_pio_16(buscookie, (uint16*)tmpbuf, 1);
					buf[transfersize-1]=tmpbuf[0];
					remainder = true;
				}
			}
		}
		startbyte += transfersize;
		bytesleft -= transfersize;
	}
	return B_NO_ERROR;
}

/*
** send packet
*/

static status_t
send_atapi_command_t(device_handle *h, uint8 *cmd_data,
                     const iovec *vec, size_t veccount, uint32 startbyte,
                     size_t *numBytes,
                     bool write, bool expect_short_read, bigtime_t timeout,
                     uint8 *sense_data, size_t sense_data_size,
                     int max_retry_count)
{
	const char * const debugprefix = "IDE ATAPI -- send_packet:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	ide_device_info *di = d->di;
	size_t len, reqlen=0;
	status_t err = B_NO_ERROR;
	bool do_dma = false;

	DF(2) dprintf("%s cmd 0x%02x\n", debugprefix, cmd_data[0]);

	len = *numBytes;
	if (len > 0xfffe) {
		len = 0xfffe;
	}

retry_command:
	do_dma = false;
	if(di->capabilities.dma_supported &&
	   d->priv->dma_enabled &&
	   !(len & d->bad_alignment_mask) &&
	   len != 0) {
//	if (di->capabilities.dma_supported && d->priv->dma_enabled && len >= 2048) {
		size_t transfersize = *numBytes;
		DF(3) dprintf("%s calling prepare_dma with size %ld\n",
		              debugprefix, transfersize);
		err = bus->prepare_dma(buscookie, vec, veccount, startbyte, 1,
		                       &transfersize, write);
		if (err == B_NO_ERROR) {
			if (*numBytes != transfersize) {
				DE(2) dprintf("%s couldn't do transfer in one chunk (%ld of "
				              "%ld)\n", debugprefix, transfersize, *numBytes);
				bus->finish_dma(buscookie);
			} else {
				do_dma = true;
				DF(2) dprintf("%s using DMA\n", debugprefix);
			}
		}
		else if(err == B_NOT_ALLOWED) {
			d->priv->dma_status = B_IDE_DMA_STATUS_CONTROLLER_DISABLED;
			d->priv->dma_enabled = false;
		}
	}
	
	tf->atapi_input.dma = do_dma;
	tf->atapi_input.ovl = 0;
	tf->atapi_input.na1 = 0; /* may be used by future devices */
	tf->atapi_input.command = 0xa0;
	tf->atapi_input.byte_count_0_to_7 = len;
	tf->atapi_input.byte_count_8_to_15 = len>>8;

	err = ide->send_ata(bus, buscookie, tf, false);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s send command failed\n", debugprefix);
		goto err;
	}

	switch (di->general_configuration.CMD_DRQ_type) {
	case 0x0:
		/* Microprocessor DRQ
		 * The drive is supposed to be ready within 3ms, but some drives
		 * are a lot slower than that
		 */
		err = ide->wait_status(bus, buscookie, busy_drq_p, 3000*MILLISEC);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s not ready for command(1)\n"
			              "%25s error: %s\n"
			              "%25s status = 0x%02x\n",
			              debugprefix, "", strerror(err),
			              "", bus->get_altstatus(buscookie));
			goto err;
		}
		break;

	case 0x1:
		/* Interrupt DRQ
		 * These pre-ATAPI-4 devices assert INTRQ at this point.
		 * Wait for an interrupt for 10ms.
		 */
		err = bus->intwait(buscookie, 10*MILLISEC);
		if (err < B_NO_ERROR) {
			DE(2) dprintf("%s intwait failed after PACKET command,\n"
			              "%25s %s\n", debugprefix, "", strerror(err));
			goto err;
		}
		if((bus->get_altstatus(buscookie) & 0xa9) != ATA_DRQ) {
			DE(2) dprintf("%s not ready for command(2)! status = 0x%02x\n",
			              debugprefix, bus->get_altstatus(buscookie));
			err = B_IO_ERROR;
			goto err;
		}
		break;
		
	case 0x2:	/* Accelerated DRQ */
		err = ide->wait_status(bus, buscookie, busy_drq_p, 50);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s device not ready for command(3),\n"
			              "%25s %s\n", debugprefix, "", strerror(err));
			goto err;
		}
		break;
	
	default:
		DE(1) dprintf("%s invalid DRQ mode\n", debugprefix);
		err = B_ERROR;
		goto err;
	}
	
	bus->write_pio_16(buscookie, (uint16 *) cmd_data, ATAPI_CMDLEN/2);

	if (!do_dma) {
		const iovec *tmpvec = vec;
		size_t tmpveccount = veccount;
		uint32 tmpstartbyte = startbyte;
		reqlen = *numBytes;
		
		while(reqlen > 0){
			while(tmpveccount > 0 && tmpvec->iov_len <= tmpstartbyte) {
				tmpstartbyte -= tmpvec->iov_len;
				tmpvec++;
				tmpveccount--;
			}
			if(tmpveccount == 0) {
				err = B_BAD_VALUE;
				goto err;
			}
			/*
			 * Wait for cmd packet tranfer complete / data ready
			 */
			err = int_nbsy_wait(bus, buscookie, timeout);
			if (err < 0) {
				DE(2) dprintf("%s intwait failed waiting for data ready\n"
				              "%25s error %s\n"
				              "%25s status = 0x%02x\n"
				              "%25s command = 0x%02x\n",
				              debugprefix, "", strerror(err),
				              "", bus->get_altstatus(buscookie),
				              "", cmd_data[0]);
				goto err;
			}
			if(0) {
try_pio:
				tmpvec = vec;
				tmpveccount = veccount;
				tmpstartbyte = startbyte;
				reqlen = *numBytes;
			}
			{
				uint8 status = bus->get_altstatus(buscookie);
				if((status & ATA_DRQ) == 0) {
					//dprintf("IDE ATAPI -- send_packet: error, drq not set\n");
					goto no_more_drive_data;
				}
			}

			/*
			 * Bytecount lives in the ATAPI cylinder register in the
			 * task file.
			 */
			bus->read_command_block_regs(buscookie, tf, ide_mask_cylinder);
			len = tf->atapi_input.byte_count_0_to_7 |
			      tf->atapi_input.byte_count_8_to_15 << 8;
			if (len > reqlen) {
				DE(2) dprintf("%s byte count mismatch (sending 0x%lx, req "
				              "0x%lx)\n", debugprefix, len, reqlen);
				err = B_ERROR;
				goto err;
			}

			reqlen -= len;

			/*
			 * Transfer data
			 */
			err = pio_vec(h, tmpvec, tmpveccount, tmpstartbyte, len, write);
			if(err != B_NO_ERROR)
				goto err;
			tmpstartbyte += len;
		}
	}

	if (do_dma) {
		err = bus->start_dma(buscookie);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s could not start dma\n", debugprefix);
			goto dma_failed;
		}
		err = int_nbsy_wait(bus, buscookie, timeout);
		if(err != B_NO_ERROR) {
			bus->finish_dma(buscookie);
			if(err == B_TIMED_OUT) {
				uint8 status;
dma_failed:
				status = bus->get_altstatus(buscookie);
				bus->read_command_block_regs(buscookie, tf,
				                             ide_mask_sector_count);
				DE(1) dprintf("%s DMA operation failed (status 0x%02x "
				              "irr=0x%02x),\n"
				              "%25s disabling DMA for this drive\n",
				              debugprefix, status, tf->read.sector_count, "");
				ide->reset(bus, buscookie);
				d->priv->dma_enabled = false;
				d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_FAILURE;
				goto retry_command;
			}
		}
	}
	else {
		err = int_nbsy_wait(bus, buscookie, timeout);
	}
	if (err < B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed waiting for transfer complete,\n"
		              "%25s %s\n"
		              "%25s status: 0x%02x\n", debugprefix,
		              "", strerror(err), "", bus->get_altstatus(buscookie));
		goto err;
	}
	
	if (do_dma) {
		DF(2) dprintf("%s finishing DMA\n", debugprefix);
		err = bus->finish_dma(buscookie);
		if (err == B_INTERRUPTED && expect_short_read)
			err = B_WOULD_BLOCK;
		else if (err != B_NO_ERROR) {
			DE(2) dprintf("%s finishing DMA failed: %s\n",
			              debugprefix, strerror(err));
		}
		
		if((bus->get_altstatus(buscookie) & (ATA_BSY|ATA_DRQ)) == ATA_DRQ) {
			DE(3) dprintf("%s drive did not perform dma protocol,\n"
			              "%25s disable DMA and try to complete as PIO\n",
			              debugprefix, "");
			d->priv->dma_enabled = false;
			d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_FAILURE;
			do_dma = false;
			goto try_pio;
		}
	}
	
no_more_drive_data:
	{
		uint8 status = bus->get_altstatus(buscookie);
		if(status & (ATA_ERR | ATA_BSY)) {
			bus->read_command_block_regs(buscookie, tf, ide_mask_error);
			err = request_sense(h, cmd_data, sense_data, sense_data_size);
			if (err == B_NO_ERROR) {
				if(max_retry_count-- > 0)
					goto retry_command;
			   	else {
					DE(2) dprintf("%s too many retries\n", debugprefix);
					return B_IO_ERROR;
				}
			}
			if((err != B_DEV_NO_MEDIA) && (err != B_DEV_NOT_READY)) {
				DE(2) dprintf("%s command 0x%02x failed, "
				              "status 0x%02x error 0x%02x\n"
				              "%25s with error %s\n",
				              debugprefix, cmd_data[0], status, tf->read.error,
				              "", strerror(err));
			}
			return err;
		}
		if(status & ATA_DRQ) {
			DE(2) dprintf("%s command 0x%02x did not complete, status 0x%02x\n",
			              debugprefix, cmd_data[0], status);
			return B_IO_ERROR;
		}
	}
	if (!do_dma  &&  reqlen != 0) {
		/*
		 * Not all data transferred?
		 */
		if(!expect_short_read)
			DE(2) dprintf("%s data underrun command 0x%02x, got %ld of %ld "
			              "bytes\n", debugprefix,
			              cmd_data[0], *numBytes-reqlen, *numBytes);
		*numBytes -= reqlen;
		return B_WOULD_BLOCK;
	}
	
	return err;

err:
	if (do_dma) {
		bus->finish_dma(buscookie);
	}
	DF(2) dprintf("%s cmd 0x%02x, error exit %s\n", debugprefix,
	              cmd_data[0], strerror(err));
	return err;
}

/*
** send packet (fixed timeout and retry count, no sense data)
*/

status_t
send_atapi_command(device_handle *h, uint8 *cmd_data,
                   const iovec *vec, size_t veccount, uint32 startbyte,
                   size_t *numBytes,
                   bool write, bool expect_short_read)
{
	return send_atapi_command_t(h, cmd_data, vec, veccount, startbyte, numBytes,
	                            write, expect_short_read,
	                            20000*MILLISEC+*numBytes*64, NULL, 0, 20);
}


/*------------------------------------------------------------------------------
** Audio
*/

static status_t
play_audio_track(device_handle *h, scsi_play_track *track_info)
{
	uint16 index = 0;
	uint16 end;
	scsi_play_position pos;
	status_t err;
	scsi_toc *tocp = &h->priv->li->toc;

	err = read_toc(h, tocp);
	if (err != B_NO_ERROR)
		return err;
	
	if (track_info->start_track > tocp->toc_data[3])
		return B_ERROR;

	while (tocp->toc_data[4 + (index * 8) + 2] != track_info->start_track) {
		index++;
	}

	pos.start_m = tocp->toc_data[4 + (index * 8) + 5];
	pos.start_s = tocp->toc_data[4 + (index * 8) + 6];
	pos.start_f = tocp->toc_data[4 + (index * 8) + 7];
	index = 0;

	end = track_info->end_track;

	if (end > tocp->toc_data[3])
		end = tocp->toc_data[3];

	while (tocp->toc_data[4 + (index * 8) + 2] != end) {
		index++;
	}

	index++;
	pos.end_m = tocp->toc_data[4 + (index * 8) + 5];
	pos.end_s = tocp->toc_data[4 + (index * 8) + 6];
	pos.end_f = tocp->toc_data[4 + (index * 8) + 7];

	if (pos.end_f > 0) {
		pos.end_f -= 1;
	} else if (pos.end_s > 0) {
		pos.end_s -= 1;
		pos.end_f = 74;
	} else {
		pos.end_m -= 1;
		pos.end_s = 59;
		pos.end_f = 74;
	}

	return play_audio_msf(h, &pos);
}

static status_t
get_volume(device_handle *h, scsi_volume *volume)
{
	uint8 mode_data[24];
	size_t buflen = sizeof(mode_data);
	status_t err;
	
	err = mode_sense(h, 0, 0xe, mode_data, buflen);
	if (err != B_NO_ERROR)
		return err;
	
	volume->flags = 0xff;
	volume->port0_channel = mode_data[0x08 + 0x08] & 0x0f;
	volume->port0_volume = mode_data[0x08 + 0x09];
	volume->port1_channel = mode_data[0x08 + 0x0a] & 0x0f;
	volume->port1_volume = mode_data[0x08 + 0x0b];
	volume->port2_channel = mode_data[0x08 + 0x0c] & 0x0f;
	volume->port2_volume = mode_data[0x08 + 0x0d];
	volume->port3_channel = mode_data[0x08 + 0x0e] & 0x0f;
	volume->port3_volume = mode_data[0x08 + 0x0f];
	return B_NO_ERROR;
}

static status_t
set_volume(device_handle *h, scsi_volume *volume)
{
	uint8 mode_data[24];
	size_t buflen = sizeof(mode_data);
	status_t err;
	
	err = mode_sense(h, 0, 0xe, mode_data, buflen);
	if (err != B_NO_ERROR)
		return err;
	
	memset(mode_data, 0, 8);	/* clear the header since it is */
								/* reserved for mode select */

	if (volume->flags & B_SCSI_PORT0_CHANNEL)
		mode_data[0x08 + 0x08] = volume->port0_channel & 0x0f;
	if (volume->flags & B_SCSI_PORT0_VOLUME)
		mode_data[0x08 + 0x09] = volume->port0_volume;
	if (volume->flags & B_SCSI_PORT1_CHANNEL)
		mode_data[0x08 + 0x0a] = volume->port1_channel & 0x0f;
	if (volume->flags & B_SCSI_PORT1_VOLUME)
		mode_data[0x08 + 0x0b] = volume->port1_volume;
	if (volume->flags & B_SCSI_PORT2_CHANNEL)
		mode_data[0x08 + 0x0c] = volume->port2_channel & 0x0f;
	if (volume->flags & B_SCSI_PORT2_VOLUME)
		mode_data[0x08 + 0x0d] = volume->port2_volume;
	if (volume->flags & B_SCSI_PORT3_CHANNEL)
		mode_data[0x08 + 0x0e] = volume->port3_channel & 0x0f;
	if (volume->flags & B_SCSI_PORT3_VOLUME)
		mode_data[0x08 + 0x0f] = volume->port3_volume;

	err = mode_select(h, false, mode_data, buflen);
	if (err != B_NO_ERROR)
		return err;

	return B_NO_ERROR;	
}

/*------------------------------------------------------------------------------
** Device handling
*/

bool
is_supported_device(ide_device_info *di)
{
	const char * const debugprefix = "IDE ATAPI -- is_supported_device:";
	if (di->general_configuration.protocol_type != 2) {
		/*
		 * 2 is the ATAPI protocol
		 */
		DE(1) dprintf("%s device protocol (%d) is not atapi\n", debugprefix,
		              di->general_configuration.protocol_type);
		return false;
	}
	
	if (di->general_configuration.command_packet_size != 0x0) {
		/*
		 * Only 12 byte packet sizes are currently supported
		 * for ATAPI.
		 */
		DE(1) dprintf("%s unsupported packet size (cps=0x%02x)\n", debugprefix,
		              di->general_configuration.command_packet_size);
		return false;
	}
	
	switch (di->general_configuration.device_type) {
		case B_CD:				/* Supported types */
		case B_DISK:
		case B_OPTICAL:
			return true;
		default:
			DE(1) dprintf("%s unsupported device type 0x%x\n", debugprefix,
			              di->general_configuration.device_type);
			return false;
	}
}

static void
print_cdrom_capabilities(cdrom_capabilities *cc)
{
	const char *Loading_Mechanism_Types[8] = {
		"Caddy",
		"Tray",
		"Pop-up",
		"UNKNOWN",
		"Changer - Individual disc loading",
		"Changer = Cartridge",
		"UNKNOWN",
		"UNKNOWN"
	};

	dprintf("CD-ROM Capabilities and Mechanical Status:\n");
	dprintf("Page Code: 0x%02x\n", cc->Page_Code);
	dprintf("PS: %d\n", cc->PS);
	dprintf("Page Length: 0x%02x\n", cc->Page_Length);
	dprintf("CD-R Method 2 read: %d\n", cc->Method2);
	dprintf("CD-R Rd: %d\n", cc->CD_R_Rd);
	dprintf("CD-R Wr: %d\n", cc->CD_R_Wr);
	dprintf("CD-E Rd: %d\n", cc->CD_E_Rd);
	dprintf("CD-E Wr: %d\n", cc->CD_E_Wr);
	dprintf("Audio Play: %d\n", cc->AudioPlay);
	dprintf("Composite Audio/Video: %d\n", cc->Composite);
	dprintf("Digital output port 1: %d\n", cc->DigitalPort1);
	dprintf("Digital output port 2: %d\n", cc->DigitalPort2);
	dprintf("Mode 2 Form 1: %d\n", cc->Mode2Form1);
	dprintf("Mode 2 Form 2: %d\n", cc->Mode2Form2);
	dprintf("Multi Session: %d\n", cc->MultiSession);
	dprintf("CD-DA: %d\n", cc->CD_DA);
	dprintf("CD-DA Stream is Accurate: %d\n", cc->DAAccu);
	dprintf("R-W Supported: %d\n", cc->RW_Supported);
	dprintf("R-W De-interleaved & Corrected: %d\n",
	        cc->RW_Deinterleaved_and_Corrected);
	dprintf("C2 Pointers are Supported: %d\n", cc->C2_Pointers);
	dprintf("ISRC: %d\n", cc->ISRC);
	dprintf("UPC: %d\n", cc->UPC);
	dprintf("Lock: %d\n", cc->Lock);
	dprintf("Lock State: %d\n", cc->Lock_State);
	dprintf("Prevent Jumper: %d\n", cc->Prevent_Jumper);
	dprintf("Eject Command: %d\n", cc->Eject);
	dprintf("Loading Mechanism: %s\n",
		Loading_Mechanism_Types[cc->Loading_Mechanism_Type]);
	dprintf("Separate Volume Levels: %d\n", cc->Separate_Volume_Levels);
	dprintf("Separate Channel Mute: %d\n", cc->Separate_Channel_Mute);
	dprintf("Supports Disc Present: %d\n", cc->Support_Disc_Present);
	dprintf("Software Slot Selection: %d\n", cc->Software_Slot_Selection);
	dprintf("Maximum Speed Supported: %dkB/s\n",
		B_BENDIAN_TO_HOST_INT16(cc->Maximum_Speed_Supported));
	dprintf("Current Speed Selected: %dkB/s\n",
		B_BENDIAN_TO_HOST_INT16(cc->Current_Speed_Selected));
	dprintf("Number of Volume Levels Supported: %d\n",
		B_BENDIAN_TO_HOST_INT16(cc->Number_of_Volume_Levels_Supported));
	dprintf("Buffer Size supported by Drive: %dKB\n",
		B_BENDIAN_TO_HOST_INT16(cc->Buffer_Size_supported_by_Drive));
	if(cc->Page_Length < 15)
		return;
	dprintf("Digital output:\n");
	dprintf("	BCKF: %d\n", cc->BCK);
	dprintf("	RCK: %d\n", cc->RCK);
	dprintf("	LSBF: %d\n", cc->LSBF);
	dprintf("	Length: %d\n", cc->Length);
}

static bool
is_write_protected(device_handle *h)
{
	uint8 mode_data[8];
	status_t err;
	err = mode_sense(h, 0, 0x3f /* all pages */, mode_data, sizeof(mode_data));
	if(err == B_NO_ERROR)
		return (mode_data[3] & 0x80) == 0x80;
	else
		return false;
}

static void
update_capacity_data(device_handle *h)
{
	const char * const debugprefix = "IDE ATAPI -- update_capacity_data:";
	device_entry *d = h->d;
	atapi_capacity_data data;
	uint32 lun;
	status_t err;
	if(d->priv->lia == NULL)
		return;		// ignore media changed at startup

	for(lun=0; lun <= d->maxlun; lun++) {
		if(d->maxlun != 0) {
			err = load_unload_cd(h, lun, true /*load*/);
			if(err != B_NO_ERROR) {
				DE(4) dprintf("%s could not load cd %ld\n", debugprefix, lun);
				goto no_capacity_data;
			}
		}
		err = read_capacity(h, &data);
		if (err == B_NO_ERROR) {
			d->priv->lia[lun].capacity =
				B_BENDIAN_TO_HOST_INT32(data.last_lba)+1;
			d->priv->lia[lun].blocksize =
				B_BENDIAN_TO_HOST_INT32(data.blocksize);
			DI(2) dprintf("%s capacity = %ld, blocksize = %ld\n", debugprefix,
			              d->priv->lia[lun].capacity,
			              d->priv->lia[lun].blocksize);
			if (d->di->general_configuration.device_type == B_CD) {
				d->priv->lia[lun].blocksize = 2048;
				d->priv->lia[lun].write_protect = true;
			}
			else {
				d->priv->lia[lun].write_protect = is_write_protected(h);
			}
			d->priv->lia[lun].capacity_valid = true;
		}
		else {
no_capacity_data:
			d->priv->lia[lun].capacity = 0;
			d->priv->lia[lun].capacity_valid = (err == B_DEV_NO_MEDIA);
			d->priv->lia[lun].write_protect = true;
			switch (d->di->general_configuration.device_type) {
				case B_CD:	
					d->priv->lia[lun].blocksize = 2048;
					break;
				case B_DISK:
					d->priv->lia[lun].blocksize = 512;
					break;
				case B_OPTICAL:
					d->priv->lia[lun].blocksize = 512;
					break;
				default:
					DE(0) dprintf("%s unsupported device type (%d)\n",
					              debugprefix,
					              d->di->general_configuration.device_type);
			}
		}
	}
	if(d->maxlun != 0)
		load_unload_cd(h, h->lun, true /*load*/);
}

status_t
reconfigure_device(device_handle *h)
{
	status_t err = B_NO_ERROR;
	if(h->d->maxlun > 0)
		err = reconfigure_changer(h->d);
	else
		err = media_changed_in_lun(h->d, 0);
		
	update_capacity_data(h);
	return err;
}

status_t
use_settings_device(device_entry *d, driver_parameter *p)
{
	const char *debugprefix = "IDE ATAPI -- use_settings_device:";

	if(strcmp(p->name, "nodma") == 0) {
		d->priv->dma_enabled = false;
		d->priv->dma_status = B_IDE_DMA_STATUS_USER_DISABLED_CONFIG;
	}
	else if(strcmp(p->name, "forcedma") == 0) {
		if(!d->priv->dma_enabled) {
			dprintf("%s dma reenabled by user config\n", debugprefix);
		}
		d->priv->dma_enabled = true;
		d->priv->dma_status = B_IDE_DMA_STATUS_ENABLED;
	}
	else {
		dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
	}
	return B_NO_ERROR;
}

status_t
configure_device(device_entry *d)
{
	const char * const debugprefix = "IDE ATAPI -- configure_device:";
	status_t err;
	cdrom_capabilities cdrom_capabilities;
	device_handle h;
	driver_device_handle hpriv;
	
	h.next = NULL;
	h.name = "temp handle";
	h.d = d;
	h.lun = -1; // global commands only
	h.priv = &hpriv;
	h.priv->li = NULL;
	h.status = B_NO_ERROR;
	h.priv->command_in_progress = false;

	DF(2) dprintf("%s\n", debugprefix);

	d->priv = malloc(sizeof(driver_device_entry));
	if(d->priv == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}

	d->priv->lia = NULL;
	d->priv->ms = NULL;
	d->priv->current_lun = -1;
	d->priv->dma_enabled = false;
	d->priv->locked = false;	
	
	if(d->di->removable_media.media_status_notification_supported == 1) {
		status_t err;
		d->tf->write.command = 0xef;
		d->tf->write.features = 0x95;
		err = acquire_bus(d);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock bus\n", debugprefix);
		}
		else {
			err = ide->send_ata(d->bus, d->buscookie, d->tf, false);
			if (err == B_NO_ERROR)  {
				err = d->bus->intwait(d->buscookie, 1000*MILLISEC);
			}
			release_bus(d);
		}
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s couldn't enable media status notification\n",
			              debugprefix);
		}
		else {
			DI(2) dprintf("%s enabled media status notification\n",
			              debugprefix);
		}
	}
	
	d->priv->Support_Disc_Present = false;
	d->priv->read_write_12_unsupported = false;
	d->priv->ms = NULL;
	d->priv->media_change_requested = false;

	if(d->di->general_configuration.device_type == B_OPTICAL &&
	   strncmp(d->di->model_number, "FUJITSU", strlen("FUJITSU")) == 0) {
		DI(1) dprintf("%s found FUJITSU MO drive, use write(10)\n",
		              debugprefix);
		d->priv->read_write_12_unsupported = true;
	}
	
	if (d->di->general_configuration.device_type == B_CD) {
		err = acquire_bus(d);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock bus\n", debugprefix);
		}
		else {
			cdrom_capabilities.mode_data_length = 0;
			memset(&cdrom_capabilities, 0, sizeof(cdrom_capabilities));
			err = mode_sense(&h, 0, 0x2a, (uint8*)&cdrom_capabilities,
			                 sizeof(cdrom_capabilities));
			if(err == B_WOULD_BLOCK) {
				int len;
				len = B_BENDIAN_TO_HOST_INT16(cdrom_capabilities.mode_data_length);
				if((len >= 8) && (cdrom_capabilities.Page_Length == (len-8))) {
					err = B_NO_ERROR;
				}
				else {
					DE(2) dprintf("%s no capability data\n", debugprefix);
				}
			}
			if(err == B_NO_ERROR) {
				DI(2) print_cdrom_capabilities(&cdrom_capabilities);
				if((cdrom_capabilities.Loading_Mechanism_Type == 4) ||
				   (cdrom_capabilities.Loading_Mechanism_Type == 5) ) {
					configure_changer(d);
					d->priv->Support_Disc_Present =
						cdrom_capabilities.Support_Disc_Present;
				}
			}
			release_bus(d);
		}
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not get CD-ROM capabilities page\n"
			              "%30s error %s\n", debugprefix, "", strerror(err));
		}
	}
	
	d->priv->lia = malloc(sizeof(lun_info) * (d->maxlun+1));
	if(d->priv->lia == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}
	err = acquire_bus(d);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock bus\n", debugprefix);
	}
	else {
		update_capacity_data(&h);
		release_bus(d);
	}
	d->priv->dma_enabled = default_dma_enable;
	if(d->di->capabilities.dma_supported)
		d->priv->dma_status = B_IDE_DMA_STATUS_ENABLED;
	else
		d->priv->dma_status = B_IDE_DMA_STATUS_UNSUPPORTED;
	if(d->priv->dma_enabled && d->di->capabilities.dma_supported) {
		uint32	drive_dma_mode = 0;
		uint32	controller_dma_mode = ~0;

		if(get_selected_dma_mode(d->di, &drive_dma_mode) != B_NO_ERROR) {
			DE(2) dprintf("%s selected dma mode bad,\n"
			              "%30s disable dma for this device\n",
			              debugprefix, "");
			d->priv->dma_enabled = false;
			d->priv->dma_status = B_IDE_DMA_STATUS_DRIVER_DISABLED_CONFIG;
		}
		else {
			DI(1) dprintf("%s drive dma mode: 0x%02lx\n",
			              debugprefix, drive_dma_mode);
	
			if(d->bus->get_dma_mode(d->buscookie, d->devicenum,
			                        &controller_dma_mode) == B_OK) {
				DI(1) dprintf("%s controller dma mode: 0x%02lx\n",
				              debugprefix, controller_dma_mode);
				if((drive_dma_mode & 0xf0) != (controller_dma_mode & 0xf0) ||
				   (drive_dma_mode & 0x0f) < (controller_dma_mode & 0x0f)) {
					DE(2) dprintf("%s drive and controller dma mode mismatch,\n"
					              "%30s disable dma for this device\n",
					              debugprefix, "");
					d->priv->dma_enabled = false;
					d->priv->dma_status =
						B_IDE_DMA_STATUS_DRIVER_DISABLED_CONFIG;
				}
			}
		}
	}
	read_settings_device(d, "atapi");
	if(default_dma_enable == false) {
		d->priv->dma_enabled = false;
		d->priv->dma_status = B_IDE_DMA_STATUS_USER_DISABLED_SAFE_MODE;
	}
	
	return B_NO_ERROR;

err2:
	free(d->priv);
err1:
	return err;
}

static void
last_atapi_command(device_entry *d)
{
	const char * const debugprefix = "IDE ATAPI -- last_atapi_command:";
	status_t err;

	device_handle h;
	driver_device_handle hpriv;
	
	h.next = NULL;
	h.name = "temp handle";
	h.d = d;
	h.lun = -1; // global commands only
	h.priv = &hpriv;
	h.priv->li = NULL;
	h.status = B_NO_ERROR;
	h.priv->command_in_progress = false;

	err = acquire_bus(d);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s could not lock bus\n", debugprefix);
		return;
	}

	test_unit_ready(&h);

	release_bus(d);
}

void
release_device(device_entry *d)
{
	const char * const debugprefix = "IDE ATAPI -- release_device:";
	DF(2) dprintf("%s\n", debugprefix);
	
	d->priv->dma_enabled = false;
	
	last_atapi_command(d);

	if(d->di->removable_media.media_status_notification_supported == 1) {
		status_t err;
		d->tf->write.command = 0xef;	/* SET FEATURES */
		d->tf->write.features = 0x31;	/* Disable Media Status Notification */
		err = acquire_bus(d);
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s could not lock bus\n", debugprefix);
		}
		else {
			err = ide->send_ata(d->bus, d->buscookie, d->tf, false);
			if (err == B_NO_ERROR)  {
				err = d->bus->intwait(d->buscookie, 1000*MILLISEC);
			}
			release_bus(d);
		}
		if(err != B_NO_ERROR) {
			DE(2) dprintf("%s couldn't disable media status notification\n",
			              debugprefix);
		}
		else {
			DI(2) dprintf("%s disabled media status notification\n",
			              debugprefix);
		}
	}
	if(d->priv->ms != NULL)
		free(d->priv->ms);
	free(d->priv->lia);
	free(d->priv);
}

void
wake_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- wake_device:";
	if(d->di->command_set_feature_supported.valid != 1 ||
	   d->di->command_set_feature_supported.power_management == 0) {
		DF(1) dprintf("%s no power_management\n", debugprefix);
		return;
	}
	DF(1) dprintf("%s power_management supported\n", debugprefix);
}

void
suspend_device(device_entry *d)
{
	const char *debugprefix = "IDE ATA -- suspend_device:";
	if(d->di->command_set_feature_supported.valid != 1 ||
	   d->di->command_set_feature_supported.power_management == 0) {
		DF(1) dprintf("%s no power_management\n", debugprefix);
		return;
	}
	DF(1) dprintf("%s power_management supported\n", debugprefix);
}

/*------------------------------------------------------------------------------
** removable media feature set functions
*/

static status_t
check_media_change_requested(device_handle *h)
{
	const char *debugprefix = "IDE ATAPI -- get_media_status:";
	device_entry *d = h->d;
	ide_task_file *tf = d->tf;
	ide_bus_info *bus = d->bus;
	uint32 buscookie = d->buscookie;
	status_t err;
	uint8	status;

	if(d->di->removable_media.media_status_notification_supported != 1)
		return B_NO_ERROR;

	if(d->priv->media_change_requested) {
		DI(1) dprintf("%s report stored media change requested\n", debugprefix);
		d->priv->media_change_requested = false;
		err = B_DEV_MEDIA_CHANGE_REQUESTED;
		goto err;
	}
	tf->write.command = 0xda;	/* GET MEDIA STATUS */
	err = ide->send_ata(bus, buscookie, tf, true);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s send command failed\n", debugprefix);
		goto err;
	}
	err = bus->intwait(buscookie, 5000000);
	if (err != B_NO_ERROR) {
		DE(2) dprintf("%s intwait failed\n", debugprefix);
		goto err;
	}
	status = bus->get_altstatus(buscookie);
	if(status & ATA_BSY) {
		DE(2) dprintf("%s error, busy after interrupt\n", debugprefix);
		err = B_IO_ERROR;
		goto err;
	}
	if(status & ATA_ERR) {
		bus->read_command_block_regs(buscookie, tf, ide_mask_error);
		if(tf->read.error & 0x40) {
			DI(2) dprintf("%s media write protected\n", debugprefix);
		}
		if(tf->read.error & 0x08) {
			DI(1) dprintf("%s media change requested\n", debugprefix);
			err = B_DEV_MEDIA_CHANGE_REQUESTED;
			goto err;
		}
		if(tf->read.error & 0x02) {
			/* no media, already reported by atapi sense data, ignore */
			/*DI(2) dprintf("%s no media\n", debugprefix);*/
			/*err = B_DEV_NO_MEDIA;*/
			/*goto err;*/
		}
		if(tf->read.error & 0x20) {
			/* media changed, already reported by atapi sense data, ignore */
			/*goto err;*/
		}
		if(tf->read.error & 0x95) {
			DI(2) dprintf("%s error 0x%02x\n", debugprefix, tf->read.error);
			err = B_ERROR;
			goto err;
		}
	}
err:
	DI(2) dprintf("%s %s\n", debugprefix, strerror(err));
	return err;
}

/*------------------------------------------------------------------------------
** user_atapi_command, send atapi commands from applications
*/

static status_t
user_atapi_command(device_handle *h, raw_device_command *cmd)
{
	status_t err;
	size_t buffersize=0;
	device_entry *d = h->d;
	bool data_in;
	bool short_read_valid;
	size_t	transfer_count;
	uint8	command_buf[12];
	uint8	*command_p;
	iovec	vec;

	err = lock_memory(cmd, sizeof(raw_device_command), B_DMA_IO);
	if(err != B_NO_ERROR)
		goto err1;
	if(cmd->command_length == 12) {
		command_p = cmd->command;
	}
	else {
		if(cmd->command_length >= 6 && cmd->command_length < 12) {
			command_p = command_buf;
			memcpy(command_p, cmd->command, cmd->command_length);
			memset(command_p+cmd->command_length, 0, 12-cmd->command_length);
		}
		else {
			err = B_NOT_ALLOWED;
			goto err2;
		}
	}
	data_in = cmd->flags & B_RAW_DEVICE_DATA_IN ? true : false;
	short_read_valid = cmd->flags & B_RAW_DEVICE_SHORT_READ_VALID ? true:false;
	if(cmd->data) {
		buffersize = cmd->data_length;
		err = lock_memory(cmd->data, buffersize,
		                  B_DMA_IO | (data_in ? B_READ_DEVICE : 0));
		if(err != B_NO_ERROR)
			goto err2;
	}
	else {
		if(cmd->data_length != 0) {
			DE(1) dprintf("IDE ATAPI -- user_atapi_command: "
			              "no transfer buffer provided\n");
			err = B_BAD_VALUE;
			goto err2;
		}
	}
	if(cmd->sense_data) {
		err = lock_memory(cmd->sense_data, cmd->sense_data_length,
		                  B_DMA_IO | B_READ_DEVICE);
		if(err != B_NO_ERROR)
			goto err3;
	}
	else {
		if(cmd->sense_data_length != 0) {
			DE(1) dprintf("IDE ATAPI -- user_atapi_command: "
			              "no mode sense buffer provided\n");
			err = B_BAD_VALUE;
			goto err3;
		}
	}
	if(cmd->timeout < 10000) { /* allow at least 10 ms */
		err = B_TIMED_OUT;
		goto err3;
	}
	err = acquire_bus(d);
	if(err != B_NO_ERROR)
		goto err4;
	err = h->status;
	if(err == B_DEV_MEDIA_CHANGED) {
		h->status = B_NO_ERROR;
		if(cmd->sense_data) {
			uint8 *sd = cmd->sense_data;
			size_t sl = cmd->sense_data_length;
			memset(sd, 0, sl);
			if(sl > 0)
				sd[0] = 0xf0;
			if(sl > 2)
				sd[2] = 0x06;	/* UNIT ATTENTION */
			if(sl > 12)
				sd[12] = 0x28;
		}
		cmd->scsi_status = 2;
		cmd->cam_status = CAM_REQ_CMP_ERR;
		err = B_NO_ERROR;
		goto err5;
	}
	if(err != B_NO_ERROR)
		goto err5;
	err = load_cd(h);
	if(err == B_DEV_NOT_READY)
		err = B_NO_ERROR;
	if(err != B_NO_ERROR)
		goto err5;
	transfer_count = cmd->data_length;
	vec.iov_base = cmd->data;
	vec.iov_len = transfer_count;
	err = send_atapi_command_t(h, command_p, &vec, 1, 0, &transfer_count,
	                           !data_in, short_read_valid, cmd->timeout,
	                           (uint8*)cmd->sense_data, cmd->sense_data_length,
	                           0);
	if(short_read_valid && err==B_WOULD_BLOCK) {
		transfer_count = 0;
		err = B_NO_ERROR;
	}
	if(err == B_NO_ERROR) {
		cmd->scsi_status = 0;
		cmd->cam_status = CAM_REQ_CMP;
	}
	else {
		cmd->scsi_status = 2;
		cmd->cam_status = CAM_REQ_CMP_ERR;
		err = B_NO_ERROR;
	}
	if(cmd->flags & B_RAW_DEVICE_REPORT_RESIDUAL) {
		cmd->data_length = transfer_count;
	}
err5:
	release_bus(d);
err4:
	if(cmd->sense_data)
		unlock_memory(cmd->sense_data, cmd->sense_data_length,
		              B_DMA_IO | B_READ_DEVICE);
err3:
	if(cmd->data) {
		unlock_memory(cmd->data, buffersize,
		              B_DMA_IO | (data_in ? B_READ_DEVICE : 0));
	}
err2:
	unlock_memory(cmd, sizeof(raw_device_command), B_DMA_IO);
err1:
	return err;
}


/*------------------------------------------------------------------------------
** block io functions
*/

uint32 get_capacity(void *cookie)
{
	device_handle *h = cookie;

	if(!h->priv->li->capacity_valid) {
		if(acquire_bus(h->d) != B_NO_ERROR)
			return 0;
		update_capacity_data(h);
		release_bus(h->d);
	}
	return h->priv->li->capacity;
}

uint32 get_blocksize(void *cookie)
{
	device_handle *h = cookie;
	return h->priv->li->blocksize;
}

uint32 get_max_transfer_size(void * UNUSED(cookie))
{
	UNUSED_ARG(cookie)
	return 1024*1024;	/* limit transfersize to 1MB */
}

uint32 get_bad_alignment_mask(void *cookie)
{
	device_handle *h = cookie;
	device_entry *d = h->d;

	if(d->di->capabilities.dma_supported && d->priv->dma_enabled)
		return d->bad_alignment_mask;
	else
		return 0;
}

status_t read_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
                     const iovec *vec, size_t veccount, uint32 startbyte)
{
	device_handle *h = cookie;
	status_t err;

	if(h->status != B_NO_ERROR) {
		return h->status;
	}
	do {
		err = acquire_bus(h->d);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("IDE ATAPI -- read_blocks: %s,\n"
			              "                          failed to acquire bus, "
			              "%s\n", h->name, strerror(err));
			break;
		}
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = readsector(h, start_block, num_blocks,
			                 vec, veccount, startbyte);

		release_bus(h->d);
		if(err == B_DEV_NOT_READY) {
			snooze(200*MILLISEC);
		}
	} while(err == B_DEV_NOT_READY);
	
	return err;
}

status_t write_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
                      const iovec *vec, size_t veccount, uint32 startbyte)
{
	device_handle *h = cookie;
	status_t err;

	if(h->status != B_NO_ERROR) {
		return h->status;
	}
	do {
		err = acquire_bus(h->d);
		if (err != B_NO_ERROR) {
			DE(2) dprintf("IDE ATAPI -- write blocks: %s,\n"
			              "                           failed to acquire bus, "
			              "%s\n", h->name, strerror(err));
			break;
		}

		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = writesector(h, start_block, num_blocks,
			                  vec, veccount, startbyte);

		release_bus(h->d);
		if(err == B_DEV_NOT_READY) {
			snooze(200*MILLISEC);
		}
	} while(err == B_DEV_NOT_READY);

	return err;
}

/*------------------------------------------------------------------------------
** Device entry points
*/

static status_t
atapi_open(const char *name, uint32 UNUSED(flags), device_handle **h)
{
	const char * const debugprefix = "IDE ATAPI -- open:";
	status_t err;
	device_entry *d;
	UNUSED_ARG(flags)
	
	if ((err = open_device(name, h, IDE_ATAPI)) != B_NO_ERROR) {
		DE(1) dprintf("%s could not open %s\n", debugprefix, name);
		return err;
	}
	(*h)->priv = malloc(sizeof(driver_device_handle));
	if((*h)->priv == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}
	(*h)->priv->li = &(*h)->d->priv->lia[(*h)->lun];
	(*h)->priv->command_in_progress = false;

	d = (*h)->d;
	err = acquire_bus(d);
	if(err != B_NO_ERROR) {
		DE(2) dprintf("%s %s not ready\n", debugprefix, name);
		goto err2;
	}
	(*h)->not_ready_on_open = true;
	err = load_cd(*h);
	if(err == B_DEV_MEDIA_CHANGED)
		err = load_cd(*h);
	if(err == B_NO_ERROR) {
		err = test_unit_ready(*h);
		if(err == B_DEV_MEDIA_CHANGED)
			err = test_unit_ready(*h);
	}

	(*h)->not_ready_on_open = err == B_DEV_NOT_READY;

	release_bus(d);

	return B_NO_ERROR;

err2:
	free((*h)->priv);
err1:
	close_device(*h);
	return err;
}

static status_t
atapi_close(device_handle * UNUSED(h))
{
	UNUSED_ARG(h)
	return B_NO_ERROR;
}

static status_t
atapi_free(device_handle *h)
{
	free(h->priv);
	return close_device(h);
}

static status_t
atapi_control(device_handle *h, uint32 msg, void *arg1, size_t UNUSED(len))
{
	const char * const debugprefix = "IDE ATAPI -- control:";
	device_entry *d = h->d;
	lun_info *li = h->priv->li;
	ide_device_info *di = d->di;
	status_t err;
	UNUSED_ARG(len)

	//dprintf("IDE ATAPI -- control: atapi_control(0x%08x)\n", msg);
	switch (msg) {
	case B_GET_MEDIA_STATUS:
		DF(1) dprintf("%s B_GET_MEDIA_STATUS\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;

		err = h->status;
		if(err != B_NO_ERROR)
			h->status = B_NO_ERROR; /* B_GET_MEDIA_STATUS reset state */
		else
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = check_media_change_requested(h);
		if(err == B_NO_ERROR)
			err = test_unit_ready(h);
		if(err == B_DEV_MEDIA_CHANGED)
			h->status = B_NO_ERROR; /* B_GET_MEDIA_STATUS reset state */
		release_bus(d);
		(*(status_t*)arg1) = err;
		return B_NO_ERROR;
		
	case B_GET_GEOMETRY: {
		uint32	capacity, blocksize;
		device_geometry *dinfo = (device_geometry *) arg1;

		DF(1) dprintf("%s B_GET_GEOMETRY\n", debugprefix);
		
		dinfo->sectors_per_track = 1;

		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		if(!h->priv->li->capacity_valid) {
			update_capacity_data(h);
		}
		err = h->status;
		capacity = li->capacity;
		blocksize = li->blocksize;
		release_bus(d);

		dinfo->cylinder_count   = capacity;
		dinfo->bytes_per_sector = blocksize;
		
		dinfo->head_count        = 1;

		dinfo->removable = di->general_configuration.removable_media;

		dinfo->device_type = di->general_configuration.device_type;
		dinfo->write_once = dinfo->device_type == B_WORM;
		dinfo->read_only = li->write_protect;
		return err;
	}	
#if 0
	case B_FORMAT_DEVICE:
		if (!*((char *) arg1))
			return B_NO_ERROR;
		
		block = (uchar*)malloc(d->blocksize);
		memset(block, 0, d->blocksize);
		sectors = d->sectors_per_cylinder * d->cylinders;
		for (loop = 0; loop < sectors; loop++) {
			sz = d->blocksize;
			err = ide_write(d, loop * d->blocksize, block, &sz);
			if (err || (sz != d->blocksize)) {
				free(block);
				return B_ERROR;
			}
		}
		free(block);
		return B_NO_ERROR;
#endif
	case B_GET_DEVICE_SIZE: {
		size_t	device_size;
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		device_size = li->capacity * li->blocksize;
		release_bus(d);		
		*(size_t*)arg1 = device_size;
		DF(1) dprintf("%s B_GET_DEVICE_SIZE, size = %ld\n", debugprefix,
		              *(size_t*)arg1);
		return err;
	}
		
	case B_GET_ICON:
		DF(1) dprintf("%s B_GET_ICON\n", debugprefix);
		switch (((device_icon *)arg1)->icon_size) {
		case B_LARGE_ICON:
			if (di->general_configuration.device_type == 0x5) {
				memcpy(((device_icon *)arg1)->icon_data, &icon_cd,
					   B_LARGE_ICON * B_LARGE_ICON);
			} else if (di->general_configuration.removable_media) {
				memcpy(((device_icon *)arg1)->icon_data, &icon_removable,
					   B_LARGE_ICON * B_LARGE_ICON);
			} else {
				memcpy(((device_icon *)arg1)->icon_data, &icon_disk,
					   B_LARGE_ICON * B_LARGE_ICON);
			}
			break;
		case B_MINI_ICON:
			if (di->general_configuration.device_type == 0x5) {
				memcpy(((device_icon *)arg1)->icon_data, &icon_cd_mini,
					   B_MINI_ICON * B_MINI_ICON);
			} else if (di->general_configuration.removable_media) {
				memcpy(((device_icon *)arg1)->icon_data, &icon_removable_mini,
					   B_MINI_ICON * B_MINI_ICON);
			} else {
				memcpy(((device_icon *)arg1)->icon_data, &icon_disk_mini,
					   B_MINI_ICON * B_MINI_ICON);
			}
			break;
		default:
			return B_BAD_TYPE;
		}
		return B_NO_ERROR;

	case B_SCSI_INQUIRY:
		DF(1) dprintf("%s B_SCSI_INQUIRY\n", debugprefix);
		err = lock_memory(arg1, sizeof(scsi_inquiry), B_DMA_IO | B_READ_DEVICE);
		if(err == B_NO_ERROR) {
			err = acquire_bus(d);
			if(err == B_NO_ERROR) {
				err = h->status;
				if(err == B_NO_ERROR)
					err = load_cd(h);
				if(err == B_NO_ERROR)
					err = atapi_inquiry(h, (scsi_inquiry *)arg1);
				release_bus(d);
			}
			unlock_memory(arg1, sizeof(scsi_inquiry), B_DMA_IO | B_READ_DEVICE);
		}
		return err;

	case B_SCSI_GET_TOC:
		DF(1) dprintf("%s B_SCSI_GET_TOC\n", debugprefix);
		err = lock_memory(arg1, sizeof(scsi_toc), B_DMA_IO | B_READ_DEVICE);
		if(err == B_NO_ERROR) {
			err = acquire_bus(d);
			if(err == B_NO_ERROR) {
				err = h->status;
				if(err == B_NO_ERROR)
					err = load_cd(h);
				if(err == B_NO_ERROR)
					err = read_toc(h, (scsi_toc *)arg1);
				release_bus(d);
			}
			unlock_memory(arg1, sizeof(scsi_toc), B_DMA_IO | B_READ_DEVICE);
		}
		return err;

	case B_SCSI_PLAY_TRACK: {
		scsi_play_track spt = *(scsi_play_track *)arg1;
		DF(1) dprintf("%s B_SCSI_PLAY_TRACK\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = play_audio_track(h, &spt);
		release_bus(d);
		return err;
	}

	case B_SCSI_PLAY_POSITION: {
		scsi_play_position spp = *(scsi_play_position *)arg1;
		DF(1) dprintf("%s B_SCSI_PLAY_POSITION\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = play_audio_msf(h, &spp);
		release_bus(d);
		return err;
	}

	case B_SCSI_STOP_AUDIO:
		DF(1) dprintf("%s B_SCSI_STOP_AUDIO\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = stop_play_scan_cd(h);
		release_bus(d);
		return err;

	 case B_SCSI_PAUSE_AUDIO:
		DF(1) dprintf("%s B_SCSI_PAUSE_AUDIO\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = pause_resume(h, false);
		release_bus(d);
		return err;

	case B_SCSI_RESUME_AUDIO:
		DF(1) dprintf("%s B_SCSI_RESUME_AUDIO\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = pause_resume(h, true);
		release_bus(d);
		return err;

	 case B_SCSI_GET_POSITION:
		DF(1) dprintf("%s B_SCSI_GET_POSITION\n", debugprefix);
		err = lock_memory(arg1, sizeof(scsi_position), B_DMA_IO|B_READ_DEVICE);
		if(err == B_NO_ERROR) {
			err = acquire_bus(d);
			if(err == B_NO_ERROR) {
				err = h->status;
				if(err == B_NO_ERROR)
					err = load_cd(h);
				if(err == B_NO_ERROR)
					err = get_current_position(h, (scsi_position *) arg1);
				release_bus(d);
			}
			unlock_memory(arg1, sizeof(scsi_position), B_DMA_IO|B_READ_DEVICE);
		}
		return err;

	case B_SCSI_GET_VOLUME: {
		scsi_volume sv;
		DF(1) dprintf("%s B_SCSI_GET_VOLUME\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = get_volume(h, &sv);
		release_bus(d);
		*(scsi_volume *)arg1 = sv;
		return err;
	}

	case B_SCSI_SET_VOLUME: {
		scsi_volume sv = *(scsi_volume *)arg1;
		DF(1) dprintf("%s B_SCSI_SET_VOLUME\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = set_volume(h, &sv);
		release_bus(d);
		return err;
	}

	case B_SCSI_READ_CD: {
		scsi_read_cd read_info = *(scsi_read_cd *)arg1;
		DF(1) dprintf("%s B_SCSI_READ_CD\n", debugprefix);
		err = lock_memory(read_info.buffer, read_info.buffer_length,
		                  B_DMA_IO | B_READ_DEVICE);
		if(err == B_NO_ERROR) {
			err = acquire_bus(d);
			if(err == B_NO_ERROR) {
				err = h->status;
				if(err == B_NO_ERROR)
					err = load_cd(h);
				if(err == B_NO_ERROR)
					err = read_cd(h, &read_info);
				release_bus(d);
			}
			unlock_memory(read_info.buffer, read_info.buffer_length,
			              B_DMA_IO | B_READ_DEVICE);
		}
		return err;
	}
	
	case B_SCSI_SCAN: {
		scsi_scan ss = *(scsi_scan *) arg1;
		DF(1) dprintf("%s B_SCSI_SCAN\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
		if(err == B_NO_ERROR)
			err = scan_audio(h, &ss);
		release_bus(d);
		return err;
	}

	case B_EJECT_DEVICE:
	case B_SCSI_EJECT:
		DF(1) dprintf("%s B_EJECT_DEVICE\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		if(d->priv->locked)
			err = B_NOT_ALLOWED;
		else
			err = h->status;
		if(err == B_NO_ERROR) {
			err = load_cd(h);
			if(err == B_DEV_NOT_READY)
				err = B_NO_ERROR;
		}
		if(err == B_NO_ERROR)
			err = start_stop_unit(h, false, true);
		release_bus(d);
		return err;
		
	case B_LOAD_MEDIA:
		DF(1) dprintf("%s B_LOAD_MEDIA\n", debugprefix);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR) {
			err = load_cd(h);
			if(err == B_DEV_NOT_READY)
				err = B_NO_ERROR;
		}
		if(err == B_NO_ERROR)
			err = start_stop_unit(h, true, true);
		release_bus(d);
		return err;

	case B_SCSI_PREVENT_ALLOW: {
		bool prevent = *(bool *)arg1;
		DF(1) dprintf("%s B_SCSI_PREVENT_ALLOW %d\n", debugprefix, prevent);
		err = acquire_bus(d);
		if(err != B_NO_ERROR)
			return err;
		err = h->status;
		if(err == B_NO_ERROR)
			err = load_cd(h);
			// XXX some devices do not support individual disk locking
		if(err == B_NO_ERROR) {
			d->priv->locked = prevent;
			err = prevent_allow(h, prevent);
		}
		release_bus(d);
		return err;
	}

	case B_GET_IDE_DEVICE_INFO:
		DF(1) dprintf("%s B_GET_IDE_DEVICE_INFO\n", debugprefix);
		memcpy(arg1, h->d->di, sizeof(ide_device_info));
		return B_NO_ERROR;
	
	case B_GET_IDE_DEVICE_STATUS: {
		ide_device_status *ds = arg1;
		uint32 drive_dma_mode = ~0;
		DF(1) dprintf("%s B_GET_IDE_DEVICE_STATUS\n", debugprefix);
		if(ds->version != 1)
			return B_BAD_VALUE;
		ds->dma_status = d->priv->dma_status;
		ds->pio_mode = di->pio_cycle_timing.pio_data_transfer_mode_number;
		get_selected_dma_mode(di, &drive_dma_mode);
		ds->dma_mode = drive_dma_mode;
		return B_NO_ERROR;
	}

	case B_RAW_DEVICE_COMMAND:
		DF(1) dprintf("%s B_RAW_DEVICE_COMMAND\n", debugprefix);
		return user_atapi_command(h, arg1);

	default:
		DE(2) dprintf("%s unknown ioctl 0x%08lx\n", debugprefix, msg);
		return B_ERROR;
	}
	return B_ERROR;
}

static device_hooks atapi_device = {
	(long (*)(const char *, unsigned long, void **))&atapi_open,
	(long (*)(void *))&atapi_close,
	(long (*)(void *))&atapi_free,
	(long (*)(void *, unsigned long, void *, unsigned long))&atapi_control,
	&driver_read,
	&driver_write,
	NULL, /* select */
	NULL, /* deselect */
	&driver_readv,
	&driver_writev
};

/*------------------------------------------------------------------------------
** Driver entry points
*/

static int debug_atapi(int argc, char **argv);

#if _BUILDING_kernel || BOOT
#define ATAPI_EXPORT static
#else
#define ATAPI_EXPORT
#endif

ATAPI_EXPORT const char **
publish_devices(void)
{
	void *settingshandle;
	status_t err;

	settingshandle = load_driver_settings("atapi");
	err = create_devicelist(IDE_ATAPI);
	if(settingshandle != NULL)
		unload_driver_settings(settingshandle);
	if(err == B_NO_ERROR) {
		return (const char **)ide_names;
	}
	else {
		static const char *no_ide_names[] = {
			NULL
		};
		dprintf("IDE ATAPI -- publish_devices: failed\n");
		return no_ide_names;
	}
}

ATAPI_EXPORT device_hooks *
find_device(const char * UNUSED(name))
{
	UNUSED_ARG(name)
	return &atapi_device;
}

ATAPI_EXPORT status_t
init_driver(void)
{
	status_t err;
	
#if LOAD_ONLY_WITH_DEBUG_ENABLED
	{
		bool db = set_dprintf_enabled(true);
		set_dprintf_enabled(db);
		if(!db)
			return B_ERROR;
	}
#endif
	DF(1) dprintf("IDE ATAPI -- init_driver\n");
	err = init_scratch_buffer();
	if(err < B_NO_ERROR)
		goto err1;

	err = init_devicelist(IDE_ATAPI);
	if(err < B_NO_ERROR)
		goto err2;

	err = get_module(IDE_MODULE_NAME, (module_info **)&ide);
	if (err < B_NO_ERROR) {
		dprintf("IDE ATAPI -- init_driver: could not get ide bus manager\n");
		goto err3;
	}

	default_dma_enable = !ide_dma_disabled();

	read_settings_global();
	
	add_debugger_command("atapidb", debug_atapi, "atapidb <args> - atapi info");

	DF(1) dprintf("IDE ATAPI -- init_driver: done\n");
	return B_NO_ERROR;

err3:
	uninit_devicelist();
err2:
	uninit_scratch_buffer();
err1:
	dprintf("IDE ATAPI -- init: FAILED with error %s\n", strerror(err));
	return err;
}

ATAPI_EXPORT void
uninit_driver(void)
{
	DF(1) dprintf("IDE ATAPI -- uninit_driver\n");
	remove_debugger_command("atapidb", debug_atapi);
	uninit_devicelist();
	uninit_scratch_buffer();
	put_module(IDE_MODULE_NAME);
#ifdef CHECKMALLOC
	my_memory_used();
#endif
}

#if _BUILDING_kernel || BOOT
fixed_driver_info atapi_driver_info = {
	"atapi",
	B_CUR_DRIVER_API_VERSION,
	NULL,
	&publish_devices,
	&find_device,
	&init_driver,
	&uninit_driver
};
#else
ATAPI_EXPORT int32
api_version = B_CUR_DRIVER_API_VERSION;
#endif

/*------------------------------------------------------------------------------
** debugger
*/

static int
debug_atapi(int argc, char **argv)
{
	if (argc == 1) {
		kprintf("atapidb <command> <args>\n"
		        "atapidb help - list atapi debug commands\n");
		return 0;
	}
	if(strcmp(argv[1], "help") == 0) {
		kprintf("atapidb debug commands:\n");
		kprintf("        debug [eif] <level> - set debug level\n");
		kprintf("        dump  - print device information\n");
	}
	else if(strcmp(argv[1], "debug") == 0) {
		if(argc != 4) {
			kprintf("    debug e <level> - set error output level\n");
			kprintf("    debug i <level> - set information output level\n");
			kprintf("    debug f <level> - set flow output level\n");
		}
		else {
			switch(argv[2][0]) {
				case 'e':	debug_level_error =	atoi(argv[3]);	break;
				case 'i':	debug_level_info =	atoi(argv[3]);	break;
				case 'f':	debug_level_flow =	atoi(argv[3]);	break;
				default:
					kprintf("    debug <class> <level>, class must be "
					        "e, i or f\n");
			}
		}
	}
	else if(strcmp(argv[1], "dump") == 0) {
		extern void kdump_devicelist();
		kdump_devicelist();
	}
	else if(strcmp(argv[1], "dma") == 0) {
		extern device_entry *kget_device_ptr(int busnum, int devicenum);
		device_entry *dev;
		if(argc < 4)
			kprintf("    dma <bus> <device> <state>\n");
		else {
			dev = kget_device_ptr(atoi(argv[2]), atoi(argv[3]));
			if(dev != NULL) {
				if(dev->priv == NULL)
					kprintf("device not valid\n");
				else {
					if(argc < 5) {
						kprintf("dma %s for device %s on bus %s\n",
						        dev->priv->dma_enabled ? "enabled" : "disabled",
						        argv[3], argv[2]);
					}
					else {
						dev->priv->dma_enabled = argv[4][0] == 'e';
					}
				}
			}
		}
	}
	else {
		kprintf("unknown command %s\n", argv[1]);
	}
	return 0;
}

