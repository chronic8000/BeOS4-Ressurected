#include "cifs_globals.h"
#include "netio.h"
#include "packet.h"
	
// Write Commands

#define CIFS_WRITE_DBG 10

status_t
send_write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf,
			size_t *len, filecookie *cookie) {

	status_t	result;
	packet		*pkt=NULL;

	uchar		wordcount = 14;
	ulong		OffsetLow;
	ulong		OffsetHigh;
	ushort		bytecount = *len + 3;
		
	DPRINTF(CIFS_WRITE_DBG, ("send_write_to_file entering\n"));
	DPRINTF(CIFS_WRITE_DBG, ("want to write %d (dec) bytes at pos %x\n", *len, pos));
	
	result = new_smb_packet(vol, & pkt, SMB_COM_WRITE_ANDX);
	if (result != B_OK) goto exit1;
	
	OffsetLow  = (ulong) (pos & 0x00000000FFFFFFFF);
	OffsetHigh = (ulong) ((pos & 0xFFFFFFFF00000000) >> 32);
	
	insert_uchar(pkt,	wordcount);
	insert_uchar(pkt,	0xff);
	insert_uchar(pkt,	0);
	insert_ushort(pkt, 	0);
	insert_ushort(pkt,	cookie->fid);
	insert_ulong(pkt,	OffsetLow);
	insert_ulong(pkt, 	0);// mbz;
	insert_ushort(pkt,	0);// xxx WriteMode
	insert_ushort(pkt,	0);// Remaining
	insert_ushort(pkt,	0);// DataLengthHigh, we don't support transfers > 2^16 (yet)
	insert_ushort(pkt,	bytecount - 3);  // datalength
	insert_ushort(pkt, 	66);//offset to data in this packet
	insert_ulong(pkt,	OffsetHigh);
	insert_ushort(pkt,	bytecount);
	
	insert_uchar(pkt, 0); // pad
	insert_uchar(pkt, 0); // pad
	insert_uchar(pkt, 0); // pad
	insert_void(pkt, buf, *len);
	
	result = SendSMBPacket(vol, pkt);
	

	free_packet(& pkt);
exit1:
	DPRINTF(CIFS_WRITE_DBG, ("send_write_to_file exiting with result %d\n", result));
	return result;
}
	
status_t
recv_write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf, size_t *len) {

	status_t	result;
	packet		*pkt=NULL;
	
	
	DPRINTF(CIFS_WRITE_DBG, ("recv_write_to_file entering \n"));
	
	result = RecvSMBPacket(vol, & pkt, SMB_COM_WRITE_ANDX);
	if (result != B_OK) goto exit1;

	free_packet(& pkt);
exit1:
	DPRINTF(CIFS_WRITE_DBG, ("recv_write_to_file exiting with result %d \n", result));
	return result;
}	

status_t
small_write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf,
				size_t *len, filecookie *cookie) {

	status_t	result;
	
	
	DPRINTF(CIFS_WRITE_DBG, ("write_to_file   entering\n"));
	result = send_write_to_file(vol, node, pos, buf, len, cookie);
	if (result != B_OK) {
		DPRINTF(-1, ("error on send_write_to_file\n"));
		return result;
	}
	
	result = recv_write_to_file(vol, node, pos, buf, len);
	if (result != B_OK) {
		DPRINTF(-1, ("ERROR on recv_write_to_file\n"));
		return result;
	}
	
	
	DPRINTF(CIFS_WRITE_DBG, ("write_to_file   exiting\n"));
	return result;
}



status_t
send_large_write_req(nspace *vol, vnode *node, off_t pos, const void *buf,
							size_t *len, filecookie *cookie) {

	status_t	result;
	packet		*pkt=NULL;
	uchar		wordcount = 14;
	ulong		OffsetLow;
	ulong		OffsetHigh;
	ushort		bytecount = 0;

	ushort	dlength;	

	
	//dprintf("send_large_write_req entering\n");
	//dprintf("want to write %d (dec) bytes at pos %x\n", *len, pos);

	result = new_smb_packet(vol, & pkt, SMB_COM_WRITE_RAW);
	if (result != B_OK) return result;

	OffsetLow  = (ulong) (pos & 0x00000000FFFFFFFF);
	OffsetHigh = (ulong) ((pos & 0xFFFFFFFF00000000) >> 32);
	
	insert_uchar(pkt,	wordcount);
	insert_ushort(pkt,	cookie->fid);

	insert_ushort(pkt,	(ushort)*len);
	insert_ushort(pkt,	0);//mbz
	insert_ulong(pkt,	OffsetLow);
	insert_ulong(pkt,	0);//timeout
	insert_ushort(pkt,	0);
	insert_ulong(pkt,	0);//mbz
	insert_ushort(pkt,	0);//DataLength (this packet)
	insert_ushort(pkt,	0);//DataOffset (this packet)
	insert_ulong(pkt,	OffsetHigh);
	insert_ushort(pkt,	bytecount);
	
	result = SendSMBPacket(vol, pkt);
	
	free_packet(& pkt);

	DPRINTF(CIFS_WRITE_DBG, ("send_large_write_req exiting with result %d\n", result));
	return result;
}


