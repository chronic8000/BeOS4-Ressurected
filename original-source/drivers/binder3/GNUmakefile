
BINDER2_SRCFILES:= \
	binder_driver.cpp \
	binder_node.cpp \
	binder_team.cpp \
	binder_thread.cpp \
	binder_transaction.cpp \
	binder_dev.cpp \
#

BINDER2_INCLUDES := \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc \
#

# Makefile for driver

#TARGET_NAME:=binder2
#SRC_DIR:=src/drivers/$(TARGET_NAME)

#SRCS:= $(BINDER2_SRCFILES)
#MY_INCLUDES:= $(BINDER2_INCLUDES)

#LINKS := dev/misc/$(TARGET_NAME)

#SYSTEM_LIBS :=

#MY_CFLAGS := -fno-exceptions

#include $(SDIR_TEMPLATES)/DriverTemplate.mk

# Makefile for driver built as a library (for debugging)

TARGET_NAME := libbinderdbg.so
SRC_DIR:=src/kit/libbinderdbg

SRCS:= $(BINDER2_SRCFILES)
MY_INCLUDES:= $(BINDER2_INCLUDES)

MY_DFLAGS := -DBINDER_DEBUG_LIB=1 
MY_CFLAGS += $(F_NO_EXCEPTIONS) -O0 -g
MY_INSTALL_DIR := $(INSTALL_LIBS)
SYSTEM_LIBS := libroot.so
include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
