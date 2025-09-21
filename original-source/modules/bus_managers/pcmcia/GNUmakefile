include $(call INCLUDE_IF_AVAILABLE, \
	cb_enabler \
, src/modules/bus_managers/pcmcia)

ifeq ($(call MFLAG_IS_SET,USE_IAD_PCMCIA_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	cs \
	ds \
, src/modules/bus_managers/pcmcia)
else
include $(call INCLUDE_IF_AVAILABLE, \
	cs_iad \
	ds_iad \
, src/modules/bus_managers/pcmcia)
endif
