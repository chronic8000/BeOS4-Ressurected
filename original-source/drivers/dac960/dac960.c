
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

#include "dac960.h"

#define DEBUG_BUILD 1

#define dxprintf(a)
//#define dxprintf(a) dprintf a

static pci_module_info		*pci;
static char	pci_name[] = B_PCI_MODULE_NAME;

#define MAXCARDS 8

static DAC960 *card[MAXCARDS] = { 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL 
};

	 
typedef struct 
{
	DAC960 *dac;
	uint32 number;
} disk;

static status_t
dac960_int_dispatch(void *data)
{
	DAC960 *dac = (DAC960 *) data;
	uint32 n,s;
	 
	if((*dac->outbound_doorbell) & OUT_STATUS_AVAIL){
		n = *dac->status_sequence;
		s = *dac->status;
		*dac->outbound_doorbell = OUT_STATUS_AVAIL;
		*dac->inbound_doorbell = IN_STATUS_ACK;
		if(n < 15){
			dac->cmds[n].status = s;
			release_sem_etc(dac->cmds[n].done, 1, B_DO_NOT_RESCHEDULE);
		} else {
			kprintf("XC? %d\n",n);
		}
		return B_HANDLED_INTERRUPT;
	} else {
		kprintf("UI\n");
		return B_UNHANDLED_INTERRUPT;
	}
}

status_t exec_command(DAC960 *dac, DACCMD *cmd)
{
//	dprintf("cmd 0x%02x exec\n",cmd->u.byte[0]);
	acquire_sem(dac->mailbox_lock);
	
	/* wait for card to become ready */
	while((*dac->inbound_doorbell) & IN_MAILBOX_FULL) spin(5);
	
	/* copy the command in */
	dac->mailbox[1] = cmd->u.word[1];
	dac->mailbox[2] = cmd->u.word[2];
	dac->mailbox[3] = cmd->u.word[3];
	dac->mailbox[0] = cmd->u.word[0];
	
	/* inform the controller */
	*dac->inbound_doorbell = IN_NEW_COMMAND;
	
	release_sem(dac->mailbox_lock);
//	dprintf("cmd 0x%02x wait\n",cmd->u.byte[0]);
	acquire_sem(cmd->done);
	if(cmd->status) dprintf("cmd 0x%02x status 0x%04x\n",cmd->u.byte[0],cmd->status);

	return cmd->status;
//	dprintf("cmd 0x%02x status 0x%04x\n",cmd->u.byte[0],cmd->status);
}

DACCMD *alloc_cmd(DAC960 *dac)
{
	cpu_status	ps;
	DACCMD *cmd;

	if(atomic_add(&dac->free_count, -1) < 1) acquire_sem(dac->free_sem);

	ps = disable_interrupts();
	acquire_spinlock(&dac->free_lock);
		
	cmd = dac->free_cmd;
	dac->free_cmd = cmd->next;
	cmd->u.word[0] = 0;
	cmd->u.word[1] = 0;
	cmd->u.word[2] = 0;
	cmd->u.word[3] = 0;
	cmd->u.byte[1] = cmd->id;
	
	release_spinlock(&dac->free_lock);
	restore_interrupts (ps);		
	return cmd;
}

void free_cmd(DAC960 *dac, DACCMD *cmd)
{
	cpu_status	ps;

	ps = disable_interrupts();
	acquire_spinlock(&dac->free_lock);
		
	cmd->next = dac->free_cmd;
	dac->free_cmd = cmd;
	
	release_spinlock(&dac->free_lock);
	restore_interrupts (ps);		

	if(atomic_add(&dac->free_count,1) < 0) release_sem(dac->free_sem);	
}

status_t get_drive_info(DAC960 *dac, int number, drive_info *di)
{
	int j,k;
	DACCMD *cmd = alloc_cmd(dac);
	sys_drv *sd = (sys_drv *) cmd->sglist;
	sys_drv *info = &di->sys_drv;
	device_state *ds = (device_state *) cmd->sglist;

	memset(di, 0, sizeof(drive_info));
	di->blocks = dac->system_drives[number];
	
	cmd->u.type3.phys_ptr = cmd->sglist_phys;
	cmd->u.type3.opcode = 0x51;
	cmd->u.type3.drive = number;
	if(exec_command(dac,cmd)) {
		free_cmd(dac,cmd);
		return B_ERROR;
	}
	
	memcpy(info, sd, sizeof(sys_drv));
	
	cmd->u.getstate.opcode = 0x50;
	cmd->u.getstate.phys_ptr = cmd->sglist_phys;
	
	for(j=0;j<info->valid_spans;j++){
		for(k=0;k<info->valid_arms;k++){
			cmd->u.getstate.channel = info->span[j].arm[k].channel;
			cmd->u.getstate.target = info->span[j].arm[k].target;
			exec_command(dac,cmd);
			memcpy(&(di->state[j][k]), ds, sizeof(device_state));
		}
	}
	free_cmd(dac,cmd);
	return B_OK;			
}


