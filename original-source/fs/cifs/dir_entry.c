#include "cifs_globals.h"
#include "vcache.h"
#include "packet.h"
#include "netio.h"
#include "trans2.h"
#include "skiplist.h"
#include "cifs_utils.h"

#define DIR_ENTRY_DEBUG 200

status_t fill_vnode(nspace *vol, vnode *dir_vnode, vnode *this_vnode);
status_t get_entry(nspace *vol, vnode *dir, struct dirent *extern_entry, dircookie *cookie);
status_t update_entries(nspace *vol, vnode *vn, bool force);
status_t form_dir(nspace *vol, vnode *dir, uchar *server_dir, ushort num_entries, bool remove_previous);
status_t get_dir(nspace *vol, vnode *dir, const char *FindString);
status_t get_remaining_dir(nspace *vol, vnode *dir, ushort SearchID);
status_t close_dir_search(nspace *vol, ushort SearchID);
status_t oldstyle_rfsstat_send(nspace *vol);
status_t oldstyle_rfsstat_recv(nspace *vol);
status_t oldstyle_rfsstat(nspace *vol);


// Functions for use with the skiplists

int _entry_compare(void *_keya, void *_keyb) {
	
	int_dir_entry *keya = (int_dir_entry*)_keya;
	int_dir_entry *keyb = (int_dir_entry*)_keyb;
	int err;	
	
	if ((keya == NULL) || (keyb == NULL)) {
		debug(-1, ("_entry_compare given a null key : a %x b %x\n", keya, keyb));
		return B_ERROR;
	}
	
	//dprintf("_entry_compare vnida is %Lx vnidb is %Lx\n", keya->vnid, keyb->vnid);


	if (keya->vnid < keyb->vnid)
		err = -1;
	if (keya->vnid == keyb->vnid)
		err = 0;
	if (keya->vnid > keyb->vnid)
		err = 1;
	
	//dprintf("_entry_compare returning %x\n", err);
	return err;	
}

void
_entry_free(void *key) {

	if (key != NULL) {
		FREE(key);
	} else {
		debug(-1,("_entry_free given NULL key\n"));
	}
}

status_t
fill_vnode(nspace *vol, vnode *dir_vnode, vnode *this_vnode) {

	int_dir_entry	*entry=NULL;
	
	debug(DIR_ENTRY_DEBUG, ("fill_vnode entering for file %Lx %s\n", this_vnode->parent_vnid, this_vnode->name));

	if (! this_vnode) {
		DPRINTF(-1, ("fill_vnode exiting cause this_vnode is NULL\n"));
		return B_ERROR;
	}
	
	if (dir_vnode->contents == NULL) {
		DPRINTF(-1, ("remove_entry calling update_entries 1\n"));
		update_entries(vol, dir_vnode, false);
	}
	
	entry = find_in_dir(dir_vnode, this_vnode->vnid);
	if (entry == NULL) {
		debug(-1, ("fill_vnode exiting cause no entry found NULL\n"));
		return ENOENT;
	}		

	this_vnode->mode = entry->mode;
	this_vnode->uid = entry->uid;
	this_vnode->gid = entry->gid;
	this_vnode->crtime = entry->crtime;
	this_vnode->atime = entry->atime;
	this_vnode->mtime = entry->mtime;
	this_vnode->ctime = entry->ctime;
	this_vnode->size = entry->size;
	this_vnode->blksize = entry->blksize;

	this_vnode->nlink = 1;  // ???

	debug(DIR_ENTRY_DEBUG, ("fill_vnode exiting\n"));
	return B_OK;

}


status_t
get_entry(nspace *vol, vnode *dir, struct dirent *extern_entry, dircookie *cookie) {

	int_dir_entry *entry=NULL;
	
	if (dir->contents == NULL) {
		debug(DIR_ENTRY_DEBUG, ("dir->contents is null\n"));
	} else {
		debug(DIR_ENTRY_DEBUG, ("dir does have contents\n"));
		debug(DIR_ENTRY_DEBUG, ("\t NumInSL is %d\n", NumInSL(dir->contents)));
	}


	if (! extern_entry) {
		debug(-1,("no extern entry\n"));
		return B_ERROR;
	}	
	if (cookie == NULL) {
		debug(-1,("get_entry given null cookie\n"));
		return B_ERROR;
	}	



	entry = next_in_dir(dir, cookie);	
	if (entry == NULL) {
		debug(DIR_ENTRY_DEBUG, ("next_in_dir returned NULL\n"));
		return ENOENT;
	}
	cookie->previous = entry->vnid;
	
	strncpy(extern_entry->d_name, entry->filename, 255);
	extern_entry->d_ino = entry->vnid;
	extern_entry->d_reclen = strlen(extern_entry->d_name) + 1;
	extern_entry->d_dev = vol->nsid;

	cookie->index++;


	return B_OK;
}


