# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:= radeon

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS		:= detect.c driver.c radeon_bios.c crt.c MemMgrKrnlInc.c

MY_DFLAGS := -DDEBUG_EXTENSIONS=1

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver \
					 -I$(BUILDHOME)/src/inc \
					 -I$(BUILDHOME)/src/modules/bus_managers/agp 

SYSTEM_LIBS :=

LINKS := dev/graphics/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/DriverTemplate.mk

