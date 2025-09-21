
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

//#define printf dprintf
#define printf

extern "C" {
	#include <ide_calls.h>
	#include <fsproto.h>
	#include <fs_attr.h>
	#include <TypeConstants.h>
	#include <lock.h>
//	#include <cache.h>
}

enum {
	USPACE_FS_NOTIFY_CALLBACK = B_DEVICE_OP_CODES_END+1
};

typedef struct 	vnode vnode;
typedef struct 	nspace nspace;
typedef struct 	cacherec cacherec;
typedef struct 	driver_desc driver_desc;
typedef struct	dirpos dirpos;
typedef struct 	dir_info dir_info;
typedef struct 	dev_info dev_info;
typedef struct 	partition_desc partition_desc;

#define panic(a) { printf(a); printf("Spinning forever\n"); for (1;1;1); }

struct Cookie {
	int32			cookieLen;
	void *			cookie;
	int32			readfrom,writeto;
};

struct vnode {
	nspace			*ns;
	vnode_id		vnid;
	int32			idLen;
	void *			id;
};

struct port_pool
{
	port_id			port;
	port_pool *		next;
};

struct mountparms {
	port_id			outPort;
};

struct mylock {
	sem_id			s;
	int32			c;
};

struct nspace {
	nspace_id		nsid;
	team_id			serverTeam;
    char *			device;
    char *			name;
    char *			fsname;
	int32			flags;
	int64			rootID;
	vnode *			root;
	port_id			outPort;
//	port_id			notifyPort;
//	thread_id		notifyThread;
	lock			inPortLock;
	port_pool *		inPort;
};

/*
struct notify_data {
	nspace_id nsid;
	port_id notifyPort;
};
*/

port_pool * obtain_port(nspace *ns)
{
	port_pool *pp;
	LOCK(ns->inPortLock);
	if (!ns->inPort) {
		port_id thePort = create_port(1,"uspacefs inPort");
		if (set_port_owner(thePort,ns->serverTeam) != B_OK) {
			delete_port(thePort);
			UNLOCK(ns->inPortLock);
			return NULL;
		};
		ns->inPort = (port_pool*)malloc(sizeof(port_pool));
		ns->inPort->port = thePort;
		ns->inPort->next = NULL;
	};
	pp = ns->inPort;
	ns->inPort = pp->next;
	UNLOCK(ns->inPortLock);
	return pp;
};

void release_port(nspace *ns, port_pool *pp)
{
	LOCK(ns->inPortLock);
	pp->next = ns->inPort;
	ns->inPort = pp;
	UNLOCK(ns->inPortLock);
};

void delete_ports(nspace *ns)
{
	LOCK(ns->inPortLock);
	while (ns->inPort) {
		port_pool *p = ns->inPort;
		delete_port(p->port);
		ns->inPort = ns->inPort->next;
		free(p);
	};
	UNLOCK(ns->inPortLock);
	delete_sem(ns->inPortLock.s);
	delete_port(ns->outPort);
//	delete_port(ns->notifyPort);
};

#define IO_SIZE 4096
#define SEND_TIMEOUT B_INFINITE_TIMEOUT // (1000000 * 60)
#define RECV_TIMEOUT B_INFINITE_TIMEOUT // (1000000 * 60)

enum USpaceFSCommand {
	USPACEFS_READ_VNODE = 1,
	USPACEFS_WRITE_VNODE,
	USPACEFS_REMOVE_VNODE,
	USPACEFS_WALK,
	USPACEFS_MOUNT,
	USPACEFS_UNMOUNT,
	USPACEFS_RSTAT,
	USPACEFS_OPENDIR,
	USPACEFS_READDIR,
	USPACEFS_REWINDDIR,
	USPACEFS_CLOSEDIR,
	USPACEFS_FREEDIRCOOKIE,
	USPACEFS_OPENATTRDIR,
	USPACEFS_READATTRDIR,
	USPACEFS_REWINDATTRDIR,
	USPACEFS_CLOSEATTRDIR,
	USPACEFS_FREEATTRDIRCOOKIE,
	USPACEFS_READATTR,
	USPACEFS_WRITEATTR,
	USPACEFS_STATATTR,
	USPACEFS_OPEN,
	USPACEFS_READ,
	USPACEFS_WRITE,
	USPACEFS_CLOSE,
	USPACEFS_FREECOOKIE,
	USPACEFS_OPENREADPIPE,
	USPACEFS_OPENWRITEPIPE,
	USPACEFS_CREATE,
	USPACEFS_READLINK,
	USPACEFS_IOCTL
};

