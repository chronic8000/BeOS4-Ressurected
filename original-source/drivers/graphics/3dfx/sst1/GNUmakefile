# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:= 3dfx_sst1

SRC_DIR:=src/drivers/graphics/3dfx/sst1

SRCS		:= 3dfx_driver.c

MY_CFLAGS := 

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver \
					-I$(BUILDHOME)/src/inc/graphics_p \
#

SYSTEM_LIBS :=

LINKS := dev/graphics/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/DriverTemplate.mk

