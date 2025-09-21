#include "ntfs.h"
#include "attributes.h"
#include "extent.h"
#include "util.h"
#include "io.h"
#include "mime_table.h"

static status_t ntfs_set_mime_type(vnode *v, const char *filename)
{
	struct ext_mime *p;
	int32 namelen, ext_len;

	DEBUGPRINT(ATTR, 7, ("ntfs_set_mime_type of (%s) entry\n", filename));

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
		DEBUGPRINT(ATTR, 8, ("ntfs_set_mime_type got type '%s' for file '%s'\n", v->mime, filename));

	return B_OK;
}

static status_t ntfs_add_attribute_to_vnode_list(vnode *v, void *attr_struct_to_add)
{
	
	if(!attr_struct_to_add) return EINVAL;
	
	DEBUGPRINT(ATT, 6, ("ntfs_add_attribute_to_vnode_list adding attribute to list, now %ld attributes.\n", v->attr_list_size+1));
	
	if(v->attr_list_size == 0) {
		v->attr_list = attr_struct_to_add;
	} else {
		attr_header *h = (attr_header *)v->last_attr;
		h->next = attr_struct_to_add;
	}
		
	v->last_attr = attr_struct_to_add;
	
	v->attr_list_size++;
		
	return B_NO_ERROR;
}

// returns a buffer full of whatever the runlist points to
static status_t ntfs_load_extent_target(ntfs_fs_struct *ntfs, extent_storage *extent, size_t length, void **buffer)
{
	status_t err;
	size_t length_read;	
		
	DEBUGPRINT(ATT, 5, ("ntfs_load_extent_target entry.\n"));

	*buffer=ntfs_malloc(length);
	if(!(*buffer)) {
		err = ENOMEM;
		goto error;
	}

	err = ntfs_read_extent(ntfs, extent, *buffer, 0, length, &length_read);
	if(err < B_NO_ERROR) goto error1;
	if(length_read != length) {
		// It must have had some problem reading all of the buffer
		// Assume we don't want this data
		goto error1;
	}
	
	DEBUGPRINT(ATT, 5, ("ntfs_load_extent_target exit.\n"));
	return B_NO_ERROR;
	
error1:
	ntfs_free(*buffer);
error:
	return err;
}

// returns a buffer full of the data contained in the non-resident attribute
// it works by creating an extent, loading it, and returning a buffer full of what it points at.
static status_t ntfs_load_nonresident_attribute_data(ntfs_fs_struct *ntfs, vnode *v, void *buf, void **outbuf)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_nonresident *nres_header;
	extent_storage extent;
	status_t err;

	DEBUGPRINT(ATT, 5, ("ntfs_load_nonresident_attribute_data entry.\n"));
	
	if(!header->non_resident) {
		// This must be a resident attribute
		err = EINVAL;
		goto error;
	}
	
	nres_header = (ntfs_attr_nonresident *)((char *)buf+0x10);
	
	// Set up the extent
	extent.extent_head = extent.extent_tail = NULL;
	extent.compressed = B_LENDIAN_TO_HOST_INT16(header->compressed);
	extent.num_file_blocks = 0;
	extent.att_id = v->attr_list_size;
	extent.vnid = v->vnid;

	// store how much of the file is initialized
	extent.uninit_data_start_block =
		B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) / ntfs->cluster_size;
	extent.uninit_data_start_offset =
		B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) % ntfs->cluster_size;

	// load the runlist
	err = ntfs_load_extent_from_runlist(&extent,
		((char *)buf+B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset)),
		B_LENDIAN_TO_HOST_INT32(header->length) - B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset),
		(strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->last_VCN))+1) - strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->starting_VCN)),
		extent.compressed);
	if(err < B_NO_ERROR) goto error1;
		
	// load the data the runlist points to
	err = ntfs_load_extent_target(ntfs, &extent, B_LENDIAN_TO_HOST_INT64(nres_header->real_size), outbuf);
	if(err < B_NO_ERROR) goto error1;

	// free the extent memory
	ntfs_free_extent(&extent);	

	DEBUGPRINT(ATT, 5, ("ntfs_load_nonresident_attribute_data exit.\n"));

	return B_NO_ERROR;

error1:
	ntfs_free_extent(&extent);
error:
	return err;
}

