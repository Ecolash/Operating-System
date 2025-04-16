#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234 // Key for the shared memory segment

int main()
{
    // Access the shared memory segment
    int shmid = shmget(SHM_KEY, sizeof(int), 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment
    int *shared_memory = (int *)shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Read the value from shared memory
    printf("Child: Read value %d from shared memory.\n", *shared_memory);

    // Detach the shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    return 0;
}