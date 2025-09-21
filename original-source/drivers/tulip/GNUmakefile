# Makefile for driver

TARGET_NAME:=tulip

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).cpp

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := \
					-I$(BUILDHOME)/src/inc/driver \
					-I$(BUILDHOME)/src/inc/driver/pcmcia

SYSTEM_LIBS :=

MY_CFLAGS :=

### XXX - Fix the following when we do PPC cleanup (This isn't a good way to do it.)
ifeq ($(TARGET_PLATFORM), mac)
MY_CFLAGS			+= -DMAC
endif


include $(SDIR_TEMPLATES)/DriverTemplate.mk
