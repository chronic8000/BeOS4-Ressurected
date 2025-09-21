# Makefile for driver

TARGET_NAME:=mvp4g.src

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= $(ALL_DOT_C)

LINKS := dev/audio/game/$(TARGET_NAME)

MY_INCLUDES := \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/drivers/audio/game_audio.src/inc

SYSTEM_LIBS := game_audio.a

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
