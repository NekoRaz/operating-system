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
    /* data needed to restore the context */
    int exitCode;
    ucontext_t caller, gen;
    int tid;
    char* status;
    void* yield;
    char mem[STACK_SIZE];
} tcb_t;

int threadCounter = -1;

tcb_t* queue;
tcb_t* queueFinished;

tcb_t* current_thread;
//tcb_t* queueFinished;

void ult_init(ult_f f)
{
    arrayInit(queue);
    arrayInit(queueFinished);
    threadCounter = 1;
    // create the new stack
    current_thread->gen.uc_link = 0;

    current_thread->gen.uc_stack.ss_flags = 0;
    current_thread->gen.uc_stack.ss_size = STACK_SIZE;
    current_thread->gen.uc_stack.ss_sp = current_thread->mem;
    current_thread->status = "ready";
    current_thread->tid = threadCounter;

}

int ult_spawn(ult_f f)
{
    // increase thread counter
    threadCounter++;

    // initialize the context by cloning ours
    getcontext(&current_thread->gen);

    // create the new stack
    current_thread->gen.uc_link = 0;
    current_thread->gen.uc_stack.ss_flags = 0;
    current_thread->gen.uc_stack.ss_size = STACK_SIZE;
    current_thread->gen.uc_stack.ss_sp = current_thread->mem;
    current_thread->status = "ready";
    current_thread->tid = threadCounter;

    if (current_thread->gen.uc_stack.ss_sp == NULL)
        return -1;

    // modify the context
    makecontext(&current_thread->gen, f, 0);

    arrayPush(queue) = current_thread;
    
    // use threadCounter as thread id
    return threadCounter;
}

void ult_yield()
{
    if(!strncmp("done",current_thread-> status , 4) == 0) current_thread->status = "wait";
    swapcontext(&current_thread->gen, &current_thread->caller);
}

void ult_exit(int status)
{
    current_thread->status = "done";
    current_thread->exitCode = status;
    arrayPush(queueFinished) = *current_thread;
    ult_yield();
}

int ult_join(int tid, int* status)
{
    tcb_t* tempArray;
    arrayInit(tempArray);
    int inArray = 0;
    int exitCode = 0;
    
    while (!arrayIsEmpty(queueFinished)){
        tcb_t tempFinishedThread = arrayPop(queueFinished);
        if (tid == tempFinishedThread.tid) {
            inArray = 1;
            exitCode = tempFinishedThread.exitCode;
        }
        arrayPush(tempArray) = tempFinishedThread;
    }
    
    while (arrayIsEmpty(tempArray)) {
        tcb_t tempFinishedThread = arrayPop(tempArray);
        arrayPush(queueFinished) = tempFinishedThread;
    }
    
    arrayRelease(tempArray);
    
    if (inArray == 1) {
        return exitCode;
    }else{
        ult_yield;
    }
    

    
	return -1;
}

ssize_t ult_read(int fd, void* buf, size_t size)
{
    return 0;
}
