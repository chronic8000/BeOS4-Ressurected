/* bulk_only.h 
**
** Bulk-only transport layer for mass storage devices
**
** Based on Universal Serial Bus Mass Storage Class - Bulk-Only Transport, rev 1.0
** http://www.usb.org/developers/data/devclass/usbmassbulk_10.pdf
*/

#ifndef _BULK_ONLY_H
#define _BULK_ONLY_H

#define CBW_SIGNATURE			0x43425355
#define CSW_SIGNATURE			0x53425355

/* Class specific requests */
#define REQUEST_DEVICE_RESET	0xff
#define REQUEST_GET_MAX_LUN		0xfe

typedef struct /* from table 5.1 */
{
	int32	signature;
	int32	tag;
	int32	data_len;
	int8	flags;
	int8	lun;			/* 7..4 should be 0, 3..0 should be LUN */
	int8	cmd_len;		/* 7..5 should be 0, 4..0 should be command length */
	char	cmd[16];		/* The command being wrapped */
} _PACKED cmd_block_wrapper;

typedef struct /* from table 5.2 */
{
	int32	signature;
	int32	tag;
	int32	residue;
	int8	status;
} _PACKED cmd_status_wrapper;

enum /* command block status values - table 5.3 */
{
	CMD_PASSED = 0x00,
	CMD_FAILED = 0x01,
	PHASE_ERROR = 0x02
};

#endif
