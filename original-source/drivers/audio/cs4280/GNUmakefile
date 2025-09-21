# Makefile for driver

TARGET_NAME:=cs4280

SRC_DIR:=src/drivers/audio/cs4280

SRCS		:= debug.c general_info.c hwio.c misc.c cs4280.c

LINKS := dev/audio/old/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/drivers/audio/audio_drv/inc \
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS := libaudio_drv.a

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
