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


int
init_tmp_blocks(bfs_info *bfs)
{
	int         i;
	tmp_blocks *tmp;
	
	bfs->tmp_blocks_sem = create_sem(1, "tmp_blocks_sem");
	if (bfs->tmp_blocks_sem < 0)
		return ENOMEM;


	bfs->tmp_blocks = (tmp_blocks *)malloc(sizeof(tmp_blocks));
	if (bfs->tmp_blocks == NULL)
		goto err1;

	tmp = bfs->tmp_blocks;
	tmp->next = NULL;
	
	tmp->data = (char *)malloc(NUM_TMP_BLOCKS * bfs->dsb.block_size);
	if (tmp->data == NULL)
		goto err2;

	for(i=0; i < NUM_TMP_BLOCKS; i++)
		tmp->block_ptrs[i] = tmp->data + (i * bfs->dsb.block_size);
		
	return 0;

 err2:
	free(bfs->tmp_blocks);
	bfs->tmp_blocks = NULL;
 err1:
	delete_sem(bfs->tmp_blocks_sem);
	bfs->tmp_blocks_sem = -1;
	
	return ENOMEM;
}


void
shutdown_tmp_blocks(bfs_info *bfs)
{
	int         i;
	tmp_blocks *tmp, *next;
	
	for(tmp=bfs->tmp_blocks; tmp; tmp=next) {
		for(i=0; i < NUM_TMP_BLOCKS; i++)
			if (tmp->block_ptrs[i] == NULL)
				printf("unfree'd tmp block @ 0x%lx index %d\n",
					   (ulong)(tmp->data + i * bfs->dsb.block_size), i);

		free(tmp->data);
		next = tmp->next;
		free(tmp);
	}

	if (bfs->tmp_blocks_sem > 0)
		delete_sem(bfs->tmp_blocks_sem);
	bfs->tmp_blocks_sem = -1;
}


char *
get_tmp_blocks(bfs_info *bfs, int nblocks)
{
	int         i, j, start;
	char       *blocks;
	tmp_blocks *tmp;

	if (nblocks > NUM_TMP_BLOCKS) {
		printf("*** error: requested %d tmp blocks but can only have %d\n",
			   nblocks, NUM_TMP_BLOCKS);
		return NULL;
	}

	acquire_sem(bfs->tmp_blocks_sem);

	for(tmp=bfs->tmp_blocks; tmp; tmp=tmp->next) {
		blocks = NULL;
		start  = -999999;

		for(i=0; i < NUM_TMP_BLOCKS; i++) {
			if (blocks == NULL && tmp->block_ptrs[i]) {
				start = i;
				blocks = tmp->block_ptrs[i];
			}

			if (blocks && tmp->block_ptrs[i] != NULL &&
				((i + 1) - start) == nblocks) {
				for(j=start; j <= i; j++) {
					if (tmp->block_ptrs[j] == NULL) {
						bfs_die("you fucking whore of the revelation\n");
					}
					tmp->block_ptrs[j] = NULL;
				}

				release_sem(bfs->tmp_blocks_sem);

				return blocks;
			} else if (tmp->block_ptrs[i] == NULL) {
				/* reset our pointers and keep on truckin' */
				blocks = NULL;
				start  = -9999999;
			}
		}
	}

	/*
	   if we get here then we couldn't find any free space and we have
	   to go allocate some with malloc.
	*/   
	tmp = (tmp_blocks *)malloc(sizeof(tmp_blocks));
	if (tmp == NULL)
		return NULL;
	
	tmp->data = (void *)malloc(NUM_TMP_BLOCKS * bfs->dsb.block_size);
	if (tmp->data == NULL) {
		free(tmp);
		return NULL;
	}

	for(i=0; i < nblocks; i++)
		tmp->block_ptrs[i] = NULL;
	
	for(; i < NUM_TMP_BLOCKS; i++)
		tmp->block_ptrs[i] = tmp->data + (i * bfs->dsb.block_size);
		
	tmp->next = bfs->tmp_blocks;
	bfs->tmp_blocks = tmp;
	
	release_sem(bfs->tmp_blocks_sem);

	return tmp->data;
}


void
free_tmp_blocks(bfs_info *bfs, char *blocks, int nblocks)
{
	int         i, j;
	char       *end;
	tmp_blocks *tmp;

	if (blocks == NULL)
		bfs_die("free_tmp_blocks called w/null block ptr (%d)\n", nblocks);

	if (nblocks > NUM_TMP_BLOCKS)
		bfs_die("*** free_tmp_blocks: nblocks == %d but should be < %d\n",
				nblocks, NUM_TMP_BLOCKS);

	acquire_sem(bfs->tmp_blocks_sem);
	
	for(tmp=bfs->tmp_blocks; tmp; tmp=tmp->next) {
		end = tmp->data + NUM_TMP_BLOCKS * bfs->dsb.block_size;
		
		if (blocks >= tmp->data && blocks < end) {  /* we have a match! */
			i = (blocks - tmp->data) / bfs->dsb.block_size;

			for(j=0; j < nblocks; j++, blocks+=bfs->dsb.block_size) {
				if (tmp->block_ptrs[i + j] != NULL) {
					bfs_die("free_tmp_blocks ptr 0x%x already free?!?\n",
						   tmp->block_ptrs[i + j]);
					break;
				}
				
				tmp->block_ptrs[i + j] = blocks;
			}

			break;
		}
	}

	if (tmp == NULL)
		bfs_die("free_tmp_blocks: failed to free %d blocks @ 0x%x\n",
				nblocks, blocks);

	release_sem(bfs->tmp_blocks_sem);
}


