#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compiler.h"

int main(int argc, char **argv) {
    struct stat statbuf;
    assert(argc > 1);
    stat(argv[1], &statbuf);

    FILE *fd = fopen(argv[1], "r");
    assert(fd);

    char *data = calloc(statbuf.st_size + 1, sizeof(char));
    fread(data, sizeof(char), statbuf.st_size, fd);
    lex(data);

    size_t line_no = 1;
    for (struct lexeme *l = LEXEMES; l < (LEXEMES + N_LEXEMES); l++) {
        for (; l->line_no > line_no; line_no++)
            printf("\n");
        printf("%s ", l->string);
    }
    printf("\n");
    return 0;
}
