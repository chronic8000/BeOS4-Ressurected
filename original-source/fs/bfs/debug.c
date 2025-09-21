#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "bfs.h"
#include "btlib/btree.h"
#include "btlib/btconf.h"

#define DEBUG 1
#ifdef USER
#define printf myprintf
int myprintf(const char *fmt, ...);
#else
#undef  printf
#define printf kprintf
#endif



void
print_sizes(void)
{
	printf("sizeof(block_run) == %d\n", sizeof(block_run));
	printf("sizeof(data_stream) == %d\n", sizeof(data_stream));
	printf("sizeof(bfs_inode) == %d\n", sizeof(bfs_inode));
	printf("sizeof(disk_super_block) == %d\n", sizeof(disk_super_block));
}

void
dump_block_run(block_run *da)
{
	printf("\tag : %u\t", da->allocation_group);
	printf("start : %u\t", da->start);
	printf("len : %u\n", da->len);
}

void
dump_inode_addr(inode_addr *ia)
{
	printf("\tag : %u\t", ia->allocation_group);
	printf("start : %u\t", ia->start);
	printf("len : %u\n", ia->len);
}


void
dump_data_stream(data_stream *ds)
{
	int i;
	
	printf("size: %Ld\n", ds->size);
	printf("direct blocks:\n");
	for(i=0; i < NUM_DIRECT_BLOCKS; i++) {
		if (ds->direct[i].start == 0 && ds->direct[i].len == 0)
			continue;
		
		dump_block_run(&ds->direct[i]);
	}
	printf("max direct range: %Ld\n", ds->max_direct_range);

	if (ds->indirect.start == 0xffff && ds->indirect.len == 0xffff)
		return;
	
	printf("indirect block:\n");
	dump_block_run(&ds->indirect);
	printf("max indirect range: %Ld\n", ds->max_indirect_range);

	if (ds->double_indirect.start == 0xffff &&
		ds->double_indirect.len == 0xffff)
		return;

	printf("double_indirect:\n");
	dump_block_run(&ds->double_indirect);
	printf("max double indirect range: %Ld\n", ds->max_double_indirect_range);
}


int dump_dirs = 0;


void
dump_btree_header(BT_INDEX *b)
{
	struct bt_cache *p;
	
	printf("btree @ 0x%lx\n", (ulong)b);
	printf("magic 0x%x root %Ld high %Ld freep %Ld (lru 0x%x mru 0x%x)\n",
		   b->sblk.magic, b->sblk.root, b->sblk.high, b->sblk.freep,
		   b->lru, b->mru);

	if (b->sblk.magic == BT_DFLTMAGIC) {  /* only do this if it looks sane */
		for(p=b->mru; p; p=p->prev)
			if (p->num != (bt_off_t)-1)
				printf("   page %8Ld flags 0x%2x data @ 0x%x\n", p->num,
				p->flags, p->p);

		if (dump_dirs) {
			for(p=b->mru; p; p=p->prev)
				bt_dump(b, p);
		}
	}
}

void
dump_inode(bfs_inode *inode)
{
	printf("magic1: 0x%x\n", inode->magic1);
	printf("inode size: %d\n", inode->inode_size);
	printf("inode #: %d:%d:%d\n", inode->inode_num.allocation_group,
		   inode->inode_num.start, inode->inode_num.len);
	printf("uid: %d ", inode->uid);
	printf("gid: %d ", inode->gid);
	printf("mode: %d ", inode->mode);
	printf("flags: %d\n", inode->flags);
	printf("creation time: %Ld (%Ld)\n", inode->create_time,
		   inode->create_time >> TIME_SCALE);
	printf("modified time: %Ld (%Ld)\n", inode->last_modified_time,
		   inode->last_modified_time >> TIME_SCALE);

	printf("parent inode #: %d:%d:%d\n", inode->parent.allocation_group,
		   inode->parent.start, inode->parent.len);

	printf("attr inode #: %d:%d:%d\n", inode->attributes.allocation_group,
		   inode->attributes.start, inode->attributes.len);

	if (S_ISLNK(inode->mode) == 0)
		printf("data stream @ 0x%lx  (size: %Ld)\n", (ulong)&inode->data,
			   inode->data.size);
	else
		printf("symlink (data @ 0x%lx): %s\n", (ulong)&inode->data,
			   (char *)&inode->data);

	printf("etc @ 0x%x\n", inode->etc);

	if (inode->etc->btree)
		dump_btree_header(inode->etc->btree);
}


