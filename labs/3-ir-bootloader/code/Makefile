# list out the driver program source 
PROGS := 1-keypress.c
PROGS := 0-dump-driver.c 
PROGS := 2-thread-driver.c
PROGS := 3-sw-uart.c
PROGS := 1-hello.c
PROGS := 2-ping-pong-byte.c

COMMON_SRC = ir-put-get.c 

STAFF_OBJS += rpi-thread.o
STAFF_OBJS += rpi-thread-asm.o
STAFF_OBJS += staff-ir-put-get.o

CFLAGS += -I../small-prog

LIB_POST = ../../1-mailboxes/code.staff/libgcc.a
# set RUN = 1 if you want the code to automatically run after building.
RUN = 1
OPT_LEVEL = -O2

DEPS = ./Makefile
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2

libs::
	make -C ../small-prog
