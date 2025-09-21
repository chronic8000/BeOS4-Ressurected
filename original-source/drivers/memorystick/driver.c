#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <scsi.h>
#include <malloc.h>
#include <ByteOrder.h>
#include "memory_stick.h"
#include "memory_stick_regs.h"
#include "memory_stick_commands.h"
#include "tpc.h"

sem_id              global_ms_lock;
memory_stick_info  *global_ms_info;
bigtime_t           global_ms_info_time;

//typedef struct {
//	sem_id mutex;
//	memory_stick_info *current_card;
//} ms_drive_info;

typedef struct {
//	memory_stick_info  *ms_info;
	bool                interruptable;
	bigtime_t           last_media_change_time;
} ms_handle;

typedef struct {
	const iovec  *vec;
	size_t        veccount;
	size_t        start_offset;
} iovec_desc;

static status_t write_lblock(memory_stick_info *info, uint32 lblock,
                             uint8 start_page, iovec_desc *data);

static void
free_ms_info(memory_stick_info *info)
{
	if(info == NULL)
		return;
	uninit_block_translation(info);
	free(info);
}

status_t acquire_ms(ms_handle *h, bool ignore_media_status)
{
	status_t err;
	err = acquire_sem_etc(global_ms_lock, 1,
	                      h->interruptable ? B_CAN_INTERRUPT : 0, 0);
	if(err != B_NO_ERROR)
		return err;
	if(ignore_media_status)
		return B_NO_ERROR;
	err = tpc_get_media_status(h->last_media_change_time);
	if(err != B_NO_ERROR) {
		goto err;
	}
	return B_NO_ERROR;
	
err:
	release_sem(global_ms_lock);
	return err;
}

status_t release_ms(ms_handle *h)
{
	return release_sem(global_ms_lock);
}

/* driver api */

status_t
init_driver()
{
	const char *debugprefix = "parallelms -- init_driver: ";
	status_t err;

	dprintf("%s\n", debugprefix);
	global_ms_info = NULL;
	err = global_ms_lock = create_sem(1, "memory stick mutex");
	if(err < B_NO_ERROR)
		goto err0;
	err = tpc_init();
	if(err != B_NO_ERROR)
		goto err1;
	dprintf("%sdone\n", debugprefix);
	return B_NO_ERROR;

err1:
	delete_sem(global_ms_lock);
err0:
	return err;
}

bigtime_t total_read_time = 0;
off_t total_read_bytes = 0;
bigtime_t total_write_time = 0;
off_t total_write_bytes = 0;
int total_io_count = 0;

void
uninit_driver()
{
	const char *debugprefix = "parallelms -- uninit_driver: ";
	if(total_read_time != 0)
		dprintf("%savg read time: %Ld bytes/sec\n", debugprefix,
		        total_read_bytes * 1000 * 1000 / total_read_time);
	if(total_write_time != 0)
		dprintf("%savg write time: %Ld bytes/sec\n", debugprefix,
		        total_write_bytes * 1000 * 1000 / total_write_time);
	tpc_uninit();
	free_ms_info(global_ms_info);
	delete_sem(global_ms_lock);
	dprintf("%sdone\n", debugprefix);
}

static status_t
read_boot_block_data(void *bootblock1data, int *bootblock1location,
                     bool *bootblock1corrected,
                     void *bootblock2data, int *bootblock2location,
                     bool *bootblock2corrected)
{
	status_t           err = B_ERROR;
	int                boot_block = 0;
	int                boot_block_count = 0;
	void              *data;
	memory_stick_regs  regs;

	data = bootblock1data;
	*bootblock1location = -1;
	*bootblock2location = -1;
	*bootblock1corrected = false;
	*bootblock2corrected = false;
	while(boot_block_count < 2 && boot_block < 12) {
		err = read_sector(data, boot_block, 0);
		if(err != B_NO_ERROR)
			goto err;
	
		err = tpc_read(TPC_READ_REG, regs, sizeof(regs));
		if(err != B_NO_ERROR)
			goto err;
		//dprintf("read_boot_block: block %d: owf 0x%02x mf 0x%02x s1 0x%02x\n",
		//        boot_block, regs[REG_OWFLAG], regs[REG_MFLAG], regs[REG_STATUS1]);
		if(regs[REG_OWFLAG] != 0xc0)
			goto err;
		if((regs[REG_MFLAG] & REG_MFLAG_MASK_SYSFLG) != 0)
			goto err;
		if(regs[REG_STATUS1] != 0) {
			if(boot_block_count == 0)
				*bootblock1corrected = true;
			else
				*bootblock2corrected = true;
		}
		err = read_sector(data + 512, boot_block, 1);
		if(err != B_NO_ERROR)
			goto err;
		err = tpc_read(TPC_READ_REG, regs, sizeof(regs));
		if(err != B_NO_ERROR)
			goto err;
		if(regs[REG_STATUS1] != 0) {
			if(boot_block_count == 0)
				*bootblock1corrected = true;
			else
				*bootblock2corrected = true;
		}
		boot_block_count++;
		if(boot_block_count == 1) {
			*bootblock1location = boot_block;
			data = bootblock2data;
		}
		else {
			*bootblock2location = boot_block;
		}
err:
		boot_block++;
	}
	if(boot_block_count > 0)
		return B_NO_ERROR;
	if(err == B_NO_ERROR)
		err = B_ERROR;
	return err;
}

