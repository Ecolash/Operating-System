#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_NO_CLD 10

int main()
{
    int i, ncld, wtime, status;
    pid_t cpid, mypid, parpid;

    mypid = getpid();
    parpid = getppid();
    printf("Parent: PID = %u, PPID = %u\n", mypid, parpid);
    printf("Parent: Number of children = ");
    scanf("%d", &ncld);

    ncld = (ncld <= 0) ? 1 : ncld;
    ncld = (ncld > MAX_NO_CLD) ? MAX_NO_CLD : ncld;
    printf("\n");

    for (i = 0; i < ncld; ++i)
    {
        cpid = fork();
        if (cpid == 0)
        {
            mypid = getpid();
            parpid = getppid();
            srand(mypid);
            wtime = 1 + rand() % 10;
            printf("Child %d: PID = %u | PPID = %u | Time = %d sec\n", i, mypid, parpid, wtime);

            sleep(wtime);
            printf("\nChild %d: Work done...\n", i);
            exit(i);
        }
    }

    for (i = 0; i < ncld; ++i)
    {
        wait(&status);
        printf("Parent: Child %d terminates...\n", WEXITSTATUS(status));
    }

    printf("\nParent terminates...\n");
    exit(0);
}