int bfs_really_hang = 1;


int
bfs_die(const char *fmt, ...)
{
  va_list     ap;
  static char buff[512];

  printf("\n");
  va_start(ap, fmt);
  vsprintf(buff, fmt, ap);
  va_end(ap);

  printf("%s\n", buff);
  
  if (bfs_really_hang) {
	  printf("spinning forever.\n");
	  while(1)
		  ;
  }

/*  kernel_debugger(NULL); */
  return 0;
}




#define HT_DEFAULT_MAX   128


int
init_hash_table(my_hash_table *ht)
{
	ht->max   = HT_DEFAULT_MAX;
	ht->mask  = ht->max - 1;
	ht->num_elements = 0;

	ht->table = (my_hash_ent **)calloc(ht->max, sizeof(my_hash_ent *));
	if (ht->table == NULL)
		return ENOMEM;

	return 0;
}


void
shutdown_hash_table(my_hash_table *ht)
{
	int       i, hash_len;
	my_hash_ent *he, *next;

	for(i=0; i < ht->max; i++) {
		he = ht->table[i];

		for(hash_len=0; he; hash_len++, he=next) {
			next = he->next;
			free(he);
		}
	}

	if (ht->table)
		free(ht->table);
	ht->table = NULL;
}

static void
print_hash_stats(my_hash_table *ht)
{
	int       i, hash_len, max = -1, sum = 0;
	my_hash_ent *he, *next;

	for(i=0; i < ht->max; i++) {
		he = ht->table[i];

		for(hash_len=0; he; hash_len++, he=next) {
			next = he->next;
		}
        if (hash_len)
			printf("bucket %3d : %3d\n", i, hash_len);

		sum += hash_len;
		if (hash_len > max)
			max = hash_len;
	}

	printf("max # of chains: %d,  average chain length %f (sum %d / max %d)\n",
		   max, (double)sum/(double)ht->max, sum, ht->max);
}


#define HASH(b)   (b * 37)

static my_hash_ent *
new_hash_ent(dr9_off_t bnum, void *data)
{
	my_hash_ent *he;

	he = (my_hash_ent *)malloc(sizeof(*he));
	if (he == NULL)
		return NULL;

	he->hash_val = HASH(bnum);
	he->bnum     = bnum;
	he->data     = data;
	he->next     = NULL;

	return he;
}


static int
grow_hash_table(my_hash_table *ht)
{
	int        i, omax, newsize, newmask;
	dr9_off_t      hash;
	my_hash_ent **new_table, *he, *next;
	
	if (ht->max & ht->mask) {
		printf("*** hashtable size %d or mask %d looks weird!\n", ht->max,
			   ht->mask);
	}

	omax    = ht->max;
	newsize = omax * 2;        /* have to grow in powers of two */
	newmask = newsize - 1;

	new_table = (my_hash_ent **)calloc(newsize, sizeof(my_hash_ent *));
	if (new_table == NULL)
		return ENOMEM;

	for(i=0; i < omax; i++) {
		for(he=ht->table[i]; he; he=next) {
			hash = he->hash_val & newmask;
			next = he->next;
			
			he->next        = new_table[hash];
			new_table[hash] = he;
		}
	}
	
	free(ht->table);
	ht->table = new_table;
	ht->max   = newsize;
	ht->mask  = newmask;
		
	return 0;
}




int
hash_insert(my_hash_table *ht, dr9_off_t bnum, void *data)
{
	dr9_off_t    hash;
	my_hash_ent *he, *curr;

	hash = HASH(bnum) & ht->mask;

	curr = ht->table[hash];
	for(; curr != NULL; curr=curr->next)
		if (curr->bnum == bnum)
			break;

	if (curr && curr->bnum == bnum) {
		printf("entry %Ld already in the hash table!\n", bnum);
		return EEXIST;
	}

	he = new_hash_ent(bnum, data);
	if (he == NULL)
		return ENOMEM;
	
	he->next        = ht->table[hash];
	ht->table[hash] = he;

	ht->num_elements++;
	if (ht->num_elements >= ((ht->max * 3) / 4)) {
		if (grow_hash_table(ht) != 0)
			return ENOMEM;
	}

	return 0;
}

void *
hash_lookup(my_hash_table *ht, dr9_off_t bnum)
{
	my_hash_ent *he;

	he = ht->table[HASH(bnum) & ht->mask];

	for(; he != NULL; he=he->next) {
		if (he->bnum == bnum)
			break;
	}

	if (he)
		return he->data;
	else
		return NULL;
}


void *
hash_delete(my_hash_table *ht, dr9_off_t bnum)
{
	void     *data;
	dr9_off_t     hash;
	my_hash_ent *he, *prev = NULL;

	hash = HASH(bnum) & ht->mask;
	he = ht->table[hash];

	for(; he != NULL; prev=he,he=he->next) {
		if (he->bnum == bnum)
			break;
	}

	if (he == NULL) {
		printf("*** hash_delete: tried to delete non-existent block %Ld\n",
			   bnum);
		return NULL;
	}

	data = he->data;

	if (ht->table[hash] == he)
		ht->table[hash] = he->next;
	else if (prev)
		prev->next = he->next;
	else
		panic("hash table is inconsistent\n");

	free(he);
	ht->num_elements--;

	return data;
}
