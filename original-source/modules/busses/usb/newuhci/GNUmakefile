# Makefile for driver

TARGET_NAME:=uhci

SRC_DIR:=src/modules/busses/usb/newuhci

SRCS		:= uhci.c debug_uhci.c roothub.c endpoint.c transfer.c

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/usb_p \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/nukernel/inc


SYSTEM_LIBS:=

# MY_DFLAGS	+= -DMY_MEMALIGN -DLOAD_ONLY_WITH_SERIAL_DEBUG -DDEBUG_HW_SW_SYNC


INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := busses/usb/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

ifneq ($(call MVAR_EXISTS, USER_SETTINGS_UHCI),)
SRC_DIR:=src/modules/busses/usb/newuhci/settings
SOURCE:=$(call MVAR_TO_TARGET_NAME, USER_SETTINGS_UHCI)
TARGET:=$(INSTALL_B_USER_SETTINGS_DIRECTORY)/kernel/drivers/uhci
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
endif
