#include <malloc.h>
#include <KernelExport.h>
#include "memory_stick.h"
#include "memory_stick_regs.h"

#define FREE_BLOCKS_PER_SEGMENT 18

static status_t create_translation_table(memory_stick_info *info, uint16 lblock);

status_t 
init_block_translation(memory_stick_info *mi)
{
	int i;
	int segments = mi->num_pblocks / 512;
	
	mi->ti.pblock = malloc(sizeof(mi->ti.pblock[0]) * mi->num_lblocks);
	if(mi->ti.pblock == NULL)
		goto err0;
	mi->ti.next_free_block_index =
		malloc(sizeof(mi->ti.next_free_block_index[0]) * segments);
	if(mi->ti.next_free_block_index == NULL)
		goto err1;
	mi->ti.free_blocks = malloc(sizeof(mi->ti.free_blocks[0]) *
	                            FREE_BLOCKS_PER_SEGMENT * segments);
	if(mi->ti.free_blocks == NULL)
		goto err2;

	for(i = 0; i < mi->num_lblocks; i++) {
		mi->ti.pblock[i] = 0xfffe;
	}
	for(i = 0; i < segments; i++) {
		mi->ti.next_free_block_index[i] = 0;
	}

	return B_NO_ERROR;

//err3:
//	free(mi->ti.pblock);
err2:
	free(mi->ti.next_free_block_index);
err1:
	free(mi->ti.pblock);
err0:
	return B_NO_MEMORY;
}

void
uninit_block_translation(memory_stick_info *mi)
{
	free(mi->ti.pblock);
	free(mi->ti.next_free_block_index);
	free(mi->ti.free_blocks);
}

typedef struct {
	uint16   segment_size;
	uint16  *pblock;
	uint16  *free_blocks;
	uint8   *status_bits;
	int      mapped_free_blocks;
} segment_info;

static void
add_free_block(segment_info *si, uint16 pblock)
{
	const char *debugprefix = "parallelms -- add_free_block: ";
	int         i;

	for(i = 0; i < FREE_BLOCKS_PER_SEGMENT; i++) {
		if(si->free_blocks[i] == 0xffff) {
			si->free_blocks[i] = pblock;
			return;
		}
	}
	if(si->mapped_free_blocks == 0)
		dprintf("%sfree blocks list full, map unmapped lblocks\n", debugprefix);
	si->mapped_free_blocks++;
	for(i = 0; i <= si->segment_size; i++) {
		if(si->pblock[i] == 0xffff) {
			si->pblock[i] = pblock;
			si->status_bits[i] = 0x01;
			return;
		}
	}
	dprintf("%s internal error: no open block\n", debugprefix);
}

uint32 
get_free_block(memory_stick_info *mi, uint32 lblock)
{
	int      segment;
	int      free_block_index;
	uint16  *free_blocks;
	
	if(lblock < 494) {
		segment = 0;
	}
	else {
		segment = (lblock - 494) / 496 + 1;
	}
	free_block_index = mi->ti.next_free_block_index[segment];
	free_blocks = mi->ti.free_blocks + segment * FREE_BLOCKS_PER_SEGMENT;
	free_block_index = (free_block_index + 1) % FREE_BLOCKS_PER_SEGMENT;
	while(free_block_index != mi->ti.next_free_block_index[segment]) {
		if(free_blocks[free_block_index] != 0xffff)
			break;
		free_block_index = (free_block_index + 1) % FREE_BLOCKS_PER_SEGMENT;
	}
	mi->ti.next_free_block_index[segment] = free_block_index;
	return free_blocks[free_block_index];
}

void 
remap_block(memory_stick_info *mi, uint32 lblock)
{
	int      segment;
	int      free_block_index;
	uint16  *free_blocks;
	uint16   tmp;
	
	if(lblock < 494) {
		segment = 0;
	}
	else {
		segment = (lblock - 494) / 496 + 1;
	}
	free_block_index = mi->ti.next_free_block_index[segment];
	free_blocks = mi->ti.free_blocks + segment * FREE_BLOCKS_PER_SEGMENT;

	tmp = mi->ti.pblock[lblock];
	mi->ti.pblock[lblock] = free_blocks[free_block_index];
	free_blocks[free_block_index] = tmp;
}

void 
remove_free_block(memory_stick_info *mi, uint32 lblock)
{
	int      segment;
	int      free_block_index;
	uint16  *free_blocks;

	if(lblock < 494) {
		segment = 0;
	}
	else {
		segment = (lblock - 494) / 496 + 1;
	}
	free_block_index = mi->ti.next_free_block_index[segment];
	free_blocks = mi->ti.free_blocks + segment * FREE_BLOCKS_PER_SEGMENT;

	free_blocks[free_block_index] = 0xffff;
}

status_t 
translate_block(memory_stick_info *mi, uint32 lblock, uint32 *pblock)
{
	status_t err;
	if(lblock > mi->num_lblocks)
		return B_DEV_SEEK_ERROR;
	if(mi->ti.pblock[lblock] == 0xfffe) {
		err = create_translation_table(mi, lblock);
		if(err != B_NO_ERROR)
			return err;
	}
	if(mi->ti.pblock[lblock] == 0xffff) {
		dprintf("parallelms -- translate_block: lblock %ld unmapped\n",
		        lblock);
		return B_IO_ERROR;
	}
	*pblock = mi->ti.pblock[lblock];
	return B_NO_ERROR;
}