status_t
update_entries(nspace *vol, vnode *vn, bool force) {

	status_t err;
	uint32 now;
	char		*findstring=NULL;
	
	debug(DIR_ENTRY_DEBUG, ("update_entries entering"));
	
	if (! (vn->mode & S_IFDIR)) {
		debug(-1, ("Update_entries called on non-dir or NULL skiplist\n"));
		return B_ERROR;
	}

	// Explanation : As of Jan 25, 1999, the libraries make many, many
	// calls to walk through directories.  This will give a large
	// performance increaase traded off with a N second delay in
	// coherency.  When the libs handle network filesystems better,
	// this _should_ be taken out.
	now = real_time_clock();
	if (! force) {
		if (((now - vn->last_update_time) < (CIFS_UPDATE_GRANULARITY * 1000000)) &&
			(vn->contents != NULL))
			debug(DIR_ENTRY_DEBUG, ("update_entries delaying\n"));
			return B_OK;
	}
	vn->last_update_time = now;

	err = get_full_name(vol, vn, "*", & findstring);
	if (err != B_OK) {
		debug(-1, ("update_entries failed on get_full_name\n"));
		return err;
	}

	err = get_dir(vol, vn, findstring);
	if (err != B_OK) {
		debug(-1, ("update_entries failed on get_dir : looking for %s\n", findstring));
	}
		
	FREE(findstring);
	return B_OK;
	
}


status_t
get_dir(nspace *vol, vnode *dir, const char* FindString) {
			
	status_t	err;
	packet	*SendSetup = NULL;
	packet	*SendParam = NULL;
	ushort 	SearchAttributes;
	ushort	SearchCount;
	ushort	Flags;
	ushort	InformationLevel;
	
	packet	*RecvParam=NULL;
	packet	*RecvData=NULL;
	ushort	SearchID;
	ushort	Entries;	
	ushort 	EndOfSearch;
	ushort	dumbushort;
	ushort	LastNameOffset;	

	err = init_packet(& SendSetup, -1);
	if (err != B_OK) goto exit1;
	err = init_packet(& SendParam, -1);
	if (err != B_OK) goto exit2;
	
	set_order(SendSetup, false);
	set_order(SendParam, false);
		
	// SendSetup
	insert_ushort(SendSetup, (ushort) TRANS2_FIND_FIRST2);
	
	
	// SendParam
	SearchAttributes = (FILE_ATTR_SYSTEM | FILE_ATTR_HIDDEN | FILE_ATTR_DIRECTORY);
	SearchCount = 512; // If this doesnt get all the files, we loop using get_remaining_dir
	Flags = 0x2;  // XXX xxx need to check this
	InformationLevel = SMB_FIND_FILE_FULL_DIRECTORY_INFO;
	
	insert_ushort(SendParam, 	SearchAttributes);
	insert_ushort(SendParam, 	SearchCount);
	insert_ushort(SendParam, 	Flags);
	insert_ushort(SendParam, 	InformationLevel);
	insert_ulong(SendParam,		0);//SearchStorageType
	insert_string(SendParam,	FindString);


	err = send_trans2_req(vol, SendSetup, SendParam, NULL);
	if (err != B_OK) goto exit3;
	
	
	err = recv_trans2_req(vol, NULL, & RecvParam, & RecvData);
	if (err == ENOENT) {
		DPRINTF(-1, ("Those Wacky Samba People - got ENOENT on a dir listing.\n"));
		// Samba servers return FILE_NOT_FOUND when you look for * in an empty
		// directory.  You know the great thing about standards ...
		// The awful thing about this is that if the directory itself doesn't
		// exist, they also return ENOENT.
		err = B_OK;
		goto exit3;
	}
	if (err != B_OK) {
		DPRINTF(-1, ("get_dir -> error recv_trans2_req : %d %s\n", err, strerror(err)));
		goto exit3;
	}
	
	remove_ushort(RecvParam,	& SearchID);
	remove_ushort(RecvParam,	& Entries);
	remove_ushort(RecvParam,	& EndOfSearch);
	remove_ushort(RecvParam,	& dumbushort);
	remove_ushort(RecvParam,	& LastNameOffset);
	
	/* dprintf("get_dir dump :\nSearchID		%d\nEntries		%d\nEndOfSearch	%d\ndumbushort		%d\nLastNameOffset	%d\n",
			SearchID, Entries, EndOfSearch, dumbushort, LastNameOffset);
	*/

	err = form_dir(vol, dir, (uchar*)RecvData->buf, Entries, true);
	if (err != B_OK) {
		DPRINTF(-1, ("form_dir failed in get_dir\n"));
		goto exit5;
	}

	if (! EndOfSearch) {
		err = get_remaining_dir(vol, dir, SearchID);
		if (err != B_OK)
			debug(-1, ("get_dir : get_remaining_entries returning %d %s\n", err, strerror(err)));
	}

	
exit6:

exit5:
	free_packet(& RecvParam);
	free_packet(& RecvData);
exit4:


exit3:
	free_packet(& SendParam);
exit2:
	free_packet(& SendSetup);
exit1:
	return err;
}	
	
	
	
