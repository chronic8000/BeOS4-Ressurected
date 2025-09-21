#
# Important note: Original defined BEEPFILES as a prerequisite of install
#   This one doesn't deal with BEEPFILES at all.
#

TARGET_NAME:=app_server

TYPE:=APP

SRC_DIR:=src/servers/app
MY_DIRS_PREFIX:=src/servers/app


SRCS :=  \
	bitmapcollection.cpp \
	session.cpp \
	app.cpp \
	asm.S \
	code.asm \
	as_debug.cpp \
	atom.cpp \
	bitmap.cpp \
	colormap.c \
	compileInfo.c \
	cursor.cpp \
	cursor_data.cpp \
	debug_update.cpp \
	dwindow.cpp \
	eventport.cpp \
	font.cpp \
	font_cache.cpp \
	font_context.cpp \
	font_data.c \
	font_engine.cpp \
	font_family.cpp \
	font_file.cpp \
	font_manager.cpp \
	font_renderer.cpp \
	font_set.cpp \
	font_style.cpp \
	font_support.cpp \
	graphicDevice.cpp \
	heap.cpp \
	input.cpp \
	keymap.c \
	lock.cpp \
	main.cpp \
	mwindow.cpp \
	nuDecor.cpp \
	off.cpp \
	old_ui_settings.cpp \
	parcel.cpp \
	rect.cpp \
	sam.cpp \
	scroll_bar.cpp \
	server.cpp \
	settings.cpp \
	support.cpp \
	system.cpp \
	token.cpp \
	view.cpp \
	window.cpp	\
	workspace.cpp

DEFAULT_FONT_ENGINE:=fontfusion

ifeq ($(call MVAR_TO_TARGET_NAME, APP_SERVER_FONT_ENGINE, $(DEFAULT_FONT_ENGINE)), fontfusion)
	MY_DIRS	:=
	SRCS	+= 	glue.cpp
else
	MY_DIRS	:= bitstream
	SRCS	+= 	glue_bs.cpp
endif

MY_DIRS += render decor

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_DEMO_CD),)
SRCS	+= 	bitmap_demo_cd.cpp \
			demo_cd.cpp
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
	SRCS	+= 	TCPIPSocket.cpp
endif

# XXX - Ah, the following can never happen.
ifeq ($(LINKING), static)
	ADDONS_DIR := $(BUILDHOME)/src/addons/app_server
	# STATIC_DRIVERS
	SRCS += \
		$(ADDONS_DIR)/matrox/accel.c \
		$(ADDONS_DIR)/matrox/driver.c \
		$(ADDONS_DIR)/matrox/video.c \
		$(ADDONS_DIR)/matrox/ops.c
	SRCS += init_app_sta.c
endif

OUR_CFLAGS :=
OUR_ASFLAGS :=
OUR_INCLUDES :=
OUR_OPTIMIZER := -O1

MORE_OBJS :=

MY_CFLAGS:= $(F_NO_EXCEPTIONS) $(F_INLINE)
MY_ASFLAGS :=

MY_DFLAGS += $(call MFLAG_TO_DFLAG, NU_SCROLL_BAR)
MY_DFLAGS += $(call MFLAG_TO_DFLAG, SONY_SCROLL_BAR)

# Enable LBX drawing
MY_DFLAGS += -DENABLE_DRAWING=1

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc

ifeq ($(call MVAR_TO_TARGET_NAME, APP_SERVER_FONT_ENGINE, $(DEFAULT_FONT_ENGINE)), fontfusion)
MY_INCLUDES += \
				-I$(BUILDHOME)/src/kit/font/fontfusion/core
endif

ifeq ($(call MVAR_TO_TARGET_NAME, APP_SERVER_FONT_ENGINE, $(DEFAULT_FONT_ENGINE)), bitstream)
MY_INCLUDES += \
				-I$(BUILDHOME)/$(SRC_DIR)/bitstream/includes
endif

MY_INCLUDES +=	-I$(BUILDHOME)/$(SRC_DIR)/render \
				-I$(BUILDHOME)/$(SRC_DIR)/decor \
				-I$(BUILDHOME)/src/inc/app_server_p \
				-I$(BUILDHOME)/src/inc/graphics_p \
				-I$(BUILDHOME)/src/inc/app_p \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/headers/drivers \
				-I$(BUILDHOME)/headers/add-ons/graphics \
				-I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS	:= libtextencoding.so libroot.so libbe.so

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc3) 
SYSTEM_LIBS += libsupc++.a
endif

