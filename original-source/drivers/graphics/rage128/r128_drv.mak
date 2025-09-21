###############################################################################
## BeOS Graphics Kernel Driver - Dummy Driver
##
## FILE : r128_drv.mak
## DESC :
##    Original version written by Todd Showalter.
##    Modified for use with the generic driver skeleton by Christopher Thomas.
##
###############################################################################


###############################################################################
## Make Settings ##############################################################
###############################################################################

.SUFFIXES: .o .c .cpp

###############################################################################
## Project Targets ############################################################
###############################################################################

NAME       = rage128
BINDIR     = bin
DRIVER     = $(BINDIR)/$(NAME)
INSTALLDIR = /boot/beos/system/add-ons/kernel/drivers/bin/$(NAME)


###############################################################################
## Includes ###################################################################
###############################################################################
 
## Include generic driver skeleton targets.

include ../skeleton_driver/driver_objs.make


###############################################################################
## Tools ######################################################################
###############################################################################

###############################################################################
## C Compiler
##    I've turned on maximum warnings, with the requirement for prototypes for
## all functions.  I find this saves me a lot of time when dealing with other
## people's code, and it doesn't hurt with mine, either.  I've turned on the
## -pedantic argument because it catches a few things that -Wall misses, and
## because when code builds cleanly under -Wall -pedantic it is more likely
## to be clean.
##    I like my code to compile at the highest possible warning level with no
## warnings.  I figure if you don't know the C incantations to make the
## warning go away, you don't know what you're doing well enough to leave the
## code generating said warning in place.

CCNAME = gcc
CC     = @$(CCNAME)
DUMMY_INC = ../r128_include
## Add -DDEBUG for a debug build.
CFLAGS = -Wall -Wmissing-prototypes -pedantic -O3 -I../skeleton_include \
	-I../skeleton_driver -I$(DUMMY_INC) 
DEBUGFLAG = -DDEBUG -DDEBUG_KDRIVER




LDNAME  = gcc
LD      = @$(LDNAME)
LDFLAGS = -nostdlib /boot/develop/lib/x86/_KERNEL_
##
## 4.1R3 link support for _kdprintf
##LDDEBUG = /boot/develop/lib/x86/libroot.so
LDDEBUG = 

###############################################################################
## Suffix Rules ###############################################################
###############################################################################

.c.o:
	@echo "     Compiler --" $<
	$(CC) $(CFLAGS) $(DEBUGFLAG) -c $< -o $@

.cpp.o:
	@echo "     Compiler --" $<
	$(CC) $(CFLAGS) $(DEBUGFLAG)  $< -o $@


###############################################################################
## Objects ####################################################################
###############################################################################

DRVOBJS =  r128_init.o \
        r128_ioctl.o \
        r128_vbi.o \
        r128_supported_devices.o \
        $(GDS_KERN_OBJS)


###############################################################################
## Primary Targets ############################################################
###############################################################################

default: header $(DRVOBJS) $(DRIVER) footer

clean:
	@-rm -rf $(BINDIR)
	@-rm -f $(DRIVER)
	@-rm -f $(DRVOBJS)
	@-rm -f *~

install: installheader
	@cp $(DRIVER) $(INSTALLDIR)

HOST = 192.168.64.0
PUNG:
	ping -c4 $(HOST)

ftp: $(DRIVER) 
	cd ../r128_accelerant; make -fr128_accel.mak ; cd ../r128_driver
	ftp </boot/beos/bin/ftp_load

###############################################################################
## Secondary Targets ##########################################################
###############################################################################

$(DRIVER): $(BINDIR) $(DRVOBJS)
	@echo "       Linker --" $(DRIVER)
	$(LD) -o $(DRIVER) $(DRVOBJS) $(LDFLAGS) $(LDDEBUG)
	@mimeset -f $(DRIVER)
	chmod 755 $(DRIVER)

$(BINDIR):
	@-mkdir $(BINDIR)

header:
	@echo
	@echo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	@echo %" "Building $(DRIVER) with $(MAKE).
	@echo %" "CC: $(CCNAME) $(CFLAGS)
	@echo %" "LD: $(LDNAME) $(LDFLAGS)
	@echo %~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

footer:
	@echo %~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo %" "Done.
	@echo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	@echo

installheader:
	@echo
	@echo Installing: " " $(DRIVER)
	@echo To: "         " $(INSTALLDIR)
	@echo


###############################################################################
## This Is The End Of The File ################################################
###############################################################################
