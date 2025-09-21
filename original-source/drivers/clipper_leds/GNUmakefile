# Makefile for driver

TARGET_NAME:=clipper_leds

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/misc/status/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
