# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=sonorus

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/drivers/audio/${TARGET_NAME}

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS		:= \
	sonorus.c sonorus_bd.c sonorus_dev.c sonorus_ioctl.c sonorus_mix.c
ifeq ($(TARGET_CPU),i586)
SRCS		+= sonorus_asm.asm
endif

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/driver
#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS := 

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libucode.a

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/audio/multi/$(TARGET_NAME) \
#

include $(SDIR_TEMPLATES)/DriverTemplate.mk

# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libucode.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	ucode/dsp_MonMix.c \
	ucode/fpga_1111b.c \
	ucode/fpga_1111d.c \
	ucode/fpga_2020b.c \
	ucode/fpga_2020d.c \
#

# The \ on the end of the line, extends the comment.
# UNUSED_SRCS := \
	ucode/dsp_bootStub.c \
	ucode/dsp_MonCh.c \
	ucode/fpga_1120b.c \
	ucode/fpga_1120d.c \
	ucode/fpga_ID.c \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
#

# MY_OPTIMIZER - the prefered optimization level


# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
# LINKS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

