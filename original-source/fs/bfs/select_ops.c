#include "fsproto.h"
#include "select_ops.h"

int
bfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
#if _KERNEL_MODE
	notify_select_event(sync, ref);
#endif

	return B_OK;
}

int
bfs_deselect(void *ns, void *node, void *cookie, uint8 event, selectsync *sync)
{
	return B_OK;
}
