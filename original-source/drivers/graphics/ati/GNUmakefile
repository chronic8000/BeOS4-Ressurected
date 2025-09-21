# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=ati

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS	:= driver.c 

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/graphics_p

SYSTEM_LIBS :=

MY_CFLAGS := -DDEBUG=0


include $(SDIR_TEMPLATES)/DriverTemplate.mk
