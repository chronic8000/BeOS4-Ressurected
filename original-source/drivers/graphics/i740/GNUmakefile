# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:= i740

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS	:= driver.c video.c cursor.c bena4.c

MY_CFLAGS :=  -DFORCE_DEBUG=1

MY_INCLUDES := \
					 -I$(BUILDHOME)/src/inc

SYSTEM_LIBS :=

LINKS := dev/graphics/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/DriverTemplate.mk

