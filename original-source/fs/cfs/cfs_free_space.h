#ifndef CFS_FREE_SPACE_H
#define CFS_FREE_SPACE_H

#include "cfs_vnode.h"

enum {
	METADATA = 0,
	FILEDATA
};

off_t cfs_get_free_space(cfs_info *info);
status_t free_disk_space(cfs_info *info, cfs_transaction *transaction,
                         off_t pos, off_t size);
status_t allocate_disk_space(cfs_info *info, cfs_transaction *transaction,
                             off_t *pos, off_t size, int data_type);

// due to differences in allocation algorithms, the different versions
// of the filesystem may touch more/less blocks during an allocate or free procedure
#define V2_ALLOCATE_MAX_LOG_SIZE	7
#define V1_ALLOCATE_MAX_LOG_SIZE	4
#define ALLOCATE_MAX_LOG_SIZE(version) (((version)>1) ? V2_ALLOCATE_MAX_LOG_SIZE : V1_ALLOCATE_MAX_LOG_SIZE)

#define V2_FREE_MAX_LOG_SIZE		7
#define V1_FREE_MAX_LOG_SIZE		4
#define FREE_MAX_LOG_SIZE(version) (((version)>1) ? V2_FREE_MAX_LOG_SIZE : V1_FREE_MAX_LOG_SIZE)



#endif

