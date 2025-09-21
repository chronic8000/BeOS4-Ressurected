/* ++++++++++
	scsiprobe.c
+++++ */

#include <BeBuild.h>
#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include "scsiprobe_driver.h"


static cam_for_driver_module_info	*cam;
static char cam_name[] = B_CAM_FOR_DRIVER_MODULE_NAME;

/* ----------
	scsi_open - called on each open().
----- */

static status_t
scsi_open (const char* _UNUSED(name), uint32 _UNUSED(flags), void** cookie)
{
	*cookie = NULL;
	return B_NO_ERROR;
}


/* ----------
	scsi_close
----- */

static status_t
scsi_close (void* _UNUSED(dev))
{
	return B_NO_ERROR;
}

/* ----------
	ide_free
----- */

static status_t
scsi_free (void* _UNUSED(dev))
{
	return B_NO_ERROR;
}

/* ----------
	scsi_read - does nothing
----- */

static status_t
scsi_read (void* _UNUSED(dev), off_t _UNUSED(pos), void* _UNUSED(buf), size_t* _UNUSED(len))
{
	return B_NO_ERROR;
}


/* ----------
	scsi_write - does nothing
----- */

static status_t
scsi_write (void* _UNUSED(dev), off_t _UNUSED(pos),
			const void* _UNUSED(buf), size_t* _UNUSED(len))
{
	return B_NO_ERROR;
}


/* ----------
	scsi_control
----- */

