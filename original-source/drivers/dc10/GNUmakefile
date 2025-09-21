# Makefile for driver

TARGET_NAME:=dc10

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/video/dc10/$(TARGET_NAME)

MY_INCLUDES := \
		-I$(BUILDHOME)/src/inc/os_p \
		-I$(BUILDHOME)/src/inc/driver
#

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
