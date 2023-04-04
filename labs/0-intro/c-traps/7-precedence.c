#include <stdio.h>

int main() { 
	unsigned a = (1<<3);

	if(a & (1<<3) != 0)
		printf("overlaps with 8! a=%d\n", a);

#if 1
    // what happens here?
	unsigned lo = 3;
	if((1U <<  (31U + lo)) != 0U)
		printf("hi! = %d\n", 1U <<  (31U + lo));
#endif
	return 0;
}
