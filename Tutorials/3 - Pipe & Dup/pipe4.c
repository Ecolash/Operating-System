/*
EXTENSION OF PIPE3.C
-------------------------------------------------
Simulating pipe between any commands using execlp
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <command1> <arg1> <command2> <arg2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int cpid1 = fork();
    if (cpid1 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid1 == 0)
    {
        close(fd[0]);               
        dup2(fd[1], STDOUT_FILENO); 
        close(fd[1]);

        execlp(argv[1], argv[1], argv[2], NULL);
        perror("execlp failed for command1");
        exit(EXIT_FAILURE);
    }

    int cpid2 = fork();
    if (cpid2 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid2 == 0)
    {
        close(fd[1]);              
        dup2(fd[0], STDIN_FILENO); 
        close(fd[0]);

        execlp(argv[3], argv[3], argv[4], NULL);
        perror("execlp failed for command2");
        exit(EXIT_FAILURE);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    return 0;
}
