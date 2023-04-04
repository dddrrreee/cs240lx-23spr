# how do you make it compile each one of them programmatically?

CC = gcc
CFLAGS = -O -MMD
# CFLAGS += -Wall

#SRC += 0-main.c 
SRC = 0-side-effects.c
SRC = 1-largest.c
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


PROGS := $(SRC:.c=)
deps := $(SRC:.c=.d)

all: $(PROGS)

-include $(deps)

./%: ./%.c ./Makefile 
	$(CC) $(CFLAGS) $< -o $@
	./$@

# always force a rebuild.
FORCE:

clean:
	rm -f *~ *.o *.d $(PROGS)
