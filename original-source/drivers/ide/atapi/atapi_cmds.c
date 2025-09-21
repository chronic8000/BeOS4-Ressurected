#include <memory.h>
#include <stdlib.h>

#include <IDE.h>
#include <KernelExport.h>
#include <ide_device_info.h>
#include <scsi.h>
#include <scsi_cd_priv.h>
#include "atapi_devicelist.h"
#include "atapi_cmds.h"
#include "atapi_changer.h"
#include <debugoutput.h>

status_t
test_unit_ready(device_handle *h)
{
	const char *debugprefix = "IDE ATAPI -- test_unit_ready:";
	size_t   count;
	uint8    command[12];
	status_t retval;

	memset(command, 0x00, sizeof(command));
	command[0] = 0;					/* TEST UNIT READY Command */
	command[4] = 0;
	count = 0;

	retval = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);
	if(retval == B_DEV_NO_MEDIA) {
		status_t err;
		uint8 buf[8];
		buf[2] = 0;
		err = mode_sense(h, 0 /* current */, 0x3f /* all */, buf, sizeof(buf));
		if(err == B_NO_ERROR) {
			DI(3) dprintf("%s medium type 0x%02x\n", debugprefix, buf[2]);
			if(buf[2] == 0x71)
				retval = B_DEV_DOOR_OPEN;
		}
		else {
			DE(4) dprintf("%s mode sense failed %s\n",
			              debugprefix, strerror(err));
		}
	}
	return retval;
}

status_t
read_capacity(device_handle *h, atapi_capacity_data *data)
{
	size_t		count;
	status_t	result;
	uint8	command[12];
	iovec	vec;

	memset(command, 0x00, sizeof(command));
	command[0] = 0x25;				/* READ CAPACITY Command */
	count = sizeof(atapi_capacity_data);
	vec.iov_base = data;
	vec.iov_len = count;
	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, false);
	return result;
}

status_t
play_audio_msf(device_handle *h, scsi_play_position *position)
{
	size_t		count = 0;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0x47;				/* PLAY AUDIO MSF Command */
	command[3] = position->start_m;
	command[4] = position->start_s;
	command[5] = position->start_f;
	h->d->priv->audio.end_m = command[6] = position->end_m;
	h->d->priv->audio.end_s = command[7] = position->end_s;
	h->d->priv->audio.end_f = command[8] = position->end_f;
	
	result = send_atapi_command(h, command, NULL, 0,0, &count, false, false);
	return result;
}

static status_t
read_subchannel(device_handle *h, uint8 data_format, uint8 *buffer, size_t buflen)
{
	size_t		count;
	status_t	result;
	uint8	command[12];
	iovec	vec;

	memset(command, 0x00, sizeof(command));
	command[0] = 0x42;				/* READ SUB-CHANNEL Command */
	command[1] = 0x02;				/* Use MSF format */
	command[2] = 0x40;				/* return Q sub-channel data */
	command[3] = data_format;
	command[7] = buflen >> 8;
	command[8] = buflen & 0xff;

	count = buflen;
	vec.iov_base = buffer;
	vec.iov_len = buflen;

	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, false);
	
	return result;
}

