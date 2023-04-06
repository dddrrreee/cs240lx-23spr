#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int foo(void) { return 0; }

int hi(void) { printf("hi "); return 1; }
int world(void) { printf("world\n"); return 1; }

int main() { 
	int i;

#if 0
    printf("part 1: what happened?\n");
	for(i=0; i < 10; i) 
		printf("i=%d\n", i);

#endif

#if 1
    printf("part 2: what happened?\n");
    if(foo)
        printf("foo != 0\n");
    else
        printf("foo == 0\n");
#endif

#if 1
	i = 0;
	i = !!!(i==4);

	printf("i=%d\n", i);
	0;

	0,
	i,
	main,
    hi(),
    world(),
	i==4;
	(void)0;

#endif
#if 0
	if(setuid != 0)
		printf("you are root!\n");
#endif

	return 0;
}
