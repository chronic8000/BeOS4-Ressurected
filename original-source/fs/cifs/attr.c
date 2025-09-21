/* attr.c
 * handles mime type information for cifsfs
 * gets/sets mime information in vnode
 */


#define MIME_STRING_TYPE 'MIMS'

#include <SupportDefs.h>
#include <KernelExport.h>

#include <fs_attr.h>
#include <string.h>
#include <malloc.h>

#include <fsproto.h>
#include <lock.h>

#include "cifs_globals.h"
#include "attr.h"

#define CIFS_ATTR_DEBUG 0

struct ext_mime {
	char *extension;
	char *mime;
};

// some of this stolen from servers/roster/Sniff.cpp

static struct ext_mime mimes[] = {
	{ "gz", "application/x-gzip" },
	{ "hqx", "application/x-binhex40" },
	{ "lha", "application/x-lharc" },
	{ "pcl", "application/x-pcl" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ "sit", "application/x-stuff-it" },
	{ "tar", "application/x-tar" },
	{ "tgz", "application/x-gzip" },
	{ "uue", "application/x-uuencode" },
	{ "z", "application/x-compress" },
	{ "zip", "application/zip" },
	{ "zoo", "application/x-zoo" },

	{ "aif", "audio/x-aiff" },
	{ "aiff", "audio/x-aiff" },
	{ "au", "audio/basic" },
	{ "mid", "audio/x-midi" },
	{ "midi", "audio/x-midi" },
	{ "mod", "audio/mod" },
	{ "ra", "audio/x-real-audio" },
	{ "wav", "audio/x-wav" },
	{ "mp3", "audio/x-mpeg" },

	{ "bmp", "image/x-bmp" },
	{ "fax", "image/g3fax" },
	{ "gif", "image/gif" },
	{ "iff", "image/x-iff" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "pbm", "image/x-portable-bitmap" },
	{ "pcx", "image/x-pcx" },
	{ "pgm", "image/x-portable-graymap" },
	{ "png", "image/png" },
	{ "ppm", "image/x-portable-pixmap" },
	{ "rgb", "image/x-rgb" },
	{ "tga", "image/x-targa" },
	{ "tif", "image/tiff" },
	{ "tiff", "image/tiff" },
	{ "xbm", "image/x-xbitmap" },

	{ "txt", "text/plain" },
	{ "doc", "text/plain" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "rtf", "text/rtf" },
	{ "c", "text/x-source-code" },
	{ "cc", "text/x-source-code" },
	{ "c++", "text/x-source-code" },
	{ "h", "text/x-source-code" },
	{ "hh", "text/x-source-code" },
	{ "cxx", "text/x-source-code" },
	{ "cpp", "text/x-source-code" },
	{ "S", "text/x-source-code" },
	{ "java", "text/x-source-code" },

	{ "avi", "video/x-msvideo" },
	{ "mov", "video/quicktime" },
	{ "mpg", "video/mpeg" },
	{ "mpeg", "video/mpeg" },

	{ 0, 0 }
};

status_t set_mime_type(vnode *node, const char *filename)
{
	struct ext_mime *p;
	int32 namelen, ext_len;

	DPRINTF(CIFS_ATTR_DEBUG, ("get_mime_type of (%s)\n", filename));

	node->mime = NULL;

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

	node->mime = p->mime;

	return B_OK;
}

int cifs_open_attrdir(void *_vol, void *_node, void **_cookie)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	uint32 *cookie = NULL;

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_open_attrdir called\n"));

	LOCK_VOL(vol);

