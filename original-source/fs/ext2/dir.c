#include "ext2.h"
#include "mime_table.h"
#include "perm.h"
#include "io.h"

static status_t ext2_set_mime_type(vnode *v, const char *filename)
{
	struct ext_mime *p;
	int32 namelen, ext_len;

	DEBUGPRINT(ATTR, 7, ("ext2_set_mime_type of (%s) entry\n", filename));

	v->mime = NULL;

	namelen = strlen(filename);

	for (p=mimes;p->extension;p++) {
		ext_len = strlen(p->extension);

		if (namelen <= ext_len)
			continue;

		if (filename[namelen-ext_len-1] != '.')
			continue;
		
		if (!strcasecmp(filename + namelen - ext_len, p->extension))
			break;
	}

	v->mime = p->mime;

	if(v->mime) 
		DEBUGPRINT(ATTR, 8, ("ext2_set_mime_type got type '%s' for file '%s'\n", v->mime, filename));

	return B_OK;
}

// Called by the fsil when the directory needs to be closed.
int	ext2_closedir(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;

	CHECK_FS_STRUCT(ext2, "ext2_closedir");
	CHECK_VNODE(v, "ext2_closedir");
	
	DEBUGPRINT(DIR, 5, ("ext2_closedir called on vnode %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

// The fsil calls fs_free_dircookie whenever it feels that the cookie can be safely
// deleted. Notice that it is seperate from the fs_closedir() call.
int ext2_free_dircookie(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;
	dircookie *d=(dircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_free_dircookie");
	CHECK_VNODE(v, "ext2_free_dircookie");
	CHECK_COOKIE(d, "ext2_free_dircookie");

	DEBUGPRINT(DIR, 5, ("ext2_free_dircookie called on node %Ld.\n", v->vnid));

	// Lookie, free cookie. :)
	free(d);
	
	return B_OK;
}

static int _ext2_opendir(ext2_fs_struct *ext2, vnode *v, dircookie **d)
{
	*d = (dircookie *)malloc(sizeof(dircookie));
	if(!(*d)) 	{
		ERRPRINT(("ext2_opendir couldn't allocate memory for cookie.\n"));
		return ENOMEM;
	}
	
	(*d)->dir_vnode = v->vnid;
	
	// Initialize the cookie to the beginning of the directory.	
	(*d)->next_pos = 0;

	// Mark the vnode as a dir.
	v->is_dir = true;
	
	return B_NO_ERROR;
}

// This function is called whenever a read/write is about to happen on a directory. Allocates a dircookie.
int ext2_opendir(void *ns, void *node, void **cookie)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	dircookie *d;
	status_t err;

	CHECK_FS_STRUCT(ext2, "ext2_opendir");
	CHECK_VNODE(v, "ext2_opendir");

	DEBUGPRINT(DIR, 5, ("ext2_opendir on vnode %Ld.\n", v->vnid));

	LOCKVNODE(v, 1);
	
	if(!S_ISDIR(v->inode.i_mode)) {
		ERRPRINT(("ext2_opendir called on a non-directory.\n"));
		UNLOCKVNODE(v, 1);
		return ENOTDIR;
	}
	
	// Check the permissions on this dir
	if (ext2_check_permission(ext2, v, O_RDONLY) == 0) {
		dprintf("ext2_opendir failed with access\n");
		UNLOCKVNODE(v, 1);
		return EACCES;
	}
	
	err = _ext2_opendir(ext2, v, &d);	

	UNLOCKVNODE(v, 1);
		
	*cookie = d;
	
	return err;
}

// This function grabs 1 or more directory entries from the current directory.
int	ext2_readdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *ent, size_t bufsize)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	dircookie *d=(dircookie *)cookie;

	char *buf;
	ext2_dir_entry *entry;
	ext2_dir_entry_2 *entry2;
	int block_offset;
	uint16 reclen =0;
	bool done;
	int curr_block;
	size_t bufleft = bufsize;
	char *bufpos = (void *)ent;
	struct dirent *tempent = ent;
	uint32 next_pos;

	*num = 0;

	CHECK_FS_STRUCT(ext2, "ext2_readdir");
	CHECK_VNODE(v, "ext2_readdir");
	CHECK_COOKIE(d, "ext2_readdir");

	DEBUGPRINT(DIR, 5, ("ext2_readdir entry on vnode %Ld, output buffer at 0x%x.\n", v->vnid, (int)ent));

	LOCKVNODE(v, 1);

	// Save the cookie's next_pos.
	// This is so if someone calls readdir or rewinddir at the same time, it doesn't get corrupted.
	next_pos = d->next_pos;
	
	do {
		done = false;
		// Find the directory entry
		while(!done) {
			// Check to see if we're at the end of the directory file.
			if(next_pos >= v->inode.i_size) {
				DEBUGPRINT(DIR, 6, ("ext2_readdir has reached the end of the directory file.\n"));
				UNLOCKVNODE(v, 1);
				return B_NO_ERROR;
			}
			
			// Load in the right part of the directory file
			curr_block = next_pos/ext2->info.s_block_size;
			buf = (char *)ext2_get_file_cache_block(ext2, v, curr_block);
			if(!buf) {
				ERRPRINT(("ext2_readdir error reading data from cache.\n"));
				UNLOCKVNODE(v, 1);
				return EIO;
			}
			
			// Now, lets calculate the offset into this block
			block_offset = next_pos % ext2->info.s_block_size;
			
			// Let's overlay the ext2 diretory structure over the buffer
			entry = (ext2_dir_entry *)(buf + block_offset);
			entry2 = (ext2_dir_entry_2 *)(buf + block_offset);
			
			DEBUGPRINT(DIR, 7, ("ext2_readdir at offset %d into file block %d.\n", block_offset, curr_block));
			if(USING_FILETYPES_FEATURE(ext2)) {
				DEBUGPRINT(DIR, 7, ("ext2_readdir looking at dir ent: inode = %ld, rec_len = %d, name_len = %d.\n",
					B_LENDIAN_TO_HOST_INT32(entry2->inode),
					B_LENDIAN_TO_HOST_INT16(entry2->rec_len),
					entry2->name_len));
			} else {
				DEBUGPRINT(DIR, 7, ("ext2_readdir looking at dir ent: inode = %ld, rec_len = %d, name_len = %d.\n",
					B_LENDIAN_TO_HOST_INT32(entry->inode),
					B_LENDIAN_TO_HOST_INT16(entry->rec_len),
					B_LENDIAN_TO_HOST_INT16(entry->name_len)));					
			}							

			if((USING_FILETYPES_FEATURE(ext2) ? B_LENDIAN_TO_HOST_INT32(entry2->inode) : B_LENDIAN_TO_HOST_INT32(entry->inode)) <= 0) {
				DEBUGPRINT(DIR, 6, ("ext2_readdir has found an invalid entry (inode = 0), trying the next one...\n"));
				ext2_release_file_cache_block(ext2, v, curr_block);
			} else {
				// The inode is good.
				done = true;

				// Calculate the record length in the return record
				reclen = 2*sizeof(dev_t) + 2*sizeof(ino_t) + sizeof(unsigned short);
				if(USING_FILETYPES_FEATURE(ext2)) {
					reclen += entry2->name_len + 1;
				} else {
					reclen += B_LENDIAN_TO_HOST_INT16(entry->name_len) + 1;
				}
				if(reclen>bufleft) {
					// not enough space to store this one
					ext2_release_file_cache_block(ext2, v, curr_block);
					UNLOCKVNODE(v, 1);
					if(*num == 0) {
						// We really didn't have enough space to store anything
						ERRPRINT(("ext2_readdir buffer too small to store directory entry.\n"));
						return ENOMEM; // proper error code ?
					} else {
						DEBUGPRINT(DIR, 6, ("ext2_readdir returning a buffer with %ld entries.\n", *num));
						return B_NO_ERROR;
					}
				}
			}
			
			// Move the pointer ahead to the next entry
			next_pos += USING_FILETYPES_FEATURE(ext2) ?
				B_LENDIAN_TO_HOST_INT16(entry2->rec_len) : B_LENDIAN_TO_HOST_INT16(entry->rec_len);
			d->next_pos = next_pos;	
		}
		
		if(USING_FILETYPES_FEATURE(ext2)) {
			// Start filling in the POSIX entry			
			tempent->d_ino = B_LENDIAN_TO_HOST_INT32(entry2->inode);
			tempent->d_reclen = entry2->name_len + 1;
			// Copy the name
			memcpy(tempent->d_name, entry2->name, entry2->name_len);
			tempent->d_name[entry2->name_len] = '\0';
		} else {
			// Start filling in the POSIX entry			
			tempent->d_ino = B_LENDIAN_TO_HOST_INT32(entry->inode);
			tempent->d_reclen = B_LENDIAN_TO_HOST_INT16(entry->name_len) + 1;
			// Copy the name
			memcpy(tempent->d_name, entry->name, B_LENDIAN_TO_HOST_INT16(entry->name_len));
			tempent->d_name[B_LENDIAN_TO_HOST_INT16(entry->name_len)] = '\0';
		}			
		// Move the result buffer up by the length of the last entry
		bufpos += reclen;
		bufleft -= reclen;
		tempent = (struct dirent *)bufpos;
		
		// Give the block back to the cache	
		ext2_release_file_cache_block(ext2, v, curr_block);
		(*num)++;
		if(*num == 1) break; // multiple entries ain't supported. Remove here to fix.
	} while(bufleft>0);
	
	UNLOCKVNODE(v, 1);
	
	DEBUGPRINT(DIR, 6, ("ext2_readdir returning a buffer with %ld entries.\n", *num));
	
	return B_NO_ERROR;

}

// Simply takes the directory back to the beginning.
int	ext2_rewinddir(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	dircookie *d=(dircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_rewinddir");
	CHECK_VNODE(v, "ext2_rewinddir");
	CHECK_COOKIE(d, "ext2_rewinddir");

	DEBUGPRINT(DIR, 5, ("ext2_rewinddir called on vnode %Ld.\n", v->vnid));

	// Someone else may be in readdir, but the position is saved at the beginning
	// of readdir, so we should be ok.
	d->next_pos = 0;

	return B_NO_ERROR;
}

// I hate this function, it's always been a pain. In this one, you are supposed to find the file,
// tell the fsil to load it with get_vnode, and return the vnode id. Easier said than done.
int	ext2_walk(void *ns, void *base, const char *file, char **newpath, vnode_id *vnid)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = base;
	dircookie *cookie;
	void *buf;
	char *bufpos;
	struct dirent *ent;
	bool found;
	long num;
	status_t err;

	CHECK_FS_STRUCT(ext2, "ext2_walk");
	CHECK_VNODE(v, "ext2_walk");

	DEBUGPRINT(DIR, 5, ("ext2_walk on vnode %Ld, looking for file '%s'.\n", ((vnode *)base)->vnid, file));

	// Check the execute bit
	{
		uid_t  uid = geteuid();		

		if(uid != 0) {
			mode_t mode = v->inode.i_mode;
			gid_t  gid = getegid();				

			if(ext2->override_uid) {
				if (uid == ext2->uid)
	        		mode >>= 6;
	    		else if (gid == ext2->gid)
					mode >>= 3;
			} else {
				if (uid == v->inode.i_uid)
	        		mode >>= 6;
	    		else if (gid == v->inode.i_gid)
					mode >>= 3;
			}
								
			if((mode & S_IXOTH) != S_IXOTH)
				return EACCES;		
		}	
	}

/*
	// If it's the dir, just return the right stuff.
	if (!strcmp(".",file))
	{
		vnode *dummy;
		
		*vnid=v->vnid;
		return get_vnode(ext2->nsid,*vnid,(void **)&dummy);
	}
*/
	// Lets open the directory and walk it.
	err = _ext2_opendir(ext2, v, &cookie);
	if (err<B_NO_ERROR) {
		ERRPRINT(("ext2_walk had problem opening directory.\n"));
		return err;
	}

	// Now, lets go get it
	buf=(void*)malloc(1024);
	if(!buf) {
		ERRPRINT(("ext2_walk failed to allocate directory buffer.\n"));
		ext2_closedir(ext2, v, &cookie);
		ext2_free_dircookie(ext2, v, cookie);		
		return ENOMEM;
	}
	
	found = false;
	do {
		int i;
	    num = 0;

		bufpos = buf;
		ent = (struct dirent *)bufpos;
	
		err=ext2_readdir(ext2, v,cookie,&num,ent,1024);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_walk had problem reading directory.\n"));
			ext2_closedir(ext2, v, &cookie);
			ext2_free_dircookie(ext2, v,cookie);
			free(buf);
			return err;
		}

		// Walk through each of the entries
		for(i=0; i<num; i++) {
			ent = (struct dirent *)bufpos;
			DEBUGPRINT(DIR, 6, ("ext2_walk looking at entry '%s'.\n", ent->d_name));
			if(!strcmp(file, ent->d_name)) {
				found = true;
				break;
			}
			bufpos += ent->d_reclen;
		}
    } while ((found==false)&&(num>0));

	ext2_closedir(ext2, v, &cookie);
	ext2_free_dircookie(ext2, v,cookie);

	if (found) {
		struct dirent *entry;
		
		entry = (struct dirent *)bufpos;
		*vnid=entry->d_ino;
		
		DEBUGPRINT(DIR, 6, ("ext2_walk found file at inode %d.\n", (int)entry->d_ino));
		
		// Ok, tell the fsil to load it. This will trigger a call to
		// read_vnode immediately before get_vnode returns.
		{
			vnode *temp_vnode;
			struct stat st;

			// create the vnode, and get it
			err=get_vnode (ext2->nsid,*vnid,(void **)&temp_vnode);
			if(err<B_NO_ERROR) {
				ERRPRINT(("ext2_walk had problem loading the vnode.\n"));
				free(buf);
				return err;
			}
			
			// Sniff the attribute type
			ext2_set_mime_type(temp_vnode, entry->d_name);

			// See if it's a symlink and handle that 			
			ext2_rstat(ext2, temp_vnode, &st);
			if(S_ISLNK(st.st_mode) && newpath) {
				// This must be a symbolic link
				char *temp_char; 
				size_t bufsize = 1024;
				char *np;
				
				temp_char = (char *)malloc(1024);
				if(!temp_char) {
					ERRPRINT(("ext2_walk had problem allocating space for symlink string.\n"));
					free(buf);
					return ENOMEM;
				}
				err=ext2_readlink(ext2, temp_vnode, temp_char, &bufsize);
				if(err<B_NO_ERROR) {
					ERRPRINT(("ext2_walk couldn't read the link.\n"));
					free(temp_char);
					free(buf);
					return err;
				}
				temp_char[bufsize] = '\0' ;
				DEBUGPRINT(DIR, 7, ("ext2_walk just called readlink, returned a string %ld bytes long.\n", bufsize));
				err = new_path(temp_char, &np);
				if(err<B_NO_ERROR) {
					ERRPRINT(("ext2_walk had problem calling new_path.\n"));
					free(temp_char);
					free(buf);
					return err;
				}
				err = put_vnode(ext2->nsid, *vnid);
				if(err<B_NO_ERROR) {
					ERRPRINT(("ext2_walk had problem calling put_vnode.\n"));
					free(temp_char);
					free(buf);
					return err;
				}
				*newpath = np;
				free(temp_char);
			}
		}
	} else {
		DEBUGPRINT(DIR, 6, ("ext2_walk didn't find file.\n"));
	}
			
	free(buf);

	if(found)
		return B_NO_ERROR;
		
	return ENOENT;

}



