# Makefile for driver

TARGET_NAME:=tlan

SRC_DIR:=src/drivers/tlanpci

SRCS:= tlanpci.c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/driver \

SYSTEM_LIBS :=

MY_CFLAGS :=

include $(SDIR_TEMPLATES)/DriverTemplate.mk
