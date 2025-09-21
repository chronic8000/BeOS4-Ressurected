#ifndef _TRANS2_H_
#define _TRANS2_H_


extern status_t send_trans2_req(nspace *vol, packet *Setup, packet *Param, packet *Data);
extern status_t recv_trans2_req(nspace *vol, packet **pSetup, packet **pParam, packet **pData);


#endif // _TRANS2_H_
