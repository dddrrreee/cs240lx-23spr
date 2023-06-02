#include "compiler.h"

struct lexeme RET_STATE;
char *PATH = "[none]";

uint64_t BUMP;

void kill(struct lexrange expr) {
    printf("%lu K %lu %lu\n", BUMP++, hash_range(expr), expr.lm->line_no);
}
void deref(struct lexrange expr) {
    printf("%lu D %lu %lu\n", BUMP++, hash_range(expr), expr.lm->line_no);
}
void nop_labelled(uint64_t label) {
    printf("%lu K 0\n", label);
}
void branch(uint64_t expr, uint64_t label_true, uint64_t label_false, uint64_t line) {
    // in case we break or continue in something that's not a loop ...
    if (!label_true || !label_false)
        return nop_labelled(BUMP++);
    printf("%lu B %lu %lu %lu %lu\n", BUMP++, expr, label_true, label_false, line);
}
void branchrange(struct lexrange expr, uint64_t label_true, uint64_t label_false) {
    branch(hash_range(expr), label_true, label_false, expr.lm->line_no);
}
void goto_(uint64_t label) {
    branch(0, label, label, 0);
}

void process_program() {
    size_t n_fns = 0;
    for (struct lexeme *l = LEXEMES; l != LEXEMES + N_LEXEMES;) {
        if (!LEXSTR(l, "{")) {
            l++;
        } else {
            struct meta meta = {0};
            meta.fn_start = l;
            meta.fn_end = match_body(l);
            if (!meta.fn_end) return;
            printf("~~~\n");
            BUMP = 1;
            meta.return_to = BUMP++;
            visit_stmt(torange(l + 1, meta.fn_end), meta);
            nop_labelled(meta.return_to);
            l = meta.fn_end;
        }
    }
}

