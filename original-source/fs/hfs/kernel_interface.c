
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

#include <ide_calls.h>
#include <fsproto.h>
#include <fs_attr.h>
#include <TypeConstants.h>
#include <lock.h>

#include "internal.h"
#include "data.h"
#include "block.h"
#include "low.h"
#include "file.h"
#include "btree.h"
#include "node.h"
#include "record.h"
#include "volume.h"

#ifdef __BEOS__
# include "lock.h"
# include "cache.h"
#endif

typedef struct 	vnode vnode;
typedef struct 	nspace nspace;
typedef struct 	cacherec cacherec;
typedef struct 	driver_desc driver_desc;
typedef struct	dirpos dirpos;
typedef struct 	dir_info dir_info;
typedef struct 	dev_info dev_info;
typedef struct 	partition_desc partition_desc;

#define HASH_RECS		256
#define HASH_SIZE		64
#define HASH_BITMASK	0x0000003F

typedef struct rsrc_header
{
  uint32 offsToData;
  uint32 offsToMap;
  uint32 dataLen;
  uint32 mapLen;
} rsrc_header;

typedef struct rsrc_data
{
  uint32 dataLen;
  uchar data[1];
} rsrc_data;

typedef struct rsrc_map
{
  uchar reserved[22];
  uint16 attributes;
  uint16 offsToTypeList;
  uint16 offsToNameList;
  uint16 numTypesMinusOne;
  uchar typeList[1];
} rsrc_map;

typedef struct rsrc_typelist
{
  uint32 rsrcType;
  uint16 numRsrcMinusOne;
  uint16 offsToRefList;
} rsrc_typelist;

typedef struct rsrc_reflist
{
  int16  rsrcID;
  uint16 offsToName;
  uint8  attributes;
  uint8  hiOffsToData;
  uint16 loOffsToData;
  uint32 reserved;
} rsrc_reflist;

typedef struct rsrc_namelist
{
  uint8 nameLength;
  char name[1];
} rsrc_namelist;

struct dirpos {
  char			    name[32];  
  int				count;
  int               fresh;
  hfsdir	*		dir;
};

typedef struct _attrdir_cookie {
    int             num;
    int             isdir;
} attrdir_cookie;

typedef struct file_cookie {
  hfsfile file;
  long    lglen;
  int     isdir;
} file_cookie;

struct vnode {
	lock			vnLock;
	vnode_id		vnid;
	vnode_id		parid;
	char			name[32];
	nspace			*ns;
	CatDataRec		cdr;
	char			dirty;
    int             deleted;
    int             iconsChecked;
    uchar *         bigIcon;
    uchar *         littleIcon;
    uint32          special;
};

struct cacherec {
	cacherec *		next;
	cacherec *		prev;
	cacherec *		fresher;
	vnode_id		vnid;
	vnode_id		parid;
	char			name[32];
    char            invalid;
};

struct nspace {
    char            devicePath[128];
    device_geometry geo;
	lock		theLock;
	lock		cacheLock;
    sem_id      masterSem;
    int			catmod;
	nspace_id	nsid;
    int         unmounted;
	hfsvol *	libvol;
	vnode *		root_vnode;
	cacherec	cache_recs[HASH_RECS];
	cacherec *	cache_hash[HASH_SIZE];
	cacherec *	cache_mentos; /* The freshmaker! */
	cacherec *	cache_mentosTail;
};

lock hfs_lock={-1,-1};

int hfs_abort(void);

#define checkpoint printf("hfs: checkpoint %s(%d)\n",__FILE__,__LINE__);

static int		machfs_read_vnode(void *ns, vnode_id vnid, char r, void **node);
static int		machfs_write_vnode(void *ns, void *node, char r);
static int		machfs_remove_vnode(void *ns, void *node, char r);
static int		machfs_secure_vnode(void *ns, void *node);
static int		machfs_walk(void *ns, void *base, const char *file,
						char **newpath, vnode_id *vnid);
static int		machfs_access(void *ns, void *node, int mode);
static int		machfs_create(void *ns, void *dir, const char *name,
					int perms, int omode, vnode_id *vnid, void **cookie);
static int      machfs_rename(void *_ns, void *_olddir, const char *_oldname,
					void *_newdir, const char *_newname);
static int		machfs_mkdir(void *ns, void *dir, const char *name, int perms);
static int		machfs_unlink(void *ns, void *dir, const char *name);
static int		machfs_rmdir(void *ns, void *dir, const char *name);
static int		machfs_opendir(void *ns, void *node, void **cookie);
static int		machfs_closedir(void *ns, void *node, void *cookie);
static int		machfs_free_dircookie(void *ns, void *node, void *cookie);
static int		machfs_rewinddir(void *ns, void *node, void *cookie);
static int		machfs_readdir(void *ns, void *node, void *cookie,
					long *num, struct dirent *buf, size_t bufsize);
static int		machfs_rstat(void *ns, void *node, struct stat *st);
static int		machfs_wstat(void *ns, void *node, struct stat *st, long mask);
static int      machfs_fsync(void *ns, void *node);
static int		machfs_open(void *ns, void *node, int omode, void **cookie);
static int		machfs_close(void *ns, void *node, void *cookie);
static int		machfs_free_cookie(void *ns, void *node, void *cookie);
static int		machfs_read(void *ns, void *node, void *cookie, off_t pos,
						void *buf, size_t *len);
static int		machfs_write(void *ns, void *node, void *cookie, off_t pos,
						const void *buf, size_t *len);
static int		machfs_ioctl(void *ns, void *node, void *cookie, int cmd,
						void *buf, size_t len);
static int		machfs_mount(nspace_id nsid, const char *device, ulong flags,
						void *parms, size_t len, void **data, vnode_id *vnid);
static int		machfs_unmount(void *ns);
static int		machfs_rfsstat(void *ns, struct fs_info *);
static int      machfs_wfsstat(void *ns, struct fs_info *, long mask);
static int		machfs_sync(void *ns);
static int      machfs_initialize(const char *devname, void *parms, size_t len);
static int      machfs_open_attrdir(void *_ns, void *_node, void **_cookie);
static int      machfs_close_attrdir(void *_ns, void *_node, void *_cookie);
static int      machfs_free_attr_dircookie(void *_ns, void *_node, void *_cookie);
static int      machfs_rewind_attrdir(void *_ns, void *_node, void *_cookie);
static int      machfs_read_attrdir(void *_ns, void *_node, void *_cookie,
									int32 *num, struct dirent *buf,
									size_t bufsize);
static int      machfs_read_attr(void *_ns, void *_node, const char *name,
								 int type, void *buf, size_t *len,
								 off_t pos);
static int      machfs_write_attr(void *_ns, void *_node, const char *name,
								 int type, const void *buf, size_t *len,
								 off_t pos);
static int      machfs_stat_attr(void *_ns, void *_node, const char *name,
								 struct attr_info *buf);
static int	  machfs_symlink(void *_ns, void *_dir, const char *name, const char *path);

/* Internally used functions */
static int      do_open(nspace *ns, vnode *node, void **cookie, int omode);
static int      do_unlink(nspace *ns, vnode *dir, vnode *vn);

#define HFS_MASTER_SEM_COUNT 1
#define VNOP_PRE             acquire_sem(((nspace*)_ns)->masterSem)
#define VNOP_POST            release_sem(((nspace*)_ns)->masterSem)

vnode_ops fs_entry =  {
	&machfs_read_vnode,
	&machfs_write_vnode,
	&machfs_remove_vnode,
	&machfs_secure_vnode,
	&machfs_walk,
	&machfs_access,
	&machfs_create,
	&machfs_mkdir,
	&machfs_symlink,
	NULL,
	&machfs_rename,
	&machfs_unlink,
	&machfs_rmdir,
	NULL,
	&machfs_opendir,
	&machfs_closedir,
	&machfs_free_dircookie,
	&machfs_rewinddir,
	&machfs_readdir,
	&machfs_open,
	&machfs_close,
	&machfs_free_cookie,
	&machfs_read,
	&machfs_write,
	NULL, /* readv */
	NULL, /* writev */
	&machfs_ioctl,
	NULL,
	&machfs_rstat,
	&machfs_wstat,
	&machfs_fsync,
	&machfs_initialize,
	&machfs_mount,
	&machfs_unmount,
	&machfs_sync,

	&machfs_rfsstat,
	&machfs_wfsstat,

	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	&machfs_open_attrdir,
	&machfs_close_attrdir,
	&machfs_free_attr_dircookie,
	&machfs_rewind_attrdir,
	&machfs_read_attrdir,

	&machfs_write_attr,
	&machfs_read_attr,
    NULL,	/* &machfs_remove_attr, */
	NULL,   /* &machfs_rename_attr, */
	&machfs_stat_attr,
	
	NULL,
	NULL,
	NULL,
	NULL
};

int32	api_version = B_CUR_FS_API_VERSION;

static void filter_mac_name(char *name);
static void unfilter_mac_name(char *name);

int  vnid_hash_func(vnode_id vnid);
void vnid_hash_init(nspace *ns);
void vnid_hash_init(nspace *ns);
cacherec *vnid_hash_getfree(nspace *ns);
int vnid_hash_insert(nspace *ns, cacherec *r);
int vnid_hash_lookup(nspace *ns, vnode_id vnid, vnode_id *parid, char *name);
int vnid_hash_memoize(nspace *ns, vnode_id vnid, vnode_id parid, const char *name);

const uint16 macromantou[] = {
0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1, 0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3, 0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF, 0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211, 0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x2126, 0x00E6, 0x00F8,
0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB, 0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA, 0x00FF, 0x0178, 0x2044, 0x00A4, 0x2039, 0x203A, 0xFB01, 0xFB02,
0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC, 0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
};

#define convert_u_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}

#define convert_utf8_to_u(str, uni_str) \
{\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		int   val;\
		val = ((str[0]&7)<<18) | ((str[1]&63)<<12) | ((str[2]&63)<<6) | (str[3]&63);\
		uni_str[0] = 0xd7c0+(val>>10);\
		uni_str[1] = 0xdc00+(val&0x3ff);\
		uni_str += 2;\
		str += 4;\
	}\
}

void convert_mac_to_utf8(const uchar *mac, uchar *utf)
{
	uint16 unicode[33];
	uint16 *u = unicode;
	while (*mac) {
	  *(u++) = macromantou[*(mac++)];
	};
	*u = 0;
	u = unicode;
	do {
	    convert_u_to_utf8(utf,u);
	} while (*(u-1) != 0);
}

void convert_utf8_to_mac(const uchar *utf, uchar *mac)
{
	int i;
	uint16 unicode[33];
	uint16 *u = unicode;
	do {
	    convert_utf8_to_u(utf,u);
	} while (*(u-1) != 0);
	u = unicode;
	while (*u) {
		if (*u <= 0x7F) {
			*(mac++) = *(u++);
		} else {
			for (i=0x80;i<0x100;i++) {
				if (macromantou[i] == *u) {
					*mac = i;
					break;
				};
			};
			if (i == 0x100) {
				*mac = '?';
			};
			mac++; u++;
		};
	};
	*mac = 0;
}

static void
filter_mac_name(char *name)
{
	for(; *name; name++) {
		if (*name == ':')
			// ':' would cause problems on the mac side
			*name = 0x7f;
		if (*name == '/')
			*name = ':';
	}
}

static void
unfilter_mac_name(char *name)
{
	for(; *name; name++) {
		if (*name == 0x7f)
			*name = ':';
		else if (*name == ':')
			*name = '/';
	}
}


int vnid_hash_func(vnode_id vnid)
{
	return vnid & HASH_BITMASK;
}

void vnid_hash_init(nspace *ns)
{
	int i;

	ns->cache_mentos = ns->cache_recs;
	ns->cache_mentosTail = ns->cache_recs+HASH_RECS-1;
	ns->cache_recs[0].vnid = 0;
	ns->cache_recs[0].fresher = ns->cache_recs+1;
	ns->cache_recs[0].next = ns->cache_recs[0].prev = NULL;
	for (i=1;i<HASH_RECS-1;i++) {
		ns->cache_recs[i].vnid = 0;
		ns->cache_recs[i].fresher = ns->cache_recs+i+1;
		ns->cache_recs[i].next = ns->cache_recs[i].prev = NULL;
	};
	ns->cache_recs[i].vnid = 0;
	ns->cache_recs[i].fresher = NULL;
	ns->cache_recs[i].next = ns->cache_recs[i].prev = NULL;

	for (i=0;i<HASH_SIZE;i++)
		ns->cache_hash[i] = NULL;
}

cacherec * vnid_hash_getfree(nspace *ns)
{
	cacherec *r;
	int i=0;

	for (r=ns->cache_mentos;r;r=r->fresher)
		i++;
	if (i<HASH_RECS)
		printf("Mentos DECREASING!!!!! ...in getfree at %d\n",i);

	if (!(ns->cache_mentos))
		printf("Out of mentos!\n");
	r = ns->cache_mentos;
	ns->cache_mentos = r->fresher;
	if (r->vnid != 0) {
		/* cacherec is in use, so we have to remove it from the
		   linked list in which it resides */
		if (r->prev == NULL) {
			/* It's the first cacherec in the list, so we have to
			   update the hash table's pointer... to find it,
			   just call the hash function */
			ns->cache_hash[vnid_hash_func(r->vnid)] = r->next;
		} else
			r->prev->next = r->next;
		if (r->next != NULL)
			r->next->prev = r->prev;
	};

	return r;
}

int vnid_hash_insert(nspace *ns, cacherec *r)
{
	int i=0;
	cacherec *tmp,**list = &ns->cache_hash[vnid_hash_func(r->vnid)];
	
	r->prev = NULL;
	r->next = *list;
	if (*list)
		(*list)->prev = r;
	*list = r;
	
	ns->cache_mentosTail->fresher = r;
	ns->cache_mentosTail = r;
	r->fresher = NULL;
	
	return 0;
}

int vnid_hash_lookup(nspace *ns, vnode_id vnid, vnode_id *parid, char *name)
{
	cacherec *r;

	LOCK(ns->cacheLock);

	r = ns->cache_hash[vnid_hash_func(vnid)];
	
	while (r && (r->vnid!=vnid)) r=r->next;
	if (!r || r->invalid) {
		UNLOCK(ns->cacheLock);
		return -1;
	};
	
	*parid = r->parid;
	strcpy(name,r->name);

	UNLOCK(ns->cacheLock);

	return 0;
}

