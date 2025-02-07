/*
This program demonstrates signal handling in a parent and child process using the fork() system call.

- The parent process prints "P" every second.
- The child process prints "C" every second.
- First SIGINT (Ctrl+C) signal terminates the child process.
- Second SIGINT (Ctrl+C) signal terminates the parent process.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void secondint(int sig)
{
    printf("\n");
    wait(NULL);
    exit(1);
}

void firstint(int sig)
{
    signal(SIGINT, secondint);
    printf("\n");
}

int main()
{
    if (fork())
    {
        signal(SIGINT, firstint);
        while (1)
        {
            printf("P");
            fflush(stdout);
            sleep(1);
        }
    }
    else
    {
        while (1)
        {
            printf("C");
            fflush(stdout);
            sleep(1);
        }
    }
}