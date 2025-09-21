#include "cifs_globals.h"
#include "cifs_ops.h"
#include "vcache.h"
#include "dir_entry.h"
#include "trans2.h"
#include "packet.h"


#define CREATE_DIR_DEBUG 0

status_t
create_dir_send(nspace *vol, vnode *dir, const char *name, int perms)
{
	status_t	err;
	uchar		wordcount;
	ushort		bytecount;
	packet		*pkt=NULL;
	char		*new_name=NULL;
	


	DPRINTF(CREATE_DIR_DEBUG, ("create_dir_send  entering\n"));
	
	err = get_full_name(vol, dir, name, & new_name);
	
	wordcount = 0;
	bytecount = strlen(new_name) + 1 + 1;
	
	err = new_smb_packet(vol, & pkt, SMB_COM_CREATE_DIRECTORY);
	if (err != B_OK) goto exit2;

	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, bytecount);
	insert_uchar(pkt, 0x04); // buffer format
	insert_string(pkt, new_name);
		
	
	err = SendSMBPacket(vol, pkt);
	
	free_packet(& pkt);
exit2:
	FREE(new_name);	
exit1:		
	DPRINTF(CREATE_DIR_DEBUG, ("create_dir_send exiting with result %s\n", strerror(err)));
	return err;
}


status_t
create_dir_recv(nspace *vol, vnode *dir) {

	status_t 	result;
	packet 		*pkt=NULL;
	
	
	DPRINTF(CREATE_DIR_DEBUG, ("create_dir_recv entering \n"));
	
	result = RecvSMBPacket(vol, & pkt, SMB_COM_CREATE_DIRECTORY);
	if (result != B_OK) {
		// I've seen servers that return FILE_NOT_FOUND when you try
		// to create new within a read-only dir.  So, force errors
		// to this.
		if (result == ENOENT)
			result = EACCES;
	}
	

exit2:
	if (result == B_OK)
		free_packet(& pkt);
exit1:
	DPRINTF(CREATE_DIR_DEBUG, ("create_dir_recv exiting with result %d \n", result));
	return result;
}
	
status_t
create_dir(nspace *vol, vnode *dir, const char *name, int perms) {

	int result = B_ERROR;
	uint32			cur_time;
	char			*hldr;
	int_dir_entry	*entry;
	
	vnode_id tmp_vnid;

	DPRINTF(CREATE_DIR_DEBUG, ("create_dir entering\n"));
	
	result = create_dir_send(vol, dir, name, perms);
	if (result != B_OK) {
		debug(-1, ("error on create_dir_send\n"));
		return result;
	}
	
	result = create_dir_recv(vol, dir);
	if (result != B_OK) {
		debug(-1, ("error on create_dir_recv\n"));
		return result;
	}
	
	// If we don't already have the contents of this new file's directory,
	// then we just go grab the whole thing from the server... 
	if (dir->contents == NULL) {
		result = update_entries(vol, dir, true);
		if (result != B_OK) {
			debug(-1, ("update_entries failed in create_dir"));
			return result;
		}
	} else {
	// Ok, so we don't have it.  We add this file's info into it's directory
	// dir_contents, then call form_vnode, just like we would for read_vnode

		// Now we need to create the dir entry.
		MALLOC(entry, int_dir_entry*, sizeof(int_dir_entry));
		if (entry == NULL) {
			return ENOMEM;
		}
		
		strncpy(entry->filename, name, 255);
		entry->mode = S_IWUSR | S_IWGRP | S_IWOTH;
		entry->mode |= S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
		entry->mode |= S_IXUSR | S_IXGRP | S_IXOTH;
		entry->uid = geteuid();
		entry->gid = getegid();

		cur_time = real_time_clock();
		entry->crtime = cur_time;
		entry->atime = cur_time;
		entry->mtime = cur_time;
		entry->ctime = cur_time;
		entry->size = 0;
		entry->blksize = vol->io_size;
		entry->nlink = 1;

		entry->vnid = generate_unique_vnid(vol);
		result = add_to_vcache(vol, entry->vnid, name, dir->vnid, 1);
		if (result != B_OK) {
			debug(-1, ("create_dir : couldnt add %s to vcache\n", name));
			return B_ERROR;
		}
		
		result = add_to_dir(dir, entry);
		if (result != B_OK) {
			debug(-1, ("create_dir : couldnt add %s to skiplist\n", name));
			return result;
		}
		


		DPRINTF(CREATE_DIR_DEBUG, ("created a directory\n"));
		return B_OK;
	}	
	

	DPRINTF(CREATE_DIR_DEBUG, ("create_dir exiting well with result %d\n", result));
	return result;
}
