/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : archive.h    1.8
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Archive manipulation library
 *
 *  Last update              : 11:00:31 - 98/04/17
 *
 *  Description              :  
 *
 * The routines defined here provide a simple interface for
 * tools which need to manipulate an existing archive.
 */

#ifndef archive_INCLUDED
#define archive_INCLUDED

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include "ar.h"
#include "Lib_Util.h"

/* Types. */
typedef	struct	ar_hdr	ar_hdr;
typedef	struct	archive	archive;
typedef	struct	module	module;

typedef	enum { AR_OK, AR_FAILED, AR_NEXIST, AR_NOTLIB, AR_NOMEM, AR_NREAD, AR_NSEEK }
	ar_stat;				/* error status type		*/
typedef	enum	ftype { F_ORD = 1, F_DTREE = 2, F_TMOBJECT = 4 } ftype; /* filetypes */


/* Archive type. */
struct archive	{
	char	*ar_name;		/* archive name			*/
	FILE	*ar_fp;			/* FILE *			*/
	int	ar_dirty;		/* iff contents changed		*/
	module	*ar_longnames;		/* long names module		*/
	module	*ar_comdef;		/* __.COMDEF module		*/
	module	*ar_symdef;		/* __.SYMDEF module		*/
	module	*ar_modlist;		/* module list			*/
};

/* Module type. */
struct module {
	module	*mod_next;		/* next module in linked list	*/
	char	*mod_name;		/* module name			*/
	archive	*mod_ap;		/* archive info			*/
	ar_hdr	mod_hdr;		/* archive header		*/
	long	mod_seek;		/* seek in archive		*/
	void	*mod_loc;		/* module location in memory	*/
	void	*mod_loc2;		/* module copy loc in memory	*/
	size_t	mod_len;		/* module length		*/
	ftype	mod_ftype;		/* file type			*/
	void	*mod_info;		/* additional module info	*/
};

#if	0
/*
 * This source should simply #include <ranlib.h>,
 * but e.g. HP-UX <ranlib.h> does not
 * define struct ranlib appropriately.
 * For the symbol table, ran_off contains the offset of the archive member
 * which contains the symbol definition.
 * For the common table, ran_off contains the required size of the common.
 */
#include <ranlib.h>
#else	/* !0 */
struct	ranlib {
	off_t	ran_strx;		/* string table index of symbol	*/
	off_t	ran_off;		/* offset of archive member	*/
};
#endif	/* !0 */

/* Macros. */
#define	AR_MAGIC_SIZE	member_size(ar_hdr, ar_fmag)
#define	AR_NAME_SIZE	member_size(ar_hdr, ar_name)
#define	AR_SIZE_SIZE	member_size(ar_hdr, ar_size)
#define	COMDEF		"__.COMDEF"		/* common table		*/
#define	COMDEF_PAD	"__.COMDEF       "
#define	LONGNAME_CHAR	'/'
#define	LONGNAMES	"//"			/* long names table	*/
#define	LONGNAMES_PAD	"//              "
#define	member_size(type, member)	(sizeof(((type *)0)->member))
#define	SYMDEF		"__.SYMDEF"		/* symbol table		*/
#define	SYMDEF_PAD	"__.SYMDEF       "

/* Functions. */
extern	ar_stat	ar_close(archive *ap);			/* close archive		*/
extern	ar_stat	ar_deletesym(archive *ap, char *sym);	/* delete symbol def		*/
extern	ar_stat	ar_findcom(archive *ap, char *sym, size_t *sizep);	/* find common def */
extern	module	*ar_findmod(archive *ap, char *path, int loadflag);	/* find module in archive */
extern	module	*ar_findsym(archive *ap, char *sym);	/* find module def'ing symbol	*/
extern	ar_stat	ar_open(char *path, archive **app);	/* open existing archive	*/
extern	void	module_free(module *mp);		/* free module			*/
extern	ar_stat	module_load(module *mp);		/* load module to memory	*/
extern	ar_stat	module_name(module *mp, char *name);	/* assign module name		*/

#endif /* archive_INCLUDED */
