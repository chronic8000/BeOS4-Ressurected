
#ifndef _CDDA_H
#define _CDDA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "fsproto.h"
#include <time.h>

#include <SupportDefs.h>
#include "cdda_io.h"

/* some fixed vnode IDs */
#define CDDA_ROOTNODE_ID 1
#define CDDA_CDDANODE_ID 2
#define CDDA_WAVNODE_ID  3
#define CDDA_SETTINGS_ID 4
#define CDDA_README_ID 5


extern const char readmefile[];

/* ======================================
	vnode_id is a simple numbering scheme: root=1, cdda-dir=2, wav-dir=3
	cdda files start at 1000
	wav files start at 2000

*/
typedef struct vnode
{
	vnode_id	id; 
//	char*		filename;
	ulong		flags;
	mode_t		mode;
} vnode;


/* ======================================
	Cookie used when walking directories.
*/
typedef struct dircookie
{
	vnode_id	vnid;
	long		counter;
} dircookie;
 
/* ======================================
	Cookie used when opening files.
	The cookie contains a buffer for
	one "block" (a number of contiguous
	sectors), used for buffering.
*/
typedef struct filecookie
{
	long buffernumber;
	char *buffer;
} filecookie;

/* ======================================
	The global filesystem structure
*/
typedef struct nspace
{
	nspace_id		id;				// ID passed in to fs_mount
	int				fd;				// File descriptor
	time_t			mounttime;
	bool			nameschanged;

	long CDDA_BLOCKREAD_SIZE;		// how many blocks to read at once
	long CDDA_BUFFER_SIZE;			// size of the per-file buffer

	char			devicePath[127];
	char			volumename[B_FILE_NAME_LENGTH+1];

	vnode			rootvnode;
	t_CDFS_TOC		toc;			// TOC in my own internal format
	sem_id			locksem;
} nspace;

/* ======================================
*/
int 	CDDAMount(const char *path, const int flags, nspace** newVol);
int		CDDAReadDirEnt(nspace* ns, dircookie* cookie, struct dirent* buf, size_t bufsize);
int		InitNode(void *_ns, vnode* rec, vnode_id vnid);

#ifdef __cplusplus
}
#endif


#endif
