/*
 * Program: sem1.3.c
 * Description:
 * - Waits for a signal from sem1.2.c (semaphore 1).
 * - Executes its task and signals sem1.4.c (semaphore 2).
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

    // Wait for semaphore 1 (from sem1.2.c)
    sem_lock(semid, 1);

    // Critical section
    printf("Program sem1.3: Executing task...\n");

    // Signal semaphore 2 (to sem1.4.c)
    sem_unlock(semid, 2);

    return 0;
}