status_t
form_dir(nspace *vol, vnode *dir, uchar *server_dir, ushort num_entries, bool remove_previous) {

	status_t result;
		
	int i;
	uchar *hldr;
	uchar *back;
	
	ulong	nextentryoffset;
	uchar	*nextentryptr = NULL;
	
	
	ulong	Attributes = 0;
	ulong	FileNameLength = 0;
	
	uint64	big_time;
	const uint64	mickysoft_sucks_shit = 11644473600;
	// This aptly named var is the number of seconds between
	// Jan 1st 1601 and Jan 1st 1970
	
	vnode_id	tmp_vnid;
	char		tmp_is_dir;
	
	int_dir_entry *entry = NULL;
	int_dir_entry *prev_entry = NULL;
		
	debug(DIR_ENTRY_DEBUG, ("form_dir_entries entering\n"));
	debug(DIR_ENTRY_DEBUG, ("number of entries is %d\n", num_entries));

	if (remove_previous) {
		if (dir->contents) {
			debug(DIR_ENTRY_DEBUG, ("form_dir calling FreeSL on dir %s\n", dir->name));
			FreeSL(dir->contents);
		}	
		debug(DIR_ENTRY_DEBUG, (" making new skiplist\n"));
		dir->contents = NewSL(_entry_compare, _entry_free, NO_DUPLICATES);
	} else {
		if (dir->contents == NULL) {
			debug(-1, ("form_dir : there arent any dir contents, and you want to add to them. Nope\n"));
			return B_ERROR;
		}
	}

	hldr = server_dir;
	for  (i = 0; i < num_entries; i++) {
	
		MALLOC(entry, int_dir_entry*, sizeof(int_dir_entry));
		if  (entry == NULL) {
			return ENOMEM;
		}
		entry->del = false;

		ulong_from_ptr(& hldr, & nextentryoffset, true);
		nextentryptr = hldr - 4 + nextentryoffset;
		//dprintf("nextentryoffset is %d (dec)\n", nextentryoffset);
		
		hldr += 4; // Skip FileIndex

		// Creation Time
		ulonglong_from_ptr(& hldr, & big_time, true);
		big_time *= 1.0e-7;
		big_time -= mickysoft_sucks_shit;
		entry->crtime = big_time;
		// Access Time
		ulonglong_from_ptr(& hldr, & big_time, true);
		big_time *= 1.0e-7;
		big_time -= mickysoft_sucks_shit;
		entry->atime = big_time;
		// Modification Time
		ulonglong_from_ptr(& hldr, & big_time, true);
		big_time *= 1.0e-7;
		big_time -= mickysoft_sucks_shit;
		entry->mtime = big_time;
		// xxx
		entry->ctime = entry->mtime;
		
		hldr += 8;  // Skipping "last attribute change time", spec p99
	
		// File Size
		longlong_from_ptr(& hldr, & entry->size, true);

		hldr += 8; // Skip allocationsize
		
		// Translate Attributes to mode
		ulong_from_ptr(& hldr, & Attributes, true);
		entry->mode = 0;
		entry->mode = S_IRUSR | S_IRGRP | S_IROTH;
		// | S_IXUSR | S_IXGRP | S_IXOTH;
		if (! (Attributes & FILE_ATTR_READ_ONLY))
			entry->mode |= S_IWUSR | S_IWGRP | S_IWOTH;
		if (Attributes & FILE_ATTR_DIRECTORY) {
			entry->mode |= S_IFDIR;
		} else {
			entry->mode |= S_IFREG;
		}
	
		// Get FileName
		ulong_from_ptr(& hldr, & FileNameLength, true);
		if (FileNameLength > 255) FileNameLength = 255;
		hldr += 4; // Skip EaSize
		memcpy(entry->filename, hldr, FileNameLength);
		// _Have_ to ensure the name is null-terminated.
		entry->filename[FileNameLength] = '\0'; 

		// Wrap to next in list
		hldr = nextentryptr;
	
		if (strcmp(entry->filename, ".") == 0) {
			FREE(entry);
			entry = NULL;
			result = B_OK;
			continue;
		}
		
		if (strcmp(entry->filename, "..") == 0) {
			FREE(entry);
			entry = NULL;
			result = B_OK;
			continue;
		}	
		

		//dprintf("got info for file %s, filelength %d\n", entry->filename, entry->size);

		// Get or Set Vnid
		result = vcache_loc_to_vnid(vol, entry->filename, dir->vnid, & tmp_vnid, & tmp_is_dir);
		if (result == B_OK) {
			entry->vnid = tmp_vnid;		
		} else {
			entry->vnid = generate_unique_vnid(vol);
			DPRINTF(DIR_ENTRY_DEBUG, ("form_dir_entries generated this vnid ->%Lx<-\n", entry->vnid));
			result = add_to_vcache(vol, entry->vnid, entry->filename,
						dir->vnid, (entry->mode & S_IFDIR) ? 1 : 0);

			notify_listener(B_ENTRY_CREATED, vol->nsid, dir->vnid, 0, tmp_vnid, entry->filename);
		}
		
		// Insert into the skiplist
		if (InsertSL(dir->contents, (void*)entry) != TRUE) {
			debug(-1, ("InsertSL failed in form_dir\n"));
			FREE(entry);
			entry = NULL;
		} else {
			//dprintf("just inserted %s %Lx into skiplist\n", entry->filename, entry->vnid);
		}

	}


	debug(DIR_ENTRY_DEBUG, ("form_dir_entries exiting : %d %s\n", result, strerror(result)));
	debug(DIR_ENTRY_DEBUG, ("\t form_dir numinsl is %d \n", NumInSL(dir->contents)));
	return result;
	
}

