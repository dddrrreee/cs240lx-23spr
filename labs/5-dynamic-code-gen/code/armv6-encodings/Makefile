# SUPPORT_OBJS := 

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# uncomment if you want it to automatically run.
RUN=1

PROGS = 0-test-bx.c
PROGS = 1-test-imm8.c

PROGS = 4-test-ldr-off12.c
PROGS = 2-test-imm8-rot4.c 
PROGS = 2-test-orr-imm8-rot4.c

PROGS = $(wildcard ./[01234]-*.c)

# CFLAGS = -I../hand-coded

LIBS += ./libgcc.a
LIB_POST := ./libgcc.a

include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2


