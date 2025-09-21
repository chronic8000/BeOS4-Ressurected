# Makefile for driver

TARGET_NAME:=pcmcia_ds

SRC_DIR:=src/modules/bus_managers/pcmcia/ds

SRCS		:= \
				ds.c

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS:=

MY_CFLAGS := 

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := bus_managers/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

FILES := pcmcia.default
SRC_DIR:=src/modules/bus_managers/pcmcia/ds
MY_INSTALL_DIR:=$(INSTALL_DRIVER_SETTINGS)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk

