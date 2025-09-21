# Makefile for fs

TARGET_NAME:=ntfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
			attr.c \
			attribute_handlers.c \
			attributes.c \
			compress.c \
			compression_buf.c \
			dir.c \
			export.c \
			extent.c \
			file.c \
			fs.c \
			io.c \
			mime_table/mime_table.c \
			ntfs.c \
			ntfsmalloc.c \
			util.c 

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc \
	-I$(BUILDHOME)/src/nukernel/lib/inc \
	-I$(BUILDHOME)/src/fs/dos/mime_table

SYSTEM_LIBS := 

MY_DFLAGS	+: -DDEBUG #-DENABLE_DPRINTF -DLOAD_SYM -DUSE_DMALLOC -DNTFS_MALLOC

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
