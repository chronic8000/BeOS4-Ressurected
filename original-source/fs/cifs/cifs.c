#include <OS.h>
#include <errno.h>
#include "cifs_globals.h"
#include "cifs_utils.h"
#include <string.h>
#include "packet.h"
#include "netio.h"

// Forward decls
status_t get_full_name(nspace *vol, vnode *dir, const char *name, char **val);


void
strtoupper(char* string) {
	int i;
	int len = strlen(string);
	
	for(i = 0; i <= len; i++) {
		string[i] = toupper(string[i]);
	}
}



void get_netbios_name(char *in_name, char *new_name, int server) {

	int length;
	char *current_charp;
	int extras = 0;
	char *name_charp;
	char name[20];


	int _tmp;

	
	strcpy(name, in_name);
	strtoupper(name);
	length = strlen(name);	
	current_charp = name;
	name_charp = new_name;
	
	
	*name_charp++ = 32;
	 
	while (* current_charp) {
		*name_charp++ = (*current_charp >> 4) + 'A';
		*name_charp++ = (*current_charp++ & 0x0F) + 'A';
		}
	
	if (length < 16) extras = 16 - length;
	
	while (extras--) {
		*name_charp++='C';
		*name_charp++='A';
		}
	
	*name_charp++ = '\0';

	if (! server) {  // trying to track down why nt 3.51 servers dont like us
		name_charp--;
		name_charp--;
		name_charp--;
		
		*name_charp++ = 'A';
		*name_charp++ = 'A';
		*name_charp++ = '\0';
	}	
	
	length = strlen(new_name);
	
	// BUF_DUMPC(new_name, length + 1);
	
}


// Don't mess with the ksocket connection if we're
// not the only ones using it.

static int32 cifs_open_count = 0;

status_t
init_socket() {
	status_t err;
	if (atomic_add(&cifs_open_count, 1) > 0) {
		dprintf("not initing sockets\n");
		return B_OK;
    } else {
		dprintf("initing sockets\n");
		err = KSOCKET_INIT();
		if (err != B_OK) {
			atomic_add(&cifs_open_count, -1);
			return err;
		} else {
			return B_OK;
		}
	}
}	

void
kill_socket() {
	
	if (atomic_add(&cifs_open_count, -1) > 1) {
		dprintf("not killing sockets");
	} else {
		dprintf("killing socket\n");
		KSOCKET_CLEANUP();
	}
}




status_t
init_netbios_connection(nspace *vol) {
// server_inetaddr should be the net address of the server, in network order

	status_t result;
	struct sockaddr_in my_addr, server_addr;
	int server_port;
	int sock;
	char *calling_name = vol->Params.Myname;
	char *called_name = vol->Params.Server;			
	uchar buf[4];
	struct smb_req_pkt_hdr {
		uchar type;
		uchar flags;
		uint16 length;
		char called_name[34];
		char calling_name[34];
	} init_packet;
	int second_try = 0;


//	result = init_socket();	
//	if (result != B_OK) return result;
	
	
	server_port = 139;  // standard port for NetBios
	memset(&server_addr, 0, sizeof(server_addr));
	memcpy((char *)&server_addr.sin_addr, & vol->Params.server_inetaddr, sizeof(ulong));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);


