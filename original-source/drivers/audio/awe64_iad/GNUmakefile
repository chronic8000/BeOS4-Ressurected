# Makefile for driver

TARGET_NAME:=awe64

SRC_DIR:=src/drivers/audio/awe64_iad

SRCS		:= awe64.c pcm.c old.c midi.c mixer.c mux.c

MY_INCLUDES := -I$(BUILDHOME)/headers/drivers \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/nukernel/arch \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/media_p

SYSTEM_LIBS :=

LINKS := dev/audio/old/$(TARGET_NAME) \
		dev/midi/$(TARGET_NAME) \
		dev/joystick/$(TARGET_NAME) \
		dev/audio/raw/$(TARGET_NAME) \
		dev/audio/mux/$(TARGET_NAME) \
		dev/audio/mix/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/DriverTemplate.mk


SRC_DIR:=src/drivers/audio/awe64_iad
SOURCE:=awe64.settings
ifneq ($(call MFLAG_IS_SET, INSTALL_AUDIO_SETTINGS),)
TARGET:=$(INSTALL_DRIVER_SETTINGS)/awe64
else
TARGET:=$(INSTALL_SAMPLE_DRIVER_SETTINGS)/awe64
endif
PERMISSIONS := $(PERMS_USER_DATA)

include $(SDIR_TEMPLATES)/DataFileTemplate.mk