int vnid_hash_invalidate(nspace *ns, vnode_id vnid)
{
	cacherec *r;

	LOCK(ns->cacheLock);
	
	/* First look to see if we have it memoized already */
	r = ns->cache_hash[vnid_hash_func(vnid)];
	
	while (r && (r->vnid!=vnid)) r=r->next;
	if (r) {
	  /* If we have it, don't bother moving it around, just invalidate it */
	  r->invalid = 1;
	};

	UNLOCK(ns->cacheLock);
	return 0;
}

int vnid_hash_memoize(nspace *ns, vnode_id vnid, vnode_id parid, const char *name)
{
	cacherec *r;

	LOCK(ns->cacheLock);
	
	/* First look to see if we have it memoized already */
	r = ns->cache_hash[vnid_hash_func(vnid)];
	
	while (r && (r->vnid!=vnid)) r=r->next;
	if (!r) {
	  /* We don't have it already */
	  r = vnid_hash_getfree(ns);
	  r->vnid = vnid;
	  r->parid = parid;
	  strcpy(r->name,name);
	  r->invalid = 0;
	  vnid_hash_insert(ns,r);
	} else {
	  /* We do have it, so replace the values */
	  r->vnid = vnid;
	  r->parid = parid;
	  strcpy(r->name,name);
	  r->invalid = 0;
	  /* We could refresh it, but I don't feel like it */
	};
	
	UNLOCK(ns->cacheLock);
	return 0;
}

/*
static int
machfs_walk_leaves(nspace *ns, vnode_id vnid)
{
	node n;
	unsigned char *ptr;
	int parid;
	char name[40];
	int recnum;
	CatKeyRec key;
	CatDataRec cdr;
	unsigned long first = ns->libvol->cat.hdr.bthFNode;
	unsigned long last = ns->libvol->cat.hdr.bthLNode;
	
	n.nnum = first;
	n.bt = &ns->libvol->cat;
		
	printf("Time to go walking...\n");
		
	while (1) {
		printf("Getting node %d\n",n.nnum);
		bt_getnode(&n);
		recnum = n.nd.ndNRecs;
		for (recnum = n.nd.ndNRecs-1; recnum >= 0; recnum--) {
			n.rnum = recnum;
			printf("Record %d of node %d\n",n.rnum,n.nnum);
			ptr = HFS_NODEREC(n,n.rnum);
			r_unpackcatkey(ptr,&key);
			strcpy(name,key.ckrCName);
			parid = key.ckrParID;
			printf("Node key: %d->\"%s\"\n",parid,name);
			r_unpackcatdata(HFS_RECDATA(ptr),&cdr);
			switch (cdr.cdrType) {
				case cdrDirRec:
					printf("  Directory record\n"
					       "  cnid = %d\n",
					       cdr.u.dir.dirDirID);
					break;
				case cdrFilRec:
					printf("  File record\n"
					       "  cnid = %d\n",
					       cdr.u.fil.filFlNum);
					break;
				case cdrThdRec:
					printf("  Directory thread\n"
					       "  parid = %d\n"
					       "  name  = \"%s\"\n",
					       cdr.u.dthd.thdParID,
					       cdr.u.dthd.thdCName);
					break;
				case cdrFThdRec:
					printf("  File thread\n"
					       "  parid = %d\n"
					       "  name  = \"%s\"\n",
					       cdr.u.fthd.fthdParID,
					       cdr.u.fthd.fthdCName);
					break;
				default:
					printf("Unknown cdr type found!\n");
					break;
			};	
		};
		if (n.nnum == last)
			break;
		n.nnum = n.nd.ndFLink;
	};
	
	return 0;
}
*/

static int
machfs_linear_find(nspace *ns, vnode_id vnid, vnode_id *parid, char *name, CatDataRec *cdr)
{
	node n;
	unsigned char *ptr;
	int recnum;
	CatKeyRec key;
	CatDataRec cdrlocal;
	unsigned long first = ns->libvol->cat.hdr.bthFNode;
	unsigned long last = ns->libvol->cat.hdr.bthLNode;
	
	if (cdr == NULL)
		cdr = &cdrlocal;
	
	n.nnum = first;
	n.bt = &ns->libvol->cat;
		
	while (1) {
		bt_getnode(&n);
		recnum = n.nd.ndNRecs;
		for (recnum = n.nd.ndNRecs-1; recnum >= 0; recnum--) {
			n.rnum = recnum;
			ptr = HFS_NODEREC(n,n.rnum);
			r_unpackcatdata(HFS_RECDATA(ptr),cdr);
			switch (cdr->cdrType) {
				case cdrDirRec:
					if (vnid == cdr->u.dir.dirDirID) {
						r_unpackcatkey(ptr,&key);
						strcpy(name,key.ckrCName);
						*parid = key.ckrParID;
						return 0;
					};
					break;
				case cdrFilRec:
					if (vnid == cdr->u.fil.filFlNum) {
						r_unpackcatkey(ptr,&key);
						strcpy(name,key.ckrCName);
						*parid = key.ckrParID;
						return 0;
					};
					break;
				case cdrThdRec:
				case cdrFThdRec:
					/* Not what we're looking for */
					break;
				default:
					printf("Unknown cdr type found, btree corrupt?!?\n");
					break;
			};	
		};
		if (n.nnum == last)
			break;
		n.nnum = n.nd.ndFLink;
	};
	
	/* Couldn't find it! */
	return -1;
}

static int
lookup_name(nspace *ns, vnode_id vnid, vnode_id *parid, char *name, CatDataRec *cdr)
{
	int i;
	CatDataRec cdrlocal;
	i = vnid_hash_lookup(ns,vnid,parid,name);

	if (cdr == NULL)
		cdr = &cdrlocal;

	if (i != 0) {
		if (v_catsearch(ns->libvol,vnid,"",cdr,NULL,NULL) <= 0) {
			if (machfs_linear_find(ns,vnid,parid,name,cdr) != 0) {
			  printf("hfs: linear leaf search on %Lx failed!  "
					 "vnode doesn't exist...\n",vnid);
			  return ENOENT;
			} else
			  return 0;
		};

		switch (cdr->cdrType) {
			case cdrThdRec:
				*parid = cdr->u.dthd.thdParID;
				strcpy(name,cdr->u.dthd.thdCName);
				break;
			case cdrFThdRec:
				*parid = cdr->u.fthd.fthdParID;
				strcpy(name,cdr->u.fthd.fthdCName);
				break;
			default:
				printf("hfs: unrecognized thread type! (%d)\n",cdr->cdrType);
				return EINVAL;
		};
	};

	if (v_catsearch(ns->libvol,*parid,name,cdr,NULL,NULL) <= 0) {
		printf("hfs: second v_catsearch failed on (%Lx:'%s')\n",
			   *parid,name);
		return ENOENT;
	};

	return 0;
}

static int
machfs_read_vnode(void *_ns, vnode_id vnid, char r, void **node)
{
	nspace		*ns;
	vnode *vn;
	vnode_id sane;
	CatDataRec thread;
	int err=0,i;
	uint32 flags;
	
	ns = (nspace *) _ns;

	VNOP_PRE;

	vn = (vnode *)malloc(sizeof(vnode));
	if (vn == NULL) {
		VNOP_POST;
		return ENOMEM;
	}
	vn->ns = ns;
	vn->vnid = vnid;
	vn->dirty = 1;
	vn->deleted = 0;
	vn->special = 0;
	vn->iconsChecked = 0;
	vn->bigIcon = vn->littleIcon = NULL;
	thread.cdrType = 250;
		
	err = lookup_name(ns,vnid,&vn->parid,vn->name,&vn->cdr);
	if (err)
		goto read_vnode_done;

	/* Sanity checks... */

	switch (vn->cdr.cdrType) {
		case cdrDirRec:
			sane = vn->cdr.u.dir.dirDirID;
			break;
		case cdrFilRec:
			sane = vn->cdr.u.fil.filFlNum;
			break;
		default:
			printf("hfs: unrecognized cdr type!\n");
			free(vn);
			err = EINVAL;
			goto read_vnode_done;
	};
	
	if (vn->vnid != sane) {
		printf("hfs: vnid does not match desired\n");
		free(vn);
		err = EINVAL;
		goto read_vnode_done;
	};

	vn->vnLock.s = create_sem(0,"HFS_vnsem");
	vn->vnLock.c = 1;

	*node = vn;

read_vnode_done:

	VNOP_POST;

	return err;
}

static int
write_vnode(nspace *ns, vnode *vn)
{
  CatKeyRec key;
  unsigned char pkey[HFS_CATKEYLEN];
  node n;
  int found,err=0;

  if (ns->libvol->flags & HFS_READONLY)
	return EPERM;
  
  if (vn->deleted)
	return 0;
  
  r_makecatkey(&key, vn->parid, vn->name);
  r_packcatkey(&key, pkey, 0);

  found = bt_search(&ns->libvol->cat, pkey, &n);
  if (found <= 0) {
	printf("hfs: write_vnode couldn't find node (%Lx:\"%s\")... "
		   "b-tree must be corrupt!\n",vn->parid,vn->name);
	printf("Spinning...\n");
	for (;1;);
	err = ENOENT;
  } else {
	if (v_putcatrec(&vn->cdr,&n) < 0) {
	  printf("hfs: write_vnode couldn't write node (%Lx:\"%s\")... "
			 "b-tree must be corrupt!\n",vn->parid,vn->name);
	  printf("Spinning...\n");
	  for (;1;);
	  err = ENOENT;
	};
  };

  if (err == 0) {
	if (v_flush(ns->libvol,0) < 0)
	  err = EINVAL;
  };
  
  return err;
}

static int
machfs_write_vnode(void *_ns, void *_vn, char r)
{
	nspace		*ns;
	vnode		*vn;
	unsigned char *ptr;
	int len,err=0;
	hfsvol *vol;
	hfsfile file;
	
	if (!r) {
	  VNOP_PRE;
	};

	ns = (nspace *) _ns;
	vn = (vnode *) _vn;

	vol = ns->libvol;

	if ((ns->libvol->flags & HFS_READONLY) || (!vn->dirty))
		goto free_it;
	
	err = write_vnode(ns,vn);

free_it:
	
	delete_sem(vn->vnLock.s);
	if (vn->bigIcon) free(vn->bigIcon);
	if (vn->littleIcon) free(vn->littleIcon);
	free(vn);

write_vnode_done:
	if (!r) {
	  VNOP_POST;
	};
	return err;
}

int remove_vnode_debug=0;

static int
machfs_remove_vnode(void *_ns, void *_node, char r)
{
	nspace		*ns;
	vnode		*node;
	hfsvol *vol;
	hfsfile file;
	int err=0;

	ns = (nspace *) _ns;
	node = (vnode *) _node;

	vol = ns->libvol;

	/*
	printf("hfs: remove_vnode (%Lx:%s)\n",node->parid,node->name);
	*/
	/* XXX is this the correct behavior? -geh */

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;

	if (!r) {
	  VNOP_PRE;
	};
	
	if (node->cdr.cdrType == cdrFilRec) {
		file.parid = node->parid;
		strcpy(file.name,node->name);
		file.cat = node->cdr;
		file.vol = vol;
		file.flags = 0;
		file.cat.u.fil.filLgLen  = 0;
		file.cat.u.fil.filRLgLen = 0;
	
		remove_vnode_debug=1;
		f_selectfork(&file, 0);
		if (f_trunc(&file) < 0)
			err = EINVAL;
		
		remove_vnode_debug=2;
		f_selectfork(&file, 1);
		if (f_trunc(&file) < 0)
			err = EINVAL;

	};

rmvn_free_it:	
	delete_sem(node->vnLock.s);
	if (node->bigIcon) free(node->bigIcon);
	if (node->littleIcon) free(node->littleIcon);
	free(node);

	if (!r) {
	  VNOP_POST;
	};
	return err;
}

static int
machfs_secure_vnode(void *_ns, void *_node)
{
  /*	printf("secure called!\n");
   */	return 0;
}

static int
lookup_id(nspace *ns, vnode *base, const char *file, vnode_id *vnid)
{
  CatDataRec cdr;
  
  if (*file == '\0') {  /* sanity check */
	printf("hfs: lookup_id called with an empty filename?!?\n");
	return ENOENT;
  }
  
  if (strcmp(file,".") == 0) {
	*vnid = base->vnid;
	return 0;
  };
  
  if (strcmp(file,"..") == 0) {
	*vnid = base->parid;
	return 0;
  };
  
  if (v_catsearch(ns->libvol,base->vnid,(char*)file,&cdr,NULL,NULL) <= 0)
	return ENOENT;

  switch (cdr.cdrType) {
  case cdrDirRec:
	*vnid = cdr.u.dir.dirDirID;
	break;
  case cdrFilRec:
	*vnid = cdr.u.fil.filFlNum;
	break;
  case cdrThdRec:
	printf("hfs: Directory thread found on walk!  Wierd.\n");
	return EINVAL;
  case cdrFThdRec:
	printf("hfs: File thread found on walk!  Wierd.\n");
	return EINVAL;
  default:
	printf("hfs: Unknown cdr type found on walk!  Scarey.\n");
	return EINVAL;
  };

  vnid_hash_memoize(ns,*vnid,base->vnid,file);
  
  return 0;
}

/* Assumes catalog is locked */
static int
lookup(nspace *_ns, vnode *base, const char *file, vnode_id *vnid, vnode **n)
{
  int err = lookup_id(_ns,base,file,vnid);
  if (err != 0)
	return err;
  
  VNOP_POST;
  err = get_vnode(_ns->nsid,*vnid,(void **)n);
  VNOP_PRE;
  if (err != 0)
	return err;

  return 0;
}

static int
machfs_walk(void *_ns, void *_base, const char *_file,
		char **newpath, vnode_id *vnid)
{
	nspace		*ns;
	vnode		*base;
	CatDataRec cdr;
	vnode vn;
	vnode *fred;
	int err=0;
	char file[HFS_MAX_FLEN*4+1];
	
	convert_utf8_to_mac((uchar*)_file,(uchar*)file);
	unfilter_mac_name(file);

	if (strlen(file) > HFS_MAX_FLEN)
		return ENAMETOOLONG;

	ns = (nspace *) _ns;
	base = (vnode *) _base;

	VNOP_PRE;	
	err = lookup(ns,base,file,vnid,&fred);
	VNOP_POST;
	
	return err;
}

static int
machfs_access(void *_ns, void *_node, int mode)
{
  return 0;
}

