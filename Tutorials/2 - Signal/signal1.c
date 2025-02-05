#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void childSigHandler(int sig)
{
    if (sig == SIGUSR1)
    {
        printf("\n+++ Child: Received signal SIGUSR1 from parent...\n");
        sleep(1);
    }
    else if (sig == SIGUSR2)
    {
        printf("\n+++ Child: Received signal SIGUSR2 from parent...\n");
        sleep(5);
    }
}

int main()
{
    int pid;
    srand((unsigned int)time(NULL));

    pid = fork(); 
    if (pid)
    { 
        int t;
        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);

        t = 1 + rand() % 2;
        printf("+++ Parent: Going to send signal SIGUSR%d to child\n", t);
        kill(pid, (t == 1) ? SIGUSR1 : SIGUSR2);

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
        printf("+++ Parent: Going to suspend child\n");
        kill(pid, SIGTSTP);

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
        printf("+++ Parent: Going to wake up child\n");
        kill(pid, SIGCONT);

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
        printf("+++ Parent: Going to wake up child\n");
        kill(pid, SIGCONT);

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
        printf("+++ Parent: Going to terminate child\n");
        kill(pid, SIGINT);

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
        waitpid(pid, NULL, 0);
        printf("+++ Parent: Child exited\n");

        t = 5 + rand() % 5;
        printf("\n+++ Parent: Going to sleep for %d seconds\n", t);
        sleep(t);
    }
    else
    {                                   
        signal(SIGUSR1, childSigHandler); 
        signal(SIGUSR2, childSigHandler); 
        while (1) sleep(1);
    }

    exit(0);
}
