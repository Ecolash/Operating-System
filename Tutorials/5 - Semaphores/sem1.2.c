/*
 * Program: sem1.2.c
 * Description:
 * - Waits for a signal from sem1.1.c (semaphore 0).
 * - Executes its task and signals sem1.3.c (semaphore 1).
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "sem.h"

#define SEM_KEY 0x1234  // Key for the semaphore array

int main() {
    // Access the semaphore array
    int semid = semget(SEM_KEY, 5, 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Wait for semaphore 0 (from sem1.1.c)
    sem_lock(semid, 0);

    // Critical section
    printf("Program sem1.2: Executing task...\n");

    // Signal semaphore 1 (to sem1.3.c)
    sem_unlock(semid, 1);

    return 0;
}