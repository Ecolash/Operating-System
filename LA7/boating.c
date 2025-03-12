#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>

int *BT;
int *BC;
bool *BA;
pthread_barrier_t *BB;

typedef struct
{
    int id;
    int vtime;
    int rtime;
} visitor;

typedef struct
{
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

void init(semaphore *s, int value)
{
    s->value = value;
    s->mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    s->cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    return;
}

void P(semaphore *s)
{
    pthread_mutex_lock(&s->mtx);
    while (s->value <= 0) pthread_cond_wait(&s->cv, &s->mtx);
    s->value--;
    pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s)
{
    pthread_mutex_lock(&s->mtx);
    s->value++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->mtx);
}

int m, n;
semaphore boat, rider;
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t EOS;

void* B_THREAD(void* arg) {
    int id = *(int*)arg;
    printf("Boat %6d \tReady\n", id);

    while(1)
    {
        V(&rider);
        P(&boat);

        pthread_mutex_lock(&bmtx);
        BA[id - 1] = true;
        BC[id - 1] = -1;
        pthread_mutex_unlock(&bmtx);
        pthread_barrier_wait(&BB[id - 1]);

        pthread_mutex_lock(&bmtx);
        BA[id - 1] = false;
        int visitor_id = BC[id - 1];
        int rtime = BT[id - 1];
        pthread_mutex_unlock(&bmtx);

        printf("Boat %6d \tStart of ride for visitor %3d\n", id, visitor_id);
        usleep(rtime * 100000);
        printf("Boat %6d \tEnd of ride for visitor %3d (ride time = %2d)\n", id, visitor_id, rtime);

        n = n - 1;
        if (n == 0)
        {
            pthread_barrier_wait(&EOS);
            break;
        }
    }
    // printf("Boat %6d \tShutting down\n", id);
    return NULL;
}

void* R_THREAD(void* arg) {
    visitor *v = (visitor*)arg;
    int rtime = v->rtime;
    int vtime = v->vtime;
    printf("Visitor %3d \tStarts sightseeing for %-3d minutes\n", v->id, vtime);
    usleep(vtime * 100000);

    printf("Visitor %3d \tReady to ride a boat (ride time = %2d)\n", v->id, rtime);
    V(&boat);
    P(&rider);
    int found_boat = -1;
    while (found_boat == -1)
    {
        pthread_mutex_lock(&bmtx);
        for (int i = 0; i < m; i++)
        {
            if (BA[i] && BC[i] == -1)
            {
                BC[i] = v->id;
                BT[i] = rtime;
                found_boat = i + 1;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);
        if (found_boat == -1) usleep(1000);
    }
    printf("Visitor %3d \tFinds boat %2d\n", v->id, found_boat);
    pthread_barrier_wait(&BB[found_boat - 1]);
    usleep(rtime * 100000);
    printf("Visitor %3d \tLeaving\n", v->id);
    return NULL;
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s m n\n", argv[0]);
        printf("--  m = Number of boats\n");
        printf("--  n = Number of visitors\n");
        exit(EXIT_FAILURE);
    }

    m = atoi(argv[1]); 
    n = atoi(argv[2]); 

    BA = (bool *)malloc(m * sizeof(int));
    BT = (int *)malloc(m * sizeof(int));
    BC = (int *)malloc(m * sizeof(int));
    BB = (pthread_barrier_t *)malloc(m * sizeof(pthread_barrier_t));

    visitor V[n]; 
    int boats[m];
    for(int i = 0; i < n; i++)
    {
        V[i].id = i + 1;
        V[i].vtime = 30 + rand() % 91;
        V[i].rtime = 15 + rand() % 46;
    }

    for (int i = 0; i < m; i++) 
    {
        boats[i] = i + 1;
        BA[i] = 0;
        BT[i] = 0;
        BC[i] = -1;
        pthread_barrier_init(&BB[i], NULL, 2);
    }

    init(&boat, 0);
    init(&rider, 0);
    pthread_barrier_init(&EOS, NULL, 2);


    pthread_t B_THREADs[m], R_THREADs[n];
    for (int i = 0; i < m; i++) pthread_create(&B_THREADs[i], NULL, B_THREAD, &boats[i]);
    for (int i = 0; i < n; i++) pthread_create(&R_THREADs[i], NULL, R_THREAD, &V[i]);
    pthread_barrier_wait(&EOS);


    for (int i = 0; i < m; i++) pthread_cancel(B_THREADs[i]);
    for (int i = 0; i < n; i++) pthread_cancel(R_THREADs[i]);

    for (int i = 0; i < m; i++) pthread_barrier_destroy(&BB[i]);
    pthread_barrier_destroy(&EOS);
    free(BA);
    free(BB);
    free(BC);
    free(BT);
    exit(0);
}