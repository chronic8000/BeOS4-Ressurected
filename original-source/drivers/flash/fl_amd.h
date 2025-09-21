/* ++++++++++
	FILE:	fl_amd.h
	REVS:	$Revision: 1.2 $
	NAME:	herold
	DATE:	Feb 3, 1992
	Copyright (c) 1992-96 by Be Incorporated.  All Rights Reserved.

	Definitions for programming the Advanced Micor Devices 29F010 
	and 29F040 flash roms.

	For more info, see the AMD 'Flash Memory Products' databook.
+++++ */

#ifndef _FL_AMD_H
#define _FL_AMD_H

/* identification */

#define FL_MANUFACTURER_amd				0x01
#define FL_DEVICE_amd29f010				0x20
#define FL_DEVICE_amd29f040				0xa4

/* sizes */

#define FL_BLOCK_SIZE_amd29f010			0x4000
#define FL_BLOCK_SIZE_amd29f040			0x10000

/* all commands start with 2 write operations to particular addresses */

#define FLCMD_PREFIX1_ADDR_amd	0x5555
#define FLCMD_PREFIX1_DATA_amd	0xAA
#define FLCMD_PREFIX2_ADDR_amd	0x2AAA
#define FLCMD_PREFIX2_DATA_amd	0x55

/* command codes for operating on a single part */

#define FLCMD_READ_amd			0xF0
#define FLCMD_AUTOSELECT_amd	0x90
#define FLCMD_ERASESETUP_amd	0x80
#define FLCMD_ERASECONFIRM_amd	0x30
#define FLCMD_PROGRAMSETUP_amd	0xA0

/* status bits (single part) */

#define FLST_DATA_amd			0x80		/* data polling */
#define FLST_TOGGLE_amd			0x40		/* toggles while busy */
#define FLST_TIMEOUT_amd		0x20		/* pgm/erase timeout */
#define FLST_SEQ_amd			0x10		/* if timeout, 1=erase 0=pgm */
#define FLST_SECTOR_amd			0x08		/* 0=not yet, 1=erasing */

#endif