status_t
request_sense(device_handle *h, uint8 *prevcmd, uint8 *sense_data_buffer,
              size_t sense_data_size)
{
	const char *debugprefix = "IDE ATAPI -- request_sense:";
	uint8 cmd[12];
	uint8 default_sense_data_buffer[18];
	uint8 asc, ascq;
	status_t err;
	uint8 *sense_data;
	uint32 len;
	iovec	vec;

	if(sense_data_size >= 18) {
		sense_data = sense_data_buffer;
		len = sense_data_size;
	}
	else {
		sense_data = default_sense_data_buffer;
		len = 18;
	}

	memset(cmd, 0x0, sizeof(cmd));
	cmd[0] = 0x03;					/* REQUEST SENSE Command */
	cmd[4] = len;
	if (prevcmd[0] == 0x03) {
		DE(2) dprintf("%s request_sense requested sense\n", debugprefix);
		return B_ERROR;
	}
	
	memset(sense_data, 0x00, len);
	vec.iov_base = sense_data;
	vec.iov_len = len;

	err = send_atapi_command(h, cmd, &vec, 1, 0, &len, false, true);
	if (err == B_WOULD_BLOCK) {
		if(len > 18) {
			DE(0) dprintf("%s send_atapi_command broken, len > 18\n",
			              debugprefix);
			return B_ERROR;
		}
	} else if (err != B_NO_ERROR) {
		DE(2) dprintf("%s failed, %s,\n"
		              "%27s error = 0x%02x, ili/sense key = 0x%02x len = %ld\n",
		              debugprefix, strerror(err),
		              "", sense_data[0], sense_data[2], len);
		return B_ERROR;
	}
	
	if(sense_data_size > 0 && sense_data_size < 18)
		memcpy(sense_data_buffer, sense_data, sense_data_size);
	
	asc = sense_data[12];
	ascq = sense_data[13];
	
	switch (asc) {
	case 0x4: /* not ready  */
		if (ascq == 0x1) {
			DI(3) dprintf("%s drive in progress of becoming ready\n",
			              debugprefix);
			return B_DEV_NOT_READY;
		}
		else {
			DE(2) dprintf("%s drive not ready ascq %02x\n", debugprefix, ascq);
		}
		return B_ERROR;
		
	case 0x5:
		if (ascq == 0x1) {
			DE(2) dprintf("%s media load eject failed\n", debugprefix);
		}
		else {
			DE(2) dprintf("%s media load error\n", debugprefix);
		}
		return B_ERROR;
		
	case 0x15:
	case 0x11:
	case 0x9: /* i/o error */
		DE(2) dprintf("%s i/o error\n", debugprefix);
		return B_IO_ERROR;
		
	case 0x21:
		DE(2) dprintf("%s logical block address out of range\n", debugprefix);
		return B_DEV_SEEK_ERROR;
		
	case 0x27:
		DE(2) dprintf("%s write protected media\n", debugprefix);
		return B_READ_ONLY_DEVICE;

	case 0x28: /* Not ready to ready transition (medium changed) */
		DI(2) dprintf("%s medium changed\n", debugprefix);
		if(h->priv->command_in_progress) {
			DE(1) dprintf("%s error medium changed again\n", debugprefix);
			return B_ERROR;
		}
		h->priv->command_in_progress = true;
		err = reconfigure_device(h);		   /* XXX need to retry read */
		h->priv->command_in_progress = false;
		if(err == B_NO_ERROR)
			err = h->status;
		return err;
		break;

	case 0x29: /* device was reset */
		break;
		
	case 0x2a: /* params changed */
		DE(1) dprintf("%s parameters changed (should not be here!!!)\n",
		              debugprefix);
		return B_ERROR;
		
	case 0x30: /* incompatible format */
		DE(2) dprintf("%s incompatible format\n", debugprefix);
		return B_DEV_FORMAT_ERROR;
		
	case 0x3a: /* no medium */
		if(prevcmd[0] != 0x00 && prevcmd[0] != 0xa6)
			/* not test unit ready or load cd command */
			DE(2) dprintf("%s no media, ascq %02x\n", debugprefix, ascq);
		if(h->priv->command_in_progress) {
			//dprintf("%s no media error\n", debugprefix);
			return B_DEV_NOT_READY;
		}
		if(h->d->maxlun > 0) {
			h->priv->command_in_progress = true;
			err = check_current_slot(h);
			h->priv->command_in_progress = false;
			if(err == B_NO_ERROR)
				return B_DEV_NOT_READY;
		}
		if(h->priv->li != NULL) {
			h->priv->li->capacity = 0;
			h->priv->li->capacity_valid = true;
		}
		return B_DEV_NO_MEDIA;
		
	case 0x53: /* load or eject failed */
		if (ascq == 0x2) {
			DE(2) dprintf("%s medium removal prevented\n", debugprefix);
		} else {
			DE(2) dprintf("%s medium load/eject failed\n", debugprefix);
		}
		return B_ERROR;
		
	case 0x5a: /* operator medium removal request */
		if (ascq == 1) {
			DI(2) dprintf("%s external removal requested\n", debugprefix);
			h->d->priv->media_change_requested = true;
			return B_NO_ERROR;
		}
		DE(2) dprintf("%s unspecified state change\n", debugprefix);
		return B_ERROR;
		
	case 0x64:
		DE(2) dprintf("%s illegal mode for this track\n", debugprefix);
		return B_DEV_FORMAT_ERROR;
		
	default:
		DE(2) dprintf("%s asc = 0x%02x, ascq = 0x%02x\n",
		              debugprefix, asc, ascq);
		{
			uint8 sense_key = sense_data[2] & 0x0f;
			if(sense_key == 0 || sense_key == 1 ||
			   sense_key == 6 || sense_key == 0xB)
				return B_NO_ERROR;
			else
				return B_IO_ERROR;
		}

	/* following for debugging only */
		
	case 0x1a:
		DE(2) dprintf("%s parameter list length error\n", debugprefix);
		return B_ERROR;
		
	case 0x20:
		DE(2) dprintf("%s invalid command operation code\n", debugprefix);
		return B_NOT_ALLOWED;
		
	case 0x24:
		DE(2) dprintf("%s invalid field in command packet\n", debugprefix);
		return B_NOT_ALLOWED;
		
	case 0x26:
		DE(2) dprintf("%s invalid field in parameter list %s\n", debugprefix,
		              ascq == 0x2 ?
		                  "(invalid value)" : 
				          (ascq == 0x1 ? "parameter not supported" : ""));
		return B_NOT_ALLOWED;
	}
	
	return B_NO_ERROR;
}


