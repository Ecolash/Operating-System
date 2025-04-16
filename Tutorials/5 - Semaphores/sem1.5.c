/*
 * Program: sem1.5.c
 * Description:
 * - Waits for a signal from sem1.4.c (semaphore 3).
 * - Executes its task and signals sem1.1.c (semaphore 4), completing the circular dependency.
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

    // Wait for semaphore 3 (from sem1.4.c)
    sem_lock(semid, 3);

    // Critical section
    printf("Program sem1.5: Executing task...\n");

    // Signal semaphore 4 (to sem1.1.c)
    sem_unlock(semid, 4);

    return 0;
}