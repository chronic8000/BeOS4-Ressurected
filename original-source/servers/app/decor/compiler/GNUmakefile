# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=decorator

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/servers/app/decor/compiler

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	LuaDecor.cpp \
	parser.cpp \
	Runtime.cpp \
	rect.cpp \
	atom.cpp \
	$(addprefix decor/, \
		DecorActionDef.cpp \
		DecorAnchorage.cpp \
		DecorAtom.cpp \
		DecorBehavior.cpp \
		DecorButton.cpp \
		DecorChoice.cpp \
		DecorClasses.cpp \
		DecorColor.cpp \
		DecorDef.cpp \
		DecorEvent.cpp \
		DecorExecutor.cpp \
		DecorFiller.cpp \
		DecorFont.cpp \
		DecorGroup.cpp \
		DecorImage.cpp \
		DecorImagePart.cpp \
		DecorPart.cpp \
		DecorSharedAtom.cpp \
		DecorStack.cpp \
		DecorState.cpp \
		DecorStream.cpp \
		DecorTextWidget.cpp \
		DecorThumb.cpp \
		DecorVariable.cpp \
		DecorVariableInteger.cpp \
		DecorVariableString.cpp \
		DecorWidget.cpp \
	)

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

APP_SERVER_DIR	:= $(BUILDHOME)/src/servers/app
DECOR_DIR		:= $(APP_SERVER_DIR)/decor

MY_INCLUDES	+= \
				-I$(BUILDHOME)/src/inc/app_server_p \
				-I$(BUILDHOME)/src/inc/interface_p \
				-I$(APP_SERVER_DIR) \
				-I$(APP_SERVER_DIR)/render \
				-I./lua-3.2-ms/include \
				-I./lua-3.2-ms/src \
				-I$(DECOR_DIR) \
				-I.

MY_DFLAGS += -DDECOR_CLIENT=1

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libtranslation.so liblua.a liblualib.a

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc3) 
SYSTEM_LIBS += libsupc++.a
endif 

$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2)

# This is referenced in the original makefile, but doesn't exist.
# RESOURCES := parser.rsrc

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BIN)

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# This will over-ride the application's version id.
# MY_VERSION := 

include $(SDIR_TEMPLATES)/AppTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, \
	lua-3.2-ms \
, src/servers/app/decor/compiler)
