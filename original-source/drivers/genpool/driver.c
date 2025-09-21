/* :ts=8 bk=0
 *
 * driver.c:	Front-end for the GenPool module.
 *
 * Leo L. Schwab					1999.08.16
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <dinky/listnode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <surface/genpool.h>
#include <drivers/genpool_module.h>


/****************************************************************************
 * #defines
 */


/****************************************************************************
 * Prototypes.
 */
/*
 * Swiped from  $BUILDHOME/src/nukernel/inc/vm.h .  I have no idea what
 * the protection bits mean.  Cyril: Howzabout putting this in
 * KernelExport.h?
 */
extern bool	is_valid_range (const void *start, size_t size, uint prot);


/*****************************************************************************
 * Bookkeeppiinngg structures.
 */
typedef struct openedpool {
	Node	op_Node;
	uint32	op_PoolID;
} openedpool;

typedef struct orb {			/*  Opener Resource Block  */
	List	orb_OpenedPools;	/*  List of openedpools		*/
	uint32	orb_OwnerID;		/*  ID of opener (for cleanup)  */
} orb;


/****************************************************************************
 * Prototypes.
 */
static status_t open_hook (const char *name, uint32 flags, void **cookie);
static status_t close_hook (void *dev);
static status_t free_hook (void *dev);
static status_t read_hook (void *dev, off_t pos, void *buf, size_t *len);
static status_t write_hook (void *dev, off_t pos, const void *buf, size_t *len);
static status_t control_hook (void *dev, uint32 msg, void *buf, size_t len);


/****************************************************************************
 * Globals.
 */
static device_hooks	gdevice_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL
};

static const char	*gdevnames[] = {
	"misc/genpool",
	NULL
};

static genpool_module	*gpm;


/****************************************************************************
 * Kernel entry points.
 */
status_t
init_hardware (void)
{
	status_t	retval;
	module_info	*mod;
	
	/*
	 * "Huh?" you ask?  Well, if the module doesn't exist, then we don't
	 * want the kernel thinking this driver can actually do anything.
	 * And if it does exist, then we close it back up since we're not
	 * actually using it yet.
	 */
	if ((retval = get_module (B_GENPOOL_MODULE_NAME, &mod)) == B_OK)
		put_module (B_GENPOOL_MODULE_NAME);

	return (retval);
}

status_t
init_driver (void)
{
	return (get_module (B_GENPOOL_MODULE_NAME, (module_info **) &gpm));
}

const char **
publish_devices (void)
{
	/*  Return the list of supported devices  */
	return (gdevnames);
}

device_hooks *
find_device (const char *name)
{
	if (!strcmp (name, gdevnames[0]))
		return (&gdevice_hooks);
	return (NULL);
}

void
uninit_driver (void)
{
	if (gpm) {
		put_module (B_GENPOOL_MODULE_NAME);
		gpm = NULL;
	}
}


void
idle_device (void *di)
{
	/*  This device doesn't consume a lot of power...  */
}


/****************************************************************************
 * Device hook (and support) functions.
 */
static status_t
open_hook (const char *name, uint32 flags, void **cookie)
{
	orb	*orb;

	/*
	 * FIXME: Maintain per-opener data so we can free resources when the
	 * application cras...  I mean, exits.
	 */
	if (!(orb = malloc (sizeof (struct orb))))
		return (B_NO_MEMORY);

	InitList (&orb->orb_OpenedPools);
	orb->orb_OwnerID = (gpm->gpm_GetUniqueOwnerID) ();

	*cookie = orb;
	return (B_OK);
}

static status_t
close_hook (void *dev)
{
	/*  We don't do anything on close: there might be dup'd fd  */
	return (B_OK);
}

/*
 * free_hook - close down the device
 */
static status_t
free_hook (void *dev)
{
	orb		*orb;
	openedpool	*op;

	orb = dev;
	while (op = (openedpool *) RemHead (&orb->orb_OpenedPools)) {
		(gpm->gpm_UnmarkRangesOwnedBy) (op->op_PoolID,
						orb->orb_OwnerID);
		free (op);
	}

	free (orb);
	return (B_OK);
}


/*
 * Read, write, and close hooks.  They do nothing gracefully.  Or rather, they
 * gracefully do nothing.  Whatever...
 */
static status_t
read_hook (void *dev, off_t pos, void *buf, size_t *len)
{
	*len = 0;
	return (B_NOT_ALLOWED);
}

static status_t
write_hook (void *dev, off_t pos, const void *buf, size_t *len)
{
	*len = 0;
	return (B_NOT_ALLOWED);
}


/*
 * control_hook - where the real work is done
 */
