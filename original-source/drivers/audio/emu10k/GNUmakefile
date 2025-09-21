# Makefile for driver

TARGET_NAME:=emu10k

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= $(TARGET_NAME).c

LINKS := dev/audio/$(TARGET_NAME) \
#

MY_INCLUDES		:= \
					-I$(BUILDHOME)/headers/drivers \
					-I$(BUILDHOME)/src/inc/driver

include $(SDIR_TEMPLATES)/DriverTemplate.mk

