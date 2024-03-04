CC = gcc
CFLAGS = -g -w

all:: clean one_thread multiple_threads multiple_threads_yield multiple_threads_mutex multiple_threads_different_workload multiple_threads_with_return test

one_thread:
	$(CC) $(CFLAGS) -o one_thread one_thread.c -L../ -lthread-worker

multiple_threads:
	$(CC) $(CFLAGS) -o multiple_threads multiple_threads.c -L../ -lthread-worker

multiple_threads_yield:
	$(CC) $(CFLAGS) -o multiple_threads_yield multiple_threads_yield.c -L../ -lthread-worker

multiple_threads_mutex:
	$(CC) $(CFLAGS) -o multiple_threads_mutex multiple_threads_mutex.c -L../ -lthread-worker

multiple_threads_different_workload:
	$(CC) $(CFLAGS) -o multiple_threads_different_workload multiple_threads_different_workload.c -L../ -lthread-worker

multiple_threads_with_return:
	$(CC) $(CFLAGS) -o multiple_threads_with_return multiple_threads_with_return.c -L../ -lthread-worker

test:
	$(CC) $(CFLAGS) -o test test.c -L../ -lthread-worker

clean:
	rm -rf test one_thread multiple_threads multiple_threads_yield multiple_threads_mutex multiple_threads_different_workload multiple_threads_with_return *.o *.dSYM
