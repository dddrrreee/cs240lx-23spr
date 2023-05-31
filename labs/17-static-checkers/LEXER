### Lexer
##### What is a lexer?
The first step in most compilers is to lex the input file. Lexing basically
means "chunking up" the input file into logical tokens, rather than individual
characters. For example, the line:

    if (bar ==foo) continue;

consists of ~30 characters, but logically the compiler should see it as 8
chunks (tokens, lexemes) like this:

    <if> <(> <bar> <==> <foo> <)> <continue> <;>

Basically, the lexer normalizes the input file, ignoring spaces and collecting
characters that are logically one into a single lexeme. For our compiler, we'll
also store a few other things on our lexemes:

    - string: the null-terminated string contents of this lexeme
    - label: the broad class of lexeme; either LEX_STR_LIT, LEX_NUM_LIT,
      LEX_OP, or LEX_IDENT.
    - line_no: the line number in the original file that this lexeme was on
      (starts at 1)
    - out_id: set to 0 for now
    - label_id: set to 0 for now

##### Implementing the lexer
I've given most of a lexer in lexer.c. Your job is to add rules that lex the
following types of tokens:

    - [ ] string literals
    - [ ] character literals
    - [ ] hex literals
    - [ ] int literals
    - [ ] identifiers (including 'foo', 'if', 'for', ...)

Note that the code to insert into the lexeme array is given to you; you just
need to set the @s pointer and @label value properly.

##### Testing the lexer
As an initial test, you can:

    $ make build/lexer_tests
    $ ./build/lexer_tests

which should print a bunch of "Passed!" messages

To go a bit further, I've included a helper script that lets you run your lexer
against an arbitrary file and see what it lexes to. This lets you compare your
lexer to mine.

    $ make build/lex_file
    $ ./build/lex_file test_inputs/mlme.c > mlme.lexed.c
    $ diff mlme.lexed.c test_inputs/mlme.lexed.staff.c

If you match my lexer output on this test case you're probably good to go
(*assuming* you've picked proper labels ...). You might even do a better job
lexing than me :)
