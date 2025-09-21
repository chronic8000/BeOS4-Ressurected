# Makefile for driver

TARGET_NAME:=rangelan

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= rangelan.c \
		net_data.c \
		lld.c \
		lldinit.c \
		lldctrl.c \
		blld.c \
		lldisr.c \
		lldpack.c \
		lldqueue.c \
		lldsend.c \
		lldrx.c \
		asm.c \
		hardware.c \
		port.c \
		pcmcia.c \
		css.c \
#

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk

SRC_DIR := $(SRC_DIR_IN)

FILES:=rangelan.conf
MY_INSTALL_DIR:=$(INSTALL_DRIVER_SETTINGS)
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

