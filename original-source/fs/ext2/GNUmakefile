# Makefile for fs

TARGET_NAME:=ext2

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
			attr.c \
			block_alloc.c \
			dir.c \
			export.c \
			ext2.c \
			file.c \
			file_extent.c \
			fs.c \
			mime_table/mime_table.c \
			io.c \
			perm.c \
			prealloc.c 

MY_INCLUDES		+= 	-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc \
					-I$(BUILDHOME)/src/nukernel/inc \
					-I$(BUILDHOME)/src/nukernel/lib/inc \
					-I$(BUILDHOME)/src/fs/dos/mime_table

SYSTEM_LIBS := 

MY_DFLAGS			+= -DRO -DDEBUG #-DENABLE_DPRINTF -DLOAD_SYM -DUSE_DMALLOC

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