status_t
get_remaining_dir(nspace *vol, vnode *dir, ushort SearchID) {

	status_t	err;
	packet	*SendSetup = NULL;
	packet	*SendParam = NULL;
	ushort	InformationLevel = SMB_FIND_FILE_FULL_DIRECTORY_INFO;
	ulong	ResumeKey = 0; // xxx
	ushort	Flags = 2+8;  // Close search if end reached, continue search from last entry given
	
	packet	*RecvParam = NULL;
	packet	*RecvData = NULL;
	ushort	Entries;	
	ushort 	EndOfSearch;
	ushort	dumbushort;
	ushort	LastNameOffset;	


	do {
	
		err = init_packet(& SendSetup, -1);
		if (err != B_OK) goto exit1;
		err = init_packet(& SendParam, -1);
		if (err != B_OK) goto exit2;
		
		set_order(SendSetup, false);
		set_order(SendParam, false);
			
		// SendSetup
		insert_ushort(SendSetup, (ushort) TRANS2_FIND_NEXT2);
	
		// SendParam
		insert_ushort(SendParam, 	SearchID);
		insert_ushort(SendParam,	(ushort) 512); // XXX arbitrary
		insert_ushort(SendParam,	InformationLevel);
		insert_ulong(SendParam,		ResumeKey);
		insert_ushort(SendParam,	Flags);
		insert_uchar(SendParam,		0);  // Null String
	
	
		err = send_trans2_req(vol, SendSetup, SendParam, NULL);
		if (err != B_OK) goto exit3;
		
		err = recv_trans2_req(vol, NULL, & RecvParam, & RecvData);
		if (err != B_OK) goto exit3;
		
		remove_ushort(RecvParam,	& Entries);
		remove_ushort(RecvParam,	& EndOfSearch);
		remove_ushort(RecvParam,	& dumbushort);
		remove_ushort(RecvParam,	& LastNameOffset);
			
		err = form_dir(vol, dir, (uchar*)RecvData->buf, Entries, false);
		if (err != B_OK) {
			debug(-1, ("form_dir failed in get_remaining_dir\n"));
			goto exit4;	
		}
		
		free_packet(& SendSetup);
		free_packet(& SendParam);
		free_packet(& RecvParam);
		free_packet(& RecvData);
		

	} while (! EndOfSearch);
	
	/*

	err = close_dir_search(vol, SearchID);
	if (err != B_OK) {
		// So what?
		debug(-1,("close_dir_search failed in get_remaining_dir : %d %s\n", err, strerror(err)));
	}
	*/
	
	
	return err;

	
exit4:
	free_packet(& RecvParam);
	free_packet(& RecvData);
exit3:
	free_packet(& SendParam);
exit2:
	free_packet(& SendSetup);
exit1:
	debug(-1, ("get_remaining_dir failing : %d %s\n", err, strerror(err)));
	return err;	
}