// this function loads a vnode data attribute structure from a NTFS data attribute buffer
// it returns < 0 on error, 0 on ok, 1 on addition to existing attribute
status_t ntfs_load_data_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	data_attr *new_attr;
	data_attr *temp_attr;
	uint32 name_offset;
	bool extension_record = false;
	int i=0;
	status_t err;

	DEBUGPRINT(ATT, 6, ("ntfs_load_data_attribute() entry.\n"));
	
	// Create a new attribute struct
	new_attr = ntfs_malloc(sizeof(data_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(data_attr));	
	new_attr->type = ATT_DATA;

	// Set up some stuff
	new_attr->compressed = B_LENDIAN_TO_HOST_INT16(header->compressed);

	// Get the name now
	if(!header->non_resident)
		name_offset = 0x18;
	else
		name_offset = 0x40;

	// Copy the name
	if(header->name_len > 0) {
		int32 unicode_name_len = header->name_len*2;
		int32 max_utf8_name_len = min(header->name_len * 3 - 1, MAX_ATT_NAME_LENGTH - 1);
		
		new_attr->name = ntfs_malloc(max_utf8_name_len + 1);
		if(!new_attr->name) {
			err = ENOMEM;
			goto error;
		}
		memset(new_attr->name, 0, max_utf8_name_len + 1);
		err = ntfs_unicode_to_utf8((char *)buf+name_offset, &unicode_name_len, new_attr->name, &max_utf8_name_len);
		if(err < B_NO_ERROR) goto error;
		DEBUGPRINT(ATT, 7, ("\tattribute has name: '%s'\n", new_attr->name));
	} else {
		DEBUGPRINT(ATT, 7, ("\tattribute has no name.\n"));
	}	

	// Check to see if this particular attribute already exists.
	// If it does, add the runlist to the current one
	do {	
		temp_attr = (data_attr *)ntfs_find_attribute(v, ATT_DATA, i);
		if(temp_attr) {
			bool freeit = false;
		
			// If the two names match
			if(new_attr->name && temp_attr->name) {
				if(!strcmp(new_attr->name, temp_attr->name)) {
					freeit = true;
				}
			}
			
			// If neither has a name
			if(!new_attr->name && !temp_attr->name) {
				freeit = true;
			}
			
			if(freeit) {	
				// Free up this attribute we just created, and point to the old one for the rest of this function
				DEBUGPRINT(ATT, 7, ("\tattribute already exists.\n"));
				if(new_attr->name) ntfs_free(new_attr->name);
				ntfs_free(new_attr);
				new_attr = temp_attr;
				extension_record = true;
			}
		}
		i++;
	} while(temp_attr != NULL);
		
	// Check the resident flag
	if(!header->non_resident) {
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);
		
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));
		// Handle the resident case first
		new_attr->stored_local = true;
		new_attr->length = B_LENDIAN_TO_HOST_INT32(res_header->specific_value_length);
		if(new_attr->length) {
			// Copy the data
			new_attr->data = ntfs_malloc(new_attr->length);
			if(!new_attr->data) {
				err = ENOMEM;
				goto error;
			}
			memcpy(new_attr->data, (char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset), new_attr->length);
		}
		
		DEBUGPRINT(ATT, 7, ("\tresident size = %Ld\n", new_attr->length));
	} else {
		ntfs_attr_nonresident *nres_header = (ntfs_attr_nonresident *)((char *)buf+0x10);
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));
		
		// Get the runlist
		err = ntfs_load_extent_from_runlist(&(new_attr->extent),
			(char *)buf + B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset),
			B_LENDIAN_TO_HOST_INT32(header->length) - B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset), 
			(strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->last_VCN))+1) - strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->starting_VCN)),
			B_LENDIAN_TO_HOST_INT16(header->compressed));
		if(err < B_NO_ERROR) goto error;
		
		// Mark the extent as coming from this attribute
		new_attr->extent.att_id = v->attr_list_size;
		new_attr->extent.vnid = v->vnid;

		// Record some other data if we are the first FILE record
		if(!extension_record) {
			new_attr->stored_local = false;
			new_attr->length = B_LENDIAN_TO_HOST_INT64(nres_header->real_size);
			new_attr->extent.uninit_data_start_block =
				B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) / ntfs->cluster_size;
			new_attr->extent.uninit_data_start_offset =
				B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) % ntfs->cluster_size;
		}
	}	
	
	// If this isn't an extension record, we need to add the new attribute struct to the vnode
	if(!extension_record) {
		err = ntfs_add_attribute_to_vnode_list(v, new_attr);
		if(err < B_NO_ERROR) goto error;
	}
	
	return B_NO_ERROR;