static int
atomic_lookup(nspace *ns, vnode *base, char *name, vnode_id *vnid, vnode **vn)
{
	/*	This interesting bit of code does a lookup while locked.  It ensures
		that if a node exists, we have it.  We need this because the volume
		is unlocked to do a get_vnode. */
		
	int err;
	
	do {
		err = lookup(ns,base,name,vnid,vn);
		if (err == ENOENT) {
			if (lookup_id(ns,base,name,vnid) == ENOENT) {
				*vn = NULL;
				return ENOENT;
			};
		} else if (err != 0)
			return err;
	} while (err);

	return 0;
}

static int
machfs_rename(void *_ns, void *_olddir, const char *_oldname,
		void *_newdir, const char *_newname)
{
  CatDataRec thread,cdr;
  CatKeyRec key;
  unsigned char record[HFS_CATRECMAXLEN];
  node n;
  nspace *ns;
  hfsvol *vol;
  vnode_id vnid,vid;
  vnode *vn,*replacedvn,*dir;
  int found,reclen=0,isdir,err=0,p;
  vnode *olddir,*newdir;
  char oldname[HFS_MAX_FLEN*4+1], newname[HFS_MAX_FLEN*4+1];
  char str[HFS_MAX_FLEN+1];

  ns = (nspace *)_ns;
  vol = ns->libvol;
  olddir = (vnode *)_olddir;
  newdir = (vnode *)_newdir;
  
  if (ns->libvol->flags & HFS_READONLY)
	return EPERM;

  convert_utf8_to_mac((uchar*)_oldname,(uchar*)oldname);
  unfilter_mac_name(oldname);
  convert_utf8_to_mac((uchar*)_newname,(uchar*)newname);
  unfilter_mac_name(newname);
  
  if (strlen(oldname) > HFS_MAX_FLEN)
	return ENAMETOOLONG;
  if (strlen(newname) > HFS_MAX_FLEN)
	return ENAMETOOLONG;

  if ((olddir->vnid == newdir->vnid) && (strcmp(oldname,newname)==0))
      return EEXIST;

  VNOP_PRE;
  
  err = lookup(ns,olddir,oldname,&vnid,&vn);

  if (err != 0) {
	/* If it's not there, we abort the whole thing */
	VNOP_POST;
	return err;
  };

  /* This stuff will never change for a given node, so we really
	 don't have to lock the vnode */
  isdir = (vn->cdr.cdrType == cdrDirRec);
  p = 0;
  if (isdir)
	p = ((vn->cdr.u.dir.dirUsrInfo.frFlags & 0x1000) != 0);
  else
	p = ((vn->cdr.u.fil.filUsrWds.fdFlags & 0x1000) != 0);

  if (p ||
	/* ...this means that the MacOS flags tell us that moving this
	   node is a bad idea.  Deny the user permission to do so. */
	  ((vn->vnid == olddir->vnid) || (vn->vnid == newdir->vnid))) {
	/* ... and this means that we are trying to move a directory
	   into or out of itself.  Not a valid action. */
	
	put_vnode(ns->nsid,vn->vnid);
	VNOP_POST;
	return (p)?EACCES:EINVAL;
  };

  err = atomic_lookup(ns,newdir,newname,&vnid,&replacedvn);
  if ((err != 0) && (err != ENOENT)) {
    put_vnode(ns->nsid,vn->vnid);
    VNOP_POST;
    return err;
  };
  
  err = 0;
  ns->catmod++; /* We'll be modifying the catalog */
  
  /* We must check that we aren't moving a folder into any of it's
	 subfolders. */
  for (vid=newdir->vnid;;) {
  
	if (vid == vn->vnid) {
	  err = EINVAL;
	  goto rename_unlock_all;
	};

	if (vid == HFS_CNID_ROOTDIR)
	  break;

	err = lookup_name(ns,vid,&vid,str,NULL);
	if (err != 0)
	  goto rename_unlock_all;
  };

  if (vn->deleted || newdir->deleted || olddir->deleted) {
	err = ENOENT;
	goto rename_unlock_all; /* All our preparations were in vain; abort */
  };
	
  /* Delete the vnode we are replacing, if it exists and hasn't been
	 deleted yet */
  if (replacedvn && !replacedvn->deleted) {
	if (err = do_unlink(ns,newdir,replacedvn))
	  goto rename_unlock_all;
  };
  
  r_makecatkey(&key,olddir->vnid,oldname);
  r_packcatkey(&key,record,0);

  if (bt_delete(&vol->cat,record) < 0) {
	err = ENOENT;
	printf("hfs: rename has found inconsistent state (bt_delete)\n");
	/* XXX FS is corrupt! */
	goto rename_unlock_all;
  };

  if (olddir->vnid != newdir->vnid) {
	if (isdir)
	  vn->cdr.u.dir.dirUsrInfo.frFlags &= 0xFEFF;
	else
	  vn->cdr.u.fil.filUsrWds.fdFlags &= 0xFEFF;
  };
  vn->parid = newdir->vnid;
  strcpy(vn->name,newname);

  r_makecatkey(&key,newdir->vnid,newname);
  r_packcatkey(&key,record,&reclen);
  r_packcatdata(&vn->cdr,HFS_RECDATA(record),&reclen);

  if (bt_insert(&vol->cat,record,reclen) < 0) {
	err = EINVAL;
	printf("hfs: rename has found inconsistent state (bt_insert)\n");
	/* XXX FS is corrupt! */
	goto rename_unlock_all;
  };
  
  if (isdir) {
	if (v_getdthread(vol,vnid,&thread,&n) <= 0) {
	  err = ENOENT;
	  printf("hfs: rename has found inconsistent state (v_getdthread)\n");
	  /* XXX FS is corrupt! */
	  goto rename_unlock_all;
	};

	thread.u.dthd.thdParID = newdir->vnid;
	strcpy(thread.u.dthd.thdCName,newname);

	if (v_putcatrec(&thread,&n) < 0) {
	  err = EINVAL;
	  printf("hfs: rename has found inconsistent state (v_putcatrec1)\n");
	  /* XXX FS is corrupt! */
	  goto rename_unlock_all;
	};
  } else {
	if (v_getfthread(vol,vnid,&thread,&n) > 0) {
	  thread.u.fthd.fthdParID = newdir->vnid;
	  strcpy(thread.u.fthd.fthdCName,newname);
	  
	  if (v_putcatrec(&thread,&n) < 0) {
		err = EINVAL;
		printf("hfs: rename has found inconsistent state (v_putcatrec2)\n");
		/* XXX FS is corrupt! */
		goto rename_unlock_all;
	  };
	};
  };
  
  if (olddir->vnid != newdir->vnid) {
	if (newdir->vnid == HFS_CNID_ROOTDIR) {
	  if (isdir)
		vol->mdb.drNmRtDirs++;
	  else
		vol->mdb.drNmFls++;	
	  vol->flags |= HFS_UPDATE_MDB;
	};

	if (olddir->vnid == HFS_CNID_ROOTDIR) {
	  if (isdir)
		vol->mdb.drNmRtDirs--;
	  else
		vol->mdb.drNmFls--;	
	  vol->flags |= HFS_UPDATE_MDB;
	};
	
	newdir->cdr.u.dir.dirVal++;
	newdir->cdr.u.dir.dirMdDat = d_tomtime(time(0));
	newdir->dirty = 1;
	if (err = write_vnode(ns,newdir)) {
	  printf("hfs: rename has found inconsistent state (write_vnode1)\n");
	  /* XXX FS is corrupt! */
	  goto rename_unlock_all;
	};
	
	olddir->cdr.u.dir.dirVal--;
	olddir->cdr.u.dir.dirMdDat = d_tomtime(time(0));
	olddir->dirty = 1;
	if (err = write_vnode(ns,olddir)) {
	  /* XXX FS is corrupt! */
	  printf("hfs: rename has found inconsistent state (write_vnode2)\n");
	  goto rename_unlock_all;
	};
  };

  if (err = write_vnode(ns,vn)) {
	printf("hfs: rename has found inconsistent state (write_vnode3)\n");
  };

  vnid_hash_memoize(ns,vn->vnid,newdir->vnid,newname);

  notify_listener(B_ENTRY_MOVED,ns->nsid,olddir->vnid,
				  newdir->vnid,vnid,_newname);
	printf("notify1\n");
  
rename_unlock_all:
rename_unlock_nodes:
  if (replacedvn)
	put_vnode(ns->nsid,replacedvn->vnid);
  put_vnode(ns->nsid,vn->vnid);
  VNOP_POST;

  return err;
}

static int
machfs_create(void *_ns, void *_dir, const char *_name,
			  int omode, int perms, vnode_id *vnid, void **cookie)
{
	nspace		*ns;
	vnode		*dir;
	hfsvol *vol;
	unsigned long id;
	int i,reclen,r,err=0;
	vnode *vn;
	CatKeyRec key;
	char record[HFS_CATRECMAXLEN];
	char name[HFS_MAX_FLEN*4+1];

	char *p;

	ns = (nspace *) _ns;
	dir = (vnode *) _dir;
	vol = ns->libvol;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;
	
	convert_utf8_to_mac((uchar*)_name,(uchar*)name);
	unfilter_mac_name(name);
	
	if (strlen(name) > HFS_MAX_FLEN)
		return ENAMETOOLONG;

	if ((strcmp(name,".")==0) ||
		(strcmp(name,"..")==0) ||
		(name[0] == 0) ||
		(name[0] == '/'))
	  return EINVAL;
	
	VNOP_PRE;

	err = lookup(ns,dir,name,vnid,&vn);

	if ((err != 0) && (err != ENOENT)) {
	  printf("hfs: error looking up file on create (not ENOENT)\n");
	  VNOP_POST;
	  return err;
	};
	
	if (err == 0) {
	  /* If the node has been deleted, we should continue as we were */
	  if (vn->deleted) {
		put_vnode(ns->nsid,vn->vnid);
		vn = NULL;
	  } else {
		err = do_open(ns,vn,cookie,omode);
		if (err != 0) {
		  put_vnode(ns->nsid,vn->vnid);
		  printf("hfs: couldn't create (%Lx:'%s') (machfs_open)\n",
				 dir->vnid,name);
		};
		VNOP_POST;
		return err;
	  };
	} else {
	  err = 0; /* Clear the ENOENT */
	};

	if (dir->deleted) {
	  VNOP_POST;
	  return ENOENT;
	};

	vol->flags |= HFS_UPDATE_MDB;
	ns->catmod++; /* We'll be modifying the catalog */
	
	id = vol->mdb.drNxtCNID++;
	vol->flags |= HFS_UPDATE_MDB;
	
	/* create the vnode */
	vn = (vnode *)malloc(sizeof(vnode));
	if (vn == NULL) {
		VNOP_POST;
		return ENOMEM;
	}
	
	vn->cdr.cdrType   = cdrFilRec;
	vn->cdr.cdrResrv2 = 0;
	
	vn->cdr.u.fil.filFlags = 0;
	vn->cdr.u.fil.filTyp   = 0;
	
	memset(&vn->cdr.u.fil.filUsrWds, 0, sizeof(vn->cdr.u.fil.filUsrWds));
	vn->cdr.u.fil.filUsrWds.fdFlags = 0;
	
	vn->cdr.u.fil.filUsrWds.fdType    = d_getl((unsigned char *)"????");
	vn->cdr.u.fil.filUsrWds.fdCreator = d_getl((unsigned char *)"????");
	
	vn->cdr.u.fil.filFlNum  = id;
	vn->cdr.u.fil.filStBlk  = 0;
	vn->cdr.u.fil.filLgLen  = 0;
	vn->cdr.u.fil.filPyLen  = 0;
	vn->cdr.u.fil.filRStBlk = 0;
	vn->cdr.u.fil.filRLgLen = 0;
	vn->cdr.u.fil.filRPyLen = 0;
	vn->cdr.u.fil.filCrDat  = d_tomtime(time(0));
	vn->cdr.u.fil.filMdDat  = vn->cdr.u.fil.filCrDat;
	vn->cdr.u.fil.filBkDat  = vn->cdr.u.fil.filCrDat;
	
	memset(&vn->cdr.u.fil.filFndrInfo, 0, sizeof(vn->cdr.u.fil.filFndrInfo));
	
	vn->cdr.u.fil.filClpSize = 0;
	
	for (i = 0; i < 3; ++i) {
		vn->cdr.u.fil.filExtRec[i].xdrStABN     = 0;
		vn->cdr.u.fil.filExtRec[i].xdrNumABlks  = 0;	
		vn->cdr.u.fil.filRExtRec[i].xdrStABN    = 0;
		vn->cdr.u.fil.filRExtRec[i].xdrNumABlks = 0;
	}
	
	vn->cdr.u.fil.filResrv = 0;
	vn->parid = dir->vnid;
	strcpy(vn->name,name);
	vn->ns = ns;
	vn->vnid = id;
	vn->dirty = 0;
	vn->deleted = 0;
	vn->special = 0;
	vn->iconsChecked = 0;
	vn->bigIcon = vn->littleIcon = NULL;
	
	r_makecatkey(&key, dir->vnid, (char *)name);
	r_packcatkey(&key, (unsigned char *)record, &reclen);
	r_packcatdata(&vn->cdr, (unsigned char *)HFS_RECDATA(record), &reclen);
	
	if (bt_space(&vol->cat, 1) < 0) {
	  printf("hfs: couldn't create (%Lx:'%s') (bt_space)\n",
			 dir->vnid,name);
	  free(vn);
	  VNOP_POST;
	  return ENOSPC;
	};

	if (bt_insert(&vol->cat, (unsigned char *)record, reclen) < 0) {
	  printf("hfs: couldn't create (%Lx:'%s') (bt_insert)\n",
			 dir->vnid,name);
	  free(vn);
	  VNOP_POST;
	  return EINVAL;
	}
	
	vol->mdb.drFilCnt++;
	if (dir->vnid == HFS_CNID_ROOTDIR)
		vol->mdb.drNmFls++;	

	dir->cdr.u.dir.dirVal++;
	dir->cdr.u.dir.dirMdDat = d_tomtime(time(0));
	dir->dirty = 1;
	err = write_vnode(ns,dir);
	if (err) {
	  printf("hfs: create cannot adjust valences on disk!\n");
	  free(vn);
	  VNOP_POST;
	  return err;
	};
		
	*vnid = id;
	*cookie = NULL;

	vn->vnLock.s = create_sem(0,"vnsem");
	vn->vnLock.c = 1;
	/* We're okay doing this out of order because no one else
	   knows about this node yet, and so couldn't have locked it */
	new_vnode(ns->nsid,id,vn);
	vnid_hash_memoize(ns,id,dir->vnid,name);

	notify_listener(B_ENTRY_CREATED,ns->nsid,dir->vnid,0,vn->vnid,_name);
	printf("notify2\n");

	if (err=do_open(ns,vn,cookie,omode)) {
		printf("Couldn't create (%Lx:'%s') (do_open)\n",
			dir->vnid,name);
		UNLOCK(vn->vnLock);
		VNOP_POST;
		return err;
	};

	VNOP_POST;
	return err;
}

