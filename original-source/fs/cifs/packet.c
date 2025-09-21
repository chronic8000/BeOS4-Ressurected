#include <ByteOrder.h>
#include <netdb.h>
#include <netinet/in.h>
#include "cifs_globals.h"

#ifdef DEBUG
void
dump_packet(packet *pkt) {
	int i;
	uchar *buf;
	
	if (pkt==NULL) {
		dprintf("dump_packet got NULL packet\n");
		return;
	}
	
	buf = pkt->buf;

	dprintf("Dumping pkt at address 0x%x, pkt-buf 0x%x\n", pkt, pkt->buf);
	dprintf("addpos = %d\n", pkt->addpos);
	dprintf("rempos = %d\n", pkt->rempos);
	dprintf("type = %d\n", pkt->type);
	dprintf("command = %d\n", pkt->command);
	dprintf("order = %d\n", pkt->order);

	if (buf != NULL) {
		dprintf("packet looks like:\n");
		for (i=0; i < pkt->addpos; i++) {
			if (!(i%4)) dprintf("\n");
			dprintf("%2x ", buf[i]);
		}
		dprintf("\n");
	} else {
		dprintf("pkt->buf == NULL\n");
	}
	return;
}
#endif

// Allocation, resize, deallocation
status_t
init_packet(packet **_pkt, int32 size) {

	packet *pkt = NULL;

	if (_pkt == NULL) {
		debug(-1, ("INIT_PACKET GIVEN NULL _pkt\n"));
		return B_ERROR;
	}
	
	if (*_pkt != NULL) {
		debug(-1,("INIT_PACKET GIVEN NON NULL *_pkt\n"));
		return B_ERROR;
	}

	//*_pkt = (packet*) malloc(sizeof(packet));
	MALLOC((*_pkt), packet*, sizeof(packet));
	
	pkt = *_pkt;
	
	if (pkt == NULL)
		return ENOMEM;


	if (size <= 0) {
		pkt->size = PACKET_DEFAULT_SIZE;
	} else {
		pkt->size = size;
	}

	//pkt->buf = (uchar*) malloc(	(pkt->size)*sizeof(uchar) + PACKET_SAFETY_MARGIN );
	MALLOC(pkt->buf, uchar*, ((pkt->size)*sizeof(uchar) + PACKET_SAFETY_MARGIN));
	if (pkt->buf == NULL)
		return ENOMEM;
	memset((void*)pkt->buf, 0, (pkt->size)*sizeof(uchar) + PACKET_SAFETY_MARGIN );
		
	pkt->addpos = 0;
	pkt->rempos = 0;
	pkt->type = 0;
	pkt->order = true;
	
	return B_OK;
}



status_t
resize_packet(packet **_pkt, size_t grow) {
	
	packet *pkt = *_pkt;
	uchar *tmp;
	
	if (pkt == NULL) {
		DPRINTF(-1,("resize_packet got NULL *_pkt\n"));
		return B_ERROR;
	}
	if ((pkt->addpos + grow) > pkt->size) {

		//dprintf("Resizing pkt, old size: %d, new size %d\n",
		//	pkt->size, (pkt->size + grow + PACKET_GROW_SIZE));
			
		tmp = pkt->buf;		
		//pkt->buf = (uchar*) malloc(pkt->size + grow + PACKET_GROW_SIZE);
		MALLOC(pkt->buf, uchar*, (pkt->size + grow + PACKET_GROW_SIZE))

		if (pkt->buf == NULL) {
			pkt->buf = tmp; // Put back like we found it
			return ENOMEM;
		} else {
			
			memset((void*)pkt->buf, 0, pkt->size + grow + PACKET_GROW_SIZE);
			memcpy(pkt->buf, tmp, pkt->size);
			pkt->size += grow + PACKET_GROW_SIZE;
			FREE(tmp);
			return B_OK;
		}

	} else {
		return B_OK;
	}

}

status_t
verify_size(packet *pkt, size_t size) {  // Can i remove size bytess?

	if (pkt == NULL) {
		DPRINTF(-1,("verify_size got NULL pkt\n"));
		return B_ERROR;
	}
	if ((pkt->addpos - pkt->rempos) >= size) {
		return B_OK;
	} else {
		return B_ERROR;
	}

}


size_t
getSize(packet *pkt) {
	if (pkt == NULL) return B_ERROR;
	return pkt->addpos;
}

size_t
getremPos(packet *pkt) {
	if (pkt == NULL) return B_ERROR;
	return pkt->rempos;
}

size_t
getRemaining(packet *pkt) {
	if (pkt == NULL) return B_ERROR;
	return (pkt->addpos - pkt->rempos);
}