status_t
close_dir_search(nspace *vol, ushort SearchID) {
	status_t err;
	
	packet	*sendpkt = NULL;
	packet	*recvpkt = NULL;
	
	
	err = new_smb_packet(vol, & sendpkt, SMB_COM_FIND_CLOSE2);
	if (err != B_OK) return err;
	insert_uchar(sendpkt, 1); // wordcount
	insert_ushort(sendpkt, SearchID);
	insert_ushort(sendpkt, 0); // bytecount
	
	err = SendSMBPacket(vol, sendpkt);
	free_packet(& sendpkt);
	if (err != B_OK) return err;	
	
	err = RecvSMBPacket(vol, & recvpkt, SMB_COM_FIND_CLOSE2);
	if (err != B_OK) return err;
	
	free_packet(& recvpkt);
	return err;
}


#define RFS_DEBUG 10

status_t
oldstyle_rfsstat_send(nspace *vol)
{
	status_t err;
	packet *pkt = NULL;
	
	err = new_smb_packet(vol, & pkt, SMB_COM_QUERY_INFORMATION_DISK);
	if (err != B_OK)
		goto exit1;

	insert_uchar(pkt,	0); // wordcount
	insert_ushort(pkt,	0); // bytecount
	
	err = SendSMBPacket(vol, pkt);
	if (err != B_OK) {
		debug(RFS_DEBUG, ("oldstyle_rfsstat_send failed on SendSMBPacket %s\n", strerror(err)));
		goto exit1;
	}

	free_packet(&pkt);
exit1:	
	return err;
}


status_t
oldstyle_rfsstat_recv(nspace *vol) {

	status_t result;
	packet *pkt = NULL;
	
	uchar wordcount;
	ushort totalunits;
	ushort blocksperunit;
	ushort blocksize;
	ushort freeunits;
	ushort reserved;
	ushort bytecount;
	


	result = RecvNBPacket(vol, & pkt, (CIFS_RECV_TIMEOUT * 1000));
	if (result != B_OK) {
		//dprintf("rfsstat_recv returning %d:%s\n", result, strerror(result));
		//dprintf("i gave a timeout of %d * 1000\n", CIFS_RECV_TIMEOUT);
		return result;
	}
	
	result = PopSMBHeader(vol, pkt, SMB_COM_QUERY_INFORMATION_DISK, false);
	if (result != B_OK) {
		debug(RFS_DEBUG,("rfsstat_recv (popsmb) returning %d:%s\n", result, strerror(result)));
		goto exit1;
	}
	
	set_order(pkt, false);
	
	remove_uchar(pkt, & wordcount);
	if (wordcount != 5) {
		debug(-1,("got bad wordcount in rfsstat_recv\n"));
		result = B_IO_ERROR;
		goto exit1;
	}
	remove_ushort(pkt, & totalunits);
	remove_ushort(pkt, & blocksperunit);
	remove_ushort(pkt, & blocksize);
	remove_ushort(pkt, & freeunits);
	remove_ushort(pkt, & reserved);
	remove_ushort(pkt, & bytecount);
	
	vol->fss_block_size = blocksize * blocksperunit;
	vol->fss_total_blocks = totalunits;
	vol->fss_free_blocks = freeunits;

exit1:
	free_packet(& pkt);
	return result;
}


status_t oldstyle_rfsstat(nspace *vol) {


	status_t result;
	
	result = oldstyle_rfsstat_send(vol);
	if (result != B_OK) {
		return result;
	}

	return oldstyle_rfsstat_recv(vol);

}
