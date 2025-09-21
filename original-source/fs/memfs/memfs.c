/******************************************************************************
	memfs.c

	This file contains the functions exported by the memfs pseudo filesystem.

	See: exp/src/nukernel/inc/fsproto.h 

*******************************************************************************/

#include <KernelExport.h>
#include <fsproto.h>
#include <dirent.h>
#include <fsil.h>
#include <OS.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <printf.h>
#include "memfs.h"

#if DEBUG == 4
	#define DEBUG_MEMFS 1
	#define DEBUG_AUX   1
	#define DEBUG_DUMP  0
	#define DEBUG_IOCTLS 1
	#define D(x) dprintf("%s %d: ", __FUNCTION__, __LINE__); dprintf x
	#ifndef DEBUG_VERIFY_FRAG_LIST
		#define DEBUG_VERIFY_FRAG_LIST 1
	#endif
#elif DEBUG == 3
	#define DEBUG_MEMFS 1
	#define DEBUG_AUX   1
	#define DEBUG_DUMP  0
	#define DEBUG_IOCTLS 1
	#define D(x)
	#ifndef DEBUG_VERIFY_FRAG_LIST
		#define DEBUG_VERIFY_FRAG_LIST 1
	#endif
#elif DEBUG == 2
	#define DEBUG_MEMFS 1
	#define DEBUG_AUX   1
	#define DEBUG_DUMP  0
	#define DEBUG_IOCTLS 1
	#define D(x)
	#ifndef DEBUG_VERIFY_FRAG_LIST
		#define DEBUG_VERIFY_FRAG_LIST 0
	#endif
#elif DEBUG == 1
	#define DEBUG_MEMFS 0
	#define DEBUG_AUX   0
	#define DEBUG_DUMP  0
	#define DEBUG_IOCTLS 1
	#define D(x)
	#ifndef DEBUG_VERIFY_FRAG_LIST
		#define DEBUG_VERIFY_FRAG_LIST 0
	#endif
#else
	#define DEBUG_MEMFS 0
	#define DEBUG_AUX   0
	#define DEBUG_DUMP  0
	#define DEBUG_IOCTLS 0
	#define D(x)
	#ifndef DEBUG_VERIFY_FRAG_LIST
		#define DEBUG_VERIFY_FRAG_LIST 0
	#endif
#endif

/* ******************************************************************************* */
/* local function prototypes */

static int check_permission( vnode *, int omode );
static int _memfs_unlink(memfs_info *info, vnode *dir, vnode *v);

// vnode creation/deletion
static int free_vnode( memfs_info *info, vnode *v, bool force, bool free_attributes);
static int malloc_vnode( memfs_info *, const char *name, vnode **);

// directory manipulation
static int insert_into_dir(vnode *dir, vnode *node, int which_dir);
static int delete_from_dir(vnode *dir, vnode *v, int which_dir);
static vnode *find_name_in_dir(vnode *dir, const char *name, int which_dir);

// file data allocation & I/O routines
static int memfs_set_file_size(memfs_info *info, vnode *v, size_t new_size);
static int memfs_read_write_file(memfs_info *info, vnode *v, char *buffer, off_t pos, size_t *len, int op);

// block allocation
static int memfs_allocate_specific_block(memfs_info *info, uint32 block);
static int memfs_allocate_block(memfs_info *info, uint32 *block);
static int memfs_free_block(memfs_info *info, uint32 block);

// block lookup
static uint8 *memfs_block_to_addr(memfs_info *info, uint32 block);
static uint8 *memfs_frag_block_to_addr(memfs_info *info, uint32 frag);

// hashtable lookup & maintenance
static void *memfs_lookup_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint32 key);
static int   memfs_insert_into_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap, uint32 key, void *ptr);
static int   memfs_remove_from_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap, uint32 key);
static uint32 memfs_find_free_slot_in_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap);

#if 0
static int verify_vnode(memfs_info *info, vnode *v);
#endif

#if DEBUG_DUMP
static void dump_stat( struct stat * );
static void dump_fs_info( fs_info * );
#endif
#if DEBUG_IOCTLS
static void dump_memfs_info( memfs_info * );
static void dump_frag_list(struct memfs_info *info);
static void dump_dir( vnode * );
static void dump_vnodes( memfs_info *info );
static void dump_vnode( vnode *v );
static void dump_area_bitmap(memfs_info *info);
static int cookie_count( vnode *n );
#endif

#if DEBUG_VERIFY_FRAG_LIST
static int verify_frag_list(memfs_info *info);
#else
#define verify_frag_list(x)
#endif

/* ******************************************************************************* */
/* vnode operations */

/*	read_vnode: given a ns & vnid, read the inode from the device, create and init
	a vnode cookie; return it in 'node'.
*/
static int
memfs_read_vnode( void *ns, vnode_id vnid, char r, void **node )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v;

#if DEBUG_MEMFS
	dprintf( "*** memfs: read_vnode looking for vnode 0x%Lx\n", vnid);
#endif

	*node = NULL;

	if(r == 0)
		lock_reader(info);
	// find the vnode
	v = memfs_lookup_hashtable(info, info->vnode_pgdir, vnid);
	if(v != NULL && v->magic == MEMFS_NODE_MAGIC && v->nsid == info->nsid && v->vnid == vnid) {
		*node = v;
	}
	if(r == 0)
		unlock_reader(info);
	
	if(*node != NULL)	
		return B_OK;
	else
		return ENOENT;
}

/*	write_vnode : write the vnode information back to permanent storage
	(for memfs, no permanent storage, nothing to do here)
*/
static int
memfs_write_vnode( void *ns, void *node, char r )
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: write_vnode\n");
#endif
	return B_OK;
}

/* remove_vnode : call free on the vnode struct, if dir the fsil should
	have cleaned out all contents, if file remove contents, if present.
	See free_vnode().
*/
static int
memfs_remove_vnode( void *ns, void *node, char r )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;
	int err;

#if DEBUG_MEMFS
	dprintf( "*** memfs: remove_vnode\n" );
#endif
	if(r == 0)
		lock_writer(info);
	err = free_vnode(info, v, false, true);
	if(r == 0)
		unlock_writer(info);
	return err;
}

/* secure_vnode : doesn't do anything
*/
static int
memfs_secure_vnode( void *ns, void *node )
{
#if DEBUG_MEMFS	
	dprintf( "*** memfs: secure_vnode\n" );
#endif
	return B_OK;
}

/******************************************************************************/
/* directory operations */

/*	walk : given a filesystem, directory vnode and file name, returns the vnid
	of the file (or if it is a symlink, returns the link vnid and returns the 
	link string in newpath).
*/
static int
memfs_walk( void *ns, void *base, const char *file, char **newpath, vnode_id *vnid )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *dir = (vnode *) base;
	vnode *tn = NULL;
	status_t err;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	char *np;
	vnode *v;

#if DEBUG_MEMFS
	dprintf( "*** memfs: walk on dir 0x%Lx looking for file '%s'\n", dir->vnid, file);
#endif
	if(!S_ISDIR(dir->mode))
		return ENOTDIR;

	lock_reader(info);

	if(dir->flags & MEMFS_VNODE_FLAG_DELETED) {
		err = ENOENT;
		goto err;
	}	

	/* check execute permission (bfs/walk.c) */
	uid = geteuid();		
	if ( uid != 0 ) {
		mode = dir->mode;
		gid = getegid();

		if ( uid == dir->uid )
			mode >>= 6;
		else if ( gid == dir->gid )
			mode >>= 3;

		if( (mode & S_IXOTH) != S_IXOTH ) {
			return EACCES;
		}
	}
	
	/* look for the file name in this directory */
	if(strcmp(file, ".") == 0) {
		v = dir;
	} else if (strcmp( file, ".." ) == 0) {
		v = dir->parent;
	} else {
		v = find_name_in_dir(dir, file, MEMFS_DIR);
	}

	if(v == NULL) {
		err = ENOENT;
		goto out;
	}

	if (S_ISLNK(v->mode) && newpath) {
		err = new_path(v->u.symlink, &np);
		if(err < B_OK)
			goto err;
		*newpath = np;
	} else {
		err = get_vnode(info->nsid, v->vnid, (void **)&tn);
	}	

	*vnid = v->vnid;	
err:
out:
	unlock_reader(info);

	return err;	
}

/*
	access: check access permissions on a vnode 
*/
static int
memfs_access( void *ns, void *node, int access_mode )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;
	mode_t mode = v->mode;
	uid_t uid;
	gid_t gid;

#if DEBUG_MEMFS
	dprintf( "*** memfs: access\n" );
#endif
	/* access() uses the real and not effective IDs, so sayeth Stevens, 4.7 */
	uid = getuid();
	gid = getgid();

	if(uid == 0)
        return B_OK;

	lock_reader(info);

	if(uid == v->uid)
		mode >>= 6;
	else if(gid == v->gid)
		mode >>= 3;

	unlock_reader(info);

	if((mode & access_mode & S_IRWXO) == access_mode)
		return B_OK;

    return EACCES;
}

