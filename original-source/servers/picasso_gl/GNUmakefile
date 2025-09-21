#
# Important note: Original defined BEEPFILES as a prerequisite of install
#   This one doesn't deal with BEEPFILES at all.
#

TARGET_NAME:=picasso_gl

TYPE:=APP

SRC_DIR:=src/servers/picasso_gl
MY_DIRS_PREFIX:=src/servers/picasso_gl


SRCS :=  \
	OGLDevice.cpp			\
	OGLPlane.cpp			\
	OGLSurface.cpp			\
	input.cpp				\
	parcel.cpp				\
	Picasso.cpp				\
	RenderHole.cpp			\
	RenderStream.cpp		\
	server.cpp				\
	Surface.cpp				\
	TextureCache.cpp		\
#

MY_DIRS	:= dummy
#fonts render

OUR_CFLAGS := -DBINDER_DEBUG_LIB=1
OUR_ASFLAGS :=
OUR_INCLUDES :=
OUR_OPTIMIZER := -O1

MY_CFLAGS:= $(F_NO_EXCEPTIONS) $(F_INLINE) -O0 -g
MY_ASFLAGS :=

MY_DFLAGS += $(call MFLAG_TO_DFLAG, NU_SCROLL_BAR)
MY_DFLAGS += $(call MFLAG_TO_DFLAG, SONY_SCROLL_BAR)

MY_INCLUDES	:=	-I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/kit/libbe2 \
				-I$(BUILDHOME)/src/inc/app_server_p \
				-I$(BUILDHOME)/src/inc/render2_p \
				-I$(BUILDHOME)/src/inc/graphics_p \
				-I$(BUILDHOME)/src/inc/app_p \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/headers/drivers \
				-I$(BUILDHOME)/headers/add-ons/graphics \
				-I$(BUILDHOME)/headers/font2 \
				-I$(BUILDHOME)/$(SRC_DIR)

#				-I$(BUILDHOME)/$(SRC_DIR)/bitstream/includes \
# 				-I$(BUILDHOME)/$(SRC_DIR)/render \
# 				-I$(BUILDHOME)/$(SRC_DIR)/fonts \
# 				-I$(BUILDHOME)/$(SRC_DIR)/cursor \
#

SYSTEM_LIBS	:= libroot.so libbe2.so libfont2.so libGL2.so libtextencoding.so

ifneq ($(call MFLAG_IS_SET, ROTATE_DISPLAY),)
	MY_DFLAGS	+= -DROTATE_DISPLAY=1
endif

ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING_INTERNAL_LEAKCHECKING),)
	MY_CFLAGS	+= -DINTERNAL_LEAKCHECKING
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

# No choice until we can bootstrap a GL device from scratch.
#ifneq ($(call MFLAG_IS_SET, USE_MINI_APP_SERVER),)
	MY_CFLAGS	+= -DMINI_SERVER
	SYSTEM_LIBS	+= libgame.so libbe.so
	SRCS += mini.cpp
#endif

MY_CFLAGS += -DENABLE_DRAWING=1 -DBE $(APP_SERVER_FLAGS)

RESOURCES:= $(addsuffix .rsrc,$(TARGET_NAME))

MY_INSTALL_DIR:=$(INSTALL_SERVERS)

LINK_DIRS :=

COLOR_FLAGS := $(MY_CFLAGS) $(MY_INCLUDES)

MORE_OBJS:=$(BELIBRARIES)/binder_addon_glue.o

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

ELF_FILES += $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)