$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2)

ifeq ($(call MVAR_TO_TARGET_NAME, APP_SERVER_FONT_ENGINE, $(DEFAULT_FONT_ENGINE)), fontfusion)
SYSTEM_LIBS	+= libfont.so
endif

ifneq ($(call MFLAG_IS_SET, USE_BOOT_ANIMATION),)
	MY_CFLAGS	+= -DBOOT_ANIMATION
endif

ifneq ($(call MFLAG_IS_SET, ROTATE_DISPLAY),)
MY_DFLAGS	+= -DROTATE_DISPLAY=1
endif

ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING_INTERNAL_LEAKCHECKING),)
	MY_CFLAGS	+= -DINTERNAL_LEAKCHECKING
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
	MY_CFLAGS	+= -DREMOTE_DISPLAY
	SYSTEM_LIBS	+= libnet.so
endif

ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING),)
	MY_CFLAGS	+= -DDEBUGGING_CONSOLE
	SYSTEM_LIBS	+= libdevel.a
	ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING_SYNC_CALL_LOGGING),)
		MY_CFLAGS	+= -DSYNC_CALL_LOG
	endif
	ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING_PROTOCOL_BANDWIDTH_LOGGING),)
		MY_CFLAGS	+= -DPROTOCOL_BANDWIDTH_LOG
	endif
endif

ifneq ($(call MFLAG_IS_SET, USE_MINI_APP_SERVER),)
	MY_CFLAGS	+= -DMINI_SERVER
	SYSTEM_LIBS	+= libgame.so
endif

MY_CFLAGS += -DENABLE_DRAWING=1 -DBE $(APP_SERVER_FLAGS)

LOCAL_RESOURCES:= $(addsuffix .rsrc,$(TARGET_NAME)) \
	resources/R5/R5.rsrc
RESOURCES:= $(LOCAL_RESOURCES)

MY_INSTALL_DIR:=$(INSTALL_SERVERS)

LINK_DIRS :=

COLOR_FLAGS := $(MY_CFLAGS) $(MY_INCLUDES)

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

# Update compileInfo whenever any source has changed.
$(OBJ_DIR_OUT)/compileInfo.o: $(SRCS_OUT)

ELF_FILES += $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)

SRC_DIR:=src/servers/app
SOURCE := colormap.c
REMOVE_CFLAGS := -pipe
include $(SDIR_TEMPLATES)/OverrideFlagsTemplate.mk

RESOURCES:= $(LOCAL_RESOURCES)
TARGET_NAME:=app_server.rsrc
SRC_DIR:=src/servers/app
MY_INSTALL_DIR:=$(INSTALL_SERVERS)
include $(SDIR_TEMPLATES)/ResourceTemplate.mk

###

# XXX - Why were all the app_server settings files dependant upon the
#   makefile? (steven 2001-02-16)

SRC_DIR:=src/servers/app
SOURCE:= $(call MVAR_TO_TARGET_NAME, USER_SETTINGS_APP_SERVER_SETTINGS, app_server_settings.others)
TARGET:=$(INSTALL_USER_SETTINGS)/app_server_settings
ATTRIBUTES:=
PERMISSIONS:=$(PERMS_USER_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

###

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:= app_server_settings.fic.crt
TARGET:=$(INSTALL_USER_SETTINGS)/app_server_settings.crt
ATTRIBUTES:=
PERMISSIONS:=$(PERMISSIONS_IN)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:= app_server_settings.fic.pal
TARGET:=$(INSTALL_USER_SETTINGS)/app_server_settings.pal
ATTRIBUTES:=
PERMISSIONS:=$(PERMISSIONS_IN)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:= app_server_settings.fic.ntsc
TARGET:=$(INSTALL_USER_SETTINGS)/app_server_settings.ntsc
ATTRIBUTES:=
PERMISSIONS:=$(PERMISSIONS_IN)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:= app_server_settings.fic.1024
TARGET:=$(INSTALL_USER_SETTINGS)/app_server_settings.1024
ATTRIBUTES:=
PERMISSIONS:=$(PERMISSIONS_IN)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk     

include $(call INCLUDE_IF_AVAILABLE, \
	decor/compiler \
	resources \
, src/servers/app)

LOCAL_RESOURCES:=
