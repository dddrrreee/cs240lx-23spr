#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

struct lexeme LEXEMES[MAX_LEXEMES] = {0};
size_t N_LEXEMES = 0;

void lex(char *s) {
    for (char *ptr = s; *ptr; ptr++)
        *ptr = tolower(*ptr);

    char *LAST_LINE = s;
    size_t LAST_LINE_NO = 1;
    for (; *s; s++) {
        enum lex_label label = LEX_NONE;
        char *start = s;
        // TODO: at the very least, also need to lex here: string literals,
        // character literals, hex literals, int literals, identifiers.
        // Note in these examples below, the first four are cases where we
        // throw away some characters (indicated by label = LEX_NONE). In the
        // final example we lex operation characters like [ and !=.
        assert(!"unimplemented");
        if (isspace(*s));
        else if (prefix(s, "/*")) {
            for (; *s && !prefix(s, "*/"); s++);
            if (*s) s++;
        } else if (prefix(s, "//")) {
            for (; *s && *s != '\n'; s++);
        } else if (s[0] == '#') {
            for (; *s && *s != '\n'; s++)
                s += (*s == '\\');
        } else {
            label = LEX_OP;
            if (strchr("()[]{}.:?,;", *s))
                s++;
            else if (prefix(s, "->"))
                s += 2;
            else if (strchr("-+&|", *s) && *s == s[1])
                s += 2;
            else if (prefix(s, "<<") || prefix(s, ">>"))
                s += 2 + (s[2] == '=');
            else
                s += 1 + (s[1] == '=');
        }
        // NOTE: This shared code builds the lexeme for you & inserts it into
        // the array of lexemes. Basically, you need to point @s to one-past
        // the last character you want to be part of the lexeme, and set @label
        // to whatever label you want it to have.
        if (label == LEX_NONE) continue;
        assert(N_LEXEMES < sizeof(LEXEMES) / sizeof(LEXEMES[0]));
        LEXEMES[N_LEXEMES].string = strndup(start, (size_t)(s - start));
        LEXEMES[N_LEXEMES].label = label;
        for (; LAST_LINE != start; LAST_LINE++)
             LAST_LINE_NO += (*LAST_LINE == '\n');
        LEXEMES[N_LEXEMES++].line_no = LAST_LINE_NO;
        s--;
    }
}
