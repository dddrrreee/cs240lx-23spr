# how do you make it compile each one of them programmatically?

CC = gcc
CFLAGS = -O0 -MMD
#CFLAGS += -Wall

#SRC += 0-main.c 
SRC = 0-side-effects.c
#SRC = 1-largest.c
#SRC = 2-sideeffects.c
#SRC = 3-comma.c
#SRC = 4-and.c
#SRC = 5-or.c
#SRC = 6-or.c
#SRC = 7-precedence.c
#SRC = 8-eval.c
#SRC = 9-eval.c
#SRC = 10-control.c
#SRC = 11-cpp.c
#SRC = 12-assertnow.c

#SRC := $(wildcard ./[0123456789]*.c)

PROGS := $(SRC:.c=)
deps := $(SRC:.c=.d)

all: $(PROGS)

-include $(deps)

print:
	# SRC=$(SRC)

./%: ./%.c ./Makefile 
	$(CC) $(CFLAGS) -c $< -o $@.o
	$(CC) $(CFLAGS) $@.o -o $@
	./$@

# always force a rebuild.
FORCE:

clean:
	rm -f *~ *.o *.d $(PROGS)

arm-dump:
	arm-none-eabi-gcc -O2 -c 4-and.c
	arm-none-eabi-objdump -d 4-and.o

.PHONY: arm-dump
