#include "cifs_globals.h"
#include "cifs_utils.h"
#include "packet.h"
#include "netio.h"


#define DEBUG_READ_FILE 299

#if 0
	if (use_cache(vol, node, pos, len_req, cookie)) {
		result = read_from_cache(vol, node, pos, len_req, cookie, buf);
		if (result != B_OK) {
			DPRINTF(DEBUG_READ_FROM_FILE, ("read_from_cache failed : %d %s\n", result, strerror(result)));
		}	
	}

#endif

status_t send_large_read_from_file(nspace *vol, vnode *node, off_t pos, size_t len, filecookie *cookie);
status_t recv_large_read_from_file(nspace *vol, vnode *node, off_t pos, void *_buf, size_t *len);


status_t
free_cache(filecookie *cookie) {
	
	if (cookie->cache.buf == NULL) {
		dprintf("free_cache got a NULL cache buf\n");
		return B_ERROR;
	}
	cookie->cache.pos = -1;
	cookie->cache.len = -1;
	FREE(cookie->cache.buf);
	cookie->cache.buf = NULL;
	return B_OK;
}

status_t
load_cache(nspace *vol, vnode *node, off_t pos, filecookie *cookie) {
	
	status_t result;
	size_t	size;

	size = min(vol->io_size, node->size);
	
	//dprintf("loading cache pos %Lx size %d\n", pos, size);
	
	if (cookie->cache.buf != NULL) {
		dprintf("cookie->cache.buf isnt null\n");
		return B_ERROR;
	}
	MALLOC((cookie->cache.buf), uchar*, size);
	if (cookie->cache.buf == NULL) return ENOMEM;
	
	result = send_large_read_from_file(vol, node, pos, size, cookie);
	if (result != B_OK) {
		DPRINTF(-1, ("error on send_large_read_from_file\n"));
		goto exit1;
	}
	
	result = recv_large_read_from_file(vol, node, pos, cookie->cache.buf, & size);
	if (result != B_OK) {
		DPRINTF(-1, ("error on recv_large_read_from_file\n"));
		goto exit1;
	}		

#if 0
//hmm.
	if (size != min(vol->io_size, node->size)) {
		dprintf("erron sending : sent %Ld node->size is %Ld\n", size, node->size);
		goto exit1;
	}
#endif
	
	cookie->cache.pos = pos;
	cookie->cache.len = size;

	return B_OK;
	
exit1:
	free_cache(cookie);
	return result;
}

bool
within_cache(off_t pos, size_t len_req, filecookie *cookie) {

	off_t cpos = cookie->cache.pos;
	size_t clen = cookie->cache.len;
	
	if (pos < cpos)
		return false;
	
	if ((pos + len_req) > (cpos + clen))
		return false;
	
	return true;

}

// Expected : pos and len_req have been normalized to fit this file,and
//	len_req isnt 0.
bool
use_cache(nspace *vol, vnode *node, off_t pos, size_t len_req, filecookie *cookie) {

	status_t	result;

	//dprintf("use_cache : pos %Lx len_req %d\n", pos, len_req );
	
	if (! vol->cache_enabled)
		return false;
	
	if (len_req == 0)
		return false;
	
	if (len_req >= vol->io_size)
		return false;
	
	// Do we need to load the cache?
	if (cookie->cache.buf == NULL) {
		//dprintf("loading cache\n");
		result = load_cache(vol, node, pos, cookie);
		if (result != B_OK) return false;
	}

	// Does the request fit into our current cache?
	if (within_cache(pos, len_req, cookie)) {
		//dprintf("using cache 1\n");
		return true;	
	} else {
		// Free the current cache, reload, and return true;
		//dprintf("using cache 2\n");
		result = free_cache(cookie);
		if (result != B_OK) return false;
		result = load_cache(vol, node, pos, cookie);
		if (result != B_OK) return false;
		return true;
	}

}

status_t
read_from_cache(off_t pos, size_t len_req, filecookie *cookie, void *buf) {

	uchar *location;

	if ((len_req == 0) || (len_req >= 65535)) {
		dprintf("weird read_from_cache : len_req is %Ld\n", len_req);
		return B_ERROR;
	}
	
	if (cookie->cache.buf == NULL) {
		dprintf("null cache.buf at pos %Ld, len_req is %Ld\n", pos, len_req);
		return B_ERROR;
	}


	location = cookie->cache.buf + (pos - cookie->cache.pos);

	memcpy(buf, location, len_req);
	return B_OK;
}

