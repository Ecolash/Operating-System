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

int m, n;
int available[MAX_RESOURCES]; 
int max_need[MAX_THREADS][MAX_RESOURCES]; 
int allocation[MAX_THREADS][MAX_RESOURCES];
int need[MAX_THREADS][MAX_RESOURCES];
bool thread_exit[MAX_THREADS];
int num_exited = 0;

pthread_mutex_t rmtx; 
pthread_mutex_t pmtx;
pthread_barrier_t bos; 
pthread_barrier_t reqb; 
pthread_barrier_t ackb[MAX_THREADS];

pthread_cond_t cv[MAX_THREADS]; 
pthread_mutex_t cmtx[MAX_THREADS]; 
bool thread_sync[MAX_THREADS];

Queue pending_queue;
Request shared_request;

// Function declarations
void initialize_queue(Queue *q);
bool is_empty(Queue *q);
bool is_full(Queue *q);
void enqueue(Queue *q, Request req);
Request dequeue(Queue *q);
Request queue_front(Queue *q);
void print_waiting_threads();
bool check_safety(int thread_id, int request[]);
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

void print_waiting_threads() {
    Queue temp = pending_queue;
    bool first = true;
    
    while (!is_empty(&temp)) {
        if (!first) printf(" ");
        printf("%d", temp.requests[temp.front].thread_id);
        dequeue(&temp);
        first = false;
    }
}

