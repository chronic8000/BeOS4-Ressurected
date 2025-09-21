# Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=usb_tests

SRC_DIR:=src/drivers/usb/tests

SRCS	:= tests_bulk0.c

LINKS := dev/misc/usb/$(TARGET_NAME)

MY_INCLUDES := -I/source/private/usb/headers
#$(PRIVATE_INCLUDE_PATH)/usb_p

SYSTEM_LIBS :=

MY_CFLAGS := 


include $(SDIR_TEMPLATES)/DriverTemplate.mk
