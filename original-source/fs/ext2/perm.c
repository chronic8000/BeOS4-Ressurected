#include "ext2.h"
#include "perm.h"

int ext2_check_permission(ext2_fs_struct *ext2, vnode *v, int omode)
{
	int     access_type, mask;
	mode_t  mode = v->inode.i_mode;
	uid_t   uid;
	gid_t   gid;

	uid = geteuid();
	gid = getegid();

	if (uid == 0)
		return 1;

	mask = (omode & O_ACCMODE);

	if (mask == O_RDONLY)
		access_type = S_IROTH;
	else if (mask == O_WRONLY)
		access_type = S_IWOTH;
	else if (mask == O_RDWR)
		access_type = (S_IROTH | S_IWOTH);
	else {
		access_type = S_IROTH;
	}

    if(ext2->override_uid) {
		if (uid == ext2->uid)
	        mode >>= 6;
	    else if (gid == ext2->gid)
	        mode >>= 3;
	} else {
		if (uid == v->inode.i_uid)
	        mode >>= 6;
	    else if (gid == v->inode.i_gid)
	        mode >>= 3;	
	}

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}

int ext2_check_access(ext2_fs_struct *ext2, vnode *v, int access_type)
{
	mode_t  mode = v->inode.i_mode;
	uid_t   uid;
	gid_t   gid;

	/* access() uses the real and not effective IDs, so sayeth Stevens, 4.7 */
	uid = getuid();
	gid = getgid();

	if (uid == 0)
		return 1;

    if(ext2->override_uid) {
		if (uid == ext2->uid)
	        mode >>= 6;
	    else if (gid == ext2->gid)
	        mode >>= 3;
	} else {
		if (uid == v->inode.i_uid)
	        mode >>= 6;
	    else if (gid == v->inode.i_gid)
	        mode >>= 3;	
	}

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}