static int
memfs_create(void *ns, void *_dir, const char *name,
					int omode, int perms, vnode_id *vnid, void **cookie)
{
	memfs_info *info = (memfs_info *) ns;
	vnode      *dir  = (vnode *) _dir;
	vnode      *v;
	file_info  *fc   = NULL;
	int        err	 = B_OK;
	bool       dir_changed = false;

#if DEBUG_MEMFS
	dprintf("*** memfs: create\n");
#endif
	if(!S_ISDIR(dir->mode))
		return EINVAL;
	
	if(*name == '\0' || !strcmp(name,".") || !strcmp(name,".."))
		return EINVAL;

	if(strlen(name) > PATH_MAX)
		return ENAMETOOLONG;

	lock_writer(info);

	if(dir->flags & MEMFS_VNODE_FLAG_DELETED) {
		err = EACCES;
		goto err;
	}	
	if(check_permission(dir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}
	
	/* look in parent for existing file */
	v = find_name_in_dir(dir, name, MEMFS_DIR);
	if(v != NULL) {  /* file already exists */
		vnode *dummy;
		if(omode & O_EXCL) {
			err = EEXIST;
			goto err;
		}		
		if(S_ISDIR(v->mode)) {
			err = EISDIR;
			goto err;
		}
		if(!S_ISREG(v->mode) && !S_ISLNK(v->mode)) {
			err = EACCES;
			goto err;
		}

		if(check_permission(v, omode)) {
			err = EACCES;
			goto err;
		}
		if(omode & O_TRUNC) {
			err = memfs_set_file_size(info, v, 0);
			if(err < B_OK)
				goto err;
		}

		/* get the vnode properly from the fsil */
		err = get_vnode(info->nsid, v->vnid, (void **)&dummy);
		if(err < B_OK)
			goto err;
	} else {
		/* must create a new vnode for a file */
		err = malloc_vnode(info, name, &v);
		if (err < B_OK) {
			err = ENOMEM;
			goto err;
		}
		v->parent = dir;
		v->mode = (perms & S_IUMSK) | S_IFREG;	

		err = new_vnode(info->nsid, v->vnid, (void *)v);
		if(err != B_OK) {
			free_vnode(info, v, false, false);
			goto err;
		}

		err = insert_into_dir(dir, v, MEMFS_DIR);
		if(err < B_OK) {
			free_vnode(info, v, false, false);
			goto err;
		}
		dir->mtime = time(NULL);
		dir_changed = true;
	}

	*vnid = v->vnid;

	/* create the cookie */
	fc = (file_info *)malloc(sizeof(file_info));
	if (fc == NULL) {
		err = ENOMEM;
		goto err;
	}

	fc->magic = MEMFS_FILE_MAGIC;
	fc->pos = 0;
	fc->omode = omode & 0xFFFF;
	fc->owner_vnid = v->vnid;
	fc->next = NULL;
	fc->next_fc = v->jar;
	v->jar = fc;

	*cookie = fc;

	unlock_writer(info);

	notify_listener(B_ENTRY_CREATED, info->nsid, dir->vnid, 0, v->vnid, name);
	if(dir_changed) notify_listener(B_STAT_CHANGED, info->nsid, 0, 0, dir->vnid, NULL);

	return B_OK;

err:
	unlock_writer(info);
	return err;
}

static int
memfs_mkdir( void *ns, void *_dir, const char *name, int perms )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *dir = (vnode *) _dir;
	vnode *newvn; 
	char *s;
	status_t err;

#if DEBUG_MEMFS
	dprintf( "*** memfs: mkdir\n" );
#endif

	if(!S_ISDIR(dir->mode))
		return ENOTDIR;

	/* 
 	 * This is to allow passing mkdir() a name with a '/' at the end.
	 */	
	s = (char *)strchr(name, '/');
	if(s && *(s+1) != 0)
		return EINVAL;

	if(*name == '\0' || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EINVAL;

	lock_writer(info);	
	
	if(dir->flags & MEMFS_VNODE_FLAG_DELETED) {
		err = EACCES;
		goto err;
	}
	if(check_permission(dir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}

	err = malloc_vnode( info, name, &newvn);
	if(err < B_OK) {
		err = ENOMEM;
		goto err;
	}
	newvn->parent = dir;
	newvn->mode |= DIR_MODE | S_IFDIR;

	err = insert_into_dir(dir, newvn, MEMFS_DIR);
	if(err < B_OK) {
		free_vnode(info, newvn, false, false);
		goto err;
	}
	
	err = new_vnode( info->nsid, newvn->vnid, (void *) newvn );
	if(err < B_OK) {
		delete_from_dir(dir, newvn, MEMFS_DIR);
		free_vnode(info, newvn, false, false);
		goto err;
	}
	
	dir->mtime = time(NULL);
		
	put_vnode(info->nsid, newvn->vnid);

	unlock_writer(info);

	notify_listener( B_ENTRY_CREATED, info->nsid, dir->vnid, 0, newvn->vnid, name );
	notify_listener( B_STAT_CHANGED, info->nsid, 0, 0, dir->vnid, NULL );

	return B_OK;
	
err:
	unlock_writer(info);
	return err;
}

static int
memfs_symlink( void *ns, void *_dir, const char *name, const char *path )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *dir = (vnode *) _dir;
	vnode *v;
	int err = B_OK;
	int n;

#if DEBUG_MEMFS
	dprintf( "*** memfs: symlink on dir 0x%Lx. name '%s', path '%s'\n", dir->vnid, name, path);
#endif

	CHECK_INFO(info,"symlink");
	CHECK_NODE(dir,"symlink");

	if(*path == '\0')
		return EINVAL;
	
	if(*name == '\0')
		return EINVAL;

	if(strchr(name, '/') || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) 
		return EINVAL;

	if(strlen(path) > PATH_MAX) 
		return ENAMETOOLONG;

	if(!S_ISDIR(dir->mode))
		return ENOTDIR;
	
	lock_writer(info);
		
	if(dir->flags & MEMFS_VNODE_FLAG_DELETED) {
		err = EACCES;
		goto err;
	}
	if(check_permission(dir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}
	if(find_name_in_dir(dir, name, MEMFS_DIR) != NULL) {
		err =  EEXIST;
		goto err;
	}

	err = malloc_vnode(info, name, &v);
	if (err < B_OK)
		goto err;

	v->parent = dir;
	v->mode |= LINK_MODE | S_IFLNK;

	n = strlen(path);
	if(info->size + n + 1 > info->max_size) {
		err = ENOSPC;
		goto err1;
	}	
	v->u.symlink = (char *)malloc(n + 1);
	if(v->u.symlink == NULL) {
		err = ENOMEM;
		goto err1;
	}
	strncpy(v->u.symlink, path, n);
	v->u.symlink[n] = '\0';
	info->size += n + 1;

	err = insert_into_dir(dir, v, MEMFS_DIR);
	if(err < B_OK)
		goto err1;
	
	err = new_vnode(info->nsid, v->vnid, (void *) v);
	if (err < B_OK) {
		delete_from_dir(dir, v, MEMFS_DIR);
		goto err1;
	}

	dir->mtime = time(NULL);

	put_vnode(info->nsid, v->vnid);

	unlock_writer(info);
	
	notify_listener(B_ENTRY_CREATED, info->nsid, dir->vnid, 0, v->vnid, name);
	notify_listener(B_STAT_CHANGED, info->nsid, 0, 0, dir->vnid, NULL );
	
	return B_OK;
	
err1:
	free_vnode(info, v, false, false);
err:
	unlock_writer(info);
	return err;
}

static int memfs_link(void *ns, void *dir, const char *name, void *vnode)
{
	dprintf( "memfs: link (not implemented)\n" );
	return ENOSYS;
}

static int	memfs_rename(void *ns, void *_olddir, const char *oldname,
                           void *_newdir, const char *newname)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *olddir = (vnode *) _olddir;
	vnode *newdir = (vnode *) _newdir;
	int newlen, oldlen;
	vnode *v = NULL;
	char *old;
	int err;
	bool vnode_unlinked = false;
	bool vnode_moved = false;
	vnode_id unlinked_vnode_dir = 0;
	vnode_id unlinked_vnode = 0;			
	
#if DEBUG_MEMFS
	dprintf( "*** memfs: rename\n" );
#endif
	if (*oldname == '\0' || *newname == '\0' || strchr(newname, '/') ) {
		return EINVAL;
	}

	if(!S_ISDIR(olddir->mode))
		return ENOTDIR;

	if(!S_ISDIR(newdir->mode))
		return ENOTDIR;

	lock_writer(info);

	if(check_permission(olddir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}

	if(olddir->vnid == newdir->vnid) {
		if(strcmp( newname, oldname) == 0)  {
			// rename over itself? ok, whatever.
			unlock_writer(info);
			return B_OK;
		} else {
			// rename to a new name inside the same dir
			struct vnode *v2;

			// find the old name
			v = find_name_in_dir(olddir, oldname, MEMFS_DIR);
			if(v == NULL) {
				err = ENOENT;
				goto err;
			}

			// see if the new name exists
			v2 = find_name_in_dir(olddir, newname, MEMFS_DIR);
			if(v2 != NULL) {
				// it already exists. We gotta erase it
				if(S_ISDIR(v2->mode)) {
					err = EPERM;
					goto err;
				}

				// see if we'll go over the memory limit
				// (want to check here so the more real EPERM error gets checked first)
				if(info->size + strlen(newname) - strlen(oldname) > info->max_size) {
					err = ENOSPC;
					goto err;
				}

				err = _memfs_unlink(info, olddir, v2);
				if(err < B_OK) {
					err = B_ERROR;
					goto err;
				}
				unlinked_vnode = v2->vnid;
				unlinked_vnode_dir = olddir->vnid;
				vnode_unlinked = true;
			} else {
				// see if we'll go over the memory limit
				if(info->size + strlen(newname) - strlen(oldname) > info->max_size) {
					err = ENOSPC;
					goto err;
				}
			}
				
			err = delete_from_dir(olddir, v, MEMFS_DIR);
			if(err < B_OK)
				goto err;

			old = v->name;
			oldlen = strlen(oldname) + 1;
			newlen = strlen(newname) + 1;
			if(oldlen != newlen) {
				v->name = (char *) malloc(newlen);
				if(v->name == NULL) {
					v->name = old;
					err = ENOMEM;
					insert_into_dir(olddir, v, MEMFS_DIR);
					goto err;
				}
				free(old);
			}
			strcpy(v->name, newname);
			vnode_moved = true;
			olddir->mtime = time(NULL);
			
			err = insert_into_dir(olddir, v, MEMFS_DIR);
			if(err < B_OK) {
				// we're screwed
				goto err;
			}				

		   	err = B_OK;	
		}
	} else {
		// rename to another dir
		vnode *v2;

		if(check_permission(newdir, O_WRONLY)) {
			err = EACCES;
			goto err;
		}
			
		// find the old name
		v = find_name_in_dir(olddir, oldname, MEMFS_DIR);
		if ( v == NULL ) {
			err = ENOENT;
			goto err;
		}

		// make sure we're not moving into one of our subdirs
		if(S_ISDIR(v->mode)) {
			for(v2 = v; v2 != info->root; v2 = v2->parent) {
				if(v2 == v) {
					err = EINVAL;
					goto err;
				}
			}
		}
			
		// see if the new name exists
		v2 = find_name_in_dir(newdir, newname, MEMFS_DIR);
		if(v2 != NULL) {
			// it already exists. We gotta erase it
			if(S_ISDIR(v2->mode)) {
				err = EPERM;
				goto err;
			}

			// see if we'll go over the memory limit
			// (want to check here so the more real EPERM error gets checked first)
			if(info->size + strlen(newname) - strlen(oldname) > info->max_size) {
				err = ENOSPC;
				goto err;
			}
			
			err = _memfs_unlink(info, newdir, v2);
			if(err < B_OK) {
				err = B_ERROR;
				goto err;
			}
			unlinked_vnode = v2->vnid;
			unlinked_vnode_dir = newdir->vnid;
			vnode_unlinked = true;
		} else {		
			// see if we'll go over the memory limit
			if(info->size + strlen(newname) - strlen(oldname) > info->max_size) {
				err = ENOSPC;
				goto err;
			}
		}
		err = delete_from_dir(olddir, v, MEMFS_DIR);
		if(err < B_OK) {
			err = B_ERROR;
			goto err;
		}
		
		err = insert_into_dir(newdir, v, MEMFS_DIR);
		if(err < B_OK) {
			// insert back into the old dir
			insert_into_dir(olddir, v, MEMFS_DIR);
			err = B_ERROR;
			goto err;
		}

		olddir->mtime = time(NULL);
		newdir->mtime = time(NULL);
		
		vnode_moved = true;		
		err = B_OK;
	}

	info->size += strlen(newname) - strlen(oldname);
	
err:
	unlock_writer(info);

	if(vnode_unlinked)
		notify_listener(B_ENTRY_REMOVED, info->nsid, unlinked_vnode_dir, 0, unlinked_vnode, NULL ); 

	if(vnode_moved)
		notify_listener(B_ENTRY_MOVED, info->nsid, olddir->vnid, newdir->vnid, v->vnid, newname);

	return err;
}

static int _memfs_unlink(memfs_info *info, vnode *dir, vnode *v)
{
	int err;
	vnode *dummy;

	err = get_vnode(info->nsid, v->vnid, (void **)&dummy);
	if(err < B_OK)
		return err;

	err = delete_from_dir(dir, v, MEMFS_DIR);
	if(err < B_OK)
		goto err;

	dir->mtime = time(NULL);
	v->flags |= MEMFS_VNODE_FLAG_DELETED;

	if(dummy != v) {
		err = B_ERROR;
		goto err;
	}

	err = remove_vnode( info->nsid, v->vnid );

err:
	put_vnode(info->nsid, v->vnid);

	return err;
}

/*	unlink : Remove the vnode from the directory list, but don't
	delete.  The fsil will call remove_vnode to delete it when 
	the ref count is 0.
*/
static int memfs_unlink( void *ns, void *_dir, const char *name )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *dir = (vnode *) _dir;
	vnode *v;
	status_t err;
	vnode_id unlinked_vnid = 0;

#if DEBUG_MEMFS
	dprintf( "*** memfs: unlink\n" );
#endif

	if(!S_ISDIR(dir->mode))
		return ENOTDIR;

	if(!strcmp( name, "." ) || !strcmp( name, ".." ))
		return EPERM;

	lock_writer(info);

	if(check_permission(dir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}

	v = find_name_in_dir(dir, name, MEMFS_DIR);
	if(v == NULL) {
		err = ENOENT;
		goto err;
	}

	if(S_ISDIR(v->mode)) {
		err = EISDIR;
		goto err;
	}

	dir->mtime = time(NULL);

	unlinked_vnid = v->vnid;
	err = _memfs_unlink(info, dir, v);

err:
	unlock_writer(info);
	
	if(err == B_OK)
		notify_listener(B_ENTRY_REMOVED, info->nsid, dir->vnid, 0, unlinked_vnid, NULL);
	
	return err;
}

/*	rmdir : Remove the vnode from the directory list, but don't free.
*/
static int	memfs_rmdir( void *ns, void *_dir, const char *name )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *dir = (vnode *) _dir;
	vnode *v;
	status_t err;
	vnode_id unlinked_vnid = 0;

#if DEBUG_MEMFS
	dprintf( "*** memfs: rmdir\n" );
#endif
	if(!S_ISDIR(dir->mode))
		return EINVAL;

	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EPERM;

	lock_writer(info);

	if(check_permission(dir, O_WRONLY)) {
		err = EACCES;
		goto err;
	}

	v = find_name_in_dir(dir, name, MEMFS_DIR);
	if(v == NULL) {
		err = ENOENT;
		goto err;
	}

	if(!S_ISDIR(v->mode)) {
		err = ENOTDIR;
		goto err;
	}

	if(v->u.dir_head != NULL) {
		err = ENOTEMPTY;
		goto err;
	}

	dir->mtime = time(NULL);

	unlinked_vnid = v->vnid;
	err = _memfs_unlink(info, dir, v);

err:
	unlock_writer(info);

	if(err == B_OK)
		notify_listener(B_ENTRY_REMOVED, info->nsid, dir->vnid, 0, unlinked_vnid, NULL ); 

	return err;
}

static int	memfs_readlink(void *ns, void *node, char *buf, size_t *bufsize)
{
	memfs_info *info = (memfs_info *) ns;
	vnode      *v    = (vnode *) node;
	int        len;

#if DEBUG_MEMFS
	dprintf( "*** memfs: readlink on vnode 0x%Lx\n", v->vnid);
#endif
	if(!S_ISLNK(v->mode))
		return EINVAL;

	lock_reader(info);

	len = strlen(v->u.symlink);
	if (len > *bufsize)
		len = *bufsize;

	memcpy(buf, v->u.symlink, len);

	unlock_reader(info);

	*bufsize = len;

	return B_OK;
}

/* opendir : create and initialize a file_info struct (dircookie) 
*/
static int memfs_opendir(void *ns, void *node, void **cookie)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;
	file_info *dir = NULL;

#if DEBUG_MEMFS
	dprintf( "*** memfs: opendir on vnode 0x%Lx\n", v->vnid);
#endif
	if(!S_ISDIR(v->mode))
		return ENOTDIR;

	/* create the dircookie */
	dir = (file_info *)malloc(sizeof(file_info));
	if(dir == NULL)
		return ENOMEM;

	dir->magic = MEMFS_FILE_MAGIC;
	dir->pos = 0;
	dir->omode = O_RDWR;
	dir->next = NULL;
	dir->owner_vnid = v->vnid;

	lock_writer(info);
	
	dir->next_fc = v->jar;
	v->jar = dir;

	unlock_writer(info);
	
	*cookie = (void *)dir;

	return B_OK;
}

/* closedir: (nothing to do here, cookie freed elsewhere) */
static int	memfs_closedir(void *ns, void *node, void *cookie)
{
	vnode *v = (vnode *) node;

#if DEBUG_MEMFS
	dprintf( "*** memfs: closedir\n" );
#endif
	if(!S_ISDIR(v->mode))
		return ENOTDIR;

	return B_OK;
}

static int	memfs_rewinddir(void *ns, void *node, void *cookie)
{
	vnode *v = (vnode *) node;
	file_info *dir = (file_info *) cookie;

#if DEBUG_MEMFS
	dprintf( "*** memfs: rewinddir\n" );
#endif
	if(!S_ISDIR(v->mode))
		return ENOTDIR;

	dir->pos = 0;

	return B_OK;
}

static int	memfs_readdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *ev, *pv = (vnode *) node;
	file_info *dc = (file_info *) cookie;
	int recsize;
	int pos;

#if DEBUG_MEMFS
	dprintf( "*** memfs: readdir\n" );
#endif
	if(bufsize < 1) {
		dprintf( "memfs: readdir: bad num(0x%p)/buf(0x%p)/bufsize(%ld)\n", num, buf, bufsize );
		return EINVAL;
	}

	if(!S_ISDIR(pv->mode))
		return ENOTDIR;

	lock_reader(info);

#if DEBUG_AUX
	dprintf( "=== memfs: readdir: dc->pos = %d, dc->next = %p\n", dc->pos, dc->next );
#endif

	recsize = 2*sizeof(dev_t)+2*sizeof(ino_t)+sizeof(unsigned short);
	ev = dc->next;
	pos = dc->pos;
	
	if(pos == 0) {  /* "." */
		dc->next = NULL;
		dc->pos = 1;

		recsize += 2;
		buf->d_dev = info->nsid;
		buf->d_ino = (ino_t) pv->vnid; 
		buf->d_reclen = recsize; 
		strncpy( buf->d_name, "." , (bufsize - sizeof(struct dirent)) );
		buf->d_name[1] = '\0';

		*num = 1L;
	} else if(pos == 1) {  /* ".." */
		dc->next = pv->u.dir_head;
		dc->pos = 2;

		recsize += 3;
		buf->d_dev = info->nsid;
		buf->d_ino = (ino_t) pv->parent->vnid;
		buf->d_reclen = recsize; 
		strncpy(buf->d_name, "..", (bufsize - sizeof(struct dirent)) );
		buf->d_name[2] = '\0';

		*num = 1L;
	} else if(pos >= 2 && ev != NULL) {
		dc->next = ev->next;
		atomic_add((long *)&dc->pos, 1);

		recsize += strlen(ev->name) + 1;
		buf->d_dev = info->nsid;
		buf->d_ino = (ino_t) ev->vnid;
		buf->d_pino = (ino_t) pv->vnid;
		buf->d_reclen = recsize; 
		strncpy(buf->d_name, ev->name , (bufsize - sizeof(struct dirent)) );
		buf->d_name[strlen(ev->name)] = '\0';

		*num = 1L;
	} else { /* no more entries */
		*num = 0L;
	}
	
	unlock_reader(info);

	return B_OK;
}

/******************************************************************************/
/* file operations */

static int memfs_open(void *ns, void *node, int omode, void **cookie)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;
	file_info *fc = (file_info *) *cookie;
	status_t err;

#if DEBUG_MEMFS
	dprintf( "*** memfs: open on vnode 0x%Lx\n", v->vnid);
#endif
	if(S_ISDIR(v->mode) && (omode & O_WRONLY))
		return EPERM;

	if((omode & O_RWMASK) == 3)
		return EINVAL;

	lock_reader(info);

	if(check_permission( v, omode)) {
		err = EACCES;
		goto err;
	}

	/* create the file cookie */
	fc = (file_info *)malloc(sizeof(file_info));
	if (fc == NULL) {
		err =  ENOMEM;
		goto err;
	}

	fc->magic = MEMFS_FILE_MAGIC;
	fc->pos = 0;
	fc->omode = omode;
	fc->owner_vnid = v->vnid;
	fc->next = NULL;
	fc->next_fc = v->jar;
	v->jar = fc;

	err = B_OK;
	
	*cookie = (void *)fc;

err:
	unlock_reader(info);
	
	return err;
}

/*	close : 
*/
static int memfs_close(void *ns, void *node, void *cookie)
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: close\n" );
#endif
	return B_OK;
}

static int memfs_free_cookie(void *ns, void *node, void *cookie)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node; 
	file_info *file = (file_info *) cookie;
	file_info *curr, *last;

#if DEBUG_MEMFS
	dprintf( "*** memfs: free_cookie\n" );
