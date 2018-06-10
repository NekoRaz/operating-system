#include "ult.h"
#include "array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

#define STACK_SIZE 64*1024


/* thread control block */
typedef struct tcb_s
{
	ucontext_t caller, gen;
	void* yield;
	char mem[STACK_SIZE];
	/* data needed to restore the context */
} tcb_t;

void ult_init(ult_f f)
{


	return 0;
}

int ult_spawn(ult_f f)
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

void ult_yield()
{}

void ult_exit(int status)
{}

int ult_join(int tid, int* status)
{
	return -1;
}

ssize_t ult_read(int fd, void* buf, size_t size)
{
	return 0;
}