TRYAGAIN_NT351:
	
	errno = 0;
	sock = KSOCKET(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		DPRINTF(-1, ("init_netbios_connection : couldnt create socket, sock is %d, errno is %s\n", sock, strerror(errno)));
		vol->sockid = -1;
		return errno;
	}


	errno = 0;
	result = KCONNECT(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (result != B_OK) {
		DPRINTF(-1, ("init_netbios_connection : couldnt connect\n"));
		return errno;
	}

	
	init_packet.type = NB_SESSION_REQUEST;
	init_packet.flags = 0;
	init_packet.length = B_HOST_TO_BENDIAN_INT16(68);  // 2*34 (size of name);
	
	if (! second_try)
		get_netbios_name(called_name, init_packet.called_name, true);
	else
		get_netbios_name(vol->Params.Server, init_packet.called_name, true);
	
	
	get_netbios_name(calling_name, init_packet.calling_name, false);
	
	
	result = KSEND(sock, &init_packet, 72, 0);  // + 4 is for header
	if (result != 72) {
		DPRINTF(CIFS_C_DBG_NUM, ("Hmm... only sent %d bytes.\n", result));
		return B_IO_ERROR;
	}
	
	
	if (KRECV(sock, buf, 4, 0) < 4) {
		DPRINTF(CIFS_C_DBG_NUM, ("Coulnd't get bytes off of the connection!!\n"));
		return B_IO_ERROR;
	}
	
	if (buf[0] != NB_POSITIVE_SESSION_RESPONSE) {
		if (! second_try) {
			DPRINTF(CIFS_C_DBG_NUM, ("Got response of %d, going to try again.\n", buf[0]));
			// For NT 3.51, try replacing the called name (usually *SMBSERVER)
			// with the name of the server.
			KCLOSESOCKET(sock);
			second_try = 1;
			goto TRYAGAIN_NT351;
		} else {
			KCLOSESOCKET(sock);
			return B_IO_ERROR;
		}
	} else {
		vol->sockid = sock;
		DPRINTF(CIFS_C_DBG_NUM, ("init_netbios_connection ... exiting with B_OK.\n"));
		return B_OK;
	}
	
}



status_t
kill_netbios_connection(nspace *vol)
{
	status_t err;
	if (vol == NULL) {
		debug(-1, ("kill_netbios_connection : vol is NULL\n"));
	}
	err = KCLOSESOCKET(vol->sockid);
	if (err < B_OK) {
		debug(-1, ("kill_netbios_connection : KCLOSESOCKET failed errno %d\n", strerror(errno)));
	}
	vol->sockid = -2;
	return err;
}



void
put_nb_length(nb_header_t* header, int len) {

	header->Length = B_HOST_TO_BENDIAN_INT16(len);

}


#define OPEN_FILE_DEBUG 200
status_t
nt_send_open_file(nspace *vol, vnode *node, int omode)
{
	status_t	err;
	packet		*pkt=NULL;
	uchar		wordcount = 24;
	ushort		bytecount;
	
	ulong		DesiredAccess = 0;
	ulong		ShareAccess = 0;
	ulong 		ExtFileAttributes = 0;
	ulong		CreateDisposition = 0;
	
	char *name=NULL;
	
	DPRINTF(OPEN_FILE_DEBUG, ("send_open_file entering\n"));
	
	err = get_full_name(vol, node, NULL, & name);
	if (err != B_OK)
		goto exit1;

	bytecount = strlen(name) + 1;
	
	// CreateDisposition
	// Note  - these are guesses.  These values are not
	// documented anywhere I could find.  Whee.
	if (omode & O_TRUNC) {
		CreateDisposition |= 0x4;
		DPRINTF(OPEN_FILE_DEBUG, ("CreateDisposition is %x, trying to truncat\n"));
	} else {
		CreateDisposition |= 0x1;
		DPRINTF(OPEN_FILE_DEBUG, ("CreateDisposition is %x, _not_ trying to truncat\n"));
	}
	// Here's what I got from sniffing:
	// 0x0 Supersede: If exist, Supersede, else create
	// 0x1 Open: if exist, open, else fail
	// 0x2 "If exist fail, else fail create the file"
	// 0x3 Open If: If exist, open, else create it
	// 0x4 Overwrite: If exist, open and overwrite, else fail
	

	// ShareAccess
	ShareAccess = 0x7;  // Anybody else can grab the file
		
	// File Attributes
	ExtFileAttributes = 0x080; // Normal file, ie, not system, hidden, or directory
	
	// DesiredAccess
	// Detailed in Sec 3.8 of the draft spec.
	DPRINTF(OPEN_FILE_DEBUG, ("omode is %x\n", omode));
	if (omode & O_WRONLY)
		DesiredAccess |= 0x2;
	if (omode & O_RDWR)
		DesiredAccess |= 0x3;
	if (omode == O_RDONLY)
		DesiredAccess = 0x1;
	
	
	err = new_smb_packet(vol, & pkt, SMB_COM_NT_CREATE_ANDX);
	if (err != B_OK) goto exit2;


	insert_uchar(pkt,	wordcount);
	insert_uchar(pkt,	0xff);//andxcommand
	insert_uchar(pkt,	0);//mbz
	insert_ushort(pkt,	0);//andxoffset
	insert_uchar(pkt,	0);//mbz
	insert_ushort(pkt,	bytecount);//namelength
	insert_ulong(pkt, 	0); //XXX Flags - set the oplock status.  '0' means don't lock
	insert_ulong(pkt, 	0); //rootdirectoryfid
	insert_ulong(pkt, 	DesiredAccess);
	insert_uint64(pkt,	0);//allocationsize
	insert_ulong(pkt, 	ExtFileAttributes);
	insert_ulong(pkt,	ShareAccess);
	insert_ulong(pkt,	CreateDisposition);
	insert_ulong(pkt,	0);//create options
	insert_ulong(pkt,	0);// Impersonation Level (?)
	insert_uchar(pkt,	0);// SecurityFlags
	
	insert_ushort(pkt, 	bytecount);
	insert_string(pkt,	name);
	

	err = SendSMBPacket(vol, pkt);
	
	
	free_packet(& pkt);
exit2:	
	FREE(name);	
exit1:	
	DPRINTF(OPEN_FILE_DEBUG, ("send_open_file exiting : %s\n", strerror(err)));
	return err;
}


