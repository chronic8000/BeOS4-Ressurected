# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=nvidia

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS	:= drv.c dev.c init.c nv_setup.c video.c cursor.c overlay.c irq.c \
	   debugcmd.c riva_hw.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include \
		   -I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS := libdinky.a

MY_CFLAGS := -DDEBUG=1


include $(SDIR_TEMPLATES)/DriverTemplate.mk
