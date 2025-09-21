# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
FILES := g400

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/drivers/graphics/g400

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DRIVERS)/bin

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

ELF_FILES += $(addprefix $(MY_INSTALL_DIR)/, $(FILES))

include $(SDIR_TEMPLATES)/DataTemplate.mk

SOURCE := $(INSTALL_DRIVERS)/bin/g400
LINK_RELATION := $(INSTALL_DRIVERS)
LINKS := $(INSTALL_DRIVERS)/dev/graphics/$(notdir $(SOURCE))
MY_DEPS := $(SRC_DIR_OUT)/$(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/SymlinkTemplate.mk