status_t
recv_large_write_req(nspace *vol) {

	status_t	result;
	packet		*pkt=NULL;
	
	
	DPRINTF(CIFS_WRITE_DBG, ("recv_large_write_req entering \n"));
	
	result = RecvSMBPacket(vol, & pkt, SMB_COM_WRITE_RAW);
	if (result != B_OK) goto exit1;

	free_packet(& pkt);
exit1:
	DPRINTF(CIFS_WRITE_DBG, ("recv_large_write_req exiting with result %d %s\n", result, strerror(result)));
	return result;
}	

	


status_t
send_large_write(nspace *vol, const void* buf, size_t *len)
{
	status_t	err;
	packet		*pkt=NULL;
	

	err = init_packet( & pkt, (int32)(*len+4));
	if (err != B_OK) return err;
	
	pkt->type = PACKET_SMB_TYPE;
	put_nb_header(pkt, *len);
	pkt->addpos += 4;

	// ugh - no better way to do this till we get
	// real iovec sends.  sigh.	
	err = insert_void(pkt, buf, *len);
	if (err != B_OK) goto exit1;

	err = SendNBPacket(vol, pkt, false);

exit1:
	free_packet(& pkt);
	return err;
}

status_t
large_write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf,
							size_t *len, filecookie *cookie) {

	status_t	result;
	
	DPRINTF(CIFS_WRITE_DBG, ("LARGE write_to_file entering, sending %d (dec) bytes\n", *len));
	
	result = send_large_write_req(vol, node, pos, buf, len, cookie);
	if (result != B_OK) {
		dprintf("ERROR on send_large_write_req\n");
		return result;
	}
	
	DPRINTF(CIFS_WRITE_DBG, ("LARGE write_to_file 2\n"));
	result = recv_large_write_req(vol);
	if (result != B_OK) {
		dprintf("ERROR on recv_large_write_req\n");
		return result;
	}

	DPRINTF(CIFS_WRITE_DBG, ("LARGE write_to_file 3\n"));
	result = send_large_write(vol, buf, len);
	if (result != B_OK) {
		dprintf("ERROR on do_large_write\n");
		return result;
	}


	DPRINTF(CIFS_WRITE_DBG, ("LARGE write_to_file exiting\n"));
	return result;
}


status_t
write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf,
				size_t *len, filecookie *cookie) {
				
	status_t	result;
	uchar		*cur_bufp;
	off_t		cur_pos;
	size_t		rem,len_req,howmuch;
	
	len_req = *len;

	cur_bufp = (uchar*) buf;
	cur_pos = pos;
	rem = len_req;
	
	while (rem > 0) {
			
		howmuch = get_io_size(vol,rem);
		if (howmuch <= vol->small_io_size) {
			result = small_write_to_file(vol, node, cur_pos, cur_bufp, & howmuch, cookie);
			if (result != B_OK) {
				return result;
			}
			rem -= howmuch;
			cur_bufp += howmuch;
			cur_pos += howmuch;
			continue;
		}
		
		result = large_write_to_file(vol, node, cur_pos, cur_bufp, & howmuch, cookie);
		if (result != B_OK) {
			return result;
		}
			
		rem -= howmuch;
		cur_bufp += howmuch;
		cur_pos += howmuch;

	}
	
	return result;

}







#if 0	
	uchar_to_ptr(& hldr, WordCount);
	uchar_to_ptr(& hldr, 0xFF); // AndXCommand
	uchar_to_ptr(& hldr, 0); // AndXReserved
	ushort_to_ptr(& hldr, 0); // AndXOffset
	ushort_to_ptr(& hldr, cookie->fid);
	
	OffsetLow = (ulong) (pos & 0x00000000FFFFFFFF);
	OffsetHigh = (ulong) (pos & 0xFFFFFFFF00000000);
	DPRINTF(CIFS_WRITE_DBG, ("pos stuff: pos is %Lx, low is %x, high is %x\n", pos, OffsetLow, OffsetHigh));
	ulong_to_ptr(& hldr, OffsetLow);

	ulong_to_ptr(& hldr, 0); // mbz;
	ushort_to_ptr(& hldr, 0); // xxx WriteMode
	ushort_to_ptr(& hldr, 0); // Remaining
	ushort_to_ptr(& hldr, 0); // DataLengthHigh, we don't support transfers > 2^16 (yet)
	ushort_to_ptr(& hldr, ByteCount);
	
	ushort_to_ptr(& hldr, (COMMON_HEADER_SIZE + 2*WordCount + 1 + 2));
	ulong_to_ptr(& hldr, OffsetHigh);
	ushort_to_ptr(& hldr, ByteCount);
	
	DPRINTF(CIFS_WRITE_DBG, (" forming nb packet\n"));

	nb_header.Command = NB_SESSION_MESSAGE;
	nb_header.Flags = 0;
	put_nb_length(& nb_header, COMMON_HEADER_SIZE + 2*WordCount + ByteCount + 3);
	
	vector[0].iov_base = (char *) & nb_header;
	vector[0].iov_len  = sizeof(nb_header_t);
	vector[1].iov_base = (char *) packet_buf;
	vector[1].iov_len  = COMMON_HEADER_SIZE + 2*WordCount + 3;
	vector[2].iov_base = (char *) buf;
	vector[2].iov_len = ByteCount;
	
	
	if ((result = fake_sendv(vol->sockid, vector, 3)) != B_OK) {
		DPRINTF(CIFS_WRITE_DBG, ("send_write_to_file failed in fake_sendv\n"));
		goto exit1;
	}
	
#endif
