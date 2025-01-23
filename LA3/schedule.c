#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF               1e9
#define MAX_BURSTS        1024
#define FILENAME          "proc.txt"

#define NEW_ARRIVAL       100000
#define IO_COMPLETE       100001
#define TERMINATION       100002
#define TIMEOUT           100003
#define CPU_BURST         100004

/*
----------------------------------------------------------------------------------------------------------
Process Control Block (PCB) Structure:
==========================================================================================================
- id:                 Process ID
- burst_count:        Number of bursts in the process
- bursts:             Array of burst times
- burst_types:        Array of burst types (CPU or I/O)

- current_burst:      Index of the current burst
- burst_remaining:    Remaining time of the current burst
- state:              State of the process

- arrival_time:       Arrival time of the process
- event_time:         Time of the next CPU event
- turnaround_time:    Turnaround time of the process
- waiting_time:       Waiting time of the process
- running_time:       Total running time of the process (sum of all burst times)
==========================================================================================================
*/

typedef struct
{
    int id;                       
    int burst_count;              
    int bursts[MAX_BURSTS];       
    char burst_types[MAX_BURSTS]; 

    int current_burst;            
    int burst_remaining;      
    int state;                    

    int arrival_time;    
    int event_time;
    int turnaround_time;
    int waiting_time;
    int running_time;        
} Process;

Process *processes;
int process_count;

//=========================================================================================================
// QUEUE IMPLEMENTATION
//=========================================================================================================

typedef struct {
    int *items;
    int front, rear, size, capacity;
} Queue;

Queue *initQ(int capacity) {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->items = (int *)malloc(capacity * sizeof(int));
    q->capacity = capacity;
    q->size = 0;
    q->front = -1;
    q->rear = -1;
    return q;
}

int isFull(Queue *q)  { return q->size == q->capacity; }
int isEmpty(Queue *q) { return q->size == 0; }
void freeQ(Queue *q)  { free(q->items); }

void enqueue(Queue *q, int value) {
    if (isFull(q)) { fprintf(stderr, "Queue is full! Cannot enqueue %d.\n", value); exit(EXIT_FAILURE); }
    if (isEmpty(q)) q->front = 0;
    q->rear = (q->rear + 1) % q->capacity;
    q->items[q->rear] = value;
    q->size++;
}

