
-include $(call INCLUDE_IF_AVAILABLE, \
	hid \
	kodak \
	printer \
	raw \
	tests \
	usb_acm \
	vdsp \
, src/drivers/usb)

ifeq ($(call MFLAG_IS_SET,USE_IAD_GEMPC430_DRIVER_SOURCES),)
-include $(call INCLUDE_IF_AVAILABLE, \
	gempc430 \
, src/drivers/usb)
else
-include $(call INCLUDE_IF_AVAILABLE, \
	gempc430_iad \
, src/drivers/usb)
endif
