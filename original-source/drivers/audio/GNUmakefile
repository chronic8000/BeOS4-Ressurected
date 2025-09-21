# Drivers Makefile
# This should (currently) have an entry for everything

include $(call INCLUDE_IF_AVAILABLE, \
	audio_drv \
	cs4236 \
	cs4280 \
	echogals \
	emu10k \
	es137x \
	ess1938 \
	gamedriver \
	i801 \
	null_audio \
	opti931 \
	sonic_vibes \
	sonorus \
	speaker \
	ua100 \
	vortex \
	ymf715 \
	ymf724 \
, src/drivers/audio)

ifeq ($(call MFLAG_IS_SET,USE_IAD_AWE64_DRIVER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	awe64 \
, src/drivers/audio)
else
include $(call INCLUDE_IF_AVAILABLE, \
	awe64_iad \
, src/drivers/audio)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_MVP4_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	mvp4 \
, src/drivers/audio)
else
include $(call INCLUDE_IF_AVAILABLE, \
	mvp4.src \
, src/drivers/audio)
endif

