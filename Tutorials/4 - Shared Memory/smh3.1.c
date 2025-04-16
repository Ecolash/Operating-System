/*
 * Description:
 
    - This program creates a shared memory segment containing two integer arrays (`a` and `b`).
    - It fills array `a` with random numbers and leaves array `b` empty for another process to use.
    - Synchronization is achieved using a semaphore to signal when the shared memory is ready.
    - The program writes random values to array `a`, signals the semaphore, 
    - It waits for the other process to sort array `a` and write the sorted version into array `b`.
    - After the sorting process is complete, the program reads and displays the contents of array `b`.
    - Finally, it cleans up the shared memory and semaphore resources before exiting.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>

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

int main()
{
    // Create shared memory segment
    int shmid = shmget(SHM_KEY, sizeof(int) * ARRAY_SIZE * 2, IPC_CREAT | 0666);
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

    // Create semaphore
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore to 0
    if (semctl(semid, 0, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    // Fill array `a` with random numbers
    srand(time(NULL));
    printf("Writer: Filling array a with random numbers...\n");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        shared_memory[i] = rand() % 100; // Random number between 0 and 99
        printf("a[%d] = %d\n", i, shared_memory[i]);
    }

    // Signal the semaphore to allow the sorter process to proceed
    sem_unlock(semid);

    // Detach shared memory
    if (shmdt(shared_memory) == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    printf("Writer: Done. Waiting for sorter to complete...\n");

    // Wait for sorter to complete (semaphore lock)
    sem_lock(semid);

    // Re-attach shared memory to read sorted array
    shared_memory = (int *)shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Writer: Sorted array b received:\n");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        printf("b[%d] = %d\n", i, shared_memory[ARRAY_SIZE + i]);
    }

    // Detach and remove shared memory
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

    // Remove semaphore
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    printf("Writer: Exiting.\n");
    return 0;
}