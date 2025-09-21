############### TYPE1 MAKEFILE:###################
#
# Unix make file to make nsample which reads Type 1 fonts:
#
#
# $Header: //bliss/user/archive/rcs/type1/type1.mku,v 33.0 97/03/17 18:00:41 shawn Release $
#
# $Log:	type1.mku,v $
#  Revision 33.0  97/03/17  18:00:41  shawn
#  TDIS Version 6.00
#  
#  Revision 32.4  96/08/20  13:38:36  shawn
#  Added RCS/Header keyword.
#  
#  Revision 32.3  96/07/03  14:20:37  shawn
#  Removed -DAPOLLO conditional.
#  
#  Revision 32.2  96/07/03  12:24:26  shawn
#  Restored QuickWriter support.
#  
#  Revision 32.1  96/06/05  15:05:29  shawn
#  Added support for the GrayWriter.
#  Added more compile options.
#  
#  Revision 32.0  95/02/15  16:47:44  shawn
#  Release
#  
#  Revision 31.2  95/02/15  14:29:00  shawn
#  Inserted a space between -DINCL_2D and -DINCL_OUTLINE
#  
#  Revision 31.1  95/01/04  16:41:36  roberte
#  Release
#  
#  Revision 30.1  94/11/14  13:10:18  roberte
#  Rewrite for new includes sub-directory distribution disk layout.
#  
#  Revision 30.0  94/05/04  09:35:45  roberte
#  Release
#  
#  Revision 29.0  94/04/08  12:11:43  roberte
#  Release
#  
#  Revision 28.31  94/04/07  16:21:53  roberte
#  Changed name of old compile-time flag for Type1 from PROTOTYPE to T1_PROTOTYPE. Conflict with new 4in1 option.
#  
#  Revision 28.30  93/08/30  15:46:07  roberte
#  Release
#  
#  Revision 28.26  93/04/23  17:57:30  roberte
#  Corrected some line continuations that should have been commented out.
#  
#  Revision 28.25  93/04/16  11:14:57  roberte
#  Changed pathname dependencies of useropt.h to ../speedo/useropt.h
#  
#  Revision 28.24  93/03/15  13:12:44  roberte
#  Release
#  
#  Revision 1.4  93/02/23  12:20:16  roberte
#  Added define of Math_Lib, changed link statement to call cc rather than ld. Added Math_Lib to link list.
#  Removed -g flag from debug_defines. 
#  
#  Revision 1.3  93/02/17  11:39:11  roberte
#  Removed TAB from end of line following BACKSLASH in out_bl2d.o build command.
#  
#  Revision 1.2  93/02/11  14:27:07  roberte
#  Removed -Tbsd4.2 "unix_define" command line flag.
#  
#  Revision 1.1  93/02/02  14:40:32  roberte
#  Initial revision
#  
# Revision 28.15  93/01/21  13:18:30  roberte
# Turned on -g debug option.  Added clean: target to remove * o's.
#
# Revision 28.14  93/01/19  10:37:48  davidw
# *** empty log message ***
#
# Revision 28.13  93/01/14  11:00:47  roberte
# Changed definitions of type1_dir, speedo_dir and 4in1_dir
# to archive path.
# Added include of type1_dir to compiles of all speedo
# modules.
#
# Revision 28.12  93/01/11  13:20:12  roberte
# Moved -D's of PROC_PCL and PROC_TRUETYPE to macro
# speedo_defines.
#
# Revision 28.11  93/01/11  12:10:30  roberte
# Changed command line defines -D options to shut off
#  PROC_PCL and
#  PROC_TRUETYPE.
#
# Revision 28.10  92/12/08  09:39:39  davidw
# *** empty log message ***
#
# Revision 28.9  92/12/02  17:49:30  laurar
# add speedo.h to tr_trans.obj dependency list.
#
# Revision 28.8  92/12/02  15:08:58  laurar
# update dependency list.
#
#  Revision 28.8  92/11/24  13:13:17  laurar
#  add 4in1 as an include directory, so make can find fino.h
#
#  Revision 28.7  92/11/19  15:23:17  weili
#  Release
#
#  Revision 26.4  92/11/16  18:33:42  laurar
#  define UNIX under the unix_defines variable.
#
#  Revision 26.3  92/11/16  12:13:38  laurar
#  Add UNIX define.
#
#  Revision 26.2  92/10/19  17:36:54  laurar
#  change fnt.h to fnt_a.h for nsample dependency.
#
#  Revision 26.1  92/06/26  10:22:01  leeann
#  Release
#
#  Revision 25.1  92/04/06  11:38:44  leeann
#  Release
#
#  Revision 24.1  92/03/23  14:06:11  leeann
#  Release
#
#  Revision 23.2  92/03/23  14:05:07  leeann
#  spell out source file names in "cc" command line
#
#  Revision 23.1  92/01/29  16:57:22  leeann
#  Release
#
#  Revision 22.2  92/01/29  16:54:11  leeann
#  add T1_PROTOTYPE turned on when NAME_STRUCT is turned on
#
#  Revision 22.1  92/01/20  13:28:36  leeann
#  Release
#
#  Revision 21.1  91/10/28  16:40:58  leeann
#  Release
#
#  Revision 20.1  91/10/28  15:24:43  leeann
#  Release
#
#  Revision 19.2  91/10/28  15:24:10  leeann
#  Release
#
#  Revision 19.1  91/10/23  17:27:23  leeann
#  Release
#
#  Revision 18.1  91/10/17  11:36:22  leeann
#  Release
#
#  Revision 17.1  91/06/13  10:40:54  leeann
#  Release
#
#  Revision 16.1  91/06/04  15:31:30  leeann
#  Release
#
#  Revision 15.1  91/05/08  18:04:02  leeann
#  Release
#
#  Revision 14.1  91/05/07  16:25:05  leeann
#  Release
#
#  Revision 13.1  91/04/30  16:59:38  leeann
#  Release
#
#  Revision 12.1  91/04/29  14:50:38  leeann
#  Release
#
#  Revision 11.5  91/04/29  14:27:34  leeann
#  make paths relative to current directory
#
#  Revision 11.3  91/04/10  13:22:41  leeann
#  cleanup
#
#  Revision 11.2  91/04/04  10:53:42  leeann
#  Release
#
#  Revision 11.1  91/04/04  10:53:08  leeann
#  Release
#
#  Revision 10.2  91/04/04  10:51:21  leeann
#  EDGE_ALIGN is an option
#  .'
#
#  Revision 10.1  91/03/14  14:23:50  leeann
#  Release
#
#  Revision 9.1  91/03/14  10:02:24  leeann
#  Release
#
#  Revision 8.3  91/03/13  17:37:33  leeann
#  add RESTRICTED_ENVIRON defines
#
#  Revision 8.2  91/02/12  13:09:02  leeann
#  put in perf_test
#
#  Revision 8.1  91/01/30  18:59:14  leeann
#  Release
#
#  Revision 7.1  91/01/22  14:22:47  leeann
#  Release
#
#  Revision 6.2  91/01/17  17:01:11  joyce
#  *** empty log message ***
#
#  Revision 6.1  91/01/16  10:49:43  leeann
#  Release
#
#  Revision 5.1  90/12/12  17:16:39  leeann
#  Release
#
#  Revision 4.1  90/12/12  14:42:14  leeann
#  Release
#
#  Revision 3.2  90/12/12  14:13:01  leeann
#  remove iface compilation
#
#  Revision 3.1  90/12/06  10:24:51  leeann
#  Release
#
#  Revision 2.1  90/12/03  12:53:30  mark
#  Release
#
#  Revision 1.4  90/11/30  12:49:22  joyce
#  Another name change
#  tr_mk_chr.c to tr_mkchr.c
#  tr_ld_fnt. to tr_ldfnt.c
#  The previous names are too long for the PC
#
#  Revision 1.3  90/11/30  12:16:46  joyce
#  renamed four C files and two header files:
#  1. chr_nmes.c to tr_names.c
#  2. ld_fnt_a.c to tr_ld_fnt.c
#  3. mk_chr_a.c to tr_mk_chr.c
#  4. trans_a.c to tr_trans.c
#  5. fnt_a.h to fnt.h
#  6. ps_qem.h to type1.h
#  This is the makefile that should be used to build
#  the type1 rasterizer. The other makefile, "makefile"
#  contains obsolete file names.
#
#  Revision 1.2  90/11/29  18:03:02  leeann
#  simplify makefile
#
#  Revision 1.1  90/08/13  15:25:07  arg
#  Initial revision
#
#


# Debugger option
debug = -O
#debug = -g
# Profiler debug
#debug = -g -pg

#DIRECTORIES
spdo_dir = ../speedo
incl_dir = ../includes
incl     = -I$(incl_dir)

#
Math_Lib = -lm

#
unix_defs = -c

#
spdo_defs = -DINCL_ALL_WRITERS -DINCL_KEYS -DSHORT_LISTS=0 -DNO_PROCS -DPROC_TYPE1


#
# Optional Configurations (pick 1)
type1_defs =  -DINCL_PFB
#type1_defs = -DINCL_PFB -DRESTRICTED_ENVIRON
#type1_defs = -DINCL_PFB -DRESTRICTED_ENVIRON -DTIMEIT
#type1_defs = -DINCL_PFB -DEDGE_ALIGN=1 -DNAME_STRUCT -DT1_PROTOTYPE
#type1_defs = -DINCL_PFB -DTIMEIT
#type1_defs = -DINCL_PFB -DRESTRICTED_ENVIRON -DTIMEIT
#type1_defs = -DRESTRICTED_ENVIRON -DINCL_PFB -DRESTRICTED_STATS

#
type1_objs = tr_names.o tr_ldfnt.o tr_mkchr.o tr_trans.o

#
speedo_objs = out_util.o out_outl.o out_blk.o out_wht.o out_scrn.o out_gray.o out_bl2d.o out_qck.o

