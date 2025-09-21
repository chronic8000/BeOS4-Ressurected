
TARGET_NAME:= ua100

SRC_DIR:=src/drivers/audio/$(TARGET_NAME)

SRCS		:= ua100.c pcm.c old.c midi.c parser.c

LINKS := \
		dev/audio/old/$(TARGET_NAME) \
#

MY_INCLUDES		:= \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/src/inc/media_p \
					-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS :=

MY_CFLAGS :=

MY_DFLAGS += # -DDEBUG=1

include $(SDIR_TEMPLATES)/DriverTemplate.mk
