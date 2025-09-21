# Makefile for driver

TARGET_NAME:=epic100

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES :=  \
	-I$(BUILDHOME)/src/inc

SYSTEM_LIBS :=

# MY_DFLAGS := -DSTAT_COUNTING
MY_DFLAGS := -DDEBUG_VERSION

include $(SDIR_TEMPLATES)/DriverTemplate.mk
