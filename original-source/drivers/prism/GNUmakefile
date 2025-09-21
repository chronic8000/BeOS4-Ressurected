# Makefile for driver

TARGET_NAME:=prism

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:=	\
		prism.c \
		prism2mgmt.c \
		fclock.c \
		p80211conv.c \
		net_data.c \
#

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := \
			-I$(BUILDHOME)/src/inc/driver \
			-I$(BUILDHOME)/src/inc/driver/pcmcia \
#

SYSTEM_LIBS :=

MY_CFLAGS := -g -UWLAN_AP -DWLAN_STA=1 -Wall -Wcast-align -Wpointer-arith

include $(SDIR_TEMPLATES)/DriverTemplate.mk

