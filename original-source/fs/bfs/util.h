
int   init_tmp_blocks(bfs_info *bfs);
void  shutdown_tmp_blocks(bfs_info *bfs);

char *get_tmp_blocks(bfs_info *bfs, int nblocks);
void  free_tmp_blocks(bfs_info *bfs, char *blocks, int nblocks);

int   my_byte_order(void);
int   check_byte_order(int disk_val);
int   get_device_block_size(int fd);
dr9_off_t get_num_device_blocks(int fd);
int   device_is_read_only(const char *device);

int  bfs_die(const char *str, ...);


/* simply hash table structure and routines used by INODE_NO_CACHE
   files (i.e. the VM swap file) to prevent re-entering the cache
   while servicing a page-fault.

   this stuff is ripped straight out of the kernel cache code.  ick.
*/
typedef struct my_hash_ent {
	dr9_off_t           bnum;
	dr9_off_t           hash_val;
	void               *data;
	struct my_hash_ent *next;
} my_hash_ent;


typedef struct my_hash_table {
    my_hash_ent **table;
    int           max;
    int           mask;          /* == max - 1 */
    int           num_elements;
} my_hash_table;

int   init_hash_table(my_hash_table *ht);
void  shutdown_hash_table(my_hash_table *ht);
int   hash_insert(my_hash_table *ht, dr9_off_t bnum, void *data);
void *hash_lookup(my_hash_table *ht, dr9_off_t bnum);
void *hash_delete(my_hash_table *ht, dr9_off_t bnum);

