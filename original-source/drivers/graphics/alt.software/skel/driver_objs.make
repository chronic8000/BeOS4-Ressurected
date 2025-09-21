###############################################################################
## BeOS Generic Driver Skeleton
##
##    This file contains a list of objects that need to be built for the
## generic kernel driver.
##
###############################################################################


###############################################################################
## Objects ####################################################################
###############################################################################

## Path of the generic accelerant source files.

GDS_KERN_PATH = ../alt.software/skel

## List of generic kernel driver objects.

GDS_KERN_OBJS =                                 \
  $(GDS_KERN_PATH)/driver_file_hooks.o          \
  $(GDS_KERN_PATH)/driver_gen_ioctl.o           \
  $(GDS_KERN_PATH)/driver_gen_vbi.o             \
  $(GDS_KERN_PATH)/driver_globals.o             \
  $(GDS_KERN_PATH)/driver_init.o                \
  $(GDS_KERN_PATH)/driver_kernel_hooks.o        \
  $(GDS_KERN_PATH)/driver_util.o


###############################################################################
## This Is The End Of The File ################################################
###############################################################################
