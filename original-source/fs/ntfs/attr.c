#define MIME_STRING_TYPE 'MIMS'
#define RAW_TYPE 'RAWT'

#include "ntfs.h"
#include "attributes.h"
#include "io.h"

int ntfs_open_attrdir(void *ns, void *node, void **cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie   *d;
	data_attr		*data;

	TOUCH(ntfs);TOUCH(v);

	DEBUGPRINT(DIR, 5, ("ntfs_open_attrdir on vnode %Ld.\n", v->vnid));

	// Allocate the cookie
	d = (attrdircookie *)ntfs_malloc(sizeof(attrdircookie));
	if(!d) {
		ERRPRINT(("ntfs_open_attrdir had problem allocating memory for cookie.\n"));
		return ENOMEM;
	}
	// Initialize it
	memset(d, 0, sizeof(attrdircookie));

	// see if a BEOS:TYPE attribute already exists in ntfs for some reason.	
	data = ntfs_find_attribute_by_name(v, ATT_DATA, "BEOS:TYPE");		
	if(data) {
		// It already exists, so mark the flag in the cookie as done
		d->read_sniffed_mimetype = true;
	}
	
	*cookie = d;
	
	DEBUGPRINT(ATTR, 5, ("ntfs_open_attrdir exit.\n"));
	
	return B_NO_ERROR;
}


int ntfs_close_attrdir(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
		
	DEBUGPRINT(ATTR, 5, ("ntfs_close_attrdir called on node %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

int ntfs_free_attrdircookie(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;

	if(!d) return EINVAL;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
	
	DEBUGPRINT(ATTR, 5, ("ntfs_free_attrdircookie called on node %Ld.\n", v->vnid));

	// Lookie, free cookie.
	ntfs_free(d);
	
	return B_OK;
}

int ntfs_rewind_attrdir(void *ns, void *node, void *cookie)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;
	data_attr		*data;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);

	if(!d) return EINVAL;

	DEBUGPRINT(ATTR, 5, ("ntfs_rewind_attrdir called on vnid %Ld.\n", v->vnid));
	
	memset(d, 0, sizeof(attrdircookie));

	// see if a BEOS:TYPE attribute already exists in ntfs for some reason.	
	data = ntfs_find_attribute_by_name(v, ATT_DATA, "BEOS:TYPE");		
	if(data) {
		// It already exists, so mark the flag in the cookie as done
		d->read_sniffed_mimetype = true;
	}
	
	return B_NO_ERROR;
}


int ntfs_read_attrdir(void *ns, void *node, void *cookie, long *num, struct dirent *entry, size_t bufsize)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	attrdircookie 	*d 	= (attrdircookie *)cookie;
	data_attr *data;

	TOUCH(ntfs);TOUCH(v);TOUCH(d);
	
	if(!d) return EINVAL;

	DEBUGPRINT(ATTR, 5, ("ntfs_read_attrdir entry on vnode %Ld.\n", v->vnid));	

	*num = 0;
	
start:
	// If we haven't read in the BEOS:TYPE attribute, read it here
	if(!d->read_sniffed_mimetype) {
		d->read_sniffed_mimetype = true;
		if(v->mime) {
			entry->d_ino = 0;
			entry->d_reclen = strlen("BEOS:TYPE") + 1;
			strcpy(entry->d_name, "BEOS:TYPE");
			
			*num = 1;
		} else {
			goto start;
		}
	} else {		
		// Else, lets read in the next data attribute
getdataattr:
		data = ntfs_find_attribute(v, ATT_DATA, d->data_attr_num++);
		if(data) {
			if(data->name == NULL) {
				// We don't want the primary data attribute
				goto getdataattr;
			}
		
			entry->d_ino = d->data_attr_num;
			entry->d_reclen = strlen(data->name) + 1;
			strcpy(entry->d_name, data->name);
			*num = 1;
		}
	}
	
	return B_NO_ERROR;
}