void get_info(DAC960 *dac)
{
	DACCMD *cmd = alloc_cmd(dac);
	enquiry_data *ed = (enquiry_data *) cmd->sglist;
	enquiry2_data *ed2 = (enquiry2_data *) cmd->sglist;
		
	cmd->u.type3.opcode = 0x53;
	cmd->u.type3.phys_ptr = cmd->sglist_phys;
	exec_command(dac, cmd);

#if 0	
	dprintf("dac960/%d: Firmware Version %x.%02x\n",dac->num,
			ed->firmware_major_version,ed->firmware_minor_version);
	dprintf("dac960/%d: %d system drive%s\n",dac->num,ed->system_drives,
			ed->system_drives == 1 ? "" : "s");
	dprintf("dac960/%d: %d offline, %d critical, %d dead, %d rebuilding\n",
			dac->num,ed->offline_sd_count,ed->critical_sd_count,
			ed->dead_count, ed->rebuild_count);
#endif
	dprintf("dac960/%d: V%x.%02x, %d drive%s (%dX/%dC/%dD/%dR)\n",
			dac->num, 	ed->firmware_major_version,ed->firmware_minor_version,
			ed->system_drives,	ed->system_drives == 1 ? "" : "s",
			ed->offline_sd_count,ed->critical_sd_count,
			ed->dead_count, ed->rebuild_count);
		
	memcpy(dac->system_drives, ed->size, 32 * 4);	

	cmd->u.type3.opcode = 0x1c;
	exec_command(dac, cmd);
	
	dprintf(" Firmware ID: %d.%02d-%c-%d\n",
			ed2->firmware_id[0],ed2->firmware_id[1],
			ed2->firmware_id[3],ed2->firmware_id[2]);
	dprintf(" Clock Speed: %dns\n",ed2->clock_speed);
	dprintf(" Max Commands: %d\n",ed2->max_commands);
	dprintf(" Max SG Entries: %d\n",ed2->max_sg_entries);
	dprintf(" Latency: %ds\n",ed2->latency);
	dprintf(" Cache Line Size: %d\n",ed2->cache_line_size * 512);
	
	
	free_cmd(dac,cmd);
}

void destroy_cardinfo(DAC960 *dac)
{
	int i;

	if(dac->irq){
		*dac->interrupt_mask = INT_DISABLE;
		remove_io_interrupt_handler(dac->irq, dac960_int_dispatch, dac);
	}
	
	if(dac->shared_area >= 0) delete_area(dac->shared_area);
	if(dac->workspace_area >= 0) delete_area(dac->workspace_area);
	if(dac->mailbox_lock >= 0) delete_sem(dac->mailbox_lock);	
	if(dac->free_sem >= 0) delete_sem(dac->free_sem);
	
	for(i=0;i<15;i++) {
		if(dac->cmds[i].done >= 0) delete_sem(dac->cmds[i].done);
	}
	free(dac);
}

