# Makefile for driver

TARGET_NAME:=wavelan

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= wavelan.c \
		hcfio.c \
		hcf.c \
#

LINKS := dev/net/$(TARGET_NAME)

MY_INCLUDES := -I$(PRIVATE_INCLUDE_PATH)/driver

SYSTEM_LIBS :=

MY_CFLAGS := -Wall

ifneq ($(call MFLAG_IS_SET, WAVELAN_IS_MINIPCI),)
	MY_CFLAGS += -DHCF_HSI_VAR_2=1
endif


include $(SDIR_TEMPLATES)/DriverTemplate.mk
