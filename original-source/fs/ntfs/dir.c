#include "ntfs.h"
#include "attributes.h"
#include "util.h"
#include "io.h"

static status_t ntfs_get_next_dir_entry(ntfs_fs_struct *ntfs, vnode *v, dircookie *d, dir_entry *entry);
static status_t ntfs_walk_dir(ntfs_fs_struct *ntfs, vnode *v, const char *file_utf8, uint16 *file, int file_name_len, dir_entry *entry, int level, void *index_buf);

// Called by the fsil when the directory needs to be closed.
int	ntfs_closedir(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	dircookie 		*d 	= (dircookie *)cookie;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
		
	DEBUGPRINT(DIR, 5, ("ntfs_closedir called on node %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

// The fsil calls fs_free_dircookie whenever it feels that the cookie can be safely
// deleted. Notice that it is seperate from the fs_closedir() call.
int ntfs_free_dircookie(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	dircookie 		*d 	= (dircookie *)cookie;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
	
	DEBUGPRINT(DIR, 5, ("ntfs_free_dircookie called on node %Ld.\n", v->vnid));

	// Lookie, free cookie.
	ntfs_free(d);
	
	return B_OK;
}

// This function is called whenever a read/write is about to happen on a directory. Allocates a dircookie.
int ntfs_opendir(void *ns, void *node, void **cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	dircookie 		*d;
	index_root_attr *index_root;
	status_t err;

	TOUCH(ntfs);TOUCH(v);
	
	DEBUGPRINT(DIR, 5, ("ntfs_opendir on vnode %Ld.\n", v->vnid));
	
	if(!v->is_dir) {
//		ERRPRINT(("ntfs_opendir called on non-directory.\n"));
		err = ENOTDIR;
		goto error;
	}
		
	// Find the index root. this will also test if it's a directory
	index_root = (index_root_attr *)ntfs_find_attribute(node, ATT_INDEX_ROOT, 0);
	if(!index_root) {
		ERRPRINT(("ntfs_opendir couldn't find index root attribute on file, it's probably not a directory\n"));
		err = ENOTDIR;
		goto error;
	}

	// Allocate the cookie
	d = (dircookie *)ntfs_malloc(sizeof(dircookie));
	if(!d) {
		ERRPRINT(("ntfs_opendir had problem allocating memory for cookie.\n"));
		err = ENOMEM;
		goto error;
	}

	// Initialize the cookie to the beginning of the directory.	
	d->flags = DC_IN_ROOT;
	d->next_pos = 0;
	d->next_index_buffer = 0;
	
	*cookie = d;
	
	DEBUGPRINT(DIR, 5, ("ntfs_opendir exit.\n"));
	
	return B_NO_ERROR;

error:
	return err;
}

// This function grabs the next dir entry in the directory
int	ntfs_readdir(void *ns, void *node, void *cookie, long *num, struct dirent *ent, size_t bufsize)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	dircookie 		*d 	= (dircookie *)cookie;
	dircookie 		tempcookie;
	status_t err;
	dir_entry entry;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
	
	DEBUGPRINT(DIR, 5, ("ntfs_readdir entry on vnode %Ld.\n", v->vnid));	

	*num = 0;

	// Copy the current cookie to a temporary one that readdir will use
	LOCK(ntfs->dir_lock);
	tempcookie.next_index_buffer = d->next_index_buffer;
	tempcookie.next_pos = d->next_pos;
	tempcookie.flags = d->flags;
	UNLOCK(ntfs->dir_lock);
		
	err = ntfs_get_next_dir_entry(ntfs, v, &tempcookie, &entry);
	
	// Copy the one readdir messed with back to here
	LOCK(ntfs->dir_lock);
	d->next_index_buffer = tempcookie.next_index_buffer;
	d->next_pos = tempcookie.next_pos;
	d->flags = tempcookie.flags;
	UNLOCK(ntfs->dir_lock);
	
	// we read something
	if(err >= B_NO_ERROR) {
		// Start filling in the POSIX entry			
		ent->d_dev = ntfs->nsid;
		ent->d_ino = strip_high_16(entry.record);
		strcpy(ent->d_name, entry.name);
		ent->d_reclen = 2*sizeof(dev_t)+2*sizeof(ino_t)+sizeof(unsigned short)+strlen(ent->d_name)+1;		
		*num = 1;
		DEBUGPRINT(DIR, 6, ("ntfs_readdir read entry '%s'\n", entry.name));
	}
	
	// we hit end of file
	if(err == ENOENT) {
		err = B_NO_ERROR;
	}
	
	DEBUGPRINT(DIR, 5, ("ntfs_readdir exit with error code %ld.\n", err));
	
	return err;
}

// Simply takes the directory back to the beginning.
int	ntfs_rewinddir(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs 	= ns;
	vnode			*v 		= node;
	dircookie 		*d		=(dircookie *)cookie;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);

	DEBUGPRINT(DIR, 5, ("ntfs_rewinddir called on vnid %Ld.\n", v->vnid));

	// Lock this so the operation is essentially atomic
	LOCK(ntfs->dir_lock);
	d->flags = DC_IN_ROOT;
	d->next_pos = 0;
	d->next_index_buffer = 0;
	UNLOCK(ntfs->dir_lock);

	DEBUGPRINT(DIR, 5, ("ntfs_rewinddir exit.\n"));

	return B_NO_ERROR;
}

int	ntfs_walk(void *ns, void *base, const char *file, char **newpath, vnode_id *vnid)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)base;
	void *buf = NULL;
	uint16 *unicode_file = NULL;
	status_t err;
	int32 srcLen;
	int32 dstLen;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(DIRWALK, 5, ("ntfs_walk entry on vnode %Ld, looking for file '%s'\n", v->vnid, file));

	// Now, lets go get it
	buf = ntfs_malloc(sizeof(dir_entry));
	if(!buf) {
		err = ENOMEM;
		goto out;
	}

	// For performance reasons, the filename string we pass into the walk function
	// needs to be unicode. ick.
	srcLen = strlen(file); // utf8
	dstLen = srcLen * 2;   // max possible unicode size
	unicode_file = ntfs_malloc(dstLen);
	if(!unicode_file) {
		err = ENOMEM;
		goto out;
	}

	// Convert the incoming utf8 file to unicode
	err = ntfs_utf8_to_unicode(file, &srcLen, (char *)unicode_file, &dstLen);
	if(err < B_NO_ERROR) goto out;

	// Walk it
	err = ntfs_walk_dir(ntfs, v, file, unicode_file, dstLen, (dir_entry *)buf, 0, NULL);
	if(err < B_NO_ERROR) {
		goto out;
	}

	// Get the return
	{
		dir_entry *entry;
		vnode *temp_vnode;
		
		entry = (dir_entry *)buf;
		*vnid = strip_high_16(entry->record);
		
		DEBUGPRINT(DIRWALK, 6, ("ntfs_walk found file '%s' at vnid %Ld.\n", file, *vnid));
				
		err = get_vnode(ntfs->nsid, *vnid, (void **)&temp_vnode);
		if(err < B_NO_ERROR) goto out;
	}
			