#endif
	lock_writer(info);

	/* remove cookie from vnode's list of open cookies */
	last = NULL;
	curr = v->jar;
	while(curr != NULL) {
		if(curr == file) {
			if(last != NULL) {
				last->next_fc = curr->next_fc; 
			} else {
				v->jar = curr->next_fc;
			}
			break;
		}
		last = curr;
		curr = curr->next_fc;
	}

	unlock_writer(info);

	free(file);

	return B_OK;
}

static int 
memfs_read( void *ns, void *node, void *cookie, off_t pos, void *buf, size_t *len )
{
	memfs_info *info   = (memfs_info *) ns;
	vnode      *v      = (vnode *) node;
	file_info  *fc     = (file_info *)cookie;
	int		   err     = B_OK;
		
#if DEBUG_MEMFS
	dprintf( "*** memfs: read entry: vnid 0x%Lx pos %Ld len %ld\n",
		v->vnid, pos, *len );
#endif
	if(S_ISDIR(v->mode)) {
		dprintf( "memfs: read: 0x%Lx is a directory\n", v->vnid ); 
		return EISDIR;
	}
	if(S_ISLNK(v->mode)) {
		dprintf( "memfs: read: 0x%Lx is a symlink\n", v->vnid ); 
		return EPERM;
	}
	if(pos < 0) {
		dprintf( "memfs: read: negative pos %Ld\n", pos ); 
		pos = 0;
	}
	if(*len <= 0) {
		*len = 0;
		return B_OK;
	}

	lock_reader(info);
	
	if((fc->omode & O_RWMASK) == O_WRONLY) {
		err = EACCES;
		goto out;
	}
	if(pos > v->size) {
		*len = 0;
		err = B_OK;
		goto out;
	}

	// trim reads to not read past end of file	
	*len = min(pos + *len, v->size) - pos;
	err = memfs_read_write_file(info, v, (char *)buf, pos, len, MEMFS_READ);
	if(err < B_OK)
		goto err;
	
out:
err:
	unlock_reader(info);

	return err;
}

static int memfs_write( void *ns, void *node, void *cookie, 
				off_t pos, const void *buf, size_t *len )
{
	memfs_info *info   = (memfs_info *) ns;
	vnode      *v      = (vnode *) node;
	file_info  *file   = (file_info *) cookie;
	size_t     newsize;
	status_t   err     = B_OK;
	bool       size_changed = false;

#if DEBUG_MEMFS
	dprintf( "*** memfs: write on vnid 0x%Lx, pos %Ld, len %ld\n",
		v->vnid, pos, *len );
#endif
	if(S_ISDIR(v->mode)) {
		dprintf( "memfs: write: 0x%Lx is a directory\n", v->vnid ); 
		return EISDIR;
	}
	if(S_ISLNK(v->mode)) {
		dprintf( "memfs: write: 0x%Lx is a symlink\n", v->vnid ); 
		return EPERM;
	}
	if(*len <= 0) 
		return B_OK;	

	if(pos < 0) {
		dprintf( "memfs: write: negative pos (%Ld), setting pos = 0\n", pos ); 
		pos = 0;
	}

	lock_writer(info);
	
	if((file->omode & O_RWMASK) == O_RDONLY) {
		err = EACCES;
		goto err;
	}

	if(file->omode & O_APPEND)
		pos = v->size;

	// resize the file
	newsize = max(pos + *len, v->size);
	if(newsize > v->size) {
		size_t old_size = v->size;
		err = memfs_set_file_size(info, v, newsize);
		if(err < B_OK) {
			memfs_set_file_size(info, v, old_size);
			goto err;
		}
		size_changed = true;
	}

	err = memfs_read_write_file(info, v, (char *)buf, pos, len, MEMFS_WRITE);

	v->mtime = time(NULL);

err:
	unlock_writer(info);
	
	if(size_changed)
		notify_listener( B_STAT_CHANGED, info->nsid, 0, 0, v->vnid, NULL );			

	return err;
}

static int memfs_ioctl(void *ns, void *_node, void *cookie, int cmd, void *buf, size_t len)
{
	memfs_info *info = (memfs_info *) ns;
	int err = B_OK;
#if DEBUG_IOCTLS
	vnode      *v    = (vnode *)_node;
#endif

#if DEBUG_MEMFS
	dprintf( "*** memfs: ioctl\n" );
#endif

	info = info;

	switch( cmd ) {
	case 10002:
		/* DOME : return creation time as bigtime_t t = node->ctime << 16;
		 * See bfs.  May not be needed for anything. */ 
		err = EINVAL;
		break;
	case 10003:
		/* DOME : return modification time as bigtime_t t = node->mtime << 16;
		 * See bfs.  May not be needed for anything. */ 
		err = EINVAL;
		break;
#if DEBUG_IOCTLS
	case MEMFS_IOCTL_DUMP_INFO:	/* memfs : dump filesystem structures */ 
		lock_reader(info);
		dump_memfs_info( info );
		dump_area_bitmap( info );
		unlock_reader(info);
		err = B_OK;
		break;
	case MEMFS_IOCTL_DUMP_DIR_INFO:
		lock_reader(info);
		dump_dir(v);
		unlock_reader(info);
		err = B_OK;
		break;
	case MEMFS_IOCTL_DUMP_VNODE_INFO:
		lock_reader(info);
		dump_vnode(v);
		unlock_reader(info);
		err = B_OK;
		break;
	case MEMFS_IOCTL_DUMP_ALL_VNODES:
		lock_reader(info);
		dump_vnodes(info);
		unlock_reader(info);
		err = B_OK;
		break;
	case MEMFS_IOCTL_DUMP_FRAG_LIST:
		lock_reader(info);
		dump_frag_list(info);
		unlock_reader(info);
		err = B_OK;
		break;
#endif
	default:
		dprintf( "memfs: ioctl: invalid command %d (0x%x)\n" , cmd, (unsigned) cmd );
		err = EINVAL;
		break;
	}
	
	return err;;
}

static int memfs_setflags(void *ns, void *node, void *cookie, int flags)
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: setflags\n" );
#endif
	return ENOSYS;
}

/*
	see: /src/exp/headers/posix/sys/stat.h
*/

static int
memfs_rstat( void *ns, void *node, struct stat *st )
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v         = (vnode *) node;

#if DEBUG_MEMFS
	dprintf( "*** memfs: rstat on vnode 0x%Lx\n", v->vnid);
#endif
	lock_reader(info);

    st->st_dev = info->nsid;	/* NOT USED: "device" that this file resides on */
    st->st_ino = v->vnid;		/* this file's inode #, unique per device */
    st->st_mode = v->mode;		/* mode bits (rwx for user, group, etc) */
    st->st_nlink = 1;			/* number of hard links to this file */
    st->st_uid = v->uid;		/* user id of the owner of this file */
    st->st_gid = v->gid;		/* group id of the owner of this file */
    st->st_size = v->size;		/* size in bytes of this file */
    st->st_rdev = 0;			/* NOT USED: device type (not used) */
    st->st_blksize = MEMFS_BLOCKSIZE;	/* preferred block size for i/o */
    st->st_atime = v->mtime;	/* NOT USED: last access time */
    st->st_mtime = v->mtime;	/* last modification time */
    st->st_ctime = v->mtime;	/* NOT USED: last change time, not creation time */
    st->st_crtime = v->ctime;	/* creation time */

	unlock_reader(info);

#if DEBUG_DUMP
	dump_stat( st );
#endif

	return B_OK;
}

static int
memfs_wstat( void *ns, void *node, struct stat *st, long mask )
{
	int ok;
	int err = B_OK;
	uid_t uid;
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;

#if DEBUG_MEMFS
	dprintf( "*** memfs: wstat\n" );
#endif

	lock_writer(info);
	
	/* check permission and make changes (lifted from bfs/file.c) */
	uid = geteuid();
	if (uid != 0) { /* if not root, enforce permissions */
		ok = 1;
		if ((mask & WSTAT_PROT_OWNER) && (v->uid != uid)) {
			/* chown/chgrp/chmod only allowed on files that we own */
			ok = 0;
		}
		if ((mask & WSTAT_PROT_TIME) && (v->uid != uid) ) {
			/* the time changing operations should go through if you
			own the file or you have write permissions */
			ok = 0;
		}
		if (mask & WSTAT_PROT_OTHER) {
			/* other operations just use regular permission bits */
			ok = 0;
		}
		if ( !ok ) {
			unlock_writer(info);
			return EACCES;
		}
	}
		
	if (mask & WSTAT_MODE) {
		v->mode = (v->mode & S_IFMT) | (st->st_mode & S_IUMSK);
	}

	if (mask & WSTAT_UID) {
		if((uid == 0) || (st->st_uid == uid)) {
			v->uid = st->st_uid;
		}
	}

	if (mask & WSTAT_GID) {
		if((uid == 0) || (st->st_gid == getegid())) {
			v->gid = st->st_gid;
		}
	}

	if (mask & WSTAT_SIZE) {
		if(!S_ISREG(v->mode)) {
			unlock_writer(info);
			err = EISDIR;
			return err;
		}
		err = memfs_set_file_size(info, v, st->st_size);
		if(err < B_OK) {
			unlock_writer(info);
			return err;
		}
	}

	if (mask & WSTAT_CRTIME) {
		v->ctime = st->st_crtime;
	}

	if (mask & WSTAT_MTIME) {
		if ( st->st_mtime == 0 )
			v->mtime = time(NULL);
		else 
			v->mtime = st->st_mtime;
	}

	if (mask & WSTAT_ATIME) {
		/* do nothing, no atime */
	}

	unlock_writer(info);

	/* notify listeners */
	notify_listener(B_STAT_CHANGED, (nspace_id) info->nsid, 0, 0, v->vnid, NULL);

	return B_OK;
}

/* fsync : no permanent storage */
static int
memfs_fsync( void *ns, void *node )
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: fsync\n" );
#endif
	return B_OK;
}

static int
memfs_initialize( const char *device, void *params, size_t len )
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: initialize\n" );
#endif
	return ENOSYS;
}

/*	mount : called to mount (create and initialize) the file system
	- assumes memfs always mounted read/write
	- 
*/
static int
memfs_mount(	nspace_id nsid,		/* the assigned id from fsil */
				const char *device,
				ulong flags,
				void *params,		/* must contain size of filesystem, in bytes. */
				size_t len,			/* not used */
				void **ns,			/* return the nspace cookie here */
				vnode_id *vnid )	/* return the vnode_id of the root */
{
	status_t err = B_OK;
	memfs_info *info;
	vnode *root;
	size_t new_size;
	uint32 lock;
	
#if DEBUG_MEMFS
	load_driver_symbols("memfs");
	dprintf( "*** memfs_mount\n" );
#endif

#ifdef USE_DMALLOC
	dprintf("checking memory\n");
	check_mem();
#endif

	/* check to make sure the params has passed in a reasonable size */
	if(params == NULL) {
		dprintf("memfs_mount: no size passed\n");
		err = EINVAL;
		goto err;
	}
	new_size = *(size_t *)params;
	if(new_size < MEMFS_MIN_SIZE || new_size > MEMFS_MAX_SIZE) {
		dprintf("memfs_mount: invalid size passed (%ld)\n", new_size);
		err = EINVAL;
		goto err;
	}
	new_size = ROUNDUP(new_size, B_PAGE_SIZE);

	/* allocate the nspace object */
	info = (memfs_info *)malloc(sizeof(memfs_info));
	if(info == NULL) {
		dprintf( "memfs_mount: no memory for info block\n" );
		err = ENOMEM;
		goto err;
	}

	info->magic = MEMFS_MAGIC;
	sprintf(info->name, "memfs-%X", (unsigned int)nsid);
	info->nsid = nsid;
	info->flags = B_FS_HAS_MIME|B_FS_HAS_ATTR;
	info->vnode_count = 0;
	info->size = sizeof(struct memfs_info);
	info->all_head = NULL;
	info->vnode_pgdir = 0;
	info->all_frags = NULL;
	info->unused_frags = NULL;
	info->next_version = 0;
	
	info->lock = create_sem(MAX_READERS, "memfs lock" );
	if(info->lock < 0) {
		dprintf( "memfs_mount: failed to create semaphore\n" );
		err = ENOMEM;
		goto err1;
	}

	// allocate the area we'll be working out of
	//  take care of this flag: if B_SLOWMEM exists, use it. (only on BeIA)
	//  NOTE: using B_SLOWMEM changes the memory usage of the system.
	//  B_SLOWMEM preallocates all of the data in the area, while B_NO_LOCK
	//  lazy allocates the pages. Thus, at resize in B_SLOWMEM, all of the
	//  memory the filesystem will use is grabbed right then. This is good on
	//  a machine with no swap, because you're relatively sure it wont die
	//  unexpectedly as it fails to bring in a page later.
	#ifdef B_SLOWMEM
	lock = B_SLOWMEM;
	#else
	lock = B_FULL_LOCK;
	#endif

	info->area = create_area(info->name, (void **)&info->area_ptr,
		B_ANY_KERNEL_ADDRESS, new_size, lock, B_READ_AREA | B_WRITE_AREA);
	if(info->area < 0) {
		err = ENOMEM;
		goto err2;
	}	

	info->area_size = new_size / B_PAGE_SIZE;
	info->area_bitmap = (uint8 *)malloc(ROUNDUP(info->area_size, 8) / 8);
	if(info->area_bitmap == NULL) {
		err = ENOMEM;
		goto err3;
	}
	memset(info->area_bitmap, 0, ROUNDUP(info->area_size, 8) / 8);
	info->max_size = new_size;
	info->size += ROUNDUP(info->area_size, 8) / 8;

	// set up the pgdir for the vnode hash
	err = memfs_allocate_specific_block(info, 0);
	if(err != B_OK)
		goto err4;
	info->vnode_pgdir = 0;
	memset(memfs_block_to_addr(info, info->vnode_pgdir), 0, sizeof(B_PAGE_SIZE));
	memset(info->vnode_page_full_bitmap, 0, sizeof(info->vnode_page_full_bitmap));
	info->size += B_PAGE_SIZE;

	/* allocate the fs root vnode */
	err = malloc_vnode( info, "fs root", (vnode **) &root);
	if (err < B_OK) {
		dprintf( "memfs_mount: no memory for root vnode\n" );
		goto err4;
	}
	root->mode = DIR_MODE | S_IFDIR;
#if DEBUG_AUX
	dprintf("=== mount: root: root %08x, mode %08x, vnid %08x, parent %08x\n",
	 	(unsigned) root, root->mode, (unsigned) root->vnid, (unsigned) root->parent );
#endif
	
	info->root = root;

	/* return nspace (memfs_info) cookie  */
	*ns = (void *) info;
	/* return root vnode id */
	*vnid = info->root->vnid;

#if DEBUG_AUX
	dprintf("=== calling new_vnode: nsid %d, *vnid 0x%X, ptr 0x%X\n",
		(unsigned) info->nsid, (unsigned) *vnid, (unsigned) info->root);
#endif

	/* call new_vnode for the root directory */
	err = new_vnode( nsid, *vnid, (void *) root );
	if ( err != B_OK ) {
		dprintf( "memfs: mount: new_vnode() failed\n" );
		goto err4;
	}
	
	return B_OK;

err4:
	free(info->area_bitmap);
err3:
	delete_area(info->area);
err2:	
	delete_sem(info->lock);	
err1:
	free(info);
err:
	return err;
}

