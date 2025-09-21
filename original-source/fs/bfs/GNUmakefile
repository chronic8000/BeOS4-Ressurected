# Makefile for fs

TARGET_NAME:=bfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				attr.c \
				bitmap.c \
				debug.c \
				dir.c \
				dstream.c \
				file.c \
				select_ops.c \
				fsinfo.c \
				index.c \
				inode.c \
				io.c \
				log.c \
				mount.c \
				query.c \
				sysdep.c \
				util.c \
				walk.c \
				\
				btlib/btclose.c \
				btlib/btdebug.c \
				btlib/btdelete.c \
				btlib/btdup.c \
				btlib/bterrors.c \
				btlib/btfind.c \
				btlib/btgoto.c \
				btlib/btinsert.c \
				btlib/btio.c \
				btlib/btlabel.c \
				btlib/btload.c \
				btlib/btoopen.c \
				btlib/btopen.c \
				btlib/btpage1.c \
				btlib/btpage2.c \
				btlib/btravrs.c \
				btlib/btseek.c \
				btlib/btzap.c  \
				\
				match.c \
				\
				bfs_pm.c

MY_INCLUDES := \
	-I$(BUILDHOME)/$(SRC_DIR) \
	-I$(BUILDHOME)/$(SRC_DIR)/btlib \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

LONG_LONG       := -DREAL_LONG_LONG=1
MY_CFLAGS	:= -DDEBUG=1 $(LONG_LONG)

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
