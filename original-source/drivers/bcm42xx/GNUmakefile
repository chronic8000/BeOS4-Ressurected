# Makefile using DriverTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=bcm42xx

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/drivers/$(TARGET_NAME)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS		:= ilc.c ilc_cert.c il_stats.c crc.c il_beos.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/include \
#

# MY_DFLAGS - Template User provided define-type flags
MY_DFLAGS := -DDBG=1 -DDMA -DIL_FIXES -DIL_STATS -DIL_CERT \
		-DMEANSE -DBCMDBG

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
# SYSTEM_LIBS:= 

# LINKS	- symlinks starting from the driver directory
#			(dev/misc/$(TARGET_NAME) etc...)
LINKS := dev/net/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/DriverTemplate.mk