#if 0
	if (check_nspace_magic(vol, "cifs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}
#endif

	MALLOC(cookie, uint32*, sizeof(uint32));
	if (cookie == 0) {
		UNLOCK_VOL(vol);
		return ENOMEM;
	}
	*cookie = 0;
	*_cookie = cookie;
	
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int cifs_close_attrdir(void *_vol, void *_node, void *_cookie)
{
	#pragma unused(_node)

	nspace *vol = (nspace *)_vol;
	int32 *cookie = _cookie;

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_close_attrdir called\n"));

	LOCK_VOL(vol);

#if 0
	if (check_nspace_magic(vol, "cifs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}
#endif

	*(int32 *)_cookie = 1;
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int cifs_free_attrcookie(void *_vol, void *_node, void *_cookie)
{
	#pragma unused(_vol)
	#pragma unused(_node)

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_free_attrcookie called\n"));

	if (_cookie == NULL) {
		dprintf("error: cifs_free_attrcookie called with null cookie\n");
		return EINVAL;
	}

	*(int32 *)_cookie = 0x87654321;
	FREE(_cookie);

	return 0;
}

int cifs_rewind_attrdir(void *_vol, void *_node, void *_cookie)
{
	#pragma unused(_vol)
	#pragma unused(_node)

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_rewind_attrdir called\n"));

	if (_cookie == NULL) {
		dprintf("error: cifs_rewind_attrcookie called with null cookie\n");
		return EINVAL;
	}

	*(uint32 *)_cookie = 0;
	return 0;
}

int cifs_read_attrdir(void *_vol, void *_node, void *_cookie, long *num, 
	struct dirent *entry, size_t bufsize)
{
	#pragma unused(bufsize)

	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	int32 *cookie = (int32 *)_cookie;
	status_t result = B_OK;

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_read_attrdir called\n"));

	*num = 0;

	LOCK_VOL(vol);
#if 0
	if (check_nspace_magic(vol, "cifs_read_attrdir") ||
		check_vnode_magic(node, "cifs_read_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}
#endif
	if ((*cookie == 0) && (node->mime)) {
		*num = 1;
		
		entry->d_ino = node->vnid;
		entry->d_dev = vol->id;
		entry->d_reclen = 10;
		strcpy(entry->d_name, "BEOS:TYPE");
	}

	*cookie = 1;

	UNLOCK_VOL(vol);
	
	return 0;
}

int cifs_stat_attr(void *_vol, void *_node, const char *name, struct attr_info *buf)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_stat_attr called (%s)\n", name));

	if (strcmp(name, "BEOS:TYPE"))
		return ENOENT;

	LOCK_VOL(vol);
#if 0
	if (check_nspace_magic(vol, "cifs_read_attr") ||
		check_vnode_magic(node, "cifs_read_attr")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}
#endif
	if (node->mime == NULL) {
		UNLOCK_VOL(vol);
		return ENOENT;
	}
	
	buf->type = MIME_STRING_TYPE;
	buf->size = strlen(node->mime) + 1;

	UNLOCK_VOL(vol);
	return 0;
}

int cifs_read_attr(void *_vol, void *_node, const char *name, int type, void *buf, 
	size_t *len, off_t pos)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_read_attr called on %s\n",name));
	if (!strcmp(name, "BEOS:TYPE")) {
			
		if (type != MIME_STRING_TYPE)
			return ENOENT;
	
		LOCK_VOL(vol);
	
		if (node->mime == NULL) {
			UNLOCK_VOL(vol);
			return ENOENT;
		}
	
		strncpy(buf, node->mime + pos, *len - 1);
		((char *)buf)[*len - 1] = 0;
		*len = strlen(buf) + 1;
	
		UNLOCK_VOL(vol);
		return 0;
	}

	return ENOENT;
#if 0
	dprintf("Calling open attr\n");
	return open_file_attr(vol,node,O_RDONLY,NULL,name);
#endif	
}

// suck up application attempts to set mime types; this hides an unsightly
// error message printed out by zip
int cifs_write_attr(void *_vol, void *_node, const char *name, int type,
	const void *buf, size_t *len, off_t pos)
{
	#pragma unused(_vol,_node,name,type,buf,len,pos)

	DPRINTF(CIFS_ATTR_DEBUG, ("cifs_write_attr called, write to attr %s\n",name));

	*len = 0;

	if (strcmp(name, "BEOS:TYPE"))
		return ENOSYS;
		
	if (type != MIME_STRING_TYPE)
		return ENOSYS;

	return 0;
}
