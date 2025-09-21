# Drivers Makefile

include $(BUILDHOME)/src/drivers/linux_ether/ec9xx/$(MAKEFILE_LEAFNAME)

ifeq ($(call MFLAG_IS_SET,USE_IAD_EEPRO_DRIVER_SOURCES),)
include $(BUILDHOME)/src/drivers/linux_ether/eepro100/$(MAKEFILE_LEAFNAME)
else
include $(BUILDHOME)/src/drivers/linux_ether/eepro100_iad/$(MAKEFILE_LEAFNAME)
endif