static int32 USPACE_TIMEOUT = 60000000; // ten seconds


class uspace_io {
	public:
		
		nspace *ns;
		uint8 buffer[IO_SIZE];
		int32 pos,len,error;
		
		uspace_io(nspace *_ns) {
			ns = _ns;
			len = 0;
			pos = 4;
		};

		void write(int32 intData) {
			*((int32*)(buffer+pos)) = intData;
			pos += 4;
		};
		void write(USpaceFSCommand cmd) {
			write((int32)cmd);
		};
		void write(int64 intData) {
			*((int64*)(buffer+pos)) = intData;
			pos += 8;
		};
		void write(void *voidData, int32 dataLen) {
			memcpy(buffer+pos,voidData,dataLen);
			pos += dataLen;
		};
		void write(const char *stringData) {
			if (!stringData) { write((int32)0); return; };
			int32 slen = strlen(stringData);
			write(slen);
			memcpy(buffer+pos,stringData,slen);
			pos += slen;
		};
		
		void reset() { pos = 4; };


		int32 transact() {
			int32 lerror;
			ssize_t size;
			*((uint32*)buffer) = pos;
			port_pool *inPort = obtain_port(ns);
			if (!inPort) return EIO;
			if ((lerror = write_port_etc(
						ns->outPort,
						inPort->port,
						buffer,pos,
						B_TIMEOUT | B_CAN_INTERRUPT,
						USPACE_TIMEOUT)) != B_OK) return error = lerror;
			if ((size = port_buffer_size_etc(
						inPort->port,
						B_TIMEOUT | B_CAN_INTERRUPT,
						USPACE_TIMEOUT)) < 0) return error = size;
			if (size > IO_SIZE) return error = B_ERROR;
			if ((lerror = read_port_etc(
						inPort->port,
						&error,buffer,
						IO_SIZE,
						B_TIMEOUT | B_CAN_INTERRUPT,
						USPACE_TIMEOUT)) < 0) return error = lerror;
			len = size;
			release_port(ns,inPort);
			pos = 0;
			return error;
		};
		
		int32 read(int32 *intData) {
			if ((pos+4) > len) return B_IO_ERROR;
			*intData = *((int32*)(buffer+pos));
			pos += 4;
			return B_OK;
		};
		int32 read(int64 *intData) {
			if ((pos+8) > len) return B_IO_ERROR;
			*intData = *((int64*)(buffer+pos));
			pos += 8;
			return B_OK;
		};
		int32 read(void *voidData, int32 dataLen) {
			if ((pos+dataLen) > len) return B_IO_ERROR;
			memcpy(voidData,buffer+pos,dataLen);
			pos += dataLen;
			return B_OK;
		};
		int32 read(char **stringData) {
			int32 slen,lerror;
			if (lerror=read(&slen)) return error=lerror;
			*stringData = (char*)malloc(slen+1);
			if (lerror=read(*stringData,slen)) {
				free(*stringData);
				*stringData = NULL;
				return error=lerror;
			};
			(*stringData)[slen] = 0;
			return B_OK;
		};
};


#define checkpoint printf("uspacefs: checkpoint %s(%d)\n",__FILE__,__LINE__);