error:
	if(header->non_resident) ntfs_free_extent(&new_attr->extent);
	if(new_attr->name) ntfs_free(new_attr);
	ntfs_free(new_attr);
	new_attr = NULL;
	return err;
}

// this function loads a index allocation attribute from a MFT record
status_t ntfs_load_index_alloc_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	data_attr *temp_attr;
	data_attr *new_attr;
	status_t err;
	uint32 name_offset;
	bool extension_record = false;
	
	DEBUGPRINT(ATT, 6, ("ntfs_load_index_alloc_attribute() entry.\n"));

	// Create a new attribute struct
	new_attr = ntfs_malloc(sizeof(index_alloc_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(index_alloc_attr));	
	new_attr->type = ATT_INDEX_ALLOCATION;

	// Set up some stuff
	new_attr->compressed = B_LENDIAN_TO_HOST_INT16(header->compressed);		

	// Get the name now
	if(!header->non_resident) 
		name_offset = 0x18;
	else
		name_offset = 0x40;

	// Copy the name
	if(header->name_len > 0) {
		int32 unicode_name_len = header->name_len*2;
		int32 max_utf8_name_len = min(header->name_len * 3 - 1, MAX_ATT_NAME_LENGTH - 1);;
		
		new_attr->name = ntfs_malloc(max_utf8_name_len + 1);
		if(!new_attr->name) {
			err = ENOMEM;
			goto error;
		}
		memset(new_attr->name, 0, max_utf8_name_len + 1);
		
		err = ntfs_unicode_to_utf8((char *)buf+name_offset, &unicode_name_len, new_attr->name, &max_utf8_name_len);
		if(err < B_NO_ERROR) goto error;
		DEBUGPRINT(ATT, 7, ("\tattribute has name: '%s'\n", new_attr->name));
	} else {
		ERRPRINT(("ntfs_load_index_alloc_attribute found index alloc attribute with no name?.\n"));
		err = B_NO_ERROR;
		goto error;
	}	

	// We are only concerned with index attributes named '$I30'
	if(strcmp(new_attr->name, "$I30") != 0) {
		ERRPRINT(("ntfs_load_index_alloc_attribute found index allocation attribute not named $I30.\n"));
		ERRPRINT(("\tnamed = %s\n", new_attr->name));
		err = B_NO_ERROR;
		goto error;
	}

	// Check to see if this particular attribute already exists.
	// If it does, add the runlist to the current one
	temp_attr = (data_attr *)ntfs_find_attribute(v, ATT_INDEX_ALLOCATION, 0);
	if(temp_attr) {
		DEBUGPRINT(ATT, 7, ("\tattribute already exists.\n"));
		ntfs_free(new_attr);
		new_attr = temp_attr;
	}

	// Check the resident flag
	if(!header->non_resident) {
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);

		DEBUGPRINT(ATT, 7, ("\tresident.\n"));
		// Handle the resident case first
		new_attr->stored_local = true;
		new_attr->length = B_LENDIAN_TO_HOST_INT32(res_header->specific_value_length);
		
		if(new_attr->length) {
			// Copy the data
			new_attr->data = ntfs_malloc(new_attr->length);
			if(!new_attr->data) {
				err = ENOMEM;
				goto error;
			}
			memcpy(new_attr->data, (char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset), new_attr->length);
		}
		
		DEBUGPRINT(ATT, 7, ("\tresident size = %Ld\n", new_attr->length));	
	} else {
		ntfs_attr_nonresident *nres_header = (ntfs_attr_nonresident *)((char *)buf+0x10);
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));
		
		// Get the runlist
		err = ntfs_load_extent_from_runlist(&new_attr->extent,
			(char *)buf + B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset),
			B_LENDIAN_TO_HOST_INT32(header->length) - B_LENDIAN_TO_HOST_INT16(nres_header->runlist_offset), 
			(strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->last_VCN))+1) - strip_high_16(B_LENDIAN_TO_HOST_INT64(nres_header->starting_VCN)),
			B_LENDIAN_TO_HOST_INT16(header->compressed));
		if(err < B_NO_ERROR) goto error;
		// Mark the extent as coming from this attribute
		new_attr->extent.att_id = v->attr_list_size;
		new_attr->extent.vnid = v->vnid;
		
		// Record some other data if we are the first FILE record
		if(!extension_record) {
			new_attr->stored_local = false;
			new_attr->length = B_LENDIAN_TO_HOST_INT64(nres_header->real_size);
			new_attr->extent.uninit_data_start_block =
				B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) / ntfs->cluster_size;
			new_attr->extent.uninit_data_start_offset =
				B_LENDIAN_TO_HOST_INT64(nres_header->initialized_data_size) % ntfs->cluster_size;
		}
		
	}	
	
	// If this isn't an extension record, we need to add the new attribute struct to the vnode
	if(!extension_record) {
		err = ntfs_add_attribute_to_vnode_list(v, new_attr);
		if(err < B_NO_ERROR) goto error;
	}

	return B_NO_ERROR;

