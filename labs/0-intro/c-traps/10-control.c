#include <stdio.h>

int main() { 
    int a = 1, b = 2, c = 3;
    if(a > b);
        a = b;
    printf("a =%d (?!?)\n", a);

#if 0
	a = 1, b = 2, c = 3;
	if(a > b) 
		if(b > c)
			printf("a > b > c\n");
	else
		printf("a < b\n");
#endif


#if 0
	if(a > b)
		return
	a = b;
	printf("a = %d\n", a);
	return 0;
#endif
}
