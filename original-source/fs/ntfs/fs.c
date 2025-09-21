#include <scsi.h>

#include "ntfs.h"
#include "attributes.h"
#include "io.h"
#include "util.h"


static status_t ntfs_load_uppercase_table(ntfs_fs_struct *ntfs)
{
	data_attr *data;
	size_t bytes_read;
	size_t toread;
	status_t err = B_NO_ERROR;
	int i;

	// get the data attribute for the UpCase file
	data = ntfs_find_attribute(ntfs->UpCase, ATT_DATA, 0);
	if(!data) {
		err = EIO;
		goto out;
	}
	
	toread = min(data->length, 64*1024*2);

	// Read in the raw table
	err = ntfs_read_extent(ntfs, &data->extent, ntfs->upcase,
		0, toread, &bytes_read);
	if(err < B_NO_ERROR) goto out;
	if(bytes_read != toread) err = EIO;

#if __BIG_ENDIAN	
	// Byte-swap, if necessary
	for(i=0; i<64*1024; i++) {
		ntfs->upcase[i] = B_LENDIAN_TO_HOST_INT16(ntfs->upcase[i]);
	}
#endif

out:
	return err;
}

static uint8 ntfs_count_0s_in_byte(uint8 byte, uint8 bits_from_left)
{
	int i;
	uint8 count = 0;
	// This table stores the number of zeros in a particular nibble
	static uint8 bit_table[] = {
		4, 3, 3, 2, 3, 2, 2, 1, // 0 - 7 
		3, 2, 2, 1, 2, 1, 1, 0 // 8 - 15
	};
	
	if(bits_from_left < 8) {
		// If we're counting something less than 8 bits,
		// use the traditional shift and count method
		for(i=0; i<bits_from_left; i++) {
			if(!(byte & 0x80)) count++;
			byte = byte << 1; 
		}
	} else {
		// We're counting for the whole byte
		count += bit_table[byte & 0x0F];
		count += bit_table[(byte & 0xF0) >> 4];			
	}	
	
	return count;
}

static status_t ntfs_calculate_free_space(ntfs_fs_struct *ntfs)
{
	data_attr *data;
	status_t err;
	uint8 *buf = NULL;
	off_t file_pos = 0;
	size_t bytes_read;

	// get the data attribute for the BITMAP file
	data = ntfs_find_attribute(ntfs->BITMAP, ATT_DATA, 0);
	if(!data) {
		err = EIO;
		goto error;
	}

	// get a buffer to read the bitmap into
	buf = ntfs_malloc(ntfs->cluster_size);
	if(!buf) {
		err = ENOMEM;
		goto error;
	}
	
	// Scan through the bitmap file
	while(file_pos < data->length) {
		if((file_pos % ntfs->cluster_size) == 0) {			
			// we have just entered a new file block
			err = ntfs_read_extent(ntfs, &data->extent, buf,
				file_pos, min(data->length - file_pos, ntfs->cluster_size), &bytes_read);
			if(err < B_NO_ERROR) goto error1;
			if(bytes_read != min(data->length - file_pos, ntfs->cluster_size)) {
				err = EIO;
				goto error1;
			}	
		}
		
		ntfs->free_blocks += ntfs_count_0s_in_byte(buf[file_pos % ntfs->cluster_size],
			min((ntfs->nbb.total_sectors/ntfs->nbb.sectors_per_cluster) - (file_pos * 8), 8));
		
		file_pos++;
	}

	err = B_NO_ERROR;
error1:
	if(buf) ntfs_free(buf);
error:
	return err;
}

