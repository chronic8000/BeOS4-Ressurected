#ifndef MEMORY_STICK_H
#define MEMORY_STICK_H

#include <SupportDefs.h>

typedef struct {
	uint16 *pblock;
	int    *next_free_block_index;
	uint16 *free_blocks;
} block_translation_info;

typedef struct {
	uint16                  block_size;
	uint16                  num_pblocks;
	uint16                  num_lblocks;
	bool                    read_only;
	int                     unused_block_count;
	uint16                 *unused_blocks; /* boot, info and defect */
	block_translation_info  ti;
	uint16                  current_lblock_written;
	uint16                  current_pblock_src;
	uint16                  current_pblock_dest;
	uint32                  current_page_written_mask;
} memory_stick_info;

#define MS_DATA_GOOD 0
#define MS_DATA_CORRECTED 1
#define MS_EDATA_CORRECTED 2
#define MS_DATA_AND_EDATA_CORRECTED 3

/* memory_stick */
status_t reset_memory_stick(void);
status_t read_sector(uint8 *buffer, uint32 block, uint8 page);
status_t read_data_sector(uint8 *buffer, uint32 block, uint8 page);
status_t check_sector(uint8 *regs, uint32 block, uint8 page);
status_t read_sector_extra_data(uint8 *regs, uint32 block, uint8 page);
status_t mark_block_bad(uint32 block);
status_t mark_block_for_update(uint32 block);
status_t copy_sector(uint32 src_block, uint8 src_page,
                     uint32 dest_block, uint8 dest_page, bool *source_is_bad);
status_t write_sector(const uint8 *data, uint32 pblock, uint8 page, uint32 lblock);
status_t erase_block(uint32 block);

/* block_translation */
status_t init_block_translation(memory_stick_info *mi);
void     uninit_block_translation(memory_stick_info *mi);
status_t translate_block(memory_stick_info *mi, uint32 lblock, uint32 *pblock);
uint32   get_free_block(memory_stick_info *mi, uint32 lblock);
void     remap_block(memory_stick_info *mi, uint32 lblock);
void     remove_free_block(memory_stick_info *mi, uint32 lblock);

#endif