status_t
read_boot_block(memory_stick_info *info)
{
	const char *debugprefix = "parallelms -- read_boot_block: ";
	status_t    err;
	uint8      *block_data_buffer;
	uint8      *block_data;
	bool        block1_corrected;
	bool        block2_corrected;
	int         i;
	int         block1_location;
	int         block2_location;
	
	block_data = block_data_buffer = malloc(512*2*2);
	if(block_data_buffer == NULL) {
		err = B_NO_MEMORY;
		goto err0;
	}
	
	block_data[0] = 0xff;
	block_data[1024] = 0xff;
	err = read_boot_block_data(block_data, &block1_location, &block1_corrected,
	                           block_data+1024, &block2_location,
	                           &block2_corrected);
	if(err != B_NO_ERROR)
		goto err1;

	if(block1_corrected && !block2_corrected && block2_location != -1) {
		dprintf("%susing second bootblock\n", debugprefix);
		block_data += 512*2;
	}

	/* boot block header */
	if(block_data[0] != 0 || block_data[1] != 1) {
		dprintf("%sbad boot block sig: 0x%02x%02x\n",
		        debugprefix, block_data[0], block_data[1]);
		if(block_data == block_data_buffer)
			block_data += 512*2;
		else
			block_data = block_data_buffer;
		if(block_data[0] != 0 || block_data[1] != 1) {
			dprintf("%sbad boot block sig: 0x%02x%02x\n",
			        debugprefix, block_data[0], block_data[1]);
			err = B_DEV_FORMAT_ERROR;
			goto err1;
		}
	}
	//dprintf("boot block entries: 0x%02x\n", block_data[2+2+184]);

	/* boot block system entry */
	if(0) {
		struct {
			uint32 start_address;
			uint32 data_size;
			uint8  data_type_id;
			uint8  reserved[3];
		} *system_entry = (void*)&block_data[368];
		dprintf("%ssystem_entry 0\n", debugprefix);
		dprintf("%sstart address: 0x%08lx\n", debugprefix,
		        B_BENDIAN_TO_HOST_INT32(system_entry->start_address));
		dprintf("%ssize: 0x%08lx\n", debugprefix,
		        B_BENDIAN_TO_HOST_INT32(system_entry->data_size));
		dprintf("%stype: 0x%02x\n", debugprefix, system_entry->data_type_id);
		system_entry++;
		dprintf("%ssystem_entry 1\n", debugprefix);
		dprintf("%sstart address: 0x%08lx\n", debugprefix,
		        B_BENDIAN_TO_HOST_INT32(system_entry->start_address));
		dprintf("%ssize: 0x%08lx\n", debugprefix,
		        B_BENDIAN_TO_HOST_INT32(system_entry->data_size));
		dprintf("%stype: 0x%02x\n", debugprefix, system_entry->data_type_id);
	}
	/* boot block boot and attribute information */
	{
		struct {
			uint8  ms_class;
			uint8  ms_type;
			uint16 block_size; /* in KB */
			uint16 num_blocks;
			uint16 num_eblocks;
			uint16 page_size;
			uint8  edata_size;
			uint8  reserved_1_val_1;
			uint8  assembly_date[8];
			uint8  reserved_2_val_0;
			uint8  manufacturer_area[3];
			uint8  assembler_code;
			uint8  assembler_model_code[3];
			uint16 memory_maker_code;
			uint16 memory_device_code;
			uint16 memory_size; /* in MB */
			uint8  reserved_3_val_1;
			uint8  reserved_4_val_1;
			uint8  vcc_voltage; /* in 0.1V */
			uint8  vpp_voltage; /* in 0.1V */
			uint16 controller_number;
			uint8  reserved_5[14];
			uint8  format_type; /* 1: FAT */
			uint8  purpose;
			uint8  reserved_6[23];
			uint8  reserved_7_val_10;
			uint8  reserved_8_val_1;
			uint8  reserved_9[15];
		} *bi = (void*)&block_data[368+48];
		info->block_size = B_BENDIAN_TO_HOST_INT16(bi->block_size);
		info->num_pblocks = B_BENDIAN_TO_HOST_INT16(bi->num_blocks);
		info->num_lblocks = B_BENDIAN_TO_HOST_INT16(bi->num_eblocks) - 2;

#if 0
		dprintf("ms class: 0x%02x\n", bi->ms_class);
		dprintf("ms type: 0x%02x\n", bi->ms_type);
		dprintf("block size: %d KB\n", B_BENDIAN_TO_HOST_INT16(bi->block_size));
		dprintf("num blocks: %d\n", B_BENDIAN_TO_HOST_INT16(bi->num_blocks));
		dprintf("num effective blocks: %d\n", B_BENDIAN_TO_HOST_INT16(bi->num_eblocks));
#endif
#if 0
		dprintf("page size %d\n", B_BENDIAN_TO_HOST_INT16(bi->page_size));
		dprintf("edata size %d\n", bi->edata_size);
		dprintf("Assembly date: td 0x%02x\n", bi->assembly_date[0]);
		dprintf("Assembly date: %d/%d/%d %d:%d:%d\n",
		        bi->assembly_date[3], bi->assembly_date[4],
		        bi->assembly_date[1] << 8 | bi->assembly_date[2],
		        bi->assembly_date[5], bi->assembly_date[6], bi->assembly_date[7]
		        );
		dprintf("memory size: %d MB\n", B_BENDIAN_TO_HOST_INT16(bi->memory_size));
		dprintf("voltage: vcc %d.%d vpp %d.%d\n",
		        bi->vcc_voltage / 10, bi->vcc_voltage % 10, 
		        bi->vpp_voltage / 10, bi->vpp_voltage % 10);
#endif
	}
	info->unused_block_count = 2;
	for(i = 0; i < 512; i += 2) {
		if(block_data[512+i] == 0xff && block_data[512+i+1] == 0xff)
			break;
		info->unused_block_count++;
	}

	info->unused_blocks = malloc(info->unused_block_count *
	                             sizeof(info->unused_blocks[0]));
	if(info->unused_blocks == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}
	info->unused_blocks[0] = block1_location; /* boot blocks */
	info->unused_blocks[1] = block2_location;
	for(i = 0; i < info->unused_block_count - 2; i++) {
		info->unused_blocks[i+2] = block_data[512+i*2] << 8 |
		                           block_data[512+i*2+1];
	}

#if 0
	dprintf("disabled blocks:\n");
	dprintf("information block no 0x%02x%02x\n",
	        block_data[512+0], block_data[512+1]);
	{
		int i;
		for(i = 2; i < 512; i += 2) {
			if(block_data[512+i] == 0xff && block_data[512+i+1] == 0xff)
				break;
			dprintf("defect block no 0x%02x%02x\n",
			        block_data[512+i], block_data[512+i+1]);
		}
	}
#endif

	free(block_data_buffer);
	return B_NO_ERROR;

//err2:
//	free(info->unused_blocks);
err1:
	free(block_data_buffer);
err0:
	return err;
}

