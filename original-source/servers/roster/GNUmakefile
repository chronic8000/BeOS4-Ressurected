
TARGET_NAME:=registrar

SRC_DIR:=src/servers/roster

SRCS :=  \
	clip.cpp \
	main.cpp \
	mime.cpp \
	mime_table.cpp \
	Monitor.cpp \
	Shutdown.cpp \
	ShutdownUtil.cpp \
	RecentFiles.cpp \
	Timer.cpp \
	task.cpp

MY_CFLAGS:=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/app_p \
				-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/src/inc/interface_p \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/tracker_p \
				-I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS:= libbe.so libroot.so

RESOURCE_FORK:= $(addsuffix .rsrc,$(TARGET_NAME))

MY_INSTALL_DIR:=$(INSTALL_SERVERS)

LINK_DIRS :=


include $(SDIR_TEMPLATES)/AppTemplate.mk