status_t
send_large_read_from_file(nspace *vol, vnode *node, off_t pos,
						size_t len, filecookie *cookie) {

	status_t	result;
	packet		*pkt=NULL;

	uchar		wordcount = 10;
	ulong		OffsetLow;
	ulong		OffsetHigh;
	ushort		bytecount = 0;
	
	DPRINTF(DEBUG_READ_FILE, ("send_LARGE_read_from_file entering\n"));

	DPRINTF(DEBUG_READ_FILE, ("want to read %d(dec) bytes at pos %d(dec)\n", len, pos));

	result = new_smb_packet(vol, & pkt, SMB_COM_READ_RAW);
	if (result != B_OK) goto exit1;

	OffsetLow = (ulong) (pos & 0x00000000FFFFFFFF);
	OffsetHigh = (ulong) ((pos & 0xFFFFFFFF00000000) >> 32);
	
	insert_uchar(pkt,	wordcount);
	insert_ushort(pkt,	cookie->fid);
	insert_ulong(pkt,	OffsetLow);
	insert_ushort(pkt,	len);
	insert_ushort(pkt,	0);
	insert_ulong(pkt,	0);
	insert_ushort(pkt,	0);
	insert_ulong(pkt,	OffsetHigh);
	insert_ushort(pkt,	bytecount);
		

	result = SendSMBPacket(vol, pkt);
		
	free_packet(&pkt);
exit1:
	DPRINTF(DEBUG_READ_FILE, ("send_read_from_file exiting with result %d\n", result));
	return result;
}


status_t
recv_large_read_from_file(nspace *vol, vnode *node, off_t pos, void *buf, size_t *len)
{
	status_t	err;
	int			length;
	packet		*pkt=NULL;
	
	DPRINTF(DEBUG_READ_FILE, ("recv_large_read_from_file entering \n"));

	err = RecvNBPacket(vol, &pkt, (CIFS_RECV_TIMEOUT * 1000)); //timeout = 10 sec
	if (err != B_OK) {
		debug(-1,("recv_large_read_from_file returning error %d %s\n", err, strerror(err)));
		return err;
	}
	
	length = getSize(pkt);
	if (*len != length) {
		debug(DEBUG_READ_FILE, ("recv_large_read_from_file got %d bytes, expecting %d\n", length, length));
	}
	
	err = remove_void(pkt, buf, length);
	if (err != B_OK) {
		debug(-1, ("recv_large_read_from_file : weird error on remove_void\n"));
		free_packet(&pkt);
		return err;
	}

	*len = length;
	
	DPRINTF(DEBUG_READ_FILE, ("recv_read_from_file exiting OK, got %d bytes\n", *len));
	free_packet(&pkt);
	return B_OK;
}	







status_t
send_read_from_file(nspace *vol, vnode *node, off_t pos,
						size_t len, filecookie *cookie) {

	status_t	result;
	packet		*pkt=NULL;

	uchar		wordcount = 12;
	ulong		OffsetLow;
	ulong		OffsetHigh;
	ushort		bytecount = 0;

	DPRINTF(DEBUG_READ_FILE, ("send_read_from_file entering\n"));

	DPRINTF(DEBUG_READ_FILE, ("want to read %d(dec) bytes at pos %d(dec)\n", len, pos));

	result = new_smb_packet(vol, & pkt, SMB_COM_READ_ANDX);
	if (result != B_OK) return result;

	OffsetLow = (ulong) (pos & 0x00000000FFFFFFFF);
	OffsetHigh = (ulong) ((pos & 0xFFFFFFFF00000000) >> 32);
	DPRINTF(DEBUG_READ_FILE, ("pos stuff: pos is %Lx, low is %x, high is %x\n", pos, OffsetLow, OffsetHigh));

	insert_uchar(pkt,	wordcount);
	insert_uchar(pkt,	0xff);
	insert_uchar(pkt,	0);
	insert_ushort(pkt,	0);
	insert_ushort(pkt,	cookie->fid);
	insert_ulong(pkt,	OffsetLow);
	insert_ushort(pkt,	len);// MaxCount
	insert_ushort(pkt,	0); // MinCount, spec says this is reserved, mbz
	insert_ulong(pkt,	0); // reserved
	insert_ushort(pkt,	0); //mbz
	insert_ulong(pkt,	OffsetHigh);
	insert_ushort(pkt,	bytecount);
	
	result = SendSMBPacket(vol, pkt);
	
exit1:
	free_packet(& pkt);
	DPRINTF(DEBUG_READ_FILE, ("send_read_from_file exiting with result %d\n", result));
	return result;
}