int dequeue(Queue *q) {
    if (isEmpty(q)) { fprintf(stderr, "Queue is empty! Cannot dequeue.\n"); exit(EXIT_FAILURE); }
    int x = q->items[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    if (q->size == 0) q->front = q->rear = -1;
    return x;
}

void displayQueue(Queue *q) {
    printf("Queue elements: ");
    if (isEmpty(q)) return;
    int i = q->front;
    for (int count = 0; count < q->size; count++) 
    {
        printf("%d ", q->items[i]);
        i = (i + 1) % q->capacity;
    }
    printf("\n");
}

//=========================================================================================================
// MIN HEAP IMPLEMENTATION
//=========================================================================================================

typedef struct {
    int *data;
    int size;
    int capacity;
} MinHeap;

MinHeap *initH(int capacity) {
    MinHeap *heap;
    heap = (MinHeap *)malloc(sizeof(MinHeap));
    heap->data = (int *)malloc(capacity * sizeof(int));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int compare(int a, int b) {
    Process p1 = processes[a];
    Process p2 = processes[b];
    if (p1.event_time != p2.event_time) return p1.event_time - p2.event_time;

    if (p1.state == p2.state) return p1.id - p2.id;
    if (p1.state == NEW_ARRIVAL || p1.state == IO_COMPLETE) return -1;
    if (p2.state == NEW_ARRIVAL || p2.state == IO_COMPLETE) return 1;
    if (p1.state == TIMEOUT) return -1;
    if (p2.state == TIMEOUT) return 1;
    return 1;
}

void heapifyUp(MinHeap *heap, int index) {
    int parent = (index - 1) / 2;
    while (index > 0 && compare(heap->data[index], heap->data[parent]) < 0) {
        swap(&heap->data[index], &heap->data[parent]);
        index = parent;
        parent = (index - 1) / 2;
    }
}

void heapifyDown(MinHeap *heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && compare(heap->data[left], heap->data[smallest]) < 0)  smallest = left;
    if (right < heap->size && compare(heap->data[right], heap->data[smallest]) < 0) smallest = right;
    if (smallest != index) {
        swap(&heap->data[index], &heap->data[smallest]);
        heapifyDown(heap, smallest);
    }
}

void insert(MinHeap *heap, int value) {
    if (heap->size == heap->capacity) { fprintf(stderr, "Heap is full! Cannot insert %d.\n", value); exit(EXIT_FAILURE); }
    heap->data[heap->size] = value;
    heap->size++;
    heapifyUp(heap, heap->size - 1);
}

int extractMin(MinHeap *heap) {
    if (heap->size == 0) { fprintf(stderr, "Heap is empty! Cannot extract minimum value.\n"); exit(EXIT_FAILURE); }
    int min = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heapifyDown(heap, 0);
    return min;
}

//=========================================================================================================


Process* getProcs(const char *filename, int *process_count) {
    FILE *file = fopen(filename, "r");
    if (!file) exit(EXIT_FAILURE); 
    fscanf(file, "%d", process_count);

    Process *processes = (Process *)malloc((*process_count) * sizeof(Process));
    if (!processes) exit(EXIT_FAILURE); 

    for (int i = 0; i < *process_count; i++) {
        Process *p = &processes[i];
        fscanf(file, "%d %d", &p->id, &p->arrival_time);
        p->running_time = 0;

        int burst;
        char type = 'C'; 
        while (1) {
            fscanf(file, "%d", &burst);
            if (burst == -1) break;
            p->running_time += burst;
            p->bursts[p->burst_count] = burst;
            p->burst_types[p->burst_count] = type;
            p->burst_count++;
            type = (type == 'C') ? 'I' : 'C'; 
        }
    }

    fclose(file);
    return processes;
}

void printProcesses(const Process processes[], int process_count) {
    for (int i = 0; i < process_count; i++) {
        const Process *p = &processes[i];
        printf("PID: %d\n", p->id);
        printf("Arrival Time: %d\n", p->arrival_time);
        printf("n(Burst): %d\n", p->burst_count);
        printf("Burst Details:\n");
        for (int j = 0; j < p->burst_count; j++) printf(" >> %3d: %d (%c)\n", j + 1, p->bursts[j], p->burst_types[j]);
        printf("\n");
    }
}

void resetproc()
{
    for (int i = 0; i < process_count; i++)
    {
        processes[i].state = NEW_ARRIVAL;
        processes[i].event_time = processes[i].arrival_time;
        processes[i].burst_remaining = processes[i].bursts[0];
        processes[i].current_burst = 0;
        processes[i].turnaround_time = 0;
        processes[i].waiting_time = 0;
    }
}

void print_stats(Process *p, int time)
{
    printf("%-6d: ", time);
    printf("Process %-2d exits. ", p->id);
    double pctime = (double)p->turnaround_time / (double)p->running_time * 100.0;
    printf("Turnaround time: %-4d (%.2f %%), ", p->turnaround_time, pctime);
    printf("Wait time: %d\n", p->waiting_time);
}

void simulate(int time_quantum)
{
    int CURR_RUNNING_PROC = -1;
    int CPU_IDLE_START = 0;
    int CPU_IDLE_TIME = 0;
    int CPU_IDLE = 0;
    int TIME = 0;

    if (time_quantum == INF) printf("\n----------------------------- FCFS Scheduling -----------------------------\n");
    else printf("\n------------------------ RR Scheduling with q = %d ------------------------\n", time_quantum);

    MinHeap *EVENT_QUEUE = initH(process_count);
    Queue *READY_QUEUE = initQ(process_count);

    resetproc();
    for (int i = 0; i < process_count; i++) insert(EVENT_QUEUE, i);
    #ifdef VERBOSE
    printf("%-6d: Starting...\n", TIME);
    #endif

    while (EVENT_QUEUE->size > 0)
    {
        int curr = extractMin(EVENT_QUEUE);
        Process *p = &processes[curr];
        TIME = p->event_time;

        if (CURR_RUNNING_PROC == -1 && CPU_IDLE == 1)
        {
            CPU_IDLE_TIME += TIME - CPU_IDLE_START;
            CPU_IDLE_START = TIME;
        }

        switch (p->state) {
            case NEW_ARRIVAL:
                #ifdef VERBOSE
                printf("%-6d: ", TIME);
                printf("Process %d joins ready queue upon arrival\n", p->id);
                #endif 
                enqueue(READY_QUEUE, curr);
                break;

            case IO_COMPLETE:
                #ifdef VERBOSE
                printf("%-6d: ", TIME);
                printf("Process %d joins ready queue after IO completion\n", p->id);
                #endif
                p->burst_remaining = p->bursts[p->current_burst];
                enqueue(READY_QUEUE, curr);
                break;

            case TIMEOUT:
                #ifdef VERBOSE
                printf("%-6d: ", TIME);
                printf("Process %d joins ready queue after timeout\n", p->id);
                #endif 
                CURR_RUNNING_PROC = -1;
                enqueue(READY_QUEUE, curr);
                break;

            case CPU_BURST:
                CURR_RUNNING_PROC = -1;
                p->current_burst = p->current_burst + 2;
                if (p->current_burst < p->burst_count)
                {
                    p->state = IO_COMPLETE;
                    p->event_time = TIME + p->bursts[p->current_burst - 1];
                    insert(EVENT_QUEUE, curr);
                }
                else
                {
                    p->state = TERMINATION;
                    p->event_time = TIME;
                    p->turnaround_time = TIME - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->running_time;
                    print_stats(p, TIME);
                }
                break;

            default: break;
        }

        if (CURR_RUNNING_PROC == -1)
        {
            if (!isEmpty(READY_QUEUE))
            {
                CURR_RUNNING_PROC = dequeue(READY_QUEUE);
                Process *p = &processes[CURR_RUNNING_PROC];
                int T1 = p->burst_remaining;
                int T2 = time_quantum;

                int T = (T1 < T2) ? T1 : T2;
                if (CPU_IDLE == 1)
                {
                    CPU_IDLE = 0;
                    CPU_IDLE_TIME += TIME - CPU_IDLE_START;
                }
                #ifdef VERBOSE
                printf("%-6d: ", TIME);
                printf("Process %d is scheduled to run for time %d\n", p->id, T);
                #endif
                p->event_time = TIME + T;

                p->state = (T == T1)? CPU_BURST : TIMEOUT;
                switch(p->state)
                {
                    case CPU_BURST:
                        p->burst_remaining = 0;
                        insert(EVENT_QUEUE, CURR_RUNNING_PROC);
                        break;

                    case TIMEOUT:
                        p->burst_remaining -= T;
                        insert(EVENT_QUEUE, CURR_RUNNING_PROC);
                        break;

                    default: break;
                }
            }

            else
            {
                if (CPU_IDLE == 0)
                {
                    CPU_IDLE = 1;
                    CPU_IDLE_START = TIME;
                    if (TIME > 0) {
                        #ifdef VERBOSE
                        printf("%-6d: ", TIME);
                        printf("CPU goes idle\n");
                        #endif
                    }
                }
            }
        }
    }

    if (CPU_IDLE == 1) CPU_IDLE_TIME += TIME - CPU_IDLE_START;

    double TOTAL_WAITING_TIME = 0.0;
    double AVG_WAITING_TIME = 0.0;
    double TOTAL_TURNAROUND_TIME = (double)TIME;
    double CPU_UTILIZATION = 0.0;

    for (int i = 0; i < process_count; i++) TOTAL_WAITING_TIME += processes[i].waiting_time;
    AVG_WAITING_TIME = TOTAL_WAITING_TIME / process_count;
    CPU_UTILIZATION = (TIME - CPU_IDLE_TIME) / (double)TIME * 100.0;

    printf("\n");
    printf("[+] Average wait time: %.2f\n", AVG_WAITING_TIME);
    printf("[+] Total Turnaround time: %.2f\n", TOTAL_TURNAROUND_TIME);
    printf("[+] CPU idle time: %.2f\n", (double)CPU_IDLE_TIME);
    printf("[+] CPU Utilization: %.2f%%\n", CPU_UTILIZATION);
    printf("\n");
}

int main() {
    process_count = 0;
    processes = getProcs(FILENAME, &process_count);
    // printProcesses(processes, process_count);

    simulate(INF);   // FCFS 
    simulate(10);    // RR (TS = 10)
    simulate(5);     // RR (TS = 5)

    free(processes);
    return 0;
}