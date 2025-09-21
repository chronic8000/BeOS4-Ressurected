# ide/ata Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=libide_ata.a

SRC_DIR:=src/drivers/ide/ata/lib

SRCS	:= 	common/devicelist.c \
			common/ide_device_info.c \
			common/block_io.c \
			common/ide_settings.c

SRCS += ata.c

MY_INSTALL_DIR :=

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/support_p \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/drivers/ide/include \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM)

SYSTEM_LIBS :=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE ATA -- \""

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk


TARGET_NAME:=ata

SRC_DIR:=src/drivers/ide/$(TARGET_NAME)

SRCS	:= 	common/devicelist.c \
			common/ide_device_info.c \
			common/block_io.c \
			common/ide_settings.c

# ifeq (x$(BUILD_LIB), x)
SRCS	+=	common/device_icons.c
# endif

SRCS += $(TARGET_NAME).c

LINKS := dev/disk/ide/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/support_p \
	-I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(BUILDHOME)/src/drivers/ide/include \
	-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM)

SYSTEM_LIBS :=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE ATA -- \""


include $(SDIR_TEMPLATES)/DriverTemplate.mk

