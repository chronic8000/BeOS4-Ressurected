
TARGET_NAME:= gamedriver

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= gamedriver.c

LINKS := \
		dev/audio/game/$(TARGET_NAME) \

MY_INCLUDES		:= \
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver \
#

SYSTEM_LIBS :=

MY_CFLAGS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk

