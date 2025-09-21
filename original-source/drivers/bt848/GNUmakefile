# Makefile for driver

TARGET_NAME:=bt848

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/video/bt848/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
