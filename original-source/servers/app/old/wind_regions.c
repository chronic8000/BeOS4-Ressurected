//******************************************************************************
//
//	File:		wind_regions.c
//
//	Description:	special regions manipulation.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1993, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#include <stdio.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#ifndef	MACRO_H
#include "macro.h"
#endif

#ifndef	PROTO_H
#include "proto.h"
#endif

/*----------------------------------------------------------------*/
long	a_lefts[99];
long	a_rights[99];
long	b_lefts[99];
long	b_rights[99];
rect	outrects[MAXREGION];
/*----------------------------------------------------------------*/

void w_process_sub(region_a, region_b, top, bottom, result, hint_a, hint_b)
region	*region_a;
region	*region_b;
long	top;
long	bottom;

region	*result;
long	*hint_a;
long	*hint_b;
{
	long	max_a;
	long  	max_b;
	rect	r;
	long	a_left;
	long	a_right;
	long	b_left;
	long	b_right;
	long	left_open;
	long	right_open;
	long	left_out;
	long	right_out;
	long	*tmp_ptr;

	short	i1;
	short	ia;
	short	ib;
	short	result_idx;
	short	cpt_a;
	short	cpt_b;
	short	n_hint_a = -1;
	short	n_hint_b = -1;

	
	cpt_a = 0;
	cpt_b = 0;

	max_a = region_a->count;
	max_b = region_b->count;

	for (i1 = *hint_a; i1 < max_a; i1++) {
		r = ra(region_a, i1);
		if (r.top > bottom)
			break;

		if (r.bottom >= top) {
			
			if (n_hint_a == -1)
				n_hint_a = i1;
			
			if (max(r.top, top) <= min(r.bottom, bottom)) {
				a_lefts[cpt_a] = r.left;
				a_rights[cpt_a] = r.right;
				cpt_a++;
				if (cpt_a > 99) {
					debugger ("overflow in sub\n");
					printf("pp12\n");
					*((char *)0x00) = 0;
					return;
				}
			}
		}
	}


	for (i1 = *hint_b; i1 < max_b; i1++) {
		r = ra(region_b, i1);
		if (r.top > bottom)
			break;

		if (r.bottom >= top) {
			
			if (n_hint_b == -1)
				n_hint_b = i1;

			if (max(r.top, top) <= min(r.bottom, bottom)) {
				b_lefts[cpt_b] = r.left;
				b_rights[cpt_b] = r.right;
				cpt_b++;
				if (cpt_a > 99) {
					debugger ("overflow in sub\n");	
					printf("pp13\n");
					*((char *)0x00) = 0;
					return;
				}
			}
		}
	}

	if (n_hint_a != -1)
		*hint_a = n_hint_a;
	if (n_hint_b != -1)
		*hint_b=n_hint_b;


	if (cpt_a > 1)
		sort_trans(&a_lefts[0], &a_rights[0], cpt_a);
	if (cpt_b > 1)
		sort_trans(&b_lefts[0], &b_rights[0], cpt_b);

	/*now we've got the transitions ordered in left order */

	if (cpt_b == 0)
		return;


	ia = 0;
	ib = 0;
	while (ib < cpt_b) {
		b_left = b_lefts[ib];
		b_right = b_rights[ib];


		ib++;

		left_open = -2000000000;
		right_open = 2000000000;


		if (cpt_a != 0) {
			for (ia = 0; ia < cpt_a; ia++) {
				a_left = a_lefts[ia];
				a_right = a_rights[ia];
				right_open = a_left;

/* now the interval left_open,right_open defines a transparent window */
/* on this scan line						      */

				left_out = max(b_left, left_open + 1);
				right_out = min(b_right,right_open - 1);
			
			
				if (left_out <= right_out) {
					result_idx = result->count;
					tmp_ptr = (long *)&(ra(result, result_idx));
					*tmp_ptr++ = left_out;
					*tmp_ptr++ = top;
					*tmp_ptr++ = right_out;
					*tmp_ptr = bottom;
					result->count++;
				}
				left_open = a_right;
			}
			right_open = 2000000000;
			left_out = max(b_left, left_open + 1);
			right_out = min(b_right, right_open - 1);
			
			
			if (left_out <= right_out) {
				result_idx = result->count;
				tmp_ptr = (long *)&(ra(result, result_idx));
				*tmp_ptr++ = left_out;
				*tmp_ptr++ = top;
				*tmp_ptr++ = right_out;
				*tmp_ptr = bottom;

				result->count++;
			}

		}
		else {
			result_idx = result->count;

			tmp_ptr = (long *)&(ra(result, result_idx));
			*tmp_ptr++ = b_left;
			*tmp_ptr++ = top;
			*tmp_ptr++ = b_right;
			*tmp_ptr = bottom;

			result->count++;
		}
	}
}


/*----------------------------------------------------------------*/
long	ys[299];
long	origin[299];
/*----------------------------------------------------------------*/