/*	unmount :
	Since there is a master list of vnodes, on unmount all the remaining data 
	structures are freed.
*/
static int
memfs_unmount( void *ns )
{
	memfs_info *info = (memfs_info *) ns;

#if DEBUG_MEMFS
	dprintf( "*** memfs: unmount\n" );
#endif

	// free all of the vnodes
	while(info->all_head != NULL) {	
		free_vnode(info, info->all_head, true, false);
	}

	// free all of the frags
	while(info->all_frags != NULL) {
		struct frag_desc *next = info->all_frags->next;
		free(info->all_frags);
		info->all_frags = next;
	}

	if(info->area >= 0) {
		delete_area(info->area);
		free(info->area_bitmap);
	}

	delete_sem(info->lock);
	free(info);
	
#ifdef USE_DMALLOC
	dprintf("checking memory\n");
	check_mem();
#endif	
	
	return B_OK;
}

/* sync : no permanent storage */
static int
memfs_sync(void *ns)
{
#if DEBUG_SYNC
	dprintf( "*** memfs: sync\n" );
#endif
	return B_OK;
}

static int memfs_rfsstat( void *ns, struct fs_info *fst )
{
	memfs_info *info = (memfs_info *) ns;

#if DEBUG_MEMFS
	dprintf( "*** memfs: rfsstat\n" );
#endif

	lock_reader(info);

	fst->dev = info->nsid;
	fst->root = info->root->vnid;
	fst->flags = info->flags;					/* file system flags */
	fst->block_size = 1;			/* fundamental block size */
	fst->io_size = MEMFS_BLOCKSIZE;				/* optimal io size */
	fst->total_blocks = info->max_size;
	fst->free_blocks = info->max_size - info->size;	/* number of free blocks */
	fst->total_nodes = info->vnode_count;		/* total number of nodes */
	fst->free_nodes = 0;						/* number of free nodes */
	fst->device_name[0] = '\0';					/* device holding fs */
	strcpy(fst->volume_name, info->name);		/* volume name */
	strcpy(fst->fsh_name, "memfs");		/* name of fs handler */

#if DEBUG_DUMP
	dump_memfs_info( info );
	dump_fs_info( fst );
#endif

	unlock_reader(info);

	return B_OK;
}

/* wststat : change volume_name
*/
static int memfs_wfsstat( void *ns, struct fs_info *fs, long mask )
{
	memfs_info *info = (memfs_info *) ns;

#if DEBUG_MEMFS
	dprintf( "*** memfs: wfsstat\n" );
#endif

	if(mask & WFSSTAT_NAME) {
		if(strchr(fs->volume_name, '/'))
			return EINVAL;

		lock_writer(info);
		
		strncpy(info->name, fs->volume_name, sizeof(info->name));
		info->name[sizeof(info->name) - 1] = '\0';

		unlock_writer(info);
	}
	
	return B_OK;
}

int	memfs_open_attrdir(void *ns, void *node, void **cookie)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v = (vnode *) node;
	file_info *dir = NULL;

#if DEBUG_MEMFS
	dprintf( "*** memfs: open_attrdir on vnode 0x%Lx\n", v->vnid);
#endif
	/* create the dircookie */
	dir = (file_info *)malloc(sizeof(file_info));
	if(dir == NULL)
		return ENOMEM;

	dir->magic = MEMFS_FILE_MAGIC;
	dir->pos = 0;
	dir->omode = O_RDWR;
	dir->owner_vnid = v->vnid;

	lock_writer(info);
	
	dir->next_fc = v->jar;
	v->jar = dir;

	unlock_writer(info);
	
	*cookie = (void *)dir;

	return B_OK;
}

int	memfs_close_attrdir(void *ns, void *node, void *cookie)
{
#if DEBUG_MEMFS
	dprintf( "*** memfs: close_attrdir\n" );
#endif
	return B_OK;
}

int	memfs_rewind_attrdir(void *ns, void *node, void *cookie)
{
	file_info *dir = (file_info *) cookie;

#if DEBUG_MEMFS
	dprintf( "*** memfs: rewind_attrdir\n" );
#endif

	dir->pos = 0;

	return B_OK;
}

int	memfs_read_attrdir(void *ns, void *node, void *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	memfs_info *info = (memfs_info *)ns;
	vnode *ev, *pv = (vnode *)node;
	file_info *dc = (file_info *)cookie;
	size_t recsize;
	int pos;

#if DEBUG_MEMFS
	dprintf( "*** memfs: read_attrdir\n" );
#endif
	if(bufsize < 1) {
		dprintf( "memfs: read_attrdir: bad num(0x%p)/buf(0x%p)/bufsize(%ld)\n", num, buf, bufsize );
		return EINVAL;
	}

	lock_reader(info);

#if DEBUG_AUX
	dprintf( "=== memfs: read_attrdir: dc->pos = %d, dc->next = %p\n", dc->pos, dc->next );
#endif

	pos = dc->pos;
	if(pos == 0)
		ev = pv->attr_head;
	else
		ev = dc->next;
	if(ev != NULL) {		
		dc->next = ev->next;
		atomic_add((long *)&dc->pos, 1);

		recsize = 2*sizeof(dev_t)+2*sizeof(ino_t)+sizeof(unsigned short);
		recsize += strlen(ev->name) + 1;
		if(recsize > bufsize)
			return EINVAL;

		buf->d_dev = info->nsid;
		buf->d_ino = (ino_t)pos;
		buf->d_pino = (ino_t)pv->vnid;
		buf->d_reclen = recsize; 
		strncpy(buf->d_name, ev->name, (bufsize - sizeof(struct dirent)));
		buf->d_name[strlen(ev->name)] = '\0';

		*num = 1L;
	} else { /* no more entries */
		*num = 0L;
	}
	
	unlock_reader(info);

	return B_OK;
}

int memfs_free_attrdircookie(void *ns, void *node, void *cookie)
{
	memfs_info *info = (memfs_info *)ns;
	vnode *v = (vnode *)node; 
	file_info *file = (file_info *)cookie;
	file_info *curr;
	file_info *last;

#if DEBUG_MEMFS
	dprintf( "*** memfs: free_attrdircookie\n" );
#endif
	lock_writer(info);

	/* remove cookie from vnode's list of open cookies */
	last = NULL;
	curr = v->jar;
	while(curr != NULL) {
		if(curr == file) {
			if(last != NULL) {
				last->next_fc = curr->next_fc; 
			} else {
				v->jar = curr->next_fc;
			}
			break;
		}
		last = curr;
		curr = curr->next_fc;
	}

	unlock_writer(info);

	free(file);

	return B_OK;
}

int	memfs_remove_attr(void *ns, void *node, const char *name)
{
	memfs_info *info = (memfs_info *)ns;
	vnode *dir = (vnode *)node;
	vnode *v;
	status_t err;

#if DEBUG_MEMFS
	dprintf( "*** memfs: remove_attr\n" );
#endif

	lock_writer(info);

	v = find_name_in_dir(dir, name, MEMFS_ATTRDIR);
	if(v == NULL) {
		err = ENOENT;
		goto err;
	}

	dir->mtime = time(NULL);

	err = delete_from_dir(dir, v, MEMFS_ATTRDIR);
	if(err < B_OK)
		goto err;

	err = free_vnode(info, v, false, false);

err:
	unlock_writer(info);
	
	if(err == B_OK)
		notify_listener(B_ATTR_CHANGED, info->nsid, 0, 0, dir->vnid, name);
	
	return err;
}

#if 0
// no one else does this, not sure the semantics of it
int	memfs_rename_attr(void *ns, void *node, const char *oldname, const char *newname)
{
	return B_ERROR;
}
#endif

int	memfs_stat_attr(void *ns, void *node, const char *name, struct attr_info *buf)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v         = (vnode *) node;
	vnode *attr;
	int err = B_OK;

#if DEBUG_MEMFS
	dprintf( "*** memfs: stat_attr on vnode 0x%Lx\n", v->vnid);
#endif
	lock_reader(info);

	attr = find_name_in_dir(v, name, MEMFS_ATTRDIR);
	if(attr == NULL) {
		err = ENOENT;
		goto err;
	}

	buf->size = attr->size;
	buf->type = attr->u.data.attr_type;

err:
	unlock_reader(info);

	return err;
}

int	memfs_write_attr(void *ns, void *node, const char *name, int type, const void *buf, size_t *len, off_t pos)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v         = (vnode *) node;
	vnode *attr;
	int err;

#if DEBUG_MEMFS
	dprintf( "*** memfs: write_attr on vnode 0x%Lx\n", v->vnid);
#endif

	lock_writer(info);

	attr = find_name_in_dir(v, name, MEMFS_ATTRDIR);
	if(attr != NULL) {
		// truncate it first
		err = memfs_set_file_size(info, attr, 0);
		if(err < 0)
			goto err;
	} else {
		// create one
		err = malloc_vnode(info, name, &attr);
		if (err < B_OK) {
			err = ENOMEM;
			goto err;
		}
		attr->parent = v;
		attr->mode = S_IFREG | S_ATTR;

		err = insert_into_dir(v, attr, MEMFS_ATTRDIR);
		if(err < B_OK) {
			free_vnode(info, attr, false, false);
			goto err;
		}
	}

	err = memfs_set_file_size(info, attr, pos + *len);
	if(err < B_OK)
		goto err;

	attr->u.data.attr_type = type;
	err = memfs_read_write_file(info, attr, (char *)buf, pos, len, MEMFS_WRITE);	

err:
	unlock_writer(info);

	if(err >= B_OK)
		notify_listener(B_ATTR_CHANGED, info->nsid, 0, 0, v->vnid, name);

	return err;
}

int	memfs_read_attr(void *ns, void *node, const char *name, int type, void *buf, size_t *len, off_t pos)
{
	memfs_info *info = (memfs_info *) ns;
	vnode *v         = (vnode *) node;
	int err;
	vnode *attr;

#if DEBUG_MEMFS
	dprintf( "*** memfs: read_attr on vnode 0x%Lx\n", v->vnid);
#endif

	lock_reader(info);

	attr = find_name_in_dir(v, name, MEMFS_ATTRDIR);
	if(attr == NULL) {
		err = ENOENT;
		goto err;
	}

	if(pos > attr->size) {
		*len = 0;
		err = B_OK;
		goto out;
	}

	// trim reads to not read past end of file	
	*len = min(pos + *len, attr->size) - pos;
	err = memfs_read_write_file(info, attr, (char *)buf, pos, len, MEMFS_READ);

err:
out:
	unlock_reader(info);

	return err;
}

/* index operations not implemented */
#if 0
int	memfs_open_indexdir(void *ns, void **cookie);
int	memfs_close_indexdir(void *ns, void *cookie);
int	memfs_rewind_indexdir(void *ns, void *cookie);
int	memfs_read_indexdir(void *ns, void *cookie, long *num, struct dirent *buf, size_t bufsize);
int	memfs_create_index(void *ns, const char *name, int type, int flags);
int	memfs_remove_index(void *ns, const char *name);
int	memfs_rename_index(void *ns, const char *oldname, const char *newname);
int	memfs_stat_index(void *ns, const char *name, struct index_info *buf);
int	memfs_open_query(void *ns, const char *query, ulong flags, port_id port, long token, void **cookie);
int	memfs_close_query(void *ns, void *cookie);
int	memfs_read_query(void *ns, void *cookie, long *num, struct dirent *buf, size_t bufsize);
#endif

/* ******************************************************************************* */
/* local functions */
/* ******************************************************************************* */

/* lifted from bfs/file.c */
/* return:	0 = ok
 * 			1 = nope
 */
static int check_permission( vnode *v, int omode )
{
	unsigned int  access_type, mask;
	mode_t        mode = v->mode;
	uid_t         uid;
	gid_t         gid;

	uid = geteuid();
	gid = getegid();

	if ( uid == 0 )
		return 0;

	mask = (omode & O_ACCMODE);

	if (mask == O_RDONLY)
		access_type = S_IROTH;
	else if (mask == O_WRONLY)
		access_type = S_IWOTH;
	else if (mask == O_RDWR)
		access_type = (S_IROTH | S_IWOTH);
	else {
		dprintf("strange looking omode 0x%x\n", omode);
		//access_type = S_IROTH;
		access_type = (S_IROTH | S_IWOTH); /* Default is Read-Write */
	}

    if (uid == v->uid)
        mode >>= 6; /* check only user bits */
    else if (gid == v->gid)
        mode >>= 3; /* check only group bits */

    if ( (mode & access_type & S_IRWXO) == access_type )
        return 0;

	return 1;
}

/* ******************************************************************************* */

/*	malloc_vnode :
	- parent must be set externally (set to self by default)
*/
static int malloc_vnode( memfs_info *info, const char *name, vnode **node)
{
	vnode *tn;
	char buf[B_OS_NAME_LENGTH];

#if DEBUG_MEMFS
	dprintf( "*** memfs: malloc_vnode\n" );
#endif

	if(info->size + sizeof(vnode) >= info->max_size)
		return ENOSPC;
	
	tn = (vnode *)malloc(sizeof(vnode));
	if(tn == NULL)
		return ENOMEM;
	
	tn->magic = MEMFS_NODE_MAGIC;
	if(name != NULL) {
		tn->name = (char *)malloc(strlen(name) + 1);
		if (tn->name == NULL) {
		 	free(tn);
			return ENOMEM;
		}
		strcpy( tn->name, name ); 
	} else {
		sprintf( buf, "[%p]", tn );		
		tn->name = (char *)malloc(strlen(buf) + 1);
		if(tn->name == NULL) {
		 	free(tn);
			return ENOMEM;
		}
		strcpy(tn->name, buf);
	}
	
	tn->nsid = info->nsid;
	tn->uid = getuid();
	tn->gid = getgid();
	tn->mode = 0;
#if 0
	if (is_dir) {
		tn->mode = DIR_MODE | S_IFDIR;	/* set this DIR, R, W, !PERM, ... */
	} else {
		tn->mode = FILE_MODE & (~S_IFDIR);
	}
#endif
	tn->flags = 0x0;
	tn->parent = tn;
	tn->next = NULL;
	tn->prev = NULL;
	tn->jar = NULL;
	tn->attr_head = NULL;
	tn->size = 0;
	memset(&tn->u, 0, sizeof(tn->u));
	
	tn->ctime = tn->mtime = time(NULL);

	{
		uint32 hashtable_slot;
		int err;

		// insert it into the global hashtable, getting a vnid while we're at it
		hashtable_slot = memfs_find_free_slot_in_hashtable(info, info->vnode_pgdir, info->vnode_page_full_bitmap);
		if(hashtable_slot == 0xffffffff) {
			free(tn->name);
			free(tn);
			return ENOMEM;
		}
		err = memfs_insert_into_hashtable(info, info->vnode_pgdir, info->vnode_page_full_bitmap, hashtable_slot, tn);
		if(err < B_OK) {
			free(tn->name);
			free(tn);
			return err;
		}
		tn->vnid = info->next_version++;
		tn->vnid = (tn->vnid << 32) + hashtable_slot;
	}

	/* put on head of global vnode list */
	tn->all_next = info->all_head;
	tn->all_prev = NULL;
	if(info->all_head != NULL)
		info->all_head->all_prev = tn;
	info->all_head = tn;

	info->vnode_count++;
	info->size += sizeof(vnode) + strlen(tn->name) + 1;

	*node = tn;
	return B_OK;
}

