# Makefile for driver

TARGET_NAME:=i801

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= debug.c general_info.c hwio.c misc.c i801.c

LINKS := dev/audio/old/$(TARGET_NAME)

MY_INCLUDES :=		\
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver \
					-I$(BUILDHOME)/src/drivers/audio/audio_drv/inc

SYSTEM_LIBS := libaudio_drv.a

MY_CFLAGS := -D_SOUND_COMPATIBILITY


include $(SDIR_TEMPLATES)/DriverTemplate.mk