/********* PARSING **********/
void visit_stmt(struct lexrange range, struct meta meta) {
    struct lexeme *lm = range.lm;
    if (range.rm > meta.fn_end) range.rm = meta.fn_end;
    // Hit the end of the program!
    if (!lm || lm >= range.rm) return;
    if (lm->out_id) return;
    lm->out_id = BUMP++;

    struct lexeme *end_paren = NULL, *body_end = NULL;
    if (LEXSTR(lm + 1, "(")) end_paren = find(lm + 1, NULL, ")");
    if (end_paren) body_end = match_body(end_paren + 1);

    struct meta submeta = meta;
    if (LEXSTR(lm, "for")) {
        // given: for (init; cond; upd) body
        if (!end_paren || !body_end) goto fail;
        struct lexeme *first_semi = find(lm + 2, NULL, ";");
        if (!first_semi)    goto fail;
        struct lexeme *second_semi = find(first_semi + 1, NULL, ";");
        if (!second_semi) goto fail;

        uint64_t cond = BUMP++, update = BUMP++, body = BUMP++, exit = BUMP++;

        visit_stmt(torange(lm + 2, first_semi + 1), meta);

        nop_labelled(cond);
        submeta = meta;
        submeta.true_branch = body;
        submeta.false_branch = exit;
        visit_expr(torange(first_semi + 1, second_semi), submeta);

        submeta = meta;
        submeta.continue_to = update;
        submeta.break_to = exit;
        nop_labelled(body);
        visit_stmt(torange(end_paren + 1, body_end), submeta);

        nop_labelled(update);
        visit_expr(torange(second_semi + 1, end_paren), meta);
        goto_(cond);
        nop_labelled(exit);
        return visit_stmt(torange(body_end, range.rm), meta);
    } else if (LEXSTR(lm, "while")) {
        if (!end_paren || !body_end) goto fail;

        // you'll probably need three labels: cond, body, and exit
        // the condition expression is [lm+2, end_paren)
        // the body is [end_paren+1, body_end)
        assert(!"unimplemented");

        return visit_stmt(torange(body_end, range.rm), meta);
    } else if (LEXSTR(lm, "if")) {
        if (!end_paren || !body_end) goto fail;

        // you'll probably need three labels: then, else_, and exit
        // the condition expression is [lm+2, end_paren)
        // the body is [end_paren+1, body_end)
        assert(!"unimplemented");

        if (LEXSTR(body_end, "else")) {
            struct lexeme *else_end = match_body(body_end + 1);

            // the else body is [body_end + 1, else_end)
            assert(!"unimplemented");

            // the end of the whole statement is the end of the body
            body_end = else_end;
        }

        // in case you need something here ...
        assert(!"unimplemented");

        return visit_stmt(torange(body_end, range.rm), meta);
    } else if (LEXSTR(lm, "do")) {
        if (!(body_end = match_body(lm + 1))) goto fail;

        // you'll probably need cond, body, and exit
        // the body is [lm + 1, body_end)
        assert(!"unimplemented");

        end_paren = find(body_end + 1, NULL, ")");
        // the condition is [body_end, end_paren)
        assert(!"unimplemented");

        return visit_stmt(torange(end_paren + 1, range.rm), meta);
    } else if (LEXSTR(lm, "switch")) {
        /****** NASTY, IGNORE ME ******/
        if (!end_paren || !body_end) goto fail;

        uint64_t exit = BUMP++, body = BUMP++;
        submeta = meta;
        submeta.true_branch = body;
        submeta.false_branch = body;
        visit_expr(torange(lm + 2, end_paren), submeta);

        nop_labelled(body);
        submeta = meta;
        submeta.break_to = exit;

        int is_case = 0;
        size_t first_case = BUMP;
        for (struct lexeme *l = end_paren + 1; l != body_end; l++) {
            is_case = is_case || LEXSTR(l, "case") || LEXSTR(l, "default");
            if (is_case && LEXSTR(l, ":")) {
                BUMP++;
                is_case = 0;
            }
        }
        size_t n_cases = BUMP - first_case;
        for (size_t i = 0; i < n_cases; i++)
                branch(0, first_case + i, BUMP + 1, range.lm->line_no);
        nop_labelled(BUMP++);

        int i = first_case;
        for (struct lexeme *l = end_paren + 1; l != body_end; l++) {
            is_case = is_case || LEXSTR(l, "case") || LEXSTR(l, "default");
            if (is_case && LEXSTR(l, ":")) {
                nop_labelled(first_case++);
                visit_stmt(torange(l + 1, body_end), submeta);
                is_case = 0;
            }
        }
        nop_labelled(exit);
        return visit_stmt(torange(body_end, range.rm), meta);
        /****** END NASTY ******/
    } else if (LEXSTR(lm, "case") || LEXSTR(lm, "default") || LEXSTR(lm + 1, ":")) {
        struct lexeme *colon = find(lm + 1, NULL, ":");
        if (!colon) goto fail;
        if (!(lm->label_id)) lm->label_id = BUMP++;
        nop_labelled(lm->label_id);
        return visit_stmt(torange(colon + 1, range.rm), meta);
    } else if (LEXSTR(lm, "continue")) {
        goto_(meta.continue_to);
        return visit_stmt(torange(lm + 2, range.rm), meta);
    } else if (LEXSTR(lm, "break")) {
        assert(!"unimplemented");
    } else if (LEXSTR(lm, "return")) {
        assert(!"unimplemented");
    } else if (LEXSTR(lm, "goto")) {
        for (struct lexeme *l = meta.fn_start; l != meta.fn_end; l++) {
            if (LEXSTR(l, (lm + 1)->string) && LEXSTR(l + 1, ":")) {
                if (!(l->label_id)) l->label_id = BUMP++;
                goto_(l->label_id);
                break;
            }
        }
        return visit_stmt(torange(lm + 3, range.rm), meta);
    } else if (LEXSTR(lm, "{") || LEXSTR(lm, "}")) {
        return visit_stmt(torange(lm + 1, range.rm), meta);
    }

    // match foo() { ... }
    if (end_paren && body_end) {
        visit_stmt(torange(end_paren + 1, body_end), meta);
        return visit_stmt(torange(body_end, range.rm), meta);
    }

    /****** NASTY, IGNORE ME ******/
    struct lexeme *semi = find(lm, NULL, ";");
    int saw_star = 0, anything_interesting = LEXSTR(lm, "*");
    for (struct lexeme *l = lm; l != semi && l != LEXEMES + N_LEXEMES; l++) {
        saw_star |= LEXSTR(l, "*");
        if (semi && !LEXSTR(lm, "*") && (LEXSTR(l, ",") || LEXSTR(l, "="))) {
            struct lexeme *og_l = l;
            while (1) {
                l = find(l, semi, "=");
                if (!l) break;
                struct lexeme *rm = find(l, semi, ",");
                if (!rm) rm = semi;
                visit_expr(torange(l + 1, rm), meta);
                l = rm;
            }
            if (LEXSTR(og_l, "=")) kill(torange(lm, og_l));
            goto finish;
        }
        // Definitely an expression...
        if (!(l->label == LEX_IDENT || LEXSTR(l, "*") || LEXSTR(l, "[") || LEXSTR(l, "]"))) {
            anything_interesting = 1;
            break;
        }
    }
    /****** END NASTY ******/
    if (!semi) goto fail;
    if (anything_interesting)
        // It's a line
        visit_expr(torange(lm, semi), meta);
finish:
    return visit_stmt(torange(semi + 1, range.rm), meta);
fail:
    return visit_stmt(torange(lm + 1, range.rm), meta);
}

