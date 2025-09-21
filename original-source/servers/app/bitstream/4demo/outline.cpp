#include "path.h"
#include "be_glue.h"
#include <Debug.h>

static void record_max(float *bbox, float h, float v) {
	if (h < bbox[0])
		bbox[0] = h;
	if (h > bbox[2])
		bbox[2] = h;
	if (v < bbox[1])
		bbox[1] = v;
	if (v > bbox[3])
		bbox[3] = v;
}

/***********************************************************************************/
/* extension for outline output module. */ 
void sp_open_outline_wrap(	SPEEDO_GLOBALS *sp_global_ptr, 
										fix31	x_width,
										fix31	y_width,
										fix31	xmin,
										fix31	xmax,
										fix31	ymin,
										fix31	ymax) {
	SPath	*path;

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL)
		path->Clear();
	else {
		float		*bbox;
		
		bbox = ((fc_renderer_context*)(sp_global_ptr->UserPtr))->bbox;
		bbox[0] = 1000000;
		bbox[1] = 1000000;
		bbox[2] = -1000000;
		bbox[3] = -1000000;
	}
}

void sp_start_new_char_wrap(SPEEDO_GLOBALS *sp_global_ptr) {
	SPath	*path;
	fpoint	pt;

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL) {
		pt.h = 0.0;
		pt.v = 0.0;
		path->OpenShapes(pt);
	}
}

void sp_start_contour_wrap(	SPEEDO_GLOBALS *sp_global_ptr,
										fix31	x,
										fix31	y,
										char	outside) {
	SPath	*path;
	fpoint	pt;

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL) {
		pt.h = ((float)x)*(1/65536.0);
		pt.v = ((float)y)*(1/65536.0);
		path->MoveTo(pt, true);
	}
	else 
		record_max(((fc_renderer_context*)(sp_global_ptr->UserPtr))->bbox,
				   (float)x*(1.0/65536.0), (float)y*(1.0/65536.0));
}

void sp_curve_to_wrap(	SPEEDO_GLOBALS *sp_global_ptr,
									fix31	x1,
									fix31	y1,
									fix31	x2,
									fix31	y2,
									fix31	x3,
									fix31	y3) {
	SPath	*path;
	fpoint	pt[3];

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL) {
		pt[0].h = ((float)x1)*(1/65536.0);
		pt[0].v = ((float)y1)*(1/65536.0);
		pt[1].h = ((float)x2)*(1/65536.0);
		pt[1].v = ((float)y2)*(1/65536.0);
		pt[2].h = ((float)x3)*(1/65536.0);
		pt[2].v = ((float)y3)*(1/65536.0);
		path->AddBezierSegments(pt, 1);
	}
}

void sp_line_to_wrap(	SPEEDO_GLOBALS *sp_global_ptr,
								fix31	x,
								fix31	y) {
	SPath	*path;
	fpoint	pt;

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL) {
		pt.h = ((float)x)*(1/65536.0);
		pt.v = ((float)y)*(1/65536.0);
		path->AddLineSegments(&pt, 1);
	}
	else 
		record_max(((fc_renderer_context*)(sp_global_ptr->UserPtr))->bbox,
				   (float)x*(1.0/65536.0), (float)y*(1.0/65536.0));
}

void sp_close_contour_wrap(SPEEDO_GLOBALS *sp_global_ptr) {
}

void sp_close_outline_wrap(SPEEDO_GLOBALS *sp_global_ptr) {
	SPath	*path;

	path = (SPath*)(((fc_renderer_context*)(sp_global_ptr->UserPtr))->glyph_path);
	if (path != NULL)
		path->CloseShapes(true);
}

/* character outline rendering entry */
extern "C" int fc_outline_call_glue(struct fc_renderer_context *ctxt, void *char_desc);

int fc_make_outline(struct fc_renderer_context *ctxt, void *char_desc, void **glyph) {
	int			ret_val;
	
	ctxt->gspecs.flags = MODE_OUTLINE | CURVES_OUT;		
	ctxt->mode_id = MODE_OUTLINE;
	ctxt->error = 0;
	ctxt->glyph_path = new SPath(NULL);

	ret_val = fc_outline_call_glue(ctxt, char_desc);

	*glyph = (void*)ctxt->glyph_path;
	ctxt->glyph_path = NULL;
	
	return ret_val;
}