void
dump_disk_super_block(bfs_info *bfs, disk_super_block *sb)
{
	printf("name: %.32s\n", sb->name);
	printf("magic1: 0x%x\n", sb->magic1);

	printf("fs byte order: ");
	if (check_byte_order(sb->fs_byte_order) == BFS_BIG_ENDIAN)
		printf("big endian\n");
	else
		printf("little endian\n");
	
	printf("block size: %d\n", sb->block_size);
	printf("num blocks: %Ld\n", sb->num_blocks);
	printf("num blocks used: %Ld\n", sb->used_blocks);
	printf("inode size: %d\n", sb->inode_size);
	
	printf("magic2: 0x%x\n", sb->magic2);
	printf("num blocks per allocation group: %d\n", sb->blocks_per_ag);
	printf("ag shift: %d\n", sb->ag_shift);
	printf("num ag's: %d\n", sb->num_ags);
	printf("flags: 0x%x\n", sb->flags);
	printf("magic3: 0x%x\n", sb->magic3);


	printf("log start: %Ld -- log end: %Ld\n", sb->log_start, sb->log_end);
	printf("log area: ");
	dump_block_run(&sb->log_blocks);

	printf("root dir inode:");
	dump_inode_addr(&sb->root_dir);
#if 0
	printf("root dir inode contents:\n");
	load_inode(bfs, &sb->root_dir, &bi);
	dump_inode(&bi.i);
	printf("--------------------------------\n");
#endif	
	printf("indices inode: ");
	dump_inode_addr(&sb->indices);

	printf("pad: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		   sb->pad[0], sb->pad[1], sb->pad[2], sb->pad[3], 
		   sb->pad[4], sb->pad[5], sb->pad[6], sb->pad[7]);
	printf("------------------------------------------\n");

	printf("nsid: %ld nbitmap blocks %Ld bv @ 0x%lx bbm_sem %d\n", bfs->nsid,
		   bfs->bbm.num_bitmap_blocks, (ulong)bfs->bbm.bv, (int)bfs->bbm_sem);
	printf("sem %d fd %d dev block size %d\n", bfs->sem, bfs->fd,
		   bfs->dev_block_size);
	printf("inodes: root 0x%lx index 0x%lx name 0x%lx size 0x%lx\n",
		   (ulong)bfs->root_dir, (ulong)bfs->index_index,
		   (ulong)bfs->name_index, (ulong)bfs->size_index);
	printf("create time 0x%lx last m. time 0x%lx\n",
		   (ulong)bfs->create_time_index,(ulong)bfs->last_modified_time_index);
	printf("index list @ 0x%lx\n", (ulong)bfs->index);

	printf("log sem %ld cur_log_end %Ld cur_lh 0x%lx active_lh %d\n",
		   bfs->log_sem, bfs->cur_log_end, (ulong)bfs->cur_lh, bfs->active_lh);
	printf("completed log entries @ 0x%lx tmp blocks @ 0x%lx tmp b. sem %ld\n",
		   (ulong)bfs->completed_log_entries, (ulong)bfs->tmp_blocks,
		   bfs->tmp_blocks_sem);
}


void
dump_dir(bfs_info *bfs, inode_addr *ia)
{
	int        len, nblocks;
	char       buf[256];
	dr9_off_t      inode_num, bnum;
	BT_INDEX  *b;
	bfs_inode *bi;
	inode_addr fia;

	bnum    = block_run_to_bnum(*ia);
	nblocks = bfs->dsb.inode_size / bfs->dsb.block_size;

	bi = get_block(bfs->fd, bnum, bfs->dsb.block_size);
	if (bi == NULL) {
		printf("could not read the inode %d %d %d!\n", ia->allocation_group,
			   ia->start, ia->len);
		return;
	}

    b = bt_optopen(BT_INODE,    &bi,
				   BT_BFS_INFO, bfs,
				   BT_CFLAG,    BT_NOCACHE,
				   BT_CACHE,    0,
				   BT_PSIZE,    512,
				   0);

	if (b == NULL) {
		printf("could not read the btree for inode %d %d %d!\n",
			   ia->allocation_group, ia->start, ia->len);
		release_block(bfs->fd, bnum);
		return;
	}

	if (bt_goto(b, BT_BOF) != BT_OK) {
		printf("could not go to beginning of btree for directory\n");
		bt_close(b);
		release_block(bfs->fd, bnum);
		return;
	}

	while(bt_traverse(b, BT_EOF, (bt_chrp)buf, sizeof(buf), &len, &inode_num) == BT_OK) {
		buf[len] = '\0';
		vnode_id_to_inode_addr(fia, inode_num);
		printf("entry: %s  (%d : %d : %d  (%Ld))\n", buf, fia.allocation_group,
			   fia.start, fia.len, block_run_to_bnum(fia));
	}
	
	bt_close(b);
	release_block(bfs->fd, bnum);
}


