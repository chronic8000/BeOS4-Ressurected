# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

# Note: The old-system makefile had a hack to create the vesa config file.
#   In the new system, that functionality is handled by the VESA_MODE_HACK MVAR
#   and should be defined on a device-specific basis.

TARGET_NAME:=webpad.graphics

SRC_DIR:=src/drivers/graphics/webpad

SRCS	:= driver.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/graphics_p

SYSTEM_LIBS :=

MY_CFLAGS := 


include $(SDIR_TEMPLATES)/DriverTemplate.mk
