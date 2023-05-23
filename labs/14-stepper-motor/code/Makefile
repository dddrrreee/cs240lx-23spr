# list out the driver program source 
PROGS = 0-circle-test.c

# swap if you want.
# COMMON_SRC = i2s.c
# STAFF_OBJS = staff-i2s.o

# pick one of your attachment points (first or last) consistently.
# BOOTLOADER = my-install --last

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

LIB_POST := $(CS240LX_2023_PATH)/lib/libgcc.a
LIBS += $(LIB_POST)

DEPS = ./Makefile
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2
