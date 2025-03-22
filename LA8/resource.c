#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define MAX_THREADS 100
#define MAX_RESOURCES 20
#define MAX_QUEUE_SIZE 1000

#define ADDITIONAL 0
#define RELEASE 1
#define QUIT 2

typedef struct {
    int thread_id;
    int request_type;
    int request[MAX_RESOURCES];
} Request;

typedef struct {
    Request requests[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;


/*
=============================================================
GLOBAL VARIABLES
-------------------------------------------------------------
| Variable     | Description                            |
|--------------|----------------------------------------|
| m            | Number of resource types               |
| n            | Number of threads                      |
| available    | Available resources                    |
| max_need     | Maximum need of each thread            |
| allocation   | Current allocation                     |
| need         | Need = Max - Allocation                |
-------------------------------------------------------------
*/

int m, n;
int available[MAX_RESOURCES]; 
int max_need[MAX_THREADS][MAX_RESOURCES]; 
int allocation[MAX_THREADS][MAX_RESOURCES];
int need[MAX_THREADS][MAX_RESOURCES];
int status[MAX_THREADS];


/*
=============================================================
THREAD SYNCHRONIZATION VARIABLES
-------------------------------------------------------------
| Variable     | Description                            |
|--------------|----------------------------------------|
| rmtx         | Resource mutex                         |
| pmtx         | Print mutex                            |
| bos          | Beginning of session barrier           |
| reqb         | Request barrier                        |
| ackb         | Acknowledgment barriers                |
| cv           | Condition variables for each thread    |
| cmtx         | Mutex for condition variables          |
-------------------------------------------------------------
*/

pthread_mutex_t rmtx; 
pthread_mutex_t pmtx;
pthread_barrier_t bos; 
pthread_barrier_t reqb; 
pthread_barrier_t ackb[MAX_THREADS];

pthread_cond_t cv[MAX_THREADS]; 
pthread_mutex_t cmtx[MAX_THREADS]; 

Queue pending_queue;
Request shared_request;
int active_threads = 0;

void initialize_queue(Queue *q);
bool is_empty(Queue *q);
bool is_full(Queue *q);
void enqueue(Queue *q, Request req);
Request dequeue(Queue *q);
Request queue_front(Queue *q);
void print_state();
void print_queue();
bool check_safety();
bool can_satisfy_request(int thread_id, int req[]);
void process_pending_requests();
void *thread_function(void *arg);
void *master_function(void *arg);

void initialize_queue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

bool is_empty(Queue *q) { return q->size == 0; }
bool is_full(Queue *q)  { return q->size == MAX_QUEUE_SIZE; }

void enqueue(Queue *q, Request req) {
    if (is_full(q)) {
        printf("Queue is full\n");
        exit(1);
    }
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->requests[q->rear] = req;
    q->size++;
}

Request dequeue(Queue *q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        exit(1);
    }
    Request req = q->requests[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    return req;
}

Request queue_front(Queue *q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        exit(1);
    }
    return q->requests[q->front];
}
// Used for debugging
void print_state() {
    int i, j;
    
    pthread_mutex_lock(&pmtx);
    
    printf("Current system state:\n");
    printf("Available resources: ");
    for (i = 0; i < m; i++) printf("%d ", available[i]);
    printf("\n");
    
    printf("Allocation matrix:\n");
    for (i = 0; i < n; i++) {
        printf("Thread %d: ", i);
        for (j = 0; j < m; j++) printf("%d ", allocation[i][j]);
        printf("\n");
    }
    
    printf("Need matrix:\n");
    for (i = 0; i < n; i++) {
        printf("Thread %d: ", i);
        for (j = 0; j < m; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
    
    pthread_mutex_unlock(&pmtx);
}

void print_queue() {
    pthread_mutex_lock(&pmtx);
    for (int i = 0; i < pending_queue.size; i++)
    {
        Request curr = pending_queue.requests[(pending_queue.front + i) % MAX_QUEUE_SIZE];
        int curr_id = curr.thread_id;
        printf("%d ", curr_id);
    }
    printf("\n");   
    pthread_mutex_unlock(&pmtx);
}

bool check_safety() {
#ifdef _DLAVOID
    int work[MAX_RESOURCES];
    bool finish[MAX_THREADS];
    int i, j, k;
    
    // Initialize work and finish arrays
    for (i = 0; i < m; i++) {
        work[i] = available[i];
    }
    
    for (i = 0; i < n; i++) {
        finish[i] = false;
    }
    
    // Try to find a thread that can finish
    bool found;
    do {
        found = false;
        for (i = 0; i < n; i++) {
            if (!finish[i]) {
                // Check if this thread can finish
                bool can_finish = true;
                for (j = 0; j < m; j++) {
                    if (need[i][j] > work[j]) {
                        can_finish = false;
                        break;
                    }
                }
                
                if (can_finish) {
                    // This thread can finish, so add its resources to work
                    for (j = 0; j < m; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                }
            }
        }
    } while (found);
    
    // Check if all threads can finish
    for (i = 0; i < n; i++) {
        if (!finish[i]) {
            return false; // System is not in a safe state
        }
    }
    
    return true; // System is in a safe state
#else
    return true; // If deadlock avoidance is not enabled, always return true
#endif
}

// Check if a request can be satisfied
bool can_satisfy_request(int thread_id, int req[]) 
{
    for (int i = 0; i < m; i++)  if (req[i] > need[thread_id][i]) return false;
    for (int i = 0; i < m; i++)  if (req[i] > available[i]) return false;

    // If deadlock avoidance is enabled, check if granting the request leads to a safe state
#ifdef _DLAVOID
    // Try to allocate resources temporarily
    int temp_available[MAX_RESOURCES];
    int temp_allocation[MAX_THREADS][MAX_RESOURCES];
    int temp_need[MAX_THREADS][MAX_RESOURCES];
    
    int i = 0;
    for (i = 0; i < m; i++) {
        temp_available[i] = available[i] - req[i];
    }
    
    for (i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            temp_allocation[i][j] = allocation[i][j];
            temp_need[i][j] = need[i][j];
        }
    }
    
    for (i = 0; i < m; i++) {
        temp_allocation[thread_id][i] += req[i];
        temp_need[thread_id][i] -= req[i];
    }
    
    // Save the current state
    int saved_available[MAX_RESOURCES];
    int saved_allocation[MAX_THREADS][MAX_RESOURCES];
    int saved_need[MAX_THREADS][MAX_RESOURCES];
    
    for (i = 0; i < m; i++) {
        saved_available[i] = available[i];
        available[i] = temp_available[i];
    }
    
    for (i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            saved_allocation[i][j] = allocation[i][j];
            saved_need[i][j] = need[i][j];
            allocation[i][j] = temp_allocation[i][j];
            need[i][j] = temp_need[i][j];
        }
    }
    
    // Check if the system is in a safe state
    bool safe = check_safety();
    
    // Restore the state
    for (i = 0; i < m; i++) {
        available[i] = saved_available[i];
    }
    
    for (i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            allocation[i][j] = saved_allocation[i][j];
            need[i][j] = saved_need[i][j];
        }
    }
    
    return safe;
#else
    return true; // If deadlock avoidance is not enabled, just check if enough resources are available
#endif
}

void process_pending_requests() {
    int i;
    bool processed_any = true;
    pthread_mutex_lock(&pmtx);
    printf("Master thread tries to grant pending requests\n");
    pthread_mutex_unlock(&pmtx);
    
    while (processed_any && !is_empty(&pending_queue)) {
        processed_any = false;
        int num_requests = pending_queue.size;

        for (i = 0; i < num_requests; i++) 
        {
            if (is_empty(&pending_queue)) break;            
            Request req = queue_front(&pending_queue);
            int thread_id = req.thread_id;
            
            if (can_satisfy_request(thread_id, req.request)) {
                pthread_mutex_lock(&pmtx);
                printf("Master thread grants resource request for thread %d\n", thread_id);
                pthread_mutex_unlock(&pmtx);

                
                for (int j = 0; j < m; j++) 
                {
                    available[j] -= req.request[j];
                    allocation[thread_id][j] += req.request[j];
                    need[thread_id][j] -= req.request[j];
                }
                
                dequeue(&pending_queue);
                pthread_mutex_lock(&pmtx);
                printf("\t\tWaiting Threads: ");
                pthread_mutex_unlock(&pmtx);
                print_queue();

                status[thread_id] = 0;
                pthread_mutex_lock(&cmtx[thread_id]);
                pthread_cond_signal(&cv[thread_id]);
                pthread_mutex_unlock(&cmtx[thread_id]);
                processed_any = true;
            } else {
                dequeue(&pending_queue);
                pthread_mutex_lock(&pmtx);
                printf("\t+++ Insufficient resources to grant request of thread %d \n", thread_id);
                pthread_mutex_unlock(&pmtx);
                enqueue(&pending_queue, req);
            }
        }
    }
}

// Thread function
void *thread_function(void *arg) {
    int thread_id = *(int *)arg;
    pthread_mutex_lock(&pmtx);
    printf("\tThread %-2d born\n", thread_id);
    pthread_mutex_unlock(&pmtx);
    pthread_barrier_wait(&bos);
    
    char filename[50];
    FILE *fp;
    int i, j;
    sprintf(filename, "input/thread%02d.txt", thread_id);
    fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }
    
    for (i = 0; i < m; i++) {
        fscanf(fp, "%d", &max_need[thread_id][i]);
        need[thread_id][i] = max_need[thread_id][i];
    }
    
    int delay;
    char request_type;
    bool quit = false;
    
    while (!quit) {
        fscanf(fp, "%d", &delay);
        usleep(delay * 1000);
        
        fscanf(fp, "%c", &request_type);
        
        if (request_type == 'Q') {
            
            pthread_mutex_lock(&rmtx);
            Request req;
            req.thread_id = thread_id;
            req.request_type = QUIT;
            shared_request = req;
            
            pthread_barrier_wait(&reqb);
            pthread_barrier_wait(&ackb[thread_id]);
            pthread_mutex_unlock(&rmtx);

            pthread_mutex_lock(&pmtx);
            printf("Thread %d is going to quit\n", thread_id);
            pthread_mutex_unlock(&pmtx);
            break;
        } 

        else if (request_type == 'R') 
        {
            int req[MAX_RESOURCES];
            int type = RELEASE;            
            for (i = 0; i < m; i++) fscanf(fp, "%d", &req[i]);
            for (i = 0; i < m; i++) if (req[i] > 0) type = ADDITIONAL;
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d sends resource request: type = ADDITIONAL\n", thread_id);
            pthread_mutex_unlock(&pmtx);

            pthread_mutex_lock(&rmtx);
            shared_request.thread_id = thread_id;
            shared_request.request_type = type;
            for (i = 0; i < m; i++) shared_request.request[i] = req[i];
            printf("Thread %d \n" , thread_id);

            pthread_barrier_wait(&reqb);
            pthread_barrier_wait(&ackb[thread_id]);
            pthread_mutex_unlock(&rmtx);

            if (type == RELEASE) continue;
            else
            {
                if (status[thread_id] == -1) status[thread_id] = 1;
                pthread_mutex_lock(&cmtx[thread_id]);
                while (status[thread_id] == 1) pthread_cond_wait(&cv[thread_id], &cmtx[thread_id]);
                pthread_mutex_unlock(&cmtx[thread_id]);
                status[thread_id] = -1;

                pthread_mutex_lock(&pmtx);
                printf("\tThread %d is granted its last resource request\n", thread_id);
                pthread_mutex_unlock(&pmtx);
            }
        }
    }
    
    fclose(fp);
    return NULL;
}

