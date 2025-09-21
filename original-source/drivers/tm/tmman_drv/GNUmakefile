# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=tm

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/drivers/$(TARGET_NAME)/tmman_drv

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := ../tm.c
SRCS += debug.c
SRCS += osal.c
SRCS += rtal.c
#SRCS += tm_main.c	=> LINUX-specific
SRCS += tmhal.c
SRCS += tmif.c
SRCS += tmman.c
SRCS += ../mydefs.c
SRCS += ../tmman_lib/misc.c
SRCS += ../tmman_lib/memory.c
SRCS += ../tmman_lib/namespc.c
SRCS += ../tmman_lib/channel.c
SRCS += ../tmman_lib/vintr.c
SRCS += ../tmman_lib/event.c
SRCS += ../tmman_lib/message.c
SRCS += ../tmman_lib/queue.c
SRCS += ../tmman_lib/verinfo.c


# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := -DTMMAN_HOST

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
#MY_INCLUDES	:= \
#			-I$(BUILDHOME)/src/nukernel/drivers
#MY_INCLUDES := \
#		-I$(BUILDHOME)/src/inc/os_p
MY_INCLUDES := -I$(BUILDHOME)/src/drivers/tm/tmman_libload
MY_INCLUDES += -I$(BUILDHOME)/src/drivers/tm/tmman_api

#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS :=

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= 

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/misc/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/DriverTemplate.mk
