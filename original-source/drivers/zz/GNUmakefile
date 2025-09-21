# Makefile for driver

TARGET_NAME:=zz

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= zz.c conf.c

LINKS := dev/ports/$(TARGET_NAME)

MY_INCLUDES :=  -I$(BUILDHOME)/src -I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/inc/os_p

include $(SDIR_TEMPLATES)/DriverTemplate.mk
