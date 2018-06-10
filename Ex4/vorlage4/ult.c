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
    //void* yield;
    char mem[STACK_SIZE];
} tcb_t;

int threadCounter;

tcb_t* queue;
tcb_t* queueFinished;

tcb_t currentThread;
//tcb_t* queueFinished;

void ult_init(ult_f f)
{
    threadCounter = 1;

    //init all
    arrayInit(queue);
    arrayInit(queueFinished);
    threadCounter = 1;

    tcb_t thread;

    thread.gen.uc_link = 0;
    thread.gen.uc_stack.ss_flags = 0;
    thread.gen.uc_stack.ss_size = STACK_SIZE;
    thread.gen.uc_stack.ss_sp = currentThread.mem;
    thread.status = "ready";
    thread.tid = threadCounter;

    currentThread = thread;

    makecontext(&currentThread.gen, f, 0);
}

int ult_spawn(ult_f f)
{
    // increase thread counter
    threadCounter++;

    // initialize the context by cloning ours
    getcontext(&currentThread.gen);

    // create the new stack
    currentThread.gen.uc_link = 0;
    currentThread.gen.uc_stack.ss_flags = 0;
    currentThread.gen.uc_stack.ss_size = STACK_SIZE;
    currentThread.gen.uc_stack.ss_sp = currentThread.mem;
    currentThread.status = "ready";
    currentThread.tid = threadCounter;

    if (currentThread.gen.uc_stack.ss_sp == NULL)
        return -1;

    // modify the context
    makecontext(&currentThread.gen, f, 0);

    arrayPush(queue) = currentThread;
    
    // use threadCounter as thread id
    return threadCounter;
}

void ult_yield()
{
    if(!strncmp("done",currentThread. status , 4) == 0)
    {
        currentThread.status = "wait";
        arrayPush(queue) = currentThread;
        currentThread = arrayPop(queue);
    }
    swapcontext(&currentThread.gen, &currentThread.caller);
}

void ult_exit(int status)
{
    currentThread.status = "done";
    currentThread.exitCode = status;
    arrayPush(queueFinished) = currentThread;
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
