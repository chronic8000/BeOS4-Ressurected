#include <KernelExport.h>
#include <Drivers.h>
#include <Mime.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <block_io.h>
#include <scsi.h>
#include <driver_settings.h>

#include <lock.h>

#if __INTEL__
#include <biosdriveinfo.h>
#endif

#if _BUILDING_kernel || BOOT
#include <drivers_priv.h>
#endif

#include "stdcomp.h"
#include "flcustom.h"
#include "flsocket.h"
#include "blockdev.h"
#include "dosformt.h"
#include "diskonc.h"
#include "nftllite.h"

#include "doch_ioctl.h"


#include <debugoutput.h>


extern NFDC21Vars* getNFDC21Vars(unsigned driveNo);
extern Anand *getAnandRec(unsigned drive);

#if defined(__MWERKS__)
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif

extern unsigned char icon_disk[];
extern unsigned char icon_disk_mini[];

#define SECTOR_SIZE (1 << SECTOR_SIZE_BITS)


typedef struct device_handle {
	uint32		device_num;
	FLStatus	volume_status;
	uint32		total_sectors;
	
	bool					valid_bios_drive_info;
	struct bios_drive_info 	bios_drive_info;
	PhysicalInfo			phys_info;
	
	lock		rw_buffer_lock;					
	char		rw_buffer[SECTOR_SIZE];
} device_handle;



static device_handle*	devices[DRIVES];
static const char*		device_names[DRIVES+1];
static const char	doch_device_root_name[] = DOCH_DEVICE_ROOT_NAME;


static void dump_phys_info(const PhysicalInfo* phys_info);
static void get_bios_drive_info(device_handle *h);
static status_t
transfer_sectors(device_handle *h, long sect, int nsect, const iovec *vec,
                 size_t veccount, uint32 startbyte, bool write);


/*------------------------------------------------------------------------------
** debug output
*/


int debug_level_error = 3;
int debug_level_info = 1;
int debug_level_flow = 0;


static status_t
map_error(FLStatus st)
{
	return (-st);	/* FIXME */
}


/*
	Format progress callback routine.
	The callback routine is called after erasing each unit,
	and its parameters are the total number of erase units
	to format and the number erased so far.
	The callback routine returns a Status value. A value of
	OK (0) allows formatting to continue. Any other value
	will abort the formatting with the returned status code.
*/
FLStatus format_progress_callback(int totalUnitsToFormat, int totalUnitsFormattedSoFar)
{
	long grid_units = 100 * totalUnitsFormattedSoFar;
	grid_units /= totalUnitsToFormat;
	grid_units *= totalUnitsToFormat;
	grid_units /=100;

	if((totalUnitsFormattedSoFar - grid_units) <= 1)
	{
		status_t	st; 
		//dprintf("Formated %d%%\n", (100*totalUnitsFormattedSoFar)/totalUnitsToFormat);
		st = snooze_etc(100, 0 /* B_DEFAULT_TIME_BASE*/, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT);
		if(st)
			dprintf("Format was interrupted.\n");
		return st;
	}
	return B_OK;
}



static status_t
mount_device_and_get_size(device_handle* h)
{
	const char*	debugprefix = DEBUGPREFIX "mount_device_and_get_size:";
	IOreq		params;
	status_t	err;	

	h->total_sectors = 0;

	params.irHandle = h->device_num;
	h->volume_status = flAbsMountVolume(&params);
	err = map_error(h->volume_status);
	if(err != B_OK)
	{
		DE(1) dprintf("%s Error %u mounting NFTL volume\n", debugprefix, h->volume_status);
		goto err0;
	}

	params.irHandle = h->device_num;
	err =  map_error(flSectorsInVolume(&params));
	if(err != B_OK)
	{
		DE(1) dprintf("%s flSectorsInVolume FAILED with status %ld \n", debugprefix, err);
	}
	else
	{
		h->total_sectors = params.irLength;
		DI(1) dprintf("%s flSectorsInVolume() = %lu\n", debugprefix, h->total_sectors);
	}
err0:
	return err;
}




