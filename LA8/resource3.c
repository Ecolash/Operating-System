#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define INPUT_PATH "Sample/input1"
#define MAX_RESOURCES 20
#define MAX_THREADS 100
#define ADDITIONAL 1
#define RELEASE 2
#define QUIT 3

// //Barrier implementation
// typedef struct {
//     pthread_mutex_t mutex;
//     pthread_cond_t cond;
//     int count;
//     int trip_count;
// } pthread_barrier_t;

// void pthread_barrier_init(pthread_barrier_t *barrier, void *attr, int count) {
//     pthread_mutex_init(&barrier->mutex, NULL);
//     pthread_cond_init(&barrier->cond, NULL);
//     barrier->count = count;
//     barrier->trip_count = 0;
// }

// void pthread_barrier_wait(pthread_barrier_t *barrier) {
//     pthread_mutex_lock(&barrier->mutex);
    
//     barrier->trip_count++;
    
//     if (barrier->trip_count >= barrier->count) {
//         barrier->trip_count = 0;
//         pthread_cond_broadcast(&barrier->cond);
//     } else {
//         pthread_cond_wait(&barrier->cond, &barrier->mutex);
//     }

//     pthread_mutex_unlock(&barrier->mutex);
// }

// void pthread_barrier_destroy(pthread_barrier_t *barrier) {
//     pthread_mutex_destroy(&barrier->mutex);
//     pthread_cond_destroy(&barrier->cond);
// }
// //Barrier implementation ends

// Queue implementation starts
typedef struct Node {
    int thread_id;
    int req[MAX_RESOURCES];
    struct Node* next;
} Node;

// Queue structure
typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue* q, int thread_id, int req[MAX_RESOURCES]) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->thread_id = thread_id;
    for (int i = 0; i < MAX_RESOURCES; i++) {
        newNode->req[i] = req[i];
    }
    newNode->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
        return;
    }
    q->rear->next = newNode;
    q->rear = newNode;
}

Node* dequeue(Queue* q) {
    if (q->front == NULL) {
        printf("Queue is empty!\n");
        return NULL;
    }
    Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL)
        q->rear = NULL;
    return temp;
}
// Queue implementation ends

// Resource Structure
typedef struct Resource{
  int type;
  int thread_id;
  int req[MAX_RESOURCES];
} Resource;


// int available[MAXRESOURCES];
// int max_need[MAXTHREADS][MAXRESOURCES];
// int need[MAXTHREADS][MAXRESOURCES];
// int allocation[MAXTHREADS][MAXRESOURCES];
Resource request;
int m,n,*status,active_threads,*active;

pthread_mutex_t rmtx,pmtx;
pthread_barrier_t REQB,BOS;
pthread_barrier_t *ACKB;
pthread_cond_t *cv;
pthread_mutex_t *cmtx;

int is_safe(int available[],int need[][m],int allocation[][m]){
        int finish[n],work[m];
    for(int i=0;i<n;i++){
        finish[i]=(active[i]==0)?1:0;
    }
    for(int i=0;i<m;i++){
        work[i]=available[i];
    }
    int count=0;
    while(count<active_threads){
        int found=0;
        for(int i=0;i<n;i++){
            if(!finish[i]){
                int can_allocate=1;
                for(int j=0;j<m;j++){
                    if(need[i][j]>work[j]){
                        can_allocate=0;
                        break;
                    }
                }
                if(can_allocate){
                    for(int j=0;j<m;j++){
                        work[j]+=allocation[i][j];
                    }
                    finish[i]=1;
                    found=1;
                    count++;
                }
            }
        }
        if(!found){
            return 0;
        }
    }
    return 1;
}

int bankers_algo(int requests[],int thread_id,int available[],int need[][m],int allocation[][m]){
    int tmp_need[n][m],tmp_allocation[n][m];
    int tmp_available[m];
    for(int i=0;i<m;i++){
        tmp_available[i]=available[i]-requests[i];
    }
    for(int i=0;i<n;i++){
        for(int j=0;j<m;j++){
            tmp_need[i][j]=need[i][j];
            tmp_allocation[i][j]=allocation[i][j];
        }
    }
    for(int i=0;i<m;i++){
        tmp_need[thread_id][i]-=requests[i];
        tmp_allocation[thread_id][i]+=requests[i];
    }
    if(!is_safe(tmp_available,tmp_need,tmp_allocation)){
        return 1;
    }
    return 0;
}

