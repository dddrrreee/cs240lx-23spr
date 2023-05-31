#include "compiler.h"

struct lexeme *find(struct lexeme *l, struct lexeme *rm, char *str) {
  if (!rm) rm = LEXEMES + N_LEXEMES;
  int paren_depth = 0,
      delta = (l <= rm) ? 1 : -1;
  for (; l != rm; l += delta) {
    paren_depth += LEXSTR(l, "(") - LEXSTR(l, ")");
    paren_depth += LEXSTR(l, "[") - LEXSTR(l, "]");
    paren_depth += LEXSTR(l, "{") - LEXSTR(l, "}");
    if (!paren_depth && LEXSTR(l, str))
      return l;
  }
  return NULL;
}

// TODO: fails on while (...) if (...) ...; else ...;
struct lexeme *match_body(struct lexeme *l) {
  int paren_depth = 0;
  for (; l != LEXEMES + N_LEXEMES; l++) {
    if (!paren_depth && LEXSTR(l, "{")) {
      struct lexeme *match = find(l, NULL, "}");
      return match ? match + 1 : 0;
    }
    paren_depth += LEXSTR(l, "(") - LEXSTR(l, ")");
    paren_depth += LEXSTR(l, "[") - LEXSTR(l, "]");
    paren_depth += LEXSTR(l, "{") - LEXSTR(l, "}");
    if (!paren_depth && LEXSTR(l, ";"))
      return l + 1;
  }
  return NULL;
}

// http://www.cse.yorku.ca/~oz/hash.html
uint64_t hash_range(struct lexrange range) {
    unsigned long hash = 5381;
    for (struct lexeme *l = range.lm; l < range.rm; l++)
        for (char *c = l->string; *c; c++)
            hash = ((hash << 5) + hash) + (*c);
    return hash;
}
