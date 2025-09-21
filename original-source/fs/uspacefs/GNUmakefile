# Makefile for fs

TARGET_NAME:=uspacefs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= uspacefs.cpp

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_DFLAGS	:= -DJOE=1

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