void
dump_bfs(bfs_info *bfs)
{
	printf("BFS Dump:\n");
	printf("fd %d dev_block_size %u dev_block_conversion %Ld\n",
		   bfs->fd, bfs->dev_block_size, bfs->dev_block_conversion);
	printf("tmp block data @ 0x%lx\n", (ulong)bfs->tmp_blocks);
	printf("num bitmap blocks: %Ld\n", bfs->bbm.num_bitmap_blocks);

	printf("Disk Super Block:\n");
	dump_disk_super_block(bfs, &bfs->dsb);

#if 0
	dump_dir(bfs, &bfs->dsb.root_dir);
	printf("-----------------------------------\n");
	vnode_id_to_inode_addr(ia, bfs->name_index->vnid);
	dump_dir(bfs, &ia);
#endif
}




#ifdef DEBUG 
static long num_fs = 0;
static bfs_info *bfs_arr[64];

int
bfs_dbg(int argc, char **argv)
{
	int       i;
	bfs_info *bfs;
	
	if (argc == 1) {
		if (num_fs == 1) {
			printf("bfs @ 0x%x\n", bfs_arr[0]);
			dump_bfs(bfs_arr[0]);
		} else {
			for(i=0; i < num_fs; i++)
				if (bfs_arr[i])
					printf("bfs_info @ 0x%x\n", bfs_arr[i]);
		}

		return 1;
	}

	for(i=1; i < argc; i++) {
		bfs = (bfs_info *)strtoul(argv[i], NULL, 0);
		if (bfs == NULL)
			continue;

		dump_bfs(bfs);
	}
	
	return 1;
}


int
binode_dbg(int argc, char **argv)
{
	int        i;
	bfs_inode *bi;
	
	if (argc < 2) {
		printf("usage: %s <addr>\n", argv[0]);
		return 1;
	}

	for(i=1; i < argc; i++) {
		bi = (bfs_inode *)strtoul(argv[i], NULL, 0);
		if (bi == NULL)
			continue;

		dump_inode(bi);
	}
	
	return 1;
}


int
dstream_dbg(int argc, char **argv)
{
	int          i;
	data_stream *ds;
    bfs_inode   *bi;
	
	if (argc < 2) {
		printf("usage: %s <addr>\n", argv[0]);
		return 1;
	}

	for(i=1; i < argc; i++) {
		ds = (data_stream *)strtoul(argv[i], NULL, 0);
		if (ds == NULL)
			continue;

		bi = (bfs_inode *)ds;
 		if (bi->magic1 == INODE_MAGIC1)	
			ds = &bi->data;

		dump_data_stream(ds);
	}
	
	return 1;
}


int
btree_dbg(int argc, char **argv)
{
	int       i;
	BT_INDEX *b;
	
	if (argc < 2) {
		printf("usage: %s <addr>\n", argv[0]);
		return 1;
	}

	for(i=1; i < argc; i++) {
		b = (BT_INDEX *)strtoul(argv[i], NULL, 0);
		if (b == NULL)
			continue;

		dump_btree_header(b);
	}
	
	return 1;
}



int
dir_dbg(int argc, char **argv)
{
	bfs_info   *bfs;
	inode_addr *ia;
	
	if (argc < 3) {
		printf("usage: %s <bfs_info addr> <inode_addr addr>\n", argv[0]);
		return 1;
	}


	bfs = (bfs_info *)strtoul(argv[1], NULL, 0);
	if (bfs == NULL)
		return 1;

	ia = (inode_addr *)strtoul(argv[2], NULL, 0);
	if (ia == NULL)
		return 1;

	dump_dir(bfs, ia);
	
	return 1;
}


int
dump_block_runs(int argc, char **argv)
{
	int         i, ag_shift = 13;
	block_run  *br;
	inode_addr *ia;
	static bfs_info _bfs_info;
	bfs_info *bfs = &_bfs_info;
	
	if (argc < 2) {
		printf("usage: %s <address of block> [<ag shift>]\n", argv[0]);
		return 1;
	}


	br = (block_run *)strtoul(argv[1], NULL, 0);
	if (br == NULL)
		return 1;

	if (argc == 3) 
		ag_shift = strtoul(argv[2], NULL, 0);
	if (ag_shift < 13) {
		printf("ag shift %d seems weird (should be > 13).\n", ag_shift);
		return 1;
	}

	bfs->dsb.ag_shift = ag_shift;  /* make block_run_to_bnum() work. sneaky. */
	

    printf("Dumping 1k worth of block runs:\n");
	for(i=0; i < 1024/sizeof(block_run); i++) {
		printf("%4d : %5d : %5d (%9Ld) ##",
			   br[i].allocation_group, br[i].start, br[i].len,
			   block_run_to_bnum(br[i]));
		if (i & 1)
			printf("\n");

		if (block_run_to_bnum(br[i]) == (dr9_off_t)0)
			break;
	}
	
	if ((i % 1) == 0)
		printf("\n");

	return 1;
}

