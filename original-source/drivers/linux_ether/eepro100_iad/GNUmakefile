# Makefile for driver

TARGET_NAME:= eepro100

SRC_DIR:= src/drivers/linux_ether/eepro100_iad

SRCS:= $(TARGET_NAME).c $(TARGET_NAME)_wrapper.c

SRCS += lib/linux.c lib/generic_wrapper.c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/drivers/linux_ether/include

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk

