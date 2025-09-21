/* :ts=8 bk=0
 *
 * genpool_module.h:	Definitions for GenPool module.
 *
 * Leo L. Schwab					1999.08.12
 */
#ifndef	_GENPOOL_MODULE_H
#define	_GENPOOL_MODULE_H

#ifndef	_MODULE_H
#include <drivers/module.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * Module description.
 */
typedef struct genpool_module {
	module_info	gpm_ModuleInfo;
	struct BPoolInfo *
			(*gpm_AllocPoolInfo) (void);
	void		(*gpm_FreePoolInfo) (struct BPoolInfo *pi);
	status_t	(*gpm_CreatePool) (struct BPoolInfo *pi,
					   uint32 maxallocs,
					   uint32 userdatasize);
	status_t	(*gpm_ClonePool) (struct BPoolInfo *pi);
	status_t	(*gpm_DeletePool) (uint32 poolid);
	uint32		(*gpm_FindPool) (const char *name);
/**/	uint32		(*gpm_GetNextPoolID) (uint32 *cookie);
	uint32		(*gpm_GetUniqueOwnerID) (void);

	status_t	(*gpm_LockPool) (uint32 poolid);
	void		(*gpm_UnlockPool) (uint32 poolid);

	status_t	(*gpm_MarkRange) (uint32 poolid,
					  struct BMemRange *cl_mr,
					  const void *userdata,
					  uint32 userdatasize);
	status_t	(*gpm_UnmarkRange) (uint32 poolid, uint32 offset);
	status_t	(*gpm_UnmarkRangesOwnedBy) (uint32 poolid,
						    uint32 owner);

	/*  Default allocation logic  */
	status_t	(*gpm_AllocByMemSpec) (struct BMemSpec *ms);
	status_t	(*gpm_FreeByMemSpec) (struct BMemSpec *ms);

	/*  FIXME: Move to above commented-out slot on next release.  */
} genpool_module;


/*****************************************************************************
 * The Name.
 */
#define	B_GENPOOL_MODULE_NAME	"generic/genpool/v2"


#ifdef __cplusplus
}
#endif

#endif	/*  _GENPOOL_MODULE_H  */