void *thread_func(void* arg){
    long thread_id=(long)arg;
    pthread_mutex_lock(&pmtx);
    printf("\tThread %ld born\n",thread_id);
    pthread_mutex_unlock(&pmtx);
    pthread_barrier_wait(&BOS);
    int need[m];
    char filename[100];
    if(thread_id<10){
        sprintf(filename,INPUT_PATH "/thread0%ld.txt", thread_id);
    }else{
        sprintf(filename,INPUT_PATH "/thread%ld.txt", thread_id);
    }
    FILE *fp=fopen(filename,"r");
    if(fp==NULL){
        printf("File open error\n");
        exit(1);
    }
    for(int i=0;i<m;i++){
        fscanf(fp,"%d",&need[i]);
    }
    int allocation[m];
    for(int i=0;i<m;i++){
        allocation[i]=0;
    }
    int delay;
    char type;
    while(1){
        fscanf(fp,"%d",&delay);
        usleep(delay*1000);
        fscanf(fp," %c",&type);
        if(type=='Q'){
            pthread_mutex_lock(&rmtx);
            request.type=QUIT;
            request.thread_id=thread_id;
            for(int i=0;i<m;i++){
                request.req[i]=(-1)*allocation[i];
            }
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACKB[thread_id]);
            pthread_mutex_unlock(&rmtx);

            pthread_mutex_lock(&pmtx);
            printf("\tThread %ld going to quit\n",thread_id);
            pthread_mutex_unlock(&pmtx);
            break;
        }
        int requests[m];
        int req_type=RELEASE;
        for(int i=0;i<m;i++){
            fscanf(fp,"%d",&requests[i]);
            if(requests[i]>0){
                req_type=ADDITIONAL;
            }
        }

        pthread_mutex_lock(&pmtx);
        printf("\tThread %ld sends resource request: type = %s\n",thread_id,req_type==ADDITIONAL?"ADDITIONAL":"RELEASE");
        pthread_mutex_unlock(&pmtx);

        pthread_mutex_lock(&rmtx);
        for(int i=0;i<m;i++){
            request.req[i]=requests[i];
        }
        request.type=req_type;
        request.thread_id=thread_id;
        pthread_barrier_wait(&REQB);
        pthread_barrier_wait(&ACKB[thread_id]);
        pthread_mutex_unlock(&rmtx);

        if(req_type==ADDITIONAL){
            if(status[thread_id]==-1)status[thread_id]=1;
            pthread_mutex_lock(&cmtx[thread_id]);
            while(status[thread_id]==1)pthread_cond_wait(&cv[thread_id],&cmtx[thread_id]);
            pthread_mutex_unlock(&cmtx[thread_id]);
            status[thread_id]=-1;

            pthread_mutex_lock(&pmtx);
            printf("\tThread %ld is granted its last resource request\n", thread_id);
            pthread_mutex_unlock(&pmtx);
        }else{
            pthread_mutex_lock(&pmtx);
            printf("\tThread %ld is done with its resource release request\n", thread_id);
            pthread_mutex_unlock(&pmtx);
        }
        for(int i=0;i<m;i++){
            allocation[i]+=requests[i];
        }
    }
    fclose(fp);
    return NULL;
}

