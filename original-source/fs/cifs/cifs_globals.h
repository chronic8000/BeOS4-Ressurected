#ifndef _CIFS_GLOBALS_H_
#define _CIFS_GLOBALS_H_


#include <Errors.h>
#include <stdio.h>
#include <SupportDefs.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef KERNEL_SPACE
#include <sys/socket_module.h>
#else
#include <sys/socket.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <fsproto.h>
#include <dprintf.h>
#include <string.h>
#include <termio.h>
#include <stdlib.h>
#include <lock.h>
#include <time.h>
#include <dirent.h>
#include <kalloc.h>
#include "skiplist.h"


enum {
	CommonHeader = 36,  //  Size for Netbios and SMB header
	CIFS_UPDATE_GRANULARITY = 3, // 3 seconds between dir listings
	CIFS_RECV_TIMEOUT = 10,
	CIFS_MAX_RETRIES = 2 // Number of times to try restarting a connection
};




#define COMMON_HEADER_SIZE 32
#define CIFS_IO_SIZE 32768
#define ARTIFICIAL_VNID_BITS (0xAEFDLL << 48)


#define CIFS_C_DBG_NUM 0
#define DBGWRITE 15
#define aprintf dprintf
#define STANDARD_DEBUG_NUMBER 90

#define CIFS_DEBUG_BASE 400

#if DEBUG >= 1
#ifdef KERNEL_SPACE
#define DPRINTF(a,b) if ((a > CIFS_DEBUG_BASE) || \
							(a == -1)) { \
							dprintf b; \
							}; 
#else
#define DPRINTF(a,b) if ((a > CIFS_DEBUG_BASE) || \
							(a == -1)) \
							printf b;
#endif
#else
#define DPRINTF(a,b) ;
#endif

#define debug DPRINTF

//dprintf("ALLOC %x : %d bytes : %s line %d\n", var , size , __FILE__ , __LINE__ ); \
//dprintf("FREE  %x : %s line %d\n", var , __FILE__ , __LINE__ ); \

/*
						var = (type) malloc((size)); \
						var = (type) dcalloc((size), sizeof(char), __LINE__, __FILE__); \
*/
#define MALLOC(var, type, size)		{ \
					if (size != 0) { \
						var = (type) smalloc((size)); \
						if (var != NULL) memset(var,0,size); \
						if (var == NULL) debug(-1, ("%s line %d couldnt alloc %d bytes\n", __FILE__, __LINE__, size)); \
					} else { \
						DPRINTF(-1, ("%s line %d wants zero bytes of memory!\n", __FILE__, __LINE__)); \
						var = NULL; \
					} \
				};
/*
					dfree(var, __LINE__, __FILE__); \
					free(var); \
*/
#define FREE(var)	{ \
					if (var == NULL) { \
					debug(-1, ("FREE NULL at %s %d\n", __FILE__, __LINE__)); \
					} else { \
					sfree(var); \
					} \
					};




#define LOCK_VOL(vol) \
{ \
if (vol == NULL) { \
	return EINVAL; \
} \
if (vol->socket_state != 0) {  \
	DPRINTF(-1, ("socket_state is %d\n", vol->socket_state)); \
	return vol->socket_state; \
} \
\
LOCK((vol)->vlock); \
/*dprintf("locked at %s %d\n", __FILE__, __LINE__); */ \
} 

#define UNLOCK_VOL(vol) \
{ \
/* dprintf("\tunlocking at %s %d\n", __FILE__, __LINE__); */ \
UNLOCK((vol)->vlock); \
} 



typedef struct nb_header {
	uchar 	Command;
	uchar	Flags;
	uint16	Length;
} nb_header_t;
              

typedef struct cifs_common_header {

	char		Protocol[4];
	uchar 		Command;
	ulong 		Status;
	uchar 		Flags;
	ushort		Flags2;
	ushort		Pad[6];
	
	ushort		Tid;
	ushort		Pid;
	ushort		Uid;
	ushort		Mid;

} cifs_common_header;


