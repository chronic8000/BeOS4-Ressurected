status_t ntfs_load_extent_from_runlist(extent_storage *extent, void *runlist, int32 length_to_read, uint64 blocks_to_load, bool possibly_compressed);
status_t ntfs_get_block_from_extent(extent_storage *extent, uint64 *block, uint64 file_block);
status_t ntfs_count_runs_in_extent(extent_storage *extent, uint64 *count);
status_t ntfs_get_compression_group_extent(extent_storage *extent, uint64 compression_group, comp_file_extent *comp_extent);
void ntfs_free_extent(extent_storage *extent);
