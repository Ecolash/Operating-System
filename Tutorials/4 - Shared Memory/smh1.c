#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>

#define SHM_KEY 0x1234 

int main()
{
    int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    int *sh_int = (int *)shmat(shmid, NULL, 0);
    if (sh_int == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        while (1)
        {
            int x = *sh_int;
            printf("Reader: Shared memory value = %d\n", x);
            if (x == 9) break;
            sleep(1);
        }
        printf("Reader: Exiting...\n");
        if (shmdt(sh_int) == -1)
        {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            *sh_int = i; 
            printf("Writer: Updated shared memory to %d\n", i);
            sleep(2);
        }

        // Detach shared memory in parent
        if (shmdt(sh_int) == -1)
        {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }

        // Remove shared memory segment
        if (shmctl(shmid, IPC_RMID, NULL) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}