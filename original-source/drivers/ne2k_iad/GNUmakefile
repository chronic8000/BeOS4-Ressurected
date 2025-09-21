# Makefile for driver

TARGET_NAME:=ne2k

SRC_DIR:=src/drivers/ne2k_iad

SRCS:= $(TARGET_NAME).c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk


SRC_DIR:=src/drivers/ne2k_iad
SOURCE := $(call MVAR_TO_TARGET_NAME, NE2K_SETTINGS, ne2k.sample_config)
TARGET := $(INSTALL_USER_SETTINGS)/kernel/drivers/ne2k
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := ne2k.sample_config
SRC_DIR:=src/drivers/ne2k_iad
TARGET := $(INSTALL_SAMPLE_DRIVER_SETTINGS)/ne2k
PERMISSIONS := $(PERMS_USER_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
