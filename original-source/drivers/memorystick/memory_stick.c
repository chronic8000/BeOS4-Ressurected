#include "memory_stick.h"
#include "memory_stick_regs.h"
#include "memory_stick_commands.h"
#include "tpc.h"
#include <KernelExport.h>

#define MS_MISC_TIMEOUT 5000
#define MS_READ_TIMEOUT 5000
#define MS_WRITE_TIMEOUT 10000
#define MS_ERASE_TIMEOUT 100000

static status_t
tpc_set_cmd(uint8 cmd)
{
	return tpc_write(TPC_SET_CMD, &cmd, 1);
}

status_t 
reset_memory_stick()
{
	status_t err;
	err = tpc_reset();
	if(err != B_NO_ERROR)
		return err;
	err = tpc_set_cmd(CMD_RESET);
	if(err != B_NO_ERROR)
		return err;
	return B_NO_ERROR;
}

static status_t
check_intreg(uint8 intreg)
{
	const char *debugprefix = "parallelms -- check_intreg: ";

	if(intreg & REG_INT_MASK_CMDNK) {
		dprintf("%sCMD not accepted\n", debugprefix);
		return B_ERROR;
	}
	if(intreg & REG_INT_MASK_CED) {
		if(intreg & REG_INT_MASK_ERR) {
			status_t err;
			uint8 regs[31];
			//dprintf("check_intreg: some error\n");
			err = tpc_read(TPC_READ_REG, regs, sizeof(regs));
			if(err != B_NO_ERROR)
				return err;

			if((regs[REG_STATUS1] & ~REG_STATUS1_MASK_CORRECTED) == 0) {
				bool dg = !(regs[REG_STATUS1] & REG_STATUS1_MASK_DTER);
				bool eg = !(regs[REG_STATUS1] & REG_STATUS1_MASK_EXER) &&
				          !(regs[REG_STATUS1] & REG_STATUS1_MASK_FGER);
				return dg ?
				       (eg ? MS_DATA_GOOD : MS_EDATA_CORRECTED) :
				       (eg ? MS_DATA_CORRECTED : MS_DATA_AND_EDATA_CORRECTED);
			}
			
			dprintf("%slast CMD failed\n", debugprefix);
			dprintf("%ssr0 0x%02x, sr1 0x%02x, owf 0x%02x, mf 0x%02x, "
			        "la 0x%02x%02x\n",
			        debugprefix, regs[REG_STATUS0], regs[REG_STATUS1],
			        regs[REG_OWFLAG], regs[REG_MFLAG],
			        regs[REG_LADDR1], regs[REG_LADDR0]);
			return B_ERROR;
		}
		return B_NO_ERROR;
	}
	dprintf("%sCMD not completed\n", debugprefix);
	return B_ERROR;
}

static status_t
get_msint(bigtime_t timeout)
{
	status_t err;
	uint8 intreg;

	err = tpc_wait_int(timeout);
	if(err != B_NO_ERROR)
		return err;

	err = tpc_read(TPC_GET_INT, &intreg, 1);
	if(err != B_NO_ERROR) {
		return err;
	}
//	dprintf("get_msint: 0x%02x\n", intreg);
	return check_intreg(intreg);
}

static status_t
internal_read_sector(uint8 *buffer, uint32 block, uint8 page, bool data)
{
	const char        *debugprefix = "parallelms -- read_sector: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_regs  register_set;
	status_t           data_err;

retry:
	register_set[REG_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REG_BLOCKADDR2]  = block >> 16;
	register_set[REG_BLOCKADDR1]  = block >> 8;
	register_set[REG_BLOCKADDR0]  = block;
	register_set[REG_CMDPAR]      = REG_CMDPAR_PAGE_MODE;
	register_set[REG_PAGEADDR]    = page;
	memset(register_set+REGW_BASE+REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);

	err = tpc_write(TPC_WRITE_REG, register_set + REGW_BASE,
	                sizeof(register_set) - REGW_BASE);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_READ);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_wait_int(MS_READ_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_read(TPC_READ_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = data_err = check_intreg(register_set[REG_INT]);
	if(err < B_NO_ERROR)
		goto err;

	err = tpc_read(TPC_READ_PAGE_DATA, buffer, 512);
	if(err != B_NO_ERROR)
		goto err;

	if(data &&
	   (register_set[REG_OWFLAG] & REG_OWFLAG_MASK_PGST) != REG_OWFLAG_PGST_OK) {
		data_err = B_DEV_READ_ERROR;
		dprintf("%sbad page status block %ld, page %d, owf 0x%02x\n",
		        debugprefix, block, page, register_set[REG_OWFLAG]);
	}

	return data_err;

err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%sread pblock %ld, page %d failed, retry...\n",
		        debugprefix, block, page);
		goto retry;
	}
	dprintf("%sfailed to read block %ld, page %d\n", debugprefix, block, page);
	return err;
}

