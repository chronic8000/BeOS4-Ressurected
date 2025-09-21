# Makefile for driver

TARGET_NAME:=scsi_dsk

SRC_DIR :=src/nukernel/drivers

SRCS := $(TARGET_NAME).c device_icons.c scsi_common.c

LINKS := dev/disk/scsi/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU) \
	-I$(BUILDHOME)/src/nukernel/inc

SYSTEM_LIBS :=

MY_CFLAGS := -DBUILD_LOADABLE -DICONS_DISK



include $(SDIR_TEMPLATES)/DriverTemplate.mk