static int
machfs_mkdir(void *_ns, void *_dir, const char *_name, int perms)
{
	nspace	*ns;
	vnode	*dir,*vn;
	vnode_id vnid;
	int hideIt = 0;
	char name[HFS_MAX_FLEN*4+1];
	int err=0;
	
	ns = (nspace *) _ns;
	dir = (vnode *) _dir;

	convert_utf8_to_mac((uchar*)_name,(uchar*)name);
	unfilter_mac_name(name);
	
	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;

	if (strlen(name) > HFS_MAX_FLEN)
		return ENAMETOOLONG;

	VNOP_PRE;
	
	if (dir->deleted) {
	  VNOP_POST;
	  return EINVAL;
	};

	err = atomic_lookup(ns,dir,name,&vnid,&vn);
	if (err != ENOENT) {
	  if (err == 0) {
		put_vnode(ns->nsid,vnid);
		err = EEXIST;
	  };
	  VNOP_POST;
	  return err;
	};
	err = 0;
	
	ns->catmod++;
	
	vnid = ns->libvol->mdb.drNxtCNID;
	if (v_newfolder(ns->libvol,dir->vnid,(char *)name,hideIt) < 0) {
		printf("hfs: couldn't create (%Lx:'%s') (v_newfolder)\n",
			dir->vnid,name);
		VNOP_POST;
		return EINVAL;
	};

	/* We're being slightly inefficient by letting v_newfolder adjust
	   the valence of the directory on disk (actually in the block cache)
	   and then doing it again here, but it should be only a memory copy
	   penalty due to Dominic's nifty block caching, and besides the whole
	   damn FS is pretty inefficient. */
	dir->cdr.u.dir.dirVal++;
	dir->cdr.u.dir.dirMdDat = d_tomtime(time(0));
	dir->dirty = 1;
	/* Just in case... */
	err = write_vnode(ns,dir);
	
	notify_listener(B_ENTRY_CREATED,ns->nsid,dir->vnid,0,vnid,_name);
	printf("notify3\n");
	
	VNOP_POST;
	return err;
}

static int
machfs_unlink(void *_ns, void *_dir, const char *_name)
{
	nspace		*ns;
	vnode		*dir;
	vnode       *vn;
	vnode_id    vnid;
	int			err=0;
	char        name[HFS_MAX_FLEN*4+1];
	char *p;

	ns = (nspace *) _ns;
	dir = (vnode *) _dir;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;
	
	convert_utf8_to_mac((uchar*)_name,(uchar*)name);
	unfilter_mac_name(name);

	if (strlen(name) > HFS_MAX_FLEN)
		return ENAMETOOLONG;

	VNOP_PRE;

	err = lookup(ns,dir,name,&vnid,&vn);

	if (err != 0) {
	  VNOP_POST;
	  return err;
	};

	if (vn->cdr.cdrType == cdrDirRec) {
		err = EISDIR;
		goto done;
	}

	err = do_unlink(ns, dir, vn);

done:
	put_vnode(ns->nsid,vn->vnid);
	VNOP_POST;

	return err;
}

static int
machfs_rmdir(void *_ns, void *_dir, const char *_name)
{
	nspace		*ns;
	vnode		*dir;
	vnode       *vn;
	vnode_id    vnid;
	int			err=0;
	char        name[HFS_MAX_FLEN*4+1];

	ns = (nspace *) _ns;
	dir = (vnode *) _dir;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;
	
	convert_utf8_to_mac((uchar*)_name,(uchar*)name);
	unfilter_mac_name(name);

	if (strlen(name) > HFS_MAX_FLEN)
		return ENAMETOOLONG;

	VNOP_PRE;

	err = lookup(ns,dir,name,&vnid,&vn);

	if (err != 0) {
	  VNOP_POST;
	  return err;
	};
	
	if (vn->cdr.cdrType != cdrDirRec) {
		err = ENOTDIR;
		goto done;
	}

	err = do_unlink(ns, dir, vn);

done:
	put_vnode(ns->nsid,vn->vnid);
	VNOP_POST;
	return err;
}

static int
do_unlink(nspace *ns, vnode *dir, vnode *vn)
{
	hfsvol		*vol;
	CatKeyRec key;
	unsigned char pkey[HFS_CATKEYLEN];
	int found,err=0;

	vol = ns->libvol;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;

	if (dir->deleted || vn->deleted)
	  return ENOENT;

	if ((vn->cdr.cdrType == cdrDirRec) &&
		(vn->cdr.u.dir.dirVal != 0)) {
		return ENOTEMPTY;
	};

	vnid_hash_invalidate(ns,vn->vnid);

	if (vn->cdr.cdrType == cdrDirRec) {
		vol->mdb.drDirCnt--;
		if (dir->vnid == HFS_CNID_ROOTDIR)
			vol->mdb.drNmRtDirs--;
	} else {
		vol->mdb.drFilCnt--;
		if (dir->vnid == HFS_CNID_ROOTDIR)
			vol->mdb.drNmFls--;
	};
	vol->flags |= HFS_UPDATE_MDB;
	
	r_makecatkey(&key, dir->vnid, vn->name);
	r_packcatkey(&key, (unsigned char *)pkey, 0);

	if (bt_delete(&vol->cat, (unsigned char *)pkey) < 0) {
		printf("hfs: couldn't unlink (%Lx:'%s') (bt_delete)\n",
			dir->vnid,vn->name);
		return ENOENT;
	};
		
	r_makecatkey(&key, vn->vnid, "");
	r_packcatkey(&key, pkey, 0);
		
	if ((vn->cdr.cdrType != cdrFilRec) ||
		v_getfthread(vol, vn->cdr.u.fil.filFlNum, 0, 0)) {
		if (bt_delete(&vol->cat, pkey) < 0) {
			printf("hfs: couldn't unlink thread for \"%s\" in dir %Lx (bt_delete)\n",
				vn->name,dir->vnid);
			return ENOENT;
		};
	};

	dir->cdr.u.dir.dirVal--;
	dir->cdr.u.dir.dirMdDat = d_tomtime(time(0));
	dir->dirty = 1;
	err = write_vnode(ns,dir);
	if (err) {
	  printf("hfs: do_unlink (%Lx:'%s') couldn't adjust valence on disk\n",
			 dir->vnid,vn->name);
	  return err;
	};
	
	ns->catmod++;

	remove_vnode(ns->nsid,vn->vnid);
	vn->deleted = 1;
	
	notify_listener(B_ENTRY_REMOVED,ns->nsid,dir->vnid,0,vn->vnid,NULL);
	printf("notify4\n");

	return err;
}

static int
machfs_opendir(void *_ns, void *_node, void **cookie)
{
	nspace	*ns;
	vnode	*node;
	dirpos *dir;
	CatKeyRec key;
	unsigned char pkey[HFS_CATKEYLEN];
	int err=0;
	
	ns = (nspace *) _ns;
	node = (vnode *) _node;

	if (node->cdr.cdrType != cdrDirRec)
	  return ENOTDIR;
	
	if (node->deleted)
	  return EINVAL;

	VNOP_PRE;
	
	dir = (dirpos *)malloc(sizeof(dirpos));
	if (dir == NULL) {
		err = ENOMEM;
		goto opendir_done;
	}

	dir->dir = (hfsdir *)malloc(sizeof(hfsdir));	
	if (dir->dir == NULL) {
		free(dir);
		err = ENOMEM;
		goto opendir_done;
	}
	dir->dir->vol = ns->libvol;
   	dir->dir->dirid = node->cdr.u.dir.dirDirID;
	dir->dir->vptr  = 0;
	
	r_makecatkey(&key, dir->dir->dirid, "");
	r_packcatkey(&key, pkey, 0);
	
	if (bt_search(&ns->libvol->cat, pkey, &dir->dir->n) <= 0) {
		printf("hfs: couldn't open thread for dir %Lx (bt_search)\n",
			node->vnid);
		free(dir->dir);
		free(dir);
		err = ENOENT;
		goto opendir_done;
	}
	
	dir->fresh = 1;
	dir->count = ns->catmod;	
	dir->name[0] = 0;
	/*
	dir->dir->prev = 0;
	dir->dir->next = ns->libvol->dirs;	
	if (ns->libvol->dirs)
	  ns->libvol->dirs->prev = dir->dir;
	ns->libvol->dirs = dir->dir;
	*/
	
	/*
	printf("hfs: opendir for %Lx:'%s'\n",node->parid,node->name);
	*/
	
	*cookie = dir;

opendir_done:
	VNOP_POST;
	return err;
}

static int
machfs_closedir(void *_ns, void *_node, void *_cookie)
{
	return 0;
}

static int
machfs_free_dircookie(void *_ns, void *_node, void *_cookie)
{
	nspace		*ns;
	vnode		*node;
	dirpos		*cookie;

	VNOP_PRE;
	
	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (dirpos *) _cookie;
		
	if (cookie->dir)
		free(cookie->dir);
	if (cookie)
		free(cookie);

	VNOP_POST;
	return 0;
}

static int
machfs_rewinddir(void *_ns, void *_node, void *_cookie)
{
	nspace		*ns;
	vnode		*node;
	dirpos		*cookie;

	VNOP_PRE;

	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (dirpos *) _cookie;

	cookie->name[0] = 0;
	cookie->fresh = 1;
	cookie->count--;

	VNOP_POST;
	return 0;
}

static int
machfs_readdir(void *_ns, void *_node, void *_cookie,
		long *num, struct dirent *buf, size_t bufsize)
{
	nspace		*ns;
	vnode		*node;
	dirpos		*cookie;
	CatKeyRec key;
	CatDataRec cdr;
	unsigned char pkey[HFS_CATKEYLEN];
	hfsdirent entry;
	int err=0;
	int numEnts;
	
	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (dirpos *) _cookie;

	if (node->deleted)
	  return EINVAL;
	
	numEnts = bufsize/(38*4);
	if (numEnts == 0) { /* 38*4 == maximum size of a dirent for HFS (in UTF-8)*/
		printf("hfs: need dirent buffer of at least 38*4 bytes!\n",node->name);
		return EINVAL;
	};

	VNOP_PRE;
	
	/* Check to make sure the catalog tree couldn't have
	   changed since we saw it last */
	if (cookie->count != ns->catmod) {
	  /* Has changed, so we have to re-find the last entry */
	  r_makecatkey(&key, cookie->dir->dirid, cookie->name);
	  r_packcatkey(&key, pkey, 0);
	  if (bt_search(&ns->libvol->cat, pkey, &cookie->dir->n) <= 0) {
		/* We didn't find it, so it must have been deleted.
		   Oh well.  Recover by trying to find the next
		   non-deleted entry. */
		if (cookie->dir->n.rnum == 0) {
		  cookie->dir->n.nnum = cookie->dir->n.nd.ndBLink;
		  /* This is to tell the directory code that we
			 want the first record in the next node */
		  cookie->dir->n.rnum = HFS_MAXRECS+1;
		} else
		  cookie->dir->n.rnum--;
	  };
	};
	cookie->count = ns->catmod;
	
	if (hfs_readdir(cookie->dir,&entry) < 0) {
	  *num = 0;
	  err = 0;
	  goto readdir_done;
	};


	/* Sane? */
	if (entry.parid != node->vnid) {
		err = EINVAL;
		printf(	"hfs: read entry from dir %s(%Lx): %s(%Lx)\n"
				"...it thinks it's parent is %Lx!!!\n",node->name,node->vnid,
			entry.name,entry.cnid,entry.parid);
		goto readdir_done;
	};

	/* Anticipate that the calling code might walk this node
	   after listing the dir.  Should speed up ls -l. */
	vnid_hash_memoize(ns,entry.cnid,entry.parid,entry.name);

	strcpy(cookie->name,entry.name);

	buf->d_ino = entry.cnid;
	filter_mac_name(entry.name);
	convert_mac_to_utf8((uchar *)entry.name,(uchar*)buf->d_name);
	buf->d_reclen = strlen(buf->d_name);

	*num = 1;
readdir_done:
	VNOP_POST;
	return err;
}

static int
machfs_rstat(void *_ns, void *_node, struct stat *st)
{
	nspace		*ns;
	vnode		*node;

	ns = (nspace *) _ns;
	node = (vnode *) _node;

	VNOP_PRE;

	st->st_dev = ns->nsid;
	st->st_ino = node->vnid;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_blksize = 64 * 1024;
	switch (node->cdr.cdrType) {
		case cdrDirRec:
			st->st_mode = 0x1B6 | S_IFDIR;
			st->st_size = node->cdr.u.dir.dirVal;
			/* st->st_blksize = 0; */
			st->st_crtime = d_toutime(node->cdr.u.dir.dirCrDat);
			st->st_atime = st->st_mtime  = st->st_ctime =
			  d_toutime(node->cdr.u.dir.dirMdDat);
			break;
		case cdrFilRec:
			st->st_mode = 0x1B6 | S_IFREG;
			st->st_size = node->cdr.u.fil.filLgLen;
			/* st->st_blksize = node->cdr.u.fil.filClpSize; */
			st->st_crtime = d_toutime(node->cdr.u.fil.filCrDat);
			st->st_atime = st->st_mtime  = st->st_ctime =
			  d_toutime(node->cdr.u.fil.filMdDat);
			break;
	};
	
	VNOP_POST;
	return 0;
}

#define panic(a) { printf(a); printf("Spinning forever\n"); for (1;1;1); }