error:
	if(header->non_resident) ntfs_free_extent(&new_attr->extent);
	if(new_attr->name) ntfs_free(new_attr->name);
	ntfs_free(new_attr);
	new_attr = NULL;
	return err;
}

// This function loads a file name attribute from a MFT record
status_t ntfs_load_file_name_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_FILE_NAME *file_name_buf;
	file_name_attr *new_attr;
	status_t err;

	DEBUGPRINT(ATT, 6, ("ntfs_load_file_name_attribute entry.\n"));
	
	// Allocate a new attribute
	new_attr = ntfs_malloc(sizeof(file_name_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(file_name_attr));	
	new_attr->type = ATT_FILE_NAME;

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);			
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));

		file_name_buf = (ntfs_attr_FILE_NAME *)((char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset));		
	} else {
		// non-resident
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		err = ntfs_load_nonresident_attribute_data(ntfs, v, buf, (void *)&file_name_buf);
		if(err < B_NO_ERROR) goto error;
	}
	
	// Set up the new structure
	new_attr->dir_file_rec = B_LENDIAN_TO_HOST_INT64(file_name_buf->container_directory);
	new_attr->file_length = B_LENDIAN_TO_HOST_INT64(file_name_buf->real_size);
	new_attr->name_length = file_name_buf->name_length;
	new_attr->name_type = file_name_buf->file_name_type;
	new_attr->flags = B_LENDIAN_TO_HOST_INT64(file_name_buf->flags);
	new_attr->file_creation_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(file_name_buf->file_creation_time));
	new_attr->last_modification_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(file_name_buf->last_modification_time));
	new_attr->last_FILE_rec_mod_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(file_name_buf->last_FILE_rec_mod_time));
	new_attr->last_access_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(file_name_buf->last_access_time));
	
	// copy the name
	{
		int32 unicode_name_len = file_name_buf->name_length*2;
		int32 max_utf8_name_len = min(file_name_buf->name_length * 3 - 1, MAX_FILENAME_LENGTH - 1);
				
		new_attr->name = ntfs_malloc(max_utf8_name_len + 1);
		if(!new_attr->name) {
			err = ENOMEM;
			goto error;
		}
		memset(new_attr->name, 0, max_utf8_name_len + 1);
		
		err = ntfs_unicode_to_utf8((char *)file_name_buf+0x42, &unicode_name_len, new_attr->name, &max_utf8_name_len);
		if(err < B_NO_ERROR) goto error;
	}
	
	if(header->non_resident) {
		ntfs_free(file_name_buf);
	}

	err = ntfs_add_attribute_to_vnode_list(v, new_attr);
	if(err < B_NO_ERROR) goto error;

	// figure the mime type
	if(v->mime == NULL) 
		ntfs_set_mime_type(v, new_attr->name);

	DEBUGPRINT(ATT, 7, ("\tname = '%s'\n", new_attr->name));
	DEBUGPRINT(ATT, 6, ("ntfs_load_file_name_attribute exit.\n"));

	return B_NO_ERROR;

error:
	if(new_attr->name) ntfs_free(new_attr->name);
	ntfs_free(new_attr);
	return err;
}

