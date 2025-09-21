#ifndef	PROTO_H
#define	PROTO_H

#include <stdio.h>
#include <OS.h>

#include "gr_types.h"

extern server_color_map *system_cs;

#ifdef	__cplusplus
extern	"C" {
#endif

/****************************************************/
/*	Some globals... should be moved somewhere else, */
/*	or at least given good names					*/

extern	long		reroute_port;
extern	char		is_mac;

/********************/
/* Standard cursors */

extern	uchar		cross_cursor[];
extern	uchar		arrow_cursor[];
extern	uchar		ibeam_cursor[];
extern	uchar		mover_cursor[];
extern	uchar		finger_cursor[];
extern	uchar		dot_cursor[];
extern	uchar		empty_cursor[];
extern	uchar		watch_cursor[];
extern	uchar		hand_cursor[];

/******************************************/
/* Prototypes for code all over the place */
/* Should move thse into proper places    */

long		start_task(void *arg);
region		*calc_desk(void);

/********************************/
/* Memory allocation prototypes */

void		*gr_malloc(long);
void		gr_free(void *);
void		*sh_malloc_g(long);
void		*sh_realloc(void*, long);
void		sh_free_g(void *);

/******************************************/
/* A lot of crap that doesn't belong here */

void 		desk_fill(region *the_region);

frect		rect_to_frect(rect r);
rect		scale_rect(frect r, float scale);
void	xprintf(char *format, ...);
bool	any_screens(void);


#ifdef	__cplusplus
}
#endif


#endif
