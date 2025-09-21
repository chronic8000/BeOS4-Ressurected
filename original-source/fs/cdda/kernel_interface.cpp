
/*
	cdda-fs - Read audio CD's as CDROMs
	
	Loosely based on the iso9660 sample code (used it as a skeleton)
*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <KernelExport.h>
#include <time.h>
#include "fsproto.h"
#include "cdda_supportfuncs.h"
#include <malloc.h>

#include "iso.h"

static int	fs_mount(nspace_id nsid, const char *device, ulong flags, void *parms, size_t len, void **data, vnode_id *vnid);
static int	fs_unmount(void *_ns);
static int	fs_walk(void *_ns, void *_base, const char *file, char **newpath, vnode_id *vnid);
static int	fs_read_vnode(void *_ns, vnode_id vnid, char r, void **node);
static int	fs_write_vnode(void *_ns, void *_node, char r);
static int	fs_rstat(void *_ns, void *_node, struct stat *st);
static int	fs_open(void *_ns, void *_node, int omode, void **cookie);
static int	fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len);
static int	fs_write(void *_ns, void *_node, void *cookie, off_t pos,	const void *buf, size_t *len);
static int	fs_free_cookie(void *ns, void *node, void *cookie);
static int	fs_close(void *ns, void *node, void *cookie);
static int	fs_access(void *_ns, void *_node, int mode);
static int	fs_opendir(void* _ns, void* _node, void** cookie);
static int	fs_readdir(void *_ns, void *_node, void *cookie, long *num, struct dirent *buf, size_t bufsize);
static int	fs_rewinddir(void *_ns, void *_node, void *cookie);
static int	fs_closedir(void *_ns, void *_node, void *cookie);
static int	fs_free_dircookie(void *_ns, void *_node, void *cookie);
static int	fs_rfsstat(void *_ns, struct fs_info *);
static int	fs_wfsstat(void *ns, struct fs_info *, long mask);
static int	fs_rename(void *ns, void *olddir, const char *oldname, void *newdir, const char *newname);

// attribute functions are defined in cdda_attr.h
#include "cdda_attr.h"

vnode_ops fs_entry =  {
	&fs_read_vnode,
	&fs_write_vnode,
	NULL, 								// remove_vnode
	NULL,								// secure_vnode
	&fs_walk,
	&fs_access,
	NULL, 								// create
	NULL, 								// mkdir
	NULL,								// symlink
	NULL,								// link
	&fs_rename,
	NULL, 								// unlink
	NULL, 								// rmdir
	NULL,								// readlink
	&fs_opendir,
	&fs_closedir,
	&fs_free_dircookie,
	&fs_rewinddir,
	&fs_readdir,
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	&fs_write,
	NULL,								// readv
	NULL,								// writev
	NULL,								// ioctl
	NULL,								// setflags file
	&fs_rstat,
	NULL, 								// wstat
	NULL,								// fsync
	NULL, 								// initialize
	&fs_mount,
	&fs_unmount,
	NULL,								// sync
	&fs_rfsstat,
	&fs_wfsstat,
	NULL,								// select
	NULL,								// deselect
	NULL,								// open_indexdir
	NULL,								// close_indexdir
	NULL,								// free cookie
	NULL,								// rewind index
	NULL,								// read
	NULL,								// create index
	NULL,								// remove index
	NULL,								// rename index
	NULL,								// stat index

	&fs_open_attrdir,
	&fs_close_attrdir,
	&fs_free_attrcookie,
	&fs_rewind_attrdir,
	&fs_read_attrdir,
	&fs_write_attr,
	&fs_read_attr,
	NULL,								// fs_remove_attr,
	NULL,								// fs_rename_attr,
	&fs_stat_attr,

	NULL,								// open_query
	NULL,								// close_query
	NULL,								// free_querycookie
	NULL								// read_query
};

int32	api_version = B_CUR_FS_API_VERSION;

static char* 	gFSName = "CDDA";	// replace w/ your fs name.


static int 
fs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **data, vnode_id *vnid)
{
	int 		result = EINVAL;
	
	nspace*		ns;
	
	//dprintf("fs_mount - ENTER\n");

	// Try and mount volume as a CDDA volume.
	result = CDDAMount(device, O_RDONLY, &ns);
	
	// If it is CDDA
	if (result == B_NO_ERROR)
	{
		*vnid = CDDA_ROOTNODE_ID;
		*data = (void*)ns;
		
		ns->id = nsid;
		
		// You MUST do this. Create the vnode for the root.
		result = new_vnode(nsid, *vnid, (void*)&(ns->rootvnode));
		if (result != B_NO_ERROR)
		{
			free(ns);
			result = EINVAL;
		}
		sprintf(ns->volumename,"Audio CD");
		// read track names from file on harddisk, using cddb-id as identifier
		GetCDDANames(ns,true);
	}
	//dprintf("fs_mount - EXIT, result code is %s\n", strerror(result));
	return result;
}

static int		
fs_unmount(void *_ns)
{
	int result = B_NO_ERROR;
	
	nspace* ns = (nspace*)_ns;
	//dprintf("fs_unmount - ENTER\n");
	result = close(ns->fd);
	
	if(ns->nameschanged==true)
		DumpCDDANames(ns);
	
	for(int i=0;i<ns->toc.numtracks;i++)
		free(ns->toc.tocentry[i].name);
	
	free(ns);
	//dprintf("fs_unmount - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_rfsstat - Fill in fs_info struct for device.
static int	fs_rfsstat(void *_ns, struct fs_info * fss)
{
	nspace* ns = (nspace*)_ns;
	
	// File system flags.
	// note: in 4.1, you can't edit names in the Tracker if the volume is not writable
	fss->flags = B_FS_HAS_ATTR|B_FS_HAS_MIME|B_FS_IS_REMOVABLE|B_FS_IS_PERSISTENT;
	
	// FS block size.
	fss->block_size = 1024;
	
	// IO size - specifies buffer size for file copying
	fss->io_size = 1024*128;
	
	// Total blocks?
	fss->total_blocks=0;
	for(int i=0;i<ns->toc.numtracks;i++)
	{
		fss->total_blocks+=ns->toc.tocentry[i].length*2352+WAV_HEADER_SIZE;
	}
	fss->total_blocks/=1024;
	
	fss->free_blocks = 0;
	
	strncpy(fss->device_name, ns->devicePath, sizeof(fss->device_name));

	strncpy(fss->volume_name, ns->volumename, sizeof(fss->volume_name));
	
	// File system name
	strcpy(fss->fsh_name,"CDDA");
	
	//dprintf("fs_rfsstat - EXIT\n");
	return 0;
}


int fs_wfsstat(void *_ns, struct fs_info *info, long mask)
{
	nspace*		ns = (nspace*)_ns;

	if(mask==WFSSTAT_NAME)
	{
		ns->nameschanged=true;
		//dprintf("name changed\n");
		strncpy(ns->volumename,info->volume_name,B_FILE_NAME_LENGTH);
		return B_OK;
	}
	return EINVAL;
};


/* fs_walk - the walk function just "walks" through a directory looking for
	the specified file. When you find it, call get_vnode on its vnid to init
	it for the kernel.
*/