out:
	DEBUGPRINT(DIRWALK, 5, ("ntfs_walk exiting with code %ld.\n", err));
	if(unicode_file) ntfs_free(unicode_file);
	if(buf) ntfs_free(buf);	
		
	return err;		
}

static status_t ntfs_fill_dir_entry(ntfs_index_entry *ntfs_ie, ntfs_attr_FILE_NAME *ntfs_fn, dir_entry *entry)
{
	entry->record = B_LENDIAN_TO_HOST_INT64(ntfs_ie->record);

	{
		status_t err;
		int32 unicode_name_len = ntfs_fn->name_length*2;
		int32 max_utf8_name_len = MAX_FILENAME_LENGTH;
		
		err = ntfs_unicode_to_utf8((char *)((int)ntfs_fn + FILE_NAME_ATTR_NAME_OFFSET),
			&unicode_name_len, entry->name, &max_utf8_name_len);
		if(err < B_NO_ERROR) return B_ERROR;
		entry->name[max_utf8_name_len] = 0;
	}		

	DEBUGPRINT(DIR, 7, ("ntfs_fill_dir_entry filled entry for '%s', vnid 0x%8Lx.\n", entry->name, entry->record));	
		
	entry->name_type = ntfs_fn->file_name_type;
	
	return B_NO_ERROR;
}