status_t
readsector(device_handle *h, uint32 sect, uint32 nsect,
           const iovec *vec, size_t veccount, uint32 startbyte)
{
	uint8				command[12];
	size_t				count;
	status_t			err = B_ERROR;
	driver_device_entry	*d_priv = h->d->priv;

	DF(2) dprintf("IDE ATAPI -- readsector: sect = %ld, nsect = %ld\n",
	              sect, nsect);	

	if(d_priv->read_write_12_unsupported)
		goto read_10;

	memset(command, 0x0, sizeof(command));
	command[0] = 0xa8;				/* READ (12) Command */
	*((uint32 *)(&command[2])) = B_HOST_TO_BENDIAN_INT32(sect);
	*((uint32 *)(&command[6])) = B_HOST_TO_BENDIAN_INT32(nsect);

	count = nsect * h->priv->li->blocksize;
	err = send_atapi_command(h, command, vec, veccount, startbyte,
	                         &count, false, false);
	switch(err) {
		case B_NO_ERROR:
		case B_DEV_NO_MEDIA:
		case B_DEV_DOOR_OPEN:
		case B_DEV_NOT_READY:
			return err;
	}
	
read_10:
	memset(command, 0x0, sizeof(command));
	command[0] = 0x28;				/* READ (10) Command */
	while(nsect > 0) {
		uint16 ntsect = (nsect > 0xffff) ? 0xffff : nsect;
		*((uint32 *)(&command[2])) = B_HOST_TO_BENDIAN_INT32(sect);
		*((uint16 *)(&command[7])) = B_HOST_TO_BENDIAN_INT16(ntsect);
		count = ntsect * h->priv->li->blocksize;
		err = send_atapi_command(h, command, vec, veccount, startbyte,
		                         &count, false, false);
		if(err != B_NO_ERROR)
			return err;
		sect += ntsect;
		nsect -= ntsect;
		startbyte += ntsect * h->priv->li->blocksize;
	}
	if(err == B_NO_ERROR && !d_priv->read_write_12_unsupported) {
		DE(2) dprintf("IDE ATAPI -- readsector: %s\n"
		              "                         giving up read/write(12)\n",
		              h->name);	
		d_priv->read_write_12_unsupported = true;
	}
	return err;
}