status_t
set_order(packet *pkt, bool order) {
	pkt->order = order;
	return B_OK;
}



// skip is handy when you code the buffer
// yourself, and you need to adjust.
// Notice that no call is made to verify_size.
// *Dont be stupid bob*
status_t
skip_over(packet *pkt, size_t size) {
	if (pkt==NULL) return B_ERROR;
	pkt->addpos += size;
	return B_OK;
}

status_t
skip_over_remove(packet *pkt, size_t size) {
	if (pkt==NULL) return B_ERROR;
	pkt->rempos += size;
	return B_OK;
}



status_t
free_packet(packet **_pkt) {
	
	if (_pkt == NULL) {
		DPRINTF(-1,("FREE_packet got NULL _pkt\n"));
		return B_ERROR;
	}

	if (*_pkt == NULL) {
		DPRINTF(-1,("FREE_packet got NULL *_pkt\n"));
		return B_ERROR;
	}

	if ((*_pkt)->buf) {
		FREE((*_pkt)->buf);
		(*_pkt)->buf = NULL;
	}

	(*_pkt)->size = -1;
	(*_pkt)->addpos = -1;
	FREE(*_pkt);
	*_pkt = NULL;
	
	return B_OK;
}



// Insertion

status_t 
insert_char(packet *pkt, char data) {

	status_t result;
	
	result = resize_packet(&pkt, sizeof(char));
	if (result != B_OK) return result;

	*(pkt->buf + pkt->addpos) = data;
	
	pkt->addpos += sizeof(char);
	
	return B_OK;
}

status_t 
insert_uchar(packet *pkt, uchar data) {

	status_t result;
	
	result = resize_packet(&pkt, sizeof(uchar));
	if (result != B_OK) return result;

	*(pkt->buf + pkt->addpos) = data;
	
	pkt->addpos += sizeof(uchar);
	
	return B_OK;
}

