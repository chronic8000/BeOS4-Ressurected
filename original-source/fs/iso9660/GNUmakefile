# Makefile for fs

TARGET_NAME:=iso9660

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				kernel_interface.c \
				iso.c

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

MY_DFLAGS	:= -DJOE=1

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
