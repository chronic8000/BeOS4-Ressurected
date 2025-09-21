# Makefile for driver

TARGET_NAME:=doch

SRC_DIR:=src/drivers/disk_on_chip

# COMMON_SRCS
SRCS	:=	$(addprefix src/, \
	blockdev.c	\
	diskonc.c	\
	docsoc.c	\
	docsys.c	\
	flbase.c	\
	flflash.c	\
	flsocket.c	\
	fltl.c		\
	nftllite.c	\
	reedsol.c	\
)

#	dosformt.c	\
#	fatlite.c	\
#	flparse.c	\


# BEOS_SRCS
SRCS	+=	\
	custom/flcustom.c	\
	systems/beos/flsystem.c	\
	systems/beos/block_io.c	\
	systems/beos/driver.c	\
	systems/beos/device_icons.c
	

#	devicelist.c \
#	ide_device_info.c \
#	ide_settings.c
#	device_icons.c

MY_CFLAGS := -DDEBUGPREFIX="\"DOCH -- \""

# checkmalloc
#COMMON_SRCS += checkmalloc.c
#MY_CFLAGS += -DCHECKMALLOC

LINKS := dev/disk/doch/$(TARGET_NAME)

MY_INCLUDES := 	-I$(BUILDHOME)/src/inc/driver \
					-I$(BUILDHOME)/src/inc/support_p \
	   				-I$(BUILDHOME)/src/nukernel/arch/$(PLATFORM) \
	   				-I$(BUILDHOME)/src/nukernel/inc \
					-I$(BUILDHOME)/src/inc/os_p \
					-I$(BUILDHOME)/$(SRC_DIR)/src	\
					-I$(BUILDHOME)/$(SRC_DIR)/custom	\
					-I$(BUILDHOME)/$(SRC_DIR)/systems/beos

SYSTEM_LIBS :=


include $(SDIR_TEMPLATES)/DriverTemplate.mk
