#ifndef PACKET_H
#define PACKET_H

extern void dump_packet(packet *pkt);
extern status_t init_packet(packet **_pkt, int32 size);
extern status_t resize_packet(packet **_pkt, size_t grow);
extern status_t verify_size(packet *pkt, size_t size);
extern size_t getSize(packet *pkt);
extern size_t getremPos(packet *pkt);
extern size_t getRemaining(packet *pkt);
extern status_t set_order(packet *pkt, bool order);
extern status_t skip(packet *pkt, size_t size);
extern status_t free_packet(packet **_pkt);
extern status_t insert_char(packet *pkt, char data);
extern status_t insert_uchar(packet *pkt, uchar data);
extern status_t insert_short(packet *pkt, short data);
extern status_t insert_ushort(packet *pkt, unsigned short data);
extern status_t insert_long(packet *pkt, int data);
extern status_t insert_ulong(packet *pkt, unsigned int data);
extern status_t insert_int64(packet *pkt, int64 data);
extern status_t insert_uint64(packet *pkt, uint64 data);
extern status_t insert_string(packet *pkt, const char *data);
extern status_t insert_void(packet *pkt, const void *data, size_t datasize);
extern status_t insert_packet(packet *dst_pkt, packet *src_pkt);
extern status_t remove_char(packet *pkt, char *pdata);
extern status_t remove_uchar(packet *pkt, uchar *pdata);
extern status_t remove_short(packet *pkt, short *pdata);
extern status_t remove_ushort(packet *pkt, unsigned short *pdata);
extern status_t remove_int(packet *pkt, int *pdata);
extern status_t remove_uint(packet *pkt, unsigned int *pdata);
extern status_t remove_int64(packet *pkt, int64 *pdata);
extern status_t remove_uint64(packet *pkt, uint64 *pdata);
extern status_t remove_string(packet *pkt, char *pdata, size_t maxsize);
extern status_t remove_void(packet *pkt, void *pdata, size_t size);
extern status_t remove_packet(packet *srcpkt, packet *dstpkt, size_t size);

#endif // PACKET_H
