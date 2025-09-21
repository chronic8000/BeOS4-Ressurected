

#ifndef _CDDA_IO_H
#define _CDDA_IO_H

#include <SupportDefs.h>

// CD drive gives back one of these for each track (and one extra 
// so you can calculate the length of the last track).

typedef struct CDaddress
{
	uint8 unused;
	uint8 minute;
	uint8 second;
	uint8 frame;
} t_CDaddress;

typedef struct CDtocentry 
{
	char reserved1;
	unsigned char flags;
	unsigned char tracknumber;
	char reserved2;
	t_CDaddress address;
} t_CDtocentry;

typedef struct CDtoc
{
	uchar header[2];
	uchar mintrack;
	uchar maxtrack;
	t_CDtocentry tocentry[100];
} t_CDtoc;


/* 
	For convenience, I parse the CD-native TOC
	into something more managable:
*/
typedef struct CDFS_TOC_Entry 
{
	ulong startaddress;
	ulong length;
	ulong realtracknumber;
	char  *name;
} t_CDFS_TOC_Entry;

typedef struct CDFS_TOC
{
	long numtracks;
	t_CDFS_TOC_Entry tocentry[100];
	ulong CDDBid;
	ulong CDPlayerkey;
} t_CDFS_TOC;

// number of databytes in an audio sector
const long CDDA_SECTOR_SIZE=2352;


const char cddbfiles[]="/boot/home/config/settings/cdda";
const char cddbfilestodo[]="/boot/home/config/settings/cdda/TODO";
const char SETTINGSFILE[]="/boot/home/config/settings/kernel/drivers/cdda";

#ifdef __cplusplus
extern "C" {
#endif
status_t cdda_readdata(void *_ns, struct filecookie *cookie, char *buf,off_t cdda_offset, size_t length);
int read_toc(int cdfile, t_CDFS_TOC *mytoc);
unsigned long cddb_discid(t_CDtoc *cdtoc);
unsigned long cdplayerkey(t_CDtoc *cdtoc);
#ifdef __cplusplus
}
#endif

#endif
