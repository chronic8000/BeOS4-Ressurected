SRC_DIR:=src/drivers/tm/tmman_libload

TARGET_NAME := tmman_libload.a

SRCS := Lib_Exceptions.c
SRCS += Lib_FileIODriver.c
SRCS += Lib_IODrivers.c
SRCS += Lib_Local.c
SRCS += Lib_Mapping.c
SRCS += Lib_Memspace.c
SRCS += Lib_Messages.c
SRCS += Lib_StdFuncs.c
SRCS += Scatter_Shuffle.c
SRCS += SymtabSearch.c
SRCS += TMDownLoader.c
SRCS += TMDownLoaderFromFile.c
SRCS += TMRelocate.c
SRCS += TMScatter.c
SRCS += XFlatObject.c
SRCS += bsearch.c
SRCS += compr_shuffle.c
SRCS += compr_unshuffle.c
SRCS += lib_list.c
SRCS += lib_set.c
SRCS += objrepr.c


MY_CFLAGS := 

MY_INCLUDES :=

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

#SRC_DIR := $(SRC_DIR_IN)
#FILES := zlib.h zconf.h
#MY_INSTALL_DIR := $(INSTALL_DEV_GNU_HEADERS)
#PERMISSIONS := $(PERMS_SYSTEM_DATA)
#include $(SDIR_TEMPLATES)/DataTemplate.mk

