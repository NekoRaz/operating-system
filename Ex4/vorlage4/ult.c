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

int threadCounter = -1;

/* thread control block */
typedef struct tcb_s
{
	ucontext_t caller, gen;
    tid;
	void* yield;
	char mem[STACK_SIZE];
	/* data needed to restore the context */
} tcb_t;

tcb_t* current_thread;

void ult_init(ult_f f)
{
    threadCounter = 0;

	return 0;
}

int ult_spawn(ult_f f)
{
    // increase thread counter
    threadCounter++;
    
    // initialize the context by cloning ours
    getcontext(&self->gen);
    
    // create the new stack
    self->gen.uc_link = 0;
    self->gen.uc_stack.ss_flags = 0;
    self->gen.uc_stack.ss_size = STACK_SIZE;
    self->gen.uc_stack.ss_sp = self->mem;
    self->tid = threadCounter;
    
    if (self->gen.uc_stack.ss_sp == NULL)
        return -1;
    
    // modify the context
    makecontext(&self->gen, func, 0);
    
    // use threadCounter as thread id
	return threadCounter;
}

void ult_yield()
{
	current_thread->yield = yield;
	swapcontext(&current_thread->gen, &current_thread->caller);
}

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