bool check_safety(int thread_id, int request[]) {
#ifdef _DLAVOID
    int work[MAX_RESOURCES];
    bool finish[MAX_THREADS];
    int i, j;
    
    // Create temporary allocation and need for the requesting thread
    int temp_alloc[MAX_RESOURCES], temp_need[MAX_RESOURCES];
    
    // Initialize work array
    for (i = 0; i < m; i++) {
        work[i] = available[i] - request[i];
        temp_alloc[i] = allocation[thread_id][i] + request[i];
        temp_need[i] = need[thread_id][i] - request[i];
        if (work[i] < 0 || temp_need[i] < 0) {
            return false;
        }
    }
    
    // Initialize finish array
    for (i = 0; i < n; i++) {
        finish[i] = thread_exit[i];
    }
    
    // Try to find a thread that can finish
    int count = 0;
    bool found = true;
    
    while (found && count < n) {
        found = false;
        for (i = 0; i < n; i++) {
            if (!finish[i]) {
                bool can_finish = true;
                
                for (j = 0; j < m; j++) {
                    int need_val = (i == thread_id) ? temp_need[j] : need[i][j];
                    if (need_val > work[j]) {
                        can_finish = false;
                        break;
                    }
                }
                
                if (can_finish) {
                    for (j = 0; j < m; j++) {
                        int alloc_val = (i == thread_id) ? temp_alloc[j] : allocation[i][j];
                        work[j] += alloc_val;
                    }
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }
    }
    
    // Check if all threads can finish
    for (i = 0; i < n; i++) {
        if (!thread_exit[i] && !finish[i]) {
            return false;
        }
    }
    
    return true;
#else
    // Check if request is less than available and need
    for (int i = 0; i < m; i++) {
        if (request[i] > available[i] || request[i] > need[thread_id][i]) {
            return false;
        }
    }
    return true;
#endif
}

void process_pending_requests() {
    int queue_size = pending_queue.size;
    bool processed_at_least_one = false;
    
    for (int i = 0; i < queue_size; i++) {
        if (is_empty(&pending_queue)) break;
        
        Request front = queue_front(&pending_queue);
        dequeue(&pending_queue);
        
        int thread_id = front.thread_id;
        
        bool insufficient_resources = false;
        for (int j = 0; j < m; j++) {
            if (front.request[j] > available[j]) {
                insufficient_resources = true;
                break;
            }
        }
        
        bool unsafe_state = false;
        if (!insufficient_resources) {
#ifdef _DLAVOID
            if (!check_safety(thread_id, front.request)) {
                unsafe_state = true;
            }
#endif
        }
        
        bool can_grant = !insufficient_resources && !unsafe_state;
        
        pthread_mutex_lock(&pmtx);
        if (!can_grant) {
            if (insufficient_resources) {
                printf("    +++ Insufficient resources to grant request of thread %d\n", thread_id);
            } else if (unsafe_state) {
                printf("    +++ Unsafe to grant request of thread %d\n", thread_id);
            }
            enqueue(&pending_queue, front);
        } else {
            printf("Master thread grants resource request for thread %d\n", thread_id);
            for (int j = 0; j < m; j++) {
                available[j] -= front.request[j];
                allocation[thread_id][j] += front.request[j];
                need[thread_id][j] -= front.request[j];
            }
            
            pthread_mutex_lock(&cmtx[thread_id]);
            thread_sync[thread_id] = false;
            pthread_cond_signal(&cv[thread_id]);
            pthread_mutex_unlock(&cmtx[thread_id]);
            
            processed_at_least_one = true;
        }
        pthread_mutex_unlock(&pmtx);
    }
    
    pthread_mutex_lock(&pmtx);
    printf("\t\tWaiting threads: ");
    print_waiting_threads();
    printf("\n");
    pthread_mutex_unlock(&pmtx);
    
    if (processed_at_least_one && !is_empty(&pending_queue)) {
        process_pending_requests();
    }
}

void *thread_function(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);
    
    pthread_mutex_lock(&pmtx);
    printf("\tThread %d born\n", thread_id);
    pthread_mutex_unlock(&pmtx);
    
    pthread_barrier_wait(&bos);
    
    char filename[50];
    sprintf(filename, "input/thread%02d.txt", thread_id);
    
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening %s\n", filename);
        exit(1);
    }
    
    // Read maximum resource requirements
    for (int i = 0; i < m; i++) {
        fscanf(fp, "%d", &max_need[thread_id][i]);
        need[thread_id][i] = max_need[thread_id][i];
    }
    
    char line[256];
    // Skip first line (already read)
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        int delay;
        char request_type;
        sscanf(line, "%d %c", &delay, &request_type);
        
        usleep(delay * 10000); // Convert to microseconds
        
        if (request_type == 'Q') {
            // Prepare release request for all allocated resources
            int release_request[MAX_RESOURCES];
            for (int j = 0; j < m; j++) {
                release_request[j] = -allocation[thread_id][j];
            }
            
            pthread_mutex_lock(&rmtx);
            shared_request.thread_id = thread_id;
            shared_request.request_type = QUIT;
            for (int j = 0; j < m; j++) {
                shared_request.request[j] = release_request[j];
            }
            
            pthread_barrier_wait(&reqb);
            pthread_barrier_wait(&ackb[thread_id]);
            pthread_mutex_unlock(&rmtx);
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d going to quit\n", thread_id);
            pthread_mutex_unlock(&pmtx);
            
            break;
        } else if (request_type == 'R') {
            // Parse resource request
            int request[MAX_RESOURCES] = {0};
            char *token = strtok(line + 3, " \t\n");  // Skip delay and request_type
            for (int j = 0; j < m && token != NULL; j++) {
                request[j] = atoi(token);
                token = strtok(NULL, " \t\n");
            }
            
            bool is_release_only = true;
            for (int j = 0; j < m; j++) {
                if (request[j] > 0) {
                    is_release_only = false;
                    break;
                }
            }
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d sends resource request: type = %s\n", 
                    thread_id, (is_release_only ? "RELEASE" : "ADDITIONAL"));
            pthread_mutex_unlock(&pmtx);
            
            pthread_mutex_lock(&rmtx);
            shared_request.thread_id = thread_id;
            shared_request.request_type = is_release_only ? RELEASE : ADDITIONAL;
            for (int j = 0; j < m; j++) {
                shared_request.request[j] = request[j];
            }
            
            pthread_barrier_wait(&reqb);
            pthread_barrier_wait(&ackb[thread_id]);
            pthread_mutex_unlock(&rmtx);
            
            if (!is_release_only) {
                pthread_mutex_lock(&cmtx[thread_id]);
                if (thread_sync[thread_id]) {
                    pthread_cond_wait(&cv[thread_id], &cmtx[thread_id]);
                }
                thread_sync[thread_id] = true;
                pthread_mutex_unlock(&cmtx[thread_id]);
                
                pthread_mutex_lock(&pmtx);
                printf("\tThread %d is granted its last resource request\n", thread_id);
                pthread_mutex_unlock(&pmtx);
            } else {
                pthread_mutex_lock(&pmtx);
                printf("\tThread %d is done with its resource release request\n", thread_id);
                pthread_mutex_unlock(&pmtx);
            }
        }
    }
    
    fclose(fp);
    return NULL;
}