static status_t
scsi_control (void* _UNUSED(dev), uint32 msg, void *buf, size_t _UNUSED(len))
{
	long								result = B_ERROR;
	CCB_HEADER 							*ccbh;
	CCB_SCSIIO							*ccb;
	scsiprobe_extended_path_inquiry	*extended;
	scsiprobe_format					*format;
	scsiprobe_inquiry					*inquiry;
	scsiprobe_path_inquiry				*path;
	scsiprobe_reset						*reset;
	system_info							info;

	switch (msg) {
		case B_SCSIPROBE_INQUIRY:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			inquiry = (scsiprobe_inquiry *)buf;
			ccbh->cam_path_id = inquiry->path;
			ccbh->cam_target_id = inquiry->id;
			ccbh->cam_target_lun = inquiry->lun;

			ccb = (CCB_SCSIIO *)ccbh;

			ccbh->cam_flags = CAM_DIR_IN + CAM_DIS_DISCONNECT;
    		ccb->cam_dxfer_len = inquiry->len;
			ccb->cam_data_ptr = (uchar *)&inquiry->data;
			ccb->cam_cdb_len = 6;						/* scsi cmd len */
    		ccb->cam_cdb_io.cam_cdb_bytes[0] = 0x12;	/* INQUIRY cmd */
    		ccb->cam_cdb_io.cam_cdb_bytes[1] =			/* lun */
					ccbh->cam_target_lun << 5;
			ccb->cam_cdb_io.cam_cdb_bytes[2] = 0x00;	/* page code */
			ccb->cam_cdb_io.cam_cdb_bytes[3] = 0x00;	/* reserved */
			ccb->cam_cdb_io.cam_cdb_bytes[4] = inquiry->len;
			ccb->cam_cdb_io.cam_cdb_bytes[5] = 0x00;	/* control */

			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP)
				result = B_NO_ERROR;
			(*cam->xpt_ccb_free) (ccbh);
			return result;

		case B_SCSIPROBE_RESET:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			reset = (scsiprobe_reset *)buf;
			ccbh->cam_path_id = reset->path;
			ccbh->cam_func_code = XPT_RESET_BUS;
			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP)
				result = B_NO_ERROR;
			(*cam->xpt_ccb_free) (ccbh);
			return result;

		case B_SCSIPROBE_VERSION:
			return 4;

		case B_SCSIPROBE_HIGHEST_PATH:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			ccbh->cam_path_id = 0xff;
			ccbh->cam_func_code = XPT_PATH_INQ;
			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP)
				result = ((CCB_PATHINQ *)ccbh)->cam_hpath_id;
			(*cam->xpt_ccb_free) (ccbh);
			return result;

		case B_SCSIPROBE_PATH_INQUIRY:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			path = (scsiprobe_path_inquiry *)buf;
			ccbh->cam_path_id = path->path;
			ccbh->cam_func_code = XPT_PATH_INQ;
			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP) {
				memcpy(&path->data, ccbh, sizeof(CCB_PATHINQ));
				result = B_NO_ERROR;
			}
			(*cam->xpt_ccb_free) (ccbh);
			return result;

		case B_SCSIPROBE_PLATFORM:
			get_system_info(&info);
			return info.platform_type;

		case B_SCSIPROBE_EXTENDED_PATH_INQUIRY:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			extended = (scsiprobe_extended_path_inquiry *)buf;
			ccbh->cam_path_id = extended->path;
			ccbh->cam_func_code = XPT_EXTENDED_PATH_INQ;
			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP) {
				memcpy(&extended->data, ccbh, sizeof(CCB_EXTENDED_PATHINQ));
				result = B_NO_ERROR;
			}
			else {
				ccbh->cam_func_code = XPT_PATH_INQ;
				(*cam->xpt_action) (ccbh);
	
				if (ccbh->cam_status == CAM_REQ_CMP) {
					memcpy(&extended->data, ccbh, sizeof(CCB_PATHINQ));
					sprintf(extended->data.cam_sim_version, "%d.0", extended->data.cam_path.cam_version_num);
					sprintf(extended->data.cam_hba_version, "%d.0", extended->data.cam_path.cam_version_num);
					strcpy(extended->data.cam_controller_family, "unknown");
					strcpy(extended->data.cam_controller_type, "unknown");
					result = B_NO_ERROR;
				}
			}
			(*cam->xpt_ccb_free) (ccbh);
			return result;

		case B_SCSIPROBE_FORMAT:
			if (!(ccbh = (*cam->xpt_ccb_alloc)()))
				return result;
			format = (scsiprobe_format *)buf;
			ccbh->cam_path_id = format->path;
			ccbh->cam_target_id = format->id;
			ccbh->cam_target_lun = format->lun;

			ccb = (CCB_SCSIIO *)ccbh;

			ccbh->cam_flags = CAM_DIR_NONE + CAM_DIS_DISCONNECT;
 			ccb->cam_cdb_len = 6;						/* scsi cmd len */
    		ccb->cam_cdb_io.cam_cdb_bytes[0] = 0x04;	/* FORMAT cmd */
    		ccb->cam_cdb_io.cam_cdb_bytes[1] =			/* lun */
					ccbh->cam_target_lun << 5;
			ccb->cam_cdb_io.cam_cdb_bytes[2] = 0x00;	/* vendor specific */
			ccb->cam_cdb_io.cam_cdb_bytes[3] = 0x00;	/* interleave (part 1) */
			ccb->cam_cdb_io.cam_cdb_bytes[4] = 0x00;	/* interleave (part 2) */
			ccb->cam_cdb_io.cam_cdb_bytes[5] = 0x00;	/* control */

			(*cam->xpt_action) (ccbh);

			if (ccbh->cam_status == CAM_REQ_CMP)
				result = B_NO_ERROR;
			(*cam->xpt_ccb_free) (ccbh);
			return result;
			break;

		default:
			return B_ERROR;
	}
	return B_NO_ERROR;
}


/* -----
	driver-related structures
----- */

static const char *scsiprobe_name[] = {
	"bus/scsi/probe",
	NULL
};

static device_hooks scsiprobe_device = {
	&scsi_open,
	&scsi_close,
	&scsi_free,
	&scsi_control,
	&scsi_read,
	&scsi_write,
	NULL,
	NULL
};

const char **
publish_devices()
{
	return scsiprobe_name;
}

device_hooks *
find_device(const char *name)
{
	return &scsiprobe_device;
}

status_t
init_driver()
{
	return get_module(cam_name, (module_info **) &cam);
}

void
uninit_driver()
{
	put_module(cam_name);
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
