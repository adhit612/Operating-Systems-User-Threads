// File:	thread-worker.c

// List all group member's name:
/* Sanjay Kethineni -> sk2425
Adhit Thakur -> at1186*/ 
// username of iLab: at1186
// iLab Server: pascal.cs.rutgers.edu

#include "thread-worker.h"
#include "thread_worker_types.h"
#include <sys/time.h>
#include <signal.h>
#include <string.h> //included because of memset warning

//16*1024
#define STACK_SIZE SIGSTKSZ
#define QUANTUM 10 * 1000

// INITIALIZE ALL YOUR OTHER VARIABLES HERE
int init_scheduler_done = 0;

struct node *runQueueHead = NULL;
struct node *blockedQueueHead = NULL;
struct node *mutexQueueHead = NULL;

ucontext_t schedrr_ctx;

int timerHasBeenCreated = 0;

int schedulerHasBeenCreated = 0;

struct itimerval timer;

int globalId = 1;
int saveMainContext = 0;
int gSchedulerInSwapContext =0;

sigset_t signal_set;

//declare functions here
//worker_create
int worker_create(worker_t *, pthread_attr_t *,
                  void *(void*), void *);
int worker_yield();
void worker_exit(void *);
int worker_join(worker_t, void **);
int worker_mutex_init(worker_mutex_t *,
                      const pthread_mutexattr_t *);
int worker_mutex_lock(worker_mutex_t *);
int worker_mutex_unlock(worker_mutex_t *);
int worker_mutex_destroy(worker_mutex_t *);
static void schedule(int sig_nr, siginfo_t* info, void *context);
static void sched_rr();
void init_timer();
int join_check(struct node*);
void blocked_list_check();
void addToRunQueue(struct node*);
void freeNode(struct node*);
void setTimerAtInterval();
void  mutexListManipulation();


/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
                  void *(*function)(void *) , void *arg)
{
    tcb *new_tcb;
    new_tcb = malloc(sizeof(tcb));

    ucontext_t* worker_thread_ctx;
    worker_thread_ctx = malloc(sizeof(ucontext_t));

    //set main context
    if (timerHasBeenCreated == 0){
        timerHasBeenCreated = 1;

        getcontext(&schedrr_ctx);
        schedrr_ctx.uc_link = NULL;
        schedrr_ctx.uc_stack.ss_sp = malloc(STACK_SIZE);
        schedrr_ctx.uc_stack.ss_size = STACK_SIZE;
        schedrr_ctx.uc_stack.ss_flags = 0;
        makecontext(&schedrr_ctx, sched_rr,0);
    }

    getcontext(worker_thread_ctx);

    //initialize context
    worker_thread_ctx->uc_link = NULL;

    worker_thread_ctx->uc_stack.ss_sp = malloc(STACK_SIZE);

    worker_thread_ctx->uc_stack.ss_size = STACK_SIZE;
    worker_thread_ctx->uc_stack.ss_flags = 0;

    makecontext(worker_thread_ctx, (void*)(function), 1, arg);

    new_tcb->thread_context = worker_thread_ctx;

    new_tcb->thread_id = globalId;
    *thread = globalId;
    globalId ++;

    new_tcb->thread_state = THREAD_READY;

    new_tcb->worker_T_TCB = *thread;

    new_tcb->thread_stack = NULL;

    //create TCB node for runqueue
    struct node *currNode = NULL;
    currNode = malloc(sizeof(struct node));
    currNode->data = new_tcb;
    currNode->next = NULL;

    if(runQueueHead == NULL){
        runQueueHead = currNode;
    }
    else{
        struct node* current = runQueueHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = currNode;
        currNode->next = NULL;
    }

    if(timerHasBeenCreated == 1){
        timerHasBeenCreated = 2;
        init_timer();
    }

    return 0;
}

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{
    // - change worker thread's state from Running to Ready
    setTimerAtInterval();
    swapcontext(runQueueHead->data->thread_context,&schedrr_ctx); 
    return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
    // - if value_ptr is provided, save return value
    runQueueHead->data->thread_state = THREAD_DONE;
    if(value_ptr != NULL){
        struct node* temp = blockedQueueHead;
        while(temp != NULL){
            if(temp->data->threadWeWaitingOn->thread_id == runQueueHead->data->thread_id){
                temp->data->childValWeWaitingOn = *((int *)value_ptr);
                break;
            }
            temp = temp->next;
        }
    }

    setTimerAtInterval();
    setcontext(&schedrr_ctx);
}

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{
    ucontext_t contextWeAreIn;
    getcontext(&contextWeAreIn);

    //node for thread calling thread is calling join on
    tcb *tcbWeWaitingOn;
    tcbWeWaitingOn = malloc(sizeof(tcb));

    struct node* whereParentIs;

    //go through ready queue and find the thread we're waiting for
    struct node *readyCurrNode = runQueueHead;
    int found = 0;
    while(readyCurrNode != NULL){
        if(readyCurrNode->data->thread_id == thread){
            tcbWeWaitingOn = readyCurrNode->data;
            found = 1;
        }
        readyCurrNode = readyCurrNode->next;
    }

    if( found == 0 )
        return thread;

        //have to create new node
        tcb *new_tcb;
        new_tcb = malloc(sizeof(tcb));

        ucontext_t* new_ctx;
        new_ctx = malloc(sizeof(ucontext_t));
        new_ctx = &contextWeAreIn;
        //create main context node
        new_tcb->thread_state = THREAD_BLOCKED_JOIN;
        new_tcb->thread_context = new_ctx;
        new_tcb->thread_stack = NULL;
        new_tcb->worker_T_TCB = 0;
        new_tcb->threadWeWaitingOn = tcbWeWaitingOn;
        new_tcb->thread_id = 0;
        struct node *new_node = NULL;
        new_node = malloc(sizeof(struct node));
        new_node->data = new_tcb;

        if(blockedQueueHead == NULL){
            blockedQueueHead = new_node;
            blockedQueueHead->next = NULL;
            getcontext(blockedQueueHead->data->thread_context);
        }
        else{
            struct node *tempBlockedHead = blockedQueueHead;
            while(tempBlockedHead->next != NULL){
                tempBlockedHead = tempBlockedHead->next;
            }
            tempBlockedHead->next = new_node;
            new_node->next = NULL;
            getcontext(new_node->data->thread_context);
        }
        whereParentIs = new_node;

    //switch to scheduler / timer context
    swapcontext(whereParentIs->data->thread_context,&schedrr_ctx);
    //remove parent from your list after join is done
    struct node* tempN = runQueueHead;
    runQueueHead = runQueueHead->next;
    if(value_ptr != NULL){
        int** valuePtr1 = (int**)value_ptr;
        *valuePtr1 = (int*) malloc(sizeof(int));
        **valuePtr1=tempN->data->childValWeWaitingOn;
    }
    freeNode(tempN);   
    return 0;
};

