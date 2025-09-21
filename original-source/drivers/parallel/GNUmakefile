# Makefile for driver

TARGET_NAME:= parallel

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS		:= parallel.c ieee1284.c detect.c conf.c dongle.c

LINKS := dev/parallel/$(TARGET_NAME) \
	dev/misc/dongle/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=

include $(SDIR_TEMPLATES)/DriverTemplate.mk


