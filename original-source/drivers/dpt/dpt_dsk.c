
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#ifdef __INTEL__
/* XXX - bjs this is needed for motherbd.h */
#define INTEL 1
#endif
#include <SupportDefs.h>
#include <OS.h>
#include <PCI.h>
#include "Drivers.h"
#include <KernelExport.h>

#include <Errors.h>
#include <Mime.h>
#include <vm.h>

#include "dpt.h"

static pci_module_info *pci;
static i2o_module_info *i2o;

dpt_card *first_card = NULL;	
dpt_disk *first_disk = NULL;

void new_disk(i2o_target *targ, dpt_card *card, size_t blocksize, size_t blocks)
{
	I2O_BSA_DEVICE_INFO_SCALAR di;
	I2O_BSA_CACHE_CONTROL_SCALAR cc;
	uint32 x[12];
	dpt_disk *disk = (dpt_disk *) malloc(sizeof(dpt_disk));
	
	if(disk){
		disk->target.tid = targ->tid;
		disk->target.ii = targ->ii;
		disk->targ = &disk->target;
		disk->blocks = blocks;
		disk->blocksize = blocksize;
		
		sprintf(disk->devname,"disk/raid/dpt/%d/%03x",card->num,targ->tid);
		
		dprintf("dpt%d: /dev/%s, %d blocks @ %d bytes ea\n",card->num,
				disk->devname,disk->blocks,disk->blocksize);
			
		if(i2o_util_params_get(disk->targ,I2O_BSA_DEVICE_INFO_GROUP_NO,&di,sizeof(di)) == B_OK){
			dprintf(">>> Type           0x%02x        Paths 0x%02x\n",di.DeviceType,di.NumberOfPaths);
			dprintf(">>> PowerState     0x%04x      BlockSize 0x%08x\n",di.PowerState,di.BlockSize);
			dprintf(">>> DeviceCapacity 0x%08x  Capabilities 0x%08x\n",(int)di.DeviceCapacity,di.DeviceCapabilitySupport);
			dprintf(">>> DeviceState    0x%08x\n",di.DeviceState);
		}
		if(i2o_util_params_get(disk->targ,I2O_BSA_CACHE_HISTORICAL_STATS_GROUP_NO,x,48) == B_OK){
			dprintf("** %08x %08x (access)\n",x[0],x[1]);
			dprintf("** %08x %08x (hit)\n",x[2],x[3]);
			dprintf("** %08x %08x (partial)\n",x[4],x[5]);
			dprintf("** %08x %08x (size)\n",x[6],x[7]);
		}
		
		if(i2o_util_params_get(disk->targ,I2O_BSA_CACHE_CONTROL_GROUP_NO,&cc,sizeof(cc)) == B_OK){
			dprintf(">>> Cache 0x%08x / 0x%08x / 0x%08x - T/R/W\n",
					cc.TotalCacheSize, cc.ReadCacheSize, cc.WriteCacheSize);
			dprintf(">>> Policy 0x%02x / 0x%02x / 0x%02x\n",
					cc.WritePolicy, cc.ReadPolicy, cc.ErrorCorrection);
		}
		
		// XXX locking
		disk->next = first_disk;
		first_disk = disk;
	}
}

typedef struct _I2O_BSA_MEDIA_INFO_SCALARx {
    long long   Capacity;
    U32         BlockSize;
} I2O_BSA_MEDIA_INFO_SCALARx;

static dpt_card*
create_cardinfo(int num, pci_info *pci)
{
	char name[32];
	I2O_BSA_MEDIA_INFO_SCALARx minfo;
	I2O_LCT_ENTRY entry;
	dpt_card *d = NULL;
	i2o_target targ;
	int i;
	
	if(!(d = (dpt_card *) malloc(sizeof(dpt_card)))) return NULL;
	
	d->num = num;
	sprintf(name, "dpt%d", d->num);

	if(!(d->ii = i2o_init_device(name,pci))) {
		dprintf("dpt%d: failed to setup i2o interface\n",d->num);
		free(d);
		return NULL;
	} else {
		dprintf("dpt%d: i2o interface resources allocated\n",d->num);
	}
		
	for(i=0;i2o_get_nth_lct(d->ii,i,&entry) == B_OK;i++){
		if((entry.ClassID.Class == I2O_CLASS_RANDOM_BLOCK_STORAGE) &&
		   (entry.UserTID == 0xfff)){
			   targ.tid = entry.LocalTID;
			   targ.ii = d->ii;
			   dprintf("dpt%d: claiming %04x (%d)\n",d->num,targ.tid,i2o_util_claim(&targ));
			   if(i2o_util_params_get(&targ,I2O_BSA_MEDIA_INFO_GROUP_NO,&minfo,12) == B_OK){
				   new_disk(&targ, d, minfo.BlockSize, (minfo.Capacity / minfo.BlockSize));
			   }
		}
	}
	return d;
}


