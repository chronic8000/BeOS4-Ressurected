/*
This file was shamelessly stolen from Victor.  I changed the hashing
functions to handle a vnid + a filename, unfortunately necessary
for the CIFS protocol.
-Alfred


Note - should change all the strcmp's to strncmp's to avoid running through mem.


Todo:  modify vcache_entry so that we dynamically allocate size for
the filename.  Otherwise, we are really hogging memory.


Ok, as i understand it now, we need to deal with vnode_id's in the following
circumstancess:

walk() : You get the vnode_id of the directory, and the filename.  Find the vnid.

read_vnode : You get the vnid of the file.  Load it into memory.  We'll need to
find the filename and the directory it's in.

*/



#ifdef NEED_TO_LOCK_CACHE
#define LOCK_CACHE_R acquire_sem(vol->vcache.vc_sem)
#define LOCK_CACHE_W acquire_sem_etc(vol->vcache.vc_sem, READERS, 0, 0)
#define UNLOCK_CACHE_R release_sem(vol->vcache.vc_sem)
#define UNLOCK_CACHE_W release_sem_etc(vol->vcache.vc_sem, READERS, 0)
#else
#define LOCK_CACHE_R 
#define LOCK_CACHE_W 
#define UNLOCK_CACHE_R 
#define UNLOCK_CACHE_W 
#endif



#include <fsproto.h>
#include <KernelExport.h>

#include <stdio.h>
#include <string.h>

#include "cifs_globals.h"

//#include "vcache.h"

#define DBG_VCACHE 0

#define MAXFILENAMESIZE 255

typedef char* filename_t;
//#define filename_t char*

#define ARTIFICIAL_VNID_BITS (0xAEFDLL << 48)
#define READERS 424242
#define ASSERT(x) 0



#define FILEEQUALS(_entry, _filename, _parent_dir) (((_entry->parent_dir == _parent_dir) && \
													(strncmp((_entry)->filename, _filename, MAXFILENAMESIZE) == 0)))
													
#define FILEGREATER(_entry, _filename, _parent_dir) (((_entry)->parent_dir > _parent_dir) && \
													 (strncmp((_entry)->filename, _filename, MAXFILENAMESIZE) > 0))
													
#define FILELESS(_entry, _filename, _parent_dir) ( ! FILEGREATER(_entry, _filename, _parent_dir))
													

#define hash(v) ((v) & (vol->vcache.cache_size-1))

inline int filehash(nspace* vol, const char* file) {
	const char* p = file;
	int val = 0;
	
	for ( ; *p ; ++p)
		val = 5*val + *p;
	return (val & (vol->vcache.cache_size-1));
}


struct vcache_entry {
	vnode_id	vnid;		/* originally reported vnid */
//	vnode_id	loc;		/* where the file is now */
	char		filename[MAXFILENAMESIZE];
	vnode_id	parent_dir;
	char		is_dir;
	char		is_del;
	struct vcache_entry *next_vnid; /* next entry in vnid hash table */
	struct vcache_entry *next_loc;  /* next entry in location hash table */
};

void dump_vcache(nspace *vol)
{
	int i;
	struct vcache_entry *c;
	DPRINTF(-1, ("vnid cache size %x, cur vnid = %Lx\n\tvnid             loc\n", vol->vcache.cache_size, vol->vcache.cur_vnid));
	for (i=0;i<vol->vcache.cache_size;i++) {
		DPRINTF(-1, ("Cache:%d ->\n", i+1));
		for (c = vol->vcache.by_vnid[i];c;c=c->next_vnid)
			DPRINTF(-1, ("vnid: %16Lx name: %s dirvnid: %16Lx is_dir ->%x<-\n", c->vnid, c->filename, c->parent_dir, c->is_dir));
		}
}

status_t init_vcache(nspace *vol)
{
	char name[16];

	vol->vcache.cur_vnid = ARTIFICIAL_VNID_BITS;
	vol->vcache.cache_size = 512; /* must be power of 2 */
	
	MALLOC(vol->vcache.by_vnid, struct vcache_entry **,(vol->vcache.cache_size * sizeof(struct vcache_entry *)));
	if (vol->vcache.by_vnid == NULL) {
		return ENOMEM;
	}

	MALLOC(vol->vcache.by_loc, struct vcache_entry **,(vol->vcache.cache_size * sizeof(struct vcache_entry *)));
	if (vol->vcache.by_loc == NULL) {
		FREE(vol->vcache.by_vnid);
		vol->vcache.by_vnid = NULL;
		return ENOMEM;
	}

	sprintf(name, "cifs cache %x", vol->id);
	if ((vol->vcache.vc_sem = create_sem(READERS, name)) < 0) {
		FREE(vol->vcache.by_vnid); vol->vcache.by_vnid = NULL;
		FREE(vol->vcache.by_loc); vol->vcache.by_loc = NULL;
		return vol->vcache.vc_sem;
	}

	return 0;
}

