# list out the driver program source 
PROGS = 4-measure.c  1-cache.c 0-simple.c # 1-cache.c  4-measure.c  


# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
COMMON_SRC := stop-inline.c
# STAFF_OBJS = staff-hc-sr04.o

# define this if you need to give the device for your pi
TTYUSB = 

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template

