# Makefile for driver

TARGET_NAME:=pcmcia_cs

SRC_DIR:=src/modules/bus_managers/pcmcia/cs

SRCS		:= \
				rsrc_mgr.c cistpl.c \
				cs.c bulkmem.c cardbus.c \
				cs_timer.c wchan.c

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS:=

MY_CFLAGS := 

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := bus_managers/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

