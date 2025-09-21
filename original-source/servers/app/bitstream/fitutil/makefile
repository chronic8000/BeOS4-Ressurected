#################################################################
#		Unix Makefile for building FIT utility and test programs
#
# $Header: //bliss/user/archive/rcs/fit/fitutil.mku,v 6.0 97/03/17 17:36:23 shawn Release $
#
# $Log:	fitutil.mku,v $
#  Revision 6.0  97/03/17  17:36:23  shawn
#  TDIS Version 6.00
#  
#  Revision 4.2  97/03/11  11:21:14  shawn
#  COMPRESSED_PFR is now the only option.
#  Added DBG conditional to the compile statements.
#  
#  Revision 4.1  96/08/20  13:09:11  shawn
#  Added RCS/Header keyword.
#  
#  Revision 4.0  95/02/15  16:49:34  shawn
#  Release
#  
#  Revision 3.2  95/01/04  16:31:15  roberte
#  Release
#  
#  Revision 3.1  95/01/04  11:45:21  roberte
#  Release
#  
#  Revision 2.3  94/12/28  11:56:57  roberte
#  Added commented out CFLAGS for compressed PFR build.
#  
#  Revision 2.2  94/11/14  13:36:13  roberte
#  Rewrote for new includes sub-dir setup of delivery diskettes.
#  
#  Revision 2.1  94/07/20  08:59:35  roberte
#  Added commented-out CFLAGS for INCL_MFT (Merged Font Technology) build.
#  
#  Revision 2.0  94/05/04  09:25:56  roberte
#  Release
#  
#  Revision 1.22  94/04/08  11:28:04  roberte
#  Release
#  
#  Revision 1.15  94/03/11  16:14:54  roberte
#   Change RCS path from defunct ninja to bliss.
#  
#  Revision 1.14  93/04/06  09:59:38  roberte
#  Changed name of finfotest.c to finfotes.c
#  
#  Revision 1.13  93/03/15  12:41:34  roberte
#  Release
#  
#  Revision 1.2  93/03/03  09:28:47  roberte
#  Changed tt_dir from "true_typ" to "tt"
#  Removed general rule for making objects.
#  
#  Revision 1.1  93/02/02  15:01:08  roberte
#  Initial revision
#  
#  Revision 1.4  93/02/02  14:58:40  roberte
#  Added comment header and RCS control string.  Made macro
#  "tt_dir" and use this in include macro.
#  
#

# for standard Speedo or TrueDoc, or compressed Speedo fonts, use this CFLAGS:
CFLAGS = -c
# for Merged Font Technology, use this CFLAGS:
#CFLAGS = -c -DINCL_MFT=1
# for compressed TrueDoc PFR, use this CFLAGS:
#CFLAGS = -c -DNO_PROCS -DPROC_TRUEDOC=1

LINKS = finfotbl.o 
 
# Debug Option         
#DBG = -g
DBG = 

# Include Directory Path
incl_dir = ../includes

# Include Directive
incl = -I$(incl_dir)

all: fitutil finfotest

fitutil : $(LINKS)
	cc -o fitutil $(LINKS)


finfotbl.o: finfotbl.c \
			$(incl_dir)/pclhdrs.h \
			$(incl_dir)/finfotbl.h
	cc $(CFLAGS) $(DBG) $(incl) $*.c
 
#test program target:
finfotest : finfotes.o
	cc -o finfotest finfotes.o

finfotes.o: finfotes.c \
			$(incl_dir)/pclhdrs.h \
			$(incl_dir)/finfotbl.h
	cc $(CFLAGS) $(DBG) $(incl) $*.c
 