status_t
init_media()
{
	status_t err;
	memory_stick_info *ms_info = NULL;
	memory_stick_regs regs;
	
	err = reset_memory_stick();
	if(err != B_NO_ERROR)
		goto err0;


	err = B_NO_MEMORY;
	ms_info = malloc(sizeof(*ms_info));
	if(ms_info == NULL) {
		goto err0;
	}

	err = tpc_read(TPC_READ_REG, regs, sizeof(regs));
	if(err != B_NO_ERROR)
		goto err1;

	ms_info->read_only = (regs[REG_STATUS0] & REG_STATUS0_MASK_WP) != 0;
	err = read_boot_block(ms_info);
	if(err != B_NO_ERROR)
		goto err1;

	err = init_block_translation(ms_info);
	if(err != B_NO_ERROR)
		goto err2;
		

	global_ms_info = ms_info;
	return B_NO_ERROR;

err2:
	free(ms_info->unused_blocks);
err1:
	free(ms_info);
err0:
	return err;
}

status_t
get_ms_info(ms_handle *h, memory_stick_info **info)
{
	status_t err;
	
	// Find Memory Stick
	err = tpc_get_media_status(global_ms_info_time);
	if(err != B_NO_ERROR) {
		free_ms_info(global_ms_info);
		global_ms_info = NULL;
	}

	err = tpc_get_media_status(h->last_media_change_time);
	if(err != B_NO_ERROR) {
		return err;
	}
	if(global_ms_info != NULL) {
		*info = global_ms_info;
		return B_NO_ERROR;
	}
	err = init_media();
	if(err != B_NO_ERROR)
		return err;
	global_ms_info_time = h->last_media_change_time;
	*info = global_ms_info;
	return B_NO_ERROR;
}