status_t 
insert_short(packet *pkt, short data) {

	status_t result;
	size_t datasize = sizeof(short);
	short d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) { // Network Order
		d = htons(data);
	} else { // The other guys order
		d = B_HOST_TO_LENDIAN_INT16(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}

status_t 
insert_ushort(packet *pkt, unsigned short data) {

	status_t result;
	size_t datasize = sizeof(short);
	unsigned short d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) {
		d = htons(data);
	} else {
		d = B_HOST_TO_LENDIAN_INT16(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}


status_t 
insert_long(packet *pkt, int data) {

	status_t result;
	size_t datasize = sizeof(int);
	int d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) {
		d = htonl(data);
	} else {
		d = B_HOST_TO_LENDIAN_INT32(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}

status_t 
insert_ulong(packet *pkt, unsigned int data) {

	status_t result;
	size_t datasize = sizeof(unsigned int);
	unsigned int d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) {
		d = htonl(data);
	} else {
		d = B_HOST_TO_LENDIAN_INT32(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}


status_t 
insert_int64(packet *pkt, int64 data) {

	status_t result;
	size_t datasize = sizeof(int64);
	int64 d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) {
		d = B_HOST_TO_BENDIAN_INT64(data);
	} else {
		d = B_HOST_TO_LENDIAN_INT64(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}

status_t 
insert_uint64(packet *pkt, uint64 data) {

	status_t result;
	size_t datasize = sizeof(uint64);
	uint64 d;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	if (pkt->order) {
		d = B_HOST_TO_BENDIAN_INT64(data);
	} else {
		d = B_HOST_TO_LENDIAN_INT64(data);
	}	

	memcpy(pkt->buf + pkt->addpos, &d, datasize);

	pkt->addpos += datasize;
	
	return B_OK;
}


status_t
insert_string(packet *pkt, const char *data) {

	status_t result;
	size_t datasize = strlen(data) + 1;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;
	
	strcpy((char *) (pkt->buf + pkt->addpos), data);
	
	pkt->addpos += datasize;

	return B_OK;	
}

status_t
insert_void(packet *pkt, const void *data, size_t datasize) {

	status_t result;
	
	if (data == NULL) return B_ERROR;
	
	result = resize_packet(&pkt, datasize);
	if (result != B_OK) return result;

	memcpy(pkt->buf + pkt->addpos, data, datasize);

	pkt->addpos += datasize;

	return B_OK;
}

status_t
insert_packet(packet *dst_pkt, packet *src_pkt) {

	status_t result;
	
	if (src_pkt == NULL)
		return B_ERROR;
	
	result = resize_packet(&dst_pkt, src_pkt->addpos);
	if (result != B_OK) return result;

	memcpy(dst_pkt->buf + dst_pkt->addpos, src_pkt->buf, src_pkt->addpos);

	dst_pkt->addpos += src_pkt->addpos;
	
	return B_OK;
}



// Removal


status_t 
remove_char(packet *pkt, char *pdata) {

	status_t result;
	size_t datasize = sizeof(char);

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	*pdata = *(pkt->buf + pkt->rempos);
	pkt->rempos += datasize;
	return B_OK;
	
}

status_t 
remove_uchar(packet *pkt, uchar *pdata) {

	status_t result;
	size_t datasize = sizeof(uchar);

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	*pdata = *(pkt->buf + pkt->rempos);
	pkt->rempos += datasize;
	return B_OK;
	
}

status_t
remove_short(packet *pkt, short *pdata) {

	status_t result;
	size_t datasize = sizeof(short);
	short d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = ntohs(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT16(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}

status_t
remove_ushort(packet *pkt, unsigned short *pdata) {

	status_t result;
	size_t datasize = sizeof(unsigned short);
	unsigned short d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = ntohs(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT16(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}

status_t
remove_long(packet *pkt, long *pdata) {

	status_t result;
	size_t datasize = sizeof(long);
	long d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = ntohs(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT32(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}

status_t
remove_ulong(packet *pkt, unsigned long *pdata) {

	status_t result;
	size_t datasize = sizeof(unsigned long);
	unsigned long d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = ntohl(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT32(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}

status_t
remove_int64(packet *pkt, int64 *pdata) {

	status_t result;
	size_t datasize = sizeof(int64);
	int64 d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = B_BENDIAN_TO_HOST_INT64(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT64(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}

status_t
remove_uint64(packet *pkt, uint64 *pdata) {

	status_t result;
	size_t datasize = sizeof(uint64);
	uint64 d;

	result = verify_size(pkt, datasize);
	if (result != B_OK) return result;
	
	memcpy(&d, (pkt->buf + pkt->rempos), datasize);
	
	if (pkt->order) {
		*pdata = B_BENDIAN_TO_HOST_INT64(d);
	} else {
		*pdata = B_LENDIAN_TO_HOST_INT64(d);
	}
	
	pkt->rempos += datasize;

	return B_OK;
}


status_t
remove_string(packet *pkt, char *pdata, size_t maxsize) {

	char *c;
	
	if (pkt == NULL) return B_ERROR;
	if (pdata == NULL) return B_ERROR;
	
	c = (char *)(pkt->buf + pkt->rempos);

	DPRINTF(-1,("remove_string sucks\n"));	
	if (verify_size(pkt, strlen(c)) != B_OK &&
			verify_size(pkt, maxsize) != B_OK) {
		return B_ERROR;
	}
	// Isn't this strlen(c) bad? What if c doesn't have
	// a null before the end of the packet?

	strncpy(pdata, c, maxsize);
	pkt->rempos += strlen(c) + 1;

	return B_OK;
}


status_t
remove_unicode(packet *pkt, char *cstring, size_t maxlen) {

	char *p;
	int i=0;
	char *buf;

	if (pkt == NULL) return B_ERROR;
	if (cstring == NULL) return B_ERROR;

	buf = (char *)(pkt->buf + pkt->rempos);
	
	// Cant do strlen on wide chars.
	DPRINTF(-1,("remove_unicode sucks\n"));
	
	p = cstring;
	while ( (*buf != 0) && (i < maxlen) ) {
		*p++ = *buf;
		buf += 2;
		pkt->rempos += 2;
		i++;
	}
	*p = 0;

	return B_OK;
}

status_t
remove_void(packet *pkt, void *pdata, size_t size) {

	status_t result;
	char *c;
	int i;

	if (pkt == NULL) return B_ERROR;
	if (pdata == NULL) return B_ERROR;
	
	c = (char *)(pkt->buf + pkt->rempos);
	
	result = verify_size(pkt, size);
	if (result != B_OK) return result;
	
	memcpy((char*)pdata, c, size);
	pkt->rempos += size;

	return B_OK;
}

status_t
remove_packet(packet *srcpkt, packet *dstpkt, size_t size) {

	status_t result;
	char *c;
	
	if (size <= 0) { // Copy the whole packet
		return B_ERROR;
		//
	} else { // Remove size bytes, add them to dstpkt

		result = verify_size(srcpkt, size);
		if (result != B_OK) return result;
		
		c = (char*)(srcpkt->buf + srcpkt->rempos);
		
		result = insert_void(dstpkt, (void*) c, size);
		if (result != B_OK) return result;
		srcpkt->rempos += size;
		return B_OK;
	}
	
	// This is not an exit.
	return B_ERROR; 
}
