# Makefile for driver

TARGET_NAME:=xpress

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= xpress.c xpress_hw.c xpress_ioctl.c xpress_stream.c \
				ac97.c xpress_ac97glue.c xpress_mix.c

LINKS := dev/audio/game/$(TARGET_NAME)

MY_INCLUDES := \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
