# Makefile for fs

TARGET_NAME:=memfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= memfs.c

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/os_p -I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

MY_CFLAGS := -DDEBUG=0


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
