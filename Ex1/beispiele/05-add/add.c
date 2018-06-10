#include <stdio.h>

int main()
{
	int a = 3, b = 4, c;
	
	asm(
		"add %%ebx, %%eax"
	:	"=a"(c)
	:	"a"(a),
		"b"(b)
	);
	
	printf("%d + %d = %d\n", a, b, c);
	
	return 0;
}
