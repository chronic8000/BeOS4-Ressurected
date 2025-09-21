SRC_DIR:=src/drivers/audio/audio_drv

TARGET_NAME := libaudio_drv.a

SRCS		:= audio_drv.c device.c joy.c midi.c pcm.c

MY_DFLAGS := -D_SOUND_COMPATIBILITY=1

ifneq ($(call MFLAG_IS_SET, DEBUG_BUILD),)
MY_DFLAGS += -DDEBUG
else
MY_DFLAGS += -DNDEBUG
endif

MY_INCLUDES := 	\
			-I$(BUILDHOME)/$(SRC_DIR)/inc \
			-I$(BUILDHOME)/src/inc/media_p \
			-I$(BUILDHOME)/src/inc/driver

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

