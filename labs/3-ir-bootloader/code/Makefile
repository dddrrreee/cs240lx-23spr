# list out the driver program source 
PROGS := 2-ping-pong-byte.c
PROGS := 1-hello.c

COMMON_SRC = ir-put-get.c 

STAFF_OBJS += rpi-thread.o
STAFF_OBJS += rpi-thread-asm.o
STAFF_OBJS += staff-ir-put-get.o

CFLAGS += -I../small-prog

LIB_POST = ./libgcc.a
# set RUN = 1 if you want the code to automatically run after building.
RUN = 1
OPT_LEVEL = -O2

DEPS = ./Makefile
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2

#libs::
#	make -C ../small-prog