#ifndef RO
// called when the fsil wants a directory created.
int	ext2_mkdir(void *ns, void *dir, const char *name, int perms)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = dir;

	CHECK_FS_STRUCT(ext2, "ext2_mkdir");
	CHECK_VNODE(v, "ext2_mkdir");

	if(ext2->read_only) {
		ERRPRINT(("ext2_mkdir called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(DIR, 5, ("ext2_mkdir called on node %Ld, ", v->vnid));
	DEBUGPRINT(DIR, 5, ("new directory '%s'.\n", name));
	return B_ERROR;
}

// hmm, wonder what this does? :)
int	ext2_rename(void *ns, void *olddir, const char *oldname,
				void *newdir, const char *newname)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*oldDir = olddir;
	vnode			*newDir = newdir;

	CHECK_FS_STRUCT(ext2, "ext2_rename");
	CHECK_VNODE(oldDir, "ext2_rename");
	CHECK_VNODE(newDir, "ext2_rename");

	if(ext2->read_only) {
		ERRPRINT(("ext2_rename called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(DIR, 5, ("ext2_rename called on node %Ld.\n", oldDir->vnid));
	return B_ERROR;
}

// removes a directory entry. I'm not sure if you're supposed to assume all of the
// file entries in this directory are dead already? Probably not.
int	ext2_rmdir(void *ns, void *dir, const char *name)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = dir;

	CHECK_FS_STRUCT(ext2, "ext2_rmdir");
	CHECK_VNODE(v, "ext2_rmdir");

	if(ext2->read_only) {
		ERRPRINT(("ext2_rmdir called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(DIR, 5, ("ext2_rmdir called on node %Ld.\n", v->vnid));
	return B_ERROR;
}


#endif
