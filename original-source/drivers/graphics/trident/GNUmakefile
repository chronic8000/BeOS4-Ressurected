# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=trident

SRC_DIR:=src/drivers/graphics/$(TARGET_NAME)

SRCS	:= driver.c video.c cursor.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(PRIVATE_INCLUDE_PATH) \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS := libdinky.a

MY_CFLAGS := -DFORCE_CLEAR_SCREEN


include $(SDIR_TEMPLATES)/DriverTemplate.mk
