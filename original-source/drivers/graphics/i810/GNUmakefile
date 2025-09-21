# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=i810

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS	:= drv.c dev.c init.c video.c cursor.c irq.c gfxbuf.c debugcmd.c
SRCS	+= overlay.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include \
		   -I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS := libdinky.a

MY_CFLAGS := -DDEBUG=1 -D_KERNEL_MODE=1


include $(SDIR_TEMPLATES)/DriverTemplate.mk

SOURCE := i810.settings
SRC_DIR:=src/drivers/graphics/i810
TARGET := $(INSTALL_SAMPLE_DRIVER_SETTINGS)/i810
PERMISSIONS := $(PERMS_USER_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
