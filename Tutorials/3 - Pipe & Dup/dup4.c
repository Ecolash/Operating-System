/*
Write a C program that:
    - Enters a loop.
    - Prints the prompt `$` to the screen.
    - Reads user inputs.

        If the user enters `s`: Print a fortune to the screen.
        If the user enters `f`: Print a fortune to the file `fortunes.txt`.
        If the user enters anything else: Break the loop and terminate the program.

-- Use an `exec` function to run `fortune` without any command-line argument.
-- Use `dup` for suitable redirections of `stdout`.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
    int stdout = dup(STDOUT_FILENO);
    FILE *file = fopen("fortunes.txt", "w");
    int file_fd = fileno(file);

    while(1)
    {
        close(STDOUT_FILENO);
        dup(stdout);
        printf("$ ");
        char c;
        scanf(" %c", &c);
        close(STDOUT_FILENO);
        if (c == 's')
        {
            dup(stdout);
            if (fork() == 0) execlp("fortune", "fortune", (char *)NULL);
            wait(NULL);
        }
        else if (c == 'f')
        {
            dup(file_fd);
            if (fork() == 0) execlp("fortune", "fortune", (char *)NULL);
            wait(NULL);

        }
        else break;
    }
}