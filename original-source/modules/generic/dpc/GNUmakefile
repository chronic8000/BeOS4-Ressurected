# Makefile for driver

TARGET_NAME:=dpc

SRC_DIR:=src/modules/generic/$(TARGET_NAME)

SRCS		:= $(TARGET_NAME).c

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/support_p -I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM)

SYSTEM_LIBS:=

MY_CFLAGS := 


INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := generic/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