static status_t
format_device(device_handle* h, IOreq* ioreq )
{
	uint32		old_total_sectors;
	status_t	err;

	old_total_sectors = h->total_sectors;	
	h->total_sectors = 0;
	
	ioreq->irHandle = h->device_num;
	((FormatParams*)(ioreq->irData))->progressCallback = format_progress_callback;
	err = flFormatVolume(ioreq);
	if(err)
		goto end;
		
	// mount and update geometry
	err = mount_device_and_get_size(h);
	if((err != B_OK) || (old_total_sectors != h->total_sectors))
		h->valid_bios_drive_info = FALSE;
end:
	return err;
}




static 	device_handle*
new_device(uint32 device_num)
{
	const char		*debugprefix = DEBUGPREFIX "new_device:";
	device_handle*	h;
	status_t		err;
	IOreq			params;
	
	h = malloc(sizeof(device_handle));
	if(!h)
	{
		DE(0) dprintf("%s no memory\n", debugprefix);
		goto err0;
	}
	
	memset(h, 0, sizeof(device_handle));
	h->device_num = device_num;

	if(new_lock(&h->rw_buffer_lock, "doch_rw_buffer_lock") != B_OK)
	{
		DE(0) dprintf("%s can't create the rw_buffer lock\n", debugprefix);
		goto err1;
	}

	params.irHandle = device_num;
	params.irData = &h->phys_info;
	err =  map_error(flGetPhysicalInfo(&params));
	if(err != B_OK)
	{
		DE(1) dprintf("%s flGetPhysicalInfo FAILED with status %ld \n", debugprefix, err);
		goto end;
	}
	else
	{
		DI(1) dump_phys_info(&h->phys_info);
	}

	if(mount_device_and_get_size(h) != B_OK)
		goto end;

	get_bios_drive_info(h);

end:
	return h;
	
err1:
	free(h);
err0:
	return NULL;
}
  

static void
delete_device(device_handle* h)
{
	if(h)
	{
		IOreq	params;

		params.irHandle = h->device_num;
		flDismountVolume(&params);
		free_lock(&h->rw_buffer_lock);
		free(h);
	}
}




static const char*
new_device_name(uint32 device_num)
{
	const char* name;

	name = malloc(sizeof(doch_device_root_name) + 4 + strlen("raw") );	/* up to 1000 disks */
	if(name == NULL)
		return NULL;
	
	sprintf((char*)name, "%s%lu/raw", doch_device_root_name, device_num);
	return name;
} 

#define delete_device_name(name_) free((void*)name_)



static status_t
uninit_devices(void)
{
	int 		i;

	for(i=0; i< DRIVES; i++)
	{
		delete_device_name(device_names[i]);
		device_names[i] = NULL;
		delete_device(devices[i]);
		devices[i] = NULL;
	}

	flExit();
	return B_OK;
}



static status_t
init_devices(void)
{
	const char *debugprefix = DEBUGPREFIX "init_devices:";
	int 		i;
	status_t	err;
	
	err = map_error(flInit());
	if(err != B_OK)
	{
		DE(1) dprintf("%s flInit() FAILED with status %ld\n", debugprefix, err);
		goto err0;
	}
	DI(1) dprintf("%s found %d disk(s)\n", debugprefix, noOfDrives);
	
	if(noOfDrives <= 0)
	{
		err = ENODEV;
		goto err1;
	}
	
	for(i=0; i< noOfDrives; i++)
	{
		devices[i] = new_device(i);
		device_names[i] = new_device_name(i);
		if((!devices[i]) || (!device_names[i]))
		{
			err = B_NO_MEMORY;
			goto err2;
		}
	}

	return B_OK;

err2:
err1:
err0:
	uninit_devices();
	return err;
}







#if __INTEL__
static uint32 get_checksum(device_handle *h, uint32 sector, uint32 num)
{
	uint32	drive_sum;
	status_t err = 0;	/* FIXME */
	uint32 *buffer;
	iovec	tmpvec;
		
	buffer = malloc(num * 0x200);
	if (!buffer) 
		goto err0;
	
	tmpvec.iov_base = buffer;
	tmpvec.iov_len = num * 0x200;
	err = transfer_sectors(h, sector, num, &tmpvec, 1, 0, false); /* read */
	drive_sum = boot_calculate_hash(buffer, num * 0x200);
	free(buffer);
	if (err != B_NO_ERROR)
		goto err0;
	return drive_sum;
	
err0:
	dprintf("DOCH -- get_sector_1_checksum: %s reading first block\n", strerror(err));
	return 0;
}


static void
get_bios_drive_info(device_handle *h)
{
	const char *debugprefix = "DOCH -- get_bios_driveinfo:";
	char my_id[17];
	struct bios_drive_info *bd;
	uint32 i;
	bool available_geometry = false;

	h->valid_bios_drive_info = FALSE;
	if(bios_drive_info == NULL)
		return;

	sprintf(my_id, "doch/%lu", h->device_num);	

	for(bd = bios_drive_info; bd->bios_id; bd++)
	{
		if(strcmp(bd->beos_id, my_id) == 0)
		{
			DI(1) dprintf("%s %s match bios drive 0x%0x\n", debugprefix, my_id, bd->bios_id);
			h->bios_drive_info = *bd;
			h->valid_bios_drive_info = TRUE;
			return;
		}
		if(bd->beos_id[0] == '\0')
			available_geometry = true;
	}
	if(!available_geometry)
	{
		DI(1) dprintf("%s all bios drives match other devices\n", debugprefix);
		return;
	}

	for(bd = bios_drive_info; bd->bios_id; bd++)
	{
		struct bios_drive_info *checkbd;

		if(bd->beos_id[0] != '\0')
			continue;

		for (i=0;i<bd->num_hashes;i++)
		{
			if (get_checksum(h, (uint32)(bd->hashes[i].offset / 0x200),
					bd->hashes[i].size / 0x200) != bd->hashes[i].hash)
				break;
		}

		if (i != bd->num_hashes) continue;

		for(checkbd = bios_drive_info; checkbd->bios_id; checkbd++)
		{
			if (bd == checkbd) continue;
			if (bd->num_hashes != checkbd->num_hashes) continue;

			for (i=0;i<bd->num_hashes;i++)
			{
				if ((bd->hashes[i].offset != checkbd->hashes[i].offset) ||
					(bd->hashes[i].size != checkbd->hashes[i].size) ||
					(bd->hashes[i].hash != checkbd->hashes[i].hash))
					break;
			}
			if (i != bd->num_hashes) continue;

			if (bd->c == checkbd->c &&
				bd->h == checkbd->h &&
				bd->s == checkbd->s)
				continue;

			DE(1) dprintf("%s first block checksum not unique\n",
			              debugprefix);
			return;
		}

		DI(1) dprintf("%s checksum of %s matched bios drive 0x%0x\n", debugprefix, my_id, bd->bios_id);
		strcpy(bd->beos_id, my_id);
		h->bios_drive_info = *bd;
		h->valid_bios_drive_info = TRUE;
		return;
	}
}
#endif // #if __INTEL__