void freeNode(struct node* temp){
    if(temp->data->thread_id != 0){
        free(temp->data->thread_context->uc_stack.ss_sp);
    }
    free(temp->data);
    free(temp);
}

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
                      const pthread_mutexattr_t *mutexattr)
{
    //- initialize data structures for this mutex
    mutex->currOwner = NULL;
    mutex->state = 0; //currently not in use
    mutex->mutexInQuestion = mutex;
    return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{
   ucontext_t *tempCtx;
   tempCtx = malloc(sizeof(ucontext_t));
   getcontext(tempCtx);

   int retVal = __sync_lock_test_and_set(&(mutex->state),1);
   if( retVal == 1 )
   {
        //if it's taken right now
        //add to mutex blocked queue
        runQueueHead->data->thread_state = THREAD_BLOCKED_MUTEX;
        runQueueHead->data->mutexWeWaitingOn = mutex;

        struct node* temp = runQueueHead;
        runQueueHead = runQueueHead->next;
        if(mutexQueueHead == NULL){
            mutexQueueHead = temp;
            temp->next = NULL;
        }
        else{
            struct node *looper = mutexQueueHead;
            while(looper->next != NULL){
                looper = looper->next;
            }
            looper->next = temp;
            temp->next = NULL;
        }
        temp->data->thread_context = tempCtx;

        setTimerAtInterval();
        setcontext(&schedrr_ctx);
   }
   else {
    mutex->currOwner = runQueueHead->data;
   }
   
   return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
    // - release mutex and make it available again.
    // - put one or more threads in block list to run queue
    // so that they could compete for mutex later.
    if(mutex->currOwner->thread_id != runQueueHead->data->thread_id){
        return -1;
    }

    mutex->currOwner = NULL;
    __sync_lock_release(&(mutex->state));
    mutex->state=0;

    mutexListManipulation(mutex);

    setTimerAtInterval();
    swapcontext(runQueueHead->data->thread_context,&schedrr_ctx);

    return 0;
};

void  mutexListManipulation(worker_mutex_t *mutex){
    struct node* prev = NULL;
    struct node* current = mutexQueueHead;
    while(current != NULL){
        if(current->next == NULL && prev == NULL){ //only 1 node
            if(current->data->mutexWeWaitingOn == mutex){ //doesn't exist, do linked list manipulation
                addToRunQueue(current);
                mutexQueueHead = NULL;
            }
            break;
        }
        else if(current->next != NULL && prev == NULL){ //looking at the head
            if(current->data->mutexWeWaitingOn == mutex){ //doesn't exist
                mutexQueueHead = current->next;
                prev = NULL;
                addToRunQueue(current);
                current = mutexQueueHead;
            }
            else{
                prev = current;
                current = current->next;
            }
        }
        else{ //normal execution
            if(current->data->mutexWeWaitingOn == mutex){
                prev->next = current->next;
                addToRunQueue(current);
                current = prev->next;
            }
            else{
                prev = prev->next;
                current = current->next;
            }
        }
    }
}

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
    // - make sure mutex is not being used
    // - de-allocate dynamic memory created in worker_mutex_init
    if(mutex->state == 1){
        return -1;
    }
    mutexListManipulation(mutex);
    return 0;
};

