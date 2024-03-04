#include <stdio.h>
#include <unistd.h>
#include "../thread-worker.h"

void dummy_work(void *arg)
{
    printf("enter thread function\n");
    int i = 0;
    int j = 0;
    int n = *((int *)arg);

    for (i = 0; i < 20; i++)
    {
        for (j = 0; j < 10000000; j++)
        {
        }
        printf("Thread %d running with i %d\n", n,i);
    }

    printf("Thread %d exiting\n", n);
    worker_exit(NULL);
}

int main(int argc, char **argv)
{
    printf("Running main thread\n");
    worker_t thread;

    int id = 1;
    worker_create(&thread, NULL, &dummy_work, &id);

    printf("Main thread waiting\n");
    worker_join(thread, NULL);

    //while(1);

    printf("Main thread resume\n");

    printf("Main thread exit\n");
    return 0;
}
