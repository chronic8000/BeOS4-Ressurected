# Makefile for driver

TARGET_NAME:=null_audio

SRC_DIR:=src/drivers/audio/null_audio

SRCS:= $(ALL_DOT_C)

LINKS := dev/audio/old/$(TARGET_NAME)

MY_INCLUDES := 		-I$(BUILDHOME)/src/drivers/audio/audio_drv/inc \
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS := libaudio_drv.a

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