#if 0
static status_t
read_settings_global()
{
	const char *debugprefix = DEBUGPREFIX "read_settings_global:";
	const char *drivername = "doch";
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
#endif







/*
** transfer sectors 
*/

static status_t
transfer_sectors(device_handle *h, long sect, int nsect, const iovec *vec,
                 size_t veccount, uint32 startbyte, bool write)
{
	const char *debugprefix = "DOCH -- transfersectors:";
	IOreq		params;
	status_t	err = 0;
	
	
	LOCK(h->rw_buffer_lock);	
	while (nsect > 0)
	{
		while(veccount > 0 && vec->iov_len <= startbyte)
		{
			startbyte -= vec->iov_len;
			vec++;
			veccount--;
		}
		if(veccount == 0)
			return B_BAD_VALUE;

		// do flAbsRead/flAbsWrite
		params.irHandle = h->device_num;
	    params.irData = h->rw_buffer;		
		params.irSectorCount = 1;
		params.irSectorNo =	sect;	
		if(write)
		{
			vec2memcpy(params.irData, vec, veccount, startbyte, 512);
			if(err != B_OK)
			{
				DE(1) dprintf("%s vec2memcpy() FAILED with err = %ld\n", debugprefix, err);
				goto end;
			}

			err = map_error(flAbsWrite(&params));
			if(err != B_OK)
			{
				DE(1) dprintf("%s lAbsWrite() FAILED with err = %ld\n", debugprefix, err);
				goto end;
			}
		}
		else
		{
			err = map_error(flAbsRead(&params));
			if(err != B_OK)
			{
				DE(1) dprintf("%s lAbsRead() FAILED with err = %ld\n", debugprefix, err);
				goto end;
			}
			
			mem2veccpy(vec, veccount, startbyte, params.irData, 512);
			if(err != B_OK)
			{
				DE(1) dprintf("%s mem2veccpy() FAILED with err = %ld\n", debugprefix, err);
				goto end;
			}
		}
		startbyte += 512;
		sect++;
		nsect--;
	}

end:
	UNLOCK(h->rw_buffer_lock);
	return err;
}




/*------------------------------------------------------------------------------
** block io functions
*/

uint32 get_capacity(void* cookie)
{
	const device_handle *h = cookie;

	return h->total_sectors;
}


uint32 get_blocksize(void* cookie)
{
	//const device_handle *h = cookie;
	return 512; //h->phys_info.unitSize;
}

uint32 get_bad_alignment_mask(void * UNUSED(cookie))
{
	return 0;
}

uint32 get_max_transfer_size(void * UNUSED(cookie))
{
	return 256*512;	/* we can only transfer 256 sectors at a time */	/* FIXME ???*/
}


status_t
read_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
            const iovec *vec, size_t count, uint32 startbyte)
{
	device_handle *h = cookie;
	status_t err;

	err = transfer_sectors(h, start_block, num_blocks, vec, count,
	                       startbyte, false); /* read */
	return err;
}

status_t
write_blocks(void *cookie, uint32 start_block, uint32 num_blocks,
             const iovec *vec, size_t count, uint32 startbyte)
{
	device_handle *h = cookie;
	status_t err;

	err = transfer_sectors(h, start_block, num_blocks, vec, count,
		                       startbyte, true); /* write */
	return err;
}



/*------------------------------------------------------------------------------
** Device entry points
*/

static status_t
doch_open(const char *name, uint32 UNUSED(flags), device_handle **h)
{
	int			i;
	
	i = atoi(name + strlen(doch_device_root_name)); 

	if((i<0) || (i >= noOfDrives))
		return B_DEV_BAD_DRIVE_NUM;

	*h = devices[i];

	//get_media_status(*h);	/* FIXME ??? */
	return B_OK;
}

static status_t
doch_close(device_handle * UNUSED(h))
{
	return B_NO_ERROR;
}

static status_t
doch_free(device_handle *h)
{
	return B_OK;
}

