#include "ntfs.h"
#include "attributes.h"
#include "attribute_handlers.h"

static int load_attribute(ntfs_fs_struct *ntfs, vnode *file_vnode, void *buf, uint64 *MFT_records, uint32 *num_MFT_records);

// this function loads all of the applicable attributes from a MFT FILE record into 
// the vnode corresponding to the record.
status_t load_attributes(ntfs_fs_struct *ntfs, vnode *file_vnode, void *FILE_buf, uint64 *MFT_records, uint32 *num_MFT_records)
{
	ntfs_FILE_record *file_rec = FILE_buf;
	char *curr_ptr = FILE_buf;
	int32 length_to_read;
	
	DEBUGPRINT(ATT, 5, ("load_attributes entry.\n"));
	
	// Do some sanity checks
	if(!FILE_buf) return EINVAL;	

	// This is the maximum size of the file record
	length_to_read = B_LENDIAN_TO_HOST_INT32(file_rec->real_FILE_record_size);

	// Fill in some vnode data
	file_vnode->hard_link_count = B_LENDIAN_TO_HOST_INT16(file_rec->hard_link_count);
	if(B_LENDIAN_TO_HOST_INT16(file_rec->flags) & FILE_REC_IS_DIR_FLAG) file_vnode->is_dir = true;
	else file_vnode->is_dir = false;

	// Check the upcoming pointer munge
	if(B_LENDIAN_TO_HOST_INT16(file_rec->offset_to_attributes) > ntfs->mft_recordsize) return EINVAL;

	// Push the current pointer to the first attribute
	curr_ptr += B_LENDIAN_TO_HOST_INT16(file_rec->offset_to_attributes);
	{
		int retval;
		
		do {
			retval = load_attribute(ntfs, file_vnode, curr_ptr, MFT_records, num_MFT_records);
			ntfs_verify_attribute_list(file_vnode);
			if(retval < 0) return retval;
			if(retval == 0) break;
			curr_ptr += retval;
			length_to_read -= retval;
		} while(length_to_read > 0); 
	}
		
	return B_NO_ERROR;
}

// Loads the current attribute into the vnode, returns the number of bytes to push the curr_ptr by,
// or <= 0, meaning there was some sort of error.
static int load_attribute(ntfs_fs_struct *ntfs, vnode *file_vnode, void *buf, uint64 *MFT_records, uint32 *num_MFT_records)
{
	ntfs_attr_header *header;
	status_t err;

	// Sanity check
	if(!buf) return EINVAL;

	// Set the attribute header
	header = buf;

	DEBUGPRINT(ATT, 5, ("load_attribute found attribute type 0x%x, length = %ld\n",
		B_LENDIAN_TO_HOST_INT32((unsigned int)header->type),	
		B_LENDIAN_TO_HOST_INT32(header->length)));
	
	// Select what to do with the attribute
	switch(B_LENDIAN_TO_HOST_INT32(header->type)) {
		case ATT_STD_INFO:
			err = ntfs_load_std_info_attribute(file_vnode, buf, ntfs);		
			break;
		case ATT_ATT_LIST:
			err = ntfs_load_att_list_attribute(file_vnode, buf, ntfs, MFT_records, num_MFT_records);
			break;
		case ATT_FILE_NAME:
			err = ntfs_load_file_name_attribute(file_vnode, buf, ntfs);		
			break;
		case ATT_VOLUME_NAME:
			err = ntfs_load_volume_name_attribute(file_vnode, buf, ntfs);		
			break;
		case ATT_DATA:			
			err = ntfs_load_data_attribute(file_vnode, buf, ntfs);
			break;
		case ATT_INDEX_ROOT:
			err = ntfs_load_index_root_attribute(file_vnode, buf, ntfs);
			break;		
		case ATT_INDEX_ALLOCATION:
			err = ntfs_load_index_alloc_attribute(file_vnode, buf, ntfs);
			break;
		case 0xffffffff:
			return 0;
			break;
		default:
			DEBUGPRINT(ATT, 6, ("\tunable to handle attribute type 0x%x\n", (unsigned int)header->type));
			return B_LENDIAN_TO_HOST_INT32(header->length);
	}

	if(err < 0) 
		return err;
		
	return B_LENDIAN_TO_HOST_INT32(header->length);
}

