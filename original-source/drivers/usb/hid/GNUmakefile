# usb/hid Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=hid

SRC_DIR:=src/drivers/usb/$(TARGET_NAME)

SRCS		:= hid.c keyboard.c mouse.c

LINKS := dev/input/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/os_p

SYSTEM_LIBS :=

MY_DFLAGS :=  -DB_USB_USE_V2_MODULE

include $(SDIR_TEMPLATES)/DriverTemplate.mk
