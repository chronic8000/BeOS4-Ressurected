# Makefile for driver

TARGET_NAME:=1394

SRC_DIR :=src/drivers/1394

SRCS := $(TARGET_NAME).c list.c

LINKS := dev/bus/1394/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/nukernel/lib/inc \
#

SYSTEM_LIBS :=

MY_CFLAGS := -DUSE_DMALLOC=0

MY_OPTIMIZER:=

include $(SDIR_TEMPLATES)/DriverTemplate.mk
