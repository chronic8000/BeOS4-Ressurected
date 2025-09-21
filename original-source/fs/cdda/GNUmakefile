# Makefile for fs

TARGET_NAME:=cdda

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= \
				cdda_attr.cpp \
				cdda_io.cpp \
				cdda_supportfuncs.cpp \
				iso.cpp \
				kernel_interface.cpp

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := 

# MY_DFLAGS	:= -DDEBUG=1

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