static int ntfs_get_and_compare_filename(ntfs_fs_struct *ntfs, ntfs_attr_FILE_NAME *fn, uint16 *file, int file_name_len, bool *bresult)
{
	status_t err;
	int result;
	int result1;
	
	*bresult = false;
	
	// Do the case insensitive compare, using the supplied UpCase table.
	result = ntfs_strcasecmp_unicode(ntfs, file, file_name_len,
		(uint16 *)((int)fn + FILE_NAME_ATTR_NAME_OFFSET), fn->name_length*2);
	if(result == 0) {
		// The case-insensitive compare matched, but did the case-sensitive match work?
		result1 = ntfs_strcmp_unicode(file, file_name_len,
			(uint16 *)((int)fn + FILE_NAME_ATTR_NAME_OFFSET), fn->name_length*2);
		// This returns if it was an absolute match or not
		if(result1 == 0) *bresult = true;
	}
		
	DEBUGPRINT(DIRWALK, 7, ("\tresult of compare is %d\n", result));

	return result;
}

static status_t ntfs_generate_dotdot_entry(ntfs_fs_struct *ntfs, vnode *v, dir_entry *entry)
{
	file_name_attr *file_name;	
		
	DEBUGPRINT(DIR, 6, ("\tgenerating '..' entry.\n"));

	// Look up the file name attribute for the current node. There cannot be any
	// hard links to directories, so this should be safe.
	file_name = ntfs_find_attribute(v, ATT_FILE_NAME, 0);
	if(!file_name) return EIO;
	
	entry->record = file_name->dir_file_rec;	

	// Set the name	
	strcpy(entry->name, "..");
	entry->name_type = 1; // UNICODE name			
	
	return B_NO_ERROR;
}

static status_t ntfs_generate_dot_entry(vnode *v, dir_entry *entry)
{
	DEBUGPRINT(DIR, 6, ("\tgenerating '.' entry.\n"));

	entry->record = v->vnid;

	// Set the name	
	strcpy(entry->name, ".");
	entry->name_type = 1; // UNICODE name

	return B_NO_ERROR;
}


