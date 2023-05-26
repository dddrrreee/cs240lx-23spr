PROGS := ./tests/0-epsilon-test.c
PROGS := ./tests/3-circular-ok-test.c
PROGS := ./tests/4-trylock-ok-test.c
PROGS := ./tests/5-circular-q-ok.c
PROGS := ./tests/1-epsilon-should-fail-test.c
PROGS := ./tests/2-increment-fail-test.c
PROGS = $(wildcard ./tests/[1234567]*-test.c)

COMMON_SRC = check-interleave.c  check-interleave-asm.S 

# switch these for trylock
# COMMON_SRC += syscall.c
STAFF_OBJS += staff-syscall.o

STAFF_OBJS += staff-user-fn.o 
STAFF_OBJS += single-step.o 
STAFF_OBJS += staff-interleave-asm.o 

BOOTLOADER=my-install
RUN = 1
CAN_EMIT=1

EXCLUDE ?= grep -v simple_boot
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2

clean::
	make -C tests clean