static int
machfs_wstat(void *_ns, void *_node, struct stat *st, long mask)
{
    int err = 0;
    nspace		*ns;
	vnode		*node;

	ns = (nspace *) _ns;
	node = (vnode *) _node;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;
	
	VNOP_PRE;

	switch (node->cdr.cdrType) {
		case cdrDirRec:
 		  if (mask & WSTAT_CRTIME)
			node->cdr.u.dir.dirCrDat = d_tomtime(st->st_crtime);
		  if (mask & WSTAT_MTIME)
			node->cdr.u.dir.dirMdDat = d_tomtime(st->st_mtime);
		  node->dirty = 1;
		  break;
		case cdrFilRec:
 		  if (mask & WSTAT_SIZE) {
			if (st->st_size != node->cdr.u.fil.filLgLen) {
				hfsfile file;
				node2file(node,&file,0);
				while (st->st_size > node->cdr.u.fil.filLgLen) {
					err = f_alloc(&file);
					if (err) return err;
					file2node(node,&file);
					node->cdr.u.fil.filLgLen = node->cdr.u.fil.filPyLen;
					node2file(node,&file,0);
				};
				if (node->cdr.u.fil.filLgLen != st->st_size) {
					file.cat.u.fil.filLgLen = node->cdr.u.fil.filLgLen = st->st_size;
					err = f_trunc(&file);
					if (err) return err;
				};
 		  	};
 		  };
 		  if (mask & WSTAT_CRTIME)
		    node->cdr.u.fil.filCrDat = d_tomtime(st->st_crtime);
		  if (mask & WSTAT_MTIME)
			node->cdr.u.fil.filMdDat = d_tomtime(st->st_mtime);
		  node->dirty = 1;
		  break;
	};
	
	err = write_vnode(ns,node);
	
	notify_listener(B_STAT_CHANGED,ns->nsid,0,0,node->vnid,NULL);
	printf("notify5\n");
	
	VNOP_POST;
	return err;
}

int node2file(vnode *node, hfsfile *file, int fork);
int file2node(hfsfile *file, vnode *node, int fork);

int node2file(vnode *node, hfsfile *file, int fork)
{
  file->vol = node->ns->libvol;
  file->cat = node->cdr;
  file->parid = node->parid;
  strcpy(file->name,node->name);
  
  file->clump = file->cat.u.fil.filClpSize;
  file->flags = 0;
  
  f_selectfork(file, fork);
  return 0;
}

int file2node(hfsfile *file, vnode *node, int fork)
{
  if (file->flags | HFS_UPDATE_CATREC) {
	node->cdr = file->cat;
	node->dirty = 1;
  };
  
  return 0;
}

static int
do_open(nspace *ns, vnode *node, void **cookie, int omode)
{
	hfsvol *vol;
	file_cookie *fc;
	hfsfile *file;
	int err=0;

	vol = ns->libvol;

	if (node->deleted)
	  return EINVAL;
	
	fc = (file_cookie*)malloc(sizeof(file_cookie));
	if (fc == NULL)
		return ENOMEM;

	file = (hfsfile*)fc;
	if (file == NULL) {
		printf("hfs: couldn't open file (%Lx:'%s') (oom)\n",
			node->parid,node->name);
		return ENOMEM;
	}
	
	fc->isdir = (node->cdr.cdrType != cdrFilRec);
	fc->lglen = node->cdr.u.fil.filLgLen;
	file->cat = node->cdr;
	file->parid = node->parid;
	strcpy(file->name,node->name);
    file->vol   = vol;	
    file->clump = file->cat.u.fil.filClpSize;
    file->flags = 0;    
	f_selectfork(file, 0);

	if (omode & O_TRUNC) {
		fc->lglen = file->cat.u.fil.filLgLen = 0;
		if (f_trunc(file) < 0) {
			free(fc);
			return EINVAL;
		};
		node->cdr = file->cat;
	};

	*cookie = file;
    return 0;
}

static int
machfs_open(void *_ns, void *_node, int omode, void **cookie)
{
	nspace		*ns;
	vnode		*node;
	int          err=0;

	ns = (nspace *) _ns;
	node = (vnode *) _node;

	VNOP_PRE;
	err = do_open(ns,node,cookie,omode);
	VNOP_POST;
    return err;
}

static int
machfs_close(void *_ns, void *_node, void *_cookie)
{
	nspace		*ns;
	node n;
	vnode		*node;
	hfsfile		*cookie;
	file_cookie *fc;
	hfsvol *vol;
	int result = 0;
	int r1=0,found;
	CatKeyRec key;
	unsigned char pkey[HFS_CATKEYLEN];
	size_t pydata,pyrsrc;
	
	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (hfsfile *) _cookie;
	fc = (file_cookie *) _cookie;
	vol = ns->libvol;

	if (ns->libvol->flags & HFS_READONLY)
	  return 0;

	if (fc->isdir)
	  return 0;
	
	VNOP_PRE;
	
	node2file(node,cookie,0);
	
	f_selectfork(cookie, 0);
	if (f_trunc(cookie) < 0)
	  r1 = EINVAL;
	
	file2node(cookie,node,0);
	
	node->cdr.u.fil.filStBlk = cookie->cat.u.fil.filExtRec[0].xdrStABN;
	node->cdr.u.fil.filRStBlk = cookie->cat.u.fil.filRExtRec[0].xdrStABN;
	node->cdr.u.fil.filClpSize = cookie->clump;
	node->dirty = 1;
	
	if (r1 < 0) {
	  printf("hfs: couldn't close file (%Lx:'%s') (f_trunc)\n",
			 node->parid,node->name);
	  result = EINVAL;
	} else if (!node->deleted) {
	  result = write_vnode(ns,node);
	  if (result != 0) {
		printf("hfs: close couldn't write vnode! (%Lx:'%s') (write_vnode)\n",
			   node->parid,node->name);
	  };
	};

	if (fc->lglen != node->cdr.u.fil.filLgLen) {
	  notify_listener(B_STAT_CHANGED,ns->nsid,0,0,node->vnid,NULL);
	printf("notify6\n");
	};
	
	VNOP_POST;
	return result;
}

static int
machfs_free_cookie(void *_ns, void *_node, void *_cookie)
{
	nspace		*ns;
	vnode		*node;
	hfsfile		*cookie;

	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (hfsfile *) _cookie;

	free(cookie);

	return 0;
}

static int
internal_read(nspace *_ns, hfsfile *file, vnode *node, unsigned char *ptr,
	long pos, long len, int fork)
{
	unsigned long *lglen, count;
	f_getptrs(file, &lglen, 0, 0);

	/*	
	printf("hfs: internal_read pos=%d, len=%d, lglen=%d\n",pos,len,*lglen);
	*/
	
	if ((pos > *lglen) || (pos < 0) || (len < 0))
	  return -1;

	if (pos + len > *lglen)
	  len = *lglen - pos;

	if (len < 0)
	  return -1;
  
	count = len;
	while (count) {
	  unsigned long bnum, offs, chunk;
	  
	  bnum  = pos / file->vol->blockSize;
	  offs  = pos % file->vol->blockSize;
	  
	  chunk = file->vol->blockSize - offs;
	  if (chunk > count)
		chunk = count;
	  
	  if (f_getblock(file, bnum, &file->b) < 0) {
		len = -1;
		goto read_done;
	  };
	  memcpy(ptr, (char *) &file->b + offs, chunk);
	  ptr += chunk;
	  
	  pos		+= chunk;
	  count	-= chunk;

	  /* Let's give someone else a chance... */

	  file2node(file,node,fork);
/*
	  VNOP_POST;
	  VNOP_PRE;
*/
	  node2file(node,file,fork);

	}
	
read_done:
	return len;
}

static int
internal_write(nspace *_ns, hfsfile *file, vnode *node, unsigned char *ptr,
	long pos, size_t *theLen, int fork)
{
  unsigned long *lglen, *pylen, count;
  int err=0;
  size_t len = *theLen;
    
  f_getptrs(file, &lglen, &pylen, 0);
  count = len;

  while (pos > *lglen) {
	err = f_alloc(file);
	if (err<0) {
		err = ENOSPC;
		goto write_done;
	} else
		err = 0;
	if (pos > *pylen)
		*lglen = *pylen;
	else
		*lglen = pos;
  };
	
  if (count) {
	file->cat.u.fil.filMdDat = d_tomtime(time(0));
	file->flags |= HFS_UPDATE_CATREC;
  }
  
  while (count) {
	unsigned long bnum, offs, chunk;
	
	bnum  = pos / _ns->libvol->blockSize;
	offs  = pos % _ns->libvol->blockSize;
	
	chunk = _ns->libvol->blockSize - offs;
	
	if (chunk > count)
	  chunk = count;		
	
	if (pos + chunk > *pylen) {
	  if (bt_space(&_ns->libvol->ext, 1) < 0) {
		err = ENOSPC;
		len = len - count;
		printf("hfs: internal_write failed to allocate space (bt_space)\n");
		goto write_done;
	  };
	  if (f_alloc(file) < 0) {
		err = ENOSPC;
		len = len - count;
		printf("hfs: internal_write failed to allocate space (f_alloc)\n");
		goto write_done;
	  };
	  if (pos + chunk > *pylen) {
		len = len - count;
		err = EIO;
		printf("hfs: internal_write - bt_space and f_alloc didn't work\n");
		goto write_done;
	  };
	}
	
	if ((offs > 0) || (chunk < _ns->libvol->blockSize)) {
	  if (f_getblock(file, bnum, &file->b) < 0) {
		err = EIO;
		len = len - count;
		goto write_done;
	  };
	}		
	
	memcpy((char *) &file->b + offs, ptr, chunk);
	ptr += chunk;
	if (f_putblock(file, bnum, &file->b) < 0) {
	  err = EIO;
	  len = len - count;
	  goto write_done;
	};
	
	pos		+= chunk;
	count	-= chunk;

	if ((pos > *lglen)&&(pos <= *pylen)) {
	  *lglen = pos;
	};
		
	/* Let's give someone else a chance. */
	file2node(file,node,fork);
/*
	VNOP_POST;
	VNOP_PRE;
*/
	node2file(node,file,fork);
  }
  
write_done:
  
  *theLen = len;

  if (err != 0)
	printf("hfs: internal_write returning err = %d\n",err);

  return err;
}

static int
machfs_read(void *_ns, void *_node, void *_cookie, off_t pos,
		void *buf, size_t *len)
{
	nspace		*ns;
	vnode		*node;
	hfsfile		*cookie;
	file_cookie *fc;
	int err=0;

	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (hfsfile *) _cookie;
	fc = (file_cookie *) _cookie;

	if (fc->isdir)
	  return EISDIR;
	
	VNOP_PRE;
	node2file(node,cookie,0);
	*len = internal_read(ns,cookie,node,buf,pos,*len,0);
	VNOP_POST;
	if (*len < 0) {
	  err = EINVAL;
	  *len = 0;
	};

	return err;
}

static int
machfs_write(void *_ns, void *_node, void *_cookie, off_t pos,
		const void *buf, size_t *len)
{
	nspace		*ns;
	vnode		*node;
	hfsfile		*cookie;
	file_cookie *fc;
	int err=0;

	ns = (nspace *) _ns;
	node = (vnode *) _node;
	cookie = (hfsfile *) _cookie;
	fc = (file_cookie *) _cookie;

	if (fc->isdir)
	  return EISDIR;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;

	if (node->cdr.u.fil.filFlags & 0x80) {
	  printf("hfs: denying write permission for (%Lx:%d)\n",
			 node->parid,node->name);
	  return EPERM;
	};

	if (ns == NULL) {
	  printf("hfs: write - ns ptr is NULL!\n");
	  return EINVAL;
	};

	if (len == NULL) {
	  printf("hfs: write - len ptr is NULL!\n");
	  return EINVAL;
	};
	
	if (cookie == NULL) {
	  printf("hfs: write - cookie ptr is NULL!\n");
	  return EINVAL;
	};

	if (node == NULL) {
	  printf("hfs: write - node ptr is NULL!\n");
	  return EINVAL;
	};

	VNOP_PRE;
	node2file(node,cookie,0);
	err = internal_write(ns,cookie,node,(void *)buf,pos,len,0);
	file2node(cookie,node,0);
	VNOP_POST;

	return err;
}

static int
machfs_ioctl(void *_ns, void *_node, void *_cookie, int cmd,
		void *buf, size_t len)
{  
	return EINVAL;
}

static int
machfs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **data, vnode_id *vnid)
{
    hfsvol *vol;

	if (hfs_lock.s == -1) {
	  hfs_lock.s = create_sem(0,"hfs_lock");
	  hfs_lock.c = 1;
	};

	LOCK(hfs_lock);

	dprintf("hfs: nsid %d device %s\n",nsid,device);

#ifdef HFS_IS_READONLY
	vol = hfs_mount((char *)device,1,O_RDONLY);
#else
	if(flags & B_MOUNT_READ_ONLY) 
		vol = hfs_mount((char *)device,1,O_RDONLY);
	else	
		vol = hfs_mount((char *)device,1,O_RDWR);	
#endif
	if (vol) {
	    nspace *ns = (nspace *)calloc(sizeof(nspace), 1);

#ifdef HFS_IS_READONLY
		vol->flags |= HFS_READONLY;
#endif
		if (vol->mdb.drAtrb & HFS_ATRB_UMOUNTED)
		  ns->unmounted = 1;
		else
		  ns->unmounted = 0;		
		strncpy(ns->devicePath,device,127);
		ns->libvol = vol;
		ns->nsid = nsid;
		ioctl(vol->fd, B_GET_GEOMETRY, &ns->geo);
		if (ns->geo.read_only)
			vol->flags |= HFS_READONLY;
		if(vol->flags & HFS_READONLY)
			printf("hfs: volume is read-only\n");
		else
			printf("hfs: volume is read/write\n");

		ns->theLock.s = create_sem(0,"HFS_vol_lock");
		ns->theLock.c = 1;
		ns->cacheLock.s = create_sem(0,"HFS_cache_lock");
		ns->cacheLock.c = 1;
		ns->masterSem = create_sem(HFS_MASTER_SEM_COUNT,"HFS_master_sem");
		vnid_hash_init(ns);
		*vnid = HFS_CNID_ROOTDIR;
		*data = ns;
		machfs_read_vnode(ns,*vnid,0,(void**)&ns->root_vnode);
		if (new_vnode(nsid,*vnid,(void *)ns->root_vnode) != 0) {
			free(ns);
			remove_cached_device_blocks(vol->fd, 0);
			UNLOCK(hfs_lock);
			return EINVAL;
		};
		UNLOCK(hfs_lock);
		return 0;
	};
	UNLOCK(hfs_lock);
	return EINVAL;
}