void *master_function(void *arg) {
    pthread_barrier_wait(&bos);
    
    while (true) {
        pthread_barrier_wait(&reqb);
        
        int thread_id = shared_request.thread_id;
        int request[MAX_RESOURCES];
        for (int i = 0; i < m; i++) {
            request[i] = shared_request.request[i];
        }
        
        pthread_barrier_wait(&ackb[thread_id]);
        
        bool is_quit = true;
        for (int i = 0; i < m; i++) {
            if (request[i] != -allocation[thread_id][i]) {
                is_quit = false;
                break;
            }
        }
        
        if (is_quit) {
            pthread_mutex_lock(&pmtx);
            for (int i = 0; i < m; i++) {
                available[i] += allocation[thread_id][i];
                allocation[thread_id][i] = 0;
            }
            
            thread_exit[thread_id] = true;
            num_exited++;
            
            printf("\t\tWaiting threads: ");
            print_waiting_threads();
            printf("\n");
            
            int active_thread_count = 0;
            printf("%d threads left: ", n - num_exited);
            
            for (int i = 0; i < n; i++) {
                if (!thread_exit[i]) {
                    if (active_thread_count > 0) printf(" ");
                    printf("%d", i);
                    active_thread_count++;
                }
            }
            printf("\n");
            
            printf("Available resources: ");
            for (int i = 0; i < m; i++) {
                printf("%d", available[i]);
                if (i < m - 1) printf(" ");
            }
            printf("\n");
            
            pthread_mutex_unlock(&pmtx);
            
            if (num_exited == n) {
                break;
            }
            
            process_pending_requests();
        } else {
            bool is_release_only = true;
            for (int i = 0; i < m; i++) {
                if (request[i] > 0) {
                    is_release_only = false;
                    break;
                }
            }
            
            pthread_mutex_lock(&pmtx);
            if (is_release_only) {
                printf("Master thread tries to grant pending requests\n");
            } else {
                printf("Master thread stores resource request of thread %d\n", thread_id);
            }
            pthread_mutex_unlock(&pmtx);
            
            for (int i = 0; i < m; i++) {
                if (request[i] < 0) {
                    available[i] += -request[i];
                    allocation[thread_id][i] += request[i];
                    need[thread_id][i] = max_need[thread_id][i] - allocation[thread_id][i];
                    if (!is_release_only) {
                        request[i] = 0;
                    }
                }
            }
            
            if (!is_release_only) {
                pthread_mutex_lock(&pmtx);
                printf("\t\tWaiting threads: ");
                print_waiting_threads();
                if (!is_empty(&pending_queue) || thread_id >= 0) {
                    if (!is_empty(&pending_queue)) printf(" ");
                    printf("%d", thread_id);
                }
                printf("\n");
                pthread_mutex_unlock(&pmtx);
                
                Request req;
                req.thread_id = thread_id;
                for (int i = 0; i < m; i++) {
                    req.request[i] = request[i];
                }
                enqueue(&pending_queue, req);
            }
            
            process_pending_requests();
        }
    }
    
    return NULL;
}

int main() {
    initialize_queue(&pending_queue);
    
    FILE *fp = fopen("input/system.txt", "r");
    if (fp == NULL) {
        printf("Error opening file input/system.txt\n");
        exit(1);
    }
    
    fscanf(fp, "%d %d", &m, &n);
    
    for (int i = 0; i < m; i++) {
        fscanf(fp, "%d", &available[i]);
    }
    
    fclose(fp);
    
    // Initialize arrays
    for (int i = 0; i < n; i++) {
        thread_exit[i] = false;
        thread_sync[i] = true;
        for (int j = 0; j < m; j++) {
            allocation[i][j] = 0;
            max_need[i][j] = 0;
            need[i][j] = 0;
        }
    }
    
    // Initialize synchronization primitives
    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_init(&pmtx, NULL);
    pthread_barrier_init(&bos, NULL, n + 1);
    pthread_barrier_init(&reqb, NULL, 2);
    
    for (int i = 0; i < n; i++) {
        pthread_barrier_init(&ackb[i], NULL, 2);
        pthread_cond_init(&cv[i], NULL);
        pthread_mutex_init(&cmtx[i], NULL);
    }
    
    pthread_t threads[n + 1];
    
    if (pthread_create(&threads[0], NULL, master_function, NULL) != 0) {
        printf("Error creating master thread\n");
        exit(1);
    }
    
    for (int i = 0; i < n; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        if (pthread_create(&threads[i + 1], NULL, thread_function, thread_id) != 0) {
            printf("Error creating thread %d\n", i);
            exit(1);
        }
    }
    
    for (int i = 1; i <= n; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_join(threads[0], NULL);
    
    // Clean up resources
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    pthread_barrier_destroy(&bos);
    pthread_barrier_destroy(&reqb);
    
    for (int i = 0; i < n; i++) {
        pthread_barrier_destroy(&ackb[i]);
        pthread_cond_destroy(&cv[i]);
        pthread_mutex_destroy(&cmtx[i]);
    }
    
    printf("All threads have finished\n");
    return 0;
}