# Makefile for fs

TARGET_NAME:=cfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				cfs.c \
				cfs_attr.c \
				cfs_change.c \
				cfs_compress.c \
				cfs_data.c \
				cfs_dir.c \
				cfs_entry.c \
				cfs_file.c \
				cfs_free_space.c \
				cfs_fsinfo.c \
				cfs_io.c \
				cfs_lock.c \
				cfs_log.c \
				cfs_pm.c \
				cfs_vnode.c \
				cfs_walk.c

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/nukernel/inc \
	-I$(BUILDHOME)/srcx/gnu/zlib-1.1.3 \
#

SYSTEM_LIBS := libz.a

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
