
#ifndef _DPT_H
#define _DPT_H

#define PCI_DPT_VENDOR_ID         0x1044   // DPT PCI Vendor ID               
#define PCI_DPT_DEVICE_ID         0xA501   // DPT PCI I2O Device ID

#include <i2o.h>

typedef struct dpt_card
{
	struct dpt_card *next;
	i2o_info *ii;
	int num;	
} dpt_card;

typedef struct dpt_disk
{
	i2o_target *targ;	
	dpt_card *card;
	i2o_target target;
	
	char devname[64];
	size_t blocks;
	size_t blocksize;
	
	struct dpt_disk *next;	
} dpt_disk;

#endif
