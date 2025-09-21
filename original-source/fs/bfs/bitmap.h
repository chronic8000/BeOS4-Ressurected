
#include <limits.h>


typedef int chunk;
#define BITS_IN_CHUNK   (sizeof(chunk)*CHAR_BIT)


typedef struct _BitVector
{
  int    numbits;         /* total number of bits in "bits" */
  int    next_free;       /* index of the next free bit */
  int    is_full;
  chunk *bits;            /* the actual bitmap */
}BitVector;

int  create_block_bitmap(bfs_info *bfs);
int  init_block_bitmap(bfs_info *bfs);
void shutdown_block_bitmap(bfs_info *bfs);

int  allocate_block_run(bfs_info *bfs, uint ag, int num_blocks, block_run *br,
						int exact);
int  pre_allocate_block_run(bfs_info *bfs, uint ag, int num_blocks, block_run *br);
int  free_block_run(bfs_info *bfs, block_run *br);
int  check_block_run(bfs_info *bfs, block_run *br, int state); /* debugging */
void sanity_check_bitmap(bfs_info *bfs);


/*
 * for the exact argument to allocate_block_run
 */
#define EXACT_ALLOCATION	1
#define LOOSE_ALLOCATION	2
#define TRY_HARD_ALLOCATION 3