// this function loads a standard info attribute from a MFT record
status_t ntfs_load_std_info_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_STD_INFO *std_info;
	std_info_attr *new_attr;
	status_t err;

	DEBUGPRINT(ATT, 6, ("ntfs_load_std_info_attribute() entry.\n"));
	
	// Allocate a new attribute
	new_attr = ntfs_malloc(sizeof(std_info_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(std_info_attr));	
	new_attr->type = ATT_STD_INFO;

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);			
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));

		std_info = (ntfs_attr_STD_INFO *)((char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset));
		
	} else {
		// non-resident
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		err = ntfs_load_nonresident_attribute_data(ntfs, v, buf, (void *)&std_info);
		if(err < B_NO_ERROR) goto error;
	}

	// Copy the data
	new_attr->file_creation_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(std_info->file_creation_time));
	new_attr->last_modification_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(std_info->last_modification_time));
	new_attr->last_FILE_rec_mod_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(std_info->last_FILE_rec_mod_time));
	new_attr->last_access_time = ntfs_time_to_posix_time(B_LENDIAN_TO_HOST_INT64(std_info->last_access_time));
	new_attr->DOS_permissions = B_LENDIAN_TO_HOST_INT32(std_info->DOS_permissions);

	if(header->non_resident) {
		ntfs_free(std_info);
	}

	err = ntfs_add_attribute_to_vnode_list(v, new_attr);
	if(err < B_NO_ERROR) goto error;

	return B_NO_ERROR;
	
error:
	ntfs_free(new_attr);
	return err;
}

// this function loads a volume name attribute from a MFT record. Should only be used for $Volume
status_t ntfs_load_volume_name_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	void *temp;
	int32 name_length;
	int32 max_name_len = MAX_VOLUME_NAME_LENGTH-1;
	volume_name_attr *new_attr;
	status_t err;

	DEBUGPRINT(ATT, 6, ("ntfs_load_volume_name_attribute() entry.\n"));
	
	// Allocate a new attribute
	new_attr = ntfs_malloc(sizeof(volume_name_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(volume_name_attr));	
	new_attr->type = ATT_VOLUME_NAME;

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);			
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));

		temp = (char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset);
		
		name_length = B_LENDIAN_TO_HOST_INT32(res_header->specific_value_length);
	} else {
		// non-resident
		ntfs_attr_nonresident *nres_header = (ntfs_attr_nonresident *)((char *)buf+0x10);
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		err = ntfs_load_nonresident_attribute_data(ntfs, v, buf, (void *)&temp);
		if(err < B_NO_ERROR) goto error;
		
		name_length = B_LENDIAN_TO_HOST_INT64(nres_header->real_size);		
	}

	// Copy the data
	memset(new_attr->name, 0, MAX_VOLUME_NAME_LENGTH);
	//ntfs_uni2ascii(new_attr->name, temp, name_length/2);
	ntfs_unicode_to_utf8(temp, &name_length, new_attr->name, &max_name_len);
	

	DEBUGPRINT(ATT, 7, ("\tvolume name = '%s'\n", new_attr->name)); 

	if(header->non_resident)
		ntfs_free(temp);

	err = ntfs_add_attribute_to_vnode_list(v, new_attr);
	if(err < B_NO_ERROR) goto error;

	return B_NO_ERROR;
	
error:
	ntfs_free(new_attr);
	return err;
}

// this function loads a index root attribute from a MFT record
status_t ntfs_load_index_root_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_INDEX_ROOT *ir = NULL;
	index_root_attr *new_attr;
	status_t err;
	

	DEBUGPRINT(ATT, 6, ("ntfs_load_index_root_attribute() entry.\n"));

	// Allocate a new attribute
	new_attr = ntfs_malloc(sizeof(index_root_attr));
	if(!new_attr) return ENOMEM;
	memset(new_attr, 0, sizeof(index_root_attr));	
	new_attr->type = ATT_INDEX_ROOT;

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);			
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));

		ir = (ntfs_attr_INDEX_ROOT *)((char *)buf+B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset));
		
		new_attr->length = B_LENDIAN_TO_HOST_INT32(res_header->specific_value_length);
	} else {
		// non-resident
		ntfs_attr_nonresident *nres_header = (ntfs_attr_nonresident *)((char *)buf+0x10);
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		err = ntfs_load_nonresident_attribute_data(ntfs, v, buf, (void *)&ir);
		if(err < B_NO_ERROR) goto error;
				
		new_attr->length = B_LENDIAN_TO_HOST_INT64(nres_header->real_size);		
	}

	// Copy the data
	new_attr->index_buffer_size = B_LENDIAN_TO_HOST_INT32(ir->index_record_size);
	DEBUGPRINT(ATT, 7, ("\tindex record size = %ld\n", new_attr->index_buffer_size));
	new_attr->entries = ntfs_malloc(B_LENDIAN_TO_HOST_INT32(ir->size_of_entries) - 0x10);
	if(!new_attr->entries) {
		err = ENOMEM;
		goto error;
	}	
	memcpy(new_attr->entries, (char *)ir+0x20, B_LENDIAN_TO_HOST_INT32(ir->size_of_entries) - 0x10);  
	v->is_dir = true;

	if(header->non_resident)
		ntfs_free(ir);

	err = ntfs_add_attribute_to_vnode_list(v, new_attr);
	if(err < B_NO_ERROR) goto error;

	return B_LENDIAN_TO_HOST_INT32(header->length);

