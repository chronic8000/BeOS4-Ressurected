#include "ntfs.h"
#include "attributes.h"
#include "io.h"
#include "extent.h"

// Needed by ioctl
status_t frag_disk(ntfs_fs_struct *ntfs, uint32 pattern, bool random);

int	ntfs_access(void *ns, void *node, int mode)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v = (vnode *)node;

	TOUCH(ntfs);
	
	DEBUGPRINT(FILE, 5, ("ntfs_access called on node %Ld with mode 0x%x.\n", v->vnid, mode));

	if((mode == O_RDWR) || (mode == O_WRONLY)) return EROFS;

	return B_OK;
}

// This is called by the fsil whenever the file needs to be closed.
int	ntfs_close(void *ns, void *node, void *cookie)
{
	vnode			*v 	= (vnode *)node;

	DEBUGPRINT(FILE, 5, ("ntfs_close called on node %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

// The fsil calls fs_free_cookie whenever it feels that the cookie can be safely
// deleted. Notice that it is seperate from the fs_close() call. 
int ntfs_free_cookie(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(FILE, 5, ("ntfs_free_cookie called on node %Ld.\n", v->vnid));	
	
	return B_NO_ERROR;
}

int	ntfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	status_t err = B_NO_ERROR;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(FS, 5, ("ntfs_ioctl called with command %d, buf = 0x%08x\n", cmd, (unsigned int)buf));

	switch(cmd) {
		case 10001: // get the filesystem block that contains the byte number passed
		{
			uint64 block = (*(uint64 *)buf) / ntfs->cluster_size;
			data_attr *data;

			if(buf == NULL) return EINVAL;

			// lookup the data attribute
			data = ntfs_find_attribute(v, ATT_DATA, 0);
			if(!data) return EIO;
			
			if(data->stored_local) return EINVAL;
			
			err = ntfs_get_block_from_extent(&data->extent, buf, block);
			break;
		}
		case 10002 : // return real creation time
		{
			std_info_attr *std_info;
			
			if(buf == NULL) return EINVAL;

			std_info = ntfs_find_attribute(v, ATT_STD_INFO, 0);
			if(!std_info) return EIO;		

			if (buf) *(bigtime_t *)buf = std_info->file_creation_time << 16;
			break;
		}
		case 10003 : // return real last modification time
		{
			std_info_attr *std_info;
			
			if(buf == NULL) return EINVAL;

			std_info = ntfs_find_attribute(v, ATT_STD_INFO, 0);
			if(!std_info) return EIO;		
			
			if (buf) *(bigtime_t *)buf = std_info->last_modification_time << 16;
			break;
		}
		case 10101 : // return number of runs in file
					 // (Works better than 10001 on NTFS, due to compressed
					 // files, and files stored in the MFT)
		{
			uint64 *count = (uint64 *)buf;

			if(buf == NULL) return EINVAL;

			// Check to see if we have a directory
			if(v->is_dir) {
				index_alloc_attr *ia = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
				
				// if we didn't find a index allocation attribute,
				// the directory is probably stored locally.
				// this means zero runs.
				if(!ia) {
					*count = 0;
					break;
				}
				
				// If we have an index allocation attribute, but it says
				// it's stored locally, we have a zero run count
				if(ia->stored_local) {
					*count = 0;
					break;
				}

				err = ntfs_count_runs_in_extent(&ia->extent, count);
			} else {
				data_attr *data = ntfs_find_attribute(v, ATT_DATA, 0);
				
				// if we didn't find a data attribute,
				// it means there cannot be any data in the file.
				// zero runs.
				if(!data) {
					*count = 0;
					break;
				}
				
				// If we have a data attribute, but it says
				// it's stored locally, we have a zero run count
				if(data->stored_local) {
					*count = 0;
					break;
				}

				err = ntfs_count_runs_in_extent(&data->extent, count);
			}							
			break;
		}		
		case 69666: // fragment the drive
			if(buf) {
				DEBUGPRINT(FS, 4, ("ntfs_ioctl was asked to frag the disk with pattern 0x%08lx.\n", *(uint32 *)buf));
				err = frag_disk(ntfs, *(uint32 *)buf, false);
			} else {
				DEBUGPRINT(FS, 4, ("ntfs_ioctl was asked to frag the disk with random pattern\n"));
				err = frag_disk(ntfs, 0, true);
			}
			break;
		default:
			ERRPRINT(("ntfs_ioctl was asked to do something invalid (%d).\n", cmd));
			err = EINVAL;
	}

	return err;
}

// fs_open is called whenever a read/write is going to happen. Allocates the file cookie.
int	ntfs_open(void *ns, void *node, int omode, void **cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;

	TOUCH(ntfs);TOUCH(v);

	if((omode == O_WRONLY) || (omode == O_RDWR)) {
//		ERRPRINT(("ntfs_open called for write access on Read Only filesystem\n"));
		return EROFS;
	}

	if((omode & O_TRUNC) || (omode & O_CREAT)) {
//		ERRPRINT(("ntfs_open called with O_TRUNC or O_CREAT flag.\n"));
		return EROFS;
	} 
	
	DEBUGPRINT(FILE, 5, ("ntfs_open called on vnode %Ld, open mode = 0x%x.\n", v->vnid, omode));

	// We don't need no cookie
	*cookie = NULL;
	
	DEBUGPRINT(FILE, 5, ("ntfs_open exit.\n"));
	
	return B_NO_ERROR;
}

// This function is called whenever the fsil wants to read a file. Pretty simple stuff
int ntfs_read(void *ns, void *node, void *cookie, off_t pos, void *buf, size_t *len)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	size_t 			length_to_read;
	data_attr 		*data;
	status_t 		err = B_NO_ERROR;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(FILE, 5, ("ntfs_read called on vnode %Ld, starting pos %Ld length %ld.\n", v->vnid, pos, *len));

	if(v->is_dir) {
		ERRPRINT(("ntfs_read called on directory.\n"));
		*len = 0;
		return EISDIR;
	}

	if(pos < 0) {
		*len = 0;
		return EINVAL;
	}

	// Find the primary data attribute
	data = ntfs_find_attribute(v, ATT_DATA, 0);	
	if(!data) {
		ERRPRINT(("ntfs_read couldn't find primary data attribute on vnode %Ld.\n", v->vnid));
		*len = 0;
		goto out;
	}

	// Check to make sure the starting position is in the file
	if(data->length <= pos) {
		*len = 0;
	} else {
		// Make sure the passed read_len isn't too big.	
		length_to_read = min(*len, data->length - pos);
		DEBUGPRINT(FILE, 7, ("\twill read %ld bytes.\n", length_to_read));
		
		if(data->stored_local) {	
			// the data is contained here
			DEBUGPRINT(FILE, 7, ("\treading local data.\n"));
			memcpy(buf, (char *)data->data + pos, length_to_read);					
			*len = length_to_read;
		} else {
			// read it
			DEBUGPRINT(FILE, 7, ("\treading from extentnt"));
			err = ntfs_read_extent(ntfs, &data->extent, buf, pos, length_to_read, len);
		}
	}

out:
	DEBUGPRINT(FILE, 5, ("ntfs_read exit. Read %ld bytes.\n", *len));
	
	return err;
}

// This function returns information about the particular file.
int	ntfs_rstat(void *ns, void *node, struct stat *st)
{
	ntfs_fs_struct 	*ntfs 	= ns;
	vnode			*v 		= node;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(FILE, 6, ("ntfs_rstat entry called on vnode %Ld.\n", v->vnid));

	st->st_dev = ntfs->nsid;
	st->st_ino = v->vnid;
	st->st_nlink = v->hard_link_count;
	st->st_uid = 0; 
	st->st_gid = 0;
	st->st_rdev = 0;
	st->st_blksize = ntfs->cluster_size*16;
	{
		std_info_attr *std_info;
		
		std_info = ntfs_find_attribute(v, ATT_STD_INFO, 0);
		if(!std_info) {
			ERRPRINT(("ntfs_rstat had error loading STD_INFO attribute for vnid %Ld.\n", v->vnid));
			return EIO;		
		}
		
		st->st_atime	= std_info->last_access_time; // access time
		st->st_mtime	= std_info->last_modification_time; // modify time
		st->st_ctime	= std_info->last_modification_time; // change time ? How is this different from modify time?
		st->st_crtime	= std_info->file_creation_time; // create time
	}
	
	if(!v->is_dir) {
		data_attr *data;

		// This isn't a directory, so find the length of the primary data attribute		
		data = ntfs_find_attribute(v, ATT_DATA, 0);		

		if(data) {
			st->st_size = data->length;
		} else {
			ERRPRINT(("ntfs_rstat vnode %Ld doesn't have data attribute.\n", v->vnid));
			st->st_size = 0;
		}
	} else {
		index_alloc_attr *index_alloc;
		
		// This is a directory, so find the length of the index allocation attribute.
		// If there isn't one, the directory must be stored in the INDEX_ROOT attribute,
		// so return length 0.
		index_alloc = ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);	
		if(index_alloc) {
			st->st_size = index_alloc->length;
		} else {
			st->st_size = 0;
		}
	}		
	
	// Set the permissions.
	if(v->is_dir) {
		st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH | S_IFDIR;
	} else {
		st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IFREG;
	}

	DEBUGPRINT(FILE, 6, ("ntfs_rstat exit.\n"));

	return B_NO_ERROR;
}