/*	free_vnode: a helper function to deallocate all of the resources used by a vnode 
*/
static int free_vnode(memfs_info *info, vnode *v, bool force, bool free_attributes)
{
	int free_size;

#if DEBUG_MEMFS
	dprintf( "*** memfs: free_vnode\n" );
#endif

	if (S_ISDIR(v->mode)) {  /* vnode directory not empty */
		if(!force && v->u.dir_head != NULL) {
			return ENOTEMPTY;
		}
	} else if(S_ISLNK(v->mode)) {
		if(v->u.symlink != NULL) {
			free_size += 1 + strlen( v->u.symlink );
			free(v->u.symlink);
		}
	} else if(S_ISREG(v->mode)) {
		memfs_set_file_size(info, v, 0);		
	}
	if(S_ISATTR(v->mode)) {
		// we're going to get freed, but we need to maintain v->prev & v->next
		// list consistency so at the big free during unmount the list doesn't
		// get corrupted as stuff is pseudo-randomly freed
		delete_from_dir(v->parent, v, MEMFS_ATTRDIR);
	}
	// free any attributes it has
	if(free_attributes) {
		while(v->attr_head != NULL) {
			vnode *attr = v->attr_head;
			v->attr_head = attr->next;
			free_vnode(info, attr, true, false);
		}
	}	
	free_size = 1 + strlen( v->name );
	free( v->name );

	if(!force) {
		if(v->next != NULL || v->prev != NULL)
			dprintf("free_vnode: vnode has real prev & next pointers!\n");
	}

	free_size += sizeof( vnode );

	memfs_remove_from_hashtable(info, info->vnode_pgdir, info->vnode_page_full_bitmap, v->vnid);

	if(info->all_head == v) {
		info->all_head = v->all_next;
		if(info->all_head != NULL)
			info->all_head->all_prev = NULL;
	} else {
		if(v->all_next != NULL)
			v->all_next->all_prev = v->all_prev;
		if(v->all_prev != NULL)
			v->all_prev->all_next = v->all_next;
	}

	info->vnode_count--;
	info->size -= free_size;

	free( v );

	return B_OK;
}

/* ******************************************************************************* */

/*  Note: these functions do not do any locking or clean up.  That
	must be done by the caller.  They only fiddle with dir lists.
*/

/*	Inserts a vnode into the "head" list of the parent directory in
	lexicographical order. The 'directory' is selected with the which_dir
	argument: regular directory or attribute directory.
*/
static int insert_into_dir(vnode *dir, vnode *node, int which_dir)
{
	vnode *v, *last_v;
	int cmp;
	vnode **dir_head = (which_dir == MEMFS_DIR) ? &dir->u.dir_head : &dir->attr_head;

	if(*dir_head == NULL) {
		*dir_head = node;
		node->prev = node->next = NULL;
	} else {
		for(v = *dir_head, last_v = NULL; v; v = v->next ) {
			cmp = strcmp(node->name, v->name);
			if(cmp == 0)
				return EEXIST;
			if(cmp < 0) {
				node->next = v;
				node->prev = v->prev;
				if(v->prev)
					v->prev->next = node;
				v->prev = node;
				if(v == *dir_head)
					*dir_head = node;
				break;
			}
			last_v = v;
		}

		if(v == NULL) { /* append to end of the list */
			last_v->next = node;
			node->prev = last_v;
		}
	}
	node->parent = dir;

	return B_OK;
}

/* removes a vnode from a dir's head list */
static int delete_from_dir(vnode *dir, vnode *v, int which_dir)
{
	struct file_info *cookie;
	vnode **dir_head = (which_dir == MEMFS_DIR) ? &dir->u.dir_head : &dir->attr_head;

	// search through the dir's cookie jar to make sure a
	// dir cookie isn't pointing at this entry
	cookie = dir->jar;
	while(cookie != NULL) {
		if(cookie->next == v) {
			cookie->next = v->next;
		}
		cookie = cookie->next_fc;
	}
	if(v == *dir_head) {
		*dir_head = v->next;
		if(*dir_head != NULL)
			(*dir_head)->prev = NULL;
	} else {
		if(v->next != NULL)
			v->next->prev = v->prev;
		if(v->prev != NULL)
			v->prev->next = v->next;
	}
	v->next = NULL;
	v->prev = NULL;
	return B_OK;
}

/* finds a vnode from a dir's head list, returns pointer to vnode or NULL */
static vnode *find_name_in_dir(vnode *dir, const char *name, int which_dir)
{
	vnode *v;
	int cmp;
	vnode *dir_head = (which_dir == MEMFS_DIR) ? dir->u.dir_head : dir->attr_head;

	if(dir_head != NULL) {
		for(v = dir_head; v; v = v->next) {
			cmp = strcmp(v->name, name);
			if(cmp == 0 ) {
				return v;
			} else if(cmp > 0) {
				return NULL;
			}
		}
	}
	return NULL;
}

/* ******************************************************************************* */
/* data allocation operations */

#define GET_BIT(a, b) ((a) & (1 << (b)))
#define SET_BIT(a, b) ((a) | (1 << (b)))
#define CLEAR_BIT(a, b) ((a) & ~(1 << (b)))

static int
memfs_allocate_block(memfs_info *info, uint32 *block)
{
	uint32 i;
	uint8 *b = info->area_bitmap;

	// This table stores the first zero in a particular nibble
	static const uint8 bit_table[] = {
		0, 1, 0, 2, 0, 1, 0, 3,
		0, 1, 0, 2, 0, 1, 0
	};
	
	// see if we'll have enough space
	if(info->size + B_PAGE_SIZE > info->max_size)
		return ENOSPC;
	
	for(i = 0; i < info->area_size / 8; i++, b++) {
		if(*b == 0xff) continue;
		if((*b & 0x0f) != 0x0f) {
			*block = i * 8 + bit_table[*b & 0x0f];
		} else {
			*block = i * 8 + 4 + bit_table[*b >> 4];
		}			
		if(*block >= info->area_size) return ENOSPC;
		*b = SET_BIT(*b, *block % 8);
		info->size += B_PAGE_SIZE;
		return B_OK;
	}
	return ENOSPC;
}

static int
memfs_allocate_specific_block(memfs_info *info, uint32 block)
{
	uint8 *b = &info->area_bitmap[block / 8];
	
	if(GET_BIT(*b, block % 8))
		return ENOSPC; // already allocated	
	*b = SET_BIT(*b, block % 8);
	info->size += B_PAGE_SIZE;
	return B_OK;
}

static int
memfs_free_block(memfs_info *info, uint32 block)
{
	uint8 *b = &info->area_bitmap[block / 8];
	
	if(GET_BIT(*b, block % 8))
		info->size -= B_PAGE_SIZE;
	*b = CLEAR_BIT(*b, block % 8);
	return B_OK;
}

static uint8 *
memfs_block_to_addr(memfs_info *info, uint32 block)
{
	return (info->area_ptr + block * B_PAGE_SIZE);
}

static uint8 *
memfs_frag_block_to_addr(memfs_info *info, uint32 frag)
{
	return (info->area_ptr + frag * FRAG_SIZE);
}

static void *
memfs_lookup_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint32 key)
{
	uint32 *pgdir;
	void   **pgtable;

	if(key > PAGE_TABLE_SIZE*PAGE_TABLE_SIZE)
		return NULL; 	// key maps beyond where the hashtable can point

	pgdir = (uint32 *)memfs_block_to_addr(info, hash_table_pgdir);
	if(pgdir[key / PAGE_TABLE_SIZE] != 0) {
		pgtable = (void **)memfs_block_to_addr(info, pgdir[key / PAGE_TABLE_SIZE]);
		return pgtable[key & (PAGE_TABLE_SIZE-1)];
	}
	return NULL;
}

static int
memfs_insert_into_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap, uint32 key, void *ptr)
{
	uint32 *pgdir;
	int    pgdir_index;
	void   **pgtable;
	int err;

	if(key > PAGE_TABLE_SIZE*PAGE_TABLE_SIZE)
		return EINVAL; 	// key maps beyond where the hashtable can point

	pgdir = (uint32 *)memfs_block_to_addr(info, hash_table_pgdir);
	pgdir_index = key / PAGE_TABLE_SIZE;
	if(pgdir[pgdir_index] == 0) {
		uint32 pgtable_block;
		
		// we need to allocate a pagetable
		err = memfs_allocate_block(info, &pgtable_block);
		if(err < B_OK)
			return err;
		pgdir[pgdir_index] = pgtable_block;
		info->size += B_PAGE_SIZE;

		pgtable = (void **)memfs_block_to_addr(info, pgtable_block);
		memset(pgtable, 0, B_PAGE_SIZE);

		// make sure the pagetable full bit is clear for this one
		pgtable_full_bitmap[pgdir_index] = CLEAR_BIT(pgtable_full_bitmap[pgdir_index / 8], pgdir_index % 8);
	} else {
		pgtable = (void **)memfs_block_to_addr(info, pgdir[pgdir_index]);
	}

	pgtable[key & (PAGE_TABLE_SIZE-1)] = ptr;

	return B_OK;
}

static int
memfs_remove_from_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap, uint32 key)
{
	uint32 *pgdir;
	int    pgdir_index;
	void   **pgtable;
	int i;

	if(key > PAGE_TABLE_SIZE*PAGE_TABLE_SIZE)
		return EINVAL; 	// key maps beyond where the hashtable can point

	pgdir = (uint32 *)memfs_block_to_addr(info, hash_table_pgdir);
	pgdir_index = key / PAGE_TABLE_SIZE;
	if(pgdir[pgdir_index] == 0)
		return EINVAL;
		
	pgtable = (void **)memfs_block_to_addr(info, pgdir[pgdir_index]);
	pgtable[key & (PAGE_TABLE_SIZE-1)] = NULL;
	pgtable_full_bitmap[pgdir_index] = CLEAR_BIT(pgtable_full_bitmap[pgdir_index / 8], pgdir_index % 8);

	// scan through this pagetable and see if it's empty
	// free it if it is
	for(i=0; i < PAGE_TABLE_SIZE; i++) {
		if(pgtable[i] != NULL) {
			return B_OK;
		}
	}
	memfs_free_block(info, pgdir[pgdir_index]);
	info->size -= B_PAGE_SIZE;
	pgdir[pgdir_index] = 0;
	return B_OK;
}

static uint32
memfs_find_free_slot_in_hashtable(memfs_info *info, uint32 hash_table_pgdir, uint8 *pgtable_full_bitmap)
{
	int i, j;
	int first_empty_pagetable = -1;
	uint32 *pgdir;
	void   **pgtable;

	pgdir = (uint32 *)memfs_block_to_addr(info, hash_table_pgdir);
	for(i=0; i < PAGE_TABLE_SIZE; i++) {
		if(pgdir[i] == 0) {
			if(first_empty_pagetable == -1)
				first_empty_pagetable = i;
			continue;
		}
		if(GET_BIT(info->vnode_page_full_bitmap[i/8], i % 8) == 1)
			continue;
		pgtable = (void **)memfs_block_to_addr(info, pgdir[i]);
		for(j=0; j < PAGE_TABLE_SIZE; j++) {
			if(pgtable[j] == NULL)
				return i * PAGE_TABLE_SIZE + j;
		}
		// we walked entirely through this pagetable, so set it's bit in the
		// pagetable full bitmap so we dont search it again next time
		pgtable_full_bitmap[i/8] = SET_BIT(pgtable_full_bitmap[i/8], i % 8);
	}
	
	if(first_empty_pagetable != -1) {
		// we didn't find any pagetables that weren't full, but we found a free
		// place to stick a new one
		return first_empty_pagetable * PAGE_TABLE_SIZE;
	}

	return 0xffffffff;
}

static void
memfs_insert_frag_into_global_list(memfs_info *info, struct frag_desc *frag,
	struct frag_desc **_last_frag, struct frag_desc **_next_frag)
{
	struct frag_desc *next_frag = NULL;
	struct frag_desc *last_frag = NULL;

	if(_next_frag != NULL)
		next_frag = *_next_frag;
	if(_last_frag != NULL)
		last_frag = *_last_frag;
	
	if(next_frag == NULL && last_frag == NULL) {
		// search for the spot
		for(next_frag = info->all_frags; next_frag != NULL; next_frag = next_frag->next) {
			if(next_frag->start > frag->start) break;
			last_frag = next_frag;
		}
	} else if(next_frag != NULL) {
		// calculate last_frag from this
		last_frag = next_frag->prev;
	} else {
		// last_frag != NULL
		next_frag = last_frag->next;
	}
	
	frag->prev = last_frag;
	frag->next = next_frag;
	if(next_frag != NULL)
		next_frag->prev = frag;
	if(last_frag != NULL)
		last_frag->next = frag;
	if(info->all_frags == next_frag)
		info->all_frags = frag;

	// pass the search locations back if the caller wants em
	if(_last_frag != NULL)
		*_last_frag = last_frag;
	if(_next_frag != NULL)
		*_next_frag = next_frag;
}

static void
memfs_remove_frag_from_global_list(memfs_info *info, struct frag_desc *frag)
{
	if(frag->prev != NULL)
		frag->prev->next = frag->next;
	if(frag->next != NULL)
		frag->next->prev = frag->prev;
	if(info->all_frags == frag)
		info->all_frags = frag->next;
}

static void
memfs_insert_frag_into_unused_list(memfs_info *info, struct frag_desc *frag)
{
	struct frag_desc *spot, *last;	
	
	last = NULL;
	for(spot = info->unused_frags; spot != NULL; spot = spot->next_unused) {
		if(spot->start > frag->start + frag->len) break;
		last = spot;
	}

	frag->next_unused = spot;
	if(last != NULL)
		last->next_unused = frag;
	else
		info->unused_frags = frag;
}

