#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CHAIRS 5                // Number of waiting chairs
#define TOTAL_CUSTOMERS 15          // Total number of customers visiting

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t barber_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t customer_cond = PTHREAD_COND_INITIALIZER;

int waiting_customers = 0; // Customers currently waiting
int barber_sleeping = 1;   // Barber's status (1 = asleep, 0 = awake)

void *barber(void *arg)
{
    while (1)
    {++
        pthread_mutex_lock(&mutex);
        while (waiting_customers == 0)
        {
            printf("💤 Barber is sleeping\n");
            pthread_cond_wait(&barber_cond, &mutex);
        }

        waiting_customers--;
        printf("💈 Barber is cutting hair. Waiting customers: %d\n", waiting_customers);

        pthread_cond_signal(&customer_cond); 
        pthread_mutex_unlock(&mutex);
        sleep(2); 
    }
}

void *customer(void *arg)
{
    pthread_mutex_lock(&mutex);
    if (waiting_customers < MAX_CHAIRS)
    {
        waiting_customers++;
        printf("🙂 Customer %d enters. Waiting customers: %d\n", *(int *)arg, waiting_customers);
        if (barber_sleeping)
        {
            barber_sleeping = 0;
            pthread_cond_signal(&barber_cond); // Wake up the barber
        }
        pthread_cond_wait(&customer_cond, &mutex); // Wait for the barber to start his haircut
    }
    else printf("😞 Customer %d leaves (No available chairs)\n", *(int *)arg);
    pthread_mutex_unlock(&mutex);
    free(arg);
    return NULL;
}

int main()
{
    pthread_t barber_thread, customer_threads[TOTAL_CUSTOMERS];
    pthread_create(&barber_thread, NULL, barber, NULL);

    for (int i = 0; i < TOTAL_CUSTOMERS; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&customer_threads[i], NULL, customer, id);
        sleep(rand() % 3); 
    }

    for (int i = 0; i < TOTAL_CUSTOMERS; i++) pthread_join(customer_threads[i], NULL);

    pthread_cancel(barber_thread);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&barber_cond);
    pthread_cond_destroy(&customer_cond);
    exit(0);
}
