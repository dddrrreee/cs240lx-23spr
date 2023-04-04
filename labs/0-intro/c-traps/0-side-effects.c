#include <stdio.h>

int main() {
    int a = 1, b = 2;

    printf("-----------------------------------------\n");
    printf("do we get compile error from below?\n");
    a--;
    --b;

    -a;

    1;
    0;
    10;
    20+20;

    a==b;
    b==b;
    b==a;

    printf("a=%d, b=%d\n", a, b);
    return 0;
}