void *master_function(void *arg) {
    int i;
    pthread_barrier_wait(&bos);
    while (active_threads > 0) 
    {
        pthread_barrier_wait(&reqb);
        Request req = shared_request;
        int thread_id = req.thread_id;
        pthread_barrier_wait(&ackb[thread_id]);

        if (req.request_type == QUIT) {
            {
                int thread_id = req.thread_id;
                for (i = 0; i < m; i++) {
                    available[i] += allocation[thread_id][i];
                    allocation[thread_id][i] = 0;
                    need[thread_id][i] = 0;
                }
                active_threads--;
                pthread_mutex_lock(&pmtx);
                printf("%d threads left: ", active_threads);
                for (int j = 0; j < n; j++) {
                    if (allocation[j][0] != 0 || need[j][0] != 0) {
                        printf("%d ", j);
                    }
                }
                printf("\n");
                printf("Available resources: ");
                for (int j = 0; j < m; j++) printf("%d ", available[j]);
                printf("\n");
                pthread_mutex_unlock(&pmtx);
                if (active_threads == 0) break;
            }
        }
        else if (req.request_type == RELEASE) {
            int thread_id = req.thread_id;
            for (i = 0; i < m; i++) {
                available[i] -= req.request[i];
                allocation[thread_id][i] += req.request[i];
                need[thread_id][i] -= req.request[i];
            }
        } 
        else 
        {
            for (i = 0; i < m; i++)
            {
                if (req.request[i] < 0) 
                {
                    available[i] -= req.request[i];
                    allocation[req.thread_id][i] += req.request[i];
                    need[req.thread_id][i] -= req.request[i];
                    req.request[i] = 0;
                }
            }

            enqueue(&pending_queue, req);
            pthread_mutex_lock(&pmtx);
            printf("Master thread stores resource request of thread %d\n", req.thread_id);
            printf("\t\tWaiting Threads: ");
            pthread_mutex_unlock(&pmtx);
            print_queue();
        }
        process_pending_requests();
    }
    
    pthread_exit(NULL);
}

