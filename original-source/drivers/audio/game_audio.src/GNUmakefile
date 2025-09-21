# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
#
# REMEMBER: You can't comment out any of these, though some of them can be
#  set to empty. (The flat makefile system means if you don't set things to
#  empty, they likely won't be empty when you're called.)

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=game_audio.a

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/drivers/audio/game_audio.src

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:=$(ALL_DOT_C)

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR)/inc \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR :=


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
LINKS :=


include $(SDIR_TEMPLATES)/StaticLibTemplate.mk
