# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=rage128

SRC_DIR:=src/drivers/graphics/rage128

include $(SRC_DIR_OUT_FROM_SRC_DIR)/skel/$(MAKEFILE_LEAFNAME)-driver_srcs

SRCS	:= \
		r128_init.c \
		r128_ioctl.c \
		r128_vbi.c \
		r128_supported_devices.c \
		$(addprefix skel/, $(GDS_KERN_SRCS))

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
		-I$(BUILDHOME)/src/inc/graphics_p/alt.software/skel \
		-I$(SRC_DIR_OUT_FROM_SRC_DIR)/skel \
		-I$(BUILDHOME)/src/inc/graphics_p/rage128 \
		-I$(BUILDHOME)/headers \
		-I$(BUILDHOME)/headers/add-ons/graphics \
		-I$(BUILDHOME)/headers/interface \
		-I$(BUILDHOME)/headers/device \
		-I$(BUILDHOME)/headers/drivers \
		-I$(BUILDHOME)/headers/kernel \
		-I$(BUILDHOME)/headers/posix \
		-I$(BUILDHOME)/headers/support \
		-I$(BUILDHOME)/headers/storage \
		-I$(GCCHOME)/include

SYSTEM_LIBS := 

MY_CFLAGS := -DDEBUG=0 

include $(SDIR_TEMPLATES)/DriverTemplate.mk

