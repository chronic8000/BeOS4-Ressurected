#ifndef	_DEF_BT_CONFIG_H

/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.
*/


/*
	THIS SHOULD NOT BE INCLUDED BY USER-LEVEL PROGRAMS

	All reasonably user-configurable options are in this file.

	If there is something that needs to be special-cased for
	a system, put it here, PLEASE!
*/


/*
WARNING - there is one option that *should* be there but is in
btree.h because it needs to be seen by user-level programs. 
That is the definition on bt_chrp, which is at the top of the
file. If you have a weird architecture, you may want to look
at that value.
*/


/*
SEEK_SET should be whatever your systems version of lseek() takes
to tell it to go to an exact offset. zero is pretty standard.
*/
#ifndef	SEEK_SET
#define	SEEK_SET	0
#endif

/*
if this option is turned on the bt_dump() page dump function will
be generated. it probably should be left out of public libraries
but is really handy, and it would be stupid to have to rewrite it.

#undef	YES_BT_DEBUG
*/
#define	YES_BT_DEBUG	1


/*
apparently not all compilers handle register declarations intelligently.
the functions in btpage1.c and btpage2.c have register declarations, 
sometimes as many as six. if your compiler cannot handle them, just
redefine REGISTER to be nothing.

#define	REGISTER
*/
#define	REGISTER register


/*
choose one of these as your favorite memory copying function
can you believe 3 functions, 3 names, all do the same thing ?
*/
#define	USE_MEMCPY	1
/* #define	USE_MOVEMEM	1 */

/* #define	USE_BCOPY	1 */

/*
if the operating system the software will be running under supports
the ftruncate(2) system call to free a file's allocated space after
a certain length, turn this option on, and bt_zap() will free extra
space when called.
*/
/* #define	USE_FTRUNCATE	0 */

/* #define	USE_FTRUNCATE	1 */

/*
the default page size. usually use BUFSIZ or maybe (BUFSIZ * 2)
considerations are:: as buffer size increases, memory use for
cache buffers increases. on the other hand, speed increases,
depending on the size of the tree/keys/data type. disk wastage
also increases, since it is possible there may be partial pages
taking up more space than would be taken up with smaller pages.
smaller pages are a win if speed is less important than disk
wastage, though decreasing page size and boosting cache may
offset speed losses. this is not something that can be guessed
at, so use BUFSIZ as a default because presumably it is good
for the system.
*/
#define	BT_DFLTPSIZ	BUFSIZ

/*
default magic number to use, unless a user supplies us one.
this can come in handy if you add it to your file(1) types
database.
*/
#define	BT_DFLTMAGIC	0x69f6c2e8L

/*
default number of cache pages to ADD to the minimum page cache
do NOT simply edit this value here, since it can be set in
calls to bt_optopen() using BT_CACHE on a case-by-case basis.
remember also that there are (about 4) cache buffers that will
be allocated no matter what, so this value is in addition to
the minimum cache. the minimum cache buffers are used for
splitting and promotion as well as page buffers, depending
on what is being done. cache size can be overruled upwards
by the user, so a sensible value is good here.
*/
#define	BT_DFLTCACHE	1

/*
default type of caching: read only - IE pages will always write to
disk when modified, but may be read out of cache directly. using
read-only caching is the safe bet. whatever default is selected
here can be overruled by the user, so a safe value is OK here.
*/ 
#define	BT_DFLTCFLAG	BT_ROCACHE

/*
default data type to store: strings are a pretty good bet. this can
be reset anyhow in bt_optopen(), so leaving it alone is recommended.
WARNING - do not set this to be BT_USRDEF! bt_open() does not have
any way of setting the function pointer for the comparison function.
using the BT_STRING data type should be encouraged anyhow, since 
string keys can be prefixed for greater efficiency.
*/
#define	BT_DFLTDTYPE	BT_STRING

/*
that is all. this file does not need to be installed where a user
can include it.
*/

#define	_DEF_BT_CONFIG_H
#endif

#define		u_long	long
#include	<fcntl.h>
