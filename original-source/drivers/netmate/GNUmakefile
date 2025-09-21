# Makefile for driver

TARGET_NAME:=netmate

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= $(TARGET_NAME).c

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver \
				-I$(PRIVATE_INCLUDE_PATH)/usb_p

SYSTEM_LIBS :=

MY_CFLAGS := -DB_USB_USE_V2_MODULE=1


include $(SDIR_TEMPLATES)/DriverTemplate.mk
