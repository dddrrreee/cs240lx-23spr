#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

enum opcode {
    OPCODE_BRANCH = 'B',
    OPCODE_KILL = 'K',
    OPCODE_DEREF = 'D',
};

struct exprmap {
    uint64_t *exprs, *deref_labels;
    size_t n_exprs;
};

// each of these should be ~5-10loc for a basic implementation. if you want to
// go crazy, try keeping it sorted & using binary search (only once you have
// everything working initially!).
uint64_t lookup_exprmap(const struct exprmap set, uint64_t expr) {
    // you can return 0 if you don't find anything
    assert(!"unimplemented");
}

struct exprmap insert_exprmap(const struct exprmap old, uint64_t expr, uint64_t deref_label) {
    assert(!"unimplemented");
}

struct exprmap remove_exprmap(const struct exprmap old, uint64_t expr) {
    assert(!"unimplemented");
}

int subset_exprmap(const struct exprmap small, const struct exprmap big) {
    assert(!"unimplemented");
}

struct exprmap intersect_exprmaps(const struct exprmap small, const struct exprmap big) {
    assert(!"unimplemented");
}

struct instr {
    uint64_t label;
    enum opcode opcode;
    uint64_t args[3];
    struct instr *nexts[2];

    int visited;
    struct exprmap always_derefed;
};

void visit(struct instr *instr, struct exprmap derefed) {
    // Update instr->visited, instr->always_derefed, derefed, or return as
    // needed for each of the following cases:
    // (1) instr is NULL
    // (2) this is the first path to reach instr
    // (3) along every prior path to @instr every expression in @derefed has
    //     been derefed
    // (4) there are some expressions in instr->always_derefed that are not in
    //     @derefed along this path
    // should be about 10loc total; use the data structure operations you
    // implemented above!
    assert(!"unimplemented");

    // now actually process the instruction:
    // (1) if it's a kill, then we no longer know anything about instr->args[0]
    // (2) if it's a deref, then we should remember that instr->args[0] has
    //     been derefed for the remainder of this path (at least, until it's
    //     killed later on)
    assert(!"unimplemented");

    // now recurse on the possible next-instructions. we visit nexts[1] first
    // out of superstition (it's more likely to be NULL and we want to do the
    // most work in the tail recursive call)
    visit(instr->nexts[1], derefed);
    visit(instr->nexts[0], derefed);
}

void check(struct instr *instr) {
    if (!instr || instr->opcode != OPCODE_BRANCH) return;
    if (!lookup_exprmap(instr->always_derefed, instr->args[0])) return;
    printf("%lu:%lu\n", lookup_exprmap(instr->always_derefed, instr->args[0]),
           instr->label);
}

int main() {
    // you don't need to modify any of this; it just parses the input IR, then
    // calls visit on the first instruction, then calls check on every
    // instruction.
    struct instr *head = NULL;
    size_t n_instructions = 0, max_label = 0;
    while (!feof(stdin)) {
        struct instr *new = calloc(1, sizeof(*new));
        if (scanf("%lu %c", &(new->label), &(new->opcode)) != 2) {
            assert(feof(stdin));
            free(new); // shut the asan up
            break;
        }
        int n_args = (new->opcode == OPCODE_BRANCH) ? 3 : 1;
        for (size_t i = 0; i < n_args; i++)
            scanf(" %lu", &(new->args[i]));
        new->nexts[0] = head;
        head = new;
        n_instructions++;
        if (new->label > max_label) max_label = new->label;
        while (!feof(stdin) && fgetc(stdin) != '\n') continue;
    }

    size_t n_labels = max_label + 1;
    struct instr **label2instr = calloc(n_labels, sizeof(*label2instr));

    size_t i = 0;
    for (struct instr *instr = head; instr; instr = instr->nexts[0]) {
        assert(!label2instr[instr->label]);
        label2instr[instr->label] = instr;
    }

    struct instr *new_head = NULL;
    for (struct instr *instr = head; instr; ) {
        struct instr *real_next = instr->nexts[0];
        instr->nexts[0] = new_head;
        new_head = instr;
        instr = real_next;
    }
    head = new_head;

    for (size_t i = 0; i < n_labels; i++) {
        struct instr *instr = label2instr[i];
        if (instr && instr->opcode == OPCODE_BRANCH) {
            if (!(label2instr[instr->args[1]]) || !(label2instr[instr->args[2]])) {
                exit(1);
            }
            instr->nexts[0] = label2instr[instr->args[1]];
            instr->nexts[1] = label2instr[instr->args[2]];
        }
    }

    visit(head, (struct exprmap){NULL, NULL, 0});
    for (size_t i = 0; i < n_labels; i++)
        check(label2instr[i]);
}