// Walks down the NTFS b-tree, looking for file name
static status_t ntfs_walk_dir(ntfs_fs_struct *ntfs, vnode *v, const char *file_utf8, uint16 *file, int file_name_len, dir_entry *entry, int level, void *index_buf)
{
	status_t err = B_NO_ERROR;
	size_t length_read;
	ntfs_index_entry *ie;
	ntfs_attr_FILE_NAME *fn;
	index_root_attr *ir;

	DEBUGPRINT(DIRWALK, 5, ("ntfs_walk_dir entry. level = %d\n", level));

	// Get the index root attribute
	ir = ntfs_find_attribute(v, ATT_INDEX_ROOT, 0);
	if(!ir) {
		err = EIO;
		goto out;
	}

	// If level == 0, this is the top of the btree search
	if(level == 0) {
		int rootpos = 0;
		bool donewithroot = false;

		index_buf = NULL;
		
		// See if we need a . entry manufactured
		if(!strcmp(file_utf8, ".")) {
			return ntfs_generate_dot_entry(v, entry);
		}
		
		// See if we need a .. entry manufactured
		if(!strcmp(file_utf8, "..")) {
			return ntfs_generate_dotdot_entry(ntfs, v, entry);
		}

startroot:
		if(donewithroot) {
			err = ENOENT; // not really an error, per se...
			goto outroot;
		}
		DEBUGPRINT(DIRWALK, 7, ("\tsearching root for entry.\n"));
		// Lets see where the index root is pointing
		ie = (ntfs_index_entry *)((char *)ir->entries + rootpos);		
		if(ie->flags & INDEX_ENTRY_LAST_ENTRY) {
			DEBUGPRINT(DIRWALK, 7, ("\tlast entry in root "));
			if(!(ie->flags & INDEX_ENTRY_SUB_NODE)) {
				// no sub node, so this really is the end of the search
				DEBUGPRINT(DIRWALK, 7, ("without subnode.\n"));
				donewithroot = true;
				goto startroot;
			}
			// This is the last entry in the root. Most of the time, if the
			// directory is stored out of root, this is all of the entries
			// you'll see here. This short entry will point to a VCN in the
			// index allocation data stream.
			{
				index_alloc_attr *ia;		
				uint64 index_pos;
				
				DEBUGPRINT(DIRWALK, 7, ("with subnode.\n"));

				// Allocate a buffer to store the index buffer
				index_buf = ntfs_malloc(ir->index_buffer_size);
				if(!index_buf) {
					err = ENOMEM;
					goto outroot;
				}

				// Find the index allocation attribute				
				ia = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
				if(!ia) {
					err = EIO;
					goto outroot;
				}
				
				// Load in the index buffer to continue the search
				if(ntfs->cluster_size > ir->index_buffer_size) {
					// If the cluster size is greater than the buffer size,
					// the field means sectors into dir file, not VCNs
					uint64 *sectors_into_dir = (uint64 *)((unsigned int)ie + 0x10);
				
					index_pos = B_LENDIAN_TO_HOST_INT64(*sectors_into_dir) * ntfs->nbb.bytes_per_sector;					

					DEBUGPRINT(DIRWALK, 7, ("\tsubnode's offset is %Ld\n", index_pos));
				} else {
					VCN *index_vcn = (VCN *)((unsigned int)ie + 0x10);
					DEBUGPRINT(DIRWALK, 7, ("\tsubnode's VCN is %Ld\n", B_LENDIAN_TO_HOST_INT64(*index_vcn)));
				
					index_pos = B_LENDIAN_TO_HOST_INT64(*index_vcn) * ntfs->cluster_size;
				}

				// read the index buffer in
				err = ntfs_read_extent(ntfs, &ia->extent, index_buf,
					index_pos, ir->index_buffer_size, &length_read);
				if(err < B_NO_ERROR) goto outroot;
				if(length_read != ir->index_buffer_size) {
					err = EIO;
					goto outroot;
				}

				// Recursively call us with the new buffer
				err = ntfs_walk_dir(ntfs, v, file_utf8, file, file_name_len, entry, level+1, index_buf);
			}
		} else {
			// This isn't the last entry in the root
			int result;
			bool bresult;
			
			DEBUGPRINT(DIRWALK, 7, ("\tnot last entry in root\n"));
			
			fn = (ntfs_attr_FILE_NAME *)((char *)ir->entries + rootpos + INDEX_ENTRY_SIZE);

			// Move the pointer up
			rootpos += B_LENDIAN_TO_HOST_INT16(ie->entry_size);

//			if(fn->file_name_type == FILE_NAME_ATTR_DOS_FLAG) goto startroot;
			
			result = ntfs_get_and_compare_filename(ntfs, fn, file, file_name_len, &bresult);
			if(result == 0) {
				if(bresult) {
					err = ntfs_fill_dir_entry(ie, fn, entry);
				} else {
					// It was not a case match
					DEBUGPRINT(DIRWALK, 7, ("\tnot a case match\n"));
					donewithroot = true;
					goto startroot;
				}
			}	
			if(result > 0) goto startroot;
			if(result < 0) {
				if(ie->flags & INDEX_ENTRY_SUB_NODE) {
					index_alloc_attr *ia;		
					uint64 index_pos;
					
					DEBUGPRINT(DIRWALK, 7, ("\tgoing into subnode...\n"));

					// Allocate a buffer to store the index buffer
					index_buf = ntfs_malloc(ir->index_buffer_size);
					if(!index_buf) {
						err = ENOMEM;
						goto outroot;
					}

					ia = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
					if(!ia) {
						err = EIO;
						goto outroot;
					}

					// Load in the index buffer to continue the search
					if(ntfs->cluster_size > ir->index_buffer_size) {
						// If the cluster size is greater than the buffer size,
						// the field means sectors into file, not VCNs
						uint64 *sectors_into_dir = (uint64 *)((unsigned int)ie + B_LENDIAN_TO_HOST_INT16(ie->entry_size) - 8);
					
						index_pos = B_LENDIAN_TO_HOST_INT64(*sectors_into_dir) * ntfs->nbb.bytes_per_sector;					
	
						DEBUGPRINT(DIRWALK, 7, ("\tsubnode's offset is %Ld\n", index_pos));
					} else {
						VCN *index_vcn = (VCN *)((unsigned int)ie + B_LENDIAN_TO_HOST_INT16(ie->entry_size) - 8);
						DEBUGPRINT(DIRWALK, 7, ("\tsubnode's VCN is %Ld\n", B_LENDIAN_TO_HOST_INT64(*index_vcn)));
					
						index_pos = B_LENDIAN_TO_HOST_INT64(*index_vcn) * ntfs->cluster_size;
					}
					
					// read the index buffer in
					err = ntfs_read_extent(ntfs, &ia->extent, index_buf,
						index_pos, ir->index_buffer_size, &length_read);
					if(err < B_NO_ERROR) goto outroot;
					if(length_read != ir->index_buffer_size) {
						err = EIO;
						goto outroot;
					}
	
					// Recursively call us with the new buffer
					err = ntfs_walk_dir(ntfs, v, file_utf8, file, file_name_len, entry, level+1, index_buf);
				} else {
					// There was no subnode
					DEBUGPRINT(DIRWALK, 7, ("\tno subnode\n"));
					donewithroot = true;
					goto startroot;
				}
			}
		}
outroot:
		// We're done with the root
		if(index_buf) ntfs_free(index_buf);
	} else {
		ntfs_index_buffer_header *bh;
		int bufpos = 0;
		bool donewithbuf = false;
	
		DEBUGPRINT(DIRWALK, 7, ("\tchecking magic..."));
	
		// check the magic number of the buffer
		bh = (ntfs_index_buffer_header *)index_buf;
		if(B_LENDIAN_TO_HOST_INT32(bh->magic) != INDEX_BUFFER_MAGIC) { // INDX
			ERRPRINT(("NTFS: index buffer failed magic check\n"));
			err = EIO;
			goto out;
		}
		
		DEBUGPRINT(DIRWALK, 7, ("done\n\tdoing fixup..."));
		
		// fix it up
		err = ntfs_fixup_buffer(index_buf, ir->index_buffer_size);
		if(err < B_NO_ERROR) {
			ERRPRINT(("NTFS: index buffer failed fixup check\n"));
			err = EIO;
			goto out;
		}

		DEBUGPRINT(DIRWALK, 7, ("done\n"));
		
		// Kick the pointer past the buffer header
		bufpos += B_LENDIAN_TO_HOST_INT16(bh->header_size) + 0x18;
		if(bufpos >= ir->index_buffer_size) {
			ERRPRINT(("NTFS: index header size is too large\n"));
			err = EIO;
			goto out;
		}

startbuf:
		if(donewithbuf) {
			err = ENOENT;
			goto out;
		}
		DEBUGPRINT(DIRWALK, 7, ("\tsearching buffer for next entry. bufpos = %d\n", bufpos));
			
		// Look at the next entry
		ie = (ntfs_index_entry *)((char *)index_buf + bufpos);		
	
		if(ie->flags & INDEX_ENTRY_LAST_ENTRY) {
			DEBUGPRINT(DIRWALK, 7, ("\tlast entry in buffer "));
			if(!(ie->flags & INDEX_ENTRY_SUB_NODE)) {
				// no sub node, so this really is the end of the search
				DEBUGPRINT(DIRWALK, 7, ("without subnode.\n"));
				donewithbuf = true;
				goto startbuf;
			}
			// Last entry in the buffer. Search here.
			{
				index_alloc_attr *ia;		
				uint64 index_pos;
					
				DEBUGPRINT(DIRWALK, 7, ("with subnode.\n"));
				
				ia = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
				if(!ia) {
					err = EIO;
					goto out;
				}
				
				// Load in the index buffer to continue the search
				if(ntfs->cluster_size > ir->index_buffer_size) {
					// If the cluster size is greater than the buffer size,
					// the field means sectors into file, not VCNs
					uint64 *sectors_into_dir = (uint64 *)((unsigned int)ie + 0x10);
				
					index_pos = B_LENDIAN_TO_HOST_INT64(*sectors_into_dir) * ntfs->nbb.bytes_per_sector;					

					DEBUGPRINT(DIRWALK, 7, ("\tsubnode's offset is %Ld\n", index_pos));
				} else {
					VCN *index_vcn = (VCN *)((unsigned int)ie + 0x10);
					DEBUGPRINT(DIRWALK, 7, ("\tsubnode's VCN is %Ld\n", B_LENDIAN_TO_HOST_INT64(*index_vcn)));
				
					index_pos = B_LENDIAN_TO_HOST_INT64(*index_vcn) * ntfs->cluster_size;
				}
				
				// read the index buffer in
				err = ntfs_read_extent(ntfs, &ia->extent, index_buf,
					index_pos, ir->index_buffer_size, &length_read);
				if(err < B_NO_ERROR) goto out;
				if(length_read != ir->index_buffer_size) {
					err = EIO;
					goto out;
				}

				// Recursively call us with the new buffer
				err = ntfs_walk_dir(ntfs, v, file_utf8, file, file_name_len, entry, level+1, index_buf);
			}
		} else {
			// This isn't the last entry in this buffer
			int result;
			bool bresult;
			
			DEBUGPRINT(DIRWALK, 7, ("\tnot last entry in buffer\n"));
			
			// Move the pointer up
			bufpos += B_LENDIAN_TO_HOST_INT16(ie->entry_size);
			
			fn = (ntfs_attr_FILE_NAME *)((char *)ie + INDEX_ENTRY_SIZE);
			
			result = ntfs_get_and_compare_filename(ntfs, fn, file, file_name_len, &bresult);
			if(result == 0) {
				if(bresult) {
					err = ntfs_fill_dir_entry(ie, fn, entry);
				} else {
					// It was not a case match
					DEBUGPRINT(DIRWALK, 7, ("\tnot a case match\n"));
					donewithbuf = true;
					goto startbuf;
				}
			}	
			if(result > 0) goto startbuf;
			if(result < 0) {
				if(ie->flags & INDEX_ENTRY_SUB_NODE) {
					index_alloc_attr *ia;		
					uint64 index_pos;
							
					DEBUGPRINT(DIRWALK, 7, ("\tgoing into subnode...\n"));

					ia = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
					if(!ia) {
						err = EIO;
						goto out;
					}

					// Load in the index buffer to continue the search
					if(ntfs->cluster_size > ir->index_buffer_size) {
						// If the cluster size is greater than the buffer size,
						// the field means sectors into file, not VCNs
						uint64 *sectors_into_dir = (uint64 *)((unsigned int)ie + B_LENDIAN_TO_HOST_INT16(ie->entry_size) - 8);
					
						index_pos = B_LENDIAN_TO_HOST_INT64(*sectors_into_dir) * ntfs->nbb.bytes_per_sector;					
	
						DEBUGPRINT(DIRWALK, 7, ("\tsubnode's offset is %Ld\n", index_pos));
					} else {
						VCN *index_vcn = (VCN *)((unsigned int)ie + B_LENDIAN_TO_HOST_INT16(ie->entry_size) - 8);
						DEBUGPRINT(DIRWALK, 7, ("\tsubnode's VCN is %Ld\n", B_LENDIAN_TO_HOST_INT64(*index_vcn)));
					
						index_pos = B_LENDIAN_TO_HOST_INT64(*index_vcn) * ntfs->cluster_size;
					}
					
					// read the index buffer in
					err = ntfs_read_extent(ntfs, &ia->extent, index_buf,
						index_pos, ir->index_buffer_size, &length_read);
					if(err < B_NO_ERROR) goto out;
					if(length_read != ir->index_buffer_size) {
						err = EIO;
						goto out;
					}
	
					// Recursively call us with the new buffer
					err = ntfs_walk_dir(ntfs, v, file_utf8, file, file_name_len, entry, level+1, index_buf);
				} else {
					// the search string is less than this entry, and it doesn't have a subnode
					// so we're done.
					DEBUGPRINT(DIRWALK, 7, ("\tno subnode\n"));
					donewithbuf = true;
					goto startbuf;
				}
			}
		}
	}

out:
	DEBUGPRINT(DIRWALK, 5, ("ntfs_walk_dir (%d) exit with code %ld.\n", level, err));
	return err;
} 