status_t
read_lsector(ms_handle *h, off_t pos, void *data)
{
	const char *debugprefix = "parallelms -- read_lsector: ";
	status_t err;
	uint8 page;
	uint32 lblock;
	uint32 pblock;
	memory_stick_info *ms_info;
	
	err = get_ms_info(h, &ms_info);
	if(err != B_NO_ERROR)
		return err;

	page = (pos / 512) % (ms_info->block_size * 2);
	lblock = pos / 512 / (ms_info->block_size * 2);

	if(pos < 0 || lblock >= ms_info->num_lblocks)
		return B_DEV_SEEK_ERROR;

	err = translate_block(ms_info, lblock, &pblock);
	if(err != B_NO_ERROR)
		return err;

	//dprintf("select lblock %ld pblock %ld, page %d\n", lblock, pblock, page);
	err = read_data_sector(data, pblock, page);
	if(err != MS_DATA_GOOD) {
		if(err < B_NO_ERROR)
			return err;

		dprintf("%scorrected error in pblock %d, page %d\n",
		        debugprefix, (int)pblock, page);
		if(!ms_info->read_only) {
			iovec_desc dummy_data;
			dummy_data.vec = NULL;
			dummy_data.veccount = 0;
			dummy_data.start_offset = 0;

			err = write_lblock(ms_info, lblock, 0, &dummy_data);
			if(err < B_NO_ERROR) {
				dprintf("%sfailed to move pblock %d\n", debugprefix, (int)pblock);
			}
			else {
				dprintf("%spblock %d moved\n", debugprefix, (int)pblock);
			}
		}
	}

	if(1) {	/* extra error check, remove? */
		memory_stick_regs regs;
		uint16 edata_lblock;

		err = tpc_read(TPC_READ_REG, regs, sizeof(regs));
		if(err != B_NO_ERROR)
			return err;
		
		edata_lblock = regs[REG_LADDR1] << 8 | regs[REG_LADDR0];
		if(edata_lblock != lblock && edata_lblock != 0xffff) {
			dprintf("%sexpected lblock %ld, got %d at pblock %ld page %d\n",
			        debugprefix, lblock, edata_lblock, pblock, page);
			dprintf("%ssr0 0x%02x, sr1 0x%02x, owf 0x%02x, mf 0x%02x, "
			        "la 0x%02x%02x\n",
			        debugprefix, regs[REG_STATUS0], regs[REG_STATUS1],
			        regs[REG_OWFLAG], regs[REG_MFLAG],
			        regs[REG_LADDR1], regs[REG_LADDR0]);
			//return B_IO_ERROR;
		}
	}
	return B_NO_ERROR;
}

