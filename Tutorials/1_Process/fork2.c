#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int n, mypid, parpid;
    printf("Parent: n = ");
    scanf("%d", &n);

    if (fork())
    { /* Parent process */
        mypid = getpid();
        parpid = getppid();
        printf("Parent: PID = %u, PPID = %u...\n", mypid, parpid);
    }
    else
    { /* Child process */
        mypid = getpid();
        parpid = getppid();
        printf("Child : PID = %u, PPID = %u...\n", mypid, parpid);
        n = n * n;
    }

    printf("Process PID = %u: n = %d\n", mypid, n);
    exit(0);
}