status_t
read_sector(uint8 *buffer, uint32 block, uint8 page)
{
	return internal_read_sector(buffer, block, page, false);
}

status_t
read_data_sector(uint8 *buffer, uint32 block, uint8 page)
{
	return internal_read_sector(buffer, block, page, true);
}

status_t
check_sector(uint8 *regs, uint32 block, uint8 page)
{
	const char        *debugprefix = "parallelms -- check_sector: ";
	status_t           err, data_err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = REG_CMDPAR_PAGE_MODE;
	register_set[REGW_PAGEADDR]    = page;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);

retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_READ);
	if(err != B_NO_ERROR)
		goto err;

	err = data_err = get_msint(MS_READ_TIMEOUT);
	if(err < B_NO_ERROR)
		goto err;

	err = tpc_read(TPC_READ_REG, regs, 31);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_CLEAR_BUF);
	if(err != B_NO_ERROR)
		goto err;

	err = get_msint(MS_MISC_TIMEOUT);
	if(err < B_NO_ERROR)
		goto err;

	return data_err;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%sread pblock %ld, page %d failed, retry...\n",
		        debugprefix, block, page);
		goto retry;
	}
	dprintf("%sfailed to read block %ld, page %d\n", debugprefix, block, page);
	return err;
}

status_t
read_sector_extra_data(uint8 *regs, uint32 block, uint8 page)
{
	const char        *debugprefix = "parallelms -- read_sector_extra_data: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = REG_CMDPAR_EXDATA_MODE;
	register_set[REGW_PAGEADDR]    = page;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);

retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_READ);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_wait_int(MS_READ_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_read(TPC_READ_REG, regs, 31);
	if(err != B_NO_ERROR)
		goto err;

	return B_NO_ERROR;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%sread %ld.%d edata failed, retry...\n",
		        debugprefix, block, page);
		goto retry;
	}
	dprintf("%sfailed to read block %ld, page %d edata\n", debugprefix, block, page);
	return err;
}

status_t 
mark_block_bad(uint32 block)
{
	const char        *debugprefix = "parallelms -- mark_block_bad: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = REG_CMDPAR_FLAG_WRITE_MODE;
	register_set[REGW_PAGEADDR]    = 0;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);
	register_set[REGW_OWFLAG]      =
		(uint8)~REG_OWFLAG_MASK_BKST | REG_OWFLAG_BKST_NG;
	
retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_WRITE);
	if(err != B_NO_ERROR)
		goto err;

	err = get_msint(MS_WRITE_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	return B_NO_ERROR;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%swrite pblock %ld edata failed, retry...\n",
		        debugprefix, block);
		goto retry;
	}
	dprintf("%sfailed to write block %ld edata\n", debugprefix, block);
	return err;
}

status_t 
mark_block_for_update(uint32 block)
{
	const char        *debugprefix = "parallelms -- mark_block_for_update: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = REG_CMDPAR_FLAG_WRITE_MODE;
	register_set[REGW_PAGEADDR]    = 0;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);
	register_set[REGW_OWFLAG]      = ~REG_OWFLAG_MASK_UDST;
	
retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_WRITE);
	if(err != B_NO_ERROR)
		goto err;

	err = get_msint(MS_WRITE_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	return B_NO_ERROR;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%swrite pblock %ld edata failed, retry...\n",
		        debugprefix, block);
		goto retry;
	}
	dprintf("%sfailed to write block %ld edata\n", debugprefix, block);
	return err;
}

