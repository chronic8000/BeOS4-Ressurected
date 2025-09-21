
TARGET_NAME:= sonic_vibes

SRC_DIR:=src/drivers/audio/sonic_vibes

SRCS		:= sv.c joy.c midi.c pcm.c mux.c mixer.c

LINKS := \
		dev/midi/$(TARGET_NAME) \
		dev/joystick/$(TARGET_NAME) \
		dev/audio/raw/$(TARGET_NAME) \
		dev/audio/old/$(TARGET_NAME) \
		dev/audio/mux/$(TARGET_NAME) \
		dev/audio/mix/$(TARGET_NAME)

MY_INCLUDES		:= \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk

#TARGET_NAME:=svt
#SRC_DIR:= $(SRC_DIR_IN)
#MY_CFLAGS := $(MY_CFLAGS_IN)
#MY_INCLUDES := $(MY_INCLUDES_IN)
#SRCS:=$(TARGET_NAME).cpp
#SYSTEM_LIBS:= libroot.so
#RESOURCE_FORK :=
#LINK_DIRS :=
#MY_INSTALL_DIR := $(INSTALL_USER_BIN)
#MY_OPTIMIZER:=-O1
#include $(SDIR_TEMPLATES)/AppTemplate.mk
#
#TARGET_NAME:=svt2
#SRC_DIR:= $(SRC_DIR_IN)
#MY_CFLAGS := $(MY_CFLAGS_IN)
#MY_INCLUDES := $(MY_INCLUDES_IN)
#SRCS:=$(TARGET_NAME).cpp
#SYSTEM_LIBS:= libroot.so
#RESOURCE_FORK :=
#LINK_DIRS :=
#MY_INSTALL_DIR := $(INSTALL_USER_BIN)
#MY_OPTIMIZER:=-O1
#include $(SDIR_TEMPLATES)/AppTemplate.mk
#