#endif /* DEBUG */



void
init_debugging(bfs_info *bfs)
{
#ifdef  DEBUG
#ifndef USER
	static long called = 0;

	int which = atomic_add(&num_fs, 1);

	bfs_arr[which % (sizeof(bfs_arr)/sizeof(bfs_info *))] = bfs;

	if (atomic_add(&called, 1) != 0)
		return;
	
	add_debugger_command("bfs", bfs_dbg, "dump a bfs_info structure");
	add_debugger_command("binode", binode_dbg, "dump a bfs_info structure");
	add_debugger_command("dstream", dstream_dbg, "dump a data_stream structure");
	add_debugger_command("btree", btree_dbg, "dump a btree structure");
	add_debugger_command("ddir", dir_dbg, "dump a directory (dangerous!)");
	add_debugger_command("sdata", dump_small_data, "dump the binode small data");
	add_debugger_command("bruns", dump_block_runs, "dump a block of block runs");
#endif /* USER */
	
#endif /* DEBUG */
}



#ifdef USER
int
myprintf(const char *fmt, ...)
{
  va_list     ap;
  static char buff[512];

  va_start(ap, fmt);
  vsprintf(buff, fmt, ap);
  va_end(ap);

  return write(1, buff, strlen(buff));
}

char *
mygets(char *_s, int len)
{
	int i = 0, ret;
	char *s = _s;

	do {
		ret = read(0, &s[i++], 1);
	} while (ret > 0 && i < len && s[i-1] != '\n' && s[i-1] != 4);

	if (ret <= 0)
		s[0] = '\0';

	if (i < len)
		s[i] = '\0';
	else
		s[i-1] = '\0';

	return _s;
}

#define DUMP_LEN   64

dump_words(int argc, char **argv)
{
	static ulong addr = 0;
	static ulong len  = DUMP_LEN;
	
	if (argc >= 2)
		addr = strtoul(argv[1], NULL, 0);

	if (argc >= 3) {
		len = strtoul(argv[2], NULL, 0);
	}

	if (addr & 3) {    /* check for alignment! */
		myprintf("rounding addr 0x%x to word boundary\n", addr);
		addr &= ~3;
	}

	if ((ulong)addr < 0x80000000) {
		myprintf("oh no no... I'm not dumping 0x%x\n", addr);
		return 0;
	}

	hexdump((void *)addr, len);
	addr += len;

	return 0;
}


void
mydebugger(int signal)
{
	int  i, argc, len;
	char input[256], *ptr;
	char *args[32];
	static int in_debugger = 0;

	if (in_debugger)
		return;
	in_debugger = 1;
	
	myprintf("\nwelcome to the bfs debugger\n");

	while(1) {
		myprintf("dbg>> ");
		mygets(input, sizeof(input));

		if (input[0] == 4)
			break;

		ptr = strchr(input, '\n');
		if (ptr)
			*ptr = '\0';

		if (input[0] == '\0')
			continue;

		len = strlen(input);
		argc = 0;
		args[argc] = &input[0];
		for(i=0; i < len && input[i] != '\0'; i++) {
			if (input[i] == ' ') {
				input[i++] = '\0';
				while(input[i] == ' ' && input[i] != '\0')
					i++;
				
				if (input[i] != '\0')
					args[++argc] = &input[i];
			}
		}

		args[++argc] = NULL;
		

		if (strncmp(args[0], "bfs", 3) == 0) {
			bfs_dbg(argc, args);
		} else if (strncmp(args[0], "binode", 6) == 0) {
			binode_dbg(argc, args);
		} else if (strncmp(args[0], "dump", 4) == 0) {
			dump_words(argc, args);
		} else if (strncmp(args[0], "cont", 4) == 0) {
			break;
		} else if (strncmp(args[0], "quit", 4) == 0) {
			exit(5);
			break;
		} else {
			myprintf("command %s not understood (%s)\n", args[0], input);
		}
	}

	in_debugger = 0;
}
#endif /* USER */
