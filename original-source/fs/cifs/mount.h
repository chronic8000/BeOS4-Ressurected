#ifndef _MOUNT_H
#define _MOUNT_H

extern status_t InitNetworkConnection(nspace *vol);
extern void KillNetworkConnection(nspace *vol);
extern int copy_parms(nspace *vol, parms_t *parms);


#endif // _MOUNT_H
