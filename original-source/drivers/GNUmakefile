# Drivers Makefile
# This should (currently) have an entry for everything

include $(call INCLUDE_IF_AVAILABLE, \
	1394 \
	acer_wp_touch \
	aqua_buttons \
	audio \
	bcm42xx \
	binder \
	bt848 \
	clipper_leds \
	config \
	cpq_gpio \
	dc10 \
	disk_on_chip \
	dp83815 \
	dt300 \
	e8255x \
	epic100 \
	fic_video_switch \
	floppy \
	fmap \
	gameport \
	genpool \
	graphics \
	hamachi \
	history \
	homelan \
	ibm730_leds \
	ide \
	kb_mouse \
	linux_ether \
	lucent_hc \
	memorystick \
	misc \
	mk712 \
	netmate \
	ns9210 \
	parallel \
	pcmcia \
	pctel \
	perfmon_cpu \
	pty \
	ramdisk \
	rangelan \
	rtl8139 \
	scsiprobe \
	scsi_cd \
	scsi_dsk \
	scsi_raw \
	tcm509 \
	tcm589 \
	tcm9xx \
	tlanpci \
	trimodem \
	usb \
	vt86c100 \
	wavelan \
	zz \
, src/drivers)

ifeq ($(call MFLAG_IS_SET,USE_IAD_ETHERPCI_DRIVER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	etherpci \
, src/drivers)
else
include $(call INCLUDE_IF_AVAILABLE, \
	etherpci_iad \
, src/drivers)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_NE2K_DRIVER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	ne2k \
, src/drivers)
else
include $(call INCLUDE_IF_AVAILABLE, \
	ne2k_iad \
, src/drivers)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_TULIP_DRIVER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	tulip \
, src/drivers)
else
include $(call INCLUDE_IF_AVAILABLE, \
	tulip_iad \
, src/drivers)
endif