DAC960 *create_cardinfo(int num, pci_info *pci)
{
	DAC960 *dac;
	char name[32];
	uint32 regs, workspace, workspace_phys, i;
	physical_entry pe[2];
	
	if(!(dac = (DAC960 *) malloc(sizeof(DAC960)))) return NULL;
	
	dac->shared_area = -1;
	dac->workspace_area = -1;
	dac->mailbox_lock = -1;
	dac->free_sem = -1;
	for(i=0;i<15;i++) dac->cmds[i].done = -1;
	
	dac->num = num;
	dac->irq = 0;
	
	sprintf(name,"dac960:%d:regs",num);
	dac->shared_area = 
	map_physical_memory(name, (void *) pci->u.h0.base_registers[0],
						pci->u.h0.base_register_sizes[0],
						B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
						(void **) &regs);
	if(dac->shared_area < 0) goto err;

	sprintf(name,"dac960:%d:lock",num);
	if((dac->mailbox_lock = create_sem(1,name)) < 0) goto err;
		
	dac->inbound_doorbell = (uchar *) (regs + 0x20);
	dac->outbound_doorbell = (uchar *) (regs + 0x2c);
	dac->interrupt_mask = (uint32 *) (regs + 0x34);
	dac->mailbox = (uint32 *) (regs + 0x1000);
	dac->status_sequence = (uchar *) (regs + 0x1018);
	dac->status = (uint32 *) (regs + 0x101A);
	
	sprintf(name,"dac960:%d:workspace",num);
	dac->workspace_area = 
	create_area(name, (void **) &workspace, B_ANY_KERNEL_ADDRESS, 
				4096, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if(dac->workspace_area < 0) goto err;
	get_memory_map((void*)workspace, 4096, pe, 2);
	workspace_phys = (uint32) pe[0].address;
	
//	dprintf("dac960/%d: workspace @ 0x%08x (0x%08x)\n",dac->num,
//			workspace, workspace_phys);
	
	dac->free_cmd = NULL;
	
	for(i=0;i<15;i++){
		dac->cmds[i].id = i;
		dac->cmds[i].sglist = (SGLIST *) (workspace + sizeof(SGLIST) * i);
		dac->cmds[i].sglist_phys = (workspace_phys + sizeof(SGLIST) * i);
		sprintf(name,"dac960:%d:done_%02d",num,i);
		dac->cmds[i].done = create_sem(0,name);
		
		dac->cmds[i].next = dac->free_cmd;
		dac->free_cmd = &dac->cmds[i];	
	}
	
	sprintf(name,"dac960:%d:free_sem",num);
	dac->free_sem = create_sem(0,name);
	dac->free_count = 15;
	dac->free_lock = 0;
	
//	dprintf("dac960: regs @ 0x%08x\n",regs);
	
	dac->irq = pci->u.h0.interrupt_line;	
	install_io_interrupt_handler(dac->irq, dac960_int_dispatch, dac, 0);
	*dac->interrupt_mask = INT_ENABLE;
	
	sprintf(dac->name,"disk/dac960/%d",dac->num);
	dac->namelen = strlen(dac->name);
	
	get_info(dac);
	
	return dac;

err:
	destroy_cardinfo(dac);
	
	return NULL;
}


int find_controllers(void)
{
    int i, iobase, iosize, irq;
    int cardcount = 0;
    pci_info h;
    DAC960 *d;
	
//	dprintf("dac960: pci bus scan: %s, %s\n",__DATE__,__TIME__);
		
    for (i = 0; ; i++) {
        if ((*pci->get_nth_pci_info) (i, &h) != B_NO_ERROR) {
            break;
        }

        if ((h.vendor_id == PCI_DAC960_VENDOR_ID) &&
            (h.device_id == PCI_DAC960_DEVICE_ID)) {

            iobase = h.u.h0.base_registers[0];
            iosize = h.u.h0.base_register_sizes[0];
            irq = h.u.h0.interrupt_line;

            dprintf("dac960/%d: controller @ 0x%08x (0x%08x), irq %d\n",
                     cardcount, iobase, iosize, irq);
               
            if(d = create_cardinfo(cardcount,&h)){
				card[cardcount] = d;
                cardcount++;
            } else {
                dprintf("dac960: cannot allocate cardinfo\n");
            }
        }
		if(cardcount == MAXCARDS) break;
    }

    return cardcount;
}

extern uchar	icon_disk;
extern uchar	icon_disk_mini;

static status_t
disk_open(const char *name, uint32 flags, void **cookie)
{
	int i,n;
	disk *d;
	
//	dprintf("dac960: open(\"%s\",...)\n",name);
	
	for(i=0;i<MAXCARDS;i++){
		if(card[i] && !strncmp(card[i]->name,name,card[i]->namelen)){
			n = atoi(name + card[i]->namelen + 1);
			if(card[i]->system_drives[n]){
				d = (disk *) malloc(sizeof(disk));
				if(!d) return B_ERROR;
				d->dac = card[i];
				d->number = n;
				*cookie = (void **) d;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

static status_t
disk_close(void *cookie)
{
	disk *d = (disk *) cookie;
//	dprintf("dac960: close(\"%s/%d\",...)\n",d->dac->name,d->number);
	
	return B_OK;
}

static status_t
disk_free(void *cookie)
{
	free(cookie);
	return B_OK;
}


static status_t
disk_control (void *cookie, uint32 msg, void *s1, size_t len)
{
	disk *d = (disk *) cookie;
	device_geometry	*drive;
	
	switch (msg) {
	case 5000:
		return get_drive_info(d->dac, d->number, (drive_info *) s1);
		
	case 5001: {
		DACCMD *cmd = alloc_cmd(d->dac);
		cmd->u.getstate.opcode = 0x0c;
		cmd->u.getstate.phys_ptr = cmd->sglist_phys;
		
		if(exec_command(d->dac,cmd)){
			free_cmd(d->dac,cmd);
			return B_ERROR;
		} else {
			memcpy(s1, cmd->sglist, 4 * 3);
			free_cmd(d->dac,cmd);
			return B_OK;
		}
	}
	

	case 5002: {
		DACCMD *cmd = alloc_cmd(d->dac);
		ioctl_rebuild *r = (ioctl_rebuild *) s1;
		cmd->u.getstate.channel = r->channel;
		cmd->u.getstate.target = r->target;
		cmd->u.getstate.phys_ptr = cmd->sglist_phys;
		cmd->u.getstate.opcode = 0x16;
		
		if(exec_command(d->dac,cmd)){
			free_cmd(d->dac,cmd);
			return B_ERROR;
		} else {
			free_cmd(d->dac,cmd);
			return B_OK;
		}		
	}
	
	case B_FLUSH_DRIVE_CACHE:{
		DACCMD *cmd = alloc_cmd(d->dac);
		cmd->u.type3.opcode = 0x0a;
		exec_command(d->dac, cmd);
		free_cmd(d->dac,cmd);
		return B_OK;
	}
		
	case B_GET_GEOMETRY:
		drive = (device_geometry *)s1;
		drive->bytes_per_sector = 512;
		drive->cylinder_count = d->dac->system_drives[d->number];

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
		*(size_t*) s1 = d->dac->system_drives[d->number] * 512;
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
		dprintf("dac960/%d: ioctl(%d) unsupported\n",d->dac->num, msg);
		return B_DEV_INVALID_IOCTL;
	}
}

#define BLOCKMASK (511)

static size_t 
do_remap(physical_entry *pe, int sgmax, int *sgcount,
		 const iovec *iov, size_t count, size_t offset)
{
	physical_entry *pe0 = pe;
	size_t actual = 0;

	/* if we are starting partway into the first vec... */	
	get_memory_map((void*) (((uint32)iov->iov_base) + offset), iov->iov_len - offset, pe, sgmax);
	goto first_pass;
	
	/* map as much as we can and see how big it all is */
	while(sgmax && count){
		get_memory_map(iov->iov_base, iov->iov_len, pe, sgmax);
first_pass:
		iov++;
		count--;
		while(sgmax && pe->size){
			actual += pe->size;
			pe++;
			sgmax--;
		}
	}
	
	/* if this is not a block multiple, shave off the extra bits */
	if(actual & BLOCKMASK){
		int extra = actual & BLOCKMASK;
		actual -= extra;
		while(extra){
			pe--;
			if(pe->size <= extra){
				extra -= pe->size;
			} else {
				pe->size -= extra;
				pe++;
				break;
			}
		}
	}	
	
	*sgcount = pe - pe0;
	return actual;
}
	   
status_t disk_readv(void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *numBytes)
{
	status_t res;
	disk *d = (disk *) cookie;
	physical_entry *pe;
	size_t len, xfer, actual, offset;
	DACCMD *cmd;	
	int i,j,c;
	
	for(len=0,i=0;i<count;i++){
		len += vec[i].iov_len;
		if(lock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO)){
			for(j=0;j<i;j++){
				unlock_memory(vec[j].iov_base,vec[j].iov_len, B_DMA_IO);
			}
			return ENOMEM;
		}
	}
	
	if(len == 0){
		res = B_OK;
		goto error;
	}
	
	if((len & BLOCKMASK) || (pos & BLOCKMASK)) {
		res = B_ERROR;
		goto error;
	}
	
	*numBytes = len;
	cmd = alloc_cmd(d->dac);

	cmd->u.io.opcode = 0xb3;
	cmd->u.io.phys_ptr = cmd->sglist_phys;
	cmd->u.io.system_drive = d->number;
	pe = cmd->sglist->sglist;
	
	actual = 0;
	offset = 0;
	c = count;
	dxprintf(("RD: %08x @%08x (count %d)\n",((int)len),((int)pos),((int)count)));
	for(;;){
		xfer = do_remap(pe, 33, &i, vec, c, offset);
		cmd->u.io.block = pos / 512; // XXX shift op		
		cmd->sglist->blocks = xfer / 512;
		cmd->u.io.sgcount = i;
		
		dxprintf(("  : %08x @%08x (sg %3d)\n",((int)xfer),((int)pos),i));
		exec_command(d->dac, cmd);
		
		if((actual + xfer) == len) break;
		
		pos += xfer;
		actual += xfer;
		
		xfer += offset; /* bytes sent last time 'round */
		while(xfer){
			if(vec->iov_len <= xfer){
				xfer -= vec->iov_len;
				vec++;
				c--;
			} else {
				break;
			}
		}
		offset = xfer;
	}	
	
	res = B_OK;
	free_cmd(d->dac,cmd);
	
error:
	for(i=0;i<count;i++){
		unlock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO | B_READ_DEVICE);
	}
		
	return res;
}

status_t disk_writev(void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *numBytes)
{
	status_t res;
	disk *d = (disk *) cookie;
	physical_entry *pe;
	size_t len, xfer, actual, offset;
	DACCMD *cmd;	
	int i,j,c;
	
	for(len=0,i=0;i<count;i++){
		len += vec[i].iov_len;
		if(lock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO)){
			for(j=0;j<i;j++){
				unlock_memory(vec[j].iov_base,vec[j].iov_len, B_DMA_IO);
			}
			return ENOMEM;
		}
	}
	
	if(len == 0){
		res = B_OK;
		goto error;
	}
	
	if((len & BLOCKMASK) || (pos & BLOCKMASK)) {
		res = B_ERROR;
		goto error;
	}
	
	*numBytes = len;	
	cmd = alloc_cmd(d->dac);

	cmd->u.io.opcode = 0xb4;
	cmd->u.io.phys_ptr = cmd->sglist_phys;
	cmd->u.io.system_drive = d->number;
	pe = cmd->sglist->sglist;
	
	actual = 0;
	offset = 0;
	c = count;
	dxprintf(("WR: %08x @%08x (count %d)\n",((int)len),((int)pos),((int)count)));
	for(;;){
		xfer = do_remap(pe, 33, &i, vec, c, offset);
		cmd->u.io.block = pos / 512; // XXX shift op		
		cmd->sglist->blocks = xfer / 512;
		cmd->u.io.sgcount = i;
		
		dxprintf(("  : %08x @%08x (sg %3d)\n",((int)xfer),((int)pos),i));
		exec_command(d->dac, cmd);
		
		if((actual + xfer) == len) break;
		
		pos += xfer;
		actual += xfer;
		
		xfer += offset; /* bytes sent last time 'round */
		while(xfer){
			if(vec->iov_len <= xfer){
				xfer -= vec->iov_len;
				vec++;
				c--;
			} else {
				break;
			}
		}
		offset = xfer;
	}
			
	res = B_OK;
	free_cmd(d->dac,cmd);
	
error:
	for(i=0;i<count;i++){
		unlock_memory(vec[i].iov_base,vec[i].iov_len, B_DMA_IO | B_READ_DEVICE);
	}
		
	return res;
}

static status_t
disk_write(void *cookie, off_t pos, const void *buf, size_t *len)
{
	iovec vec;
	vec.iov_len = *len;
	vec.iov_base = (void*) buf;
	return disk_writev(cookie,pos,&vec,1,len);
}

static status_t
disk_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	iovec vec;
	vec.iov_len = *len;
	vec.iov_base = buf;
	return disk_readv(cookie,pos,&vec,1,len);
}

static device_hooks disk_device = {
	disk_open,			/* -> open entry */
	disk_close,			/* -> close entry */
	disk_free,			/* -> free cookie */
	disk_control,		/* -> control entry */
	disk_read,			/* -> read entry */
	disk_write,			/* -> write entry */
	NULL,				/* -> select entry */
	NULL,				/* -> deselect entry */
//	NULL,
//	NULL
	disk_readv,
	disk_writev
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
#if DEBUG_BUILD	
	if(!serial_dprintf_enabled) return B_ERROR;
	load_driver_symbols("dac960");
#endif
			
	if(get_module(pci_name, (module_info **) &pci) != B_OK) return B_ERROR;
	
	if(find_controllers()){
		return B_OK;
	} else {
		put_module(pci_name);
		return B_ERROR;
	}
}

#include "devlistmgr.c"

_EXPORT void 
uninit_driver()
{
	int i;
	for(i=0;i<MAXCARDS;i++) if(card[i]) destroy_cardinfo(card[i]);
	put_module(pci_name);
	free_devnames();
}

_EXPORT const char **
publish_devices()
{
	int i,num;
	char name[128];
	free_devnames();
	
	for(i=0;i<MAXCARDS;i++){
		if(card[i]){
			for(num=0;num<32;num++){
				if(card[i]->system_drives[num]){
					sprintf(name,"disk/dac960/%d/%d/raw",card[i]->num,num);
					add_devname(name);
				}
			}
		}
	}
	return get_devnames();
}

_EXPORT device_hooks *
find_device(const char *name)
{
	return &disk_device;
}

_EXPORT int32
api_version = B_CUR_DRIVER_API_VERSION;
