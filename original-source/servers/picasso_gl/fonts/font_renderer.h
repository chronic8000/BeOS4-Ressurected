/* ++++++++++
	FILE:	4demo.h
	REVS:	$Revision: 1.8 $
	NAME:	pierre
	DATE:	Tue Feb 11 14:01:22 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _FONT_RENDERER_H
#define _FONT_RENDERER_H

#include "font_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

extern  FILE *font_fp;

/**********************************************************************/
/* This is used for the font engine glue in ./bitstream/4demo/        */

struct fc_renderer_context *fc_create_new_context();
void    fc_set_file(struct fc_renderer_context *ctxt, FILE *fp, uint16 token);
int64   fc_context_weight(struct fc_renderer_context *ctxt);
void    fc_init_diffusion_table();
int     fc_setup_tt_font(struct fc_renderer_context *ctxt, fc_context *spec,
						 int gray_level, int default_flag);
int     fc_setup_ps_font(struct fc_renderer_context *ctxt, fc_context *spec,
						 int gray_level, int default_flag);
int		fc_setup_stroke_font(struct fc_renderer_context *ctxt, fc_context *spec,
						 int gray_level, int default_flag);
void    fc_release_font(struct fc_renderer_context *ctxt);
int     fc_make_char(struct fc_renderer_context *ctxt,
					 void *char_desc, void *set, FC_GET_MEM get_mem);
int     fc_make_outline(struct fc_renderer_context *ctxt, void *char_desc, void **glyph);
void    fc_make_safety_char(struct fc_renderer_context *ctxt,
							float size, int bpp, void *set, FC_GET_MEM get_mem);
void    fc_process_edges(struct fc_renderer_context *ctxt, void *char_desc, fc_edge *edge);
void	fc_process_bbox(struct fc_renderer_context *ctxt, void *char_desc, fc_bbox *bbox);

#ifdef __cplusplus
}
#endif

#endif



