void *ntfs_find_attribute_by_name(vnode *v, uint8 attribute_to_find, const char *name)
{
	attr_header *h;
	data_attr *data;
	index_alloc_attr *ia;
	int namelen;
	int i;
	
	if(!name) return NULL;

	if(!(attribute_to_find == ATT_DATA) && !(attribute_to_find == ATT_INDEX_ALLOCATION)) {
		// The attribute passed has no name
		return NULL;
	}
		
	DEBUGPRINT(ATT, 8, ("ntfs_find_attribute_by_name entry looking for attribute (%s).\n", name));
	
	ntfs_verify_attribute_list(v);
	
	namelen = strlen(name);
	
	if(v->attr_list_size == 0) return NULL;
	i = v->attr_list_size;	
		
	h = v->attr_list;
	while(h && i>0) {
		if(h->type == attribute_to_find) {
			switch(h->type) {
				case ATT_DATA:
					data = (data_attr *)h;
					if(data->name) {
						if(strcasecmp(data->name, name) == 0) {
							return h;
						}
					} else {
						if(namelen == 0) return h;
					}
					break;
				case ATT_INDEX_ALLOCATION:
					ia = (index_alloc_attr *)h;
					if(ia->name) {
						if(strcasecmp(ia->name, name) == 0) {
							return h;
						}
					} else {
						if(namelen == 0) return h;
					}
					break;
				default:
					break;
			}
		}
		// If the attribute we're looking at > than the one we're searching for, it aint here.
		// This list is sorted by attribute number order
		if(h->type > attribute_to_find) return NULL;
		h = h->next;
		i--;
	} 
	
	// If we're here, we must not have found it
	return NULL;
}

// finds the requested attribute for the vnode. Returns the num_to_findths one.
void *ntfs_find_attribute(vnode *v, uint8 attribute_to_find, uint32 num_to_find)
{
	attr_header *h;
	int i;
	
	DEBUGPRINT(ATT, 8, ("ntfs_find_attribute entry.\n"));
	
	if(v == NULL) return NULL;
	
	ntfs_verify_attribute_list(v);
	
	if(v->attr_list_size == 0) return NULL;
	i = v->attr_list_size;	
		
	h = v->attr_list;
	while(h && i>0) {
		if(h->type == attribute_to_find) {
			if(num_to_find == 0) {
				return h;
			} else {
				num_to_find--;
			}
		}
		// If the attribute we're looking at > than the one we're searching for, it aint here.
		// This list is sorted by attribute number order
		if(h->type > attribute_to_find) return NULL;
		h = h->next;
		i--;
	} 
	
	// If we're here, we must not have found it
	return NULL;
}

/*
status_t ntfs_verify_attribute_list(vnode *v)
{
	attr_header *h;
	int i;
	
	DEBUGPRINT(ATT, 8, ("ntfs_find_attribute entry.\n"));
	
	if(v->attr_list_size == 0) {
		if((v->attr_list) || (v->last_attr)) {
			ERRPRINT(("ntfs: vnode %Ld (0x%x) attr_list_size == 0, but a pointer exists.\n",
				v->vnid, (unsigned int)v));
			kernel_debugger(NULL);
		}
		return B_NO_ERROR;
	}
	i = v->attr_list_size;	
		
	h = v->attr_list;
	while(h && i>0) {
		if((h < 0x100000) || (h > 0x80000000)) {
			ERRPRINT(("ntfs: vnode %Ld (0x%x) has an attribute that points to bad spot (0x%x)\n",
				v->vnid, (unsigned int)v, (unsigned int)h));
			kernel_debugger(NULL);
		}
		if(h->type & 0xF) {
			ERRPRINT(("ntfs: vnode %Ld (0x%x) has an attribute (0x%x) with an invalid type (0x%x)\n",
				v->vnid, (unsigned int)v, (unsigned int)h, h->type));
			kernel_debugger(NULL);
		}
			
		h = h->next;
		i--;
	}
	
	// If we're here, we must not have found it
	return B_NO_ERROR;
}
*/
