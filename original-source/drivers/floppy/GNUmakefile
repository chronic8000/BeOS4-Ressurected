# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=floppy

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/nukernel

MY_OBJ_OFFSET:=floppy.driver.f

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := arch/isa/flopdma.c drivers/device_icons.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES	+= -I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU) \
   	-I$(BUILDHOME)/src/nukernel/vm/$(call MVAR_TO_TARGET_NAME, SYSTEM_KERNEL_FEATURE_VM_FLAVOUR)/common \
   	-I$(BUILDHOME)/src/nukernel/vm/$(call MVAR_TO_TARGET_NAME, SYSTEM_KERNEL_FEATURE_VM_FLAVOUR)/cpu/$(TARGET_CPU) \
	-I$(BUILDHOME)/src/nukernel/inc
#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS := -DBUILD_LOADABLE -DICONS_FLOPPY

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
# SYSTEM_LIBS:= 

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/disk/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/DriverTemplate.mk
