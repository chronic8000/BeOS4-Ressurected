# Makefile for driver

TARGET_NAME:=mvp4

SRC_DIR:=src/drivers/audio/mvp4.src

SRCS		:= debug.c general_info.c misc.c mvp4snd.c

LINKS := dev/audio/old/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/media_p \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/drivers/audio/audio_drv/inc

SYSTEM_LIBS := libaudio_drv.a

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
