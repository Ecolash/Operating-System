// PRODUCER CONSUMER PROBLEM

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5 

int buffer[BUFFER_SIZE];
int in = 0;    
int out = 0;   
int count = 0; 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void *producer(void *arg)
{
    int item = rand() % 100;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) pthread_cond_wait(&empty, &mutex);

        buffer[in] = item;
        printf("[+] BUFF[%d] = %d\n", in, item);
        in = (in + 1) % BUFFER_SIZE;
        count++;
        item  = rand() % 100;

        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);
        sleep(1); 
    }
}

void *consumer(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (count == 0) pthread_cond_wait(&full, &mutex);

        int item = buffer[out];
        printf("[-] BUFF[%d] = %d\n", out, item);
        out = (out + 1) % BUFFER_SIZE;
        count--;

        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
}

int main()
{
    pthread_t prod_thread, cons_thread;

    pthread_create(&prod_thread, NULL, producer, NULL);
    pthread_create(&cons_thread, NULL, consumer, NULL);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    return 0;
}
