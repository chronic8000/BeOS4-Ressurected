# Makefile for fs

TARGET_NAME:=hfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				block.c \
				btree.c \
				data.c \
				file.c \
				hfs.c \
				kernel_interface.c \
				low.c \
				node.c \
				record.c \
				volume.c

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

MY_DFLAGS	:= -DJOE=1 # -DHFS_IS_READONLY=1

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
