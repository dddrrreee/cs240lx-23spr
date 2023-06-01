### Compiler
The goal of the compiler is to go from lexemes down to the IR accepted by the
checker you wrote for the prelab. The compiler will compile each function in
the source C file into a different IR program, separated by the string
"~~~\n".

For example, we might have:

    $ cat test_inputs/simple_if.c
    void foo(int *x) {
        *x = 5;
        if (x)
            *x = 6;
    }

    void bar(int *x) {
        *x = 5;
        while (x)
            x = 6;
    }
    $ make build/compiler
    $ ./build/compiler test_inputs/simple_if.c
    ~~~
    3 D 177693 2
    4 K 5861415 2
    9 B 177693 6 7 3
    6 K 0
    11 D 177693 4
    12 K 5861415 4
    13 B 0 8 8 0
    7 K 0
    8 K 0
    1 K 0
    ~~~
    3 D 177693 8
    4 K 5861415 8
    6 K 0
    9 B 177693 7 8 9
    7 K 0
    11 K 177693 10
    12 B 0 6 6 0
    8 K 0
    1 K 0

Note that ./build/compiler returned one IR block per function, while the
checker from the prelab only accepts one IR block at a time. The helper script
helper_scripts/check_file.py will automatically separate these blocks and call
the prelab checker once for each block. In other words, don't worry about it.

The compilation approach we'll use here is vaguely inspired by
https://legacy.cs.indiana.edu/~dyb/pubs/ddcg.pdf although _significantly_
jankier.

One big thing to keep in mind: we're not going to be able to handle all the
minutiae of the C language, and we're not even really going to try. We're just
going to try to handle the 90% common case as well as possible in a reasonable
code budget, then fiddle with it afterwards to reduce false positives.

### Compiler Starter Code Walkthrough
I'm giving a pretty significant chunk of starter code to get you going. You're
welcome to start from scratch if you prefer. But if not, read on ...

First, take a look at main.c. It just reads the file provided into a temporary
buffer, lexes that buffer (you wrote this in part 2a!), and then calls
"process_program".

Now, open up compiler.c and take a look at process_program. The goal of this
function is to identify functions in the C source code and compile each one
individually. To do that, we walk over lexemes until we see a "{". Then, we
locate the end of that block using the match_body method and call visit_stmt
on that range of lexemes. visit_stmt does the actual work of compiling the
function. Then we keep searching for more functions after that one until the
array of lexemes is emptied.

### Implementing the Compiler
The two functions you want to work in are:

    - visit_stmt(range, meta): takes a range in the file and compiles the
      relevant statements to stdout
    - visit_expr(range, meta): takes a range in the file and compiles the
      single expression contained in that range to stdout

The struct meta @meta argument is defined in compiler.h. It contains the
following metadata that is useful to the parser at any point in time:

    - fn_start: the first lexeme in this function
    - fn_end: one-past-the-end lexeme of this function
    - continue_to: what label should the IR jump to if the C code sees a
      "continue"?
    - break_to: what label should the IR jump to if the C code sees a "break"?
    - return_to: what label should the IR jump to if the C code sees a
      "return"?
    - true_branch: what label should the IR jump to if this expression is
      true?
    - false_branch: what label should the IR jump to if this expression is
      false?

You probably don't need to worry about fn_start or fn_end. The most confusing
meta fields are true_branch and false_branch; basically, if they are non-zero
while you're inside visit_expr, they indicate that the program is branching
based on the value of this expression. Make sure you propagate these properly,
e.g., through short-circuiting operators like "&&".

Note there are some parts of the code that are nasty/old stuff I don't quite
remember why I did. They are set aside in blocks that say "NASTY, IGNORE ME"
--- please do so, at least until you've filled in all the "unimplemented"s.

### Helper Functions
You should use the helper functions kill, deref, nop_labelled, branch,
branchrange, and goto_ to output the IR. These functions make sure to output
the line number as well in a suitable way.

There are some helper functions to help with the parsing:

    - find(lm, rm, str): looks for a lexeme with string @str in the range
      [lm,rm). Note this skips over the interior of balanced parentheses.
    - match_body(lm): looks for one-past-the-end of the body of the statement
      starting at @lm. E.g., match_body([<do>, <{>, ..., <}>, <while>, ...])
      will return the <}> right before the <while>.

### Example
Supposing the function body has lexemes:

    <if> <(> <y> <)> <*> <x> <;>

You'll probably see a callstack roughly like:

    visit_stmt([<if>, ..., <;>], {})
        body_label = 1, exit_label = 2
        visit_expr([<y>], {.true_branch=1, .false_branch=2})
            branchrange([<y>], 1, 2)
        nop_labelled(1)
        visit_stmt([<*>, <x>, <;>], {})
            visit_expr([<*>, <x>], {})
                deref([<x>])
                visit_expr([<x>], {})
        nop_labelled(2)

Notice how it's actually visit_expr that compiles in the branch.

Another example, for short-circuiting operations, if you have something like

    visit_expr([<x>, <&&>, <y>], {})
        first_true = 1, exit = 2
        visit_expr([<x>], {.true_branch = 1, .false_branch = 2})
        nop_labelled(1)
        visit_expr([<y>], {})
        nop_labelled(2)

A few notes about these examples:

    - They are not fully generalizable; to handle, say, else blocks you may
      need extra labels
    - The labels 1, 2 are just examples here; in practice you can get a unique
      label using the bump allocator, i.e., just use BUMP++
