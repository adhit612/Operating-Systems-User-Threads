#ifndef TW_TYPES_H
#define TW_TYPES_H

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE SIGSTKSZ

typedef unsigned int worker_t;

enum thread_states {
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED_JOIN,
    THREAD_BLOCKED_MUTEX,
    THREAD_SCHEDULED,
    THREAD_DONE
}; 

typedef struct TCB
{
    /* add important states in a thread control block */
    int thread_id;
    int returnVal;
    int childValWeWaitingOn;
    enum thread_states thread_state;
    ucontext_t* thread_context;
    void* thread_stack;
    worker_t worker_T_TCB;
    struct TCB *threadWeWaitingOn;
    struct worker_mutext_t* mutexWeWaitingOn; 

    // thread Id
    // thread status
    // thread context
    // thread stack
    // thread priority
    // And more ...

    // YOUR CODE HERE

} tcb;

struct node {
    tcb *data;
    struct node *next;
};

#endif