static int
memfs_allocate_frag(memfs_info *info, vnode *v, size_t num_bytes_needed)
{
	int err;
	uint32 num_frags_needed = ROUNDUP(num_bytes_needed, FRAG_SIZE) / FRAG_SIZE;
	struct frag_desc *last;
	struct frag_desc *frag;
	struct frag_desc *alloc_hint_frag0 = NULL;
	struct frag_desc *alloc_hint_frag0_last = NULL;
	struct frag_desc *alloc_hint_frag1 = NULL;
	struct frag_desc *alloc_hint_frag1_last = NULL;
	
	verify_frag_list(info);
	
	D(("entry: num_bytes_needed = %ld\n", num_bytes_needed));

	// first, look for a unused frag that is large enough for this
	last = NULL;
	for(frag = info->unused_frags; frag != NULL; frag = frag->next_unused) {
		if(alloc_hint_frag0 == NULL && frag->start % FRAG_SIZE == 0) {
			// this frag is aligned on a page boundary. excellent place to potentially
			// have a new block allocated immediately before
			alloc_hint_frag0 = frag;
			alloc_hint_frag0_last = last;
		}
		if(alloc_hint_frag1 == NULL && (frag->start + frag->len) % FRAG_SIZE == 0) {
			// this frag ends on a page boundary, another excellent place to have a new
			// block allocated immediately after
			alloc_hint_frag1 = frag;
			alloc_hint_frag1_last = last;
		}
		if(frag->len >= num_frags_needed)
			break;
		last = frag;
	}	

	D(("done walking: last = %p, frag = %p\n", last, frag));

restart:	
	D(("after restart: last = %p, frag = %p\n", last, frag));
	if(frag != NULL) {
		D(("frag != NULL\n"));
		// we already have a frag that covers this much space
		if(num_frags_needed != frag->len) {
			// we need to split it
			struct frag_desc *old_frag = frag;
			
			D(("allocating frag\n"));
			
			frag = (struct frag_desc *)malloc(sizeof(struct frag_desc));
			if(frag == NULL)
				return ENOMEM;
			info->size += sizeof(struct frag_desc);
			
			frag->start = old_frag->start;
			frag->len = num_frags_needed;
			frag->next_unused = NULL;
			frag->owner = v;
			
			D(("frag %p: start %ld, len %ld, next_unused %p, owner %p\n",
				frag, frag->start, frag->len, frag->next_unused, frag->owner));
			
			old_frag->start += num_frags_needed;
			old_frag->len -= num_frags_needed;
			
			D(("old_frag %p: start %ld, len %ld, next_unused %p, owner %p\n",
				old_frag, old_frag->start, old_frag->len, old_frag->next_unused, old_frag->owner));

			memfs_insert_frag_into_global_list(info, frag, NULL, &old_frag);
			D(("frag inserted into global list\n"));
		} else {
			D(("frag being deleted\n"));
			// we're eating the entire frag
			// remove this frag from the unused list
			if(last != NULL) {
				last->next_unused = frag->next_unused;
				D(("last->next_unused = %p (frag->next_unused)\n", last->next_unused));
			} else {
				info->unused_frags = frag->next_unused;
				D(("info->unused_frags = %p (frag->next_unused)\n", info->unused_frags));
			}
			frag->owner = v;
			frag->next_unused = NULL;
			D(("frag->owner = %p (v)\nfrag->next_unused = NULL\n", frag->owner));
		}
	} else {
		// we need to allocate a frag
		uint32 frag_block;
		struct frag_desc *next;
		
		D(("frag == NULL, need to allocate a new one\n"));
		
		err = B_ERROR;
		// XXX search through the list for more 'hint' blocks. Maybe forget the idea of using hint
		// blocks altogether and just search here for good spots and try all of em
		if(alloc_hint_frag0 != NULL) {
			if(alloc_hint_frag0->start > 0) {
				frag_block = (alloc_hint_frag0->start / NUM_FRAGS_PER_BLOCK) - 1;
				err = memfs_allocate_specific_block(info, frag_block);
				if(err == B_OK) {
					// we should be able to pull the start of the frag pointed to by alloc_hint_frag0
					// back by NUM_FRAGS_PER_BLOCK to have it cover the newly allocated range. Then we
					// can just start over at the beginning of the algorithm and use that as the target.
					alloc_hint_frag0->start -= NUM_FRAGS_PER_BLOCK;
					alloc_hint_frag0->len   += NUM_FRAGS_PER_BLOCK;
					frag = alloc_hint_frag0;
					last = alloc_hint_frag0_last;
					D(("alloc_hint_frag0\n"));
					D(("allocated block %ld\n", frag_block));
					goto restart;
				}
			}
		} else if(alloc_hint_frag1 != NULL) {
			frag_block = (alloc_hint_frag1->start / NUM_FRAGS_PER_BLOCK) + 1;
			if(frag_block < info->area_size) {
				err = memfs_allocate_specific_block(info, frag_block);
				if(err == B_OK) {
					// at this point, we should be able to extend the frag pointed to by
					// alloc_hint_frag1 by NUM_FRAGS_PER_BLOCK frags. Then we can just
					// start over at the beginning of the algorithm and use that as the target.
					alloc_hint_frag1->len += NUM_FRAGS_PER_BLOCK;
					frag = alloc_hint_frag1;
					last = alloc_hint_frag1_last;
					D(("alloc_hint_frag1\n"));
					D(("allocated block %ld\n", frag_block));
					goto restart;
				}
			}
		}
		
		if(err < B_OK) {
			// either there wasn't any hints or there were and they didn't get the
			// block they wanted allocated, so we just grab a random block	
			err = memfs_allocate_block(info, &frag_block);
		}
		if(err < 0)
			return err;

		D(("allocated block %ld\n", frag_block));
			
		// we now need to build a frag to represent the used part of this block
		frag = (struct frag_desc *)malloc(sizeof(struct frag_desc));
		if(frag == NULL) {
			memfs_free_block(info, frag_block);
			return ENOMEM;
		}
		info->size += sizeof(struct frag_desc);
		
		frag->start = frag_block * NUM_FRAGS_PER_BLOCK;
		frag->len = num_frags_needed;
		frag->next_unused = NULL;
		frag->owner = v;

		D(("frag %p: start %ld, len %ld, next_unused %p, owner %p\n",
			frag, frag->start, frag->len, frag->next_unused, frag->owner));
		
		next = NULL;
		memfs_insert_frag_into_global_list(info, frag, NULL, &next);

		D(("inserted into global list, next = %p\n", next));

		// now we know the frag block immediately after where this one was inserted
		// if it's a free block, we can take it and expand it backwards to cover the
		// free space in the rest of this allocated block, otherwise, we create another
		// frag descriptor to cover the rest of the allocated block.
		if(next != NULL && next->owner == NULL && next->start == frag->start + NUM_FRAGS_PER_BLOCK) {
			// expand the next frag descriptor, which is free, to cover the rest of this block
			next->start -= NUM_FRAGS_PER_BLOCK - frag->len;
			next->len   += NUM_FRAGS_PER_BLOCK - frag->len;
			D(("next->start = %ld\nnext->len = %ld\n", next->start, next->len));
		} else if((frag->start + frag->len) % NUM_FRAGS_PER_BLOCK != 0) {
			// create a new frag descriptor to cover the unused rest of the block, and insert
			// it into the unused list
			struct frag_desc *unused_frag;
			
			unused_frag = (struct frag_desc *)malloc(sizeof(struct frag_desc));
			if(unused_frag == NULL) {
				memfs_remove_frag_from_global_list(info, frag);
				memfs_free_block(info, frag_block);
				return ENOMEM;
			}
			info->size += sizeof(struct frag_desc);
			
			unused_frag->start = frag->start + frag->len;
			unused_frag->len = NUM_FRAGS_PER_BLOCK - frag->len;
			unused_frag->owner = NULL;
			
			D(("unused_frag %p: start %ld, len %ld, next_unused %p, owner %p\n",
				unused_frag, unused_frag->start, unused_frag->len, unused_frag->next_unused, unused_frag->owner));			

			memfs_insert_frag_into_global_list(info, unused_frag, &frag, NULL);
			
			D(("unused frag in global list\n"));
			// put it in the unused list
			memfs_insert_frag_into_unused_list(info, unused_frag);
			D(("unused frag in unused list\n"));
		} else {
			// the frag block ate all of this allocated space
		}
	}

	v->u.data.last_frag = frag;
	D(("v->u.data.last_frag = %p (frag)\n", frag));

	verify_frag_list(info);

	return B_OK;	
}

static int
memfs_deallocate_frag(memfs_info *info, struct frag_desc *frag)
{
	struct frag_desc *last = NULL;
	struct frag_desc *last_last = NULL;
	struct frag_desc *next;
	bool frag_merged = false;

	verify_frag_list(info);

	D(("entry: frag %p\n", frag));

	// look for it's new spot in the unused list
	next = info->unused_frags;
	while(next != NULL) {
		if(next->start > frag->start)
			break;	
		last_last = last;	
		last = next;
		next = next->next_unused;	
	}
	
	D(("last_last = %p, last = %p, next = %p\n", last_last, last, next));
	
	// see if it's exactly one block long, in which case we can just free it
	if(frag->start % NUM_FRAGS_PER_BLOCK == 0 && frag->len == NUM_FRAGS_PER_BLOCK) {
		
		D(("one block long, freeing space at block %ld\n", frag->start / NUM_FRAGS_PER_BLOCK));
		
		memfs_free_block(info, frag->start / NUM_FRAGS_PER_BLOCK);
		memfs_remove_frag_from_global_list(info, frag);
		free(frag);
		info->size -= sizeof(struct frag_desc);
		verify_frag_list(info);
		return B_OK;
	}
	
	// see if it can be coalesced with the next pointer
	if(next != NULL && next->start == frag->start + frag->len) {
		next->start = frag->start;
		next->len  += frag->len;
	
		D(("can be coalesced with next\n"));
		D(("next->start = %ld (frag->start), next->len = %ld +(frag->len)\n", next->start, next->len));
	
		memfs_remove_frag_from_global_list(info, frag);
		
		free(frag);
		info->size -= sizeof(struct frag_desc);

		D(("freed frag\n"));

		frag = next;
		frag_merged = true;
	
		D(("frag = %p (next)\n", frag));
	
		// see if we now cover an entire block. if so, free it or shorten it.
		// we are guaranteed to not cover two or more blocks
		{
			uint32 frag_end    = frag->start + frag->len;
			uint32 start_block = frag->start / NUM_FRAGS_PER_BLOCK;
			uint32 stop_block  = frag_end / NUM_FRAGS_PER_BLOCK;
		
			D(("start_block = %ld, stop_block = %ld\n", start_block, stop_block));
		
			if((stop_block - start_block >= 2) ||
				(stop_block - start_block == 1 && frag->start % NUM_FRAGS_PER_BLOCK == 0)) {
				// the block may be split into at most two seperate blocks
				struct frag_desc *second = NULL;
				uint32 block_to_free;
				
				D(("splitting block\n"));
				
				if(frag->start % NUM_FRAGS_PER_BLOCK == 0) {
					block_to_free = start_block;
				} else if(frag_end % NUM_FRAGS_PER_BLOCK == 0) {
					block_to_free = stop_block - 1;
				} else {
					block_to_free = start_block + 1; // == stop_block - 1
				}
				
				// handle the second block first
				if(stop_block * NUM_FRAGS_PER_BLOCK < frag_end) {
					second = (struct frag_desc *)malloc(sizeof(struct frag_desc));
					if(second == NULL) {
						// uh oh, deep doo
						return ENOMEM;
					}
					info->size += sizeof(struct frag_desc);
					second->start = stop_block * NUM_FRAGS_PER_BLOCK;
					second->len   = frag_end - second->start;
					second->owner = NULL;
					second->next_unused = frag->next_unused;
					
					D(("second %p: start %ld, len %ld, next_unused %p, owner %p\n",
						second, second->start, second->len, second->next_unused, second->owner));			
					
					frag->next_unused = second;
					
					D(("frag->next_unused = %p (second)\n", frag->next_unused));
					memfs_insert_frag_into_global_list(info, second, &frag, NULL);
				}

				D(("freeing block %ld\n", block_to_free));
				memfs_free_block(info, block_to_free);

				// handle the first block
				if(start_block * NUM_FRAGS_PER_BLOCK != frag->start) {
					frag->len = NUM_FRAGS_PER_BLOCK - frag->start % NUM_FRAGS_PER_BLOCK;
					D(("frag->len = %ld\n", frag->len));
				} else {
					// the frag block needs to be killed
					if(last != NULL) {
						last->next_unused = frag->next_unused;
						D(("last->next_unused = %p (frag->next_unused)\n", last->next_unused));
					} else {
						info->unused_frags = frag->next_unused;
						D(("info->unused_frags = %p (frag->next_unused)\n", info->unused_frags));
					}
					memfs_remove_frag_from_global_list(info, frag);
					free(frag);
					info->size -= sizeof(struct frag_desc);
	
					// we've killed this block, and whether or not the second block was created,
					// no one is going to be able to join with the last block, so we can leave now
					D(("done here\n"));
					verify_frag_list(info);
					return B_OK;
				}
			}
		}
	}

	// see if it can be coalesced with the prev pointer
	if(last != NULL && last->start + last->len == frag->start) {
		last->len += frag->len;

		D(("can be coalesced with last\n"));
		D(("last->len = %ld (frag->len)\n", last->len));
				
		if(frag == next) {
			// it was already coalesced with next, so we're eating
			// the next frag at this state, so we gotta update the last's
			// next pointer to reflect this
			last->next_unused = frag->next_unused;
			D(("last->next_unused = %p (frag->next_unused)\n", frag->next_unused));
		}

		memfs_remove_frag_from_global_list(info, frag);
		
		free(frag);
		info->size -= sizeof(struct frag_desc);

		frag = last;
		frag_merged = true;

		D(("frag = %p (last)\n", frag));

		// see if we now cover an entire block. if so, free it or shorten it.
		// we are guaranteed to not cover two or more blocks
		{
			uint32 frag_end    = frag->start + frag->len;
			uint32 start_block = frag->start / NUM_FRAGS_PER_BLOCK;
			uint32 stop_block  = frag_end / NUM_FRAGS_PER_BLOCK;
		
			D(("start_block = %ld, stop_block = %ld\n", start_block, stop_block));
		
			if((stop_block - start_block >= 2) ||
				(stop_block - start_block == 1 && frag->start % NUM_FRAGS_PER_BLOCK == 0)) {
				// the block may be split into at most two seperate blocks
				struct frag_desc *second = NULL;
				uint32 block_to_free;
				
				D(("splitting block\n"));
				
				if(frag->start % NUM_FRAGS_PER_BLOCK == 0) {
					block_to_free = start_block;
				} else if(frag_end % NUM_FRAGS_PER_BLOCK == 0) {
					block_to_free = stop_block - 1;
				} else {
					block_to_free = start_block + 1; // == stop_block - 1
				}
				
				// handle the second block first
				if(stop_block * NUM_FRAGS_PER_BLOCK != frag->start + frag->len) {
					second = (struct frag_desc *)malloc(sizeof(struct frag_desc));
					if(second == NULL) {
						// uh oh, deep doo
						return ENOMEM;
					}
					info->size += sizeof(struct frag_desc);
					second->start = stop_block * NUM_FRAGS_PER_BLOCK;
					second->len   = frag_end - second->start;
					second->owner = NULL;
					second->next_unused = frag->next_unused;

					D(("second %p: start %ld, len %ld, next_unused %p, owner %p\n",
						second, second->start, second->len, second->next_unused, second->owner));			
					
					frag->next_unused = second;

					D(("frag->next_unused = %p (second)\n", frag->next_unused));
					memfs_insert_frag_into_global_list(info, second, &frag, NULL);
				}

				D(("freeing block %ld\n", block_to_free));
				memfs_free_block(info, block_to_free);

				// handle the first block
				if(start_block * NUM_FRAGS_PER_BLOCK != frag->start) {
					frag->len = NUM_FRAGS_PER_BLOCK - frag->start % NUM_FRAGS_PER_BLOCK;
					D(("frag->len = %ld\n", frag->len));
				} else {
					// the frag block needs to be killed
					if(last_last != NULL) {
						last_last->next_unused = frag->next_unused;
						D(("last_last->next_unused = %p (frag->next_unused)\n", last_last->next_unused));
					} else {
						info->unused_frags = frag->next_unused;
						D(("info->unused_frags = %p (frag->next_unused)\n", info->unused_frags));
					}
					memfs_remove_frag_from_global_list(info, frag);
					free(frag);
					info->size -= sizeof(struct frag_desc);
				}
			}
		}
	}

	if(frag_merged == false) {
		// we'll need to insert this into the unused list
		D(("not merged\n"));
		if(last != NULL) {
			last->next_unused = frag;
			D(("last->next_unused = %p (frag)\n", last->next_unused));
		} else {
			info->unused_frags = frag;
			D(("info->unused_frags = %p (frag)\n", info->unused_frags));
		}
		frag->next_unused = next;
		frag->owner = NULL;
		D(("frag->next_unused = %p (next), frag->owner = NULL\n", frag->next_unused));
	}

	verify_frag_list(info);

	return B_OK;
}

static int
memfs_add_new_run(memfs_info *info, vnode *v, struct file_run_container **last_runc,
	struct file_run_container **runc, uint32 start_block, uint32 len)
{
	if(*runc != NULL && (*runc)->num_runs >= RUN_CONTAINER_SIZE) {
		*last_runc = *runc;
		*runc = NULL;
	}
	
	// decide if we need to allocate a new block
	if(*runc == NULL) {
		// we will need to allocate a new block
		if(info->size + sizeof(struct file_run_container) > info->max_size) {
			// no more space
			return ENOSPC;
		}
		// we need to allocate a new run block
		*runc = (struct file_run_container *)malloc(sizeof(struct file_run_container));
		if(*runc == NULL)
			return ENOMEM;
	
		(*runc)->next = NULL;
		(*runc)->num_runs = 0;

		if(*last_runc != NULL) {
			(*last_runc)->next = *runc;
		} else {
			v->u.data.run_head = *runc;
		}
		info->size += sizeof(struct file_run_container);
	}
	
	// set up the new run
	(*runc)->run[(*runc)->num_runs].block = start_block;
	(*runc)->run[(*runc)->num_runs].len = len;
	(*runc)->num_runs++;
	
	return B_OK;
}

