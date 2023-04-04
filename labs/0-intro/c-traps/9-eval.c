#include <stdio.h>
 
void foo(void) { printf("foo\n"); }
double  bar(void) { printf("bar\n"); }

int main( void ) {
	int a = 12;

    
    printf("what does this print?\n");
  	int b = sizeof( ++a );
	printf("a=%d,b=%d\n", a,b);

#if 0
    printf("what does this print?\n");
	int c = sizeof foo(); 
	printf("a=%d,c=%d\n", a,c);
#endif

#if 0
	int d = sizeof bar(); 
  	printf( "a=%d, b=%d c=%d, d=%d\n", a , b, c, d);
#endif
  	return 0;
}
