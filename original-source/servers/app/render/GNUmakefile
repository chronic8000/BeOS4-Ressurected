
SRC_DIR:=src/servers/app/render

SRCS :=  \
	DrawEngine.cpp \
	LBXFormat0.cpp \
	LBXFormat1.cpp \
	Unpack.cpp \
	bezier.cpp \
	color_op.cpp \
	copyblits.cpp \
	edges.cpp \
	fastmath.cpp \
	fregion.cpp \
	lines.cpp \
	package1.cpp \
	package1516.cpp \
	package32.cpp \
	package8.cpp \
	path.cpp \
	render.cpp \
	renderLines.cpp \
	renderText.cpp \
	renderPixels.cpp \
	renderState.cpp \
	renderLock.cpp \
	renderArc.cpp \
	newPicture.cpp \
	font_draw_24.cpp \
	font_draw_8.cpp

MY_DIRS:=
MY_DIRS_PREFIX:=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/inc/app_server_p \
				-I$(BUILDHOME)/src/inc/graphics_p \
				-I$(BUILDHOME)/src/inc/app_p \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/headers/drivers \
				-I$(BUILDHOME)/headers/add-ons/graphics \
				-I$(BUILDHOME)/src/servers/app/bitstream/includes \
				-I$(BUILDHOME)/src/servers/app/render \
				-I$(BUILDHOME)/src/servers/app

MY_ASFLAGS :=

MY_CFLAGS := $(F_INLINE) $(F_NO_PIC) $(F_NO_EXCEPTIONS)

MY_DFLAGS:= -DBE -DENABLE_DRAWING=1

ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING_INTERNAL_LEAKCHECKING),)
	MY_DFLAGS	+= -DINTERNAL_LEAKCHECKING
endif

ifneq ($(call MFLAG_IS_SET, APP_SERVER_DEBUGGING),)
	MY_DFLAGS	+= -DDEBUGGING_CONSOLE
endif

ifneq ($(call MFLAG_IS_SET, ROTATE_DISPLAY),)
	MY_DFLAGS	+= -DROTATE_DISPLAY=1
endif

# The default warnings probably currently include most of these.
# MY_WARNINGS := -Wbad-function-cast -Wchar-subscripts -Wconversion \
			-Wformat -Wimplicit-function-dec -Wimplicit-int -Wno-long-long \
			-Wmain -Wmissing-braces -Wmissing-declarations -Wmissing-prototypes \
			-Wnested-externs -Wparentheses -Wpointer-arith \
			-Wstrict-prototypes -Wno-multichars

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk
