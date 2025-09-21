#ifndef _NETIO_H
#define _NETIO_H

#include "packet.h"

extern status_t put_smb_header(nspace *vol, packet *pkt);
extern status_t new_smb_packet(nspace *vol, packet **_pkt, uchar command);
extern status_t put_nb_header(packet *pkt, size_t size);
extern status_t isDataPending(int socket, long timeout);
extern status_t RecvNBPacket(nspace *vol, packet **_pkt, long timeout);
extern status_t RecvNBPacket_nocheck(nspace *vol, packet **_pkt, long timeout);
extern status_t SendNBPacket(nspace *vol, packet *pkt, bool retry);
extern status_t SendSMBPacket(nspace *vol, packet *pkt);
extern status_t PopSMBHeader(nspace *vol, packet *pkt, int command, bool getIDs);
extern status_t RecvSMBPacket(nspace *vol, packet **_pkt, int command);
extern status_t RecvSMBPacket_nocheck(nspace *vol, packet **_pkt, int command);

#endif // _NETIO_H
