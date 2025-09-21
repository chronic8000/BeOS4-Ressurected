ifeq ($(call MFLAG_IS_SET,USE_IAD_MTRR_MODULE_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	mtrr \
, src/modules/cpu)
else
include $(call INCLUDE_IF_AVAILABLE, \
	mtrr_iad \
, src/modules/cpu)
endif