status_t
nt_recv_open_file(nspace *vol, vnode *node, filecookie *cookie) {

	status_t	result;
	packet	*pkt=NULL;
	uint64		big_time;
	const uint64	mickysoft_sucks_shit = 11644473600;
	// This aptly named var is the number of seconds between
	// Jan 1st 1601 and Jan 1st 1970
	

	DPRINTF(OPEN_FILE_DEBUG, ("recv_open_file entering \n"));
	

	result = RecvSMBPacket(vol, & pkt, SMB_COM_NT_CREATE_ANDX);
	if (result != B_OK) return result;
	
	skip_over_remove(pkt, 6);
	remove_ushort(pkt, & cookie->fid);
	skip_over_remove(pkt, 4);// Skip CreateAction
	
	// Creation Time
	remove_uint64(pkt, & big_time);
	big_time *= 1.0e-7;
	big_time -= mickysoft_sucks_shit;
	node->crtime = big_time;
	
	// Access Time
	remove_uint64(pkt, & big_time);
	big_time *= 1.0e-7;
	big_time -= mickysoft_sucks_shit;
	node->atime = big_time;
	
	// Modification Time
	remove_uint64(pkt, & big_time);
	big_time *= 1.0e-7;
	big_time -= mickysoft_sucks_shit;
	node->mtime = big_time;
		
	// Change Time
	remove_uint64(pkt, & big_time);
	big_time *= 1.0e-7;
	big_time -= mickysoft_sucks_shit;
	node->ctime = big_time;
	
	skip_over_remove(pkt, 12);
	remove_int64(pkt, & node->size);
	

exit2:
	free_packet(& pkt);
exit1:
	DPRINTF(OPEN_FILE_DEBUG, ("recv_open_file exiting with result %d \n", result));
	return result;
}
	
status_t
nt_open_file(nspace *vol, vnode *node, int omode, filecookie *cookie) {

	status_t result;

	DPRINTF(OPEN_FILE_DEBUG, ("open_file entering on file %s\n", node->name));
	
	DPRINTF(OPEN_FILE_DEBUG, ("using new_send_open\n"));
	result = nt_send_open_file(vol, node, omode);
	if (result != B_OK) {
		return result;
	}
	
	result = nt_recv_open_file(vol, node, cookie);

	DPRINTF(OPEN_FILE_DEBUG, ("open_file exiting : %d %s\n", result, strerror(result)));
	return result;
	
}