// Called whenever the filesystem should be mounted.
int	ntfs_mount(nspace_id nsid, const char *devname, ulong flags, void *parms, size_t len, void **data, vnode_id *vnid)
{
	ntfs_fs_struct *ntfs;
	vnode *newNode;
	device_geometry geo;
	status_t err = B_NO_ERROR;

#ifdef USE_DMALLOC
	DEBUGPRINT(FS, 4, ("ntfs checking memory usage to see if there are any unfreed blocks.\n"));
	check_mem();
	DEBUGPRINT(FS, 4, ("ntfs memory check complete.\n"));
#endif	

#ifdef ENABLE_DPRINTF
	{
		bool old_state;
		old_state = set_dprintf_enabled(1); 
		DEBUGPRINT(FS, 4, ("ntfs_mount set_dprintf_enabled(1) = %d.\n", old_state));
	}
#endif

#ifdef LOAD_SYM
	// Try to load the driver symbols. 
	DEBUGPRINT(FS, 4, ("ntfs_mount loading symbols.\n"));

	if(load_driver_symbols("ntfs")==B_ERROR) {
		DEBUGPRINT(FS, 4, ("ntfs_mount Error loading driver symbols.\n"));
	} else {
		DEBUGPRINT(FS, 4, ("ntfs_mount Driver symbols loaded.\n"));
	}
#endif
	
	DEBUGPRINT(FS, 4,("ntfs_mount entry, args: (nsid=%d, devname='%s'", (int)nsid, devname));
	DEBUGPRINT(FS, 4, (", flags=%lx, size=%ld)\n",flags, len));

	// Make a new object
	ntfs = create_ntfs_fs_struct();
	if(!ntfs) {
		ERRPRINT(("ntfs_mount error creating ntfs struct.\n"));
		err = ENOMEM;
		goto error;
	}
	
	DEBUGPRINT(FS, 5, ("ntfs_mount just created ntfs struct.\n"));
		
	ntfs->device_name = (char *)ntfs_malloc(strlen(devname)+1);
	if(!ntfs->device_name) {
		ERRPRINT(("ntfs_mount couldn't alloc mem for device name.\n"));
		err = ENOMEM;
		goto error1;
	}
	strcpy(ntfs->device_name, devname);
	ntfs->nsid=nsid;

	// open read-only
	DEBUGPRINT(FS, 5, ("ntfs_mount opening device read-only\n"));
	if ((err = (ntfs->fd = open(ntfs->device_name, O_RDONLY | O_BINARY))) < 0) {
		ERRPRINT(("ntfs_mount unable to open %s\n", ntfs->device_name));
		err = ENODEV;
		goto error1;
	}
	
	// get device characteristics
	if (ioctl(ntfs->fd, B_GET_GEOMETRY, &geo) < 0) {
		struct stat st;
		if ((fstat(ntfs->fd, &st) >= 0) && S_ISREG(st.st_mode)) {
			// support mounting disk images
			geo.bytes_per_sector = 0x200;
			geo.sectors_per_track = 1;
			geo.cylinder_count = st.st_size / 0x200;
			geo.head_count = 1;
			geo.read_only = !(st.st_mode & S_IWUSR);
			geo.removable = true;
			if (ioctl(ntfs->fd, 10000, NULL) < 0) 
				ERRPRINT(("ntfs_mount warning, could not make file uncacheable.\n"));
		} else {
			ERRPRINT(("ntfs_mount error getting device geometry\n"));
			err = EBADF;
			goto error1;
		}
	}
	if ((geo.bytes_per_sector != 0x200) && (geo.bytes_per_sector != 0x400)
		 && (geo.bytes_per_sector != 0x800) && (geo.bytes_per_sector != 0x1000)) {
		ERRPRINT(("ntfs: unsupported device block size (%lu)\n", geo.bytes_per_sector));
		err = EBADF;
		goto error1;
	}
	if (geo.removable) {
		DEBUGPRINT(FS, 4, ("ntfs_mount %s is removable\n", ntfs->device_name));
		ntfs->removable = true;
	}
				
	// Read in the partition info
	if(ntfs_read_bios_block(ntfs) < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount sez must not be a ntfs partition.\n"));
		err = EBADF;
		goto error1;
	}

	// Initialize the cache
	if(ntfs_init_cache(ntfs) < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error initializing cache.\n"));
		err = ENOMEM;
		goto error1;
	}

	err = ntfs_read_vnode(ntfs, FILE_MFT, 1, (void *)&ntfs->MFT);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading $MFT vnode.\n"));
		goto error1;
	}

	err = ntfs_read_vnode(ntfs, FILE_VOLUME, 1, (void *)&ntfs->VOL);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading $VOL vnode.\n"));
		goto error1;
	}

	err = ntfs_read_vnode(ntfs, FILE_BITMAP, 1, (void *)&ntfs->BITMAP);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading $BITMAP vnode.\n"));
		goto error1;
	}
		
	err = ntfs_read_vnode(ntfs, FILE_UPCASE, 1, (void *)&ntfs->UpCase); // load in the upcase file
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading $UpCase vnode.\n"));
		goto error1;
	}

	// Lets load in the lower->uppercase table
	err = ntfs_load_uppercase_table(ntfs);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading uppercase table\n"));
		goto error1;		
	}
			
	// Lets calculate the number of free blocks by reading in the bitmap
	err = ntfs_calculate_free_space(ntfs);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error calculating free space\n"));
		goto error1;		
	}
			
	// Ok, now lets read in the root inode, just to be nice and get the inode list going.
	err = ntfs_read_vnode(ntfs, FILE_ROOT, 1, (void *)&newNode);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount error loading root directory vnode.\n"));
		goto error1;
	}
	
	err = new_vnode(nsid, FILE_ROOT, newNode);	
	if (err < B_NO_ERROR) {
		ERRPRINT(("ntfs_mount new_vnode returned error.\n"));
		goto error1;
	}
	
	*data=ntfs;
	*vnid=FILE_ROOT;
	DEBUGPRINT(FS, 4, ("ntfs mount complete. fs struct @ 0x%x\n", (unsigned int)ntfs));
	
	return B_NO_ERROR;
	
error1:
	if(ntfs) remove_ntfs_fs_struct(ntfs);
