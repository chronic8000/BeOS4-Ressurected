
include $(call INCLUDE_IF_AVAILABLE, \
	3dfx \
	ati \
	g400 \
	i740 \
	i810 \
	lynxem \
	matrox \
	neomagic \
	null \
	nvidia \
	rage128 \
	radeon \
	SiS \
	trident \
	udurango \
	webpad \
, src/drivers/graphics)


ifneq ($(call MVAR_EXISTS, VESA_MODE_HACK),)

# Makefile using GenericGeneratedTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
TARGET_NAME := vesa

MY_RULE := VESA_MODE_HACK_RULE
MY_DEPS :=

# The VESA_MODE_HACK_RULE works by using the VESA_MODE_HACK MVAR.

SRC_DIR :=
MY_INSTALL_DIR := $(INSTALL_DRIVER_SETTINGS)

include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

endif

