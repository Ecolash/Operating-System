/*
Simple program to redirect the output of ls -l to a file
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int file_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) { perror("open"); return 1; }

    int cpid = fork();
    if (cpid < 0) { perror("fork"); return 1; }

    if (cpid == 0)
    {
        close(STDOUT_FILENO);
        dup(file_fd);
        // close(file_fd);

        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    close(file_fd);
    wait(NULL);
    return 0;
}