status_t
writesector(device_handle *h, uint32 sect, uint32 nsect,
            const iovec *vec, size_t veccount, uint32 startbyte)
{
	uint8				command[12];
	size_t				count;
	status_t			err = B_ERROR;
	driver_device_entry	*d_priv = h->d->priv;

	DF(2) dprintf("IDE ATAPI -- writesector: sect = %ld, nsect = %ld\n",
	              sect, nsect);	

	if(d_priv->read_write_12_unsupported)
		goto write_10;

	memset(command, 0x0, sizeof(command));
	command[0] = 0xaa;				/* WRITE (12) Command */
	*((uint32 *)(&command[2])) = B_HOST_TO_BENDIAN_INT32(sect);
	*((uint32 *)(&command[6])) = B_HOST_TO_BENDIAN_INT32(nsect);

	count = nsect * h->priv->li->blocksize;
	err = send_atapi_command(h, command, vec, veccount, startbyte,
	                         &count, true, false);

	switch(err) {
		case B_NO_ERROR:
		case B_DEV_NO_MEDIA:
		case B_DEV_DOOR_OPEN:
		case B_DEV_NOT_READY:
			return err;
	}

write_10:
	memset(command, 0x0, sizeof(command));
	command[0] = 0x2a;				/* WRITE (10) Command */
	while(nsect > 0) {
		uint16 ntsect = (nsect > 0xffff) ? 0xffff : nsect;
		*((uint32 *)(&command[2])) = B_HOST_TO_BENDIAN_INT32(sect);
		*((uint16 *)(&command[7])) = B_HOST_TO_BENDIAN_INT16(ntsect);
		count = ntsect * h->priv->li->blocksize;
		err = send_atapi_command(h, command, vec, veccount, startbyte,
		                         &count, true, false);
		if(err != B_NO_ERROR)
			return err;
		sect += ntsect;
		nsect -= ntsect;
		startbyte += ntsect * h->priv->li->blocksize;
	}
	if(err == B_NO_ERROR && !d_priv->read_write_12_unsupported) {
		DE(2) dprintf("IDE ATAPI -- writesector: %s\n"
		              "                          giving up read/write(12)\n",
		              h->name);	
		d_priv->read_write_12_unsupported = true;
	}
	return err;
}

status_t
read_toc(device_handle *h, scsi_toc *tocp)
{
	uint8 command[12];
	size_t count;
	status_t result;
	uint32 len;
	iovec	vec;

	count = sizeof(scsi_toc);
	memset(tocp, 0, 2);

	memset(command, 0, sizeof(command));
	command[0] = 0x43;				/* READ TOC Command */
	command[1] = 0x02;				/* Use MSF format */
	command[7] = count >> 8;
	command[8] = count & 0xff;

	vec.iov_base = tocp;
	vec.iov_len = count;
	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, true);
	
	len = (tocp->toc_data[0] << 8) | tocp->toc_data[1];

	if (result == B_WOULD_BLOCK) {
		if (len > 4)
			return B_NO_ERROR;
		else
			return B_ERROR;			
	}
	
#if 0
	{
		int i;
		uint8 *b = (uint8 *) tocp;
		dprintf("IDE ATAPI -- read_toc: toc data len = %d\n",
				b[0]<<8 | b[1]);
		dprintf("IDE ATAPI -- read_toc: sessions go from %d to %d\n",
				b[2], b[3]);
		for (i = b[2]; i <= b[3]; i++) {
			uint8 *session = &b[4+(i*10)];
			int j;
			dprintf("IDE ATAPI -- read_toc: session %d\n", session[0]);
			for (j = 1; j <= 10; j++) {
				dprintf("IDE ATAPI -- read_toc: \t byte[%d] = 0x%02x\n", j, session[j]);
			}
		}
	}
			
	dprintf("IDE ATAPI -- read_toc: successful (tocp = 0x%x)\n", tocp);
