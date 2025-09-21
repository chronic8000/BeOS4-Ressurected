# ide/ata Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=libide_atapi.a

SRC_DIR:=src/drivers/ide/atapi/lib

SRCS	:= 	common/devicelist.c \
			common/block_io.c \
			common/ide_settings.c

SRCS += atapi.c \
				atapi_cmds.c \
				atapi_changer.c


MY_INSTALL_DIR :=

MY_INCLUDES := 	-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/inc/support_p \
   				-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/drivers/ide/include

SYSTEM_LIBS :=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE ATAPI -- \""

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk


TARGET_NAME:=atapi

SRC_DIR:=src/drivers/ide/atapi

# COMMON_SRCS
SRCS	:= 	common/devicelist.c \
			common/block_io.c \
			common/ide_settings.c

# COMMON_SRCS (not BUILD_LIB)
SRCS	+= \
	common/device_icons.c \
	common/ide_device_info.c

SRCS	+=	atapi.c \
			atapi_cmds.c \
			atapi_changer.c

LINKS := dev/disk/ide/$(TARGET_NAME)

MY_INCLUDES := 	-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/inc/support_p \
   				-I$(BUILDHOME)/src/nukernel/arch/$(TARGET_PLATFORM) \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/drivers/ide/include

SYSTEM_LIBS :=

MY_CFLAGS := -DDEBUGPREFIX="\"IDE ATAPI -- \""


include $(SDIR_TEMPLATES)/DriverTemplate.mk

