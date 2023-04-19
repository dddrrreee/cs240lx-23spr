# SUPPORT_OBJS := 
PROGS = dis-examples-c.c

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

COMMON_SRC = dis-examples-asm.S
# START = ./getput-start.o

# uncomment if you want it to automatically run.
RUN=0
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2

all:: dis


dis: dis-examples-asm.o FORCE
	arm-none-eabi-objdump -D dis-examples-asm.o

FORCE: 