int find_controllers(void)
{
    int i, iobase, iosize, irq;
    int cardcount = 0;
    pci_info h;
    dpt_card *d;
	
	dprintf("dpt: pci bus scan: %s, %s\n",__DATE__,__TIME__);
		
    for (i = 0; ; i++) {
        if ((*pci->get_nth_pci_info) (i, &h) != B_NO_ERROR) {
            break;
        }

        if ((h.vendor_id == PCI_DPT_VENDOR_ID) &&
            (h.device_id == PCI_DPT_DEVICE_ID)) {

            iobase = h.u.h0.base_registers[0];
            iosize = h.u.h0.base_register_sizes[0];
            irq = h.u.h0.interrupt_line;

            dprintf("dpt%d: controller @ 0x%08x (0x%08x), irq %d\n",
                     cardcount, iobase, iosize, irq);
               
            if(d = create_cardinfo(cardcount,&h)){
				d->next = first_card;
				first_card = d;
                cardcount++;
            } else {
                dprintf("dpt: cannot allocate cardinfo\n");
            }
        }
    }

    return cardcount;
}

extern uchar	icon_disk;
extern uchar	icon_disk_mini;
extern uchar	icon_removable;
extern uchar	icon_removable_mini;

static status_t
dpt_disk_open (const char *name, uint32 flags, void **cookie)
{
	dpt_disk *d = first_disk;
	while(d){
		if(!strcmp(d->devname,name)){
			*cookie = d;
//			dprintf("dpt: open\n");
			return B_OK;
		}
		d = d->next;
	}
	
	return B_ERROR;	
}

static status_t
dpt_disk_close (void *d)
{
//	dprintf("dpt: close\n");
	return B_OK;
}

static status_t
dpt_disk_free (void *d)
{
	return 0;
}


static status_t
dpt_disk_control (void *cookie, uint32 msg, void *s1, size_t len)
{
	dpt_disk *d = (dpt_disk *) cookie;
	device_geometry	*drive;

	switch (msg) {
	case B_FLUSH_DRIVE_CACHE:
		return B_OK;
		
	case B_GET_GEOMETRY:
		drive = (device_geometry *)s1;
		drive->bytes_per_sector = 0;
		drive->cylinder_count = 0;
		drive->bytes_per_sector = d->blocksize;
		drive->cylinder_count = d->blocks;

		/* construct a VERY simple drive geometry for now...  */
		drive->sectors_per_track = 1;
		drive->head_count = 1;
		drive->device_type = B_DISK;
		drive->removable = FALSE;
		drive->read_only = FALSE;
		drive->write_once = FALSE;
		return B_NO_ERROR;

	case B_GET_MEDIA_STATUS:{
		*((status_t*)s1) = B_OK;
		return B_NO_ERROR;
	}

	case B_FORMAT_DEVICE:
		return B_ERROR;

	case B_GET_DEVICE_SIZE:
		*(size_t*) s1 = d->blocks * d->blocksize;
		return B_NO_ERROR;

	case B_GET_ICON:
		switch (((device_icon *)s1)->icon_size) {
		case B_LARGE_ICON:
			memcpy(((device_icon *)s1)->icon_data, &icon_disk,
				   B_LARGE_ICON * B_LARGE_ICON);
			break;
		case B_MINI_ICON:
			memcpy(((device_icon *)s1)->icon_data, &icon_disk_mini,
				   B_MINI_ICON * B_MINI_ICON);
			break;
		default:
			return B_BAD_TYPE;
		}
		return B_NO_ERROR;
	
	default:
		return B_DEV_INVALID_IOCTL;
	}
}

static void
dpt_error(const iovec *vec, size_t count, const char *op)
{
	int i;
	dprintf("dpt(%s) failed\n",op);
	for(i=0;i<count;i++){
		dprintf("iov[%02d] %08x @%08x\n",i,vec[i].iov_len,vec[i].iov_base);
	}
}

/* ----------
	dpt_disk_read reads 'count' bytes from byte 'pos' on the disk
	described by 'd' into the buffer 'buf'.
----- */
static status_t
dpt_disk_read (void *cookie, off_t pos, void *buf, size_t *len)
{
	dpt_disk *d = (dpt_disk *) cookie;
	status_t	err;
	size_t		count;
	iovec vec;
	vec.iov_base = buf;
	vec.iov_len = *len;

	count = *len;
	if(lock_memory(buf, count, B_DMA_IO | B_READ_DEVICE)) return ENOMEM;
	
//	dprintf("read %08x %08x @ %08x\n",(int)pos,(int)*len,(int)buf);
	
	err = i2o_bsa_block_read(d->targ, pos, &vec, 1, len);

	if(err) dpt_error(&vec,1,"read");
	
	unlock_memory(buf, count, B_DMA_IO | B_READ_DEVICE);
	return err;
}

