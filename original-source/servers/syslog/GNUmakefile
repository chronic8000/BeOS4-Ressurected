# Makefile using AppTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:= syslog_daemon

SRC_DIR:=src/servers/syslog

MY_CFLAGS :=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/support_p \
			   	-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc \
#				-I$(BUILDHOME)/headers

SRCS := syslog_daemon.cpp

SYSTEM_LIBS:= libroot.so libbe.so

RESOURCE_FORK := $(TARGET_NAME).rsrc

LINK_DIRS :=

MY_INSTALL_DIR := $(INSTALL_SERVERS)


include $(SDIR_TEMPLATES)/AppTemplate.mk

