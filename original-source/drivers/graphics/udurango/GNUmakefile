# Driver Makefile
# Copyright (c) 2000; Be, Inc. All Rights Reserved.

TARGET_NAME:=durango.graphics

SRC_DIR:=src/drivers/graphics/udurango

SRCS	:= driver.c 

LINKS := dev/graphics/$(TARGET_NAME)

MY_INCLUDES := \
	-I. \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/inc/graphics_p \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS := 

MY_CFLAGS := -DDEBUG=0

ifneq ($(call MVAR_EXISTS, DISPLAY_TYPES),)
	ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, NTSC),NTSC)
		MY_CFLAGS += -DCH7005_SUPPORT=1
	else
		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, PAL),PAL)
			MY_CFLAGS += -DCH7005_SUPPORT=1
		endif
	endif
endif

ifeq ($(call MFLAG_IS_SET, GRAPHICS_DURANGO_TWEAK_FIFO),)
	MY_CFLAGS += -DTWEAK_FIFO=1
endif

include $(SDIR_TEMPLATES)/DriverTemplate.mk

#
#  Accelerant
#

TARGET_NAME:=durango.accelerant

SRC_DIR:=$(SRC_DIR_IN)

SRCS		:= accelerant.c

MY_CFLAGS := -DCOMPILING_ACCELERANT -DDEBUG=0 -DSERIAL_DEBUG
# MUST not use this with overlays, at least on the board I have.
# NOTE: this flag affects FIFO parameters!
#MY_CFLAGS += -DHALF_SPEED_UPDATE=1

ifeq ($(call MFLAG_IS_SET, VNORM_FROM_GPIO),)
	MY_CFLAGS += -DVNORM_FROM_GPIO=0
else
	MY_CFLAGS += -DVNORM_FROM_GPIO=1
endif

ifeq ($(call MFLAG_IS_SET, ROTATE_DISPLAY),)
	# Display is not rotated
	ifeq ($(call MVAR_EXISTS, DISPLAY_TYPES),)
		# Do not use tv-stuffs
		MY_CFLAGS += -DNORMAL_MODES=1
	else

		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, PRESET), PRESET)
			MY_CFLAGS += -DONLY_PRESET_TIMINGS=1
		else
			MY_CFLAGS += -DONLY_PRESET_TIMINGS=0
		endif

		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, NTSC),NTSC)
			MY_CFLAGS += -DCH7005_SUPPORT=1
		else
			ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, PAL),PAL)
				MY_CFLAGS += -DCH7005_SUPPORT=1
			endif
		endif

		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, NTSC),NTSC)
			MY_CFLAGS += -DNTSC_MODES=1
		else
			MY_CFLAGS += -DNTSC_MODES=0
		endif

		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, PAL),PAL)
			MY_CFLAGS += -DPAL_MODES=1
		else
			MY_CFLAGS += -DPAL_MODES=0
		endif

		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, CRT),CRT)
			MY_CFLAGS += -DNORMAL_MODES=1
		else
			MY_CFLAGS += -DNORMAL_MODES=0
		endif
		ifeq ($(call MVAR_CONTAINS, DISPLAY_TYPES, LCD),LCD)
			MY_CFLAGS += -DENABLE_FLAT_PANEL=1
			MY_CFLAGS += -D$(call MVAR_TO_TARGET_NAME, LCD_DISPLAY_SIZE)
		else
			MY_CFLAGS += -DENABLE_FLAT_PANEL=0
		endif
$(call MVAR_CHECK_FOR_UNKNOWN, DISPLAY_TYPES, LCD NTSC CRT PAL PRESET)
	endif
else
	MY_CFLAGS += -DSONY_ROTATED
endif

MY_INCLUDES:= \
	-I./inc210 \
	-I$(BUILDHOME)/src/inc/graphics_p \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS:=libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-durango.accelerant"

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk


ifneq ($(call MVAR_EXISTS, DISPLAY_TYPES),)

# DTS has never heard of durango_tv_settings.
# That means it must not currently be required. (00-09-13 steven)
ifneq ($(call MVAR_EXISTS, USER_SETTINGS_DURANGO_TV_SETTINGS),)

SRC_DIR:=$(SRC_DIR_IN)/settings

$(call MVAR_CHECK_AND_SHOW_OPTIONS, USER_SETTINGS_DURANGO_TV_SETTINGS)

SOURCE:=$(call MVAR_TO_TARGET_NAME, USER_SETTINGS_DURANGO_TV_SETTINGS)
TARGET:=$(INSTALL_USER_SETTINGS)/durango_tv_settings
ATTRIBUTES :=

MY_DEPS:=$(SRC_DIR)/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

endif

endif
