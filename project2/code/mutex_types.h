#ifndef MTX_TYPES_H
#define MTX_TYPES_H

#include "thread_worker_types.h"

/* mutex struct definition */
typedef struct worker_mutex_t
{
    /* add something here */
    // YOUR CODE HERE
    int state; //0 for not in use, 1 for in use
    struct TCB* currOwner;
    struct worker_mutex_t* mutexInQuestion;
} worker_mutex_t;

#endif
