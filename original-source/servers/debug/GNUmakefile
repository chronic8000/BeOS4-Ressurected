# Makefile using AppTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=debug_server

SRC_DIR:=src/servers/debug

MY_CFLAGS :=  -DDEBUG=1 -DNO_LAUNCH_BY_MIME_SIG=1

MY_INCLUDES	:=  \
			-I$(BUILDHOME)/headers/devel \
			-I$(BUILDHOME)/src/inc \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/app_server_p \
			-I$(BUILDHOME)/src/servers/debug \
			-I$(BUILDHOME)/src/kit/devel/$(TARGET_CPU)

SRCS :=  \
	app.cpp \
	blddate.c \
	break.c \
	cmd.c \
	cwindow.c \
	entry.c \
	expr.c \
	globals.c \
	init.c \
	launcher.cpp \
	memory.c \
	nogui.c \
	nubface.c \
	path.c \
	simple.cpp \
	str.c \
	strtable.c \
	symbol.c \
	symtable.c \
	thread.c \
	token.c \
	fpasm.S \
#

ifeq ($(call MFLAG_IS_SET,USE_IAD_DEBUG_SERVER_SOURCES),)
SRCS += \
	loadelf.c \
#
else
SRCS += \
	loadelf_iad.c \
#
endif

MY_CFLAGS += -Ddprintf=_kdprintf_
MY_INCLUDES += -I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU)

SYSTEM_LIBS:= libroot.so libbe.so libdevel.a

ifneq ($(call MFLAG_IS_SET, SUPPORTS_COMPRESSED_ELF_LOADER),)
	SYSTEM_LIBS += libuncrush.a libz.a
endif

RESOURCE_FORK := $(TARGET_NAME).rsrc

LINK_DIRS :=

MY_INSTALL_DIR := $(INSTALL_SERVERS)


include $(SDIR_TEMPLATES)/AppTemplate.mk