void setTimerAtInterval()
{
	timer.it_interval.tv_usec = 0; 
	timer.it_interval.tv_sec = 0;

	timer.it_value.tv_usec = 40000;
	timer.it_value.tv_sec = 0;

	setitimer(ITIMER_PROF, &timer, NULL);
}

void init_timer(){
    struct sigaction sa;
	memset(&sa, 0, sizeof (sa));
	sa.sa_handler = &schedule;
	sigaction(SIGPROF, &sa, NULL);

   setTimerAtInterval();
}

/* scheduler */
int gSchedulerRuns = 1;
int gInSechduler = 0;
void schedule(int sig_nr, siginfo_t* info, void *context)
{
// - every time a timer interrupt occurs, your worker thread library
// should be contexted switched from a thread context to this
// schedule() function

// - invoke scheduling algorithms according to the policy (RR or MLFQ)

//- schedule policy
    if(schedulerHasBeenCreated == 1 && gInSechduler == 0){
        #ifndef MLFQ
            //setcontext(&schedrr_ctx);
            if( gSchedulerInSwapContext == 0)
                setcontext(&schedrr_ctx);
            else
                swapcontext(runQueueHead->data->thread_context,&schedrr_ctx);
        #else
            // Choose MLFQ
        #endif
    }
}

void blocked_list_check(){
    struct node* prev = NULL;
    struct node* current = blockedQueueHead;

    while(current != NULL){
        if(current->next == NULL && prev == NULL){ //only 1 node
            if(join_check(current) == 0){ //doesn't exist, do linked list manipulation
                addToRunQueue(current);
                blockedQueueHead = NULL;
            }
            break;
        }
        else if(current->next != NULL && prev == NULL){ //looking at the head
            if(join_check(current) == 0){ //doesn't exist
                blockedQueueHead = current->next;
                prev = NULL;
                addToRunQueue(current);
                current = blockedQueueHead;
            }
            else{
                prev = current;
                current = current->next;
            }
        }
        else{ //normal execution
            if(join_check(current) == 0){
                prev->next = current->next;
                addToRunQueue(current);
                current = prev->next;
            }
            else{
                prev = prev->next;
                current = current->next;
            }
        }
    }
}

void addToRunQueue(struct node* nodeToAdd){
    if(runQueueHead == NULL){
        runQueueHead = nodeToAdd;
    }
    else{
        struct node* curr = runQueueHead;
        nodeToAdd->data->thread_state = THREAD_READY;
        while(curr->next != NULL){
            curr = curr->next;
        }
        curr->next = nodeToAdd;
        nodeToAdd->next = NULL;
    }
}

int join_check(struct node* nodeToLookAt){
    struct node* curr = runQueueHead;
    while(curr != NULL){
        if(nodeToLookAt->data->threadWeWaitingOn->thread_id == curr->data->thread_id){
            return 1;
        }
        curr = curr->next;
    }

    curr = mutexQueueHead;
    while(curr != NULL){
        if(nodeToLookAt->data->threadWeWaitingOn->thread_id == curr->data->thread_id){
            return 1;
        }
        curr = curr->next;
    }

    return 0;
}

static void sched_rr()
{
    gSchedulerInSwapContext =0;
    gInSechduler =1;
  
    schedulerHasBeenCreated = 1;
   
    ucontext_t dummyContext;

    if(runQueueHead->data->thread_state == THREAD_DONE){
        //exit has just been called so link list is already altered
        struct node* tempN = runQueueHead;
        runQueueHead = runQueueHead->next;
        freeNode(tempN);
    }

    blocked_list_check();

    if(runQueueHead->next == NULL){
        //only 1 thread currently
        runQueueHead->data->thread_state = THREAD_RUNNING;
        
        gSchedulerInSwapContext =1;
        gInSechduler =0;
        setTimerAtInterval();
        swapcontext(&dummyContext, runQueueHead->data->thread_context);
    }
    else{
        //if not, set state of head node to READY, do linked list arithmetic, and
        //put new head state to RUNNING and swap to that context
        runQueueHead->data->thread_state = THREAD_READY;

        struct node *oldHead = runQueueHead;

        runQueueHead = runQueueHead->next;

        struct node* current = runQueueHead;
        while (current->next != NULL) {
            current = current->next;
        }

        current->next = oldHead;
        oldHead->next = NULL;
        
        runQueueHead->data->thread_state = THREAD_RUNNING;

        gSchedulerInSwapContext =1;
        gInSechduler = 0;
        setTimerAtInterval();
        swapcontext(&dummyContext, runQueueHead->data->thread_context);
    } 
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq()
{
    // - your own implementation of MLFQ
    // (feel free to modify arguments and return types)
}

// Feel free to add any other functions you need.
// You can also create separate files for helper functions, structures, etc.
// But make sure that the Makefile is updated to account for the same.