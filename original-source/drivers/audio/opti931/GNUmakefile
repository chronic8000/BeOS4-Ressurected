# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=opti931

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/drivers/audio/${TARGET_NAME}

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS		:= opti.c pcm.c old.c midi.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
					-I$(BUILDHOME)/headers/drivers \
					-I$(BUILDHOME)/src/inc/driver \
					-I$(BUILDHOME)/src/nukernel/arch \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/media_p
#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS := 

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= 

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/midi/$(TARGET_NAME) \
		dev/joystick/$(TARGET_NAME) \
		dev/audio/old/$(TARGET_NAME) \
		dev/audio/mux/$(TARGET_NAME) \
		dev/audio/mix/$(TARGET_NAME) \
		dev/audio/raw/$(TARGET_NAME) \
#

include $(SDIR_TEMPLATES)/DriverTemplate.mk
