# Makefile for driver

TARGET_NAME:=memory

SRC_DIR:=src/drivers/pcmcia/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c wchan.c

LINKS := dev/bus/pcmcia/memory/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
