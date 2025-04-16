/*
 * Program: init.c
 * Description:
 * - Initializes a semaphore array with 5 semaphores.
 * - Semaphore 0 is initialized to 1 to allow sem1.1.c to run first.
 * - All other semaphores are initialized to 0.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_KEY 0x1234  // Key for the semaphore array

int main() {
    // Create a semaphore array with 5 semaphores
    int semid = semget(SEM_KEY, 5, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    for (int i = 0; i < 5; i++) {
        if (semctl(semid, i, SETVAL, (i == 0) ? 1 : 0) == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }
    }

    printf("Semaphores initialized successfully.\n");
    return 0;
}