static status_t
write_lblock(memory_stick_info *info, uint32 lblock, uint8 start_page,
             iovec_desc *data)
{
	const char   *debugprefix = "parallelms -- write_lblock: ";
	status_t      err;
	uint32        old_pblock;
	uint32        new_pblock;
	int           i;
	int           retry_count = 6;
	iovec_desc    curdata;
	size_t        bytes_written;
	bigtime_t     t1, ut, e1t = 0, wt = 0, ct = 0, ect = 0, e2t;
	int           wc = 0, cc = 0;
	bool          source_is_bad = false;
	
	if(info->read_only) {
		dprintf("%sread only device\n", debugprefix);
		return B_READ_ONLY_DEVICE;
	}

	err = translate_block(info, lblock, &old_pblock);
	if(err != B_NO_ERROR)
		return err;

	t1 = system_time();
	err = mark_block_for_update(old_pblock);
	ut = system_time() - t1;
	if(err != B_NO_ERROR)
		return err;
	
retry:
	curdata = *data;
	bytes_written = 0;
	
	new_pblock = get_free_block(info, lblock);
	if(new_pblock == 0xffff) {
		dprintf("%scould not get free block to write lblock %ld\n",
		        debugprefix, lblock);
		return B_ERROR;
	}
	//dprintf("write lblock %ld, old pblock %ld, new pblock %ld, page %d-\n",
	//        lblock, old_pblock, new_pblock, start_page);

#if 1 /* test, reenable for before checkin */
	t1 = system_time();
	err = erase_block(new_pblock);
	e1t = system_time() - t1;
	if(err != B_NO_ERROR)
		goto write_err;
#endif

	for(i = 0; i < info->block_size * 2; i++) {
		//dprintf("write pblock %d, page %d\n", new_pblock, i);
		if(i >= start_page && curdata.veccount > 0) {
			t1 = system_time();
			err = write_sector((uint8*)curdata.vec->iov_base +
			                   curdata.start_offset,
			                   new_pblock, i, lblock);
			wt += system_time() - t1;
			wc++;
			if(err != B_NO_ERROR)
				goto write_err;
			bytes_written += 512;
			curdata.start_offset += 512;
			if(curdata.start_offset >= curdata.vec->iov_len) {
				curdata.vec++;
				curdata.veccount--;
				curdata.start_offset = 0;
			}
		}
		else {
			t1 = system_time();
			err = copy_sector(old_pblock, i, new_pblock, i, &source_is_bad);
			ct += system_time() - t1;
			cc++;
			if(err != B_NO_ERROR)
				goto write_err;
		}
	}

	t1 = system_time();
	if(1) {	/* extra error check, remove? */
		memory_stick_regs regs;
		uint16 edata_lblock;
		const uint8 ow_mask = REG_OWFLAG_MASK_BKST | REG_OWFLAG_MASK_UDST;

		for(i = 0; i < info->block_size * 2; i++) {
			err = check_sector(regs, new_pblock, i);
			if(err != B_NO_ERROR)
				goto write_err;
			
			edata_lblock = regs[REG_LADDR1] << 8 | regs[REG_LADDR0];
	
			if((regs[REG_STATUS1]) != 0) {
				dprintf("%sread error pblock %d, sreg1: 0x%02x\n", debugprefix,
				        (int)new_pblock, regs[REG_STATUS1]);
				err = B_IO_ERROR;
				goto write_err;
			}
			
			if((edata_lblock != 0xffff && edata_lblock != lblock) ||
			   (regs[REG_OWFLAG] & ow_mask) != ow_mask) {
				dprintf("%swrite verify failed, pblock %ld\n",
				        debugprefix, new_pblock);
				dprintf("%sexpected lblock %ld, got %d\n",
				        debugprefix, lblock, edata_lblock);
				dprintf("%ssr0 0x%02x, sr1 0x%02x, owf 0x%02x, mf 0x%02x\n",
				        debugprefix, regs[REG_STATUS0], regs[REG_STATUS1],
				        regs[REG_OWFLAG], regs[REG_MFLAG]);
				err = B_IO_ERROR;
				goto write_err;
			}
		}
	}
	ect = system_time() - t1;

	remap_block(info, lblock);

	t1 = system_time();
	if(source_is_bad) {
		dprintf("%slblock %d, error detected in old pblock %d\n", debugprefix,
		        (int)lblock, (int)old_pblock);
		mark_block_bad(old_pblock);
		remove_free_block(info, lblock);
	}
	else {
		err = erase_block(old_pblock);
	}
	//err = erase_block_bad(old_pblock, info->block_size * 2, lblock);
	e2t = system_time() - t1;
	if(err != B_NO_ERROR)
		return err;

	//dprintf("%slblock %d was pblock %d, now %d\n",
	//        debugprefix, lblock, old_pblock, new_pblock);
	//dprintf("%su %Ld e %Ld w %Ld(%d) c %Ld(%d) ec %Ld e %Ld\n",
	//        debugprefix, ut, e1t, wt, wc, ct, cc, ect, e2t);
	
	*data = curdata;
	return bytes_written;

write_err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	if(err == B_DEV_READ_ERROR) {
		dprintf("%sread failed, lblock %ld pblock %ld\n",
		        debugprefix, lblock, old_pblock);
		erase_block(new_pblock);
		return err;
	}
	dprintf("%swrite failed, pblock %ld\n", debugprefix, new_pblock);
	mark_block_bad(new_pblock);
	remove_free_block(info, lblock);
	if(retry_count-- > 0)
		goto retry;
	if(err > B_NO_ERROR)
		err = B_IO_ERROR;
	return err;
}