#
nsample_objs = $(speedo_objs) $(type1_objs) nsample.o

#
perf_objs = $(speedo_objs) $(type1_objs) perf_test.o

#
nsample : ${nsample_objs}
	cc -o $@ ${nsample_objs} ${Math_Lib}
perf_test: ${perf_objs}
	cc -o $@ ${perf_objs}
#profile load
#	cc -pg -o $@ ${nsample_objs}

#
#TYPE 1
#
tr_names.o: tr_names.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/fino.h \
                $(incl_dir)/type1.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(type1_defs) $(incl) tr_names.c

tr_ldfnt.o: tr_ldfnt.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/fino.h \
                $(incl_dir)/type1.h \
                $(incl_dir)/fnt_a.h \
                $(incl_dir)/errcodes.h \
                $(incl_dir)/tr_fdata.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(type1_defs) $(incl) tr_ldfnt.c

tr_mkchr.o: tr_mkchr.c \
            	$(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h\
            	$(incl_dir)/type1.h \
            	$(incl_dir)/fnt_a.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(type1_defs) $(incl) tr_mkchr.c

tr_trans.o: tr_trans.c  \
                $(incl_dir)/speedo.h\
                $(incl_dir)/fino.h \
                $(incl_dir)/type1.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(type1_defs) $(incl) tr_trans.c
#
# SPEEDO
#
out_util.o: $(spdo_dir)/out_util.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_util.c

out_blk.o:  $(spdo_dir)/out_blk.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_blk.c

out_wht.o:  $(spdo_dir)/out_wht.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_wht.c

out_qck.o:  $(spdo_dir)/out_qck.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_qck.c

out_outl.o:  $(spdo_dir)/out_outl.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_outl.c

out_gray.o:  $(spdo_dir)/out_gray.c \
            	$(incl_dir)/spdo_prv.h \
            	$(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_gray.c

out_scrn.o:  $(spdo_dir)/out_scrn.c \
            	$(incl_dir)/spdo_prv.h \
                $(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_scrn.c

out_bl2d.o:  $(spdo_dir)/out_bl2d.c \
            	$(incl_dir)/spdo_prv.h \
                $(incl_dir)/fino.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(spdo_dir)/out_bl2d.c

#
#NSAMPLE
#
nsample.o: nsample.c\
                $(incl_dir)/speedo.h \
                $(incl_dir)/useropt.h \
                $(incl_dir)/type1.h \
                $(incl_dir)/fnt_a.h \
                $(incl_dir)/tr_fdata.h \
                $(incl_dir)/errcodes.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $(type1_defs) nsample.c

perf_test.o: perf_test.c \
                $(incl_dir)/spdo_prv.h \
                $(incl_dir)/speedo.h \
                $(incl_dir)/useropt.h
	cc $(debug) $(unix_defs) $(spdo_defs) $(incl) $*.c

clean:
	rm *.o

