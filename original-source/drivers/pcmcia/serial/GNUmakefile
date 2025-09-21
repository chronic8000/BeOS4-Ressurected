# Makefile for driver

TARGET_NAME:=serial

SRC_DIR:=src/drivers/pcmcia/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/bus/pcmcia/serial/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
