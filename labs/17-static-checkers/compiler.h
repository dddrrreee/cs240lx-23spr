#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define append_field(o, f) *({ \
  o.f = realloc(o.f, (++o.n_##f) * sizeof(o.f[0])); \
  o.f + (o.n_##f - 1); \
})

// Guaranteed to return 0 or 1.
#define LEXSTR(x, s) (x && x < LEXEMES + N_LEXEMES && !strcmp((x)->string, (s)))

#define prefix(hay, needle) \
  !strncmp(hay, needle, strlen(needle))

enum lex_label {
  LEX_NONE, LEX_STR_LIT, LEX_NUM_LIT,
  LEX_OP, LEX_IDENT
};

struct lexrange {
    struct lexeme *lm, *rm;
};
#define torange(l, r) (struct lexrange){l,r}

struct lexeme {
  char *string;
  enum lex_label label;
  size_t line_no;
  uint64_t out_id, label_id;
};

#define MAX_LEXEMES (1 << 20)
extern struct lexeme LEXEMES[MAX_LEXEMES];
extern size_t N_LEXEMES;
extern char *PATH;

/////// lexer.c
void lex(char *s);

/////// utils.c
uint64_t hash_range(struct lexrange range);
// search in this range for a lexeme with string @str
// NOTE: does *not* match inside of balanced parens, e.g., (a) should not match
// a, but (a)a should (the second a).
struct lexeme *find(struct lexeme *l, struct lexeme *rm, char *str);
// returns the end of the body for the statement starting at @l.
// e.g., while (x) { foo(); bar(); } ...
// will return the first lexeme in the ...s.
struct lexeme *match_body(struct lexeme *l);

///// nanochex.c
struct meta {
    struct lexeme *fn_start, *fn_end;
    uint64_t continue_to, break_to, true_branch, false_branch, return_to;
};
// statically execute the statement starting at lexeme @lm.
void visit_stmt(struct lexrange range, struct meta meta);
// statically evaluate the expression in range [@lm, @rm)
void visit_expr(struct lexrange range, struct meta meta);
// process an entire program (multiple functions)
void process_program();