static int
machfs_unmount(void *_ns)
{
	nspace		*ns;
	hfsvol *vol;
	int result=0;

	ns = (nspace *) _ns;

	dprintf("hfs: unmounting %s\n",ns->devicePath);

	vol = ns->libvol;

	/*
	vol->flags |= HFS_UPDATE_MDB|HFS_UPDATE_ALTMDB|HFS_UPDATE_VBM;
	vol->cat.flags |= HFS_UPDATE_BTHDR;
	vol->ext.flags |= HFS_UPDATE_BTHDR;
	*/
	
	if (vol->files)
	  printf("hfs: unmount WARNING... files are open!\n");
	if (vol->dirs)
	  printf("hfs: unmount WARNING... dirs are open!\n");

	/*
	while (vol->files)
		hfs_close(vol->files);
		
	while (vol->dirs)
		hfs_closedir(vol->dirs);
	*/
	
	if (!(ns->libvol->flags & HFS_READONLY)) {
	  if (v_flush(vol, ns->unmounted) < 0)
		result = EINVAL;
	  flush_device(vol->fd,1);
	};
	remove_cached_device_blocks(vol->fd, 1);
	
	if ((close(vol->fd) < 0) || (result != 0)) {
		ERROR(errno, "error closing device");
		result = EINVAL;
	}
	
	if (vol->prev)
		vol->prev->next = vol->next;
	if (vol->next)
		vol->next->prev = vol->prev;
		
	if (vol == hfs_mounts)
		hfs_mounts = vol->next;
	if (vol == hfs_curvol)
		hfs_curvol = 0;
	
	v_destruct(vol);
	delete_sem(ns->theLock.s);
	delete_sem(ns->cacheLock.s);
	free(ns);

	return result;
}

static int
machfs_fsync(void *_ns, void *_node)
{
  /*
	nspace	*ns;
	vnode *node;
	
	ns = (nspace *) _ns;
	node = (vnode *)_node;
	
	if (ns->libvol->flags & HFS_READONLY)
	  return 0;
	
  */
  printf("hfs: fsync passing off to sync\n");
  return machfs_sync(_ns);
}

static int
machfs_sync(void *_ns)
{
	nspace	*ns;

	ns = (nspace *) _ns;

	if (ns->libvol->flags & HFS_READONLY)
	  return 0;
	
	VNOP_PRE;
	
	ns->libvol->flags |= HFS_UPDATE_MDB|HFS_UPDATE_ALTMDB|HFS_UPDATE_VBM;
	ns->libvol->cat.flags |= HFS_UPDATE_BTHDR;
	ns->libvol->ext.flags |= HFS_UPDATE_BTHDR;
	/*
	printf("BEFORE RE-MARK:\n");
	l_checkvbm(ns->libvol);
	v_remark_vbm(ns->libvol);
	printf("AFTER RE-MARK:\n");
	l_checkvbm(ns->libvol);
	*/
	printf("hfs: sync of %s updating MDB, VBM and b-tree headers\n",
		   ns->devicePath);

	if (ns->libvol->files)
	  printf("hfs: sync WARNING: files still open!\n");
	if (ns->libvol->dirs)
	  printf("hfs: sync WARNING: dirs still open!\n");

	v_flush(ns->libvol,ns->unmounted);
	flush_device(ns->libvol->fd,1);
	
	VNOP_POST;
	return 0;
}

static int
machfs_rfsstat(void *_ns, struct fs_info * fss)
{
	nspace	*ns;
	char s[256];
	ns = (nspace *) _ns;

	LOCK(hfs_lock);
	VNOP_PRE;

	fss->dev = ns->libvol->fd;
	fss->root = HFS_CNID_ROOTDIR;
	fss->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME;
	if (ns->libvol->flags & HFS_READONLY)
	  fss->flags |=  B_FS_IS_READONLY;
	if (ns->geo.removable)
	  fss->flags |= B_FS_IS_REMOVABLE;
	fss->block_size = ns->libvol->mdb.drAlBlkSiz;
	fss->io_size = ns->libvol->mdb.drAlBlkSiz;
	fss->total_blocks = ns->libvol->mdb.drNmAlBlks;
	fss->free_blocks = ns->libvol->mdb.drFreeBks;
	strncpy(fss->device_name,ns->devicePath, sizeof(fss->device_name));
	strcpy(s,ns->libvol->mdb.drVN);
	filter_mac_name(s);
	convert_mac_to_utf8((uchar*)s,(uchar*)fss->volume_name);
	strncpy(fss->fsh_name,"Mac HFS", sizeof(fss->fsh_name));

	VNOP_POST;
	UNLOCK(hfs_lock);

	return 0;
}

static int
machfs_wfsstat(void *_ns, struct fs_info *fss, long mask)
{
	nspace	*ns;
	char s[256];
	ns = (nspace *) _ns;

	if (ns->libvol->flags & HFS_READONLY)
	  return EPERM;

	if (!(mask & WFSSTAT_NAME))
	  return EINVAL;
	
	convert_utf8_to_mac((uchar*)fss->volume_name,(uchar*)s);
	if (strlen(s) > HFS_MAX_FLEN)
		return ENAMETOOLONG;
	
	LOCK(hfs_lock);
	VNOP_PRE;

	unfilter_mac_name(s);
	strcpy(ns->libvol->mdb.drVN,s);

	VNOP_POST;
	UNLOCK(hfs_lock);

	return 0;
}

static int
machfs_initialize(const char *devname, void *parms, size_t len)
{
  return hfs_format((char*)devname,0,"NewDisk");
}
  
int hfs_abort(void)
{
	printf("HFS: FATAL ERROR! Spinning forever...\n");
	for (;1;) {
	};
}

static char *
wts(int32 w, char *p)
{
  p[0] = w >> 24;
  p[1] = w >> 16;
  p[2] = w >> 8;
  p[3] = w;
  p[4] = 0;
  return p;
}

static int
read_rfork(nspace *ns, vnode *node, void *buf, int32 pos, int32 len)
{
  hfsfile file;
  int r=0;
  
  node2file(node,&file,1);
  
  r = internal_read(ns,&file,node,buf,pos,len,1);
  
rfork_done:
  return r;
}

static int
write_rfork(nspace *ns, vnode *node, void *buf, int32 pos, size_t theLen)
{
  hfsfile file;
  int err = 0;
  size_t len = theLen;
  
  node2file(node,&file,1);

  err = internal_write(ns,&file,node,buf,pos,&len,1);
  if ((err == 0)&&(len != theLen))
	err = EIO;
  if (err != 0)
	goto wfork_done;

  if (f_trunc(&file) < 0) {
	err = EIO;
	goto wfork_done;
  };

  file2node(&file,node,1);
  err = write_vnode(ns,node);
  
wfork_done:
  return err;
}

#include "translate.inc"

static void
convert_m2bColors(unsigned char *src, unsigned char *dst, int32 num)
{
  for (;num!=0;num--) {
	*dst++ = m2b_colors[*src++];
  };
}

static void
convert_b2mColors(unsigned char *src, unsigned char *dst, int32 num)
{
  for (;num!=0;num--) {
	*dst++ = b2m_colors[*src++];
  };
}

static void
convert_m4tom8(char *src, char *dst, int32 num)
{
  int i,p=0;
  for (i=0;i<num;i++) {
	dst[p] = src[i] >> 4;
	dst[p+1] = src[i] & 0x0F;
	p+=2;
  };
}

static void
convert_m1tom8(char *src, char *dst, int32 num)
{
  int i,p=0,j;
  for (i=0;i<num;i++) {
	for (j=0;j<8;j++)
	  dst[p++] = ((src[i] >> (7-j)) & 1)?255:0;
  };
}

static void
big2little(uchar *big, uchar *little)
{
  int i,j;
  for (i=16;i;i--) {
	for (j=16;j;j--) {
	  *little++ = *big++;
	  big++;
	};
	big += 32;
  };
}

static void
apply_mask(char *src, char *dst, int32 num)
{
  int i,p=0,j;
  for (i=0;i<num;i++) {
	for (j=0;j<8;j++) {
	  if (!((src[i] >> (7-j)) & 1)) {
		dst[p] = 255;
	  };
	  p++;
	};
  };
}

static rsrc_typelist *
get_ref_for_type(rsrc_map *map, int32 target)
{
  int i;
  rsrc_typelist *tl;
  
  tl = (rsrc_typelist*)map->typeList;
dprintf("numtypes-1 %d, tl @ 0x%x, map @ 0x%x\n", map->numTypesMinusOne,tl,map);
  for (i=0;i<=map->numTypesMinusOne;i++) {
	if (tl->rsrcType == target)
	  break;
	tl++;
  };
  if (tl->rsrcType == target)
	return tl;

  return NULL;
}

static rsrc_reflist *
get_ref_for_icon(rsrc_map *map, rsrc_typelist *tl, uint32 iconID)
{
  rsrc_reflist *rl;
  int i;
  
  rl = (rsrc_reflist*)(((char*)map)+map->offsToTypeList+tl->offsToRefList);
  for (i=0;i<=tl->numRsrcMinusOne;i++) {
	if (rl->rsrcID == -16455)
	  break;
	rl++;
  };
  
  if (i>tl->numRsrcMinusOne) {
	rl = (rsrc_reflist*)(((char*)map)+map->offsToTypeList+tl->offsToRefList);
	for (i=0;i<=tl->numRsrcMinusOne;i++) {
	  if (rl->rsrcID == iconID)
		break;
	  rl++;
	};
  };

  if (i<=tl->numRsrcMinusOne)
	return rl;

  return NULL;
}

static int
read_rsrc_from_ref(nspace *ns, vnode *n, rsrc_header *hdr,
				   rsrc_reflist *rl, void *buf, int32 *size)
{
  int32 offs = 0;
  int s=*size;
  int err=0;
  offs |= ((uint32)rl->hiOffsToData)<<16;
  offs |= rl->loOffsToData;

  offs += hdr->offsToData;

  if (rl->attributes & 1) /* if resource is compressed */ {
	return -1;
  };
  
  err = (read_rfork(ns,n,size,offs,sizeof(int32)) == sizeof(int32))?0:-1;
  if (*size > s)
	return -1;
  if (err==0) err = (read_rfork(ns,n,buf,offs+4,*size) == *size)?0:-1;
  return err;
}

uint32 icon8[2] = {'icl8','ics8'};
uint32 icon4[2] = {'icl4','ics4'};
uint32 iconM[2] = {'ICN#','ics#'};
int32  iconSize[2] = {1024,256};

static void
assert_icons(nspace *ns, vnode *node)
{
  int err=0;
  char fourbit[512];
  char mask[256];
  rsrc_header hdr;
  rsrc_map *map;
  rsrc_typelist *tl;
  rsrc_reflist *rl,*maskRef,*frefRef;
  int32 i,offs,r,j,maskSize;
  rsrc_typelist *big,*little,*bigMask,*littleMask,*fref,*bundle;
  int32 target,iconID=128;
  vnode *n = node;
  vnode_id iconFileVNID;
  int isdir=0;
  char s[10];
  int bundleErr=0;
  int whichIcon;
  rsrc_typelist *icons[2],*iconMasks[2];
  
  if (n->iconsChecked)
	return;

  n->iconsChecked = 1;
  /*
  printf("hfs: assert_icons for %Lx:'%s'\n",node->parid,node->name);
  */
  if (node->cdr.cdrType == cdrDirRec) {
	static char *iconthingy = "Icon\15";

	isdir = 1;
	err = lookup(ns,node,iconthingy,&iconFileVNID,&n);
	
	if ((err != 0) | node->deleted | n->deleted)
	  return;
  };
  
  r = read_rfork(ns,n,&hdr,0,sizeof(hdr));
  if (r != sizeof(hdr))
	goto icon_unlock;

  if (hdr.mapLen > 4096*1024)  /* if it's bogus looking, just bail out */
	  goto icon_unlock;

  map = (rsrc_map*)malloc(hdr.mapLen);
  if (map == NULL)
	  goto icon_unlock;
  
  r = read_rfork(ns,n,map,hdr.offsToMap,hdr.mapLen);
  if (r != hdr.mapLen) {
	free(map);
	goto icon_unlock;
  };
  
  /*
  printf("rsrc_hdr = {%d,%d,%d,%d}\n",
		 hdr.offsToData,
		 hdr.offsToMap,
		 hdr.dataLen,
		 hdr.mapLen);
  
  printf("rsrc_map = {%d,%d,%d,%d}\n",
		 map->attributes,
		 map->offsToTypeList,
		 map->offsToNameList,
		 map->numTypesMinusOne);
  
  printf("%d types (%d bytes), searching type list @ map+%d=%d ...\n",
		 map->numTypesMinusOne+1,
		 (map->numTypesMinusOne+1)*sizeof(rsrc_typelist),
		 map->offsToTypeList,
		 map->offsToTypeList+hdr.offsToMap);
  */

  /* Here's the deal: first, we look for FREFS.  If we have some FREFS,
	 we look for a bundle that tells us what icon to use.  If there is
	 one, we find the id of the icon to use.  If any of this falls through,
	 we just try 128... what the hell. */

  if (!isdir) {
	fref = get_ref_for_type(map,'FREF');
	if (fref) {
	  /* We found FREFs.  Look for the application sig. */	
	  struct { int32 type; int16 localID; uint8 dummy[64]; } fref_rec;
	  long size,bundleSize;
	  int localID = 0xFFFF0000;
	  
	  bundle = get_ref_for_type(map,'BNDL');
	  if (bundle) {
		frefRef = (rsrc_reflist*)(((char*)map)+
								  map->offsToTypeList+bundle->offsToRefList);
		bundleSize = 512;
		bundleErr = read_rsrc_from_ref(ns,n,&hdr,frefRef,fourbit,&bundleSize);
	  } else
		bundleErr = -1;

	  frefRef = (rsrc_reflist*)(((char*)map)+map->offsToTypeList+fref->offsToRefList);
	  for (i=0;i<=fref->numRsrcMinusOne;i++) {
		size = sizeof(fref_rec);
		if (read_rsrc_from_ref(ns,n,&hdr,frefRef,&fref_rec,&size) != 0)
		  goto out_of_mess;
		if ((fref_rec.type == 'APPL')||(fref_rec.type == *((uint32*)fourbit))) {
		  localID = fref_rec.localID;
		  break;
		};
		frefRef++;
	  };

	  if (localID != 0xFFFF0000) {
		/* We have a local ID to look for.  Now look for a bundle. */

		if (bundleErr==0) {
		  int index=0;
		  uint32 t;
	   
		  size = bundleSize;
		  t = *((uint32*)(fourbit+index));
		  index += 8;
		  while (index < size) {
			uint32 rs = *((uint32*)(fourbit+index));
			uint16 num;
			index += 4;
			num = *((uint16*)(fourbit+index));
			num++;
			index += 2;
			if (rs == 'ICN#') {
			  uint16 loc,id;
			  for (;num;num--) {
				loc = *((uint16*)(fourbit+index));
				index += 2;
				id = *((uint16*)(fourbit+index));
				index += 2;
				if (loc == localID) {
				  iconID = id;
				  goto out_of_mess;
				};
			  };
			} else {
			  index += 4 * num;
			};
		  };
		};
	  };
	};	
  };

out_of_mess:
  
  bigMask = get_ref_for_type(map,'ICN#');
  big = get_ref_for_type(map,'icl8');
  if (!big)
	big = get_ref_for_type(map,'icl4');
  if (!big)
	big = bigMask;
  
  littleMask = get_ref_for_type(map,'ics#');
  little = get_ref_for_type(map,'ics8');
  if (!little)
	little = get_ref_for_type(map,'ics4');
  if (!little)
	little = littleMask;  

  if (!big && !little) {
	free(map);
	goto icon_unlock;
  };

  icons[0] = big;
  icons[1] = little;
  iconMasks[0] = bigMask;
  iconMasks[1] = littleMask;
  
  for (whichIcon=0;whichIcon<2;whichIcon++) {
	if (icons[whichIcon]) {
	  rl = get_ref_for_icon(map,icons[whichIcon],iconID);
	  if (rl) {
		/* We've found an icon... */
		int32 size = iconSize[whichIcon];
		uchar *buf;
		int theErr=0;

		if (iconSize[whichIcon] > 1024*1024)   /* then it's probably bogus */
			goto cleanup;

		buf = malloc(iconSize[whichIcon]);
		if (buf == NULL)
			goto cleanup;

		if (iconMasks[whichIcon]) {
		  maskRef = get_ref_for_icon(map,iconMasks[whichIcon],iconID);
		  if (maskRef) {
			maskSize = 256;
			if (read_rsrc_from_ref(ns,n,&hdr,maskRef,mask,&maskSize) != 0)
			  maskRef = NULL;
		  };
		} else
		  maskRef = NULL;
		
		if (icons[whichIcon]->rsrcType == icon8[whichIcon]) {
		  /* 8-bit, so read it in-place */
		  size = iconSize[whichIcon];
		  theErr = read_rsrc_from_ref(ns,n,&hdr,rl,buf,&size);
		} else if (icons[whichIcon]->rsrcType == icon4[whichIcon]) {
		  size = iconSize[whichIcon];
		  theErr = read_rsrc_from_ref(ns,n,&hdr,rl,fourbit,&size);
		  if (theErr==0)
			convert_m4tom8(fourbit,(char*)buf,size);
		} else if (icons[whichIcon]->rsrcType == iconM[whichIcon]) {
		  if (!maskRef)
			theErr = -1;
		  else
			convert_m1tom8(mask,(char*)buf,maskSize/2);
		} else {
		  free(buf);
		  continue;
		};

		if (theErr!=0) {
		  free(buf);
		} else {
		  convert_m2bColors(buf,buf,size);
		  if (maskRef)
			apply_mask(mask+(maskSize/2),(char*)buf,maskSize/2);
		  if (whichIcon == 0)
			  node->bigIcon = buf;
		  else
			node->littleIcon = buf;
		};
	  };
	};
  };

  if (node->bigIcon && !node->littleIcon) {
	node->littleIcon = (uchar*)malloc(256);
	big2little(node->bigIcon,node->littleIcon);
  };
  
cleanup:
  free(map);

icon_unlock:
  if (node->cdr.cdrType == cdrDirRec) {
	put_vnode(ns->nsid,n->vnid);
	n = node;
  }
}

