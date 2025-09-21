# Makefile for driver

TARGET_NAME:=kb_mouse

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/input/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
	-I$(PRIVATE_INCLUDE_PATH)/os_p

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