static int		
fs_walk(void *_ns, void *base, const char *file, char **newpath, 
			vnode_id *vnid)
{
	/* Starting at the base, find file in the subdir, and return path
		string and vnode id of file. */
	int			result = ENOENT;
	int			tracknum;

	char		extbuf[5];
	nspace*		ns = (nspace*)_ns;
	vnode* 		baseNode = (vnode*)base;
	vnode*		newNode = NULL;

	LockFS(ns);

//	dprintf("fs_walk - ENTER, looking for %s\n", file);
	
	if (strcmp(file,".") == 0) 
	{
		*vnid = baseNode->id;
		if (get_vnode(ns->id,*vnid,(void **)&newNode) != 0)
	    		result = EINVAL;
	    else
	    	result = B_NO_ERROR;
	}
	else if (strcmp(file, "..") == 0)
	{
		*vnid = CDDA_ROOTNODE_ID;
		if (get_vnode(ns->id, *vnid, (void**)&newNode) != 0)
			result = EINVAL;
		else
			result = B_NO_ERROR;
	}

	else if(baseNode->id==CDDA_ROOTNODE_ID)
	{
		*vnid=-1;
		if (strcmp(file,"cdda") == 0) 
			*vnid = CDDA_CDDANODE_ID;
		else if (strcmp(file,"wav") == 0) 
			*vnid = CDDA_WAVNODE_ID;
		else if(strcmp(file,"settings")==0)
			*vnid = CDDA_SETTINGS_ID;
		else if(strcmp(file,"README")==0)
			*vnid = CDDA_README_ID;

		if(*vnid>=0)
		{
			if(get_vnode(ns->id,*vnid,(void **)&newNode)!=0)
	    		result = EINVAL;
		    else
		    	result = B_NO_ERROR;
		}
	}

	else
	{
//		dprintf("fs_walk - checking %s\n",file);
		if((tracknum=IndexForFilename(ns,file))>0)
		{
			*vnid= -1;
			if(baseNode->id==CDDA_CDDANODE_ID)
				*vnid = 1000+tracknum;
			else if(baseNode->id==CDDA_WAVNODE_ID)
				*vnid = 2000+tracknum;

			if(*vnid>=0)
			{
				if(get_vnode(ns->id, *vnid, (void**)&newNode) != 0)
					result = EINVAL;
				else
					result = B_NO_ERROR;
			}
		}
	}
//	dprintf("fs_walk - EXIT, result is %s\n", strerror(result));
	UnlockFS(ns);
	return result;
}

