# Makefile for driver

TARGET_NAME:=8086_2415

SRC_DIR:=src/modules/media/gamedriver/i801

SRCS		:= i801.c

MY_INCLUDES := \
	-I$(BUILDHOME)/src/drivers/audio/gamedriver

SYSTEM_LIBS:= 

MY_CFLAGS := 



INSTALL_PREFIX := $(INSTALL_MODULES)/media/gamedriver/pci
INSTALL_TARGET := $(TARGET_NAME)


include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk


#  *SOURCE        := linked-to file
#	SRC_DIR		  := maintained for compatibility
#   LINK_PREFIX   := the base directory for LINKS (May be empty)
#					 (totally optional) (equilivant to MY_INSTALL_DIR)
#   LINK_RELATION := the common root dir of SOURCE and LINK_PREFIX
#                    if this is empty, you get an absolute link, otherwise
#                    relative (required for relative links)
#   LINKS         := the links to create, based on LINK_PREFIX if available
#	MY_DEPS :=	additional dependancies
SOURCE:=8086_2415
SRC_DIR:=src/modules/media/gamedriver/i801
LINK_PREFIX:=$(INSTALL_MODULES)/media/gamedriver/pci
LINK_RELATION :=
LINKS:=8086_7195 8086_2445 8086_2425
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

