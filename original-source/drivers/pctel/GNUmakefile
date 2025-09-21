# Makefile for driver

TARGET_NAME:=pctel

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS		:= halwraps.c \
			pctios.c \
			vuart.c \
			zz.c

LINKS := dev/ports/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS := pctel_control.a pctel_dsp.a

MY_CFLAGS := -D__GNU__=1


include $(SDIR_TEMPLATES)/DriverTemplate.mk

-include $(BUILDHOME)/src/drivers/pctel/pctel.libs/$(MAKEFILE_LEAFNAME)

