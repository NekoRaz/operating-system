#include <stdio.h>

int main()
{
	char hello[] = "Hello World!\n";
	
	asm(
		"mov $1, %%rax;"
	    "mov $1, %%rdi;"
	    "syscall"
	:	/* no output registers */
	:	"S"(hello),
		"d"(sizeof(hello)-1)
	);
	
	return 0;
}
