# Makefile for fs

TARGET_NAME:=binderfs

SRC_DIR:=src/fs/$(TARGET_NAME)

SRCS		:= binderfs.cpp binder_request.cpp port_pool.cpp

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH) -I$(PRIVATE_INCLUDE_PATH)/os_p -I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS := libz.a

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := file_systems/$(TARGET_NAME)

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_OPTIMIZER:= -O1
include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