status_t 
copy_sector(uint32 src_block, uint8 src_page,
            uint32 dest_block, uint8 dest_page, bool *source_is_bad)
{
	const char        *debugprefix = "parallelms -- copy_sector: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_regs  register_set;
	bool               read_failed;

	//dprintf("copy_sector src block %ld, page %d, dest block %ld, page %d\n",
	//        src_block, src_page, dest_block, dest_page);

retry:
	register_set[REG_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REG_BLOCKADDR2]  = src_block >> 16;
	register_set[REG_BLOCKADDR1]  = src_block >> 8;
	register_set[REG_BLOCKADDR0]  = src_block;
	register_set[REG_CMDPAR]      = REG_CMDPAR_PAGE_MODE;
	register_set[REG_PAGEADDR]    = src_page;
	memset(register_set + REGW_BASE + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);

	err = tpc_write(TPC_WRITE_REG, register_set+REGW_BASE,
	                sizeof(register_set)-REGW_BASE);
	if(err != B_NO_ERROR)
		goto read_err;

	err = tpc_set_cmd(CMD_BLOCK_READ);
	if(err != B_NO_ERROR)
		goto read_err;

	err = tpc_wait_int(MS_READ_TIMEOUT);
	if(err != B_NO_ERROR)
		goto read_err;

	err = tpc_read(TPC_READ_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto read_err;

	err = check_intreg(register_set[REG_INT]);
	if(err != MS_DATA_GOOD) {
		dprintf("%sdefect detected in %ld.%d\n",
		        debugprefix, src_block, src_page);
		*source_is_bad = true;
		if(err < B_NO_ERROR)
			register_set[REG_OWFLAG] = REG_OWFLAG_PGST_DATAERR |
			                 (register_set[REG_OWFLAG] & ~REG_OWFLAG_MASK_PGST);
	}
	read_failed = false;

#if 0
	/* optimization, current spec does not make this safe */
	if(register_set[REG_LADDR1] == 0xff && register_set[REG_LADDR0] == 0xff) {
		err = tpc_set_cmd(CMD_CLEAR_BUF);
		if(err != B_NO_ERROR)
			goto err;
	
		err = get_msint();
		if(err != B_NO_ERROR)
			goto err;
		
		return B_NO_ERROR;
	}
#endif

	if((register_set[REG_OWFLAG] & REG_OWFLAG_MASK_PGST) == REG_OWFLAG_PGST_NG){
		dprintf("%spage status NG detected in %ld.%d\n",
		        debugprefix, src_block, src_page);
		register_set[REG_OWFLAG] = REG_OWFLAG_PGST_DATAERR |
		                     (register_set[REG_OWFLAG] & ~REG_OWFLAG_MASK_PGST);
		*source_is_bad = true;
	}

	register_set[REG_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REG_BLOCKADDR2]  = dest_block >> 16;
	register_set[REG_BLOCKADDR1]  = dest_block >> 8;
	register_set[REG_BLOCKADDR0]  = dest_block;
	register_set[REG_CMDPAR]      = REG_CMDPAR_PAGE_MODE;
	register_set[REG_PAGEADDR]    = dest_page;
	
	register_set[REG_OWFLAG]     |= REG_OWFLAG_MASK_UDST;

	//dprintf("write dest block laddr 0x%02x%02x\n", register_set[REG_LADDR1],
	//        register_set[REG_LADDR0]);
	//dprintf("ow 0x%02x\n", register_set[REG_OWFLAG]);
	//dprintf("mf 0x%02x\n", register_set[REG_MFLAG]);
	//dprintf("sp 0x%02x\n", register_set[REG_SYSTEMPAR]);
	//dprintf("cp 0x%02x\n", register_set[REG_CMDPAR]);

	err = tpc_write(TPC_WRITE_REG, register_set+REGW_BASE,
	                sizeof(register_set)-REGW_BASE);
	if(err != B_NO_ERROR)
		goto write_err;

	err = tpc_set_cmd(CMD_BLOCK_WRITE);
	if(err != B_NO_ERROR)
		goto write_err;

	err = tpc_wait_int(MS_WRITE_TIMEOUT);
	if(err != B_NO_ERROR)
		goto write_err;

	err = tpc_read(TPC_READ_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto write_err;

	//dprintf("src write intreg: 0x%02x\n", register_set[REG_INT]);

	err = check_intreg(register_set[REG_INT]);
	if(err != MS_DATA_GOOD)
		goto write_err;

	return B_NO_ERROR;
	
read_err:
	read_failed = true;
write_err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%scopy %ld.%d to %ld.%d failed, retry...\n",
		        debugprefix, src_block, src_page, dest_block, dest_page);
		goto retry;
	}
	if(read_failed) {
		dprintf("%sfailed to read blk %ld, pg %d for copy\n",
		        debugprefix, src_block, src_page);
		return B_DEV_READ_ERROR;
	}
	dprintf("%sfailed to copy blk %ld, pg %d to blk %ld, pg %d\n",
	        debugprefix, src_block, src_page, dest_block, dest_page);
	return err;
}

status_t 
write_sector(const uint8 *data, uint32 block, uint8 page, uint32 lblock)
{
	const char        *debugprefix = "parallelms -- write_sector: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = REG_CMDPAR_PAGE_MODE;
	register_set[REGW_PAGEADDR]    = page;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);
	register_set[REGW_LADDR1]      = lblock >> 8;
	register_set[REGW_LADDR0]      = lblock;

retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_write(TPC_WRITE_PAGE_DATA, data, 512);
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_WRITE);
	if(err != B_NO_ERROR)
		goto err;

	err = get_msint(MS_WRITE_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	return B_NO_ERROR;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%swrite pblock %ld page %d failed, retry...\n",
		        debugprefix, block, page);
		goto retry;
	}
	dprintf("%sfailed to write block %ld, page %d\n", debugprefix, block, page);
	return err;
}

#if 0

/* test code, random erase failure */

status_t 
erase_block_bad(uint32 block, int page_count, uint32 lblock)
{
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = 0;
	register_set[REGW_PAGEADDR]    = 0;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);
	
retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	/* test erase atomicity */

	{
	uint8 cmd = CMD_BLOCK_ERASE;
	err = tpc_write_drop_power(TPC_SET_CMD, &cmd, 1);
	}
	if(err != B_NO_ERROR)
		goto err;
	//spin(10);
	tpc_reset();

	if(1) {	/* extra error check, remove? */
		const char   *debugprefix = "parallelms -- bad_erase: ";
		memory_stick_regs regs;
		status_t err;
		int i;
		uint16 edata_lblock;
		const uint8 ow_mask = REG_OWFLAG_MASK_BKST | REG_OWFLAG_MASK_PGST;

		for(i = 0; i < page_count; i++) {
			err = check_sector(regs, block, i);
			if(err != B_NO_ERROR)
				goto write_err;
			
			edata_lblock = regs[REG_LADDR1] << 8 | regs[REG_LADDR0];
	
			if((regs[REG_STATUS1]) != 0) {
				dprintf("%sread error pblock %d, sreg1: 0x%02x\n", debugprefix,
				        block, regs[REG_STATUS1]);
				err = B_IO_ERROR;
				goto write_err;
			}
			
			if(edata_lblock != 0xffff && edata_lblock != lblock ||
			   (regs[REG_OWFLAG] & ow_mask) != ow_mask) {
				dprintf("%swrite verify failed, pblock %ld\n",
				        debugprefix, block);
				dprintf("expected lblock %ld, got %d\n",
				        lblock, edata_lblock);
				dprintf("sreg0: 0x%02x, sreg1 0x%02x\n",
				        regs[REG_STATUS0], regs[REG_STATUS1]);
				dprintf("owf: 0x%02x, mf 0x%02x\n",
				        regs[REG_OWFLAG], regs[REG_MFLAG]);
				dprintf("la1: 0x%02x, la0 0x%02x\n",
				        regs[REG_LADDR1], regs[REG_LADDR0]);
				err = B_IO_ERROR;
				goto write_err;
			}
			if(i == 0) {
				if(edata_lblock == lblock)
					dprintf("pblock %d improperly erased, not erased\n", block);
				else
					dprintf("pblock %d improperly erased, erased\n", block);
			}
		}
	}
	return B_NO_ERROR;

write_err:
err:
	dprintf("pblock %d improperly erased, now bad\n", block);
	return B_ERROR;
}
#endif

status_t 
erase_block(uint32 block)
{
	const char        *debugprefix = "parallelms -- erase_block: ";
	status_t           err;
	int                retry_count = 3;
	memory_stick_wregs register_set;

	register_set[REGW_SYSTEMPAR]   = REG_SYSTEMPAR_MASK_BAMD;
	register_set[REGW_BLOCKADDR2]  = block >> 16;
	register_set[REGW_BLOCKADDR1]  = block >> 8;
	register_set[REGW_BLOCKADDR0]  = block;
	register_set[REGW_CMDPAR]      = 0;
	register_set[REGW_PAGEADDR]    = 0;
	memset(register_set + REGW_EXTRA_DATA, 0xff, REGW_EXTRA_DATA_SIZE);
	
retry:
	err = tpc_write(TPC_WRITE_REG, register_set, sizeof(register_set));
	if(err != B_NO_ERROR)
		goto err;

	err = tpc_set_cmd(CMD_BLOCK_ERASE);
	if(err != B_NO_ERROR)
		goto err;
		
	err = get_msint(MS_ERASE_TIMEOUT);
	if(err != B_NO_ERROR)
		goto err;

	return B_NO_ERROR;
	
err:
	if(err == B_DEV_NO_MEDIA || err == B_DEV_MEDIA_CHANGED)
		return err;
	tpc_reset();
	if(tpc_set_cmd(CMD_RESET) != B_NO_ERROR)
		dprintf("%sreset failed\n", debugprefix);

	if(retry_count-- > 0) {
		dprintf("%serase pblock %ld failed, retry...\n", debugprefix, block);
		goto retry;
	}
	dprintf("%sfailed to erase block %ld\n", debugprefix, block);
	return err;
}
