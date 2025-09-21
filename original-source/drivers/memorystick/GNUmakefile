# Makefile for driver

TARGET_NAME:=memorystick

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS		:= 	driver.c \
                memory_stick.c \
				block_translation.c \
				tpc_parallel.c \
				tpc_parallel_dma.c

LINKS := dev/disk/memorystick/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
