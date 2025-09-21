# usb/printer Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=usb_speaker

SRC_DIR:=src/drivers/usb/usb_speaker

SRCS		:= usb_speaker.c pcm.c old.c

LINKS := dev/audio/old/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/inc/usb_p

SYSTEM_LIBS :=

MY_DFLAGS := 

include $(SDIR_TEMPLATES)/DriverTemplate.mk
