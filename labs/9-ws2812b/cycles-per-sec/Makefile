# list out the driver program source 
PROGS = cycles-per-sec.c

# pick one of your attachment points (first or last) consistently.
BOOTLOADER = my-install

OPT_LEVEL = -O3

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

LIBS = $(CS240LX_2023_PATH)/lib/libgcc.a

DEPS = ./Makefile
SUPPORT_OBJS := $(SRC:.c=.o)
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2