static bool
unused_block(memory_stick_info *info, uint16 pblock)
{
	int i;
	for(i = 0; i < info->unused_block_count; i++) {
		if(info->unused_blocks[i] == pblock)
			return true;
	}
	return false;
}

static status_t
erase_if_writeable(memory_stick_info *info, uint16 pblock,
                   const char *debug, uint16 lblock)
{
	const char *debugprefix = "parallelms -- create_translation_table: ";
	status_t    err;
	
	dprintf("%spblock %d is %s %d\n", debugprefix, pblock, debug, lblock);
	if(info->read_only) {
		dprintf("%sread-only media, pblock %d not erased\n",
		        debugprefix, pblock);
		return B_NO_ERROR;
	}

	err = erase_block(pblock);
	if(err == B_NO_ERROR)
		dprintf("%spblock %d erased\n", debugprefix, pblock);
	else
		dprintf("%sfailed to erase pblock %d\n", debugprefix, pblock);
	return err;
}

static status_t
create_translation_table(memory_stick_info *info, uint16 lblock)
{
	const char *debugprefix = "parallelms -- create_translation_table: ";
	status_t err;
	uint8 regs[31];
	int i;
	uint16 first_lblock; //, last_lblock;
	uint16 first_pblock, last_pblock;
//	int segment, segment_size;
	segment_info si;
//	uint8 *status_bits;
	int error_count;
	int mapped_blocks;
	bigtime_t t1, t2;
	
	t1 = system_time();
	
	/* first segment has 494 lblocks, other segments have 496 */
	if(lblock < 494) {
		//segment = 0;
		first_lblock = 0;
		si.segment_size = 494;
		si.pblock = info->ti.pblock;
		si.free_blocks = info->ti.free_blocks;
		first_pblock = 0;
	}
	else {
		int segment = (lblock - 494) / 496 + 1;
		first_lblock = segment * 496 - 2;
		si.segment_size = 496;
		si.pblock = info->ti.pblock + segment * 496 - 2;
		si.free_blocks = info->ti.free_blocks +
		                 segment * FREE_BLOCKS_PER_SEGMENT;
		first_pblock = segment * 512;
	}
	last_pblock = first_pblock + 511;
	//last_lblock = first_lblock + segment_size - 1;
	//dprintf("building translation table for segment %d, lblock %d-%d\n",
	//        segment, first_lblock, last_lblock);

	if(si.pblock - info->ti.pblock + si.segment_size > info->num_lblocks) {
		dprintf("%sinternal error, lblock %ld, out of range\n", debugprefix,
		        (int)(si.pblock - info->ti.pblock + si.segment_size) /
		        sizeof(si.pblock[0]));
		err = B_ERROR;
		goto err0;
	}
#if 0
	if(last_lblock >= info->num_lblocks) {
		dprintf("internal error: lblock %d-%d, out of range\n",
		        first_lblock, last_lblock);
		err = B_ERROR;
		goto err0;
	}
#endif
	if(last_pblock >= info->num_pblocks) {
		dprintf("%sinternal error, pblock %d-%d, out of range\n", debugprefix,
		        first_pblock, last_pblock);
		err = B_ERROR;
		goto err0;
	}
	
	si.status_bits = malloc(sizeof(si.status_bits[0]) * si.segment_size);
	if(si.status_bits == NULL) {
		err = B_NO_MEMORY;
		goto err0;
	}
	
	for(i = 0; i < si.segment_size; i++) {
		si.pblock[i] = 0xffff;
	}
	mapped_blocks = 0;
	si.mapped_free_blocks = 0;
	for(i = 0; i < FREE_BLOCKS_PER_SEGMENT; i++) {
		si.free_blocks[i] = 0xffff;
	}
	error_count = 0;
	for(i = first_pblock; i <= last_pblock; i++) {
		uint16 laddr;
		int segrel_laddr;
		uint8 sreg1, owf, mf;
		uint8 page = 0;

		if(error_count > 18) {
			/* with 18 errors there is not enough good blocks left to
			   map all the logical blocks */
			dprintf("%sto many errors while creating translation table\n",
			        debugprefix);
			err = B_DEV_READ_ERROR;
			goto err1;
		}
		if(unused_block(info, i))
			continue;

		err = read_sector_extra_data(regs, i, page);
		if(err != B_NO_ERROR) {
			if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
				goto err1;
			dprintf("%sread failed pblock %d page 0\n", debugprefix, i);
			error_count++;
			continue;
		}
		laddr = regs[REG_LADDR1] << 8 | regs[REG_LADDR0];
		segrel_laddr = laddr - first_lblock;
		sreg1 = regs[REG_STATUS1];
		owf = regs[REG_OWFLAG];
		mf = regs[REG_MFLAG];

		if((owf & REG_OWFLAG_MASK_BKST) == REG_OWFLAG_BKST_NG) {
			dprintf("%spblock %d is marked as bad\n", debugprefix, i);
			continue;
		}
		//if((owf & REG_OWFLAG_MASK_PGST) == REG_OWFLAG_PGST_NG) {
		//	dprintf("%spblock %d, page 0 is marked as bad\n", debugprefix, i);
		//	continue;
		//}

		if((sreg1 & ~REG_STATUS1_MASK_CORRECTED) != 0) {
			dprintf("%sread error pblock %d, sreg1: 0x%02x\n", debugprefix,
			        i, sreg1);
			error_count++;
			if(!info->read_only) {
				mark_block_bad(i);
			}
			continue;
		}

		//dprintf("pblock %3d lb %3d sreg1 0x%02x ireg 0x%02x ow 0x%02x\n",
		//        i, laddr, regs[0x03], regs[0x01], regs[0x16]);
		if((mf & REG_MFLAG_MASK_ATFLG) == 0) {
			dprintf("%spblock %d is translation table\n", debugprefix, i);
		    if(erase_if_writeable(info, i, "trtable", laddr) == B_OK)
				add_free_block(&si, i);
			continue;
		}
		if(laddr == 0xffff) {
			add_free_block(&si, i);
			continue;
		}
		if(segrel_laddr < 0 || segrel_laddr >= si.segment_size) {
		//if(laddr < first_lblock || laddr > last_lblock) {
			dprintf("%sladdr %d out of range (%d-%d)\n", debugprefix,
			        laddr, first_lblock, first_lblock + si.segment_size - 1);
		    if(erase_if_writeable(info, i, "bad lblock", laddr) == B_OK)
				add_free_block(&si, i);
			continue;
		}
		if(info->ti.pblock[laddr] != 0xffff &&
		   (owf & REG_OWFLAG_MASK_UDST) == REG_OWFLAG_MASK_UDST &&
		    si.status_bits[segrel_laddr] == 0) {
		    if(erase_if_writeable(info, i, "unfinshed lblock", laddr) == B_OK)
				add_free_block(&si, i);
			continue;
		}
		if(info->ti.pblock[laddr] != 0xffff) {
			if(erase_if_writeable(info, info->ti.pblock[laddr],
			                      (si.status_bits[segrel_laddr] & 0x01) == 0 ?
			                      "old lblock" : "unfinshed lblock", laddr) ==
			   B_NO_ERROR)
				add_free_block(&si, info->ti.pblock[laddr]);
		}
		else {
			mapped_blocks++;
		}
		//dprintf("lblock %d -> pblock %d\n", laddr, i);
		info->ti.pblock[laddr] = i;
		si.status_bits[segrel_laddr] = owf & REG_OWFLAG_MASK_UDST;
	}
	t2 = system_time();
	if(mapped_blocks != si.segment_size) {
		dprintf("%s%d blocks are unmapped from %d to %d\n", debugprefix,
		        si.segment_size - mapped_blocks, first_lblock,
		        first_lblock + si.segment_size - 1);
		dprintf("%s%d already remapped {", debugprefix, si.mapped_free_blocks);
		for(i = first_lblock; i < first_lblock + si.segment_size; i++) {
			if(info->ti.pblock[i] == 0xffff)
				dprintf(" %d", i);
		}
		dprintf(" }\n");
		mapped_blocks += si.mapped_free_blocks;
		for(i = first_lblock; i < first_lblock + si.segment_size; i++) {
			if(info->ti.pblock[i] == 0xffff) {
				if(get_free_block(info, i) != 0xffff) {
					remap_block(info, i);
					dprintf("%sremapped %d to %d\n", debugprefix, i, info->ti.pblock[i]);
					mapped_blocks++;
				}
				else {
					dprintf("%sno block to remap %d\n", debugprefix, i);
				}
			}
		}
	}
	if(mapped_blocks != si.segment_size) {
		dprintf("%s%d blocks are still unmapped\n", debugprefix,
		        si.segment_size - mapped_blocks);
		err = B_DEV_READ_ERROR;
		goto err1;
	}
	dprintf("%sdone, %Ld us\n", debugprefix, t2 - t1);
#if 0
	dprintf("lblock to pblock table for segment %d:\n", first_pblock/512);
	i = 0;
	while(i < si.segment_size) {
		dprintf("%5d:", i+first_lblock);
		do {
			if(info->ti.pblock[first_lblock+i] == 0xffff)
				dprintf(" -----");
			else
				dprintf(" %5d", info->ti.pblock[i+first_lblock]);
			i++;
		} while((i < si.segment_size) && (i % 10 != 0));
		dprintf("\n");
	}
	dprintf("free blocks:\n");
	i = 0;
	while(i < FREE_BLOCKS_PER_SEGMENT) {
		do {
			if(si.free_blocks[i] != 0xffff)
				dprintf(" %5d", si.free_blocks[i]);
			i++;
		} while((i < FREE_BLOCKS_PER_SEGMENT) && (i % 10 != 0));
		dprintf("\n");
	}
#endif
	err = B_NO_ERROR;
err1:
	free(si.status_bits);
err0:
	return err;
}