status_t
close_file(nspace *vol, vnode *node, filecookie *cookie) {


	status_t	result;
	packet		*pkt=NULL;
	packet		*recvpkt=NULL;

	uchar			wordcount = 3;
	ushort			bytecount = 0;
	
	result = new_smb_packet(vol, & pkt, SMB_COM_CLOSE);
	if (result != B_OK) return result;

	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, cookie->fid);
	insert_ulong(pkt, real_time_clock());
	insert_ushort(pkt, bytecount);
	

	result = SendSMBPacket(vol, pkt);
	if (result != B_OK) goto exit1;


	result = RecvSMBPacket(vol, & recvpkt, SMB_COM_CLOSE);
	if (result != B_OK) goto exit1;

exit2:
	free_packet(&recvpkt);
exit1:
	free_packet(&pkt);
	DPRINTF(11, ("close_file exiting with result %d\n", result));
	return result;
	
}

status_t
tree_disconnect(nspace* vol)
{
	status_t	err;
	packet		*pkt=NULL;
	packet		*recvpkt=NULL;

	uchar			wordcount = 0;
	ushort			bytecount = 0;
	
	err = new_smb_packet(vol, & pkt, SMB_COM_TREE_DISCONNECT);
	if (err != B_OK) return err;

	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, bytecount);

	err = put_smb_header(vol, pkt);
	if (err != B_OK) goto exit1;	

	err = SendNBPacket(vol, pkt, false);
	if (err != B_OK) goto exit1;


	err = RecvSMBPacket(vol, & recvpkt, SMB_COM_TREE_DISCONNECT);
	if (err != B_OK) goto exit2;

	free_packet(&recvpkt);
	
exit2:
	free_packet(&pkt);
exit1:
	if (err != B_OK)
		debug(-1, ("tree_disconnect exiting with result %s\n", strerror(err)));
	return err;
}


status_t new_send_open_file(nspace *vol, vnode *node, int omode);
status_t new_recv_open_file(nspace *vol, vnode *node, filecookie *cookie);

status_t
new_open_file(nspace *vol, vnode *node, int omode, filecookie *cookie) {


	status_t result = B_ERROR;

	DPRINTF(OPEN_FILE_DEBUG, ("NEW_open_file entering on file %s\n", node->name));
	
	result = new_send_open_file(vol, node, omode);
	if (result != B_OK) {
		dprintf("new_send_open_file failed with %d\n", result);
		return result;
	}
	
	result = new_recv_open_file(vol, node, cookie);

	DPRINTF(OPEN_FILE_DEBUG, ("open_file exitig with result %d\n", result));
	return result;

}


status_t
new_send_open_file(nspace *vol, vnode *node, int omode)
{
	status_t err;
	packet *pkt = NULL;
	char *name = NULL;
	
		
	ushort desiredaccess = 0;
	ushort searchattributes = 0;
	ushort fileattributes = 0;
	ushort openfunction = 0;
	ushort bytecount = 0;
	
	err = get_full_name(vol, node, NULL, & name);
	if (err != B_OK) goto exit1;

	err = new_smb_packet(vol, & pkt, SMB_COM_OPEN_ANDX);	
	if (err != B_OK) goto exit2;
	
	insert_uchar(pkt, 15); // wordcount
	insert_uchar(pkt, 0xff); // no andxcommand
	insert_uchar(pkt, 0); // mbz
	insert_ushort(pkt, 0); // offset to next command wordcount

	insert_ushort(pkt, 0); // no locking requested

	if (omode & O_RDWR)
		desiredaccess = 0x2;
	if (omode & O_WRONLY) {
		desiredaccess = 0x1;
	}
	if (omode == O_RDONLY)
		desiredaccess = 0x0;
	desiredaccess |= 0x40; // Use read-ahead and write-behind.
	insert_ushort(pkt, desiredaccess);

	searchattributes = FILE_ATTR_HIDDEN | \
						FILE_ATTR_SYSTEM; // find them all
	insert_ushort(pkt, searchattributes);

	fileattributes = searchattributes;  // yes
	insert_ushort(pkt, fileattributes);
	insert_ulong(pkt, 0); // dont give a creation time,
							// let the server do it
	
	if (omode & O_CREAT){
		openfunction |= 16;
	}
	if (! (omode & O_EXCL)) {
		openfunction |= 1;
	}				
	if (omode & O_TRUNC) {
		openfunction |= 2;
		openfunction &= ~1;
	}
	insert_ushort(pkt, openfunction);

	insert_ulong(pkt, 0); // allocation size
	insert_ulong(pkt, -1); // open timeout 
	insert_ulong(pkt, 0); // mbz

	bytecount = strlen(name) + 1;
	insert_ushort(pkt, bytecount);
	//insert_uchar(pkt, 4); // buffer format : data
	insert_string(pkt, name);
	
	err = SendSMBPacket(vol, pkt);
	
	free_packet(& pkt);
exit2:
	FREE(name);
exit1:	
	return err;	
}

