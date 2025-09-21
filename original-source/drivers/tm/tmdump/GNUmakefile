TARGET_NAME:=tmdump

SYSTEM_LIBS:= libroot.so

MY_INCLUDES := -I$(BUILDHOME)/src/drivers/tm/tmman_api
MY_INCLUDES += -I$(BUILDHOME)/src/drivers/tm/tmman_drv
MY_INCLUDES += -I$(BUILDHOME)/src/drivers/tm/tmman_libload

SRCS := tmdump.cpp
# file access
SRCS += ../be_files.c
# tmman_api
SRCS += ../tmman_api/tmmanapi.c
SRCS += ../tmman_api/verinfo.c
SRCS += ../tmman_api/error.c
# tmman_libload
SRCS += ../tmman_libload/Lib_Exceptions.c
SRCS += ../tmman_libload/Lib_FileIODriver.c
SRCS += ../tmman_libload/Lib_IODrivers.c
SRCS += ../tmman_libload/Lib_Local.c
SRCS += ../tmman_libload/Lib_Mapping.c
SRCS += ../tmman_libload/Lib_Memspace.c
SRCS += ../tmman_libload/Lib_Messages.c
SRCS += ../tmman_libload/Lib_StdFuncs.c
SRCS += ../tmman_libload/Scatter_Shuffle.c
SRCS += ../tmman_libload/SymtabSearch.c
SRCS += ../tmman_libload/TMDownLoader.c
SRCS += ../tmman_libload/TMDownLoaderFromFile.c
SRCS += ../tmman_libload/TMRelocate.c
SRCS += ../tmman_libload/TMScatter.c
SRCS += ../tmman_libload/XFlatObject.c
SRCS += ../tmman_libload/bsearch.c
SRCS += ../tmman_libload/compr_shuffle.c
SRCS += ../tmman_libload/compr_unshuffle.c
SRCS += ../tmman_libload/lib_list.c
SRCS += ../tmman_libload/lib_set.c
SRCS += ../tmman_libload/objrepr.c



RESOURCE_FORK:=

LINK_DIRS:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRC_DIR:=src/drivers/tm/tmdump

MY_CFLAGS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
