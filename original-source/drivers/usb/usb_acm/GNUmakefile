# usb/printer Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=usb_acm

SRC_DIR:=src/drivers/usb/usb_acm

SRCS	:= usb_acm.c

LINKS := dev/ports/$(TARGET_NAME)

MY_INCLUDES :=-I$(BUILDHOME)/src/inc/usb_p

SYSTEM_LIBS :=

MY_DFLAGS := 

include $(SDIR_TEMPLATES)/DriverTemplate.mk
