
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

pid_t parent_pid;

void child_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        printf("Child: Work started after receiving SIGUSR1\n");
        sleep(2);
        printf("Child: Work done, sending SIGUSR2 back to parent...\n");
        kill(parent_pid, SIGUSR2);
    }
}

void parent_handler(int sig)
{
    if (sig == SIGUSR2)
    {
        printf("Parent: Received SIGUSR2 from child, processing the result...\n");
        exit(0);
    }
}

int main()
{
    parent_pid = getpid();
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0)
    {
        signal(SIGUSR1, child_handler);
        printf("Child: Waiting for SIGUSR1 from parent...\n");
        pause();
    }
    else
    {
        signal(SIGUSR2, parent_handler);
        printf("Parent: Waiting for child to set up signal handler...\n");
        sleep(1); // Add a delay to ensure the child is ready
        printf("Parent: Sending SIGUSR1 to child to start work...\n");
        kill(pid, SIGUSR1);
        pause();
    }

    return 0;
}