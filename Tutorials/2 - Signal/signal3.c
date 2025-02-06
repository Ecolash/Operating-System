#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void handler(int sig)
{
    printf("Child process received SIGTERM. Exiting...\n");
    exit(0);
}

int main()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0)
    {
        signal(SIGTERM, handler);
        printf("Child process waiting for signal...\n");
        while (1) { }
    }
    else
    {
        sleep(2);
        printf("Parent process sending SIGTERM to child (pid: %d)\n", pid);
        kill(pid, SIGTERM);
        wait(NULL);
        printf("Parent process exiting.\n");
    }
    return 0;
}