error:
	if(header->non_resident) 
		if(ir) ntfs_free(ir);
	ntfs_free(new_attr);
	return err;	
}

// this function handles loading a attribute list attribute. It simply loads the list of MFT records and modifies
// the vnode so that ntfs_load_vnode_from_MFT will load all of the MFT records.
status_t ntfs_load_att_list_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs, uint64 *MFT_records, uint32 *num_MFT_records)
{
	ntfs_attr_header *header = buf;
	void *data_buf = NULL;
	char *curr_ptr;
	uint32 length_to_read;
	status_t err;
	ntfs_attr_ATT_LIST_record *record;
		
	DEBUGPRINT(ATT, 6, ("ntfs_load_att_list_attribute() entry. num_MFT_records = %ld\n", *num_MFT_records));

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = (ntfs_attr_resident *)((char *)buf+0x10);			
		DEBUGPRINT(ATT, 7, ("\tresident.\n"));
		
		length_to_read = B_LENDIAN_TO_HOST_INT32(res_header->specific_value_length);
		data_buf = (char *)buf + B_LENDIAN_TO_HOST_INT16(res_header->specific_value_offset);
	} else {
		// non-resident
		ntfs_attr_nonresident *nres_header = (ntfs_attr_nonresident *)((char *)buf + 0x10);
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));
		
		// Load the attribute data		
		err = ntfs_load_nonresident_attribute_data(ntfs, v, buf, (void *)&data_buf);
		if(err < B_NO_ERROR) goto error;
		
		length_to_read = B_LENDIAN_TO_HOST_INT64(nres_header->real_size);
	}
	
	curr_ptr = data_buf;

	while(length_to_read > 0) {
		record = (ntfs_attr_ATT_LIST_record *)curr_ptr;
		// Modify the pointers
		curr_ptr += B_LENDIAN_TO_HOST_INT16(record->rec_length);
		length_to_read -= B_LENDIAN_TO_HOST_INT16(record->rec_length);
		// Add one to the FILE record list in the vnode
		{
			int i;
			FILE_REC new_record = B_LENDIAN_TO_HOST_INT64(record->attribute_record);
			bool is_new_record = true;
			// Only add it if it doesn't already exist
			for(i=0; i<*num_MFT_records; i++) 
				if(strip_high_16(new_record) == strip_high_16(MFT_records[i])) {
					if(new_record != MFT_records[i]) {
						// If we're here, we have previously loaded a pointer to the same file record with a
						// different sequence number in the high 16 bits. Looks like a filesystem consistancy bug.
						err = EINVAL;
						goto error;
					}
					is_new_record = false;
					break;
				}
		
			if(is_new_record) {
				if(*num_MFT_records < MAX_MFT_RECORDS) {
					MFT_records[*num_MFT_records] = new_record;
					(*num_MFT_records)++;
					DEBUGPRINT(ATT, 7, ("\tfound pointer to FILE record %Ld.\n", strip_high_16(new_record)));
				} else {
					ERRPRINT(("NTFS: vnode %Ld has more than %d MFT records.\n", v->vnid, MAX_MFT_RECORDS));
				}
			}
		}
	}
	
	if(header->non_resident)
		ntfs_free(data_buf);

	return B_NO_ERROR;

error:
	if(header->non_resident)
		if(data_buf) ntfs_free(data_buf);
	return err;	
}
