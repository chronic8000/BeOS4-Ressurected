# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=scsi_raw

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/drivers/$(TARGET_NAME)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := ${TARGET_NAME}.c drivers/scsi_common.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES	:= \
			-I$(BUILDHOME)/src/nukernel/drivers
#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS :=

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= 

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/bus/scsi/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/DriverTemplate.mk
