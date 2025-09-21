# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=sis

SRC_DIR:=src/drivers/graphics/SiS

SRCS		:= bena4.c driver.c setup.c sisDPMS.c sisVideoModes.c sisClock.c
SRCS		+= sisCursor.c sisCRTC.c sis630interrupts.c

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/graphics_p $(INCLUDES)
MY_INCLUDES	+= -I$(BUILDHOME)/src/inc/graphics_p/SiS
MY_INCLUDES	+= -I$(BUILDHOME)/src/kit/dinky/include
MY_INCLUDES	+= -I$(BUILDHOME)/src/kit/surface/include
MY_INCLUDES	+= -I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS := 

MY_CFLAGS		+= -DOBJ_SIS5598=1
MY_CFLAGS 		+= -DOBJ_SIS6326=1
MY_CFLAGS 		+= -DOBJ_SIS620=1
MY_CFLAGS 		+= -DOBJ_SIS630=1

include $(SDIR_TEMPLATES)/DriverTemplate.mk

