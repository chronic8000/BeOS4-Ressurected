#include "cifs_globals.h"
#include "packet.h"
#include "mount.h"

#define CIFS_NETIO_DBG 10000

status_t
RestartNetworkConnection(nspace *vol)
{
	status_t err;
	
	vol->connection_retries++;
//	if (vol->connection_retries < CIFS_MAX_RETRIES) {
	KillNetworkConnection(vol);
	err = InitNetworkConnection(vol);
	if (err != B_OK) {
		debug(CIFS_NETIO_DBG, ("RestartNetworkConnection Failed, retries at %d\n", vol->connection_retries));
		KillNetworkConnection(vol);
		return err;
	} else {
		debug(CIFS_NETIO_DBG, ("RestartNetworkConnection Successful\n"));
		vol->connection_retries = 0;
		return B_OK;
	}
//	} else {
//		return B_ERROR;
//	}		

	//notreached
	return B_ERROR;
}

status_t
CheckConnection(nspace *vol)
{
	return (vol->connect_state & TREE_INIT) ? B_OK : B_ERROR;
}

status_t
put_smb_header(nspace *vol, packet *pkt)
{
	uchar *hldr;
	int i;
	
	if ( (pkt==NULL) || (pkt->size<CommonHeader) ||
			(pkt->buf==NULL) || (pkt->type!=PACKET_SMB_TYPE)) {
		return B_ERROR;				
	}
		
	hldr = pkt->buf;

	memset(hldr, 0, CommonHeader);

	hldr += 4; // Skip the netbios header.	
	uchar_to_ptr(& hldr, 0xFF);
	uchar_to_ptr(& hldr, 'S');
	uchar_to_ptr(& hldr, 'M');
	uchar_to_ptr(& hldr, 'B');
	uchar_to_ptr(& hldr, (uchar) pkt->command);
	hldr += 4; // skip Status Field
	uchar_to_ptr(& hldr, vol->Flags);
	ushort_to_ptr(& hldr, vol->Flags2);
	hldr += 12;  // skip pad field
	ushort_to_ptr(& hldr, vol->Tid);
	ushort_to_ptr(& hldr, vol->Pid);
	ushort_to_ptr(& hldr, vol->Uid);
	ushort_to_ptr(& hldr, vol->Mid);

	return B_OK;
}

status_t
new_smb_packet(nspace *vol, packet **_pkt, uchar command)
{
	status_t err;
	packet *pkt=NULL;
	
	err = init_packet(_pkt, -1);
	if (err != B_OK) return err;

	pkt = *_pkt;
	pkt->type = PACKET_SMB_TYPE;
	pkt->command = command;
	
	// Save room for the netbios and smb header.
	err = skip_over(pkt, CommonHeader);
	if (err != B_OK) return err;

	// Set byte order to little endian.	
	set_order(pkt, false);

	return err;
}


status_t
put_nb_header(packet *pkt, size_t size) {

	uchar *p;
	uint16 tmp;

	if (pkt == NULL) return B_ERROR;
	if (pkt->size < CommonHeader) return B_ERROR;
	if (pkt->buf == NULL) return B_ERROR;
	if (pkt->type != PACKET_SMB_TYPE) return B_ERROR;
	if (size > 131071) return E2BIG;  // you're too big, baby!
	
	p = pkt->buf;	
	p[0] = NB_SESSION_MESSAGE;
	if (size > 65535) {
		p[1] = 1;
	} else {
		p[1] = 0;
	}
	tmp = B_HOST_TO_BENDIAN_INT16((uint16) (size % 65536));
	memcpy(&p[2], & tmp, 2);
	
	return B_OK;
}
 

status_t
isDataPending(int socket, long timeout) {

	status_t result;
	fd_set fd;
	struct timeval tv;
	uint32 before, after;

	if (timeout > 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		//dprintf("Waiting for %d.%d secs\n", tv.tv_sec, tv.tv_usec);
    }
    

    FD_ZERO(&fd);
   	FD_SET(socket, &fd);
    	
	result = KSELECT(socket + 1, &fd, NULL, NULL, &tv);
	if (result == 1) {	
		if (FD_ISSET(socket, &fd)) {
		    return B_OK;  // Dataispending
		} else {
			// I'm not sure if this is my bug or the net_servers.
			dprintf("isdatapending: select returned 1, but fd_isset reports false\n");
			return B_OK;
		}
	} else if (result == 0) {
		debug(-1, ("select timed out\n"));
		return B_TIMED_OUT;  // nodata pending
   	} else {
		DPRINTF(-1, ("exiting isDataPending result is %d %s\n", result, strerror(result)));
		return B_IO_ERROR; // error
	}
	
	//not reached
	dprintf("PANIC : unreachable reached in isDataPending\n");
	return B_ERROR;			
}

