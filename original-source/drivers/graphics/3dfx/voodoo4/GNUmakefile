# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:= 3dfx_voodoo4

SRC_DIR:=src/drivers/graphics/3dfx/voodoo4

SRCS		:= driver.c ../common/bena4.c MemMgrKrnlInc.c crt.c

MY_CFLAGS := -DDEBUG_EXTENSIONS=1

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver \
					 -I$(BUILDHOME)/src/inc \
					 -I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS :=

LINKS := dev/graphics/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/DriverTemplate.mk

