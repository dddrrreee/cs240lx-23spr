////// NOTE: you don't need to modify this file //////
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

int main(int argc, char **argv) {
    PATH = argv[1];

    // Read the entire file
    struct stat statbuf;
    stat(PATH, &statbuf);

    FILE *fd = fopen(PATH, "r");
    char *data = calloc(statbuf.st_size + 1, sizeof(char));
    fread(data, sizeof(char), statbuf.st_size, fd);

    // Do the lexing
    lex(data);

    // Then process the lexed program
    process_program();
}