static status_t
doch_control(device_handle *h, uint32 msg, void *arg1, size_t UNUSED(len))
{
	const char *debugprefix = "DOCH -- control:";
	IOreq	params;

	DF(2) dprintf("%s msg = 0x%08lx\n", debugprefix, msg);
	switch (msg)
	{
		case B_GET_MEDIA_STATUS:
			params.irHandle = h->device_num;
			(*(status_t*)arg1) = map_error(flCheckVolume(&params));
			return B_NO_ERROR;

		case B_GET_GEOMETRY:
		{
			device_geometry *dinfo = (device_geometry *) arg1;

			dinfo->sectors_per_track = 1;
			dinfo->cylinder_count    = h->total_sectors;
			dinfo->head_count        = 1;
			
			dinfo->bytes_per_sector = 512;
			dinfo->removable = FALSE;	/* FIXME */
			dinfo->read_only = FALSE;	/* FIXME should reflect the media */
			dinfo->device_type = B_DISK;
			dinfo->write_once = FALSE;
			return B_NO_ERROR;
		}
		
#if __INTEL__
		case B_GET_BIOS_GEOMETRY:
		{
			device_geometry *dinfo = (device_geometry *) arg1;
	
			if(!h->valid_bios_drive_info)
				return B_ERROR;
			
			dinfo->sectors_per_track = h->bios_drive_info.s;
			dinfo->cylinder_count    = h->bios_drive_info.c;
			dinfo->head_count        = h->bios_drive_info.h;
			dinfo->bytes_per_sector = 512;
			dinfo->removable = FALSE;
			dinfo->read_only = FALSE;
			dinfo->device_type = B_DISK;
			dinfo->write_once = FALSE;
			return B_NO_ERROR;
		}
		
		case B_GET_BIOS_DRIVE_ID:
		{
			uint8	*bios_id_p = (uint8*) arg1;

			if(!h->valid_bios_drive_info)
				return B_ERROR;
			*bios_id_p = h->bios_drive_info.bios_id;
			return B_NO_ERROR;
		}
#endif
		case B_FORMAT_DEVICE:
		{
			status_t			err;
			FormatParams	format_params = STD_FORMAT_PARAMS; 
			
			if (!*((char *) arg1))
				return B_NO_ERROR;

			params.irHandle = h->device_num;
			params.irFlags = TL_FORMAT | TL_FORMAT_ONLY;
			params.irData = &format_params;
			err = format_device(h, &params);
			if(err != B_OK)	DE(0) dprintf("%s Format err = %ld\n", debugprefix, err);

#if 0		
			memset(h->rw_buffer, 0xdB, SECTOR_SIZE);
			params.irHandle = h->device_num;
	    	params.irData = h->rw_buffer;		
			params.irSectorCount = 1;

			for(params.irSectorNo = 0; params.irSectorNo < h->total_sectors; params.irSectorNo++)
			{
				err = map_error(flAbsWrite(&params));
				if(err != B_OK)
				{
					dprintf("Format write err = %d at sector %d\n", err, params.irSectorNo);
					break;
				}
			}
#endif
			return err;
		}

		case B_GET_DEVICE_SIZE:
			*(size_t*)arg1 = h->total_sectors * 512;
			return B_NO_ERROR;
			
		case B_GET_ICON:
			switch (((device_icon *)arg1)->icon_size)
			{
			case B_LARGE_ICON:
				memcpy(((device_icon *)arg1)->icon_data, &icon_disk, B_LARGE_ICON * B_LARGE_ICON);
				break;
			
			case B_MINI_ICON:
				memcpy(((device_icon *)arg1)->icon_data, &icon_disk_mini, B_MINI_ICON * B_MINI_ICON);
				break;

			default:
				return B_BAD_TYPE;
			}
			return B_NO_ERROR;

		
		case B_FLUSH_DRIVE_CACHE:
			return B_UNSUPPORTED;
			
		/* *********** DOCH specific ioctl codes ************ */
		/* FIXME validate user mode buffers */
		case DOCH_copyNFDC21Vars:
			memcpy(arg1, getNFDC21Vars(h->device_num), sizeof(NFDC21Vars));
			return B_OK;

		case DOCH_copyAnandRec:
			memcpy(arg1, getAnandRec(h->device_num), sizeof(Anand));
			return B_OK;

		case DOCH_copyANANDPhysUnits:
			{
				const Anand* const anand_rec = getAnandRec(h->device_num);
				memcpy(arg1, anand_rec->physicalUnits, anand_rec->noOfUnits * sizeof(ANANDPhysUnit));
				return B_OK;
			}

		case DOCH_bdCall:
			{
				status_t err;
				DOCH_bdCall_buffer* const args = (DOCH_bdCall_buffer*)arg1;
				
				args->ioreq.irHandle = h->device_num;
				DF(3) dprintf("%s bdCall(%d)\n", debugprefix, args->functionNo);
				switch(args->functionNo)
				{
				case BD_FORMAT_VOLUME:
					err = format_device(h, &args->ioreq);
					break;

				default:
					err = map_error(bdCall(args->functionNo, &args->ioreq));
				}
				if(err != B_OK)
				{
					DE(0) dprintf("%s bdCall(%d) FAILED with error = %ld\n", debugprefix, args->functionNo, err);
				}
				
				return err;
			}
	}
	DE(2) dprintf("%s unknown ioctl 0x%08lx\n", debugprefix, msg);
	return B_DEV_INVALID_IOCTL;
}