status_t
new_recv_open_file(nspace *vol, vnode *node, filecookie *cookie) {


	status_t result;
	packet *pkt = NULL;
	
	uchar wordcount;
	uchar dumbuchar;
	ushort fid;
	ushort fileattributes;
	unsigned int dumbulong;
	unsigned int datasize;
	ushort grantedaccess;
	ushort dumbushort;
	ushort action;
	
	result = RecvSMBPacket(vol, & pkt, SMB_COM_OPEN_ANDX);
	if (result != B_OK) {
		DPRINTF(-1, ("new_recv_open : RecvNBPacket returning %d:%s\n", result, strerror(result)));
		return result;
	}
	
	
	remove_uchar(pkt, & wordcount);
	if (wordcount != 15) {
		return B_ERROR;
	}
	
	
	remove_uchar(pkt, & dumbuchar); //andxcommand
	remove_uchar(pkt, & dumbuchar); //andxreserved
	remove_ushort(pkt, & dumbushort);  //andxoffset
	remove_ushort(pkt, & cookie->fid);
	remove_ushort(pkt, & fileattributes);
	remove_ulong(pkt, & dumbulong);
	remove_ulong(pkt, & datasize);
	remove_ushort(pkt, & grantedaccess);
	remove_ushort(pkt, & dumbushort); // filetype
	remove_ushort(pkt, & dumbushort); // devicestate
	remove_ushort(pkt, & action);
	
	// Don't need to pull off anything else.


	free_packet(& pkt);
	return B_OK;

}





status_t
do_delete(nspace *vol, const char *fullname, int is_dir)
{
	status_t	err;
	packet		*pkt=NULL;
	packet		*recvpkt=NULL;	
		
	uchar		wordcount;
	ushort		bytecount = 1 + strlen(fullname) + 1;
	

	if (is_dir) {
		wordcount = 0;
		err = new_smb_packet(vol, & pkt, SMB_COM_DELETE_DIRECTORY);
	} else {
		wordcount = 1;
		err = new_smb_packet(vol, & pkt, SMB_COM_DELETE);
	}
	if (err != B_OK) goto exit1;

	
	
	insert_uchar(pkt, wordcount);
	
	if (! is_dir)
		insert_ushort(pkt, (FILE_ATTR_HIDDEN)); // FileAttributes
	
	insert_ushort(pkt, bytecount);
	insert_uchar(pkt, 0x04); // BufferFormat
	insert_string(pkt, fullname);
	

	err = SendSMBPacket(vol, pkt);
	if (err != B_OK) goto exit1;
	
	if (is_dir) {
		err = RecvSMBPacket(vol, & recvpkt, SMB_COM_DELETE_DIRECTORY);
	} else {
		err = RecvSMBPacket(vol, & recvpkt, SMB_COM_DELETE);
	}

	
	if (err == B_OK)
		free_packet( & recvpkt);

exit2:
	free_packet(& pkt);
exit1:
	return err;
}


/* If you call this as
	get_full_name(vol, file, NULL, &where);
	You should get the full_name of file.
*/