int main(){
  FILE *fp;
  fp=fopen(INPUT_PATH "/system.txt","r");
  if(fp==NULL){
    printf("File open error\n");
    exit(1);
  }
  fscanf(fp,"%d %d",&m,&n);
  int available[m];
  for(int i=0;i<m;i++){
    fscanf(fp,"%d",&available[i]);
  }
  fclose(fp);
  int max_need[n][m];
  for(int i=0;i<n;i++){
    char filename[100];
    if(i<10){
        sprintf(filename,INPUT_PATH "/thread0%d.txt", i);
    }else{
        sprintf(filename,INPUT_PATH "/thread%d.txt", i);
    }
    FILE *fp=fopen(filename,"r");
    if(fp==NULL){
        printf("File open error\n");
        exit(1);
    }
    for(int j=0;j<m;j++){
        fscanf(fp,"%d",&max_need[i][j]);
    }
  }
  Queue* Q=createQueue();
  ACKB=(pthread_barrier_t *)malloc(n*sizeof(pthread_barrier_t));
  cv=(pthread_cond_t *)malloc(n*sizeof(pthread_cond_t));
  cmtx=(pthread_mutex_t *)malloc(n*sizeof(pthread_mutex_t));
  status=(int*)malloc(n*sizeof(int));
  for(int i=0;i<n;i++){
    status[i]=-1;
  }
  pthread_barrier_init(&BOS,NULL,n+1);
  pthread_mutex_init(&rmtx,NULL);
  pthread_mutex_init(&pmtx,NULL);
  pthread_barrier_init(&REQB,NULL,2);
  for(int i=0;i<n;i++){
    pthread_barrier_init(&ACKB[i],NULL,2);
    pthread_cond_init(&cv[i],NULL);
    pthread_mutex_init(&cmtx[i],NULL);
  }
  pthread_t threads[n];


  for(long i=0;i<n;i++){
    pthread_create(&threads[i],NULL,thread_func,(void *)i);
  }
  pthread_barrier_wait(&BOS);
  //printf("All threads are born\n");

  int need[n][m], allocation[n][m];;
  for(int i=0;i<n;i++){
    for(int j=0;j<m;j++){
        need[i][j]=max_need[i][j];
        allocation[i][j]=0;
    }
  }

  int *waiting = (int *)malloc(n * sizeof(int));
  for (int i = 0; i < n; i++) {
      waiting[i] = 0;
  }
  active_threads = n;
  active=(int *)malloc(n*sizeof(int));
  for(int i=0;i<n;i++){
    active[i]=1;
}
  while(1){
    pthread_barrier_wait(&REQB);
    int req_type=request.type;
    int thread_id=request.thread_id;
    int requests[MAX_RESOURCES]={0};
    for(int i=0;i<m;i++){
        requests[i]=request.req[i];
    }
    pthread_barrier_wait(&ACKB[thread_id]);
    
    if(req_type==RELEASE){
        pthread_mutex_lock(&pmtx);
        printf("Master thread stores resource request of thread %d\n",thread_id);
        pthread_mutex_unlock(&pmtx);
        for(int i=0;i<m;i++){
            available[i]+=(-1)*requests[i];
            allocation[thread_id][i]+=requests[i];
            need[thread_id][i]=max_need[thread_id][i]-allocation[thread_id][i];
        }
    }
    else if(req_type==ADDITIONAL){
        pthread_mutex_lock(&pmtx);
        printf("Master thread stores resource request of thread %d\n",thread_id);
        pthread_mutex_unlock(&pmtx);
        for(int i=0;i<m;i++){
            if(requests[i]<=0){
                available[i]+=(-1)*requests[i];
                allocation[thread_id][i]+=requests[i];
                need[thread_id][i]=max_need[thread_id][i]-allocation[thread_id][i];
                requests[i]=0;
            }
        }
        enqueue(Q,thread_id,requests);
        waiting[thread_id]=1;
    }
    else if(req_type==QUIT){
        active[thread_id]=0;
        active_threads--;
        for(int i=0;i<m;i++){
            available[i]+=allocation[thread_id][i];
            allocation[thread_id][i]=0;
        }
        
        pthread_mutex_lock(&pmtx);
        printf("%d threads left: ",active_threads);
        for(int i=0;i<n;i++){
            if(active[i]==1){
                printf("%d ",i);
            }
        }
        printf("\n");
        printf("Available resources: ");
        for(int i=0;i<m;i++){
            printf("%d ",available[i]);
        }
        printf("\n");
        pthread_mutex_unlock(&pmtx);

        if(active_threads==0){
            break;
        }
    }

    int waiting_count=0;
    pthread_mutex_lock(&pmtx);
    printf("\t\tWaiting threads: ");
    for(int i=0;i<n;i++){
        if(waiting[i]==1){
            waiting_count++;
            printf("%d ",i);
        }
    }
    printf("\n");
    printf("Master thread tries to grant pending requests\n");
    pthread_mutex_unlock(&pmtx);

    while(waiting_count--){
        Node* node=dequeue(Q);
        int thread_id=node->thread_id;
        int* requests=node->req;
        int flag=1;
        for(int i=0;i<m;i++){
            if(requests[i]>available[i]){
                flag=0;
                break;
            }
        }
        if(flag){
            int unsafe_state=0;
            #ifdef _DLAVOID
            unsafe_state=bankers_algo(requests,thread_id,available,need,allocation);
            #endif
            if(unsafe_state){
                pthread_mutex_lock(&pmtx);
                printf("\t+++ Unsafe to grant resource request of thread %d\n",thread_id);
                pthread_mutex_unlock(&pmtx);
                enqueue(Q,thread_id,requests);
                continue;
            }
            pthread_mutex_lock(&pmtx);
            printf("Master thread grants resource request of thread %d\n",thread_id);
            pthread_mutex_unlock(&pmtx);
            for(int i=0;i<m;i++){
                available[i]-=requests[i];
                need[thread_id][i]-=requests[i];
                allocation[thread_id][i]+=requests[i];
            }
            waiting[thread_id]=0;
            status[thread_id]=0;
            pthread_mutex_lock(&cmtx[thread_id]);
            pthread_cond_signal(&cv[thread_id]);
            pthread_mutex_unlock(&cmtx[thread_id]);
        }else{
            pthread_mutex_lock(&pmtx);
            printf("\t+++ Insufficient resources to grant request of thread %d\n",thread_id);
            pthread_mutex_unlock(&pmtx);
            enqueue(Q,thread_id,requests);
        }
    }

    pthread_mutex_lock(&pmtx);
    // printf("Available resources: ");
    //     for(int i=0;i<m;i++){
    //         printf("%d ",available[i]);
    //     }
    //     printf("\n");
    printf("\t\tWaiting threads: ");
    for(int i=0;i<n;i++){
        if(waiting[i]==1){
            printf("%d ",i);
        }
    }
    printf("\n");
    pthread_mutex_unlock(&pmtx);
  }

  //Destroying all the barriers and mutexes
    pthread_barrier_destroy(&BOS);
    pthread_barrier_destroy(&REQB);
    for (int i = 0; i < n; i++) {
        pthread_barrier_destroy(&ACKB[i]);
        pthread_cond_destroy(&cv[i]);
        pthread_mutex_destroy(&cmtx[i]);
    }
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    free(ACKB);
    free(cv);
    free(cmtx);
    free(waiting);
    free(active);
  return 0;
}