void visit_expr(struct lexrange range, struct meta meta) {
    if (range.rm >= meta.fn_end) range.rm = meta.fn_end;
    if (range.lm >= range.rm) return;
    struct lexeme *lm = range.lm, *rm = range.rm;
    if (lm == rm || !lm || !rm) return;
    // Handle expression.
    struct meta og_meta = meta;
    meta.true_branch = meta.false_branch = 0;

    struct lexeme *found = NULL;
    if (LEXSTR(lm, "(") && (find(lm, rm, ")") + 1 == rm))
        return visit_expr(torange(lm + 1, rm - 1), og_meta);

    // Don't parse initializer lists ...
    if (LEXSTR(lm, "{") && (find(lm, rm, "}") + 1 == rm))
        return;

    if (found = find(lm, rm, ",")) {
        visit_expr(torange(lm, found), meta);
        return visit_expr(torange(found + 1, rm), og_meta);
    }

    char *assignops[] = {"=","+=","-=","&=","|=","<<=",">>=",NULL};
    for (char **op = &assignops[0]; *op; op++) {
        if (!(found = find(lm, rm, *op))) continue;
        assert(found + 1 != lm);
        if (found == lm || found + 1 == rm) goto fail;
        visit_expr(torange(lm, found), meta);
        visit_expr(torange(found + 1, rm), og_meta);
        return kill(torange(lm, found));
    }
    if (find(lm, rm, "?") || LEXSTR(lm, "sizeof") || LEXSTR(lm, "typeof"))
        goto opaque;

    // Handle BINOPS
    struct lexeme *strip = lm;
    for (; strip != rm && strip->label == LEX_OP; strip++);
    char *binops[] = {"||","&&","!=","==",">=","<=",">","<",">>",
                                        "<<","|","&","^","-","+","%","/","*",NULL};
    struct lexeme *op = NULL;
    for (char **b = &binops[0]; *b; b++)
            if (op = find(strip, rm, *b))
                    break;
    if (op) {
        int lhs_null = (lm + 1 == op) && (LEXSTR(lm, "null") || LEXSTR(lm, "0"));
        int rhs_null = (op + 2 == rm) && (LEXSTR(op + 1, "null") || LEXSTR(op + 1, "0"));
        assert(op + 1 <= rm);
        if (LEXSTR(op, "==") && lhs_null) {
#define NEGATE(l, r) { \
            meta.true_branch  = og_meta.false_branch; \
            meta.false_branch = og_meta.true_branch; \
            return visit_expr(torange(l, r), meta); }
            NEGATE(op + 1, rm);
        } else if (LEXSTR(op, "==") && rhs_null) {
            assert(!"unimplemented");
        } else if (LEXSTR(op, "!=") && lhs_null) {
            return visit_expr(torange(op + 1, rm), og_meta);
        } else if (LEXSTR(op, "!=") && rhs_null) {
            assert(!"unimplemented");
        } else if (LEXSTR(op, "&&")) {
            uint64_t first_true = BUMP++,
                     either_false = og_meta.false_branch ? og_meta.false_branch : BUMP++;
            meta.true_branch = first_true;
            meta.false_branch = either_false;
            visit_expr(torange(lm, op), meta);
            nop_labelled(first_true);
            visit_expr(torange(op + 1, rm), og_meta);
            if (!og_meta.false_branch)
                nop_labelled(either_false);
        } else if (LEXSTR(op, "||")) {
            assert(!"unimplemented");
        } else if (lm != op && op != rm) {
            visit_expr(torange(lm, op), meta);
            visit_expr(torange(op + 1, rm), meta);
        }
        goto opaque;
    }

    // Handle PREOPS
    if (strstr(lm->string, "ret")) {
        goto opaque;
    } else if (lm->label == LEX_OP) {
        if (LEXSTR(lm, "!"))
            NEGATE(lm + 1, rm);
        if (LEXSTR(lm, "*"))
            assert(!"unimplemented");
        if (LEXSTR(lm, "&"))
            assert(!"unimplemented");
        else
            visit_expr(torange(lm + 1, rm), meta);
        if (LEXSTR(lm, "++") || LEXSTR(lm, "--"))
            assert(!"unimplemented");
    } else if (LEXSTR(rm - 1, "++") || LEXSTR(rm - 1, "--")) {
        assert(lm <= (rm - 1));
        kill(torange(lm, rm - 1));
        visit_expr(torange(lm, rm - 1), meta);
    } else if (LEXSTR(rm - 1, "]")) {
        struct lexeme *open_bracket = find(rm - 1, lm, "[");
        if (!open_bracket) goto fail;
        deref(torange(lm, open_bracket));
        visit_expr(torange(lm, open_bracket), meta);
        visit_expr(torange(open_bracket + 1, rm - 1), meta);
    } else if (lm + 2 <= rm && LEXSTR(rm - 2, ".")) {
        visit_expr(torange(lm, rm - 2), meta);
    } else if (lm + 2 <= rm && LEXSTR(rm - 2, "->")) {
        assert(!"unimplemented");
    } else if (LEXSTR(rm - 1, ")")) {
        /****** NASTY, IGNORE ME ******/
        struct lexeme *arg_start = find(rm - 1, lm, "(");
        if (!arg_start) goto fail;
        if (lm+1 != arg_start) goto fail; // ??????
        arg_start++;
        struct lexeme *comma = find(arg_start, rm - 1, ",");
        while (comma) {
            visit_expr(torange(arg_start, comma), meta);
            arg_start = comma + 1;
            comma = find(arg_start, rm - 1, ",");
        }
        if (arg_start + 1 != rm - 1)
            visit_expr(torange(arg_start, rm - 1), meta);
        // If this is a panic, err, abort, ... then we need to *NOT* fallthrough.
        char *exits[]
            = {"panic","exit","err","abort","die","bug_on","oops",
                "ret","goto","out_of_mem","throw","usage","fatal","quit",
                "fail","stop",NULL};
        for (char **e = &exits[0]; *e; e++) {
            if (!strstr(lm->string, *e)) continue;
            // dirt: expressions can't actually kill paths, but they can reset the
            // state (which is effectively the same)
            goto opaque;
        }
        char *exact_exits[] = {"bug",NULL};
        for (char **e = &exact_exits[0]; *e; e++) {
            if (!LEXSTR(lm, *e)) continue;
            // dirt: expressions can't actually kill paths, but they can reset the
            // state (which is effectively the same)
            goto opaque;
        }
        /****** END NASTY ******/
    }
fail:
opaque:
    // If we are not branching based on this; just keep going
    if (!(og_meta.true_branch || og_meta.false_branch)) return;
    branchrange(range, og_meta.true_branch, og_meta.false_branch);
}
