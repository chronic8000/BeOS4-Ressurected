#ifndef _BIG_MOMMA_INCLUDES_H_
#define _BIG_MOMMA_INCLUDES_H_


extern int send_cifs_packet(int len, char *buf);
extern int fake_sendv(int socket, struct iovec *iov, int iovcnt);
extern int init_netbios_connection(char *server, int *socket);
extern int kill_netbios_connection(int socket);
extern int CIFS_Mount(const char *user, const char *password, nspace **vol);
extern int CIFS_Unmount();






#endif // _BIG_MOMMA_INCLUDES_H_
