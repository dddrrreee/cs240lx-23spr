#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compiler.h"

void (test_case)(char *string, struct lexeme *lexs, int n_lexs) {
    printf("Testing '%s'...\n", string);

    N_LEXEMES = 0;
    lex(string);

    assert(N_LEXEMES == n_lexs);
    for (size_t i = 0; i < N_LEXEMES; i++) {
        assert(!strcmp(LEXEMES[i].string, lexs[i].string));
        assert(LEXEMES[i].label     == lexs[i].label);
        assert(LEXEMES[i].line_no   == lexs[i].line_no);
    }

    printf("\tPassed!\n");
}

#define test_case(str, ...) \
    (test_case)( \
            strdup(str), \
            (struct lexeme[]){__VA_ARGS__}, \
            sizeof((struct lexeme[]){__VA_ARGS__})/sizeof(struct lexeme))


int main() {
    test_case(
        "int x = 5;",
        (struct lexeme){"int",  LEX_IDENT,  1,},
        (struct lexeme){"x",    LEX_IDENT,  1,},
        (struct lexeme){"=",    LEX_OP,     1,},
        (struct lexeme){"5",    LEX_NUM_LIT,1,},
        (struct lexeme){";",    LEX_OP,     1,},
    );

    test_case(
        "int x = /* foo=bar */ 5;",
        (struct lexeme){"int",  LEX_IDENT,  1,},
        (struct lexeme){"x",    LEX_IDENT,  1,},
        (struct lexeme){"=",    LEX_OP,     1,},
        (struct lexeme){"5",    LEX_NUM_LIT,1,},
        (struct lexeme){";",    LEX_OP,     1,},
    );

    test_case(
        "int x=5;",
        (struct lexeme){"int",  LEX_IDENT,  1,},
        (struct lexeme){"x",    LEX_IDENT,  1,},
        (struct lexeme){"=",    LEX_OP,     1,},
        (struct lexeme){"5",    LEX_NUM_LIT,1,},
        (struct lexeme){";",    LEX_OP,     1,},
    );

    test_case(
        "int x=5    ;",
        (struct lexeme){"int",  LEX_IDENT,  1,},
        (struct lexeme){"x",    LEX_IDENT,  1,},
        (struct lexeme){"=",    LEX_OP,     1,},
        (struct lexeme){"5",    LEX_NUM_LIT,1,},
        (struct lexeme){";",    LEX_OP,     1,},
    );

    test_case(
        "x->foo",
        (struct lexeme){"x",  LEX_IDENT,  1,},
        (struct lexeme){"->", LEX_OP,     1,},
        (struct lexeme){"foo",LEX_IDENT,  1,},
    );

    test_case(
        "x<<foo",
        (struct lexeme){"x",  LEX_IDENT,  1,},
        (struct lexeme){"<<", LEX_OP,     1,},
        (struct lexeme){"foo",LEX_IDENT,  1,},
    );

    test_case(
        "x|=foo",
        (struct lexeme){"x",  LEX_IDENT,  1,},
        (struct lexeme){"|=", LEX_OP,     1,},
        (struct lexeme){"foo",LEX_IDENT,  1,},
    );

    test_case(
        "x|=foo// hello\nthere",
        (struct lexeme){"x",    LEX_IDENT,  1,},
        (struct lexeme){"|=",   LEX_OP,     1,},
        (struct lexeme){"foo",  LEX_IDENT,  1,},
        (struct lexeme){"there",LEX_IDENT,  2,},
    );

    return 0;
}
