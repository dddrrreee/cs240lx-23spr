#include <stdio.h>


#if 0
int foo(void);
int main() { 
    foo();

	return 0;
}
#endif

int foo(int p[10]) {
	// int p[10];
	int i = 100000;

    // compile with -O2 and -O0: very interesting!!
    printf("what's happens?\n");

	if(/* (i < 10) & */ (p[i] == 10))
		printf("true\n");
	else
		printf("false\n");
}

