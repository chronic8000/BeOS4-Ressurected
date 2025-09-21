/*************************** truetype.h  *********************************
 *                                                                           *
 *   TrueType processor interface definitions
 *                                                                           */
/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/truetype.h,v 14.1 97/05/06 10:56:04 shawn Exp $                                                                       *
*                                                                                    *
*     $Log:	truetype.h,v $
 * Revision 14.1  97/05/06  10:56:04  shawn
 * Added conditional prototype statement for 'tt_get_font_size()'.
 * 
 * Revision 14.0  97/03/17  17:55:24  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.2  96/07/02  13:50:09  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.1  95/06/19  10:35:35  tamari
 * Add prototypes for NEEDTRACEFUNC for tt_set_specs, tt_make_char_idx.
 * 
 * Revision 10.0  95/02/15  16:26:18  roberte
 * Release
 * 
 * Revision 9.2  95/01/09  13:16:28  shawn
 * Changed type of tt_get_font_fragment() from (GetSFNTFunc) to (void *)
 * 
 * Revision 9.1  95/01/04  16:39:56  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  12:28:30  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/07/19  10:51:54  roberte
 *  Added function prototypes for make_char functions for vertical writing support.
 * Added function prototypes for pair kerning functions, if INCL_METRICS.
 * 
 * Revision 8.0  94/05/04  09:35:07  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  12:00:08  roberte
 * Release
 * 
 * Revision 6.94  94/04/01  16:06:38  roberte
 * Employed new PROTOTYPE macro for all function prototypes.
 * 
 * Revision 6.93  94/03/23  10:54:04  roberte
 * Added prototype for new entry point tt_get_char_width_orus() and tt_get_char_width_idx_orus().
 * 
 * Revision 6.92  93/10/22  15:15:25  roberte
 * Changed prototype of split_Qbez parameters Ax - Cy to int32
 * 
 * Revision 6.91  93/08/30  14:54:05  roberte
 * Release
 * 
 * Revision 6.45  93/07/28  13:34:09  mark
 * fix prototypes for modified split_Qbez function
 * 
 * Revision 6.44  93/03/15  13:23:47  roberte
 * Release
 * 
 * Revision 6.10  93/03/04  11:56:18  roberte
 * Added prototype of tt_load_font_params() function.
 * 
 * Revision 6.9  93/02/24  17:37:03  weili
 * Uncommented tt_get_font_fragment(), found problem in fscdefs.h.
 * 
 * Revision 6.8  93/02/24  17:22:31  roberte
 * commented out prototype of tt_get_font_fragment.  This is a problem.
 * 
 * Revision 6.7  93/02/24  16:57:23  roberte
 * Added #ifndef truetype_h flag so file can be re-included.
 * 
 * Revision 6.6  93/01/26  13:37:15  roberte
 * Added PROTO_DECL1 and PROTO_DECL2 macros in support of all reentrant function prototypes.
 * 
 * Revision 6.5  93/01/22  15:22:38  roberte
 * Changed all prototypes to use new PROTO macro.
 * 
 * Revision 6.4  93/01/08  12:17:09  roberte
 * Redefined return value of tt_release_font to ReleaseSFNTFunc rather than void.
 * They are equivalent..
 * 
 * Revision 6.3  92/11/19  16:10:29  roberte
 * Release
 * 
 * Revision 6.1  91/08/14  16:49:16  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:30:21  mark
 * Release
 * 
 * Revision 4.2  91/08/07  12:02:53  mark
 * add rcs control strings
 * 
*************************************************************************************/


#ifndef truetype_h
#define truetype_h

GLOBAL_PROTO void *tt_get_font_fragment(int32 clientID, int32 offset, int32 length);
GLOBAL_PROTO void tt_release_font_fragment(void *ptr);

#ifdef VAL_TTDIR
GLOBAL_PROTO int32 tt_get_font_size(int32 clientID);
#endif

GLOBAL_PROTO btsbool tt_reset(SPGLOBPTR1);
GLOBAL_PROTO btsbool tt_load_font(SPGLOBPTR2 int32 fontHandle);
GLOBAL_PROTO btsbool tt_load_font_params(SPGLOBPTR2 int32 fontHandle, uint16 platID, uint16 specID);

#ifdef NEEDTRACEFUNC
GLOBAL_PROTO btsbool tt_set_specs(SPGLOBPTR2 specs_t *pspecs, void(* traceFunc)());
GLOBAL_PROTO btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, void(* traceFunc)());
#else /* ifdef NEEDTRACEFUNC */
GLOBAL_PROTO btsbool tt_set_specs(SPGLOBPTR2 specs_t *pspecs);
GLOBAL_PROTO btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx);
#endif /* ifdef NEEDTRACEFUNC */

GLOBAL_PROTO btsbool tt_make_char(SPGLOBPTR2 ufix16 char_code);

#if INCL_VERTICAL
GLOBAL_PROTO btsbool tt_make_vert_char(SPGLOBPTR2 ufix16 char_code);
GLOBAL_PROTO btsbool tt_make_vert_char_idx(SPGLOBPTR2 ufix16 char_idx);
#endif

#if INCL_METRICS
GLOBAL_PROTO fix31 tt_get_char_width_idx(SPGLOBPTR2 ufix16 char_index);
GLOBAL_PROTO fix31 tt_get_char_width(SPGLOBPTR2 ufix16 char_code);
GLOBAL_PROTO fix31 tt_get_char_width_idx_orus(SPGLOBPTR2 ufix16 char_index, ufix16 *emResolution);
GLOBAL_PROTO fix31 tt_get_char_width_orus(SPGLOBPTR2 ufix16 char_code, ufix16 *emResolution);
#endif /* INCL_METRICS */

GLOBAL_PROTO btsbool tt_release_font(SPGLOBPTR1);
GLOBAL_PROTO int16 tt_rendercurve(SPGLOBPTR2 F26Dot6 Ax, F26Dot6 Ay, F26Dot6 Bx, F26Dot6 By, F26Dot6 Cx, F26Dot6 Cy);
GLOBAL_PROTO void split_Qbez(SPGLOBPTR2 int32 Ax, int32 Ay, int32 Bx, int32 By, int32 Cx, int32 Cy, fix15 depth);
GLOBAL_PROTO void dump_bitmap(SPGLOBPTR2 BitMap *bitmap, point_t *Psw);

#if INCL_APPLESCAN
#define MODE_APPLE MODE_2D+1
#endif

GLOBAL_PROTO btsbool tt_set_spglob(SPGLOBPTR2 specs_t *specsarg, fix15 font_xmin, fix15 font_ymin,
								   fix15 font_xmax, fix15 font_ymax, fix15 orupm);

#if INCL_METRICS
GLOBAL_PROTO fix15 tt_get_pair_kern(SPGLOBPTR2 ufix16 char_code1, ufix16 char_code2);
GLOBAL_PROTO fix15 tt_get_pair_kern_idx(SPGLOBPTR2 ufix16 char_index1, ufix16 char_index2);
#endif /* INCL_METRICS */

#endif /* truetype_h */
