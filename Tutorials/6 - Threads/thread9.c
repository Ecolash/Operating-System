/*
Pthread Barrier Tutorial

    A pthread barrier is used to synchronize a group of threads at a specific point
    in execution. When a thread reaches the barrier, it waits until all threads in the group
    have also reached the barrier. Once they all arrive, they are simultaneously released to continue.
    
    Use cases:
    - Synchronizing phases: Ensure multiple threads complete one phase before starting another.
    - Coordinated start: Begin processing simultaneously after all threads are ready.
    
    Detailed Function Prototypes:
    
    int pthread_barrier_init(pthread_barrier_t *restrict barrier,
                            const pthread_barrierattr_t *restrict attr,
                            unsigned count);
        - Initializes the barrier pointed to by 'barrier' for 'count' threads.
        - Parameters:
            barrier - Pointer to the barrier object.
            attr    - Pointer to the barrier attributes (NULL for default attributes).
            count   - Number of threads to synchronize.
        - Returns 0 on success.
    
    int pthread_barrier_wait(pthread_barrier_t *barrier);
        - Makes the calling thread wait at the barrier until all required threads reach it.
        - Parameters:
            barrier - Pointer to the barrier object.
        - Returns PTHREAD_BARRIER_SERIAL_THREAD to one thread (which may perform additional tasks),
        and 0 to the others.
    
    int pthread_barrier_destroy(pthread_barrier_t *barrier);
        - Destroys the barrier, freeing associated resources.
        - Parameters:
            barrier - Pointer to the barrier object.
        - Returns 0 on success.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4

pthread_barrier_t barrier;

void* thread_func(void *arg) {
    int thread_num = *((int*) arg);
    printf("Thread %d reached the barrier.\n", thread_num);
    
    int ret = pthread_barrier_wait(&barrier);
    if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
        printf("Thread %d is the serial thread at the barrier.\n", thread_num);

    printf("Thread %d passed the barrier.\n", thread_num);
    return NULL;
}

int main(void) {
    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];

    if (pthread_barrier_init(&barrier, NULL, THREAD_COUNT)) {
        perror("pthread_barrier_init");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i])) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}