#include <stdio.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int i;
    int x = 10;
    int pid1, pid2, status;
    printf("Before forking, the value of x is %d\n", x);

    if ((pid1 = fork()) == 0)
    {
        for (i = 0; i < 5; i++)
        {
            printf("[1] At first child: x = %d\n", x);
            x = x + 10;
            sleep(1);
        }
    }
    else
    {
        if ((pid2 = fork()) == 0)
        {
            for (i = 0; i < 5; i++)
            {
                printf("[2] At second child: x = %d\n", x);
                x = x + 20;
                sleep(1); 
            }
        }
        else
        {
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
            for (i = 0; i < 5; i++)
            {
                printf("[0] At parent: x = %d\n", x);
                x = x + 5;
                sleep(1); 
            }
        }
    }
}