extern "C" {
	static int		uspacefs_read_vnode(void *ns, vnode_id vnid, char r, void **node);
	static int		uspacefs_write_vnode(void *ns, void *node, char r);
	static int		uspacefs_remove_vnode(void *ns, void *node, char r);
	static int		uspacefs_secure_vnode(void *ns, void *node);
	static int		uspacefs_walk(void *ns, void *base, const char *file,
							char **newpath, vnode_id *vnid);
	static int		uspacefs_access(void *ns, void *node, int mode);
	static int		uspacefs_create(void *ns, void *dir, const char *name,
						int perms, int omode, vnode_id *vnid, void **cookie);
	static int      uspacefs_rename(void *_ns, void *_olddir, const char *_oldname,
						void *_newdir, const char *_newname);
	static int		uspacefs_mkdir(void *ns, void *dir, const char *name, int perms);
	static int		uspacefs_unlink(void *ns, void *dir, const char *name);
	static int		uspacefs_rmdir(void *ns, void *dir, const char *name);
	static int		uspacefs_opendir(void *ns, void *node, void **cookie);
	static int		uspacefs_closedir(void *ns, void *node, void *cookie);
	static int		uspacefs_free_dircookie(void *ns, void *node, void *cookie);
	static int		uspacefs_rewinddir(void *ns, void *node, void *cookie);
	static int		uspacefs_readdir(void *ns, void *node, void *cookie,
						long *num, struct dirent *buf, size_t bufsize);
	static int		uspacefs_rstat(void *ns, void *node, struct stat *st);
	static int		uspacefs_wstat(void *ns, void *node, struct stat *st, long mask);
	static int      uspacefs_fsync(void *ns, void *node);
	static int		uspacefs_open(void *ns, void *node, int omode, void **cookie);
	static int		uspacefs_close(void *ns, void *node, void *cookie);
	static int		uspacefs_free_cookie(void *ns, void *node, void *cookie);
	static int		uspacefs_read(void *ns, void *node, void *cookie, off_t pos,
							void *buf, size_t *len);
	static int		uspacefs_write(void *ns, void *node, void *cookie, off_t pos,
							const void *buf, size_t *len);
	static int		uspacefs_ioctl(void *ns, void *node, void *cookie, int cmd,
							void *buf, size_t len);
	static int		uspacefs_mount(nspace_id nsid, const char *device, ulong flags,
							void *parms, size_t len, void **data, vnode_id *vnid);
	static int		uspacefs_unmount(void *ns);
	static int		uspacefs_rfsstat(void *ns, struct fs_info *);
	static int      uspacefs_wfsstat(void *ns, struct fs_info *, long mask);
	static int		uspacefs_sync(void *ns);
	static int      uspacefs_initialize(const char *devname, void *parms, size_t len);
	static int      uspacefs_open_attrdir(void *_ns, void *_node, void **_cookie);
	static int      uspacefs_close_attrdir(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_free_attr_dircookie(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_rewind_attrdir(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_read_attrdir(void *_ns, void *_node, void *_cookie,
										int32 *num, struct dirent *buf,
										size_t bufsize);
	static int      uspacefs_read_attr(void *_ns, void *_node, const char *name,
									 int type, void *buf, size_t *len,
									 off_t pos);
	static int      uspacefs_write_attr(void *_ns, void *_node, const char *name,
									 int type, const void *buf, size_t *len,
									 off_t pos);
	static int      uspacefs_stat_attr(void *_ns, void *_node, const char *name,
									 struct attr_info *buf);
	static int		uspacefs_symlink(void *_ns, void *_dir, const char *name, const char *path);
	static int		uspacefs_readlink(void *ns, void *node, char *buf, size_t *bufsize);

	
	vnode_ops fs_entry =  {
		&uspacefs_read_vnode,
		&uspacefs_write_vnode,
		&uspacefs_remove_vnode,
		&uspacefs_secure_vnode,
		&uspacefs_walk,
		&uspacefs_access,
		&uspacefs_create,
		&uspacefs_mkdir,
		&uspacefs_symlink,
		NULL,
		&uspacefs_rename,
		&uspacefs_unlink,
		&uspacefs_rmdir,
		&uspacefs_readlink,

		&uspacefs_opendir,
		&uspacefs_closedir,
		&uspacefs_free_dircookie,
		&uspacefs_rewinddir,
		&uspacefs_readdir,

		&uspacefs_open,
		&uspacefs_close,
		&uspacefs_free_cookie,
		&uspacefs_read,
		&uspacefs_write,
		NULL, 											/* readv */
		NULL, 											/* writev */
		&uspacefs_ioctl,
		NULL,
		&uspacefs_rstat,
		&uspacefs_wstat,
		&uspacefs_fsync,
		NULL,											/* &uspacefs_initialize, */
		&uspacefs_mount,
		&uspacefs_unmount,
		&uspacefs_sync,
	
		&uspacefs_rfsstat,
		NULL, 											/* &uspacefs_wfsstat, */
	
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
	
		&uspacefs_open_attrdir,
		&uspacefs_close_attrdir,
		&uspacefs_free_attr_dircookie,
		&uspacefs_rewind_attrdir,
		&uspacefs_read_attrdir,
	
		NULL,											/* &uspacefs_write_attr, */
		&uspacefs_read_attr,
	    NULL,											/* &uspacefs_remove_attr, */
		NULL,											/* &uspacefs_rename_attr, */
		&uspacefs_stat_attr,
		
		NULL,
		NULL,
		NULL,
		NULL
	};
	
	int32 api_version = B_CUR_FS_API_VERSION;
}

struct notify_callback_struct {
	vnode_id	vnid;
	vnode_id	dirvnid;
	int32 		cmd;
	char 		name[64];
};

static int32 notify_callback(nspace_id nsid, notify_callback_struct *ncs)
{
	int32 r1,r2,r3,result;
	void *node;

	switch (ncs->cmd) {
		case B_ATTR_CHANGED: {
			notify_listener(B_ATTR_CHANGED, nsid, 0, 0, ncs->vnid, ncs->name);
		}; break;
		case B_ENTRY_REMOVED: {
			printf("B_ENTRY_REMOVED 1 %Lx\n",ncs->vnid);
			result = get_vnode(nsid,ncs->vnid,&node);
			if (result == B_OK) {
				printf("B_ENTRY_REMOVED 2\n");
				r1 = notify_listener(B_ENTRY_REMOVED, nsid, ncs->dirvnid, 0, ncs->vnid, NULL);
				r2 = remove_vnode(nsid,ncs->vnid);
				r3 = put_vnode(nsid,ncs->vnid);
				if (r3 != B_OK) result = r3;
				if (r2 != B_OK) result = r2;
				if (r1 != B_OK) result = r1;
			};
		}; break;
		
		case B_ENTRY_CREATED: {
			result = notify_listener(B_ENTRY_CREATED, nsid, ncs->dirvnid, 0, ncs->vnid, ncs->name);
			printf("Notified ENTRY_CREATED dir %Lx file %Lx : %s\n", ncs->dirvnid, ncs->vnid,ncs->name);
		}; break;
		
		default: {
		}; break;
	};
	
	return result;
};

static int
open_it(USpaceFSCommand cmd, void *_ns, void *_node, void **_cookie, int32 omode)
{
	int32 err = B_OK;
	nspace *ns = (nspace *)_ns;
	vnode *node = (vnode *)_node;
	Cookie *c = NULL;
	char *str;

	uspace_io io(ns);
	io.write(cmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	if (omode != -1) io.write(omode);
	if (err = io.transact()) goto err1;

	c = (Cookie*)malloc(sizeof(Cookie));	
	c->readfrom = c->writeto = -1;
	if (err = io.read(&c->cookieLen)) goto err2;
	c->cookie = malloc(c->cookieLen);
	if (err = io.read(c->cookie,c->cookieLen)) goto err3;

	*_cookie = c;

	return err;

err3:
	free(c->cookie);
err2:
	free(c);
err1:
	return err;
}

static int
close_it(USpaceFSCommand cmd, void *_ns, void *_node, void *_cookie)
{
	int32 err = 0;
	nspace *ns = (nspace *)_ns;
	vnode *node = (vnode *)_node;
	Cookie *c = (Cookie*)_cookie;

	if (c->readfrom) close(c->readfrom);
	if (c->writeto) close(c->writeto);

	uspace_io io(ns);
	io.write(cmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(c->cookieLen);
	io.write(c->cookie,c->cookieLen);
	err = io.transact();
	return err;
}

static int
free_cookie(USpaceFSCommand cmd, void *_ns, void *_node, void *_cookie)
{
	int32 err = B_OK;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	Cookie *c = (Cookie*)_cookie;

	uspace_io io(ns);
	io.write(cmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(c->cookieLen);
	io.write(c->cookie,c->cookieLen);
	if (err=io.transact()) return err;

	free(c->cookie);
	free(c);
	return B_OK;
}

static int
rewind_dir(USpaceFSCommand cmd, void *_ns, void *_node, void *_cookie)
{
	int32 newLen,err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	Cookie *c = (Cookie*)_cookie;

	uspace_io io(ns);
	io.write(cmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(c->cookieLen);
	io.write(c->cookie,c->cookieLen);
	if (err = io.transact()) return err;
	if (err = io.read(c->cookie,c->cookieLen)) return err;

	return err;
}

static int
read_dir(USpaceFSCommand cmd, void *_ns, void *_node, void *_cookie,
		int32 *num, struct dirent *buf,
		size_t bufsize)
{
	int32 size,err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	Cookie *c = (Cookie*)_cookie;

	uspace_io io(ns);
	io.write(cmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(c->cookieLen);
	io.write(c->cookie,c->cookieLen);
	if (err = io.transact()) {
		if (err == ENOENT) {
			*num = 0;
			return B_OK;
		};
		return err;
	};
	if (err = io.read(&buf->d_ino)) return err;
	if (err = io.read(&size)) return err;
	if ((size+12) > bufsize) return EINVAL;
	buf->d_reclen = size;
	if (err = io.read(buf->d_name,buf->d_reclen)) return err;
	buf->d_name[size] = 0;
	if (err = io.read(c->cookie,c->cookieLen)) return err;

	*num = 1;
	return err;
}

static int
uspacefs_read_vnode(void *_ns, vnode_id vnid, char r, void **node)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *vn = NULL;

	if (vnid == ns->rootID) {
		*node = ns->root;
		return B_OK;
	};

	uspace_io io(ns);
	io.write(USPACEFS_READ_VNODE);
	io.write(vnid);
	if (err = io.transact()) goto err1;
	vn = (vnode*)malloc(sizeof(vnode));
	vn->ns = ns;
	vn->vnid = vnid;
	if (err = io.read(&vn->idLen)) goto err2;
	vn->id = malloc(vn->idLen);
	if (err = io.read(vn->id,vn->idLen)) goto err3;

	*node = vn;
	return err;
	
err3:
	free(vn->id);
err2:
	free(vn);
err1:
	return err;
}

static int
uspacefs_write_vnode(void *_ns, void *_vn, char r)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *vn = (vnode*)_vn;

	if (vn == ns->root) return B_OK;

	uspace_io io(ns);
	io.write(USPACEFS_WRITE_VNODE);
	io.write(vn->idLen);
	io.write(vn->id,vn->idLen);
	if (err = io.transact()) return err;

	free(vn->id);
	free(vn);

	return err;
}

static int
uspacefs_remove_vnode(void *_ns, void *_vn, char r)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *vn = (vnode*)_vn;

	uspace_io io(ns);
	io.write(USPACEFS_REMOVE_VNODE);
	io.write(vn->idLen);
	io.write(vn->id,vn->idLen);
	if (err = io.transact()) return err;

	free(vn->id);
	free(vn);

	return err;
}

static int
uspacefs_secure_vnode(void *_ns, void *_node)
{
	int32 err = 0;
	return err;
}

static int
uspacefs_walk(void *_ns, void *_base, const char *filename,
		char **newpath, vnode_id *vnid)
{
	int32 err = 0,size;
	nspace *ns = (nspace*)_ns;
	vnode *fred,*base = (vnode*)_base;
	char path[256];
	
	uspace_io io(ns);
	io.write(USPACEFS_WALK);
	io.write(base->idLen);
	io.write(base->id,base->idLen);
	io.write((int32)(newpath?1:0));
	io.write(filename);
	if (err = io.transact()) goto err1;
	if (err = io.read(vnid)) goto err1;
	if (newpath) {
		if (err = io.read(&size)) goto err1;
		if (size > 0) {
			if (err = io.read(path,size)) goto err1;
			path[size] = 0;
			if (err = new_path(path,newpath)) goto err1;
			printf("redirecting walk of \"%s\" to \"%s\"\n",filename,*newpath);
			goto noget;
		};
	};
	
	err = get_vnode(ns->nsid,*vnid,(void**)&fred);

noget:;
err1:
	return err;
}

static int
uspacefs_access(void *_ns, void *_node, int mode)
{
	return 0;
}

static int
uspacefs_rename(void *_ns, void *_olddir, const char *_oldname,
		void *_newdir, const char *_newname)
{
	int32 err = EPERM;
	nspace *ns = (nspace*)_ns;
	vnode *olddir = (vnode*)_olddir;
	vnode *newdir = (vnode*)_newdir;
	return err;
}

static int
uspacefs_create(void *_ns, void *_dir, const char *name,
			  int omode, int perms, vnode_id *vnid, void **cookie)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node,*base = (vnode*)_dir;

	if ((strcmp(name,".")==0) ||
		(strcmp(name,"..")==0) ||
		(name[0] == 0) ||
		(name[0] == '/'))
		return EINVAL;
	
	uspace_io io(ns);
	io.write(USPACEFS_WALK);
	io.write(base->idLen);
	io.write(base->id,base->idLen);
	io.write((int32)0);
	io.write(name);
	if (!(err = io.transact())) {
		if (err = io.read(vnid)) return err;
		if (err = get_vnode(ns->nsid,*vnid,(void**)&node)) return err;
		err = open_it(USPACEFS_OPEN,_ns,node,cookie,omode);
	};

	return err;
}

static int
uspacefs_mkdir(void *_ns, void *_dir, const char *_name, int perms)
{
	int32 err = EPERM;
	nspace *ns = (nspace*)_ns;
	vnode *dir = (vnode*)_dir;
	return err;
}

static int
uspacefs_unlink(void *_ns, void *_dir, const char *_name)
{
	int32 err = EPERM;
	nspace *ns = (nspace*)_ns;
	vnode *dir = (vnode*)_dir;
	return err;
}

static int
uspacefs_rmdir(void *_ns, void *_dir, const char *_name)
{
	int32 err=EPERM;
	nspace *ns = (nspace *) _ns;
	vnode *node = (vnode *) _dir;
	return err;
}

static int
uspacefs_opendir(void *_ns, void *_node, void **_cookie)
{
	return open_it(USPACEFS_OPENDIR,_ns,_node,_cookie,-1);
}

static int
uspacefs_closedir(void *_ns, void *_node, void *_cookie)
{
	return close_it(USPACEFS_CLOSEDIR,_ns,_node,_cookie);
}

static int
uspacefs_free_dircookie(void *_ns, void *_node, void *_cookie)
{
	return free_cookie(USPACEFS_FREEDIRCOOKIE,_ns,_node,_cookie);
}

static int
uspacefs_rewinddir(void *_ns, void *_node, void *_cookie)
{
	return rewind_dir(USPACEFS_REWINDDIR,_ns,_node,_cookie);
}

static int
uspacefs_readdir(void *_ns, void *_node, void *_cookie,
		long *num, struct dirent *buf, size_t bufsize)
{
	return read_dir(USPACEFS_READDIR,_ns,_node,_cookie,num,buf,bufsize);
}

static int
uspacefs_rstat(void *_ns, void *_node, struct stat *st)
{
	int32 tmp,err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	st->st_dev = ns->nsid;
	st->st_ino = node->vnid;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_blksize = 64 * 1024;
	st->st_crtime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	st->st_atime = 0;

	uspace_io io(ns);
	io.write(USPACEFS_RSTAT);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	if (err = io.transact()) return err;
	if (err = io.read(&tmp)) return err;
	st->st_mode = tmp;
	if (err = io.read(&st->st_size)) return err;

	return err;
}

static int
uspacefs_wstat(void *_ns, void *_node, struct stat *st, long mask)
{
	int32 err = EPERM;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	return err;
}

static int
uspacefs_open(void *_ns, void *_node, int omode, void **cookie)
{
	return open_it(USPACEFS_OPEN,_ns,_node,cookie,omode);
}

static int
uspacefs_close(void *_ns, void *_node, void *_cookie)
{
	return close_it(USPACEFS_CLOSE,_ns,_node,_cookie);
}

static int
uspacefs_free_cookie(void *_ns, void *_node, void *_cookie)
{
	return free_cookie(USPACEFS_FREECOOKIE,_ns,_node,_cookie);
}

typedef ssize_t (*iocall_t)(int,void *,size_t);

static int
uspacefs_io(void *_ns, void *_node, void *_cookie, off_t pos,
			void *buf, size_t *len,
			USpaceFSCommand opencmd, USpaceFSCommand iocmd,
			int32 *fd, int32 omode, iocall_t iocall)
{
	int32 err = B_OK;
	int32 size = *len;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	Cookie *c = (Cookie*)_cookie;
	char *str;

	*len = 0;

	/*	If we've tried before to open a pipe and failed,
		trying again won't help any. */
	if (*fd == -2) return err;

	if (*fd == -1) {
		/*	Try to open a pipe */
		uspace_io io(ns);
		io.write(opencmd);
		io.write(node->idLen);
		io.write(node->id,node->idLen);
		io.write(c->cookieLen);
		io.write(c->cookie,c->cookieLen);
		err = io.transact();
		if (err = io.read(&str)) return err;
		*fd = -1;
		if (str[0] != 0) *fd = open(str,omode);
		free(str);
		if (*fd < 0) {
			err = *fd;
			*fd = -2;
			return err;
		};
	};

	uspace_io io(ns);
	io.write(iocmd);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(c->cookieLen);
	io.write(c->cookie,c->cookieLen);
	io.write(pos);
	io.write(size);
	if (err = io.transact()) return err;
	if (err = io.read(&size)) return err;

	*len = iocall(*fd,buf,size);
	if (*len < 0) {
		err = *len;
		*len = 0;
	};

	return err;
}

static int
uspacefs_read(void *_ns, void *_node, void *_cookie, off_t pos,
		void *buf, size_t *len)
{
	return uspacefs_io(_ns,_node,_cookie,pos,buf,len,
		USPACEFS_OPENREADPIPE,USPACEFS_READ,
		&((Cookie*)_cookie)->readfrom,O_CREAT|O_RDONLY,(iocall_t)&read);
};

static int
uspacefs_write(void *_ns, void *_node, void *_cookie, off_t pos,
		const void *buf, size_t *len)
{
	return uspacefs_io(_ns,_node,_cookie,pos,(void*)buf,len,
		USPACEFS_OPENWRITEPIPE,USPACEFS_WRITE,
		&((Cookie*)_cookie)->writeto,O_CREAT|O_WRONLY,(iocall_t)&write);
}

static int
uspacefs_ioctl(void *_ns, void *_node, void *_cookie, int cmd,
		void *buf, size_t len)
{  
	int32 err = B_OK;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	if (cmd == USPACE_FS_NOTIFY_CALLBACK)
		return notify_callback(ns->nsid, (notify_callback_struct*)buf);

	uspace_io io(ns);
	io.write(USPACEFS_IOCTL);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write((int32)cmd);
	err = io.transact();

	return err;
}

static int
uspacefs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **_ns, vnode_id *_vnid)
{
//	notify_data *nd;
	int32 err = 0;
	port_info pi;
	mountparms *p = (mountparms*)parms;
	nspace *ns = (nspace*)malloc(sizeof(nspace));
	uspace_io io(ns);

	ns->nsid = nsid;
	ns->inPortLock.c = 1;
	ns->inPortLock.s = create_sem(0,"uspace inPort lock");
	ns->inPort = NULL;
	ns->outPort = p->outPort;
	ns->name = NULL;
	ns->device = NULL;
/*
	ns->notifyPort = create_port(10,"notifyPort");
	if (ns->notifyPort < 0) { err = ns->notifyPort; goto err1; };
*/
	if (!get_port_info(p->outPort,&pi)) {
		printf("assiging port ownership to team %d\n",pi.team);
//		set_port_owner(ns->notifyPort,pi.team);
		ns->serverTeam = pi.team;
	} else {
		printf("get_port_info failed!\n");
		ns->serverTeam = B_BAD_VALUE;
	};

	if (!device) device = "";
	if (ns->inPortLock.s < 0) { err = ns->inPortLock.s; goto err1; };
/*
	nd = (notify_data*)malloc(sizeof(notify_data));
	nd->nsid = ns->nsid;
	nd->notifyPort = ns->notifyPort;
	ns->notifyThread = spawn_kernel_thread(notify_thread,"notify_thread",B_NORMAL_PRIORITY,nd);
	if (ns->notifyThread < 0) { err = ns->notifyThread; free(nd); goto err1; };
	if ((err=resume_thread(ns->notifyThread)) != B_OK) { free(nd); goto err1; };
*/
	io.write(USPACEFS_MOUNT);
	io.write(device);
	if (err = io.transact()) goto err1;
	if (err = io.read(&ns->rootID)) goto err1;
	if (err = io.read(&ns->flags)) goto err1;
	if (err = io.read(&ns->name)) goto err1;
	if (err = io.read(&ns->fsname)) goto err2;
	ns->device = strdup(device);

	if (!(ns->root = (vnode*)malloc(sizeof(vnode)))) { err = ENOMEM; goto err3; };

	io.reset();
	io.write(USPACEFS_READ_VNODE);
	io.write(ns->rootID);
	if (err = io.transact()) goto err4;
	if (err = io.read(&ns->root->idLen)) goto err4;
	if (!(ns->root->id = malloc(ns->root->idLen))) { err = ENOMEM; goto err4; };
	if (err = io.read(ns->root->id,ns->root->idLen)) goto err5;

	if (new_vnode(nsid,ns->rootID,ns->root) != 0) goto err5;

	ns->root->ns = ns;
	ns->root->vnid = ns->rootID;
	*_vnid = ns->rootID;
	*_ns = ns;
	return err;

err5:
	free(ns->root->id);
err4:
	free(ns->root);
err3:
	uspacefs_unmount(ns);
	return err;
err2:
	free(ns->name);
err1:
//	if (ns->notifyThread >= 0) kill_thread(ns->notifyThread);
	delete_ports(ns);
	free(ns);
	return err;
}

static int
uspacefs_rfsstat(void *_ns, struct fs_info * fss)
{
	nspace *ns = (nspace*)_ns;

	fss->dev = 0;
	fss->root = ns->rootID;
	fss->flags = ns->flags;
	fss->block_size = 0;
	fss->io_size = 0;
	fss->total_blocks = 0;
	fss->free_blocks = 0;
	strncpy(fss->device_name,ns->device,sizeof(fss->device_name));
	strncpy(fss->volume_name,ns->name,sizeof(fss->volume_name));
	strncpy(fss->fsh_name,ns->fsname,sizeof(fss->fsh_name));
	return 0;
}

static int
uspacefs_unmount(void *_ns)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;

	uspace_io io(ns);
	io.write(USPACEFS_UNMOUNT);
	io.write(ns->rootID);
	io.write(ns->name);

	err = io.transact();
	if (err != 0) {
		printf("uspacefs_unmount io.transact returning %s\n",strerror(err));
	}

	//if ((err = io.transact()) && (err == B_BAD_PORT_ID)) err = 0;
	//if (err) return err;
	
	free(ns->name);
	free(ns->fsname);
	free(ns->device);
	delete_ports(ns);
//	if (ns->notifyThread >= 0) kill_thread(ns->notifyThread);
	free(ns);
	return err;
}

static int
uspacefs_fsync(void *_ns, void *_node)
{
	return 0;
}

static int
uspacefs_sync(void *_ns)
{
	return 0;
}

static int
uspacefs_open_attrdir(void *_ns, void *_node, void **_cookie)
{
	return open_it(USPACEFS_OPENATTRDIR,_ns,_node,_cookie,-1);
}

static int
uspacefs_close_attrdir(void *_ns, void *_node, void *_cookie)
{
	return close_it(USPACEFS_CLOSEATTRDIR,_ns,_node,_cookie);
}

static int
uspacefs_free_attr_dircookie(void *_ns, void *_node, void *_cookie)
{
	return free_cookie(USPACEFS_FREEATTRDIRCOOKIE,_ns,_node,_cookie);
}

static int
uspacefs_rewind_attrdir(void *_ns, void *_node, void *_cookie)
{
	return rewind_dir(USPACEFS_REWINDATTRDIR,_ns,_node,_cookie);
}

static int
uspacefs_read_attrdir(void *_ns, void *_node, void *_cookie,
					int32 *num, struct dirent *buf,
					size_t bufsize)
{
	return read_dir(USPACEFS_READATTRDIR,_ns,_node,_cookie,num,buf,bufsize);
}

static int
uspacefs_read_attr(void *_ns, void *_node, const char *name,
				 int type, void *buf, size_t *len,
				 off_t pos)
{
	int32 len32,err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	uspace_io io(ns);
	io.write(USPACEFS_READATTR);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(name);
	len32 = *len;
	io.write(len32);
	io.write(pos);
	if (err = io.transact()) return err;
	if (err = io.read(&len32)) return err;
	if (err = io.read(buf,len32)) return err;
	*len = len32;

	return err;
}

static int
uspacefs_write_attr(void *_ns, void *_node, const char *name,
				  int type, const void *buf, size_t *len,
				  off_t pos)
{
	return EPERM;
}

static int
uspacefs_stat_attr(void *_ns, void *_node, const char *name,
				 struct attr_info *buf)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	uspace_io io(ns);
	io.write(USPACEFS_STATATTR);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	io.write(name);
	if (err = io.transact()) return err;
	if (err = io.read(buf,sizeof(attr_info))) return err;

	return err;
}

static int
uspacefs_symlink(void *_ns, void *_dir, const char *name, const char *path)
{
	return B_UNSUPPORTED;
}

static int 
uspacefs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize)
{
	int32 err = 0,size;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	uspace_io io(ns);
	io.write(USPACEFS_READLINK);
	io.write(node->idLen);
	io.write(node->id,node->idLen);
	if (err = io.transact()) return err;
	if (err = io.read(&size)) return err;
	if (size >= *bufsize) size = *bufsize - 1;
	*bufsize = size+1;
	if (err = io.read(buf,size)) return err;
	buf[size] = 0;

	return err;
}

