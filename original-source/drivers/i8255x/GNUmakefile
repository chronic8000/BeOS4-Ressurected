# Makefile for driver

TARGET_NAME:=i8255x

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS		:=	i8255x_basics.c				\
#

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := \
			-I$(BUILDHOME)/src/inc/driver	\
			-I.							\
			-Icommon

SYSTEM_LIBS :=

MY_CFLAGS :=

include $(SDIR_TEMPLATES)/DriverTemplate.mk

