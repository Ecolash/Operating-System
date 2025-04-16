#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *functionC();
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
    int rc1, rc2;
    pthread_t thread1, thread2;

    rc1 = pthread_create(&thread1, NULL, functionC, NULL);
    rc2 = pthread_create(&thread2, NULL, functionC, NULL);
    if (rc1 != 0 || rc2 != 0)
    {
        printf("Thread creation failed: %d, %d\n", rc1, rc2);
        exit(1);
    }


    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    exit(0);
}

void *functionC()
{
    pthread_mutex_lock(&mutex1);
    counter++;
    printf("Counter value: %d\n", counter);
    pthread_mutex_unlock(&mutex1);
}
