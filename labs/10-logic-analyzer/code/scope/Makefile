# list out the driver program source 
PROGS = scope.c

# pick one of your attachment points (first or last) consistently.
BOOTLOADER = my-install --first

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
# SRC := stop-inline.c
# STAFF_OBJS = staff-hc-sr04.o

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
SUPPORT_OBJS := $(SRC:.c=.o)
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template