// fs_read_vnode - Using vnode id, read vnode information into fs-specific struct,
//					and return it in node. the reenter flag tells you if this function
//					is being called via some other fs routine, so that things like 
//					double-locking can be avoided.
static int		
fs_read_vnode(void *_ns, vnode_id vnid, char reenter, void **node)
{
//	#pragma unused (reenter)

	nspace*		ns = (nspace*)_ns;
	int			result = B_NO_ERROR;
	vnode* 		newNode = (vnode*)calloc(sizeof(vnode), 1);
	
	//dprintf("fs_read_vnode - ENTER,  vnid = %Lu, node 0x%x\n", vnid, newNode);
	
	if (newNode != NULL)
	{
		if (vnid == CDDA_ROOTNODE_ID)
		{
			//dprintf("fs_read_vnode - root node requested.\n");
			memcpy(newNode, &(ns->rootvnode), sizeof(vnode));
			*node = (void*)newNode;
		}
		else
		{
			result = InitNode(ns, newNode, vnid);
			*node = (void*)newNode;
		}
	}
	else result = ENOMEM;
	
	//dprintf("fs_read_vnode - EXIT, result is %s\n", strerror(result));
	return result;
}

static int		
fs_write_vnode(void *ns, void *_node, char reenter)
{
//	#pragma unused (ns)
//	#pragma unused (reenter)
	
	int 		result = B_NO_ERROR;
	vnode* 		node = (vnode*)_node;
	
//	dprintf("fs_write_vnode - ENTER (0x%x)\n", node);
	if((node != NULL)&&(node->id != CDDA_ROOTNODE_ID))
		free(node);
	else
		result=ENOENT;
//	dprintf("fs_write_vnode - EXIT\n");
	return result;
}

