### Ultimate Lab Goal
As you've learned in this class, it's hard to write C correctly.  And operating
systems are huge C projects.  In this lab we're going to implement a simple
static checker that will help us automatically search for bugs in huge C
projects. Our checker will find C code that looks like this:

    *x = 5;
    if (x)
        return;

This code indicates a bug, because on the first line you dereference $x$ (i.e.,
requiring that it is non-null) then on the second line you check if $x$ is null
or not (i.e., asserting a belief that it may be null). One of these beliefs
must be wrong.

We're going to vaguely be implementing a small part of this:
https://mlfbrown.com/paper.pdf
Combined with a little bit of this:
https://legacy.cs.indiana.edu/~dyb/pubs/ddcg.pdf

Our implementation will be in two parts: first, a checker that runs on a super
simplified IR (the prelab). Second, a compiler from C to that IR (the main
lab). I'll provide some helper scripts to hook them up together and
pretty-print the results.

### Part 1: Prelab
Do the prelab described in prelab/README

### Part 2a: Lexer
Do the lexer described in LEXER

### Part 2b: Compiler
Do the compiler described in COMPILER

### Part 3: Check the Linux Kernel
If you're pretty confident your compiler & checker work, you can check the
whole kernel like so:

    $ git clone --depth=1 https://github.com/torvalds/linux.git linux
    $ ./helper_scripts/check_repo.sh linux
    ... bugs come out here ...

### Part 4: Checkoff
You should have:

    - [ ] Prelab that matches my output on the prelab test_inputs
    - [ ] ./build/lexer_tests passes
    - [ ] ./build/lex_file test_inputs/mlme.c either matches the staff output
          or is a better/also reasonable lexing
    - [ ] Your checker has good SNR so it's easy to find 5--10 bugs in Linux
    - [ ] Write a C file that causes your checker to have a false positive
    - [ ] Write a C file that causes your checker to have a false negative

### Part 5: Extensions
You could ...

    - Fiddle with the compiler to improve the SNR
    - Implement different checks
        - E.g., an easy one to adapt this checker to: look for cases where you
          free(x) on every path before reaching a dereference *x.
    - Our compiler is super janky; I think you can probably simplify the code
      and improve robustness of the parser by writing some kind of
      PEG-with-actions library/DSL
    - Write a compiler from another language (unsafe rust? c++? go?)
    - Run it on different codebases (freebsd tends to have a bunch of these
      bugs)
    - Compare SNR to just asking ChatGPT
    - Try reducing false positives by feeding each bug report to ChatGPT and
      asking it if it's a true bug or not
    - Pick one or two bug reports that look like true positives & track down
      Why the existing kernel checkers didn't catch it