typedef struct parms_t {
	char Unc[256];
	char Server[256];
	ulong	server_inetaddr;
	char Share[256];
	char Username[256];
	char Password[256];
	char Myname[256];
	char LocalShareName[256];
	char LocalWorkgroup[64];
} parms_t;



typedef struct _int_dir_entry {
	vnode_id		vnid;
    mode_t			mode;       /* mode bits (rwx for user, group, etc) */
	nlink_t			nlink;      /* number of hard links to this file */
    uid_t			uid;        /* user id of the owner of this file */
    gid_t			gid;        /* group id of the owner of this file */
    off_t			size;       /* size in bytes of this file */
    size_t			blksize;    /* preferred block size for i/o */
    time_t			atime;      /* last access time */
    time_t			mtime;      /* last modification time */
    time_t			ctime;      /* last change time, not creation time */
    time_t			crtime;   		
	char			filename[255];
	bool			del; // marked for deletion
} int_dir_entry;

typedef struct _dircookie {
	int				magic;
	vnode_id		previous;
	int				omode;
	int				index;
} dircookie;

typedef struct vnode {
	vnode_id		vnid;
	vnode_id		parent_vnid;
 
	// These should NOT contain info for the . and .., fyi.
	SkipList		contents;
	uint32			last_update_time;
	
	char			name[255];

    mode_t			mode;       /* mode bits (rwx for user, group, etc) */
	nlink_t			nlink;      /* number of hard links to this file */
    uid_t			uid;        /* user id of the owner of this file */
    gid_t			gid;        /* group id of the owner of this file */
    off_t			size;       /* size in bytes of this file */
    size_t			blksize;    /* preferred block size for i/o */
    time_t			atime;      /* last access time */
    time_t			mtime;      /* last modification time */
    time_t			ctime;      /* last change time, not creation time */
    time_t			crtime;

	const char *mime;
} vnode;


typedef struct nspace {

	nspace_id		id;
	int				sockid;
	status_t		socket_state;
	status_t		connect_state;
	uint32			connection_retries;	

	parms_t			Params;

#if 0
	char			Username[256];
	char			Password[256];
	char			Unc[256];
	char			Server[256];
	char			Share[256];
#endif

#define SHARE_IS_USER_MODE 0x1
#define SHARE_IS_SHARE_MODE 0x0
	uchar			SecurityMode;
	ushort			MaxMpxCount;
	ushort			MaxNumberVcs;
	ulong			MaxBufferSize;
	ulong			MaxRawBufferSize;
	ulong			SessionKey;
	ulong			Capabilities;
	char			PasswordResponse[24];
	char			OemDomainName[16];
#define OEMDOMAINNAME_LENGTH 16

	char			ServerOS[64];
	char			ServerLM[64];
	char			ServerDomain[64];

	bool		nt_smbs;

	uint64		fss_block_size;
	uint64		fss_total_blocks;
	uint64		fss_free_blocks;


	uchar		Flags;
	ushort		Flags2;

	ushort		Tid;
	ushort		Pid;
	ushort		Uid;
	ushort		Mid;
	
	int			io_size;
	int			small_io_size;
	bool		cache_enabled;
		
	nspace_id	nsid;
	vnode		root_vnode;
	
	//vcache 
	struct {
		sem_id vc_sem;
		vnode_id cur_vnid;
		uint32 cache_size;
		struct vcache_entry **by_vnid, **by_loc;
	} vcache;

	lock 			vlock;
	
	thread_id	worker_thread;
	sem_id		worker_sem;
	bigtime_t	last_traffic_time;

} nspace;

#define use_cleartext_password(vol) ((vol)->PasswordResponse[0] == 0)
#define share_in_user_mode(vol) ((vol)->SecurityMode & SHARE_IS_USER_MODE)