status_t
RecvNBPacket_nocheck(nspace *vol, packet **_pkt, long timeout) {

	status_t		err;
	nb_header_t		nb_head;
	
	int	length;
	uchar flag;
	
	uchar *buf;
	int32 total;
	int32 recvlen;
		
	err = isDataPending(vol->sockid, timeout);
	
	if (err != B_OK) {// Unless there's data pending
		debug(CIFS_NETIO_DBG, ("RecvNBPacket_nocheck: nodatapending :%s\n",strerror(err)));
		return err;
	}	

	do {  // Soak up all the keepalives
		if ((err = KRECV(vol->sockid, & nb_head, 4, 0)) < 4) {
			debug(CIFS_NETIO_DBG, ("RecvNBPacket_nocheck failed in recv NB header, returning %s\n",strerror(err)));
			return err;
		}
	} while (nb_head.Command == NB_SESSION_KEEPALIVE);


	if ((nb_head.Command != NB_SESSION_MESSAGE) ||
		(nb_head.Flags > 1)) {
		
		debug(CIFS_NETIO_DBG, ("RecvNBPacket_nocheck got malformed netbios header, returning B_IO_ERROR\n"));
		return B_IO_ERROR;
	}

	length = B_BENDIAN_TO_HOST_INT16(nb_head.Length);
	flag = nb_head.Flags;
	if (flag) length += 65536;


	// This call to init_packet means that you cant call
	// sendnbpacket on a packet, then call recvnbpacket on it.
	// The reason is that init_packet currently checks and makes
	// sure its handed a null packet before it tries to init it.
	// So, you have to make a new packet between send and recv's.
	err = init_packet(_pkt, length);
	if (err != B_OK) {
		dprintf("cifs PANIC : init_packet failed in RecvNBPacket_nocheck, on length %d\n", length);
		return err;
	}
	// Also note that the only other time init_packet fails is
	// if it cant get enough memory for the packet.  Which
	// means that if you just report ENOMEM, then you've still
	// part of the packet in the socket.  What do you do then?
	//
	// Current idea : if you just got an ENOMEM from inside the
	// kernel, you're pretty much screwed anyway.  So just
	// return the error, in case anyone is still running to
	// pick up the error.


	buf = (*_pkt)->buf;
	total = 0;
	while (total != length) {
		recvlen = KRECV(vol->sockid, buf, (length - total), 0);
		if (recvlen <= 0) {
			debug(CIFS_NETIO_DBG, ("RecvNBPacket_nocheck failed recving packet returning %s\n",strerror(recvlen)));
			free_packet(_pkt);
			return B_IO_ERROR;
		}
		total += recvlen;
		buf += recvlen;
	}
	
	(*_pkt)->addpos = length;

	return B_OK;
}

status_t
RecvNBPacket(nspace *vol, packet **_pkt, long timeout)
{
	status_t err;

	if(CheckConnection(vol) != B_OK) {
		err = RestartNetworkConnection(vol);
		if (err != B_OK) {
			debug(-1, ("RecvNBPacket : connection is down\n"));
			return err;
		}
	}	
	return RecvNBPacket_nocheck(vol, _pkt, timeout);
}

status_t
SendNBPacket(nspace *vol, packet *pkt, bool retry) {

	status_t	result;
	size_t		size;
	int8		retries = 0;
	
	
	if ((pkt == NULL) ||
		(pkt->size < CommonHeader) ||
		(pkt->buf == NULL) ||
		(pkt->type != PACKET_SMB_TYPE)) {
	
		dprintf("SendNBPacket failing on stupid error : pkt 0x%x, size %d, buf 0x%x, type %d\n",
			pkt, pkt->size, pkt->buf, pkt->type);	
		return B_ERROR;
	}

	
	size = getSize(pkt) - 4;
	// the packet has space for the 4 byte netbios header.
	
	result = put_nb_header(pkt, size);
	if (result != B_OK) return result;

	while (retries < 2) {
		result = KSEND(vol->sockid, pkt->buf, (size+4), 0);
		if (result != (size+4)) {
			if (retry) {
				debug(-1,("Error in SendNBPacket: tried to send %d\n", (size+4)));
				debug(-1,("result is %d %s\n", result, strerror(result)));
				debug(-1,("restarting connection...\n"));
				result = RestartNetworkConnection(vol);
				if (result != B_OK) {
					debug(-1, ("SendNBPacket:InitNetworkConnection failed with %d %s\n", result, strerror(result)));
				}	
				retries++;
				continue;
			} else {
				return B_IO_ERROR;
			}
		} else {
			return B_OK;
		}
	}
	dprintf("SendNBPacket couldn't connect to server\n");
	return B_IO_ERROR;
}


status_t
SendSMBPacket(nspace *vol, packet *pkt)
{
	status_t err;

	if(CheckConnection(vol) != B_OK) {
		err = RestartNetworkConnection(vol);
		if (err != B_OK) {
			debug(-1, ("SendSMBPacket : connection is down\n"));
			return err;
		}
	}	

	err = put_smb_header(vol, pkt);
	if (err != B_OK) return err;
	
	return SendNBPacket(vol, pkt, true);
}

static volatile int32 cifs_doit_wrong = 0;

