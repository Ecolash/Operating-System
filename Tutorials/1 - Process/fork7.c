#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void f(int n)
{
    if (n <= 0) return;
    while (n--)
    {
        printf("PID = %d, PPID = %d, n = %d\n", getpid(), getppid(), n);
        fflush(stdout);
        if (!fork()) f(n);
        else wait(NULL);
    }
}

int main()
{
    int n;
    printf("Enter n: ");
    scanf("%d", &n);
    f(n);
    return 0;
}
