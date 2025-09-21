# Makefile for driver

TARGET_NAME:=genpool

SRC_DIR:=src/modules/generic/$(TARGET_NAME)

SRCS		:= $(TARGET_NAME).c ranges.c metapool.c defalloc.c

MY_INCLUDES := \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/modules/generic/$(TARGET_NAME)/include

SYSTEM_LIBS:= libdinky.a

MY_CFLAGS := 


INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := generic/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

