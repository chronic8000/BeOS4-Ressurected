# Makefile for driver

TARGET_NAME:=acer_wp_touch

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/input/touchscreen/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS := -Wall


include $(SDIR_TEMPLATES)/DriverTemplate.mk
