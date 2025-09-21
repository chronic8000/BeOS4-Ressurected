# Makefile for fs

TARGET_NAME:=ofs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				ofs.c

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