typedef struct _file_cache {
	off_t	pos;
	size_t	len;
	uchar	*buf;
} file_cache;

enum {
	WAS_WRITTEN = 0x1
};


static const int CIFS_FILE_COOKIE = 0x87878787;

typedef struct _filecookie {
	int			magic;
	int			omode;
	ushort		fid;
	uint32		flags;
	file_cache 	cache;
} filecookie;

// Rename Flags
enum {
	RENAME_SAMBA_STYLE = 0x1,
	RENAME_WIN_STYLE = 0x2,
	RENAME_IS_DIR = 0x3
};



//Dialects


//NetBios Session Packet Types (RFC 1002)
#define NB_SESSION_MESSAGE 				0	
#define NB_SESSION_REQUEST				0x81
#define NB_POSITIVE_SESSION_RESPONSE	0x82
#define NB_NEGATIVE_SESSION_RESPONSE  	0x83
#define NB_RETARGET_SESSION_RESPONSE	0x84
#define NB_SESSION_KEEPALIVE			0x85


#ifdef KERNEL_SPACE
extern bone_socket_info_t *gSock;	/* cifs_ops.c */

#define KCLOSESOCKET(x) close(x)
#define KRECV(x,y,z,w) gSock->recv(x,y,z,w)
#define KSEND(x,y,z,w) gSock->send(x,y,z,w)
#define KSOCKET_CLEANUP() B_OK
#define KCONNECT(x,y,z) gSock->connect(x,y,z)
#define KSOCKET(x,y,z) gSock->socket(x,y,z)
#define KSOCKET_INIT() B_OK
#define KSELECT(a, b, c, d, e) select((a), (b), (c), (d), (e))
#else
#define KCLOSESOCKET(x) close(x)
#define KRECV(x,y,z,w) recv(x,y,z,w)
#define KSEND(x,y,z,w) send(x,y,z,w)
#define KSOCKET_CLEANUP()
#define KCONNECT(x,y,z) connect(x,y,z) 
#define KSOCKET(x,y,z) socket(x,y,z)
#define KSOCKET_INIT() B_OK
#define KSELECT(a, b, c, d, e) select((a), (b), (c), (d), (e))
#endif



#ifdef DEFINE_GLOBAL_VARS
char Dialects[11] = "NT LM 0.12";
const int command_off = 4;
const int status_off = 5;//command_off + 1;
const int flags_off = 9;//status_off + 4;
const int flags2_off = 10;//flags_off + 1;
const int pad_off = 12;//flags2_off + 2;
const int tid_off = 24;//pad_off + 12;
const int pid_off = 26;//tid_off + 2;
const int uid_off = 28;//pid_off + 2;
const int mid_off = 30;//uid_off + 2;
const int wordcount_off = 32;//mid_off + 2;
#else
extern char Dialects[];
extern const int command_off;
extern const int status_off;
extern const int flags_off;
extern const int flags2_off;
extern const int pad_off;
extern const int tid_off;
extern const int pid_off;
extern const int uid_off;
extern const int mid_off;
extern const int wordcount_off;
#endif


