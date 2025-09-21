# Makefile for fs

TARGET_NAME:=dos

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				attr.c \
				dir.c \
				dlist.c \
				dosfs.c \
				encodings.cpp \
				fat.c \
				file.c \
				iter.c \
				mime_table/mime_table.c \
				util.c \
				vcache.c \
				version.c

MY_INCLUDES	:=	-I$(BUILDHOME)/src/inc/os_p

SYSTEM_LIBS := 

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

MY_CFLAGS := $(call MFLAG_TO_DFLAG, DISABLE_BUILDDATES)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