status_t
recv_read_from_file(nspace *vol, vnode *node, off_t pos, void *buf, size_t *len) {

	status_t	result;
	packet		*pkt=NULL;
	void		*p=NULL;
	
	size_t		claimed_remaining;
	ushort		DataLength;
	ushort		DataOffset;

	DPRINTF(DEBUG_READ_FILE, ("recv_read_from_file entering \n"));
	

	result = RecvSMBPacket(vol, & pkt, SMB_COM_READ_ANDX);
	if (result != B_OK) goto exit1;

	skip_over_remove(pkt, 11);// Get to DataLength field
	remove_ushort(pkt, & DataLength);
	remove_ushort(pkt, & DataOffset);
	
	claimed_remaining = DataOffset - pkt->rempos + DataLength;
	if (verify_size(pkt, claimed_remaining) != B_OK) {
		DPRINTF(-1, ("verify size failed in recv_read_from_file\n"));
		result = B_IO_ERROR;
		goto exit2;
	}

	p = (void*)(pkt->buf + DataOffset);
	memcpy(buf, p, DataLength);
	*len = DataLength;
	p=NULL;
	
	DPRINTF(DEBUG_READ_FILE, ("recv_read_from_file got %d bytes\n", DataLength));

exit2:	
	free_packet(& pkt);
exit1:
	DPRINTF(DEBUG_READ_FILE, ("recv_read_from_file exiting with result %d \n", result));
	return result;
}	

	
status_t
read_from_file(nspace *vol, vnode *node, off_t pos,
					void *buf, size_t *len, filecookie *cookie) {

	status_t	result;
	size_t	len_req,howmuch,rem;
	
	off_t cur_pos=0;
	uchar* cur_bufp;
	

	DPRINTF(DEBUG_READ_FILE, ("read_from_file : len is %d, pos is %d\n", *len, pos));

	// All of these node->size references are coherency issues.

	// Adjust len_req to fit within known size
	if (node->size == 0) {
		*len = 0;
		return B_OK;
	}
	if (pos < 0) {
		pos = 0;
	}
	if (pos > node->size) {
		*len = 0;
		return B_OK;
	}
	len_req = min(*len, node->size);
	if (pos + len_req >= node->size) {
		len_req = node->size - pos;
	}
	DPRINTF(DEBUG_READ_FILE, ("read_from_file : len_req is %d\n", len_req));
	*len = len_req;
	if (*len == 0) {
		return B_OK;
	}	

#if 0
	if (use_cache(vol, node, pos, len_req, cookie)) {
		result = read_from_cache(pos, len_req, cookie, buf);
		if (result == B_OK) {
			return result;
		} else {
			DPRINTF(DEBUG_READ_FILE, ("read_from_cache failed : %d %s\n", result, strerror(result)));
		}
	}
#endif

	cur_bufp = (uchar*) buf;
	cur_pos = pos;
	rem = len_req;

	while (rem > 0) {
	
		howmuch = get_io_size(vol,rem);
		if (howmuch <= vol->small_io_size) {
			//dosmall(howmuch);
			result = send_read_from_file(vol, node, cur_pos, howmuch, cookie);
			if (result != B_OK) {
				DPRINTF(-1, ("error on send_read_from_file"));
				DPRINTF(-1, (": len is %d, pos is %d\n", *len, pos));
				return result;
			}
			
			result = recv_read_from_file(vol, node, cur_pos, cur_bufp, &howmuch);
			if (result != B_OK) {
				DPRINTF(-1, ("error on recv_read_from_file"));
				DPRINTF(-1, (": len is %d, pos is %d\n", *len, pos));
				return result;
			}
			
			// get_io_size is monotonically decreasing; so
			// this will always suck up the last of the data.
			rem -= howmuch;
			cur_bufp += howmuch;
			cur_pos += howmuch;
			continue;
		}

		//dolarge(howmuch);
		result = send_large_read_from_file(vol, node, cur_pos, howmuch, cookie);
		if (result != B_OK) {
			DPRINTF(-1, ("error on send_large_read_from_file\n"));
			return result;
		}
	
		result = recv_large_read_from_file(vol, node, cur_pos, cur_bufp, &howmuch);
		if (result != B_OK) {
			DPRINTF(-1, ("error on recv_large_read_from_file\n"));
			return result;
		}
	
		rem -= howmuch;
		cur_bufp += howmuch;
		cur_pos += howmuch;
	}

	DPRINTF(DEBUG_READ_FILE, ("read_from_file : exiting, read %d\n", *len));
	return result;
}