// fs_rstat - fill in stat struct
static int		
fs_rstat(void *_ns, void *_node, struct stat *st)
{
	nspace* ns = (nspace*)_ns;
	vnode*	node = (vnode*)_node;
	int 	result = B_ERROR;
	
//	dprintf("fs_rstat - ENTER\n");
	st->st_dev = ns->id;
	st->st_ino = node->id;
//	dprintf("fs_rstat - st_ino=%Ld\n", st->st_ino);
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_blksize = 65536;
	st->st_mode = node->mode;
	
	// Same for file/dir in ISO9660
	if((st->st_mode&S_IFDIR)||((node->id>1000)&&(node->id<2000)))
	{
//		dprintf("stat'ing directory or cdda file\n");
		st->st_size = 0;
		result=B_NO_ERROR;
	}
	else if(node->id==CDDA_SETTINGS_ID)
	{
		int fd=open(SETTINGSFILE,O_RDONLY);
		if(fd>=0)
		{
			result=fstat(fd,st);
// I wish Be would either fully support PowerPC, or else drop it completely
// The current "support" kinda sucks...
#ifdef __POWERPC__
			st->st_mode=(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH );
#endif
			close(fd);
		}
	}
	else if(node->id==CDDA_README_ID)
	{
		st->st_size=strlen(readmefile);
		result=B_NO_ERROR;
	}
	else
	{
		int tracknum=node->id-2000;
		int numframes;
		numframes=ns->toc.tocentry[tracknum-1].length;
		st->st_size = numframes*2352+WAV_HEADER_SIZE;
		result=B_NO_ERROR;
	}
	st->st_ctime = st->st_mtime = st->st_atime = ns->mounttime+(node->id%1000);
//	dprintf("fs_rstat - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_open - Create a vnode cookie, if necessary, to use when
// 				reading/writing a file
static int		
fs_open(void *_ns, void *_node, int omode, void **cookie)
{
	//dprintf("fs_open\n");

	nspace* ns = (nspace*)_ns;
	vnode* 	node = (vnode*)_node;
	int		result = B_NO_ERROR;

	filecookie *koekje=(filecookie *)calloc(sizeof(filecookie),1);
	if(!koekje)
		result=B_ERROR;
	else
	{
		koekje->buffernumber= -1;
		koekje->buffer=(char*)calloc(ns->CDDA_BUFFER_SIZE,1);
		if(!koekje->buffer)
		{
			free(koekje);
			result=B_ERROR;
		}
		else
			*cookie=koekje;
	}
	return result;
}


// fs_write
// the only file that can be written to is the settings file
static int		fs_write(void *_ns, void *_node, void *cookie, off_t pos,	const void *buf, size_t *len)
{
	int		result = B_NO_ERROR;
	nspace* ns = (nspace*)_ns;
	vnode*	node = (vnode*)_node;
	filecookie *koekje=(filecookie*)cookie;

	if ((node->mode & S_IFDIR)) 
		return EISDIR;

	//dprintf("write to file\n");
	if(node->id==CDDA_SETTINGS_ID)
	{
		int fd=open(SETTINGSFILE,O_WRONLY|O_CREAT|O_TRUNC);
		if(fd>=0)
		{
			lseek(fd,pos,SEEK_SET);
			*len=write(fd,buf,*len);
			close(fd);
		}
	}
	else
		result=EPERM;

	return result;
}

// fs_read
// Read a file specified by node, using information in cookie
// and at offset specified by pos. read len bytes into buffer buf.
static int		
fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, 
			size_t *len)
{
	int		result = B_NO_ERROR;
	nspace* ns = (nspace*)_ns;
	vnode*	node = (vnode*)_node;
	filecookie *koekje=(filecookie*)cookie;
	

	if ((node->mode & S_IFDIR)) 
		return EISDIR;

	if(node->id==CDDA_SETTINGS_ID)
	{
		int fd=open(SETTINGSFILE,O_RDONLY);
		if(fd>=0)
		{
			lseek(fd,pos,SEEK_SET);
			*len=read(fd,buf,*len);
			close(fd);
		}
	}
	else if(node->id==CDDA_README_ID)
	{
		int readlen=max_c(0,min_c(strlen(readmefile)-pos,*len));
		memcpy(buf,readmefile+pos,readlen);
		*len=readlen;
	}
	else
	{
		struct stat st;
		fs_rstat(_ns, _node, &st);
	
		int64 totalread= *len;
		if(pos+(*len)>st.st_size)
		{
			totalread=st.st_size-pos;
			if(totalread<0)
				totalread=0;
		}
	
	//	dprintf("fs_read: %d/%d\n",*len,totalread);
		
		char *cbuf=(char*)buf;
		for(int i=0;i<totalread;i++)
			cbuf[i]=0;
		
		if(totalread>0)
		{
			WAVHeader wh(st.st_size-WAV_HEADER_SIZE);
			char *header=(char*)&wh;
			size_t lefttoread=totalread;
			off_t currentpos=pos;
	
			if(pos<sizeof(wh))
			{
				// read (part of) header
				size_t headerpart=min_c(sizeof(wh)-currentpos,lefttoread);
				memcpy(buf,header+currentpos,headerpart);
				lefttoread-=headerpart;
				currentpos+=headerpart;
				cbuf+=headerpart;
			}
			
			if((currentpos+lefttoread)>sizeof(wh))
			{
				// read (part of) cdda data
				off_t cdda_offset=currentpos-sizeof(wh);
	
				// read 'lefttoread' bytes of data starting at 'cdda_offset'
				// into buffer starting at cbuf
	
				long tracknum=node->id-2001; // need zero-based track number!
				long trackoffset=ns->toc.tocentry[tracknum].startaddress;
				
				cdda_readdata(_ns, koekje, cbuf,(trackoffset*CDDA_SECTOR_SIZE)+cdda_offset,lefttoread);
			}
		}
		*len=totalread;
	}
	return result;
}