/* ----------
	dpt_disk_write writes 'count' bytes to byte 'pos' on the disk
	described by 'd' from the buffer 'buf'.
----- */
static status_t
dpt_disk_write (void *cookie, off_t pos, const void *buf, size_t *len)
{
	dpt_disk	*d = (dpt_disk *) cookie;
	status_t	err;
	size_t		count;
	iovec vec;
	vec.iov_base = (void *) buf;
	vec.iov_len = *len;
	
	count = *len;
	if(lock_memory((void *)buf, count, B_DMA_IO)) return ENOMEM;
	
	err = i2o_bsa_block_write(d->targ, pos, &vec, 1, len);
	if(err) dpt_error(&vec,1,"write");
	
	unlock_memory((void *)buf, count, B_DMA_IO);
	return err;
}

status_t dpt_disk_readv(void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *numBytes)
{
	dpt_disk	*d = (dpt_disk *) cookie;
	status_t	err;
	size_t		len;
	int			i,j;
	
	for(len=0,i=0;i<count;i++){
		len += vec[i].iov_len;
		if(lock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO | B_READ_DEVICE)){
			for(j=0;j<i;j++){
				unlock_memory(vec[j].iov_base,vec[j].iov_len, B_DMA_IO|B_READ_DEVICE);
			}
			return ENOMEM;
		}
	}
	
	*numBytes = len;
	
	err = i2o_bsa_block_read(d->targ, pos, (iovec*) vec, count, numBytes);
	if(err) dpt_error(vec,count,"readv");

	for(i=0;i<count;i++){
		unlock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO | B_READ_DEVICE);
	}
		
	return err;
}

status_t dpt_disk_writev(void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *numBytes)
{
	dpt_disk	*d = (dpt_disk *) cookie;
	status_t	err;
	size_t		len;
	int			i,j;
	
	for(len=0,i=0;i<count;i++){
		len += vec[i].iov_len;
		if(lock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO)){
			for(j=0;j<i;j++){
				unlock_memory(vec[j].iov_base,vec[j].iov_len, B_DMA_IO);
			}
			return ENOMEM;
		}
	}
	
	*numBytes = len;
	err = i2o_bsa_block_write(d->targ, pos, (iovec*) vec, count, numBytes);
	if(err) dpt_error(vec,count,"writev");

	for(i=0;i<count;i++){
		unlock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO);
	}
		
	return err;
}

static device_hooks dpt_disk_device = {
	dpt_disk_open,					/* -> open entry */
	dpt_disk_close,				/* -> close entry */
	dpt_disk_free,					/* -> free cookie */
	dpt_disk_control,				/* -> control entry */
	dpt_disk_read,					/* -> read entry */
	dpt_disk_write,				/* -> write entry */
	NULL,							/* -> select entry */
	NULL,							/* -> deselect entry */
	dpt_disk_readv,
	dpt_disk_writev
};


_EXPORT status_t
init_hardware(void)
{
	return B_OK;
}


extern int serial_dprintf_enabled;

_EXPORT status_t
init_driver()
{
	if(!serial_dprintf_enabled) return B_ERROR;
	load_driver_symbols("dpt");
	
	if(get_module(B_PCI_MODULE_NAME, (module_info **) &pci) != B_OK) {
		return B_ERROR;
	}
	
	if(get_module(B_I2O_MODULE_NAME, (module_info **) &i2o) != B_OK) {
		goto whoops;
	}
		
	if(find_controllers()) return B_OK;

	put_module(B_I2O_MODULE_NAME);
whoops:
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}

#include "devlistmgr.c"

_EXPORT void 
uninit_driver()
{
	dpt_disk *disk,*nextdisk;
	dpt_card *card,*nextcard;
	
	for(disk=first_disk;disk;disk=nextdisk){
		nextdisk = disk->next;
		free(disk);
	}
	for(card=first_card;card;card=nextcard){
		nextcard = card->next;
//		i2o_uninit_device(card);
		free(card);
	}
	put_module(B_I2O_MODULE_NAME);
	put_module(B_PCI_MODULE_NAME);
	free_devnames();
}

_EXPORT const char **
publish_devices()
{
	dpt_disk *disk;
	
	free_devnames();
	
	for(disk=first_disk;disk;disk=disk->next){
		add_devname(disk->devname);
	}
	
	return get_devnames();
}

_EXPORT device_hooks *
find_device(const char *name)
{
	return &dpt_disk_device;
}

_EXPORT int32
api_version = B_CUR_DRIVER_API_VERSION;