static status_t
control_hook (void *dev, uint32 msg, void *buf, size_t len)
{
	orb		*orb;
	openedpool	*op;
	status_t	retval;

//kprintf ("==+ ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len);
	orb = dev;

	switch (msg) {
	case B_IOCTL_CREATEPOOL:
#define	ICP	((BIoctl_CreatePool *) buf)
		if (len) {
			/*  Request from user space; sanity-check  */
			if (!is_valid_range (ICP->icp_PoolInfo,
					     sizeof (BPoolInfo),
					     0))
				return (B_BAD_ADDRESS);
		}
		if (!(op = malloc (sizeof (openedpool))))
			return (B_NO_MEMORY);
		retval = (gpm->gpm_CreatePool) (ICP->icp_PoolInfo,
						ICP->icp_MaxAllocs,
						ICP->icp_UserDataSize);
		if (retval == B_OK) {
			op->op_PoolID = ICP->icp_PoolInfo->pi_PoolID;
			AddHead (&op->op_Node, &orb->orb_OpenedPools);
		} else
			free (op);
		return (retval);

	case B_IOCTL_CLONEPOOL:
		if (!(op = malloc (sizeof (openedpool))))
			return (B_NO_MEMORY);
		retval = (gpm->gpm_ClonePool) ((BPoolInfo *) buf);
		if (retval == B_OK) {
			op->op_PoolID = ((BPoolInfo *) buf)->pi_PoolID;
			AddHead (&op->op_Node, &orb->orb_OpenedPools);
		} else
			free (op);
		return (retval);

	case B_IOCTL_DELETEPOOL:
		if ((retval = (gpm->gpm_DeletePool) ((uint32) buf)) == B_OK) {
			/*  Free up all marked ranges.  */
			for (op = (openedpool *)
			          FIRSTNODE (&orb->orb_OpenedPools);
			     NEXTNODE (op);
			     op = (openedpool *) NEXTNODE (op))
			{
				if (op->op_PoolID == (uint32) buf) {
					/*
					 * If the pool was actually destroyed,
					 * then this will return an error,
					 * which is harmless in this case.
					 */
					(gpm->gpm_UnmarkRangesOwnedBy)
					 (op->op_PoolID, orb->orb_OwnerID);
					RemNode (&op->op_Node);
					free (op);
					break;
				}
			}
		}
		return (retval);

	case B_IOCTL_FINDPOOL:
		return ((gpm->gpm_FindPool) ((const char *) buf));

	case B_IOCTL_GETNEXTPOOLID:
		return ((gpm->gpm_GetNextPoolID) (buf));

	case B_IOCTL_LOCKPOOL:
		return ((gpm->gpm_LockPool) ((uint32) buf));

	case B_IOCTL_UNLOCKPOOL:
		(gpm->gpm_UnlockPool) ((uint32) buf);
		return (B_OK);

	case B_IOCTL_MARKRANGE:
#define	IMR	((BIoctl_MarkRange *) buf)
		if (len) {
			/*
			 * Request coming from user space.  Make sure
			 * they're sane.
			 */
			if (IMR->imr_UserData  &&
			    IMR->imr_UserDataSize  &&
			    !is_valid_range (IMR->imr_UserData,
					     IMR->imr_UserDataSize,
					     0))
				return (B_BAD_ADDRESS);
			if (!is_valid_range (IMR->imr_MemRange,
					     sizeof (BMemRange),
					     0))
				return (B_BAD_ADDRESS);
		}
		IMR->imr_MemRange->mr_Owner = orb->orb_OwnerID;
		return ((gpm->gpm_MarkRange) (IMR->imr_PoolID,
					      IMR->imr_MemRange,
					      IMR->imr_UserData,
					      IMR->imr_UserDataSize));

	case B_IOCTL_UNMARKRANGE:
#define	IUR	((BIoctl_UnmarkRange *) buf)
		return ((gpm->gpm_UnmarkRange) (IUR->iur_PoolID,
						IUR->iur_Offset));
	case B_IOCTL_UNMARKRANGES_BYOWNER:
		return ((gpm->gpm_UnmarkRangesOwnedBy) (IUR->iur_PoolID,
							IUR->iur_Offset));

	case B_IOCTL_ALLOCBYMEMSPEC:
		((BMemSpec *) buf)->ms_MR.mr_Owner = orb->orb_OwnerID;
		return ((gpm->gpm_AllocByMemSpec) ((BMemSpec *) buf));

	case B_IOCTL_FREEBYMEMSPEC:
		return ((gpm->gpm_FreeByMemSpec) ((BMemSpec *) buf));
	}

	return (B_DEV_INVALID_IOCTL);
}