int main() 
{
    int i, j;
    pthread_t threads[MAX_THREADS];
    pthread_t master_thread;
    int thread_ids[MAX_THREADS];
    FILE *fp;
    
    initialize_queue(&pending_queue);
    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_init(&pmtx, NULL);
    
    fp = fopen("input/system.txt", "r");
    if (fp == NULL) {
        printf("Error opening file input/system.txt\n");
        exit(1);
    }
    
    fscanf(fp, "%d", &m);
    fscanf(fp, "%d", &n);
    active_threads = n;
    for (i = 0; i < m; i++) fscanf(fp, "%d", &available[i]);    
    fclose(fp);

    printf("[+] Number of resource types: %d\n", m);
    printf("[+] Number of threads: %d\n", n);
    printf("[+] Available resources: ");
    for (i = 0; i < m; i++) printf("%d ", available[i]);
    for (i = 0; i < n; i++) status[i] = -1;
    printf("\n\n");
    
    for (i = 0; i < n; i++) 
    for (j = 0; j < m; j++) 
    {
        allocation[i][j] = 0;
        need[i][j] = 0;
    }
    
    pthread_barrier_init(&bos, NULL, n + 1);
    pthread_barrier_init(&reqb, NULL, 2);
    
    for (i = 0; i < n; i++) {
        pthread_barrier_init(&ackb[i], NULL, 2);
        pthread_cond_init(&cv[i], NULL);
        pthread_mutex_init(&cmtx[i], NULL);
    }
    
    for (i = 0; i < n; i++) thread_ids[i] = i;

    pthread_create(&master_thread, NULL, master_function, NULL);
    for (i = 0; i < n; i++) { pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]); }
    
    for (i = 0; i < n; i++)  pthread_join(threads[i], NULL);
    pthread_join(master_thread, NULL);
    
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    pthread_barrier_destroy(&bos);
    pthread_barrier_destroy(&reqb);
    
    for (i = 0; i < n; i++) {
        pthread_barrier_destroy(&ackb[i]);
        pthread_cond_destroy(&cv[i]);
        pthread_mutex_destroy(&cmtx[i]);
    }
    
    printf("All threads have finished\n");
    return 0;
}