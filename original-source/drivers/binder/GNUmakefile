# Makefile for driver

TARGET_NAME:=binder

SRC_DIR:=src/drivers/$(TARGET_NAME)

SRCS:= \
	binder_dev.c \
	binder_node.c \
	port_pool.c \
	binder_request.c \
	binder_command.c \
	binder_connection.c

LINKS := dev/misc/$(TARGET_NAME)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=

include $(SDIR_TEMPLATES)/DriverTemplate.mk
