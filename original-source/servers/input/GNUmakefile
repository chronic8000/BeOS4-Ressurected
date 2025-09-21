
TARGET_NAME:=input_server

SRC_DIR:=src/servers/input

SRCS :=  \
	AddOns.cpp \
	DeviceLooper.cpp \
	InputServer.cpp \
	InputServerDevice.cpp \
	InputServerFilter.cpp \
	InputServerMethod.cpp \
	Keymap.cpp \
	MethodReplicant.cpp \
	BottomlineWindow.cpp

MY_CFLAGS:= -D_BUILDING_input=1

MY_INCLUDES	:= \
	-I$(BUILDHOME)/src/inc/app_server_p \
	-I$(BUILDHOME)/src/inc/input_server_p \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/headers/add-ons/input_server \
	-I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS:= libbe.so libroot.so

RESOURCE_FORK:= $(addsuffix .rsrc,$(TARGET_NAME))

MY_INSTALL_DIR:=$(INSTALL_SERVERS)

LINK_DIRS :=


include $(SDIR_TEMPLATES)/AppTemplate.mk

