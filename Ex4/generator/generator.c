
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 64*1024

typedef struct generator_s {
	ucontext_t caller, gen;
	void* yield;
	char mem[STACK_SIZE];
} generator_t;

generator_t* gen_curr;

int generatorInit(generator_t* self, void (*func)())
{
	// initialize the context by cloning ours
	getcontext(&self->gen);
	
	// create the new stack
	self->gen.uc_link = 0;
	self->gen.uc_stack.ss_flags = 0;
	self->gen.uc_stack.ss_size = STACK_SIZE;
	self->gen.uc_stack.ss_sp = self->mem;
	
	if (self->gen.uc_stack.ss_sp == NULL)
		return -1;
	
	// modify the context
	makecontext(&self->gen, func, 0);
	
	return 0;
}

void* Call(generator_t* self)
{
	gen_curr = self;
	swapcontext(&self->caller, &self->gen);
	return self->yield;
}

void Yield(void* yield)
{
	gen_curr->yield = yield;
	swapcontext(&gen_curr->gen, &gen_curr->caller);
}

void fib()
{
	unsigned int a = 1, b = 1;
	
	while (1)
	{
		Yield(&b);
		b = b + a;
		a = b - a;
	}
}

int main()
{
	generator_t fibGen;
	generatorInit(&fibGen, fib);
	
	for (int i = 0; i < 20; ++i)
	{
		unsigned int* res = Call(&fibGen);
		printf("%u ", *res);
	}
	putc('\n', stdout);
	
	return 0;
}
