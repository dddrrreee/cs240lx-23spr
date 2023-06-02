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
