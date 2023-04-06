#include <stdio.h>

int main() { 
    int a = 1, b = 2, c = 3;

    // what does this print?  why?
    if(a < b < c)
        printf("yea!  a<b<c\n");
    else
        printf("WAT!?!\n");

#if 1
    // what happened??
    b = 1000000;
    if(a < b < c)
        printf("bad!!!!\n");
#endif

	return 0;
}
