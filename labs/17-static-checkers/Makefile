# NOTE: We need at least -O2 to get tail call recursion without stack
# overflows.
CFLAGS = -g -lm -D_GNU_SOURCE
CFLAGS += -O3
CFLAGS += -I.
# CFLAGS += -fsanitize=address

all: build/compiler build/lexer_tests  build/lex_file

build/compiler: compiler.c lexer.c main.c utils.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

build/lexer_tests: lexer.c helper_scripts/lexer_tests.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

build/lex_file: lexer.c helper_scripts/lex_file.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build

.PHONY: all clean
