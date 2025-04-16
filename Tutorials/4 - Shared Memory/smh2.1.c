#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_KEY 0x1234 
int main()
{
    int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int *shared_memory = (int *)shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    *shared_memory = 42; 
    printf("Parent: Written value %d to shared memory.\n", *shared_memory);
    printf("Parent: Press Enter to exit and destroy shared memory\n");
    getchar();

    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    printf("Parent: Shared memory destroyed.\n");
    return 0;
}