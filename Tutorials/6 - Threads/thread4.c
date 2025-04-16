/*
🔒 Mutex (Mutual Exclusion)
   A mutex is used to ensure exclusive access to a shared resource. 
   It is ideal when:

  - Protecting shared data: When multiple threads read/write shared data, a mutex ensures only one thread can modify it at a time.
  - Critical sections: Use a mutex to guard critical code regions where data inconsistency may arise.
  - Simple synchronization: Ideal when one thread performs a task and others must wait for it to finish.

🔄 Condition Variables
   A condition variable is used for thread communication, especially when a thread must wait for a condition to become true. 
   It is ideal when:

  - Thread waiting: When a thread needs to wait until a certain condition is met (e.g., data availability, queue not empty).
  - Signaling: Useful when one thread produces data (producer) and another thread consumes it (consumer).
  - Complex synchronization: Preferred when multiple threads must coordinate under specific conditions.
*/

#include <stdio.h>
#include <pthread.h>

#define NTHREADS 10
void *thread_function(void *);
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
    pthread_t thread_id[NTHREADS];
    int i, j;

    for (i = 0; i < NTHREADS; i++) pthread_create(&thread_id[i], NULL, thread_function, (void *)i);
    for (j = 0; j < NTHREADS; j++) pthread_join(thread_id[j], NULL);
    printf("Final counter value: %d\n", counter);
}

void *thread_function(void *dummyPtr)
{
    int i = (int)dummyPtr;
    pthread_mutex_lock(&mutex1);
    counter += i;
    pthread_mutex_unlock(&mutex1);
    printf("Thread number %ld | Counter : %d\n", pthread_self(), counter);
}