status_t uninit_vcache(nspace *vol)
{
	int i, count = 0;
	struct vcache_entry *c, *n;

	LOCK_CACHE_W;

	/* FREE entries */
	for (i=0;i<vol->vcache.cache_size;i++) {
		c = vol->vcache.by_vnid[i];
		while (c) {
			count++;
			n = c->next_vnid;
			FREE(c);
			c = n;
		}
	}

	FREE(vol->vcache.by_vnid); vol->vcache.by_vnid = NULL;
	FREE(vol->vcache.by_loc); vol->vcache.by_loc = NULL;

	delete_sem(vol->vcache.vc_sem);

	return 0;
}

vnode_id generate_unique_vnid(nspace *vol)
{
	vnode_id val;
	/* only one thread per volume will be in here at any given time anyway
	 * due to volume locking */
	//dprintf("generate_uniq_vnid for vol %x, cur_vnid is %Lx\n", vol, vol->vcache.cur_vnid);
	vol->vcache.cur_vnid = vol->vcache.cur_vnid + 1;
	val = vol->vcache.cur_vnid;
	//dprintf("generate_uniq_vnid for vol %x, returning %Lx\n", vol, val);

	return val;
}

static status_t _add_to_vcache_(nspace *vol, vnode_id vnid, const char* filename,
				vnode_id parent_dir, char is_dir)
{
// xxx
	int hash1 = hash(vnid);
	int hash2 = filehash(vol, filename);
	struct vcache_entry *e, *c, *p;


	ASSERT(vnid != loc);

	MALLOC(e,struct vcache_entry *,(sizeof(struct vcache_entry)));
	if (e == NULL)
		return ENOMEM;

	// Fill in vcache entry.
	e->vnid = vnid;
	strncpy(e->filename, filename, MAXFILENAMESIZE);
	e->filename[MAXFILENAMESIZE - 1] = 0;  // ensure null
	e->parent_dir = parent_dir;
	e->is_dir = is_dir;
	e->is_del = 0;
	e->next_vnid = NULL; e->next_loc = NULL;


	// Insert entry into vnid hash table.
	c = p = vol->vcache.by_vnid[hash1];
	while (c) {
		if (vnid < c->vnid)
			break;
		ASSERT((vnid != c->vnid) && (loc != c->loc));
		p = c;
		c = c->next_vnid;
	}
	ASSERT(!c || (vnid != c->vnid));

	e->next_vnid = c;
	if (p == c)
		vol->vcache.by_vnid[hash1] = e;
	else
		p->next_vnid = e;

	// Insert entry into filename hash table.
	c = p = vol->vcache.by_loc[hash2];
	while (c) {
		if (FILEGREATER(c, filename, parent_dir))
			break;
		ASSERT((vnid != c->vnid) && (loc != c->loc));
		p = c;
		c = c->next_loc;
	}
	ASSERT(!c || (loc != c->loc));

	e->next_loc = c;
	if (p == c)
		vol->vcache.by_loc[hash2] = e;
	else
		p->next_loc = e;

	return B_OK;
}

static status_t _remove_from_vcache_(nspace *vol, vnode_id vnid, char check_integrity)
{
// xxx
	int hash1 = hash(vnid), hash2;
	struct vcache_entry *c, *p, *e;

	c = p = vol->vcache.by_vnid[hash1];
	while (c) {
		if (vnid == c->vnid)
			break;
		ASSERT(c->vnid < vnid);
		p = c;
		c = c->next_vnid;
	}
	ASSERT(c);
	if (!c) return ENOENT;

	if (p == c)
		vol->vcache.by_vnid[hash1] = c->next_vnid;
	else
		p->next_vnid = c->next_vnid;

	e = c;

	hash2 = filehash(vol, c->filename);
	c = p = vol->vcache.by_loc[hash2];

	while (c) {
		if (vnid == c->vnid)
			break;
		if (check_integrity)
			ASSERT(c->loc < e->loc);
		p = c;
		c = c->next_loc;
	}
	ASSERT(c);
	if (!c) return ENOENT;
	if (p == c)
		vol->vcache.by_loc[hash2] = c->next_loc;
	else
		p->next_loc = c->next_loc;

	FREE(c);

	return 0;
}

static struct vcache_entry *_find_vnid_in_vcache_(nspace *vol, vnode_id vnid)
{
// good
	int hash1 = hash(vnid);
	struct vcache_entry *c;
	c = vol->vcache.by_vnid[hash1];
	while (c) {
		if (c->vnid == vnid)
			break;
		if (c->vnid > vnid)
			return NULL;
		c = c->next_vnid;
	}

	return c;
}

