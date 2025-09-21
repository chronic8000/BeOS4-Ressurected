# Makefile for driver

TARGET_NAME:=config_manager

SRC_DIR:=src/modules/bus_managers/$(TARGET_NAME)

ifeq ($(call MFLAG_IS_SET,USE_IAD_CONFIG_MANAGER_SOURCES),)
SRCS := \
	config_manager.c \
#
else
SRCS := \
	config_manager_iad.c \
#
endif
SRCS += \
				config_boot.c \
				range.c \
				debug.c

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU)

MY_CFLAGS :=

SYSTEM_LIBS:=

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := bus_managers/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

SOURCE := config_manager.settings
SRC_DIR:=src/modules/bus_managers/config_manager
TARGET := $(INSTALL_SAMPLE_DRIVER_SETTINGS)/config_manager
PERMISSIONS := $(PERMS_USER_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