#define DBG_GET_FULL_NAME 0
status_t
get_full_name(nspace *vol, vnode *dir, const char *name, char **val)
{
	status_t err;
	vnode **vnplist=NULL;
	vnode *vnp;
	vnode_id vnid;
	int strsize,namesize,i,count;
	int guess = 64;  // Big enough that we shouldnt ever realloc
	const int SAFETYMARGIN = 32;
	char *p;


	debug(DBG_GET_FULL_NAME, ("get_full_name :  %s ", dir->name));
	if (name == NULL) {
		debug(DBG_GET_FULL_NAME, ("NULL\n"));
	} else {
		debug(DBG_GET_FULL_NAME, ("%s\n", name));
	}

	namesize = (name!=NULL)?strlen(name):0;
	strsize = 1 + strlen(dir->name) + 1 + \
				namesize + 1 + SAFETYMARGIN;
	
	// Short cut
	if (dir->vnid == vol->root_vnode.vnid) {
		MALLOC((*val), char*, strsize);
		if (*val == NULL)
			return ENOMEM;
		p = *val;
		*p++ = '\\';
		if (name != NULL) {
			memcpy(p, name, namesize);
			p += namesize;
		}
		*p=0;
		debug(DBG_GET_FULL_NAME, ("\t returning %s\n", *val));
		return B_OK;
	}

	MALLOC((vnplist), vnode**, (guess*sizeof(vnode*)));
	if (vnplist == NULL) {
		debug(-1, ("nomem in get_full_name\n"));
		return ENOMEM;
	}

	for (count=0, i=0, vnid = dir->parent_vnid; vnid != vol->root_vnode.vnid;
			vnid = (vnplist[i]->parent_vnid), i++) {
		if (count>guess) { // Need more mem
			vnplist = realloc(vnplist, (2*count)*sizeof(vnode*));
			if (vnplist == NULL) {
				dprintf("cifs : no mem in critical section of get_full_name\n");
				err = ENOMEM;
				goto error3;
			}
			guess = 2*count;
		}

		err = get_vnode(vol->nsid, vnid, (void**)&(vnplist[i]));
		if (err != B_OK) {
			dprintf("cifs : couldnt get_vnode in get_full_name\n");
			goto error3;
		}
		count++;
		strsize += 1 + strlen((vnplist[i])->name);
	}

	MALLOC((*val), char*, strsize);
	if (*val == NULL) {
		debug(DBG_GET_FULL_NAME,("nomem for *val\n"));
		err = ENOMEM;
		goto error3;
	}

	p = *val;
	for (;count>0;count--){
		vnp = (vnplist[count-1]);
		*p++ = '\\';
		strcpy(p, vnp->name);
		p += strlen(vnp->name);
		put_vnode(vol->nsid, (vnplist[count-1])->vnid);
	}		

	//Now put dir->name and name
	*p++ = '\\'; strcpy(p, dir->name); p += strlen(dir->name);
	if (namesize != 0) {
		*p++ = '\\'; memcpy(p, name, namesize); p += namesize;
	}
	*p=0;  //ensure null
	debug(DBG_GET_FULL_NAME, ("\t returning %s\n", *val));

	FREE(vnplist);
	return B_OK;



error3:
	for(;count>0;count--)
		put_vnode(vol->nsid, (vnplist[count-1])->vnid);
error2:
	FREE(vnplist);
error1:
	debug(DBG_GET_FULL_NAME, ("\t returning %s\n", *val));
	return err;
}



status_t
new_filecookie(filecookie **fc) {

	if (fc == NULL)
		return B_ERROR;
	
	MALLOC((*fc), filecookie*, sizeof(filecookie));
	if (*fc == NULL) return ENOMEM;
	
	(*fc)->magic = CIFS_FILE_COOKIE;
	(*fc)->omode = -1;
	(*fc)->fid 	 = 0;
	(*fc)->cache.pos = -1;
	(*fc)->cache.len = -1;
	(*fc)->cache.buf = NULL;

	return B_OK;
}

void
del_filecookie(filecookie *fc) {

	if (fc == NULL) {
		DPRINTF(-1,("del_filecookie got NULL cookie\n"));
		return;
	}
	
	if (fc->magic != CIFS_FILE_COOKIE) {
		debug(-1,("del_filecookie got a non-file cookie!\n"));
		FREE(fc);
		return;
	}

	fc->omode = -1;
	fc->fid  = 0;
	fc->cache.pos = -1;
	fc->cache.len = -1;

	if (fc->cache.buf != NULL) {
		FREE(fc->cache.buf);
		fc->cache.buf = NULL;
	}
	
	FREE(fc);	

}