static int
memfs_expand_file(memfs_info *info, vnode *v, size_t new_size)
{	
	size_t old_size = v->size;
	int i;
	int num_blocks_to_add;
	uint32 new_block;
	int err = B_OK;
	size_t new_fragment_size = new_size % B_PAGE_SIZE;
	size_t old_fragment_size = old_size % B_PAGE_SIZE;
	
	// calculate how many blocks we need to add to the file (not counting last fragment)
	num_blocks_to_add = new_size / B_PAGE_SIZE - old_size / B_PAGE_SIZE;
	if(num_blocks_to_add > 0) {
		struct file_run_container *runc;
		struct file_run_container *last_runc = NULL;

		if(info->area < 0) {
			// there is no area to allocate blocks from
			err = ENOSPC;
			goto err;
		}

		// walk the run container up to the end of the file
		runc = v->u.data.run_head;		
		while(runc != NULL) {
			if(runc->next == NULL) break;
			last_runc = runc;
			runc = runc->next;
		}
		// start allocating blocks
		for(i = 0; i < num_blocks_to_add; i++) {
			err = memfs_allocate_block(info, &new_block);
			if(err != B_OK) 
				goto err;
			
			if(runc == NULL || new_block != runc->run[runc->num_runs-1].block + runc->run[runc->num_runs-1].len) {
				// either the new block doesn't extend the current run, or we need one anyway
				err = memfs_add_new_run(info, v, &last_runc, &runc, new_block, 1);
				if(err < B_OK) {
					// undo allocating the block
					memfs_free_block(info, new_block);
					goto err;
				}
			} else {
				// we're extending the current run
				runc->run[runc->num_runs-1].len++;
			}		
			v->size += B_PAGE_SIZE;
			
			if(i == 0 && v->u.data.frag_size > 0) {
				// we've just added the first full block, so copy any partial frag data
				memcpy(memfs_block_to_addr(info, new_block),
					memfs_frag_block_to_addr(info, v->u.data.last_frag->start), v->u.data.frag_size);
			}				
		}
	}
	
	// deal with the partial file fragment
	if(FRAG_SIZE_ROUNDUP(old_fragment_size) != FRAG_SIZE_ROUNDUP(new_fragment_size)) {
		if(new_fragment_size > 0) {			
			if(v->u.data.last_frag != NULL) {
				// we need to allocate a new frag, copy the data over, and deallocate the old frag
				struct frag_desc *old_frag;			
				
				old_frag = v->u.data.last_frag;
				// XXX should try to expand the current frag first
				err = memfs_allocate_frag(info, v, new_fragment_size);
				if(err < B_OK)
					goto err;
				
				if(num_blocks_to_add == 0) {
					// in this case, the data in the old frag has not been copied anywhere,
					// so we gotta do it ourselves.
					// new frag should be in v->u.data.last_frag
					memcpy(memfs_frag_block_to_addr(info, v->u.data.last_frag->start),
						memfs_frag_block_to_addr(info, old_frag->start),
						min(old_fragment_size, new_fragment_size));
				}
			
				// deallocate the old frag
				memfs_deallocate_frag(info, old_frag);
			} else {
				// there was no frag before, so just allocate a new one
				err = memfs_allocate_frag(info, v, new_fragment_size);
				if(err < B_OK)
					goto err;
			}				
		} else {
			// just free the old frag
			memfs_deallocate_frag(info, v->u.data.last_frag);
			v->u.data.last_frag = NULL;
		}
	}
	v->u.data.frag_size = new_fragment_size;		
	v->size += new_fragment_size - old_fragment_size;

err:
	return err;
}

static int
memfs_truncate_file(memfs_info *info, vnode *v, size_t new_size)
{
	size_t old_size = v->size;
	struct file_run_container *last_runc = NULL;
	struct file_run_container *runc;	
	int num_blocks_to_remove;
	int err = B_OK;
	size_t new_fragment_size = new_size % B_PAGE_SIZE;
	size_t old_fragment_size = old_size % B_PAGE_SIZE;

	// calculate how many blocks we need to remove from the file (not counting last fragment)
	num_blocks_to_remove = old_size / B_PAGE_SIZE - new_size / B_PAGE_SIZE;	
	if(num_blocks_to_remove > 0) {
		struct file_run_container *temp_runc;
		uint32 target_block;
		uint32 curr_block = 0;
		int run = 0;
		int run_pos = 0;
		int i, j;
	
		// walk the run container up to the spot where we will start removing blocks
		target_block = new_size / B_PAGE_SIZE;		
		runc = v->u.data.run_head;		
		while(runc != NULL) {
			for(run = 0; run < runc->num_runs; run++) {
				if(curr_block + runc->run[run].len > target_block) {
					// we've found the spot
					run_pos = target_block - curr_block;
					goto outsearch;
				}
				curr_block += runc->run[run].len;
			}
			last_runc = runc;
			runc = runc->next;
		}
outsearch:
		if(runc == NULL) {
			// how did we get here
			dprintf("memfs_truncate_file: error finding truncation spot\n");
			err = B_ERROR;
			goto err;
		}
		
		// deal with moving data from the first block to be axed
		// to a possible partial block
		if(new_fragment_size > 0) {
			if(FRAG_SIZE_ROUNDUP(new_fragment_size) != FRAG_SIZE_ROUNDUP(old_fragment_size)) {
				if(v->u.data.last_frag != NULL) {
					memfs_deallocate_frag(info, v->u.data.last_frag);
					v->u.data.last_frag = NULL;
				}
				err = memfs_allocate_frag(info, v, new_fragment_size);
				if(err < B_OK) {
					v->size -= old_fragment_size;
					v->u.data.frag_size = 0;
					goto err;
				}
			}
			memcpy(memfs_frag_block_to_addr(info, v->u.data.last_frag->start),
				memfs_block_to_addr(info, runc->run[run].block + run_pos), new_fragment_size);
		} else {
			if(v->u.data.last_frag != NULL) {
				memfs_deallocate_frag(info, v->u.data.last_frag);
				v->u.data.last_frag = NULL;
			}
		}
		v->u.data.frag_size = new_fragment_size;
		v->size += new_fragment_size - old_fragment_size;	

		// remove blocks from the end of the file

		// take care of the rest of the run we're starting in
		if(run_pos > 0 || run > 0) {
			for(i = runc->run[run].len - 1; i >= run_pos; i--) {
				err = memfs_free_block(info, runc->run[run].block + i);
				if(err < B_OK)
					goto err;
				runc->run[run].len--;
				v->size -= B_PAGE_SIZE;
			}
	
			// erase the end runs from the run container we're pointed at
			for(i = runc->num_runs - 1; i > run; i--) {
				for(j = runc->run[i].len - 1; j >= 0; j--) {
					err = memfs_free_block(info, runc->run[i].block + j);
					if(err < B_OK)
						goto err;
					runc->run[i].len--;
					v->size -= B_PAGE_SIZE;
				}
				runc->num_runs--;
			}
		
			last_runc = runc;
			runc = runc->next;
		}
		
		// free the rest of the run containers
		while(runc != NULL) {
			for(i = runc->num_runs - 1; i >= 0; i--) {
				for(j = runc->run[i].len - 1; j >= 0; j--) {
					err = memfs_free_block(info, runc->run[i].block + j);
					if(err < B_OK)
						goto err;
					runc->run[i].len--;
					v->size -= B_PAGE_SIZE;
				}
				runc->num_runs--;
			}
			temp_runc = runc->next;
			if(last_runc == NULL) {
				v->u.data.run_head = temp_runc;
			} else {
				last_runc->next = temp_runc;
			}
			free(runc);
			info->size -= sizeof(struct file_run_container);
			runc = temp_runc;
		}
	} else {
		// we just need to resize the last block
		if(FRAG_SIZE_ROUNDUP(new_fragment_size) != FRAG_SIZE_ROUNDUP(old_fragment_size)) {
			if(FRAG_SIZE_ROUNDUP(new_fragment_size) > 0) {
				// we need to allocate a new frag, copy the data over, and deallocate the old frag
				struct frag_desc *old_frag;			
				
				old_frag = v->u.data.last_frag;
				// XXX should try to expand the current frag first
				err = memfs_allocate_frag(info, v, new_fragment_size);
				if(err < B_OK)
					goto err;
				
				memcpy(memfs_frag_block_to_addr(info, v->u.data.last_frag->start),
					memfs_frag_block_to_addr(info, old_frag->start),
					min(old_fragment_size, new_fragment_size));
			
				// deallocate the old frag
				memfs_deallocate_frag(info, old_frag);
			} else {
				memfs_deallocate_frag(info, v->u.data.last_frag);
				v->u.data.last_frag = NULL;
			}
		}
		v->u.data.frag_size = new_fragment_size;
		v->size += new_fragment_size - old_fragment_size;	
	}

err:
	return err;
}

static int
memfs_set_file_size(memfs_info *info, vnode *v, size_t new_size)
{
	if(new_size == v->size)
		return B_OK;
	else if(new_size > v->size)
		return memfs_expand_file(info, v, new_size);
	else
		return memfs_truncate_file(info, v, new_size);
}

static int memfs_read_write_file(memfs_info *info, vnode *v, char *buffer, off_t pos, size_t *len, int op)
{
	size_t bufsize = *len;
	int err = B_OK;
	
	*len = 0;
	if(pos < (v->size / B_PAGE_SIZE) * B_PAGE_SIZE) {
		// handle copying data with full blocks
		struct file_run_container *runc;
		uint32 run;
		uint32 run_pos = 0;
		uint32 target_block;
		uint32 curr_block = 0;		
		
		target_block = pos / B_PAGE_SIZE;	
		// find the location in the runlist where we're starting
		runc = v->u.data.run_head;		
		while(runc != NULL) {
			for(run = 0; run < runc->num_runs; run++) {
				if(curr_block + runc->run[run].len > target_block) {
					// we've found the spot
					run_pos = target_block - curr_block;
					goto outsearch;
				}
				curr_block += runc->run[run].len;
			}
			runc = runc->next;
		}			
outsearch:
		if(runc == NULL) {
			dprintf("memfs_read_write_file: error finding %s spot in runlist\n", op == MEMFS_READ ? "read" : "write");
			err = EIO;
			goto err;
		}

		// iterate over the full blocks
		while(runc != NULL) {
			for(; run < runc->num_runs; run++) {
				for(; run_pos < runc->run[run].len; run_pos++) {
					size_t offset = pos % B_PAGE_SIZE;                    // offset into current block
					size_t copy_len = min(bufsize, B_PAGE_SIZE - offset); // length to copy into current block
					
					if(op == MEMFS_READ)
						memcpy(buffer, memfs_block_to_addr(info, runc->run[run].block + run_pos) + offset, copy_len);
					else
						memcpy(memfs_block_to_addr(info, runc->run[run].block + run_pos) + offset, buffer, copy_len);
					
					buffer += copy_len;
					bufsize -= copy_len;
					pos += copy_len;
					*len += copy_len;
					if(bufsize <= 0) {
						// no more data to copy
						goto donefullblock;
					}
				}
				run_pos = 0;
			}
			runc = runc->next;
			// reset these for the next for loop
			run = 0;	
		}
	}
donefullblock:
	// handle any data left over for the partial fragment
	if(bufsize > 0) {
		size_t offset = pos % B_PAGE_SIZE;
		size_t copy_len = min(bufsize, v->u.data.frag_size - offset);
		
		if(op == MEMFS_READ) 
			memcpy(buffer, memfs_frag_block_to_addr(info, v->u.data.last_frag->start) + offset, copy_len);
		else
			memcpy(memfs_frag_block_to_addr(info, v->u.data.last_frag->start) + offset, buffer, copy_len);
			
		*len += copy_len;
	}
	
err:	
	return err;
}

/* ******************************************************************************* */

#if DEBUG_DUMP
static void dump_stat( struct stat *st )
{
	dprintf( "*** memfs: dump_stat\n" );
	dprintf( "  stat->st_dev     : 0x%lx\n", st->st_dev );
	dprintf( "  stat->st_ino     : 0x%Lx\n", st->st_ino );
	dprintf( "  stat->st_mode    : 0x%08X\n", st->st_mode );
	dprintf( "  stat->st_nlink   : %d\n", st->st_nlink );
	dprintf( "  stat->st_uid     : %d\n", st->st_uid );
	dprintf( "  stat->st_gid     : %d\n", st->st_gid );
	dprintf( "  stat->st_size    : %Ld\n", st->st_size );
	dprintf( "  stat->st_rdev    : %ld\n", st->st_rdev );
	dprintf( "  stat->st_blksize : %ld\n", st->st_blksize );
	dprintf( "  stat->st_atime   : %ld\n", st->st_atime );
	dprintf( "  stat->st_mtime   : %ld\n", st->st_mtime );
	dprintf( "  stat->st_ctime   : %ld\n", st->st_ctime );
	dprintf( "  stat->st_crtime  : %ld\n", st->st_crtime );
} 

static void dump_fs_info( struct fs_info *fsi )
{
//	struct fs_info
	dprintf( "*** memfs: dump_fs_info\n" );
	dprintf( "  fs_info->dev          : 0x%08lx\n", fsi->dev );
	dprintf( "  fs_info->root         : 0x%08Lx\n", fsi->root );
	dprintf( "  fs_info->flags        : 0x%08lx\n", fsi->flags );
	dprintf( "  fs_info->block_size   : %Ld\n", fsi->block_size );
	dprintf( "  fs_info->io_size      : %Ld\n", fsi->io_size );
	dprintf( "  fs_info->total_blocks : %Ld\n", fsi->total_blocks );
	dprintf( "  fs_info->free_blocks  : %Ld\n", fsi->free_blocks );
	dprintf( "  fs_info->total_nodes  : %Ld\n", fsi->total_nodes );
	dprintf( "  fs_info->free_nodes   : %Ld\n", fsi->free_nodes );
	dprintf( "  fs_info->device_name  : %s\n", fsi->device_name );
	dprintf( "  fs_info->volume_name  : %s\n", fsi->volume_name );
	dprintf( "  fs_info->fsh_name     : %s\n", fsi->fsh_name );
}
#endif
#if DEBUG_IOCTLS
static void dump_memfs_info( struct memfs_info *info )
{
	dprintf( "*** memfs: dump_mem_info\n" );
	// struct memfs_info {
    dprintf( "magic        : 0x%08lx\n", info->magic );
    dprintf( "name         : %s\n", info->name );
    dprintf( "next_version : %ld\n", info->next_version );
    dprintf( "nsid         : %ld\n", info->nsid );
    dprintf( "flags        : 0x%08lx\n", info->flags );;
    dprintf( "max_size     : %ld\n", info->max_size );
    dprintf( "size         : %ld\n", info->size );
    dprintf( "vnode_count  : %ld\n", info->vnode_count );
    dprintf( "area         : %ld\n", info->area );
    dprintf( "area_ptr     : %p\n", info->area_ptr );
    dprintf( "area_bitmap  : %p\n", info->area_bitmap );
    dprintf( "area_size    : %ld\n", info->area_size );
    dprintf( "lock         : %ld\n", info->lock );
    dprintf( "*root        : %p\n", info->root );
	dprintf( "*all_head    : %p\n", info->all_head );
	dprintf( "*all_frags   : %p\n", info->all_frags );
	dprintf( "*unused_frags: %p\n", info->unused_frags );
	dprintf( "vnode_pgdir  : 0x%lx\n", info->vnode_pgdir);
	dprintf( "&vnode_page_full_bitmap  : %p\n", &info->vnode_page_full_bitmap);
	/* extra ... */
	dprintf( "sizeof(memfs_info) : %ld\n", sizeof(memfs_info) );
	dprintf( "sizeof(vnode)      : %ld\n", sizeof(vnode) );
	dprintf( "sizeof(file_run_container)  : %ld\n", sizeof(struct file_run_container));
	dprintf( "sizeof(file_info)  : %ld\n", sizeof(file_info) );
	dprintf( "sizeof(frag_desc)  : %ld\n", sizeof(struct frag_desc) );
}

