#include <sys/sem.h>

// Semaphore lock (P operation)
void sem_lock(int semid, int sem_index) {
    struct sembuf sb;
    sb.sem_num = sem_index;  // Index of the semaphore to lock
    sb.sem_op = -1;          // Decrement semaphore value
    sb.sem_flg = 0;         // No special flags
    semop(semid, &sb, 1);
}

// Semaphore unlock (V operation)
void sem_unlock(int semid, int sem_index) {
    struct sembuf sb;
    sb.sem_num = sem_index;  // Index of the semaphore to unlock
    sb.sem_op = 1;           // Increment semaphore value
    sb.sem_flg = 0;         // No special flags
    semop(semid, &sb, 1);
}