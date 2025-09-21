/*  Use this file to add structures or typedefs that everyone needs. */
/*  This header is included in every module. */

/*

        $Header: //bliss/user/archive/rcs/4in1/fino.h,v 14.0 97/03/17 17:19:52 shawn Release $

        $Log:	fino.h,v $
 * Revision 14.0  97/03/17  17:19:52  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.2  96/08/19  13:05:44  shawn
 * Changed GetspGlobalPtr() function type to a 'void *'.
 * Added RCS/Header and Log statements.
 * 

*/

#ifndef fino_h
#define fino_h


#if   WINDOWS_4IN1
   
typedef struct
   {
      void (WDECL *sp_report_error)(PROTO_DECL2 int n);
      void (WDECL *sp_open_bitmap)(PROTO_DECL2 fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize);
      void (WDECL *sp_set_bitmap_bits)(PROTO_DECL2 fix15 y, fix15 xbit1, fix15 xbit2);
      void (WDECL *sp_close_bitmap)(PROTO_DECL1);
      buff_t FONTFAR* (WDECL *sp_load_char_data)(PROTO_DECL2 fix31 file_offset, fix15 no_bytes, fix15 cb_offset);
      btsbool (WDECL *get_byte)(char STACKFAR *next_char);
      unsigned char STACKFAR* (WDECL *dynamic_load)(ufix32 file_position, fix15 num_bytes, unsigned char success);
   }callback_struct;

extern   callback_struct   callback_ptrs;

/* structure that holds pointers to all callback functions. */
#endif /* WINDOWS_4IN1 */

#ifndef INCL_PCLETTO
#define INCL_PCLETTO	0
#endif /* ifndef INCL_PCLETTO */

#ifndef PFR_ON_DISK
/* support for single, file-based PFR */
#define PFR_ON_DISK	0	/* default (file_based) PFR RFS is read entirely into RAM */
#endif

#ifndef MULTIPLE_PFRS
/* support for multiple PFRs in ROM function, cspg_OpenMultCharShapeResource() in cspglue.c */
#define MULTIPLE_PFRS	1	/* default (NOT file-based) PFR RFS is enabled */
#endif

#if PROC_TRUEDOC
#if REENTRANT_ALLOC

#define TPS_GPTR1 void *pCspGlobals
#define TPS_GPTR2 TPS_GPTR1,
#define TPS_GLOB1 pCspGlobals
#define TPS_GLOB2 TPS_GLOB1,

#define CSP_PARAMS1	pCspGlobals
#define CSP_PARAMS2	pCspGlobals,
#define CSP_GDECL	void *pCspGlobals;
#define CSP_PROTO_DECL1	void *pCspGlobals
#define CSP_PROTO_DECL2	void *pCspGlobals,

GLOBAL_PROTO void *GetspGlobalPtr(void *pCspGlobals);

#else

#define TPS_GPTR1 void
#define TPS_GPTR2
#define TPS_GLOB1
#define TPS_GLOB2

#define CSP_PARAMS1
#define CSP_PARAMS2
#define CSP_GDECL
#define CSP_PROTO_DECL1 void
#define CSP_PROTO_DECL2
#endif
#else

#define TPS_GPTR1 PROTO_DECL1
#define TPS_GPTR2 PROTO_DECL2
#define TPS_GLOB1 PARAMS1
#define TPS_GLOB2 PARAMS2

#define CSP_PARAMS1 	PARAMS1
#define CSP_PARAMS2	PARAMS2
#define CSP_GDECL	GDECL
#define CSP_PROTO_DECL1	PROTO_DECL1
#define CSP_PROTO_DECL2	PROTO_DECL2
#endif

#endif /* ifndef fino_h */