// fs_close - Do whatever is necessary to close a file, EXCEPT for freeing
//				the cookie!
static int		
fs_close(void *ns, void *node, void *cookie)
{
	//dprintf("fs_close - ENTER\n");
	//dprintf("fs_close - EXIT\n");
	return 0;
}

static int		
fs_free_cookie(void *ns, void *node, void *cookie)
{
	filecookie *koekje=(filecookie*)cookie;
	free(koekje->buffer);
	free(koekje);
	return 0;
}

// fs_access - checks permissions for access.
static int		
fs_access(void *ns, void *node, int mode)
{
	// I don't seem to be needing this function
	return 0;
}


// fs_opendir - creates fs-specific "cookie" struct that keeps track of where
//					you are at in reading through directory entries in fs_readdir.
static int		
fs_opendir(void *_ns, void *_node, void **cookie)
{
	// ns 		- global, fs-specific struct for device
	// node		- directory to open
	// cookie	- allocate fs-specific cookie and return here to
	//				maintain dirent position info for reads.	
//	nspace* 				ns = (nspace*)_ns;
	vnode*					node = (vnode*)_node;
	int						result = B_NO_ERROR;
	dircookie* 				dirCookie = (dircookie*)malloc(sizeof(dircookie));

	//dprintf("fs_opendir - ENTER, node is 0x%x\n", _node); 
	if (!(node->mode & S_IFDIR))
	{
		result = EMFILE;
	}
	else
	{
		if (dirCookie != NULL)
		{
			dirCookie->vnid=node->id;
			dirCookie->counter = 1;
			*cookie = (void*)dirCookie;
		}
		else
		{
			// Mem error
			result = ENOMEM;
		}
	}
//	dprintf("fs_opendir - EXIT\n");
	return result;
}

// fs_readdir - read 1 or more dirents, keep state in cookie, return
//					0 when no more entries.
static int		
fs_readdir(void *_ns, void *_node, void *_cookie, long *num, 
			struct dirent *buf, size_t bufsize)
{
	int 		result = B_NO_ERROR;
	vnode* 		node = (vnode*)_node;
	nspace* 	ns = (nspace*)_ns;
	dircookie* 	dirCookie = (dircookie*)_cookie;

//	dprintf("fs_readdir - ENTER, bufsize: %d\n",bufsize);
	result=CDDAReadDirEnt(ns, dirCookie, buf, bufsize);
	
