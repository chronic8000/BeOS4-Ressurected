# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=neomagic

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS		:= driver.c bena4.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver \
					 -I$(BUILDHOME)/src/inc \
					 -I$(BUILDHOME)/src/kit/surface/include

SYSTEM_LIBS := 

MY_DFLAGS := -DDEBUG_EXTENSIONS=1


include $(SDIR_TEMPLATES)/DriverTemplate.mk
