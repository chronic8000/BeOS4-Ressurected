# Makefile for driver

TARGET_NAME:=speaker

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/audio/pcbeep/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/os_p \
	-I$(PRIVATE_INCLUDE_PATH)/driver

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
