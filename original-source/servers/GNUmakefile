# Makefile

include $(call INCLUDE_IF_AVAILABLE, \
	app \
	audio \
	cd \
	debug \
	hood \
	input \
	mail \
	media \
	midi \
	minisnd \
	print \
	roster \
	smoove \
	syslog \
	watcher \
, src/servers)

ifeq ($(call MFLAG_IS_SET,USE_IAD_SC_SERVER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	sc \
, src/servers)
else
include $(call INCLUDE_IF_AVAILABLE, \
	sc_iad \
, src/servers)
endif
