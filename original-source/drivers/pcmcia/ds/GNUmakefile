# Makefile for driver

TARGET_NAME:=ds

SRC_DIR:=src/drivers/pcmcia/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/bus/pcmcia/sock/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
