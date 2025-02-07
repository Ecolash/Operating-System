#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

void parent_process(int pid)
{
    printf("[+] This is the parent process  (PID: %d, Child PID:  %d)\n", getpid(), pid);
    for (int i = 0; i < 100; i++) if((i + 1) % 10 == 0) printf("Parent process [%3d / 100]\n", i + 1);
    return;
}

void child_process(int pid)
{
    printf("[+] This is the child process   (PID: %d, Parent PID: %d)\n", getpid(), getppid());
    for (int i = 0; i < 100; i++) if((i + 1) % 10 == 0) printf("Child process  [%3d / 100]\n", i + 1);
    return;
}

int main()
{
    pid_t pid = fork(); 
    switch (pid)
    {
        case -1: perror("[-] Fork failed"); return 1;
        case 0:  child_process(pid); break;
        default: parent_process(pid); break;
    }
    return 0;
}