static device_hooks doch_device = {
	(long (*)(const char *, unsigned long, void **))&doch_open,
	(long (*)(void *))&doch_close,
	(long (*)(void *))&doch_free,
	(long (*)(void *, unsigned long, void *, unsigned long))&doch_control,
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

static int debug_doch(int argc, char **argv);

#if _BUILDING_kernel || BOOT
#define DOCH_EXPORT static
#else
#define DOCH_EXPORT _EXPORT
#endif

DOCH_EXPORT const char **
publish_devices()
{
	return device_names;
}



DOCH_EXPORT device_hooks *
find_device(const char * UNUSED(name))
{
	return &doch_device;
}



DOCH_EXPORT status_t
init_driver()
{
	const char *debugprefix = "DOCH -- init:";
	status_t err;
	
#if LOAD_ONLY_WITH_DEBUG_ENABLED
	{
		bool db = set_dprintf_enabled(true);
		set_dprintf_enabled(db);
		if(!db)
			return B_ERROR;
	load_driver_symbols("doch");
	}
#endif

	DF(1) dprintf("%s, built for BeIA 1.1\n");
	
	err = init_scratch_buffer();
	if(err < B_NO_ERROR)
		goto err0;

	err = init_devices();
	if(err < B_NO_ERROR)
		goto err2;


	//read_settings_global();

	add_debugger_command("dochdb", debug_doch, "dochdb <args> - doch info");

	DF(1) dprintf("%s done\n", debugprefix);

	return B_NO_ERROR;

err2:
	uninit_scratch_buffer();
err0:
	DE(1) dprintf("%s FAILED with error %s\n", debugprefix, strerror(err));
	return err;
}



DOCH_EXPORT void
uninit_driver()
{
	DF(1) dprintf("DOCH -- uninit\n");
	remove_debugger_command("dochdb", debug_doch);
	uninit_devices();
	uninit_scratch_buffer();
}



#if _BUILDING_kernel || BOOT
fixed_driver_info doch_driver_info = {
	"doch",
	B_CUR_DRIVER_API_VERSION,
	NULL,
	&publish_devices,
	&find_device,
	&init_driver,
	&uninit_driver
};
#else
DOCH_EXPORT int32
api_version = B_CUR_DRIVER_API_VERSION;
#endif

#ifdef __ZRECOVER
# include <recovery/driver_registry.h>
fixed_driver_info disk_on_chip_driver=
{
 	"Disk On Chip",
    B_CUR_DRIVER_API_VERSION,
    NULL,
    publish_devices,
    find_device,
    init_driver,
    uninit_driver
};
REGISTER_STATIC_DRIVER(disk_on_chip_driver);         
#endif

/*------------------------------------------------------------------------------
** debugger
*/


static void
dump_phys_info(const PhysicalInfo* phys_info)
{
	dprintf("flash type %u, unit size %ld, media size %ld, chip size %ld, interleaving %d\n",
		phys_info->type, phys_info->unitSize, phys_info->mediaSize, phys_info->chipSize, phys_info->interleaving);
}


static void
kdump_devices(void)
{
	int		i;
	
	for(i=0; i<noOfDrives; i++)
	{
		/* FIXME implement */
	}
}

static int
debug_doch(int argc, char **argv)
{
	if (argc == 1) {
		kprintf("dochdb <command> <args>\n"
		        "dochdb help - list doch debug commands\n");
		return 0;
	}
	if(strcmp(argv[1], "help") == 0) {
		kprintf("dochdb debug commands:\n");
		kprintf("      debug [eif] <level> - set debug level\n");
		kprintf("      dump  - print device information\n");
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
		kdump_devices();
	}
	else {
		kprintf("unknown command %s\n", argv[1]);
	}
	return 0;
}