static void dump_frag_list(struct memfs_info *info)
{
	struct frag_desc *frag;
	
	dprintf("dumping frag list starting at %p\n", info->all_frags);
	for(frag = info->all_frags; frag != NULL; frag = frag->next) {
		dprintf("frag at %p\n", frag);
		dprintf("\tprev        = %p\n", frag->prev);
		dprintf("\tnext        = %p\n", frag->next);
		dprintf("\tnext_unused = %p\n", frag->next_unused);
		dprintf("\tstart       = %ld\n", frag->start);
		dprintf("\tlen         = %ld\n", frag->len);
		dprintf("\towner       = %p\n", frag->owner);
	}
}

static void dump_dir( vnode *v )
{
	vnode *i;

	dprintf( "%20s : 0x%08Lx\n", ".", v->vnid ); 
	dprintf( "%20s : 0x%08Lx\n", "..", v->parent->vnid ); 
	for ( i = v->u.dir_head; i; i = i->next )
		dprintf( "%20s : 0x%08Lx\n", i->name != NULL ? i->name : "(NULL)", i->vnid ); 
}

static void dump_vnodes( memfs_info *info )
{
	vnode *v;

	dprintf( "*** dump_vnodes\nADDR      PREV      NODE      NEXT      PARENT    HEAD              VNID      COOKIES     MODE\n" );
	for ( v = info->all_head; v; v = v->all_next )
		if(S_ISDIR(v->mode)) {
			dprintf( "%p  %-8s  %-8s  %-8s  %-8s  %-8s          %08Lx  %d            0x%x\n", 
				v,
				(v->prev ? v->prev->name : "(NULL)"), 
				v->name, 
				(v->next ? v->next->name : "(NULL)"), 
				v->parent->name, 
				(v->u.dir_head ? v->u.dir_head->name : "(NULL)"),
				v->vnid,
				cookie_count(v),
				v->mode
			);
		} else {
			dprintf( "%p  %-8s  %-8s  %-8s  %-8s  %-8s          %08Lx  %d            0x%x\n", 
				v,
				(v->prev ? v->prev->name : "(NULL)"), 
				v->name, 
				(v->next ? v->next->name : "(NULL)"), 
				v->parent->name, 
				"NOTDIR",
				v->vnid,
				cookie_count(v),
				v->mode
			);
		}			
}

static void dump_vnode( vnode *v )
{
	dprintf("VNODE at %p:\n", v);
	dprintf("\tmagic            = 0x%08lx\n", v->magic);
	dprintf("\tname             = %p '%s'\n", v->name, v->name ? v->name : "(NULL)");
	dprintf("\tvnid             = 0x%016Lx\n", v->vnid);
	dprintf("\tnsid             = %ld\n",	v->nsid);
	dprintf("\tctime            = 0x%08lx\n", v->ctime);
	dprintf("\tmtime            = 0x%08lx\n", v->mtime);
	dprintf("\tuid              = 0x%08x\n", v->uid);
	dprintf("\tgid              = 0x%08x\n", v->gid);
	dprintf("\tmode             = 0x%08x\n", v->mode);
	dprintf("\tflags            = 0x%08lx\n", v->flags);
	dprintf("\tparent           = %p\n", v->parent);
	dprintf("\tnext             = %p\n", v->next);
	dprintf("\tprev             = %p\n", v->prev);
	dprintf("\tjar              = %p\n", v->jar);
	dprintf("\tall_next         = %p\n", v->all_next);
	dprintf("\tall_prev         = %p\n", v->all_prev);
	dprintf("\tsize             = %ld\n",	v->size);
	dprintf("\tattr_head        = %p\n", v->attr_head);
	if(S_ISDIR(v->mode)) {
		dprintf("\tu.dir_head       = %p\n", v->u.dir_head);
	} else if(S_ISLNK(v->mode)) {
		dprintf("\tu.symlink        = %p\n", v->u.symlink);
	} else if(S_ISREG(v->mode)) {
		struct file_run_container *runc;
		int i;
		
		dprintf("\tu.data.last_frag = %p\n", v->u.data.last_frag);
		if(v->u.data.last_frag != NULL) {
			dprintf("\t\tprev        = %p\n", v->u.data.last_frag->prev);
			dprintf("\t\tnext        = %p\n", v->u.data.last_frag->next);
			dprintf("\t\tnext_unused = %p\n", v->u.data.last_frag->next_unused);
			dprintf("\t\tstart       = %ld\n", v->u.data.last_frag->start);
			dprintf("\t\tlen         = %ld\n", v->u.data.last_frag->len);
			dprintf("\t\towner       = %p\n", v->u.data.last_frag->owner);
		}
		dprintf("\tu.data.frag_size = %ld\n", v->u.data.frag_size);
		dprintf("\tu.data.run_head  = %p\n", v->u.data.run_head);
		runc = v->u.data.run_head;
		while(runc != NULL) {
			dprintf("\trun_container at %p:\n", runc);
			dprintf("\t\tnext     = %p\n", runc->next);
			dprintf("\t\tnum_runs = %ld\n", runc->num_runs);
			for(i=0; i < runc->num_runs; i++) {
				dprintf("\t\trun      = %d:\n", i);
				dprintf("\t\t\tblock    = %ld\n", runc->run[i].block);
				dprintf("\t\t\tlen      = %ld\n", runc->run[i].len);				
			}
			runc = runc->next;
		}
	}
}

static void dump_area_bitmap(memfs_info *info)
{
	unsigned int i;
	int free_start = -1;
	int inuse_start = -1;

	if(info->area_bitmap == NULL) {
		dprintf("area bitmap NULL\n");
		return;
	} else {
		dprintf("area bitmap at %p\n", info->area_bitmap);
	}
	dprintf("bitmap size (in bytes) %ld\n", info->area_size / 8);
	dprintf("bitmap size (in blocks) %ld\n", info->area_size);
	
	for(i = 0; i < info->area_size; i++) {
		if(GET_BIT(info->area_bitmap[i / 8], i % 8)) {
			if(inuse_start != -1) {
				continue;
			}
			if(free_start != -1) {
				dprintf("free from from blocks %d -> %d\n", free_start, i-1);
				free_start = -1;
			}
			inuse_start = i;
		} else {
			if(free_start != -1) {
				continue;
			}
			if(inuse_start != -1) {
				dprintf("inuse from %d -> %d\n", inuse_start, i-1);
				inuse_start = -1;
			}
			free_start = i;
		}
	}
	if(inuse_start != -1) {
		dprintf("inuse from %d -> %d\n", inuse_start, i-1);
	}
	if(free_start != -1) {
		dprintf("free from %d -> %d\n", free_start, i-1);
	}
}

static int cookie_count( vnode *v ) 
{
	file_info *i;
	int n = 0;

	for ( i = v->jar; i; i = i->next_fc )
		n++;

	return n;
}
#endif

#if DEBUG_VERIFY_FRAG_LIST
static int verify_frag_list(memfs_info *info)
{
	struct frag_desc *last = NULL;
	struct frag_desc *last_unused = NULL;
	struct frag_desc *curr;
	uint32 curr_block = 0xffffffff;
	uint32 frags_covered_in_block = 0;
	
	curr = info->all_frags;
	while(curr != NULL) {
		if(curr->start + curr->len >= info->area_size * NUM_FRAGS_PER_BLOCK) {
			dump_frag_list(info);
			panic("frag %p: extends past end of area\n", curr);
		}		
		if(curr->prev != last) {
			dump_frag_list(info);		
			panic("frag %p: prev pointer (%p) does not point to prev block %p\n", curr, curr->prev, last);
		}
		if(curr->len == 0) {
			dump_frag_list(info);
			panic("frag %p: zero len\n", curr);
		}				
		{
			uint32 frag_end    = curr->start + curr->len;
			uint32 start_block = curr->start / NUM_FRAGS_PER_BLOCK;
			uint32 stop_block  = frag_end / NUM_FRAGS_PER_BLOCK;
			uint32 i, j;

			if(frag_end % NUM_FRAGS_PER_BLOCK == 0)
				stop_block--;
			
			if(stop_block - start_block >= 1 &&
				(curr->start % NUM_FRAGS_PER_BLOCK == 0 || frag_end % NUM_FRAGS_PER_BLOCK == 0)) {
				// we cover an entire block.
				dump_frag_list(info);
				panic("frag %p covers an entire block\n", curr);
			}

			for(i = start_block, j = 0; i <= stop_block; j++, i++) {			
				if(GET_BIT(info->area_bitmap[i / 8], i % 8) == 0) {
					dump_frag_list(info);
					panic("frag %p covers block (%ld) that isn't allocated\n", curr, i);
				}
				if(i != curr_block) {
					if(curr_block == 0xffffffff) {
						// first time, so let it pass
					} else {
						if(frags_covered_in_block != NUM_FRAGS_PER_BLOCK) {
							dump_frag_list(info);
							panic("block %ld was not fully covered by frags (%ld)\n", curr_block, frags_covered_in_block);
						}
					}
					curr_block = i;
					frags_covered_in_block = 0;
				}

				switch(j) {
					case 0:
						frags_covered_in_block += min(NUM_FRAGS_PER_BLOCK - curr->start % NUM_FRAGS_PER_BLOCK, curr->len);
						break;
					case 1:
						frags_covered_in_block += curr->len - (ROUNDUP(curr->start, NUM_FRAGS_PER_BLOCK) - curr->start);
						break;
				}
			}
		}
		
		if(last != NULL) {
			if(last->start > curr->start) {
				dump_frag_list(info);
				panic("last frag %p starts past start of curr frag %p\n", last, curr);
			}
			if(last->start + last->len > curr->start) {
				dump_frag_list(info);
				panic("last frag %p extends into frag %p\n", last, curr);
			}			
			if(last->start + last->len == curr->start && last->owner == NULL && curr->owner == NULL) {
				dump_frag_list(info);
				panic("two free frags are next to each other %p, %p\n", last, curr);
			}
		}
		
		if(curr->owner == NULL) {
			if(last_unused == NULL) {
				// first unused block
				if(info->unused_frags != curr) {
					dump_frag_list(info);
					panic("first unused frag %p is not pointed to by head unused pointer (%p)\n", curr, info->unused_frags);
				}
			} else {
				if(last_unused->next_unused != curr) {
					dump_frag_list(info);
					panic("last unused frag %p does not point to us %p\n", last_unused, curr);
				}
			}
			last_unused = curr;
		}
		last = curr;
		curr = curr->next;
	}
	return B_OK;
}
#endif
#if 0
// XXX make more complete
static int verify_vnode(memfs_info *info, vnode *v)
{	
	if(v->magic != MEMFS_NODE_MAGIC)
		panic("vnode @ %p does not pass magic test\n", v);
	
	{		
		struct vnode *v2;
		v2 = info->all_head;
		while(v2 != NULL) {
			if(v2 == v)
				break;
			v2 = v2->all_next;
		}
		if(v2 == NULL)
			panic("vnode @ %p is not in global list in fs struct @ %p\n", v, info);
	}

	if(S_ISREG(v->mode)) {
		if(v->size % B_PAGE_SIZE != v->u.data.frag_size)
			panic("vnode @ %p says it's size is %ld, but frag size is %ld. (should be %ld)\n",
				v, v->size, v->u.data.frag_size, v->size % B_PAGE_SIZE);
		if(v->u.data.frag_size > 0)
			if(v->u.data.last_frag == NULL)
				panic("vnode @ %p frag size > 0, but frag ptr == NULL\n", v);
		if(v->size / B_PAGE_SIZE > 0) {
			struct file_run_container *runc;
			int i;
			
			if(v->u.data.run_head == NULL)
				panic("vnode @ %p has no runs, though it's size > %d (%ld)\n",
					v, 	B_PAGE_SIZE, v->size);
			
			runc = v->u.data.run_head;
			while(runc != NULL) {
				for(i = 0; i < runc->num_runs; i++) {
					if(runc->run[i].block + runc->run[i].len > info->area_size)
						panic("vnode @ %p has run @ %p which has bad run (block %ld, len %ld) at index %d\n",
							v, runc, runc->run[i].block, runc->run[i].len, i);
				}
				runc = runc->next;
			}
		}
	}
	return 0;
}
#endif

/* ******************************************************************************* */

int32	api_version = B_CUR_FS_API_VERSION;

/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/
vnode_ops fs_entry = {
	/* op_read_vnode */				&memfs_read_vnode,
	/* op_write_vnode */			&memfs_write_vnode,
	/* op_remove_vnode */			&memfs_remove_vnode,
	/* op_secure_vnode */			&memfs_secure_vnode,

	/* directory operations */
	
	/* op_walk */					&memfs_walk,
	/* op_access */					&memfs_access,
	/* op_create */					&memfs_create,
	/* op_mkdir	*/					&memfs_mkdir,
	/* op_symlink */				&memfs_symlink,
	/* op_link */					&memfs_link,
	/* op_rename */					&memfs_rename,
	/* op_unlink */					&memfs_unlink,
	/* op_rmdir	*/					&memfs_rmdir,
	/* op_readlink */				&memfs_readlink,
	/* op_opendir */				&memfs_opendir,
	/* op_closedir */				&memfs_closedir,
	/* op_free_dircookie */			&memfs_free_cookie,
	/* op_rewinddir	*/				&memfs_rewinddir,
	/* op_readdir */				&memfs_readdir,

	/* file operations */
	
	/* op_open */					&memfs_open,
	/* op_close */					&memfs_close,
	/* op_free_cookie */			&memfs_free_cookie,
	/* op_read */					&memfs_read,
	/* op_write */					&memfs_write,
	/* op_readv */					NULL,
	/* op_writev */					NULL,
	/* op_ioctl */					&memfs_ioctl,
	/* op_setflags */				&memfs_setflags,
	/* op_rstat */					&memfs_rstat,
	/* op_wstat */					&memfs_wstat,

	/* nspace operations */
	
	/* op_fsync */					&memfs_fsync,
	/* op_initialize */				&memfs_initialize,
	/* op_mount */					&memfs_mount,
	/* op_unmount */				&memfs_unmount,
	/* op_sync */					&memfs_sync,
	/* op_rfsstat */				&memfs_rfsstat,
	/* op_wfsstat */				&memfs_wfsstat,

	/* op_select */					NULL,
	/* op_deselect */				NULL,

	/* index operations (not implemented) */
	
	/* op_open_indexdir */			NULL,
	/* op_close_indexdir */			NULL,
	/* op_free_indexdircookie */	NULL,
	/* op_rewind_indexdir */		NULL,
	/* op_read_indexdir */			NULL,
	/* op_create_index */			NULL,
	/* op_remove_index */			NULL,
	/* op_rename_index */			NULL,
	/* op_stat_index */				NULL,

	/* attribute operations (not implemented) */
	
	/* op_open_attrdir */			memfs_open_attrdir,
	/* op_close_attrdir */			memfs_close_attrdir,
	/* op_free_attrdircookie */		memfs_free_attrdircookie,
	/* op_rewind_attrdir */			memfs_rewind_attrdir,
	/* op_read_attrdir */			memfs_read_attrdir,
	/* op_write_attr */				memfs_write_attr,
	/* op_read_attr */				memfs_read_attr,
	/* op_remove_attr */			memfs_remove_attr,
	/* op_rename_attr */			NULL,
	/* op_stat_attr */				memfs_stat_attr,
	/* op_open_query */				NULL,
	/* op_close_query */			NULL,
	/* op_free_querycookie */		NULL,
	/* op_read_query */				NULL
};