static int
machfs_open_attrdir(void *_ns, void *_node, void **_cookie)
{
  vnode *node = (vnode*)_node;
  attrdir_cookie *c = malloc(sizeof(attrdir_cookie));
  c->isdir = (node->cdr.cdrType == cdrDirRec);
  c->num = 0;
  *_cookie = c;

  if (node->deleted)
	return ENOENT;

  return 0;
}

static int
machfs_close_attrdir(void *_ns, void *_node, void *_cookie)
{
  return 0;
}

static int
machfs_free_attr_dircookie(void *_ns, void *_node, void *_cookie)
{
  free(_cookie);
  return 0;
}

static int
machfs_rewind_attrdir(void *_ns, void *_node, void *_cookie)
{
  attrdir_cookie *c = (attrdir_cookie*)_cookie;
  c->num = 0;
  return 0;
}

static char *attr_windframe = "_trk/windframe";
static char *attr_viewstate = "_trk/viewstate";
static char *attr_pose = "_trk/pinfo";
static char *attr_mimetype = "BEOS:TYPE";
static char *attr_smallicon = "BEOS:M:STD_ICON";
static char *attr_largeicon = "BEOS:L:STD_ICON";
static char *attr_rfork = "MACOS:RFORK";
static char *attr_hfsflags = "MACOS:HFS_FLAGS";
static char *attr_creator = "MACOS:CREATOR";

typedef struct _m2b_type {
  int32 mtype;
  char btype[64];
} m2b_type;

m2b_type m2btypes[] =
{
  {
	'APPL',
	"application/x-macbinary"
  },
  {
	'Gzip',
	"application/x-gzip"
  },
  {
	'ZIP ',
	"application/zip"
  },
  {
	'pZIP',
	"application/zip"
  },
  {
	'MooV',
	"video/quicktime"
  },
  {
	'MPEG',
	"video/mpeg"
  },
  {
	'AIFF',
	"audio/x-aiff"
  },
  {
	'AIFC',
	"audio/x-aifc"
  },
  {
	'Midi',
	"audio/x-midi"
  },
  {
	'RIFF',
	"audio/x-riff"
  },
  {
	'WAVE',
	"audio/x-wav"
  },
  {
	'TTRO',
	"text/plain"
  },
  {
	'TEXT',
	"text/plain"
  },
  {
	'HTML',
	"text/html"
  },
  {
	'RTF?',
	"text/rtf"
  },
  {
	'PDF ',
	"application/pdf"
  },
  {
	'PDF?',
	"application/pdf"
  },
  {
	'TIFF',
	"image/tiff"
  },
  {
	'JPEG',
	"image/jpeg"
  },
  {
	'GIF?',
	"image/gif"
  },
  {
	'TPIC',
	"image/x-targa"
  },
  {
	'TARF',
	"application/x-tar"
  },
  {
	0,
	NULL
  }
};

static char *dir_mimetype = "application/x-vnd.Be-directory";
static char *cop_out = "application/octet-stream";
static char *mac_type = "application/x-MacOS-";

static int
bft2mft(char *bft, int32 *mft)
{
  m2b_type *m2b = m2btypes;

  if (strncmp(bft,mac_type,strlen(mac_type)) == 0) {
	*mft = *((int32*)(bft+strlen(mac_type)));
	return 0;
  };

  while (m2b->mtype && (strcmp(m2b->btype,bft)!=0)) m2b++;
  if (m2b->mtype == 0)
	*mft = '????';
  else
	*mft = m2b->mtype;

  return 0;  
}

static int
mft2bft(int32 mft, char *bft)
{
  char s[5];
  
  m2b_type *m2b = m2btypes;
  while (m2b->mtype && (m2b->mtype != mft)) m2b++;
  if (m2b->mtype == 0) {
	strcpy(bft,mac_type);
	*((int32*)s) = mft;
	s[4] = 0;
	strcat(bft,s);
  } else {
	strcpy(bft,m2b->btype);
  };
  return 0;
}

static int
machfs_read_attrdir(void *_ns, void *_node, void *_cookie,
					int32 *num, struct dirent *buf,
					size_t bufsize)
{
  nspace *ns = (nspace*)_ns;
  vnode *node = (vnode*)_node;
  attrdir_cookie *c = (attrdir_cookie*)_cookie;
  char *p = NULL;
  int err=0;

  VNOP_PRE;

  if (c->isdir) {
	switch (c->num) {
	case 0:
	  if (node->vnid == HFS_CNID_ROOTDIR) {
		c->num++;
		goto skip_0;
	  };
	  p = attr_pose;
	  break;
	case 1:
skip_0:
	  p = attr_mimetype;
	  break;
   	case 2:
	  if ((node->cdr.u.dir.dirUsrInfo.frRect.top >=
		   node->cdr.u.dir.dirUsrInfo.frRect.bottom) ||
		  (node->cdr.u.dir.dirUsrInfo.frRect.left >=
		   node->cdr.u.dir.dirUsrInfo.frRect.right)) {
		c->num++;
		goto skip_to_3;
	  };
	  p = attr_windframe;
	  break;
	case 3:
skip_to_3:
	  p = attr_viewstate;
	  break;
	case 4:
	  assert_icons(ns,node);
	  if (node->bigIcon)
		p = attr_largeicon;
	  else {
		c->num++;
		goto dcase5;
	  };
	  break;
	case 5:
	dcase5:
	  assert_icons(ns,node);
	  if (node->littleIcon)
		p = attr_smallicon;
	  else {
		c->num++;
		goto dcase6;
	  };
	  break;
	case 6:
	dcase6:
	  p = attr_hfsflags;
	  break;
	};
	} else {
		switch (c->num) {
			case 0:
				p = attr_pose;
				break;
			case 1:
				if (node->cdr.u.fil.filUsrWds.fdType == '????') {
					c->num++;
					goto fcase2;
				};
				p = attr_mimetype;
				break;
			case 2:
			fcase2:
				assert_icons(ns,node);
				if (node->bigIcon)
					p = attr_largeicon;
				else {
					c->num++;
					goto fcase3;
				};
				break;
			case 3:
			fcase3:
				assert_icons(ns,node);
				if (node->littleIcon)
					p = attr_smallicon;
				else {
					c->num++;
					goto fcase4;
				};
				break;
			case 4:
			fcase4:
				p = attr_rfork;
				break;
			case 5:
				p = attr_creator;
				break;
			case 6:
				p = attr_hfsflags;
				break;
		};
	};

	if (p!=NULL) {
		*num = 1;
		strcpy(buf->d_name,p);
		buf->d_ino = c->num++;
		VNOP_POST;
		return 0;
	} else {
		*num = 0;
		VNOP_POST;
		return 0;
	};
}

typedef struct _brect_wannabe {
  float left,top,right,bottom;
} brect_wannabe;

typedef struct _bpoint_wannabe {
  float x,y;
} bpoint_wannabe;

typedef struct _pinfo {
  bool           invisible;
  ino_t          inited_dir;
  bpoint_wannabe location;
} pinfo;

typedef struct _viewstate {
  uint32         key;
  uint32         version;
  uint32         viewMode;
  uint32         lastIconMode;
  bpoint_wannabe listOrigin;
  bpoint_wannabe iconOrigin;
  uint32         priSortAttr;
  uint32         priSortType;
  uint32         secSortAttr;
  uint32         secSortType;
  bool           reverseSort;
} viewstate;

