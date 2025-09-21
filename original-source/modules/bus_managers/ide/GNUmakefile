# Makefile for driver

TARGET_NAME:=libide_bus_manager.a

SRC_DIR:=src/modules/bus_managers/ide

SRCS		:= \
				ide.c \
				protocol.c

MY_OBJ_OFFSET:=$(TARGET_NAME).d

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/support_p \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/drivers/ide/include

SYSTEM_LIBS:=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE -- \""

MY_INSTALL_DIR := 

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk


TARGET_NAME:=ide

SRC_DIR:=src/modules/bus_managers/$(TARGET_NAME)

SRCS		:= \
				ide.c \
				protocol.c

MY_OBJ_OFFSET:=$(TARGET_NAME).d

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/support_p \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/drivers/ide/include

SYSTEM_LIBS:=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE -- \""


INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := bus_managers/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

