#define MIME_STRING_TYPE 'MIMS'
#define RAW_TYPE 'RAWT'

#include "ext2.h"
#include "io.h"

int ext2_open_attrdir(void *ns, void *node, void **cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie   *d;

	CHECK_FS_STRUCT(ext2, "ext2_open_attrdir");
	CHECK_VNODE(v, "ext2_open_attrdir");

	DEBUGPRINT(DIR, 5, ("ext2_open_attrdir on vnode %Ld.\n", v->vnid));

	// Allocate the cookie
	d = (attrdircookie *)malloc(sizeof(attrdircookie));
	if(!d) return ENOMEM;
	memset(d, 0, sizeof(attrdircookie));
	
	*cookie = d;
	
	DEBUGPRINT(ATTR, 5, ("ext2_open_attrdir exit.\n"));
	
	return B_NO_ERROR;
}

int ext2_close_attrdir(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_close_attrdir");
	CHECK_VNODE(v, "ext2_close_attrdir");
	CHECK_COOKIE(d, "ext2_close_attrdir");
		
	DEBUGPRINT(ATTR, 5, ("ext2_close_attrdir called on node %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

int ext2_free_attrdircookie(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_free_attrdircookie");
	CHECK_VNODE(v, "ext2_free_attrdircookie");
	CHECK_COOKIE(d, "ext2_free_attrdircookie");
	
	DEBUGPRINT(ATTR, 5, ("ext2_free_attrdircookie called on node %Ld.\n", v->vnid));

	// Lookie, free cookie.
	free(d);
	
	return B_OK;
}

int ext2_rewind_attrdir(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_rewind_attrdir");
	CHECK_VNODE(v, "ext2_rewind_attrdir");
	CHECK_COOKIE(d, "ext2_rewind_attrdir");
	
	DEBUGPRINT(ATTR, 5, ("ext2_rewind_attrdir called on vnid %Ld.\n", v->vnid));
	
	// Just reset it to zero
	memset(d, 0, sizeof(attrdircookie));

	return B_NO_ERROR;
}

int ext2_read_attrdir(void *ns, void *node, void *cookie, long *num, struct dirent *entry, size_t bufsize)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_read_attrdir");
	CHECK_VNODE(v, "ext2_read_attrdir");
	CHECK_COOKIE(d, "ext2_read_attrdir");
	
	DEBUGPRINT(ATTR, 5, ("ext2_read_attrdir entry on vnode %Ld.\n", v->vnid));	

	*num = 0;

	if(!d->done) {
		d->done = true;
		LOCKVNODE(v, 1);		
		if(v->mime) {
			entry->d_ino = 0;
			entry->d_reclen = strlen("BEOS:TYPE") + 1;
			strcpy(entry->d_name, "BEOS:TYPE");		
			*num = 1;
		}
		UNLOCKVNODE(v, 1);
	}
		
	
	return B_NO_ERROR;
}

int ext2_stat_attr(void *ns, void *node, const char *name, struct attr_info *buf)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	status_t 		err = B_NO_ERROR;

	CHECK_FS_STRUCT(ext2, "ext2_stat_attr");
	CHECK_VNODE(v, "ext2_stat_attr");
	
	if(!name) return EINVAL;
	
	DEBUGPRINT(ATTR, 5, ("ext2_stat_attr entry on vnode %Ld with attr (%s).\n", v->vnid, name));	

	if(strcmp(name, "BEOS:TYPE")==0) {
		LOCKVNODE(v, 1);
		if (v->mime == NULL) {
			err = ENOENT;
		} else {		
			buf->type = MIME_STRING_TYPE;
			buf->size = strlen(v->mime) + 1;
		}
		UNLOCKVNODE(v, 1);
	} else {
		err = ENOENT;
	}

	return err;
}


int ext2_read_attr(void *ns, void *node, const char *name, int type, void *buf, size_t *len, off_t pos)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	size_t length_to_read;	
	
	CHECK_FS_STRUCT(ext2, "ext2_read_attr");
	CHECK_VNODE(v, "ext2_read_attr");

	if(!name) return EINVAL;
	
	if(pos < 0) return EINVAL;

	DEBUGPRINT(ATTR, 5, ("ext2_read_attr entry on vnode %Ld with attr (%s).\n", v->vnid, name));	

	// If we're reading in the mime type, it had better be the right type
	if((strcmp(name, "BEOS:TYPE") != 0) || (type != MIME_STRING_TYPE)) {
		*len = 0;
		return ENOENT;
	}

	LOCKVNODE(v, 1);

	if(!v->mime) {
		UNLOCKVNODE(v, 1);
		*len = 0;
		return ENOENT;
	}
	// Make sure the offsets and length are ok
	if(pos > strlen(v->mime)) {
		UNLOCKVNODE(v, 1);
		*len = 0;
		return B_NO_ERROR;
	}
	length_to_read = min(*len - 1, strlen(v->mime) - pos);				
	strncpy(buf, v->mime + pos, length_to_read);
	((char *)buf)[length_to_read] = 0;
	*len = length_to_read + 1;
		
	UNLOCKVNODE(v, 1);

	DEBUGPRINT(ATTR, 5, ("ext2_stat_attr exit on vnode %Ld with attr (%s).\n", v->vnid, name));	

	return 0;
}