	// If we succeeded, return 1, the number of dirents we read.
	if (result == B_NO_ERROR)
		*num = 1;
	else
		*num = 0;
	
	// When you get to the end, don't return an error, just return
	// a zero in *num.
	
	if (result == ENOENT) result = B_NO_ERROR;
//	dprintf("fs_readdir - EXIT, result is %s\n", strerror(result));

	dirCookie->counter++;
	return result;
}
			
// fs_rewinddir - set cookie to represent beginning of directory, so
//					later fs_readdir calls start at beginning.
static int		
fs_rewinddir(void *ns, void *_node, void* _cookie)
{
//	#pragma unused (ns)
//	#pragma unused (node)
	
	// ns 		- global, fs-specific struct for device
	// node		- directory to rewind
	// cookie	- current cookie for directory.	
	int			result = EINVAL;
	vnode* 		node = (vnode*)_node;
	dircookie*	cookie = (dircookie*)_cookie;
	//dprintf("fs_rewinddir - ENTER\n");
	if (cookie != NULL)
	{
		cookie->vnid=node->id;
		cookie->counter = 1;
		result = B_NO_ERROR;
	}
	//dprintf("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_closedir - Do whatever you need to to close a directory (sometimes
//					nothing), but DON'T free the cookie!
static int		
fs_closedir(void *ns, void *node, void *cookie)
{
//	#pragma unused (ns)
//	#pragma unused (node)
//	#pragma unused (cookie)
	// ns 		- global, fs-specific struct for device
	// node		- directory to close
	// cookie	- current cookie for directory.	
//	dprintf("fs_closedir - ENTER\n");
//	dprintf("fs_closedir - EXIT\n");
	return 0;
}

// fs_free_dircookie - Free the fs-specific cookie struct
static int		
fs_free_dircookie(void *ns, void *node, void *cookie)
{
//	#pragma unused (ns)
//	#pragma unused (node)
	// ns 		- global, fs-specific struct for device
	// node		- directory related to cookie
	// cookie	- current cookie for directory, to free.	
//	dprintf("fs_free_dircookie - ENTER\n");
	if (cookie != NULL)
	{
		free(cookie);
	}
//	dprintf("fs_free_dircookie - EXIT\n");
	return 0;
}

static int fs_rename(void *_ns, void *olddir, const char *oldname, void *newdir, const char *newname)
{
	nspace* 	ns = (nspace*)_ns;
	vnode *d1=(vnode*)olddir;
	vnode *d2=(vnode*)newdir;

	//dprintf("fs_rename\n");

	// can't move to other folder, and can't move cdda and wav directories
	if((d1->id!=d2->id)||(d1->id==1))
	{
		dprintf("fs_rename: can't move files\n");
		return B_NOT_ALLOWED;
	}

	LockFS(ns);
	if(IndexForFilename(ns,newname)>0)
	{
		dprintf("fs_rename: can't rename to %s: file exists\n",newname);
		UnlockFS(ns);
		return B_FILE_EXISTS;
	}

	int tracknum;
	tracknum=IndexForFilename(ns,oldname);
	
	if(tracknum<0)
	{
		dprintf("fs_rename: can't find file %s\n",oldname);
		UnlockFS(ns);
		return B_ENTRY_NOT_FOUND;
	}

	tracknum-=1;
	
	char *oldstring=ns->toc.tocentry[tracknum].name;
	ns->toc.tocentry[tracknum].name=SanitizeString(strdup(newname));
	free(oldstring);
	ns->nameschanged=true;

	// generate notification for both directories
	notify_listener(B_ENTRY_MOVED, ns->id, CDDA_CDDANODE_ID, CDDA_CDDANODE_ID,1001+tracknum, newname);
	notify_listener(B_ENTRY_MOVED, ns->id, CDDA_WAVNODE_ID,  CDDA_WAVNODE_ID, 2001+tracknum, newname);

	//dprintf("fs_rename: renamed file\n");
	UnlockFS(ns);
	return B_OK;
};
