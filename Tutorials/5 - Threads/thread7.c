// DINING PHILOSOPHERS PROBLEM

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5

pthread_mutex_t chopsticks[NUM_PHILOSOPHERS];

void *philosopher(void *num)
{
    int id = *(int *)num;
    while (1)
    {
        printf("Philosopher %d is thinking...\n", id);
        int LCS = id;
        int RCS = (id + 1) % NUM_PHILOSOPHERS; 
        if (id % 2 == 0)
        {
            pthread_mutex_lock(&chopsticks[RCS]);
            printf("Philosopher %d picked up chopstick %d (right)\n", id, RCS);

            pthread_mutex_lock(&chopsticks[LCS]);
            printf("Philosopher %d picked up chopstick %d (left)\n", id, LCS);
        }
        else
        {
            pthread_mutex_lock(&chopsticks[LCS]);
            printf("Philosopher %d picked up chopstick %d (left)\n", id, LCS);

            pthread_mutex_lock(&chopsticks[RCS]);
            printf("Philosopher %d picked up chopstick %d (right)\n", id, RCS);
        }

        printf("Philosopher %d is eating...\n", id);
        sleep(rand() % 3 + 1);

        pthread_mutex_unlock(&chopsticks[RCS]);
        printf("Philosopher %d put down chopstick %d (right)\n", id, RCS);

        pthread_mutex_unlock(&chopsticks[id]);
        printf("Philosopher %d put down chopstick %d (left)\n", id, id);
        sleep(rand() % 3 + 1);
    }

    return NULL;
}

int main()
{
    pthread_t philosophers[NUM_PHILOSOPHERS];
    int ids[NUM_PHILOSOPHERS];

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) pthread_mutex_init(&chopsticks[i], NULL);
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) ids[i] = i;

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) pthread_create(&philosophers[i], NULL, philosopher, &ids[i]);
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) pthread_join(philosophers[i], NULL);
    exit(0);
}
