# Makefile for driver

TARGET_NAME:=tcm589

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/driver \

SYSTEM_LIBS :=

MY_CFLAGS := -DISA_PNP_CARD=0 -DPCMCIA_CARD=1

include $(SDIR_TEMPLATES)/DriverTemplate.mk
