
TARGET_NAME:=mail_daemon

SRC_DIR:=src/servers/mail

SRCS := 				\
	mail_daemon.cpp		\
	SocketBuffer.cpp	\
	StatusWindow.cpp	\
	Password.cpp		\
	MiniStatusWindow.cpp \
	DeskbarReplicant.cpp \
	Bitmaps.cpp

MY_CFLAGS:=

MY_INCLUDES :=	-I$(BUILDHOME)/src/inc/app_p \
			   	-I$(BUILDHOME)/src/inc/os_p \
			   	-I$(BUILDHOME)/src/inc/tracker_p \
			   	-I$(BUILDHOME)/src/inc/mail_p\
			   	-I$(BUILDHOME)/src/inc/net_p \
			   	-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/src/inc/media_p \
				-I$(BUILDHOME)/src/inc \

SYSTEM_LIBS:= libbe.so libroot.so libtracker.so libmail.so \
				libsocket.so libbind.so libtextencoding.so

RESOURCE_FORK:= $(addsuffix .rsrc,$(TARGET_NAME))

MY_INSTALL_DIR:=$(INSTALL_SERVERS)

LINK_DIRS :=


include $(SDIR_TEMPLATES)/AppTemplate.mk
