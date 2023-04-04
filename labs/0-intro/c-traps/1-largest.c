#include <stdio.h>

int main() { 
    int a = 1, b = 2;

    printf("-----------------------------------------\n");
    printf("part 1: what is a,b?\n");
	a---b; 
	printf("a=%d, b=%d\n", a, b);


    // what is x?
	// printf("x=%d\n",x);












    // printf("-----------------------------------------\n");
    // part 2: what about:
	// a----b; 
	// a-----b; 

    // part3: what about:
	// a-- - --b; 
	// a-- - -b; 



#if 0
    printf("-----------------------------------------\n");
    printf("part4: value of a,b?\n");

	a = 1; b = 1;
	a-- - --b;
    printf("expr=%d\n", a-- - --b);
	// printf("a=%d, b=%d\n", a, b);

	b-1;
	b+1;
	b-1;
	b+1;
	b-1;
	b+1;
	b-1;
    a+1;
    a+1;
    a+1;
    a-1;
    a-1;
    a-1;
    a-1;
	printf("a=%d, b=%d\n", a, b);
#endif

#if 0
    printf("-----------------------------------------\n");
    printf("part 5: does this compile?  a=?\n");
	a = 1;

	a =- 2;
	printf("a = %d\n", a);

	a =   - 2;
	printf("a = %d\n", a);
#endif

#if 0
    printf("-----------------------------------------\n");
    printf("part6: what is b?\n");
	a = 2;  b = 20;
	int *p = &a;
	b = a /*p          /* p points at the divisor */;
	printf("b = %d\n", b);
#endif

	return 0;
}