status_t frag_disk(ntfs_fs_struct *ntfs, uint32 pattern, bool random)
{
	int fd; // we'll need to open the disk again
	data_attr *data;
	status_t err;
	uint32 *buf = NULL;
	uint64 file_pos = 0;
	uint64 file_block;
	ssize_t bytes_read;

	if(random)
		srand(time(NULL)|1);

	// get the data attribute for the BITMAP file
	data = ntfs_find_attribute(ntfs->BITMAP, ATT_DATA, 0);
	if(!data) {err = EIO;goto error;}

	// Open the volume again with O_RDWR
	fd = open(ntfs->device_name, O_RDWR);
	if(fd < 0) {err = EIO;goto error1;}
		
	// get a buffer to read the bitmap into
	buf = ntfs_malloc(ntfs->cluster_size);
	if(!buf) {err = ENOMEM;goto error1;}
	
	// Scan through the bitmap file
	while(file_pos < data->length) {
		if(((file_pos % ntfs->cluster_size) == 0)) {			
			// we have just entered a new file block
			if(file_pos != 0) {
				// write the old block out first
				err = ntfs_get_block_from_extent(&data->extent, &file_block, (file_pos - 1)/ ntfs->cluster_size);
				if(err < B_NO_ERROR) goto error2;
				write_pos(fd, file_block * ntfs->cluster_size, buf, ntfs->cluster_size);
			}
			err = ntfs_get_block_from_extent(&data->extent, &file_block, (file_pos)/ ntfs->cluster_size);
			if(err < B_NO_ERROR) goto error2;
			bytes_read = read_pos(fd, file_block * ntfs->cluster_size, buf, ntfs->cluster_size);
			if(bytes_read != ntfs->cluster_size) {err = EIO;goto error2;}	
		}
		
		// write the mask out
		if(random) {
			pattern = rand();
		}
		buf[(file_pos % ntfs->cluster_size) / 4] |= pattern;
		
		file_pos += 4;
	}

	// write the last block out
	err = ntfs_get_block_from_extent(&data->extent, &file_block, (file_pos)/ ntfs->cluster_size);
	if(err < B_NO_ERROR) goto error2;
	write_pos(fd, file_block * ntfs->cluster_size, buf, ntfs->cluster_size);

	err = B_NO_ERROR;
error2:
	if(buf) ntfs_free(buf);
error1:
	close(fd);
error:
	DEBUGPRINT(FS, 4, ("leaving frag_disk with error code %ld\n", err));
	return err;
}