#endif	
	return result;
}


status_t
atapi_inquiry(device_handle *h, scsi_inquiry *inq_data_p)
{
	uint8 command[12];
	size_t count = sizeof(scsi_inquiry);
	status_t result;
	iovec vec;

	memset(command, 0, sizeof(command));
	command[0] = 0x12;				/* INQUIRY Command */
	command[4] = sizeof(scsi_inquiry);
	
	vec.iov_base = inq_data_p;
	vec.iov_len = count;
	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, false);
	
	return result;
}

status_t
mechanism_status(device_handle *h, void *buffer, size_t buffersize)
{
	uint8 command[12];
	size_t count = buffersize;
	status_t result;
	iovec vec;

	memset(command, 0, sizeof(command));
	command[0] = 0xbd;				/* MECHANISM STATUS Command */
	command[8] = count<<8;
	command[9] = count;

	vec.iov_base = buffer;
	vec.iov_len = count;
	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, true);
	if(result == B_WOULD_BLOCK && count >= 8)
		result = B_NO_ERROR;
	
	return result;
}

status_t
mode_sense(device_handle *h, uint8 page_control, uint8 page_code, uint8 *buf, size_t buflen)
{
	size_t		count;
	status_t	result;
	uint8		command[12];
	iovec		vec;

	memset(command, 0x00, sizeof(command));
	command[0] = 0x5a;				/* MODE SENSE (10) Command */
	command[2] = (page_control << 6) | page_code;
	command[7] = buflen >> 8;
	command[8] = buflen & 0xff;

	count = buflen;
	vec.iov_base = buf;
	vec.iov_len = count;

	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, true);

	return result;
}

status_t
mode_select(device_handle *h, bool save_page, uint8 *buf, size_t buflen)
{
	size_t		count;
	status_t	result;
	uint8		command[12];
	iovec		vec;

	memset(command, 0x00, sizeof(command));
	command[0] = 0x55;				/* MODE SELECT (10) Command */
	command[1] = (save_page ? 0x11 : 0x10);
	command[7] = buflen >> 8;
	command[8] = buflen & 0xff;

	count = buflen;
	vec.iov_base = buf;
	vec.iov_len = count;

	result = send_atapi_command(h, command, &vec, 1, 0, &count, true, false);

	return result;
}

status_t
stop_play_scan_cd(device_handle *h)
{
	size_t		count;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0x4e;				/* STOP PLAY / SCAN Command */
	count = 0;

	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);

	return result;
}

status_t
pause_resume(device_handle *h, bool resume)
{
	size_t		count;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0x4b;				/* PAUSE / RESUME Command */
	command[8] = resume ? 0x1 : 0x0; 
	count = 0;

	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);

	return result;
}

status_t
load_unload_cd(device_handle *h, uint8 slot, bool load)
{
	size_t		count;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0xa6;				/* LOAD/UNLOAD CD Command*/
	command[4] = load ? 0x3 : 0x2;
	command[8] = slot;
	count = 0;

	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);

	return result;
}

status_t
start_stop_unit(device_handle *h, bool start, bool eject)
{
	size_t		count;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0x1b;				/* START STOP UNIT Command */
	command[1] = 0x00;
	command[4] = (eject << 1) | start;
	count = 0;

	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);

	return result;
}

status_t
get_current_position(device_handle *h, scsi_position *pos)
{
	return read_subchannel(h, 0x01, (uint8 *) pos, sizeof(scsi_position));
}