status_t
PopSMBHeader(nspace *vol, packet *pkt, int command, bool getIDs) {
	
	status_t result;
	char dumbchar;
	uchar dumbuchar;
	unsigned int dumbulong;
	ushort dumbushort;
	
	uchar 	errclass;
	ushort	errcode;
	
	set_order(pkt, false);
	
	//dump_packet(pkt);

	result = verify_size(pkt, CommonHeader - 4);
	// Can we pop off enough for a SMB header
	if (result != B_OK) {
		debug(-1, ("PopSMBHeader got runt packet\n"));
		return result;
	}
	
	remove_uchar(pkt, & dumbuchar);
//	if (dumbuchar != 0xff) {
	if ( (dumbuchar != 0xff) ||
		 (cifs_doit_wrong) ) {
		debug(CIFS_NETIO_DBG, ("PopSMBHeader ckpt 1 : 0x%2x\n", dumbuchar));
		//dump_packet(pkt);
		return B_ERROR;
	}
	remove_uchar(pkt, & dumbuchar);
	if (dumbuchar != 'S') {
		debug(CIFS_NETIO_DBG, ("PopSMBHeader ckpt 2: 0x%2x\n", dumbuchar));
//		dump_packet(pkt);
		return B_ERROR;
	}
	remove_uchar(pkt, & dumbuchar);
	if (dumbuchar != 'M') {
		debug(CIFS_NETIO_DBG, ("PopSMBHeader ckpt 3: 0x%2x\n", dumbuchar));
//		dump_packet(pkt);
		return B_ERROR;
	}		
	remove_uchar(pkt, & dumbuchar);
	if (dumbuchar != 'B') {
		debug(CIFS_NETIO_DBG, ("PopSMBHeader ckpt 4: 0x%2x\n", dumbuchar));
//		dump_packet(pkt);
		return B_ERROR;
	}		
	remove_uchar(pkt, & dumbuchar);
	if (dumbuchar != command) {
		debug(CIFS_NETIO_DBG, ("PopSMBHeader ckpt 5: 0x%2x when command was 0x%2x\n", dumbuchar, command));
//		dump_packet(pkt);
		return B_ERROR;
	}		

	remove_uchar(pkt, & errclass);
	remove_uchar(pkt, & dumbuchar);
	remove_ushort(pkt, & errcode);
	result = translate_error(errclass, errcode);
	if (result != B_OK) {
		//dump_packet(pkt);
		return result;
	}
	remove_uchar(pkt, & dumbuchar);
	remove_ushort(pkt, & dumbushort);
	remove_ulong(pkt, & dumbulong);
	remove_ulong(pkt, & dumbulong);
	remove_ulong(pkt, & dumbulong);

	if (getIDs) {
		/*
		dprintf("Storing ID's.  Old stuff is \n");
		dprintf("Tid %x\n", vol->Tid);
		dprintf("Pid %x\n", vol->Pid);
		dprintf("Uid %x\n", vol->Uid);
		dprintf("Mid %x\n", vol->Mid);
		*/
		remove_ushort(pkt, & vol->Tid);
		remove_ushort(pkt, & vol->Pid);
		remove_ushort(pkt, & vol->Uid);
		remove_ushort(pkt, & vol->Mid);
		/*
		dprintf("New stuff is \n");
		dprintf("Tid %x\n", vol->Tid);
		dprintf("Pid %x\n", vol->Pid);
		dprintf("Uid %x\n", vol->Uid);
		dprintf("Mid %x\n", vol->Mid);
		*/
	} else {
		remove_ushort(pkt, & dumbushort);
		remove_ushort(pkt, & dumbushort);
		remove_ushort(pkt, & dumbushort);
		remove_ushort(pkt, & dumbushort);
	}
	
	return B_OK;
}


//
// If this returns B_OK, the caller must call free_packet
// on _pkt, otherwise they'll leak.
// If this does not return B_OK, dont call free_packet
status_t
RecvSMBPacket(nspace *vol, packet **_pkt, int command) {

	status_t result;

	result = RecvNBPacket(vol, _pkt, (CIFS_RECV_TIMEOUT * 1000)); //timeout = 10 sec
	if (result != B_OK) {
		DPRINTF(CIFS_NETIO_DBG, ("RecvSMBPacket returning %d, %s\n", result, strerror(result)));
		return result;
	}
	
	result = PopSMBHeader(vol, *_pkt, command, false);
	if (result != B_OK) {
		DPRINTF(CIFS_NETIO_DBG, ("PopSMBHeader returning %d, %s\n", result, strerror(result)));
		free_packet(_pkt);
		return result;
	}

	set_order(*_pkt, false);
	return result;
}

	
status_t
RecvSMBPacket_nocheck(nspace *vol, packet **_pkt, int command) {

	status_t result;

	result = RecvNBPacket_nocheck(vol, _pkt, (CIFS_RECV_TIMEOUT * 1000)); //timeout = 10 sec
	if (result != B_OK) {
		debug(CIFS_NETIO_DBG, ("RecvSMBPacket returning %d, %s\n", result, strerror(result)));
		return result;
	}
	
	result = PopSMBHeader(vol, *_pkt, command, false);
	if (result != B_OK) {
		DPRINTF(CIFS_NETIO_DBG, ("PopSMBHeader returning %d, %s\n", result, strerror(result)));
		free_packet(_pkt);
		return result;
	}

	set_order(*_pkt, false);
	return result;
}
