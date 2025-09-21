#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif


#define	VECTOR_RTN			0
#define	VERT_VECT_RTN		1
#define	HORIZ_VECT_RTN		2
#define	VERT_VECT_RTN_PAT	3
#define	HORIZ_VECT_RTN_PAT	4
#define	RECT_FILL_PAT		5
#define	RUN_LIST			6
#define	TEXT				7
#define RUN_LIST_PAT        8

/*--------------------------------------------------------*/

extern	void	fc_draw_string24();
extern	void	line_24();
extern	void	vert_line_24();
extern	void	vert_line_pattern_24();
extern	void	horiz_line_pattern_24();
extern	void	horiz_line_24();
extern	void	rect_fill_24();
extern	void	run_list_24();
extern	void	run_list_pattern_24();

/*--------------------------------------------------------*/

PVRF	*gr_procs_24[] = {
			(PVRF *)line_24,
			(PVRF *)vert_line_24,
			(PVRF *)horiz_line_24,
			(PVRF *)vert_line_pattern_24,
			(PVRF *)horiz_line_pattern_24,
			(PVRF *)rect_fill_24,
			(PVRF *)run_list_24,
			(PVRF *)fc_draw_string24,
			(PVRF *)run_list_pattern_24
	};