static struct vcache_entry *_find_loc_in_vcache_(nspace *vol, const char *filename,
					vnode_id parent_dir)
{
	int hash2 = filehash(vol, filename);
	struct vcache_entry *c;
	
	c = vol->vcache.by_loc[hash2];

	while (c) {
		if (FILEEQUALS(c, filename, parent_dir)) 
			break;
		if (FILEGREATER(c, filename, parent_dir))
			return NULL;
		c = c->next_loc;
	}
	
	return c;
}

status_t add_to_vcache(nspace *vol, vnode_id vnid, const char* filename,
							vnode_id parent_dir, char is_dir)							
{
	status_t result;

	if ((vnid & ARTIFICIAL_VNID_BITS) == 0) {
		dprintf("VCACHE SANITY -> adding %s:%Lx on parent %Lx\n",
			filename, vnid, parent_dir);
		return B_ERROR;
	}
	
	LOCK_CACHE_W;
	debug(DBG_VCACHE, ("add_to_vcache file %s vnid %Lx parent vnid %Lx, %d\n", filename, vnid, parent_dir, is_dir));	
	result = _add_to_vcache_(vol,vnid,filename,parent_dir, is_dir);
	UNLOCK_CACHE_W;

	if (result != 0) DPRINTF(-1, ("add_to_vcache failed (%s)\n", strerror(result)));
	return result;
}

/* vyt: do this in a smarter fashion */
static status_t _update_loc_in_vcache_(nspace *vol, vnode_id vnid, filename_t filename,
							vnode_id parent_dir, char is_dir)
{
	status_t result;

	result = _remove_from_vcache_(vol, vnid, 0);
	if (result == 0)
		result = _add_to_vcache_(vol, vnid, filename, parent_dir, is_dir);

	return result;
}

status_t remove_from_vcache(nspace *vol, vnode_id vnid)
{
	status_t result;

	LOCK_CACHE_W;
	result = _remove_from_vcache_(vol,vnid,1);
	UNLOCK_CACHE_W;

	if (result != 0) DPRINTF(-1, ("remove_from_vcache failed (%s)\n", strerror(result)));
	return result;
}

status_t vcache_vnid_to_loc(nspace *vol, vnode_id vnid, char** filename,
					vnode_id *parent_dir, char* is_dir)
{
	struct vcache_entry *e = NULL;

	
	//if (! filename) return B_ERROR;
	// Now we can call vcache_vnid_to_loc(vol, vnid, NULL, NULL, NULL)
	// and just check for existence in the hash.
	

	LOCK_CACHE_R;
	e = _find_vnid_in_vcache_(vol, vnid);

	if (filename && *filename && parent_dir && e) {
			*parent_dir = e->parent_dir;
			*is_dir = e->is_dir;
			strcpy((*filename) , e->filename);
	}
	UNLOCK_CACHE_R;

	if (e && (! e->is_del))
		return B_OK;
	else
		return ENOENT;
}

status_t vcache_loc_to_vnid(nspace *vol, const char *filename, vnode_id parent_dir,
					vnode_id *vnid, char* is_dir)
{
	struct vcache_entry *e = NULL;

	LOCK_CACHE_R;

	debug(DBG_VCACHE, ("vcache_loc_to_vnid file %s parent vnid %Lx\n", filename, parent_dir));	
	e = _find_loc_in_vcache_(vol, filename, parent_dir);

	if (vnid && e) {
			*vnid = e->vnid;
			if (is_dir != NULL) *is_dir = e->is_dir;
	}
	UNLOCK_CACHE_R;

	if (e && (! e->is_del))
		return B_OK;
	else
		return ENOENT;
}

status_t set_vnid_del(nspace *vol, vnode_id vnid) {

	struct vcache_entry *e = NULL;
	
	LOCK_CACHE_R;
	
	e = _find_vnid_in_vcache_(vol, vnid);

	if (e) {
		e->is_del = 1;
	}

	UNLOCK_CACHE_R;

	if (e)
		return B_OK;
	else
		return B_ERROR;
}


#if 0
status_t vcache_set_entry(nspace *vol, vnode_id vnid, filename_t filename,
					vnode_id parent_dir)
{
// xxx I'm really not sure what this function is for.  Why does it remove
// the vnid if e->vnid == loc?  Is this FAT specific???
	struct vcache_entry *e;
	status_t result = B_OK;

	DPRINTF(0, ("vcache_set_entry: %Lx -> %s %Lx\n", vnid, filename, parent_dir));

	LOCK_CACHE_W;

	e = _find_vnid_in_vcache_(vol, vnid);

// xxx
	if (e) {
		if (e->vnid == loc)
			result = _remove_from_vcache_(vol, vnid,1);
		else
			result = _update_loc_in_vcache_(vol, vnid, filename, parent_dir);
	} else {
		if (vnid != loc)
			result = _add_to_vcache_(vol,vnid,filename,parent_dir);
	}

	UNLOCK_CACHE_W;

	return result;
}
#endif
