#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif


/*--------------------------------------------------------*/

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

extern	void	fc_draw_string8();

/*--------------------------------------------------------*/

extern	void	line_8();
extern	void	vert_line_8();
extern	void	vert_line_pattern_8();
extern	void	horiz_line_pattern_8();
extern	void	horiz_line_8();
extern	void	rect_fill_8();
extern	void	run_list_8();
extern	void	run_list_pattern_8();

/*--------------------------------------------------------*/

PVRF	*gr_procs_8[] = {
			(PVRF *)line_8,
			(PVRF *)vert_line_8,
			(PVRF *)horiz_line_8,
			(PVRF *)vert_line_pattern_8,
			(PVRF *)horiz_line_pattern_8,
			(PVRF *)rect_fill_8,
			(PVRF *)run_list_8,
			(PVRF *)fc_draw_string8,
			(PVRF *)run_list_pattern_8
	};






