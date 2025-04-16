#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*
A mutex (Mutual Exclusion) is used to prevent race conditions when multiple threads access shared data.

🔹 Mutex Functions:

pthread_mutex_init() — Initializes a mutex.
pthread_mutex_lock() — Locks the mutex.
pthread_mutex_unlock() — Unlocks the mutex.
pthread_mutex_destroy() — Destroys the mutex.
*/

typedef struct
{
    int thread_no;
    int value;
} thread_args;

void *functionC(void *args);
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
    int rc1, rc2;
    pthread_t thread1, thread2;

    thread_args args1 = {1, 10}; // Thread 1: Increment by 10
    thread_args args2 = {2, 20}; // Thread 2: Decrement by 20

    rc1 = pthread_create(&thread1, NULL, functionC, &args1);
    rc2 = pthread_create(&thread2, NULL, functionC, &args2);

    if (rc1 != 0 || rc2 != 0)
    {
        printf("Thread creation failed: %d, %d\n", rc1, rc2);
        exit(1);
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    printf("Final Counter Value: %d\n", counter);
    exit(0);
}

void *functionC(void *args)
{
    thread_args *data = (thread_args *)args;
    pthread_mutex_lock(&mutex1);
    if (data->thread_no == 1) counter += data->value;
    if (data->thread_no == 2) counter -= data->value;

    printf("Thread %d modified counter. Counter value: %d\n", data->thread_no, counter);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}