static int
machfs_read_attr(void *_ns, void *_node, const char *name,
				 int type, void *buf, size_t *len,
				 off_t pos)
{
  char aBuffer[1024];
  brect_wannabe br;
  viewstate vs;
  pinfo pi;
  nspace *ns = (nspace*)_ns;
  vnode *node = (vnode*)_node;
  int isdir = (node->cdr.cdrType == cdrDirRec);
  long size=0,err=0;
  void *readbuf=NULL;
  
  VNOP_PRE;
  
  if (isdir) {
	if (strcmp(name,attr_pose) == 0) {
	  if (node->vnid != HFS_CNID_ROOTDIR) {
		readbuf = &pi;
		size = sizeof(pinfo);
		pi.invisible =
		  ( ((node->parid == HFS_CNID_ROOTDIR) &&
			 (strcmp(node->name,"Trash")==0))           ||
			((node->parid == HFS_CNID_ROOTDIR) &&
			 (strcmp(node->name,"Desktop Folder")==0))  ||
			((node->cdr.u.dir.dirUsrInfo.frFlags & 0x4000) != 0));
		pi.location.y = node->cdr.u.dir.dirUsrInfo.frLocation.v;
		pi.location.x = node->cdr.u.dir.dirUsrInfo.frLocation.h;

		/*
		  printf("(%Lx:'%s') location: %f,%f\n",
		  node->parid,node->name,
		  pi.location.x,pi.location.y);
		*/	 
		
		pi.inited_dir =
		  ((node->cdr.u.dir.dirUsrInfo.frFlags & 0x0100) != 0)?node->parid:-1;
	  };
    } else if (strcmp(name,attr_mimetype) == 0) {
	  readbuf = dir_mimetype;
	  size = strlen(dir_mimetype)+1;
	} else if (strcmp(name,attr_windframe) == 0) {
	  if ((node->cdr.u.dir.dirUsrInfo.frRect.top <
		   node->cdr.u.dir.dirUsrInfo.frRect.bottom) &&
		  (node->cdr.u.dir.dirUsrInfo.frRect.left <
		   node->cdr.u.dir.dirUsrInfo.frRect.right)) {
		readbuf = &br;
		size = sizeof(brect_wannabe);
		
		br.top = node->cdr.u.dir.dirUsrInfo.frRect.top;
		br.left = node->cdr.u.dir.dirUsrInfo.frRect.left;
		br.right = node->cdr.u.dir.dirUsrInfo.frRect.right-1;
		br.bottom = node->cdr.u.dir.dirUsrInfo.frRect.bottom-1;
		/*
		printf("(%Lx:'%s'):windframe = {%d,%d,%d,%d}\n",
			   node->parid,node->name,
			   (int)br.left,
			   (int)br.top,
			   (int)br.right,
			   (int)br.bottom);
			   */
	  };
	} else if (strcmp(name,attr_largeicon) == 0) {
	  assert_icons(ns,node);
	  size = 1024;
	  if (node->bigIcon)
		readbuf = node->bigIcon;
	} else if (strcmp(name,attr_smallicon) == 0) {
	  assert_icons(ns,node);
	  size = 256;
	  if (node->littleIcon)
		readbuf = node->littleIcon;
	} else if (strcmp(name,attr_viewstate) == 0) {
	  vs.key = 0x6f5ef552;
	  vs.version = 10;
	  if (node->cdr.u.dir.dirUsrInfo.frView & 0x0200) {
		vs.viewMode = 'Tlst';
	  } else {
		if (node->cdr.u.dir.dirUsrInfo.frView & 0x0040)
		  vs.viewMode = 'Tmic';
		else
		  vs.viewMode = 'Ticn';
	  };
	  vs.iconOrigin.x = node->cdr.u.dir.dirFndrInfo.frScroll.h;
	  vs.iconOrigin.y = node->cdr.u.dir.dirFndrInfo.frScroll.v;
	  vs.listOrigin.x = 0;
	  if (vs.listOrigin.y < 0)
		vs.listOrigin.y = 0;
	  vs.lastIconMode =
		(node->cdr.u.dir.dirUsrInfo.frView & 0x0040)?'Tmic':'Ticn';
	  vs.priSortAttr=0;
	  vs.priSortType=0;
	  vs.secSortAttr=0;
	  vs.secSortType=0;
	  vs.reverseSort=false;
	  readbuf = &vs;
	  size = sizeof(viewstate);
	} else if (strcmp(name,attr_hfsflags) == 0) {
	  readbuf = &node->cdr.u.dir.dirUsrInfo.frFlags;
	  size = 2;
	} else {
	  err = ENOENT;
	  goto read_attr_done;
	};
	} else {
		if (strcmp(name,attr_pose) == 0) {
			readbuf = &pi;
			size = sizeof(pinfo);
			pi.invisible = ((node->cdr.u.fil.filUsrWds.fdFlags & 0x4000) != 0);
			pi.location.y = node->cdr.u.fil.filUsrWds.fdLocation.v;
			pi.location.x = node->cdr.u.fil.filUsrWds.fdLocation.h;
			pi.inited_dir =
				((node->cdr.u.fil.filUsrWds.fdFlags & 0x0100) != 0)?node->parid:0;
		} else if (strcmp(name,attr_mimetype) == 0) {
			int32 mft;
			char bft[128];
			
			mft = node->cdr.u.fil.filUsrWds.fdType;
			if (mft == '????') {
				err = ENOENT;
				goto read_attr_done;
			};
			mft2bft(mft,bft);
			readbuf = bft;
			size = strlen(bft)+1;
		} else if (strcmp(name,attr_largeicon) == 0) {
			assert_icons(ns,node);
			size = 1024;
			if (node->bigIcon)
				readbuf = node->bigIcon;
		} else if (strcmp(name,attr_smallicon) == 0) {
			assert_icons(ns,node);
			size = 256;
			if (node->littleIcon)
				readbuf = node->littleIcon;
		} else if (strcmp(name,attr_rfork) == 0) {
			if (node->cdr.u.fil.filRLgLen == 0) {
				err = ENOENT;
				goto read_attr_done;
			};
			err = read_rfork(ns,node,buf,pos,*len);
			if (err == *len) {
				err = 0;
			} else if ((err >= 0) && (err < *len)) {
				*len = err;
				err = 0;
			} else if (err < 0) {
				*len = 0;
				err = EINVAL;
			};
			goto read_attr_done;
		} else if (strcmp(name,attr_creator) == 0) {
			readbuf = &node->cdr.u.fil.filUsrWds.fdCreator;
			size = 4;
		} else if (strcmp(name,attr_hfsflags) == 0) {
			readbuf = &node->cdr.u.fil.filUsrWds.fdFlags;
			size = 2;
		} else {
			err = ENOENT;
			goto read_attr_done;
		};
	};
	
  if (readbuf == NULL) {
	err = ENOENT;
	goto read_attr_done;
  };
  
  size -= pos;
  if (size <= 0)
	*len = 0;
  else
	*len = size;
  memcpy(((char*)buf)+pos,((char*)readbuf)+pos,*len);
  
read_attr_done:
  VNOP_POST;
  return err;
}

static int
machfs_write_attr(void *_ns, void *_node, const char *name,
				  int type, const void *buf, size_t *len,
				  off_t pos)
{
  char aBuffer[1024];
  brect_wannabe *br;
  viewstate vs;
  pinfo *pi;
  const char *cbuf=(const char *)buf;
  nspace *ns = (nspace*)_ns;
  vnode *node = (vnode*)_node;
  int isdir = (node->cdr.cdrType == cdrDirRec);
  long size=0,err=0;
  void *readbuf=NULL;
  
  if (ns->libvol->flags & HFS_READONLY)
	return EPERM;
  
  VNOP_PRE;

  if (((node->parid == HFS_CNID_ROOTDIR) &&
	   (strcmp(node->name,"Trash")==0))           ||
	  ((node->parid == HFS_CNID_ROOTDIR) &&
	   (strcmp(node->name,"Desktop Folder")==0))) {
	/* We don't want to change anything about these "special"
	   folders */
	VNOP_POST;
	return 0;
  };
	
  if (isdir) {
	if (strcmp(name,attr_pose) == 0) {
		if ((pos == 0) && (*len == 20) && (node->vnid != HFS_CNID_ROOTDIR)) {
			pi = (pinfo*)buf;
			if (pi->invisible)
				node->cdr.u.dir.dirUsrInfo.frFlags |= 0x4000;
			node->cdr.u.dir.dirUsrInfo.frLocation.v = pi->location.y;
			node->cdr.u.dir.dirUsrInfo.frLocation.h = pi->location.x;
			node->dirty = 1;
		};
	} else if (strcmp(name,attr_mimetype) == 0) {
	} else if (strcmp(name,attr_windframe) == 0) {
	  br = (brect_wannabe*)buf; 
	  if ((pos == 0) && (*len == sizeof(brect_wannabe))) {
		node->cdr.u.dir.dirUsrInfo.frRect.top = br->top;
		node->cdr.u.dir.dirUsrInfo.frRect.left = br->left;
		node->cdr.u.dir.dirUsrInfo.frRect.right = br->right+1;
		node->cdr.u.dir.dirUsrInfo.frRect.bottom = br->bottom+1;
	  };
	  node->dirty = 1;
	} else if (strcmp(name,attr_largeicon) == 0) {
	} else if (strcmp(name,attr_smallicon) == 0) {
	} else if (strcmp(name,attr_viewstate) == 0) {
	  uint32 vm=0;
	  if ((pos <= 8) && ((pos+*len) >= 12)) {
		vm = *((uint32*)(cbuf-pos+8));
		if (vm == 'Tlst') {
		  node->cdr.u.dir.dirUsrInfo.frView &= ~0x0100;
		  node->cdr.u.dir.dirUsrInfo.frView |=  0x0200;
		} else if (vm == 'Tmic')
		  node->cdr.u.dir.dirUsrInfo.frView = 0x0140;
		else if (vm == 'Ticn')
		  node->cdr.u.dir.dirUsrInfo.frView = 0x0100;
	  };
	  if ((pos <= 24) && ((pos+*len) >= 32)) {	  
		bpoint_wannabe *p = (bpoint_wannabe*)(cbuf-pos+24);
		/*
		  printf("(%Lx:'%s') icon scroll: %f,%f\n",
		  node->parid,node->name,
		  p->x,p->y);
		  */
		node->cdr.u.dir.dirFndrInfo.frScroll.h = p->x;
		node->cdr.u.dir.dirFndrInfo.frScroll.v = p->y;
	  };
	  node->dirty = 1;
	} else if (strcmp(name,attr_hfsflags) == 0) { 
	  if ((pos == 0)&&(*len==2)) {
		node->cdr.u.dir.dirUsrInfo.frFlags =
		  (node->cdr.u.dir.dirUsrInfo.frFlags &  0x0100) |
		  (*((int16*)buf)                     & ~0x0100) ;
	  };
	};
  } else {
	if (strcmp(name,attr_pose) == 0) {
	  if ((pos == 0) && (*len == 20)) {
		pi = (pinfo*)buf;
		if (pi->invisible)
		  node->cdr.u.fil.filUsrWds.fdFlags |= 0x4000;
		node->cdr.u.fil.filUsrWds.fdLocation.v = pi->location.y;
		node->cdr.u.fil.filUsrWds.fdLocation.h = pi->location.x;
	  };
	  node->dirty = 1;
	} else if (strcmp(name,attr_mimetype) == 0) {
	  char *s = (char*)buf;
	  int32 mft;
	  if (strlen(s) > 256)
		err = EINVAL;
	  else {
		bft2mft(s,&mft);
		node->cdr.u.fil.filUsrWds.fdType = mft;
	  };
	} else if (strcmp(name,attr_largeicon) == 0) {
	} else if (strcmp(name,attr_smallicon) == 0) {
	} else if (strcmp(name,attr_rfork) == 0) {
	  vnode *theNode=node;
	  if (err = write_rfork(ns,node,(void*)buf,pos,*len))
		goto write_attr_done;
	  
	  if (strcmp(node->name,"Icon\15")==0) {
		err=get_vnode(ns->nsid,node->parid,(void**)&theNode);
		if (err != 0)
		  goto write_attr_done;
	  };

	  if (!theNode->deleted) {
		theNode->iconsChecked = 0;
		if (theNode->bigIcon)
		  free(theNode->bigIcon);
		if (theNode->littleIcon)
		  free(theNode->bigIcon);
		theNode->bigIcon = NULL;
		theNode->littleIcon = NULL;
		
		notify_listener(B_ATTR_CHANGED,ns->nsid,0,0,node->vnid,attr_rfork);
		notify_listener(B_ATTR_CHANGED,ns->nsid,0,0,theNode->vnid,attr_smallicon);
		notify_listener(B_ATTR_CHANGED,ns->nsid,0,0,theNode->vnid,attr_largeicon);
	printf("notify7\n");

		if (err == *len)
		  err = 0;
		if ((err >= 0) && (err < *len)) {
		  *len = err;
		  err = 0;
		};
		if (err < 0) {
		  *len = 0;
		  err = EINVAL;
		};
	  };
	  
	  if (node != theNode)
		put_vnode(ns->nsid,theNode->vnid);

	} else if (strcmp(name,attr_creator) == 0) {
	  if ((pos == 0)&&(*len == 4))
		node->cdr.u.fil.filUsrWds.fdCreator = *((int32*)buf);
	} else if (strcmp(name,attr_hfsflags) == 0) { 
	  if ((pos == 0)&&(*len==2)) {
		node->cdr.u.fil.filUsrWds.fdFlags =
		  (node->cdr.u.fil.filUsrWds.fdFlags &  0x0100) |
		  (*((int16*)buf)                    & ~0x0100) ;		  
	  };
	};
  };
  
write_attr_done:
  if (err == 0)
	err = write_vnode(ns,node);
  
  VNOP_POST;
  return err;
}

static int
machfs_stat_attr(void *_ns, void *_node, const char *name,
				 struct attr_info *buf)
{
  nspace *ns = (nspace*)_ns;
  vnode *node = (vnode*)_node;
  int isdir = (node->cdr.cdrType == cdrDirRec);
  int err=0;
  
  VNOP_PRE;

  if (isdir) {
	if (strcmp(name,attr_pose) == 0) {
	  buf->type = B_RAW_TYPE;
	  buf->size = 20;
	} else if (strcmp(name,attr_mimetype) == 0) {
	  buf->type = 'MIMS';
	  buf->size = strlen(dir_mimetype)+1;
	} else if (strcmp(name,attr_windframe) == 0) {
	  if ((node->cdr.u.dir.dirUsrInfo.frRect.top >=
		   node->cdr.u.dir.dirUsrInfo.frRect.bottom) ||
		  (node->cdr.u.dir.dirUsrInfo.frRect.left >=
		   node->cdr.u.dir.dirUsrInfo.frRect.right)) {
		err = ENOENT;
		goto stat_attr_done;
	  };
	  buf->type = B_RECT_TYPE;
	  buf->size = 16;
	} else if (strcmp(name,attr_largeicon) == 0) {
	  buf->type = B_COLOR_8_BIT_TYPE;
	  buf->size = 1024;
	} else if (strcmp(name,attr_smallicon) == 0) {	  
	  buf->type = B_COLOR_8_BIT_TYPE;
	  buf->size = 256;
	} else if (strcmp(name,attr_viewstate) == 0) {
	  buf->type = B_RAW_TYPE;
	  buf->size = sizeof(viewstate);
	} else if (strcmp(name,attr_hfsflags) == 0) { 
	  buf->type = B_UINT16_TYPE;
	  buf->size = 2;
	} else {
	  err = ENOENT;
	  goto stat_attr_done;
	};
  } else {
	if (strcmp(name,attr_pose) == 0) {
	  buf->type = B_RAW_TYPE;
	  buf->size = 20;
	} else if (strcmp(name,attr_mimetype) == 0) {
	  int32 mft;
	  char bft[128];

	  buf->type = 'MIMS';
	  mft = node->cdr.u.fil.filUsrWds.fdType;
	  mft2bft(mft,bft);
	  buf->size = strlen(bft)+1;
	} else if (strcmp(name,attr_largeicon) == 0) {
	  assert_icons(ns,node);
	  buf->type = B_COLOR_8_BIT_TYPE;
	  buf->size = 1024;
	  if (node->bigIcon == NULL) {
		err = ENOENT;
		goto stat_attr_done;
	  };
	} else if (strcmp(name,attr_smallicon) == 0) {	  
	  assert_icons(ns,node);
	  buf->type = B_COLOR_8_BIT_TYPE;
	  buf->size = 256;
	  if (node->littleIcon == NULL) {
		err = ENOENT;
		goto stat_attr_done;
	  };
	} else if (strcmp(name,attr_rfork) == 0) {	  
	  buf->type = B_RAW_TYPE;
	  buf->size = node->cdr.u.fil.filRLgLen;
	  if (buf->size == 0) {
		err = ENOENT;
		goto stat_attr_done;
	  };
	} else if (strcmp(name,attr_creator) == 0) { 
	  buf->type = B_UINT32_TYPE;
	  buf->size = 4;
	} else if (strcmp(name,attr_hfsflags) == 0) { 
	  buf->type = B_UINT16_TYPE;
	  buf->size = 2;
	} else {
	  err = ENOENT;
	  goto stat_attr_done;
	};
  };

stat_attr_done:
  VNOP_POST;
  return err;
}

static int
machfs_symlink(void *_ns, void *_dir, const char *name, const char *path)
{
	/* Pavel should be special-casing this to give a useful error message */
	return B_UNSUPPORTED;
}

