#include <stdio.h>

int gcd(int a, int b)
{
	int result;
	
	asm volatile(
		"cont:"
			"cmp $0, %%ebx;"
			"je done;"
			"mov $0, %%edx;"
			"idiv %%ebx;"
			"mov %%ebx, %%eax;"
			"mov %%edx, %%ebx;"
			"jmp cont;"
		"done:"
		: "=a"(result)
		: "a"(a), "b"(b)
	);
	
	return result;
}

int main()
{
	int a = 12, b = 9;
	
	printf("gcd(%d, %d) = %d\n", a, b, gcd(a, b));
	
	return 0;
}