enum {
	SMB_COM_CREATE_DIRECTORY = 0x0,
	SMB_COM_DELETE_DIRECTORY = 0x01,
	SMB_COM_OPEN = 0x02,
	SMB_COM_OPEN_ANDX = 0x2d,
	SMB_COM_CREATE = 0x03,
	SMB_COM_CREATE_NEW = 0x0f,
	SMB_COM_TREE_CONNECT = 0x70,
	SMB_COM_TREE_DISCONNECT = 0x71,
	SMB_COM_TREE_CONNECT_ANDX = 0x75,
	SMB_COM_NEGOTIATE = 0x72,
	SMB_COM_SESSION_SETUP_ANDX = 0x73,
	SMB_COM_LOGOFF_ANDX = 0x74,
	SMB_COM_OPEN_PRINT_FILE = 0xC0,
	SMB_COM_CLOSE = 0x04,
	SMB_COM_WRITE_ANDX = 0x2f,
	SMB_COM_WRITE = 0x0b,
	SMB_COM_WRITE_PRINT_FILE = 0xc1,
	SMB_COM_CLOSE_PRINT_FILE = 0xc2,
	SMB_COM_TRANSACTION2 = 0x32,
	SMB_COM_NT_CREATE_ANDX = 0xA2,
	SMB_COM_READ_ANDX = 0x2E,
	SMB_COM_DELETE = 0x06,
	SMB_COM_MOVE = 0x2a,
	SMB_COM_WRITE_RAW = 0x1d,
	SMB_COM_READ_RAW = 0x1a,
	SMB_COM_ECHO = 0x2b,
	SMB_COM_QUERY_INFORMATION_DISK = 0x80,
	SMB_COM_RENAME = 0x07,
	SMB_COM_QUERY_INFORMATION = 0x08,
	SMB_COM_FIND_CLOSE2 = 0x34
};

enum {
	TRANS2_FIND_FIRST2 = 1,
	TRANS2_FIND_NEXT2 = 2,
	SMB_FIND_FILE_BOTH_DIRECTORY_INFO = 0x104,
	SMB_FIND_FILE_FULL_DIRECTORY_INFO = 0x102,
	TRANS2_QUERY_PATH_INFORMATION = 0x05,
	SMB_QUERY_ALL_EAS = 4,
	SMB_INFO_STANDARD = 1
};

enum {
	FILE_ATTR_READ_ONLY = 0x01,
	FILE_ATTR_HIDDEN = 0x02,
	FILE_ATTR_SYSTEM = 0x04,
	FILE_ATTR_VOLUME = 0x08,
	FILE_ATTR_DIRECTORY = 0x10,
	FILE_ATTR_ARCHIVE = 0x20
};

enum {
	ERR_INVALID_FUNCTION = 1,
	ERR_FILE_NOT_FOUND = 2,
	ERR_ACCESS_DENIED = 5,
	ERR_BAD_FID = 6,
	ERR_REM_CD = 16,
	ERR_FILE_EXISTS = 80,
	ERR_FILE_ALREADY_EXISTS = 183,
	ERR_BAD_SHARE = 32,
	ERR_BAD_PATH = 3,
	// Server Errors
	ERR_BAD_PW = 2,
	ERR_NET_ACCESS_DENIED = 4,
	ERR_BAD_NET_NAME = 6
};

enum {
	CAP_RAW_MODE = 0x0001,
	CAP_NT_SMBS = 0x0010
};

enum {
	SOCKET_INIT = 0x1,
	NETBIOS_INIT = 0x2,
	TREE_INIT = 0x4
};


typedef struct _packet {
	uchar 		*buf;
	int32		size;
	int32		rempos;
	int32		addpos;
	int32		type;
	int32		command;
	bool		order;
} packet;


enum {
	PACKET_DEFAULT_SIZE = 256,
	PACKET_GROW_SIZE = 128,
	PACKET_SAFETY_MARGIN = 32
};

enum {
	PACKET_DATA_TYPE,
	PACKET_SMB_TYPE
};


#define BUF_DUMP(p, l)  dprintf("Here's %d bytes->", l); \
						for (_tmp=0;_tmp<l;_tmp++) { \
						if (!(_tmp%4)) aprintf("\n"); \
						dprintf("%2x ", (p)[_tmp]); } \
						dprintf("\n<--\n");


#define BUF_DUMPC(p, l) aprintf("Here's %d bytes->", l); \
						for (_tmp=0;_tmp<l;_tmp++) { \
						if (!(_tmp%4)) aprintf("\n"); \
						aprintf("%2c ", (p)[_tmp]); } \
						aprintf("\n<--\n");

/* XXX: intel hackery: sleep() isn't exported by the kernel */
#define sleep(x) snooze((x)*1000000LL)

#endif // _CIFS_GLOBALS_H_
