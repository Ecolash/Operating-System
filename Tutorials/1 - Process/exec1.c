#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        execl("/bin/date", "date", NULL);
        exit(1);
    }
    wait(NULL);

    pid = fork();
    if (pid == 0)
    {
        execlp("whoami", "whoami", NULL);
        exit(1);
    }
    wait(NULL);

    pid = fork();
    if (pid == 0)
    {
        char *args[] = {"uptime", NULL};
        execv("/usr/bin/uptime", args);
        exit(1);
    }
    wait(NULL);
    return 0;
}
