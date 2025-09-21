#ifndef _DOSFS_VCACHE_H_
#define _DOSFS_VCACHE_H_

extern void dump_vcache(nspace *vol);
extern status_t init_vcache(nspace *vol);
extern status_t uninit_vcache(nspace *vol);
extern vnode_id generate_unique_vnid(nspace *vol);
extern status_t add_to_vcache(nspace *vol, vnode_id vnid, const char *filename, vnode_id parent_dir, char is_dir);

extern status_t remove_from_vcache(nspace *vol, vnode_id vnid);
extern status_t vcache_loc_to_vnid(nspace *vol, const char* filename, vnode_id parent_dir, vnode_id *vnid, char *is_dir);
extern status_t vcache_vnid_to_loc(nspace *vol, vnode_id vnid, char **filename, vnode_id *parent_dir, char *is_dir);

extern status_t set_vnid_del(nspace *vol, vnode_id vnid);

#define find_vnid_in_vcache(vol,vnid) vcache_vnid_to_loc(vol,vnid,NULL)
#define find_loc_in_vcache(vol,loc) vcache_loc_to_vnid(vol,loc,NULL)

#endif
