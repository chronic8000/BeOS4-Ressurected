# Makefile for fs

TARGET_NAME:=cifs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
			skiplist.c \
			dir_entry.c \
			trans2.c \
			read.c \
			write.c \
			mount.c \
			packet.c \
			netio.c \
			attr.c \
			rename.c \
			create_dir.c \
			create_file.c \
			cifs_ops.c \
			cifs_utils.c \
			smbdes.c \
			vcache.c \
			kernel_interface.c \
			cifs.c

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc \
	-I$(BUILDHOME)/src/nukernel/lib/inc

SYSTEM_LIBS := 

LONG_LONG           := -DREAL_LONG_LONG=1
#MY_DFLAGS			+= -DDELAY_OPENDIR -DDEBUG=1 $(LONG_LONG) -DKERNEL_SPACE
MY_DFLAGS			+= -DDELAY_OPENDIR $(LONG_LONG) -DKERNEL_SPACE

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
