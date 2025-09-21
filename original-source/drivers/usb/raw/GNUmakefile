# usb/raw Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=usb_raw

SRC_DIR:=src/drivers/usb/raw

SRCS	:= $(TARGET_NAME).c

LINKS := dev/bus/usb/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/usb_p

SYSTEM_LIBS :=

MY_CFLAGS := 


include $(SDIR_TEMPLATES)/DriverTemplate.mk
