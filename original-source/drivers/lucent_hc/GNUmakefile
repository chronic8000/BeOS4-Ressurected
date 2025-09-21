# kernel driver Makefile with precompiled objects
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME     := ltmodem
SRC_DIR         := src/drivers/lucent_hc
SRCS			:= zz.c conf.c
LINKS           := dev/ports/$(TARGET_NAME)

MORE_OBJS := \
				lucent.objs/atcmd.o     lucent.objs/dpllhost.o  lucent.objs/lapm_lib.o \
				lucent.objs/atcmd1.o    lucent.objs/dpmain.o    lucent.objs/lapm_rad.o \
				lucent.objs/atfax.o     lucent.objs/dpshmap.o   lucent.objs/llmain.o   \
				lucent.objs/atmain.o                            lucent.objs/llutil.o   \
				lucent.objs/atparse.o   lucent.objs/dpvpcm.o    lucent.objs/llutil1.o  \
				lucent.objs/ats.o       lucent.objs/iodata.o    lucent.objs/math64.o   \
				lucent.objs/be_misc.o   lucent.objs/iodce.o     lucent.objs/mnp_5a.o   \
				lucent.objs/dpcall.o    lucent.objs/iodte.o     lucent.objs/mnp_bg.o   \
				lucent.objs/dpcmd1.o    lucent.objs/iodte1.o    lucent.objs/mnp_ctl.o  \
				lucent.objs/dpcntl.o    lucent.objs/iofax.o     lucent.objs/mnp_dce.o  \
				lucent.objs/dpdspinc.o  lucent.objs/iomain.o    lucent.objs/mnp_dte.o  \
				lucent.objs/dpfax.o     lucent.objs/iopdm.o     lucent.objs/mnp_lib.o  \
				lucent.objs/dpio.o      lucent.objs/lapm_bg.o   lucent.objs/uart.o     \
				lucent.objs/dpisr.o     lucent.objs/lapm_ctl.o  lucent.objs/v42bis.o   \
				lucent.objs/dpllcnst.o  lucent.objs/lapm_dce.o                         \
				lucent.objs/dpllgain.o  lucent.objs/lapm_dte.o                         \
#

MY_INCLUDES := 
SYSTEM_LIBS     := 

MY_CFLAGS	:=


include $(SDIR_TEMPLATES)/DriverTemplate.mk