status_t ntfs_get_next_dir_entry(ntfs_fs_struct *ntfs, vnode *v, dircookie *d, dir_entry *entry)
{	
	status_t err;
	size_t length_read;
	ntfs_index_buffer_header *ntfs_bh;
	ntfs_index_entry *ntfs_ie;
	ntfs_attr_FILE_NAME *ntfs_fn;
	index_root_attr *index_root;
	index_alloc_attr *index_alloc;
	char *index_buffer = NULL;

	// Find the index root
	index_root = (index_root_attr *)ntfs_find_attribute(v, ATT_INDEX_ROOT, 0);
	if(!index_root) return EIO;
	
	DEBUGPRINT(DIR, 5, ("ntfs_get_next_dir_entry entry on vnode %Ld.\n", v->vnid));

tryagain:
	
	DEBUGPRINT(DIR, 7, ("\tat top of function. d->flags = 0x%x, d->next_pos = %ld, d->next_index_buffer = %Ld.\n", d->flags, d->next_pos, d->next_index_buffer));
	
	if(d->flags & DC_AT_END) {
		// We've already hit the end of this directory
		DEBUGPRINT(DIR, 6, ("\tend of directory hit.\n"));
		if(index_buffer) ntfs_free(index_buffer);
		return ENOENT;
	}

	// Generate the '.' entry if it hasn't been read yet
	if(!(d->flags & DC_READ_DOT) && !(v->vnid == FILE_ROOT)) {
		err = ntfs_generate_dot_entry(v, entry);
		if(err < B_NO_ERROR) return err;
	
		// update the cookie
		d->flags |= DC_READ_DOT;

		DEBUGPRINT(DIR, 5, ("ntfs_get_next_dir_entry exit.\n"));	
		return B_NO_ERROR;
	}
	
	// Generate the '..' entry if it hasn't been read yet
	if(!(d->flags & DC_READ_DOTDOT)) {
		err = ntfs_generate_dotdot_entry(ntfs, v, entry);
		if(err < B_NO_ERROR) return err;
				
		// update the cookie
		d->flags |= DC_READ_DOTDOT;

		DEBUGPRINT(DIR, 5, ("ntfs_get_next_dir_entry exit.\n"));
	
		return B_NO_ERROR;
	}
	
	// Lets read in real directory entries	
	
	if(d->flags & DC_IN_ROOT) {
		// Handle if we're still in the index root, stored in the vnode itself

		DEBUGPRINT(DIR, 6, ("\twe're in the root.\n"));
		
		// make sure we wont look past the end of the buffer
		if(d->next_pos >= index_root->length) {
			// This meant that the last directory entry read pushed the pointer too far.
			// lets try to continue
			d->flags &= ~DC_IN_ROOT;
			d->next_index_buffer = 0;
			d->next_pos = 0;
			goto tryagain;
		}
		
		// position the structures for looking at the entry
		ntfs_ie = (ntfs_index_entry *)((char *)index_root->entries + d->next_pos);
		
		// see if we're looking at the last entry in the root
		if(ntfs_ie->flags & INDEX_ENTRY_LAST_ENTRY) {
			// this entry is null, so kick the pointers up and try again
			d->flags &= ~DC_IN_ROOT;
			d->next_index_buffer = 0;
			d->next_pos = 0;
			goto tryagain;
		}
		
		// If we've made it this far, we're looking at the indexed data stored locally in the vnode
		ntfs_fn = (ntfs_attr_FILE_NAME *)((char *)index_root->entries + d->next_pos + sizeof(ntfs_index_entry));
		d->next_pos += B_LENDIAN_TO_HOST_INT16(ntfs_ie->entry_size);
	} else {
		// Lets get the index allocation data
		index_alloc = (index_alloc_attr *)ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
		if(!index_alloc) {
			// we're done
			d->flags |= DC_AT_END;
			goto tryagain;
		} 	
			
		DEBUGPRINT(DIR, 6, ("\tlooking at nonlocal data.\n"));
				
		// make sure we aren't looking past the end of index. This would signify end-of-directory.
		if((d->next_index_buffer * index_root->index_buffer_size + d->next_pos) >= index_alloc->length) {
			d->flags |= DC_AT_END;
			goto tryagain;
		}
	
		// Allocate some space for an index buffer
		if(!index_buffer) {
			index_buffer = ntfs_malloc(index_root->index_buffer_size);
			if(!index_buffer) {
				ERRPRINT(("ntfs_get_next_dir_entry couldn't allocate enough space for an index buffer.\n"));
				return ENOMEM;
			}
		}
		
		// read the index buffer in
		err = ntfs_read_extent(ntfs, &index_alloc->extent, index_buffer, d->next_index_buffer * index_root->index_buffer_size,
				index_root->index_buffer_size, &length_read);
		if(err < B_NO_ERROR) {
			ntfs_free(index_buffer);
			return err;
		}
		if(length_read != index_root->index_buffer_size) {
			ERRPRINT(("ntfs_get_next_dir_entry had error reading in index buffer.\n"));
			ntfs_free(index_buffer);
			return EIO;
		}
		
		// check the magic number of the buffer
		ntfs_bh = (ntfs_index_buffer_header *)index_buffer;
		if(B_LENDIAN_TO_HOST_INT32(ntfs_bh->magic) != INDEX_BUFFER_MAGIC) { // INDX
			ERRPRINT(("ntfs_get_next_dir_entry found index buffer that failed the magic test.\n"));
			ntfs_free(index_buffer);
			return EIO;
		}
		
		// fix it up
		err = ntfs_fixup_buffer(index_buffer, index_root->index_buffer_size);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ntfs_get_next_dir_entry failed to fixup the index buffer.\n"));
			ntfs_free(index_buffer);
			return EIO;
		}
		
		// if the current offset pointer in the dircookie is 0, lets push it forward
		// enough to get past the index buffer header
		if(d->next_pos == 0) 
			d->next_pos += B_LENDIAN_TO_HOST_INT16(ntfs_bh->header_size) + 0x18;

		if(d->next_pos >= index_root->index_buffer_size) {
			ntfs_free(index_buffer);
			return EIO;
		}

		// find the index entry
		ntfs_ie = (ntfs_index_entry *)(index_buffer + d->next_pos);

		// see if we're looking at the last entry in the buffer
		if(ntfs_ie->flags & INDEX_ENTRY_LAST_ENTRY) {
			// this entry is null, so kick the pointers up and try again
			DEBUGPRINT(DIR, 7, ("\thit last entry in index buffer, moving up one.\n"));
			d->next_index_buffer++;
			d->next_pos = 0;
			goto tryagain;
		}
		
		// lay the standard attribute structure over the entry that d->next_pos points at
		ntfs_fn = (ntfs_attr_FILE_NAME *)(index_buffer + d->next_pos + sizeof(ntfs_index_entry));
		d->next_pos += B_LENDIAN_TO_HOST_INT16(ntfs_ie->entry_size);
	}

	// get rid of the 8.3 entries
	if(ntfs_fn->file_name_type == FILE_NAME_ATTR_DOS_FLAG) goto tryagain;

	// at this point, ntfs_fn points to the data in the directory entry
	if(ntfs_fill_dir_entry(ntfs_ie, ntfs_fn, entry) < B_NO_ERROR) {
		if(index_buffer) ntfs_free(index_buffer);
		return EIO;
	}
	
	DEBUGPRINT(DIR, 6, ("\tfound entry '%s'\n", entry->name));
	DEBUGPRINT(DIR, 5, ("ntfs_get_next_dir_entry exit.\n"));

	if(index_buffer) ntfs_free(index_buffer);

	return B_OK;
}

