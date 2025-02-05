#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_FOLLOWERS   100
#define MAX_SUM_SIZE    1000

int HashTable[MAX_SUM_SIZE] = {0};

int main(int argc, char *argv[])
{
    int n;
    int shmid;
    int *M;

    srand((unsigned int)time(NULL));
    if (argc != 2) { fprintf(stderr, "Usage: %s <n> \n", argv[0]); return 1; }
    n = atoi(argv[1]);

    key_t key = ftok("/", 'T');
    shmid = shmget(key, (MAX_FOLLOWERS + 4) * sizeof(int), IPC_CREAT | IPC_EXCL | 0777);
    if (shmid == -1) {
        perror("[-] shmget failed");
        exit(1);
    }
    M = (int *)shmat(shmid, 0, 0);
    if (M == (void *)-1) {
        perror("[-] shmat failed");
        exit(1);
    }

    M[0] = n;
    M[1] = 0;
    M[2] = 0;
    printf("[+] Waiting for followers to join...\n");
    while (M[1] < n) { };

    printf("[+] All followers have joined. Starting computation...\n\n");

    while(1)
    {
        int x = rand() % 99 + 1;
        M[3] = x;
        M[2] = 1;

        int sum = M[3];
        while(M[2] != 0) { };
        for (int i = 1; i <= n; i++) sum += M[3 + i];

        printf("%2d ", M[3]);
        for (int i = 1; i <= n; i++) printf ("+ %d ", M[3 + i]);
        printf(" = %3d\n", sum);

        if (HashTable[sum] == 1) break;
        HashTable[sum] = 1;
    }
    M[2] = -1;
    while (M[2] != 0) { };

    if (shmdt(M) == -1)
    {
        perror("[-] shmdt failed");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("[-] shmctl failed");
        exit(1);
    }
    printf("\n");
    printf("[+] Shared memory detached and removed.\n");
    printf("[+] Terminating leader...\n");
    return 0;
}