error:
	return err;	
	
}

// This function is called by the fsil whenever it needs to bring a vnode into existance for
// any reason that requires one. I use it as an opportunity to add the vnode to the global vnode list.
// It is called when I use get_vnode() in the fs_walk() function, so it is reentrant.
int ntfs_read_vnode(void *ns, vnode_id vnid, char r, void **node)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode *v = NULL;
	status_t err = B_NO_ERROR;
	
	TOUCH(ntfs);

	DEBUGPRINT(VNODE, 5, ("ntfs_read_vnode reading vnode %Ld, reentry = %d.\n", vnid, r));
	
	// see if it's the vnodes we loaded at mount time
	// If it is still mount time, these should be null, so it'll load later
	switch(vnid) {
		case FILE_MFT:
			v = ntfs->MFT;
			break;
		case FILE_VOLUME:
			v = ntfs->VOL;
			break;
		case FILE_BITMAP:
			v = ntfs->BITMAP;
			break;
		case FILE_UPCASE:
			v = ntfs->UpCase;
			break;
		default:
			break;
	}

	*node = NULL;

	// It wasn't one of them
	if(!v) {
		v = (vnode *)ntfs_malloc(sizeof(vnode));
		if(!v) {
			ERRPRINT(("ntfs_read_vnode had problem allocating space for vnode.\n"));
			return ENOMEM;
		}
		// Initialize the vnode
		ntfs_initialize_vnode(v);
		v->vnid = vnid;
		// The next function may need the ntfs->MFT pointer to be valid
		// This is due to the fact that the MFT contains the runlist to get to itself
		// and the ntfs_load_vnode_from_MFT function may need to look in the MFT
		// to load an extended file record. Yucky, but a special case.
		if(vnid == FILE_MFT) *node = v;
		err = ntfs_load_vnode_from_MFT(ntfs, v);	
		if(err < B_NO_ERROR) {
			ERRPRINT(("ntfs_read_vnode had error loading vnode data from the MFT.\n"));
			ntfs_deallocate_vnode(v);
			if(vnid == FILE_MFT) *node = NULL;
		}	
	}
	
	*node = v;
	
	return err;
}

// This function returns information about the filesystem.
int ntfs_rfsstat(void *ns, struct fs_info *info)
{
	ntfs_fs_struct 	*ntfs = ns;
	volume_name_attr *vol;
	
	TOUCH(ntfs);

	DEBUGPRINT(FS, 6, ("ntfs_rfsstat entry.\n"));
								
	info->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY | B_FS_HAS_MIME;
	strcpy(info->device_name,ntfs->device_name);	

	vol = ntfs_find_attribute(ntfs->VOL, ATT_VOLUME_NAME, 0);
	if(!vol || strlen(vol->name)==0) {
		strcpy(info->volume_name, "Untitled NTFS Volume");
	} else {
		strcpy(info->volume_name, vol->name);
	}
		
	strcpy (info->fsh_name,"NTFS");

	info->io_size 		= 16 * ntfs->cluster_size; // size of a compression group
	info->block_size	= ntfs->cluster_size;
	info->total_blocks	= ntfs->nbb.total_sectors/ntfs->nbb.sectors_per_cluster;
	info->free_blocks	= ntfs->free_blocks;
	info->total_nodes = info->free_nodes = 0;

	DEBUGPRINT(FS, 6, ("ntfs_rfsstat exit.\n"));

	return B_NO_ERROR;
}

// This is a no-brainer.
int	ntfs_unmount(void *ns)
{
	ntfs_fs_struct 	*ntfs = ns;

	TOUCH(ntfs);

	remove_ntfs_fs_struct(ntfs);
	DEBUGPRINT(FS, 4, ("ntfs unmounted. fs struct @ 0x%x.\n", (unsigned int)ntfs));
	
#ifdef USE_DMALLOC
	DEBUGPRINT(FS, 4, ("ntfs checking memory usage to see if there are any leftover blocks.\n"));
	check_mem();
	DEBUGPRINT(FS, 4, ("ntfs memory check complete.\n"));
#endif			
		
	return B_NO_ERROR;
}

// This function is called whenever the fsil figures it doesn't need a vnode anymore.
// The vnode is supposed to be deleted.
int	ntfs_write_vnode(void *ns, void *node, char r)
{
	ntfs_fs_struct 	*ntfs = ns;
	vnode			*v = node;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(VNODE, 5, ("ntfs_write_vnode called on node %Ld, reentry = %d.\n", v->vnid, r));

	// If it's the root vnode, the MFT, or a directory, don't delete it.
	// They will get deleted at fs unmount.
	if((v->vnid!=FILE_MFT) && (v->vnid!=FILE_VOLUME) && (v->vnid!=FILE_BITMAP) && (v->vnid!=FILE_UPCASE)) {
		ntfs_deallocate_vnode(v);
	}
	
	return B_NO_ERROR;
}
