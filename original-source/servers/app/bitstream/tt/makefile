########TRUETYPE MAKEFILE:################################
#
#       Unix/Makefile for demo program tt/ttsample
#
# $Header: //bliss/user/archive/rcs/true_type/true.mku,v 14.0 97/03/17 17:55:14 shawn Release $
#
# $Log:	true.mku,v $
#  Revision 14.0  97/03/17  17:55:14  shawn
#  TDIS Version 6.00
#  
#  Revision 10.3  96/08/20  13:32:49  shawn
#  Added RCS/Header keyword.
#  
#  Revision 10.2  96/07/03  12:19:46  shawn
#  Restored WhiteWriter and QuickWriter support.
#  
#  Revision 10.1  96/06/05  15:31:15  shawn
#  Added support for the GrayWriter.
#  
#  Revision 10.0  95/02/15  16:26:03  roberte
#  Release
#  
#  Revision 9.1  95/01/04  16:39:27  roberte
#  Release
#  
#  Revision 8.1  94/11/14  12:17:02  roberte
#  Rewrite for new includes sub-directory distribution disk layout.
#  
#  Revision 8.0  94/05/04  09:34:41  roberte
#  Release
#  
#  Revision 7.0  94/04/08  11:59:54  roberte
#  Release
#  
#  Revision 6.92  94/03/11  15:58:00  roberte
#   Change path from defunct ninja to bliss.
#  
#  Revision 6.91  93/08/30  14:53:50  roberte
#  Release
#  
#  Revision 6.45  93/04/16  11:13:39  roberte
#  Removed a -DAPOLLO from defines.
#  Changed pathname dependencies of useropt.h to use ../speedo/useropt.h
#  
#  Revision 6.44  93/03/15  13:23:27  roberte
#  Release
#  
#  Revision 1.3  93/03/04  11:54:55  roberte
#  Added define of Math_Lib and used throughout.  Replaced "ld" with "cc"
#  Replaced $* macros with pathnames macros preceeding with actual file root name.
#  
#  Revision 1.2  93/02/08  15:26:19  roberte
#  Removed sc.o from tt_objs.
#  
#  Revision 1.1  93/02/02  14:43:05  roberte
#  Initial revision
#  
#


# DEBUG options
#debug = -g
debug = -O

# DIRECTORIES
spd_dir  = ../speedo
incl_dir = ../includes
includes = -I$(incl_dir)

Math_Lib = -lm


# TrueType defines
defines = -DSHORT_LISTS=0 -DNO_PROCS -DPROC_TRUETYPE -DINCL_ALL_WRITERS

#
core_objs =  out_blk.o out_bl2d.o out_scrn.o out_outl.o out_wht.o out_gray.o out_util.o out_qck.o

#
tt_objs = tt_iface.o tt_specs.o fontscal.o fsglue.o fnt.o sfntd.o \
                mapstrng.o newscan.o fixed_pt.o

#
ttsample : truetype.o $(tt_objs) $(core_objs)
	cc -o $@ truetype.o $(tt_objs) $(core_objs) $(Math_Lib)

truetype.o: truetype.c \
		$(incl_dir)/useropt.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fscdefs.h \
		$(incl_dir)/fontscal.h \
		$(incl_dir)/sfnt.h \
		$(incl_dir)/fserror.h \
                $(incl_dir)/truetype.h
	cc $(debug) $(defines) $(includes) -c $*.c

sc.o: sc.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h\
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fscdefs.h \
		$(incl_dir)/sc.h \
		$(incl_dir)/fserror.h
	cc $(debug) $(defines) $(includes) -c $*.c

tt_iface.o: tt_iface.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h \
		$(incl_dir)/fscdefs.h \
		$(incl_dir)/fontscal.h \
                $(incl_dir)/sfnt.h \
		$(incl_dir)/fserror.h \
		$(incl_dir)/truetype.h
	cc $(debug) $(defines) $(includes) -c $*.c

tt_specs.o: tt_specs.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h \
		$(incl_dir)/fscdefs.h \
                $(incl_dir)/fontscal.h \
		$(incl_dir)/truetype.h
	cc $(debug) $(defines) $(includes) -c $*.c

fontscal.o: fontscal.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fserror.h \
		$(incl_dir)/fscdefs.h \
		$(incl_dir)/fontmath.h \
		$(incl_dir)/sfnt.h \
		$(incl_dir)/sc.h \
                $(incl_dir)/fnt.h \
		$(incl_dir)/fontscal.h \
		$(incl_dir)/fsglue.h \
		$(incl_dir)/privsfnt.h \
		$(incl_dir)/privfosc.h
	cc $(debug) $(defines) $(includes) -c $*.c

fsglue.o: fsglue.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fserror.h \
		$(incl_dir)/fscdefs.h \
		$(incl_dir)/fontmath.h \
		$(incl_dir)/sfnt.h \
		$(incl_dir)/sc.h \
                $(incl_dir)/fnt.h \
		$(incl_dir)/fontscal.h \
		$(incl_dir)/fsglue.h \
		$(incl_dir)/privsfnt.h
	cc $(debug) $(defines) $(includes) -c $*.c

fnt.o: fnt.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
		$(incl_dir)/fscdefs.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fontmath.h \
		$(incl_dir)/sc.h \
		$(incl_dir)/fnt.h \
		$(incl_dir)/fserror.h
	cc $(debug) $(defines) $(includes) -c $*.c

sfntd.o: sfntd.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fserror.h \
		$(incl_dir)/fscdefs.h \
		$(incl_dir)/fontmath.h \
		$(incl_dir)/sfnt.h \
		$(incl_dir)/fnt.h \
                $(incl_dir)/sc.h \
		$(incl_dir)/fontscal.h \
		$(incl_dir)/fsglue.h \
		$(incl_dir)/privsfnt.h \
		$(incl_dir)/mapstrng.h
	cc $(debug) $(defines) $(includes) -c $*.c

mapstrng.o: mapstrng.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fscdefs.h \
		$(incl_dir)/mapstrng.h
	cc $(debug) $(defines) $(includes) -c $*.c

newscan.o: newscan.c \
		$(incl_dir)/useropt.h \
		$(incl_dir)/fino.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/spdo_prv.h \
		$(incl_dir)/fscdefs.h \
		$(incl_dir)/fontscal.h
	cc $(debug) $(defines) $(includes) -c $*.c

fixed_pt.o: fixed_pt.c \
		$(incl_dir)/fino.h \
		$(incl_dir)/fixed.h \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h
	cc $(debug) $(defines) $(includes) -c $*.c

out_blk.o: $(spd_dir)/out_blk.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_blk.c

out_wht.o: $(spd_dir)/out_wht.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_wht.c

out_qck.o: $(spd_dir)/out_qck.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_qck.c

out_bl2d.o: $(spd_dir)/out_bl2d.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_bl2d.c

out_gray.o: $(spd_dir)/out_gray.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_gray.c

out_scrn.o: $(spd_dir)/out_scrn.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_scrn.c

out_outl.o: $(spd_dir)/out_outl.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_outl.c

out_util.o: $(spd_dir)/out_util.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/fino.h
	cc $(debug) $(defines) $(includes) -c $(spd_dir)/out_util.c


clean:
	rm *.o

