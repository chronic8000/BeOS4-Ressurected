#include "cifs_globals.h"
#include "dir_entry.h"
#include "netio.h"
#include "packet.h"
#include "vcache.h"

#define CREATE_FILE_DEBUG 200

status_t
create_file_send(nspace *vol, vnode *dir, const char *name,
		int omode, int perms, uint32 *cur_time)
{
	status_t 	err;
	packet		*pkt=NULL;
	uchar		wordcount = 3;
	ushort		DesiredAccess;
	ushort		SearchAttributes;
	ushort		bytecount = 0;
	
	iovec		vector[2];
	nb_header_t	nb_header;
	uchar		*hldr;
	uchar		*buf;
	char		*new_name=NULL;



	DPRINTF(CREATE_FILE_DEBUG, ("create_file_send  entering\n"));
	
	err = get_full_name(vol, dir, name, & new_name);
	if (err != B_OK) goto exit1;
	bytecount = strlen(new_name) + 1 + 1;
	
	err = new_smb_packet(vol, &pkt,	SMB_COM_CREATE_NEW);
	if (err != B_OK) goto exit2;
	
	*cur_time = real_time_clock();
	
	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, 0);
	insert_ulong(pkt, *cur_time);
	insert_ushort(pkt, bytecount);
	insert_uchar(pkt, 0x04);
	insert_string(pkt, new_name);

	err = SendSMBPacket(vol, pkt);

	free_packet(& pkt);
exit2:
	FREE(new_name);
exit1:
	DPRINTF(CREATE_FILE_DEBUG, ("create_file_send exiting with result %s\n", strerror(err)));
	return err;
}



status_t
create_file_recv(nspace *vol, vnode *dir, filecookie *cookie) {

	status_t	result;
	uchar		wordcount, dumbuchar;
	packet		*pkt=NULL;	
		
	ushort		fid;
	
	DPRINTF(CREATE_FILE_DEBUG, ("recv_open_file entering \n"));
	
	
	result = RecvSMBPacket(vol, & pkt, SMB_COM_CREATE_NEW);
	if (result != B_OK) {
		// I've seen servers that return FILE_NOT_FOUND when you try
		// to create new files in a read-only dir.  So, force errors
		// to this.
		if (result == ENOENT)
			result = EACCES;
		goto exit1;	
	}
	
	remove_uchar(pkt, & wordcount);
	remove_ushort(pkt, & cookie->fid);
	
	free_packet(& pkt);
	
exit1:
	DPRINTF(CREATE_FILE_DEBUG, ("create_file_recv exiting with result %d \n", result));
	return result;
}
	
status_t
create_file(nspace *vol, vnode *dir, const char *name,
		int omode, int perms, vnode **node, vnode_id *vnid, filecookie *cookie) {

	status_t		result;
	uint32			cur_time;
	int_dir_entry	*entry=NULL;

	vnode_id		tmp_vnid=0;
	
	DPRINTF(CREATE_FILE_DEBUG, ("create_file entering\n"));
	
	result = create_file_send(vol, dir, name, omode, perms, & cur_time);
	if (result != B_OK) {
		DPRINTF(-1, ("error on create_file_send\n"));
		return result;
	}
	
	result = create_file_recv(vol, dir, cookie);
	if (result != B_OK) {
		DPRINTF(-1, ("error on create_file_recv\n"));
		return result;
	}
	
	// Now we need to create the dir entry.
	MALLOC(entry, int_dir_entry*, sizeof(int_dir_entry));
	if (entry == NULL) {
		return ENOMEM;
	}
	
	strcpy(entry->filename, name);
	entry->mode = S_IWUSR | S_IWGRP | S_IWOTH | S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
	entry->uid = geteuid();
	entry->gid = getegid();
	entry->crtime = cur_time;
	entry->atime = cur_time;
	entry->mtime = cur_time;
	entry->ctime = cur_time;
	entry->size = 0;
	entry->blksize = vol->io_size;
	entry->nlink = 1;
	
	entry->vnid = generate_unique_vnid(vol);
	*vnid = entry->vnid;
	
	
	result = add_to_vcache(vol, *vnid, name, dir->vnid, 0);
	if (result != B_OK) {
		DPRINTF(-1, ("create_file: couldnt add %s to vcache\n", name));
		return B_ERROR;
	}
	
	result = add_to_dir(dir, entry);
	if (result != B_OK) {
		debug(-1, ("create_file: couldnt add %s to skiplist\n", name));
		return result;
	}

	result = form_vnode(vol, dir, name, *vnid, node);
	if (result != B_OK) {
		DPRINTF(-1, ("create_file: form_vnode failed\n"));
		return B_ERROR;
	}
	
	result = new_vnode(vol->nsid, *vnid, *node);
	if (result != B_OK) {
		DPRINTF(-1, ("create_file: new_vnode failed\n"));
		return B_ERROR;
	}
				

	DPRINTF(CREATE_FILE_DEBUG, ("create_file exiting with result %d %s\n", result, strerror(result)));
	return result;
}