int ntfs_stat_attr(void *ns, void *node, const char *name, struct attr_info *buf)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	data_attr		*data;
	status_t 		err = B_NO_ERROR;

	TOUCH(ntfs);TOUCH(v);
	
	if(!name) return EINVAL;
	
	DEBUGPRINT(ATTR, 5, ("ntfs_stat_attr entry on vnode %Ld with attr (%s).\n", v->vnid, name));	

	data = ntfs_find_attribute_by_name(v, ATT_DATA, name);
	if(data) {
		if(strcmp(data->name, "BEOS:TYPE")) {
			buf->type = RAW_TYPE;
		} else {
			buf->type = MIME_STRING_TYPE;
		}
		buf->size = data->length;
	} else {
		if(strcmp(name, "BEOS:TYPE")==0) {
			if (v->mime == NULL) {
				err = ENOENT;
			} else {		
				buf->type = MIME_STRING_TYPE;
				buf->size = strlen(v->mime) + 1;
			}
		} else {
			err = ENOENT;
		}
	}

	return err;
}

int ntfs_read_attr(void *ns, void *node, const char *name, int type, void *buf, size_t *len, off_t pos)
{
	ntfs_fs_struct 	*ntfs = (ntfs_fs_struct *)ns;
	vnode			*v 	= (vnode *)node;
	data_attr		*data;
	size_t length_to_read;	
	status_t err;
	
	TOUCH(ntfs);TOUCH(v);

	if(!name) return EINVAL;
	
	if(pos < 0) return EINVAL;

	DEBUGPRINT(ATTR, 5, ("ntfs_read_attr entry on vnode %Ld with attr (%s).\n", v->vnid, name));	

	// If we're reading in the mime type, it had better be the right type
	if((strcmp(name, "BEOS:TYPE") == 0) && (type != MIME_STRING_TYPE)) {
		*len = 0;
		return ENOENT;
	}

	// Look up the proper data attribute
	data = ntfs_find_attribute_by_name(v, ATT_DATA, name);

	// see if we have one from the NTFS data streams
	if(data) {
		// check the starting position
		if(pos >= data->length) {
			*len = 0;
			return B_NO_ERROR;
		}
		
		if(type == MIME_STRING_TYPE) {
			length_to_read = min(data->length - pos, *len - 1);
		} else {
			length_to_read = min(data->length - pos, *len);
		}		
		// copy the data
		if(data->stored_local) {
			memcpy(buf, data->data, length_to_read);
			// If it's the mime string type, make sure it's null-terminated
			if((type == MIME_STRING_TYPE) && (length_to_read == *len - 1)) {
				char *a = (char *)buf;
				a[*len - 1] = 0;
				*len = strlen(a) + 1;
			} else {
				*len = length_to_read;
			}
		} else {
			err = ntfs_read_extent(ntfs, &data->extent, buf, pos, length_to_read, len);
			if(err < B_NO_ERROR) return err;
			// If it's the mime string type, make sure it's null-terminated
			if((type == MIME_STRING_TYPE) && (length_to_read == *len - 1)) {
				char *a = (char *)buf;
				a[*len - 1] = 0;
				*len = strlen(a) + 1;
			} else {
				*len = length_to_read;
			}
		}
	} else {
		if(strcmp(name, "BEOS:TYPE") == 0) {
			if(!v->mime) {
				*len = 0;
				return ENOENT;
			}
			// Make sure the offsets and length are ok
			if(pos > strlen(v->mime)) {
				*len = 0;
				return B_NO_ERROR;
			}
			length_to_read = min(*len - 1, strlen(v->mime) - pos);				
			strncpy(buf, v->mime + pos, length_to_read);
			((char *)buf)[length_to_read] = 0;
			*len = length_to_read + 1;
		} else {
			*len = 0;
			return ENOENT;
		}			
	}	

	DEBUGPRINT(ATTR, 5, ("ntfs_stat_attr exit on vnode %Ld with attr (%s).\n", v->vnid, name));	

	return 0;
}
