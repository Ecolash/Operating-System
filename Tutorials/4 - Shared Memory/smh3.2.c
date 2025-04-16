/*
 * Description:
 
    - This program attaches to the shared memory segment created by the `writer.c` program.
    - It waits for the writer to finish populating array `a` using a semaphore for synchronization.
    - Once the semaphore is signaled, it reads the elements of array `a`, 
    - It sorts them, and writes the sorted values into array `b` in the shared memory segment.
    - After completing its task, it signals the semaphore to notify the writer that sorting is complete.
    - The program detaches the shared memory segment and terminates after completing its work.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#define SHM_KEY 0x1234 // Shared memory key
#define SEM_KEY 0x5678 // Semaphore key
#define ARRAY_SIZE 10  // Size of arrays

// Semaphore operation functions
void sem_lock(int semid)
{
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

void sem_unlock(int semid)
{
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

// Comparison function for qsort
int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int main()
{
    // Access shared memory segment
    int shmid = shmget(SHM_KEY, sizeof(int) * ARRAY_SIZE * 2, 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    int *shared_memory = (int *)shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Access semaphore
    int semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Wait for writer to complete (semaphore lock)
    sem_lock(semid);

    printf("Sorter: Sorting array a...\n");

    // Sort array `a` and write the result to array `b`
    int *a = shared_memory;
    int *b = shared_memory + ARRAY_SIZE;

    qsort(a, ARRAY_SIZE, sizeof(int), compare);

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        b[i] = a[i];
        printf("b[%d] = %d\n", i, b[i]);
    }

    // Signal the semaphore to allow the writer to proceed
    sem_unlock(semid);

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    printf("Sorter: Done.\n");
    return 0;
}