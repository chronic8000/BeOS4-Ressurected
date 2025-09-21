# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=null.graphics

SRC_DIR:=src/drivers/graphics/null

SRCS	:= driver.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/graphics_p/null \
#

#SYSTEM_LIBS := libdinky.a

MY_CFLAGS := -DFORCE_CLEAR_SCREEN


include $(SDIR_TEMPLATES)/DriverTemplate.mk