void w_sub_region(region *region_a, region *region_b, region *result)
{

	long	a_count;
	long	b_count;
	long	max_a;
	long	max_b;
	long	idx;
	long	i;
	long	top;
	long	bottom;
	rect	tmp_rect;
	rect	sect;
	region*	temp;
	
	long	hint_a = 0;
	long	hint_b = 0;

	
	if ((region_a->type != 0x1234) ||
	    (region_b->type != 0x1234) ||
	    (result->type != 0x1234)) {
		debugger ("bad sub region\n");
		printf("pp14\n");
		*((char *)0x00) = 0;
		return;
	}

	// Swap a and b until we redo this code so
	// this fcn produces a-b instead of b-a
	temp = region_a;
	region_a = region_b;
	region_b = temp;

	sect_rect(region_a->bound, region_b->bound, &sect);

/* bug bug, in the past we where copying the region_b in that case */

	if (empty_rect(sect)) {
		copy_region(region_b, result);
		return;
	}

	result->count = 0;


	a_count = region_a->count;
	b_count = region_b->count;

	max_a = (a_count << 1);
	max_b = (b_count << 1);


	idx = -1;

	for (i = 0; i < a_count; i++) {
		tmp_rect = ra(region_a, i);
		if (tmp_rect.top > sect.bottom)
			goto skip;
		if (tmp_rect.bottom < sect.top)
			goto skip;
		if (tmp_rect.left > sect.right)
			goto skip;
		if (tmp_rect.right < sect.left)
			goto skip;
		idx++;
		if (idx > 299) {
			debugger ("overflow in sub_region\n");
			printf("pp15\n");
			*((char *)0x00) = 0;
			return;
		}
		ys[idx] = tmp_rect.top;
		origin[idx] = 0;
		idx++;
		if (idx > 299) {
			debugger ("overflow in sub_region\n");
			printf("pp16\n");
			*((char *)0x00) = 0;
			return;
		}
		ys[idx] = tmp_rect.bottom;
		origin[idx] = 1;
		skip:;
	}

	for (i = 0; i < b_count; i++) {
		tmp_rect = ra(region_b, i);
		idx++;
		if (idx > 299) {
			debugger ("overflow in sub_region\n");
			printf("pp17\n");
			return;
		}
		ys[idx] = tmp_rect.top;
		origin[idx] = 0;
		idx++;
		if (idx > 299) {
			debugger ("overflow in sub_region\n");
			printf("pp18\n");
			*((char *)0x00) = 0;
			return;
		}
		ys[idx] = tmp_rect.bottom;
		origin[idx] = 1;
	}

	idx++;

	sort_trans(&ys[0], &origin[0], idx);

	top = ys[0];

	for (i = 1;i < idx; i++) {
		bottom = ys[i];
		if (origin[i] == 0)
			bottom--;
		if (bottom >= top) {
			w_process_sub(region_a, region_b, top, bottom, result, &hint_a, &hint_b);
			top = bottom + 1;
		}		
	}
	cleanup_region(result);
}


/*----------------------------------------------------------------*/

void w_and_region1(region_a, region_b, result)
region	*region_a;
region	*region_b;
region	*result;

{
	long	i;
	long	a_idx;
	long	b_idx;
	long	a_count;
	long	b_count;
	long	out_count;
	rect	a_rect;
	rect	b_rect;
	rect	r_result;

	a_count = region_a->count;
	b_count = region_b->count;

	out_count=0;

	for (a_idx = 0; a_idx < a_count; a_idx++) {
		a_rect = ra(region_a, a_idx);

		for (b_idx=0; b_idx < b_count; b_idx++) {
			b_rect = ra(region_b, b_idx);

			r_result.top    = max(a_rect.top, b_rect.top);
			r_result.bottom = min(a_rect.bottom, b_rect.bottom);

			if (r_result.bottom >= r_result.top) {
				r_result.left = max(a_rect.left, b_rect.left);
				r_result.right = min(a_rect.right, b_rect.right);

				if (r_result.right >= r_result.left) {
					if (out_count >= (MAXREGION - 1 )) {
						debugger ("out count overflow");
						printf("pp20\n");
						*((char *)0x00) = 0;
						return;
					}
					outrects[out_count] = r_result;
					out_count++;
				}
			}
		}
	}
	sort_rects(&outrects[0], out_count);
	for (i=0;i < out_count; i++)
		ra(result, i) = outrects[i];
	result->count = i;
	cleanup_region(result);
}

/*----------------------------------------------------------------*/

void	w_and_region(region_a, region_b, result)
region	*region_a;
region	*region_b;
region	*result;
{
	rect	r_result;
	
		
	if ((region_a->type != 0x1234) ||
	    (region_b->type != 0x1234) ||
	    (result->type != 0x1234)) {
		debugger ("bad and region\n");
		return;
	}

	sect_rect(region_a->bound, region_b->bound, &r_result);
	if (empty_rect(r_result)) {
		result->count = 0;
		result->bound.top = 0;
		result->bound.left = 0;
		result->bound.bottom = -1;
		result->bound.right = -1;
		return;
	}

	if (region_a->count == 1) {
		r_result = ra(region_a, 0);
		if (enclose(&r_result, &(region_b->bound))) {	
			copy_region(region_b, result);
			return;
		}
	}

	if (region_b->count == 1) {
		r_result = ra(region_b, 0);
		if (enclose(&r_result, &(region_a->bound))) {	
			copy_region(region_a, result);
			return;
		}
	}

	if ((region_a->count == 1) && (region_b->count == 1))  {
		result->count = 1;
		sect_rect(region_a->bound, region_b->bound, &(result->bound));
		ra(result, 0) = result->bound;
		return;
	}

	w_and_region1(region_a, region_b, result);

	if ((region_a->type != 0x1234) ||
	    (region_b->type != 0x1234) ||
	    (result->type != 0x1234)) {
		debugger ("bad and region\n");
		printf("broken in and_region\n");
		*((char *)0x00) = 0;
		return;
	}
}


/*----------------------------------------------------------------*/