status_t
write_lsectors(ms_handle *h, off_t pos,
               const iovec * const vec, size_t veccount, size_t *numBytes)
{
	status_t   err;
	uint8      start_page;
	uint32     lblock;
	iovec_desc data;
	size_t     bytes_written = 0;
	memory_stick_info *ms_info;
	
	*numBytes = 0;
	
	data.vec = vec;
	data.veccount = veccount;
	data.start_offset = 0;

	err = get_ms_info(h, &ms_info);
	if(err != B_NO_ERROR)
		return err;
	
	if(ms_info->read_only)
		return B_READ_ONLY_DEVICE;
		
	while(data.veccount > 0) {
		start_page = (pos / 512) % (ms_info->block_size * 2);
		lblock = pos / 512 / (ms_info->block_size * 2);
	
		if(pos < 0 || lblock >= ms_info->num_lblocks)
			return B_DEV_SEEK_ERROR;
		
		bytes_written = err = write_lblock(ms_info, lblock,
		                                   start_page, &data);
		if(err < 0)
			return err;
		*numBytes += bytes_written;
		pos += bytes_written;
	}
	return B_NO_ERROR;
}

/* driver entry */

status_t
open_ms(const char *name, uint32 flags, void **cookie)
{
	ms_handle *h;
	//dprintf("parallelms: open\n");

	h = malloc(sizeof(*h));
	if(h == NULL)
		return B_NO_MEMORY;
	h->interruptable = true;
	h->last_media_change_time = system_time();

	/* ignore media change before open */
	if(acquire_ms(h, false) != B_NO_ERROR)
		h->last_media_change_time = system_time();
	else
		release_ms(h);

	*cookie = h;
	return B_NO_ERROR;
}

status_t
close_ms(void *cookie)
{
	return B_NO_ERROR;
}