status_t
scan_audio(device_handle *h, scsi_scan *scan_info)
{
	uint8			command[12];
	size_t			count = 0;
	status_t		result;
	scsi_position	pos;

	if (!scan_info->direction) {
		scsi_play_position play_pos;

		result = stop_play_scan_cd(h);
		if(result != B_NO_ERROR)
			return result;
		result = get_current_position(h, &pos);
		if(result != B_NO_ERROR)
			return result;
		play_pos.start_m = pos.position[9]; 	/* Minute */
		play_pos.start_s = pos.position[10];	/* Second */
		play_pos.start_f = pos.position[11];	/* Frame */
		play_pos.end_m = h->d->priv->audio.end_m; 	/* Minute */
		play_pos.end_s = h->d->priv->audio.end_s; 	/* Second */
		play_pos.end_f = h->d->priv->audio.end_f; 	/* Frame */
		return play_audio_msf(h, &play_pos);
	} else {
		memset(command, 0, sizeof(command));
		command[0] = 0xba;			/* SCAN Command */
		if (scan_info->direction < 0) {
			command[1] = 0x10;		/* Scan backwards */
		}
		get_current_position(h, &pos);
		command[3] = pos.position[9];	/* Minute */
		command[4] = pos.position[10];	/* Second */
		command[5] = pos.position[11];	/* Frame */
		command[9] = 0x40;				/* MSF format */
	}

	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);

	return result;
}

/* ----------
	read audio data from atapi device
----- */

status_t
read_cd (device_handle *h, scsi_read_cd *read_info)
{
	uint8		command[12];
	uint32		count;
	uint32		end;
	status_t	result;
	iovec		vec;

	count = read_info->buffer_length;

	end = (read_info->length_m * 60 * 75) +	/* total frames to read */
		  (read_info->length_s * 75) +
		   read_info->length_f;

	if (count > (end * 2352)) {		/* if buffer > req_count, adjust */
		DE(2) dprintf("IDE ATAPI -- read_cd: "
		              "warning buffer size > req_count\n");
		count = end * 2352;
	}
	else if (count < (end * 2352))	/* if buffer < req_count, error */
		return B_ERROR;

	end += (read_info->start_m * 60 * 75) +		/* ending frame position */
		   (read_info->start_s * 75) +
			read_info->start_f;

	command[0] = 0xb9;				/* Read CD MSF Command */
	command[1] = 0x04;					/* Data type - Red Book CD-DA */
	command[2] = 0x00;					/* Reserved */
	command[3] = read_info->start_m;	/* Starting minute */
	command[4] = read_info->start_s;	/* Starting second */
	command[5] = read_info->start_f;	/* Starting frame */
	command[6] = end / (60 * 75);		/* Ending minute */
	end %= (60 * 75);
	command[7] = end / 75;				/* Ending second */
	end %= 75;
	command[8] = end;					/* Ending frame */
	command[9] = 0x10;					/* Flags - User data only */
	command[10] = 0x00;					/* No sub-channel data */
	command[11] = 0x00;					/* Reserved */

	vec.iov_base = read_info->buffer;
	vec.iov_len = count;
	result = send_atapi_command(h, command, &vec, 1, 0, &count, false, false);
	return result;
}

status_t
prevent_allow(device_handle *h, bool prevent)
{
	size_t		count;
	status_t	result;
	uint8	command[12];

	memset(command, 0x00, sizeof(command));
	command[0] = 0x1e;				/* PREVENT / ALLOW MEDIUM REMOVAL Command */
	command[4] = prevent & 0x1;
	count = 0;

	//dprintf("IDE ATAPI -- prevent_allow: %sing removal of %s\n",
	//		prevent ? "prevent" : "allow", d->name);
	result = send_atapi_command(h, command, NULL, 0, 0, &count, false, false);
	//d->bus->read_command_block_regs(d->buscookie, d->tf, ide_mask_sector_count);
	//dprintf("IDE ATAPI -- prevent_allow: status = 0x%02x, 0x%02x\n",
	//		d->bus->get_altstatus(d->buscookie),
	//		d->tf->read.sector_count);
	return result;
}