status_t
free_ms(void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t
control_ms(void *cookie, uint32 op, void *data, size_t len)
{
	const char *debugprefix = "parallelms -- control_ms: ";
	status_t err;
	ms_handle *h = cookie;

	switch(op) {
		case B_GET_GEOMETRY: {
			device_geometry *dinfo = (device_geometry *) data;
			memory_stick_info *ms_info;
			//dprintf("parallelms: B_GET_GEOMETRY\n");
			dinfo->sectors_per_track = 1;
			dinfo->head_count        = 1;
			err = acquire_ms(h, true);
			if(err != B_NO_ERROR)
				return err;
			if(get_ms_info(h, &ms_info) == B_NO_ERROR) {
				dinfo->cylinder_count = ms_info->num_lblocks *
				                        ms_info->block_size * 2;
				dinfo->read_only = ms_info->read_only;
			}
			else {
				dinfo->cylinder_count = 0;
				dinfo->read_only = false;
			}
			release_ms(h);
			dinfo->bytes_per_sector = 512;
			dinfo->removable = true;
			dinfo->device_type = B_DISK;
			dinfo->write_once = false;
			//dprintf("parallelms: B_GET_GEOMETRY done\n");
			return B_NO_ERROR;
		}
		case B_GET_MEDIA_STATUS:
			err = acquire_ms(h, true);
			if(err != B_NO_ERROR)
				return err;
			err = tpc_get_media_status(h->last_media_change_time);
			if(err == B_DEV_MEDIA_CHANGED)
				h->last_media_change_time = system_time();
			release_ms(h);
			(*(status_t*)data) = err;
			return B_NO_ERROR;
		case B_SET_UNINTERRUPTABLE_IO:
			dprintf("%sB_SET_UNINTERRUPTABLE_IO\n", debugprefix);
			h->interruptable = false;
			return B_NO_ERROR;
		case B_SET_INTERRUPTABLE_IO:
			dprintf("%sB_SET_INTERRUPTABLE_IO\n", debugprefix);
			h->interruptable = true;
			return B_NO_ERROR;
		case B_GET_ICON:
			return B_NOT_ALLOWED;
		case B_SCSI_PREVENT_ALLOW:
			return B_NOT_ALLOWED;
		case B_FLUSH_DRIVE_CACHE:
			return B_NO_ERROR;
		default:
			dprintf("%sunknown control %lx\n", debugprefix, op);
			return B_ERROR;
	}
}

status_t
read_ms(void *cookie, off_t pos, void *data, size_t *numBytes)
{
	const char *debugprefix = "parallelms -- read_ms: ";
	status_t err;
	ms_handle *h = cookie;
	bigtime_t t1 = system_time();
	
	int num_blocks = *numBytes / 512;
	int blocks_read = 0;
	
	//dprintf("read_ms: pos %Ld, size %ld\n", pos, *numBytes);
	*numBytes = 0;
	if(num_blocks <= 0) {
		dprintf("%sshort read, %ld bytes\n", debugprefix, *numBytes);
		return B_ERROR;
	}
	if(pos % 512 != 0) {
		dprintf("%sunaligned read, pos %Ld\n", debugprefix, pos);
		return B_ERROR;
	}
	
	while(blocks_read < num_blocks) {
		err = acquire_ms(h, false);
		if(err != B_NO_ERROR)
			return err;
		err = read_lsector(h, pos, data);
		release_ms(h);
		if(err != B_NO_ERROR)
			return err;
		data += 512;
		pos += 512;
		blocks_read++;
		*numBytes += 512;
	}
	
	if(total_io_count > 0) {
		total_read_time += system_time() - t1;
		total_read_bytes += *numBytes;
	}
	total_io_count++;
	//dprintf("%sdone\n", debugprefix);
	return B_NO_ERROR;
}

status_t
writev_ms(void *cookie, off_t pos, const iovec * const vec,
          const size_t veccount, size_t *numBytes)
{
	const char *debugprefix = "parallelms -- writev_ms: ";
	status_t err;
	ms_handle *h = cookie;
	bigtime_t t1 = system_time();
	int num_blocks = 0;
	int i;
	
	*numBytes = 0;
	if(pos % 512 != 0) {
		dprintf("%sunaligned write, pos %Ld\n", debugprefix, pos);
		return B_ERROR;
	}
	for(i = 0; i < veccount; i++) {
		if(vec[i].iov_len % 512 != 0) {
			dprintf("%sunaligned iovec, size %ld\n",
			        debugprefix, vec[i].iov_len);
			return B_ERROR;
		}
		num_blocks += vec[i].iov_len / 512;
	}
	//dprintf("writev_ms: pos %Ld, block count %d\n", pos, num_blocks);
	if(num_blocks <= 0) {
		dprintf("%sshort write, %ld bytes\n", debugprefix, *numBytes);
		return B_ERROR;
	}
	
	err = acquire_ms(h, false);
	if(err != B_NO_ERROR)
		return err;
	err = write_lsectors(h, pos, vec, veccount, numBytes);
	release_ms(h);
	if(err != B_NO_ERROR)
		return err;

	if(total_io_count > 0) {
		total_write_time += system_time() - t1;
		total_write_bytes += *numBytes;
	}
	total_io_count++;
	return B_NO_ERROR;
}

status_t
write_ms(void *cookie, off_t pos, const void *data, size_t *numBytes)
{
	iovec vec[1];

	vec[0].iov_base = (void*)data;
	vec[0].iov_len = *numBytes;
	return writev_ms(cookie, pos, vec, 1, numBytes);
}

static device_hooks parallelms_device_hooks = {
	&open_ms,
	&close_ms,
	&free_ms,
	&control_ms, /* control */
	&read_ms,
	&write_ms, /* write */
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	&writev_ms, /* writev */
};

int32 api_version = B_CUR_DRIVER_API_VERSION; 

static const char *device_name_list[] = {
	"disk/memorystick/parallel/0/raw",
	NULL
};

const char **
publish_devices()
{
	return device_name_list;
}

device_hooks *
find_device(const char *name)
{
	//dprintf("parallelms -- find device: name %s\n", name);
	return &parallelms_device_hooks;
}
